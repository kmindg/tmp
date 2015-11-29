/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_bitmap.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the code to access the bitmap for the raid group 
 *   object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/27/2009:  Created. Jean Montes.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_group_bitmap.h"                      //  this file's .h file  
#include "fbe_raid_group_rebuild.h"                     //  for fbe_raid_group_rebuild_evaluate_nr_chunk_info()
#include "fbe/fbe_raid_group_metadata.h"                //  for fbe_raid_group_paged_metadata_t
#include "fbe_base_config_private.h"                    //  for paged metadata set and get chunk info
#include "fbe_raid_verify.h"                            //  for ask_for_verify_permission
#include "fbe_raid_group_logging.h"                     //  for logging rebuild complete/lun start/lun complete 
#include "fbe_raid_group_object.h"
#include "EmcPAL_Misc.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/


//  Completion function for a read of the NR info from the paged metadata 
static fbe_status_t fbe_raid_group_bitmap_get_next_nr_chunk_info_completion(
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_packet_completion_context_t     in_context);

//  Get the paged metadata for a range of chunks
static fbe_status_t fbe_raid_group_bitmap_get_paged_metadata(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_chunk_index_t                   in_start_chunk_index,
                                    fbe_chunk_count_t                   in_chunk_count);
//  Completion function for getting the paged metadata for a range of chunks
static fbe_status_t fbe_raid_group_bitmap_get_paged_metadata_completion(
                                    fbe_packet_t* in_packet_p, 
                                    fbe_packet_completion_context_t in_context);


               
//  Determine the lba range for the given chunk range
static fbe_status_t fbe_raid_group_bitmap_determine_lba_range_for_chunk_range(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_chunk_index,
                                        fbe_chunk_count_t                   in_chunk_count,
                                        fbe_lba_t*                          out_start_lba,
                                        fbe_lba_t*                          out_end_lba);  
             
static fbe_status_t fbe_raid_group_bitmap_translate_packet_status_to_block_status(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_payload_metadata_status_t       in_packet_status,                    
                                    fbe_payload_block_operation_status_t    *block_status,
                                    fbe_payload_block_operation_qualifier_t *block_qualifier);        

static fbe_status_t fbe_raid_group_bitmap_continue_metadata_update(fbe_raid_group_t* raid_group_p,
                                                                   fbe_packet_t*     packet_p,
                                                                   fbe_bool_t *more_processing_required);
static fbe_status_t fbe_raid_group_lba_to_chunk_range(fbe_raid_group_t *raid_group_p,
                                                      fbe_chunk_index_t chunk_index,
                                                      fbe_chunk_count_t chunk_count,
                                                      fbe_lba_t        *start_lba,
                                                      fbe_lba_t        *end_lba);
static fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_update_paged_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                     fbe_memory_completion_context_t context);

/***************
 * FUNCTIONS
 ***************/


/*!****************************************************************************
 * fbe_raid_group_bitmap_get_chunk_index_for_lba()
 ******************************************************************************
 * @brief
 *   This function will get the chunk index that corresponds to the given LBA.  
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_lba                - LBA for which to find the chunk index. The LBA 
 *                                is relative to the RAID extent on a single disk.
 * @param out_chunk_index_p     - pointer which gets set to the index of the chunk
 *                                for the input LBA; only valid when function returns
 *                                success 
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/28/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_chunk_index_for_lba(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           in_lba,           
                                    fbe_chunk_index_t*  out_chunk_index_p)          
{

    fbe_lba_t                       capacity_per_disk;  //  per-disk capacity of the raid group 
    fbe_chunk_size_t                chunk_size;         //  amount of data represented by a chunk 


    //  Make sure the LBA is within the per-disk capacity of the raid group 
    capacity_per_disk = fbe_raid_group_get_disk_capacity(in_raid_group_p);
    if (in_lba >= capacity_per_disk)
    {                                                               
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: LBA 0x%llx past end of RG extent: 0x%llx\n", 
                              __FUNCTION__, (unsigned long long)in_lba,
                  (unsigned long long)capacity_per_disk);
        return FBE_STATUS_GENERIC_FAILURE;                          
    }

    //  Get the chunk size
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p);

    //  Calculate the chunk index by taking the LBA and dividing by the chunk size.  The result is a whole integer 
    //  which is the chunk index. 
    *out_chunk_index_p = (fbe_chunk_count_t) (in_lba / chunk_size); 

    //  Return success
    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_get_chunk_index_for_lba()


fbe_status_t fbe_raid_group_bitmap_get_chunk_range(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           in_lba,
                                    fbe_block_count_t   in_block_count,
                                    fbe_chunk_index_t*  out_chunk_index_p,
                                    fbe_chunk_count_t*  out_chunk_count_p)
{
    fbe_status_t        status;
    fbe_chunk_size_t    chunk_size;

    //  Get the chunk index for the LBA.
    status = fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p,
                                                           in_lba, out_chunk_index_p);
    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    //  Get the chunk count corresponding to this number of blocks.
    chunk_size  = fbe_raid_group_get_chunk_size(in_raid_group_p); 
    *out_chunk_count_p = (fbe_chunk_count_t) (in_block_count / chunk_size); 
    if (in_block_count % chunk_size != 0)
    {
        (*out_chunk_count_p)++;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_raid_group_bitmap_get_journal_chunk_index_for_lba()
 ******************************************************************************
 *
 * @brief   Since the journal area is not included in the capacity, this is a
 *          special method that should only be invoked for an lba that is in
 *          the journal area.
 *
 * @param   in_raid_group_p       - pointer to a raid group object
 * @param   in_lba        - chunk index for which to find the LBA.
 * @param   out_chunk_index_p            - pointer which gets set to the LBA of the chunk
 *                                for the input index; only valid when function
 *                                returns success 

 *
 * @return  fbe_status_t  
 *
 * @author
 *  03/14/2013  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_get_journal_chunk_index_for_lba(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           in_lba,           
                                    fbe_chunk_index_t*  out_chunk_index_p)          
{

    fbe_lba_t               journal_start_lba;
    fbe_lba_t               journal_end_lba;       
    fbe_chunk_size_t        chunk_size;         //  amount of data represented by a chunk 
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);


    //  Make sure the LBA is within the journal range
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_start_lba);
    fbe_raid_geometry_get_journal_log_end_lba(raid_geometry_p, &journal_end_lba);
    if ((in_lba < journal_start_lba) ||
        (in_lba > journal_end_lba)      )
    {                                                               
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: LBA 0x%llx not in journal area start: 0x%llx end: 0x%llx\n", 
                              __FUNCTION__, (unsigned long long)in_lba,
                              (unsigned long long)journal_start_lba, (unsigned long long)journal_end_lba);
        return FBE_STATUS_GENERIC_FAILURE;                          
    }

    //  Get the chunk size
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p);

    //  Calculate the chunk index by taking the LBA and dividing by the chunk size.  The result is a whole integer 
    //  which is the chunk index. 
    *out_chunk_index_p = (fbe_chunk_count_t) (in_lba / chunk_size); 

    //  Return success
    return FBE_STATUS_OK; 

}
/**************************************************************
 * end fbe_raid_group_bitmap_get_journal_chunk_index_for_lba()
 **************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_bitmap_get_journal_chunk_range()
 ******************************************************************************
 *
 * @brief   Since the journal area is not included in the capacity, this is a
 *          special method that should only be invoked for an lba that is in
 *          the journal area.
 *
 * @param   in_raid_group_p       - pointer to a raid group object
 * @param   in_lba - chunk index for which to find the LBA.
 * @param   in_block_count - pointer which gets set to the LBA of the chunk
 * @param   out_chunk_index_p - Out chunk index
 * @param   out_chunk_count_p - out chunk count 
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *  03/14/2013  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_get_journal_chunk_range(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           in_lba,
                                    fbe_block_count_t   in_block_count,
                                    fbe_chunk_index_t*  out_chunk_index_p,
                                    fbe_chunk_count_t*  out_chunk_count_p)
{
    fbe_status_t        status;
    fbe_chunk_size_t    chunk_size;

    //  Get the chunk index for the LBA.
    status = fbe_raid_group_bitmap_get_journal_chunk_index_for_lba(in_raid_group_p,
                                                           in_lba, out_chunk_index_p);
    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    //  Get the chunk count corresponding to this number of blocks.
    chunk_size  = fbe_raid_group_get_chunk_size(in_raid_group_p); 
    *out_chunk_count_p = (fbe_chunk_count_t) (in_block_count / chunk_size); 
    if (in_block_count % chunk_size != 0)
    {
        (*out_chunk_count_p)++;
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_bitmap_get_journal_chunk_range()
 ******************************************************/

fbe_status_t
fbe_raid_group_bitmap_determine_if_lba_range_span_user_and_paged_metadata(
                                    fbe_raid_group_t *  in_raid_group_p,
                                    fbe_lba_t           start_lba,
                                    fbe_block_count_t   block_count,
                                    fbe_bool_t*         is_range_span_across_b_p)
{

    fbe_chunk_count_t       num_exported_chunks;
    fbe_chunk_index_t       start_chunk_index;
    fbe_chunk_count_t       chunk_count;
    fbe_chunk_index_t       last_chunk_index;

    //  Get the chunk range which we need to mark needs rebuild
    fbe_raid_group_bitmap_get_chunk_range(in_raid_group_p, start_lba, block_count, &start_chunk_index, &chunk_count);

    //  Get the last chunk of the range
    last_chunk_index = start_chunk_index + chunk_count  - 1;

    //  Initialize the range span across user and paged metadata as false
    *is_range_span_across_b_p = FBE_FALSE;

    //  Get the number of chunks per disk for user data area (the exported area)
    num_exported_chunks = fbe_raid_group_get_exported_chunks(in_raid_group_p);

    //  Determine if range of the chunks spans across the user and paged metadata
    if((start_chunk_index < num_exported_chunks) && 
       (last_chunk_index >= num_exported_chunks))
    {
        *is_range_span_across_b_p = FBE_TRUE;
    }

    //  Return ok status
    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_bitmap_get_lba_for_chunk_index()
 ******************************************************************************
 * @brief
 *   This function will get the LBA that corresponds to the given chunk index.  
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_chunk_index        - chunk index for which to find the LBA.
 * @param out_lba_p             - pointer which gets set to the LBA of the chunk
 *                                for the input index; only valid when function
 *                                returns success 

 *
 * @return  fbe_status_t  
 *
 * @author
 *   111/17/2009 - Created. MEV
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_lba_for_chunk_index(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_chunk_index_t   in_chunk_index,
                                    fbe_lba_t*          out_lba_p)          
{
    fbe_chunk_size_t                chunk_size;         // amount of data represented by a chunk 
    fbe_chunk_count_t               total_chunks;       // total number of chunks in the bitmap

    //  Make sure the chunk index is within the raid group extent
    total_chunks = fbe_raid_group_get_total_chunks(in_raid_group_p);

    if (in_chunk_index >= total_chunks)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: chunk index:0x%llx greater than total chunks: 0x%llx\n", 
                              __FUNCTION__, (unsigned long long)in_chunk_index,
                  (unsigned long long)total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Calculate the LBA by multiplying the chunk index by the chunk size
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p);
    *out_lba_p = (fbe_lba_t) (in_chunk_index * chunk_size); 

    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_get_lba_for_chunk_index()


/*!****************************************************************************
 * fbe_raid_group_bitmap_read_chunk_info_for_range()
 ******************************************************************************
 * @brief
 *   This function will read the chunk info for a range of chunks.  It will 
 *   determine if the range is inside or outside of the user data area.  Then it 
 *   will read the chunk info from either the paged metadata or the non-paged 
 *   metadata accordingly.
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to the packet
 * @param in_start_chunk_index      - index of the first chunk to read 
 * @param in_chunk_count            - number of chunks to read
 *
 * @return fbe_status_t  
 *
 * @author
 *   07/20/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_read_chunk_info_for_range(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       in_packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index, 
                                        fbe_chunk_count_t                   in_chunk_count)
{
    fbe_bool_t                          is_in_data_area_b;                  // true if chunk is in the data area
    fbe_status_t                        status;                             // fbe status


    //  Determine if the chunk(s) to be read are in the user data area or the paged metadata area.  If we want
    //  to read a chunk in the user area, we use the paged metadata service to do that.  If we want to read a 
    //  chunk in the paged metadata area itself, we use the nonpaged metadata to do that (in the metadata data
    //  portion of it).
    status = fbe_raid_group_bitmap_determine_if_chunks_in_data_area(in_raid_group_p, in_start_chunk_index,
                                                                    in_chunk_count, &is_in_data_area_b);

    //  Check for an error on the call, which means there is an issue with the chunks.  If so, we will complete 
    //  the packet and return.  
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    //  If the chunk is not in the data area, then use the non-paged metadata service to read it
    if (is_in_data_area_b == FBE_FALSE)
    {
        fbe_raid_group_bitmap_read_chunk_info_using_nonpaged(in_raid_group_p, in_packet_p, in_start_chunk_index,
                                                             in_chunk_count);
        return FBE_STATUS_OK;
    }

    //  The chunks are in the user data area.  Use the paged metadata service to read them. 
    fbe_raid_group_bitmap_get_paged_metadata(in_raid_group_p, in_packet_p, in_start_chunk_index, in_chunk_count);

    //  Return success
    return FBE_STATUS_OK;

} // End fbe_raid_group_bitmap_read_chunk_info_for_range()


/*!****************************************************************************
 * fbe_raid_group_bitmap_determine_if_chunks_in_data_area()
 ******************************************************************************
 * @brief
 *   This function will determine if the given chunk(s) are in the RAID group's
 *   user data area or in its paged metadata.  
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_start_chunk_index      - index of the first chunk  
 * @param in_chunk_count            - number of chunks 
 * @param out_is_in_data_area_b_p   - pointer to data that gets set to true if
 *                                    the range of chunks is in the RG's data
 *                                    area; false if the chunks are in the 
 *                                    paged metadata area; invalid on error
 *
 * @return fbe_status_t  
 *
 * @author
 *   06/16/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_determine_if_chunks_in_data_area(
                                        fbe_raid_group_t*               in_raid_group_p,
                                        fbe_chunk_index_t               in_start_chunk_index, 
                                        fbe_chunk_count_t               in_chunk_count,
                                        fbe_bool_t*                     out_is_in_data_area_b_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_chunk_count_t       num_exported_chunks;            // num chunks of the exported area
    fbe_chunk_index_t       last_chunk_index;               // chunk index of the last chunk to update
    fbe_chunk_count_t       total_chunks;                   // num chunks of the configured area
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_lba_t               journal_start_lba;

    //  Initialize output parameter to "not in the user data area"
    *out_is_in_data_area_b_p = FBE_FALSE;

    /* Don't use paged for a raw mirror.  The metadata service does not know how to deal with 
     * the raw data payload. 
     */
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        return FBE_STATUS_OK;
    }

    /* Don't check a request to the journal area.  It is not to user space
     */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_start_lba);
    if (journal_start_lba != FBE_LBA_INVALID)
    {
        fbe_lba_t           journal_end_lba;
        fbe_block_count_t   journal_blocks;
        fbe_chunk_index_t   journal_start_chunk_index;
        fbe_chunk_count_t   journal_chunk_count;

        fbe_raid_geometry_get_journal_log_end_lba(raid_geometry_p, &journal_end_lba);
        journal_blocks = journal_end_lba - journal_start_lba + 1;
        status = fbe_raid_group_bitmap_get_journal_chunk_range(in_raid_group_p, journal_start_lba, journal_blocks, 
                                              &journal_start_chunk_index, &journal_chunk_count);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if ((in_start_chunk_index + in_chunk_count) > (journal_start_chunk_index + journal_chunk_count))
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s: chunk index: %lld plus count: 0x%x is beyond journal space \n",  
                                  __FUNCTION__, (unsigned long long)in_start_chunk_index, in_chunk_count);
            return FBE_STATUS_GENERIC_FAILURE;       
        }

        /* If the request is in the journal space then it is not in the 
         * user space.
         */
        if (in_start_chunk_index >= journal_start_chunk_index)
        {
            return FBE_STATUS_OK;
        }
    }

    //  Make sure the chunk count is valid 
    if (in_chunk_count == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: chunk count is 0\n",  
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    //  Get the total number of chunks in the RG - this includes the chunks in the data area and the paged metadata area
    //  ie. it is the number of chunks in the configured capacity 
    total_chunks = fbe_raid_group_get_total_chunks(in_raid_group_p);

    //  Get the number of chunks per disk in the user RG data area (the exported area)
    num_exported_chunks = fbe_raid_group_get_exported_chunks(in_raid_group_p);

    //  Calculate the last chunk that we want to access 
    last_chunk_index = in_start_chunk_index + in_chunk_count - 1;

    //  Make sure the chunks requested are all within the RG  
    if (last_chunk_index >= total_chunks) 
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunks 0x%llx-0x%llx are past end of configured capacity\n", 
             __FUNCTION__, (unsigned long long)in_start_chunk_index,
         (unsigned long long)last_chunk_index);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    //  The chunk range is valid

    //  If the last chunk is within the RAID group data area, then all of the chunks are within the area - set the
    //  output to true and return
    if (last_chunk_index < num_exported_chunks)
    {
        *out_is_in_data_area_b_p = FBE_TRUE;
        return FBE_STATUS_OK; 
    }

    //  The last chunk is in the paged metadata.  If the whole range is within the paged metadata, then 
    //  just return - the output parameter has already been set to false
    if (in_start_chunk_index >= num_exported_chunks)
    {
        return FBE_STATUS_OK; 
    }

    //  The chunks span the user data and paged metadata.  This is not allowed; the caller needs to break up the
    //  request into a paged area request and a non-paged area request. 
    fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunks 0x%llx-0x%llx span data and paged MD regions\n", 
        __FUNCTION__, (unsigned long long)in_start_chunk_index,
    (unsigned long long)last_chunk_index);
    return FBE_STATUS_GENERIC_FAILURE; 

} // End fbe_raid_group_bitmap_determine_if_chunks_in_data_area()


#if 0
/*!****************************************************************************
 * fbe_raid_group_bitmap_mark_verify_for_chunk()
 ******************************************************************************
 * @brief
 *   This function allocates a block operation to get the stripe lock
 *   for the first chunk of a lun. Once the stripe lock is acquired
 *   non paged and paged metadata will be updated for verify
 * 
 * 
 * @param in_packet_p           - pointer to the packet
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_chunk_index        - the index of the chunk to mark 
 * @param in_verify_type        - the type of verify operation
 * @param in_chunk_count        - number of chunks to mark verify
 *
 * @return  fbe_status_t  
 *
 * @author
 *   11/12/2009 - Created. MEV
 *   04/16/2010 - Modified Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_mark_verify_for_chunk(
                                            fbe_packet_t*               in_packet_p,
                                            fbe_raid_group_t*           in_raid_group_p,
                                            fbe_chunk_index_t           in_chunk_index,
                                            fbe_raid_verify_type_t      in_verify_type,
                                            fbe_chunk_count_t           in_chunk_count)          
{
    fbe_lba_t                              verify_lba;
    fbe_block_count_t                      block_count;


    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p,in_chunk_index, &verify_lba);
    block_count = fbe_raid_group_get_chunk_size(in_raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                         "%s marking verify type %d for %d chunks from lba 0x%x.\n",
                         __FUNCTION__, in_verify_type, in_chunk_count, (unsigned int)verify_lba);   

    fbe_raid_group_get_NP_lock(in_raid_group_p, in_packet_p, fbe_raid_group_mark_verify);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    
} // End fbe_raid_group_bitmap_mark_verify_for_chunk()
#endif

/*!****************************************************************************
 * fbe_raid_group_bitmap_paged_metadata_block_op_completion()
 ******************************************************************************
 * @brief
 *   This function is called when paged metadata operation that was initiated for
 * a block operation has completed.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/10/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_paged_metadata_block_op_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_raid_group_t                        *raid_group_p;
    fbe_payload_ex_t                        *sep_payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_status_t                            status;
    fbe_payload_block_operation_opcode_t    block_opcode;

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context;   

    //  Get the status of the metadata operation 
    sep_payload_p           = fbe_transport_get_payload_ex(in_packet_p);

    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    status = fbe_transport_get_status_code(in_packet_p);

    /* First, get the opcodes.
     */
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);
    fbe_raid_group_bitmap_translate_packet_status_to_block_status(raid_group_p, status, 
                                                                  &block_status, &block_qualifier);
    fbe_payload_block_set_status(block_operation_p, block_status, block_qualifier);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_md_block_op op:%d status:0x%x bl:0x%x blq:0x%x\n",
                              block_opcode, status, block_status, block_qualifier);
    }
    return FBE_STATUS_OK;
} // End fbe_raid_group_bitmap_paged_metadata_block_op_completion()


/*!****************************************************************************
 * fbe_raid_group_bitmap_set_pmd_err_vr_completion()
 ******************************************************************************
 * @brief
 *   This function is called when pmd write for error verify has completed.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/09/2010 - Created. Ashwin Tamilarasan
 *   01/06/2012 - Ashok Tamilarasan - Call paged metadata of metadata verify
 *
 ******************************************************************************/

fbe_status_t fbe_raid_group_bitmap_set_pmd_err_vr_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_raid_group_t*                      raid_group_p;          
    fbe_status_t                           status;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_metadata_operation_t       *metadata_operation_p = NULL;    

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Release the metadata operation
    sep_payload_p           = fbe_transport_get_payload_ex(in_packet_p);
    metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);    

    status = fbe_transport_get_status_code(in_packet_p);

    if(status != FBE_STATUS_OK)
    {
        status = FBE_STATUS_FAILED;
        fbe_transport_set_status(in_packet_p, status, 0);
        return status;
    }

    return FBE_STATUS_OK;
} // End fbe_raid_group_bitmap_set_pmd_err_vr_completion()


/*!****************************************************************************
 * fbe_raid_group_bitmap_mark_metadata_of_metadata_verify()
 ******************************************************************************
 * @brief
 *   This function will mark verify on the entire metadata of metadata
 * 
 * @param in_packet_p   
 * @param raid_group_p   
 * @param verify_flags
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   01/06/2012 - Created. Ashok Tamilarasan
 *   03/16/2012 - Modified for verify_flags argument. Prahlad Purohit 
 * 
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_mark_metadata_of_metadata_verify(fbe_packet_t*           in_packet_p,
                                                                    fbe_raid_group_t*       raid_group_p,
                                                                    fbe_raid_verify_flags_t verify_flags)
{
    fbe_u64_t           metadata_offset;
    fbe_chunk_index_t   mdd_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_u32_t           mdd_chunk_count = 0;
    fbe_raid_group_paged_metadata_t chunk_data;     
    fbe_payload_ex_t *                       payload_p;
    fbe_payload_block_operation_t *          block_operation_p;
    fbe_chunk_index_t paged_md_md_start_chunk_index, paged_md_md_end_chunk_index;
    fbe_chunk_index_t start_chunk, end_chunk;    
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* For raw mirror RG we mark the chunks.
         */ 
        payload_p = fbe_transport_get_payload_ex(in_packet_p);
        block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);

        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, block_operation_p->lba, &start_chunk);
        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, (block_operation_p->lba + block_operation_p->block_count - 1), &end_chunk);
        fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(raid_group_p, 
                                                                            start_chunk, 
                                                                            &paged_md_md_start_chunk_index);
        fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(raid_group_p, 
                                                                            end_chunk, 
                                                                            &paged_md_md_end_chunk_index);
        mdd_chunk_index = start_chunk;
        mdd_chunk_count = (fbe_u32_t)(end_chunk - start_chunk + 1);
    }
    else
    {
        /* For simiplicity we will mark entire MD region for verify.
         */ 
        mdd_chunk_index = 0;
        
        /* Even though the verify is intiated on a per LUN basis, just for 
         * simplicity sake we are marking the entire Raid Group's paged metadata 
         * for verify
         */
        fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &mdd_chunk_count);
    }

    //  Set up the bits of the metadata that need to be written, which are the verify bits 
    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_data.verify_bits = verify_flags;

    //  Set up the offset of the starting chunk in the metadata 
    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->paged_metadata_metadata[mdd_chunk_index]);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s:Mark verify:%d, Start: %d Count:%d\n",
                          __FUNCTION__, verify_flags, 
                          (int)mdd_chunk_index, mdd_chunk_count);

    fbe_base_config_metadata_nonpaged_set_bits_persist((fbe_base_config_t*) raid_group_p, in_packet_p,
                                                       metadata_offset, (fbe_u8_t*) &chunk_data, 
                                                       sizeof(fbe_raid_group_paged_metadata_t), 
                                                       mdd_chunk_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}// End fbe_raid_group_bitmap_mark_metadata_of_metadata_verify


/*!****************************************************************************
 * fbe_raid_group_bitmap_mark_mdd_verify_block_op()
 ******************************************************************************
 * @brief
 *   This function will mark verify on metadata of metadata using control 
 *   operation.
 * 
 * @param in_packet_p   
 * @param raid_group_p   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   03/16/2012 - Created. Prahlad Purohit
 * 
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_bitmap_mark_mdd_verify_block_op(fbe_packet_t* in_packet_p,
                                               fbe_packet_completion_context_t in_context)
{
    fbe_status_t                            status;
    fbe_raid_verify_flags_t                 verify_flags;
    fbe_raid_group_t *                      raid_group_p;

    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;

    payload_p           = fbe_transport_get_payload_ex(in_packet_p);
    
    if(payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
    }
    else
    {
        block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);
    }

    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    raid_group_p = (fbe_raid_group_t *)in_context;

    status = fbe_transport_get_status_code(in_packet_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:Non paged metadata write failed\n", __FUNCTION__);
        return status;
    }
    /* Set the block operation status to good since we might not even fill it in if we are not 
     * going to write the paged also. 
     */
    fbe_payload_block_set_status(block_operation_p, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_flags);

    status = fbe_raid_group_bitmap_mark_metadata_of_metadata_verify(in_packet_p,
                                                                    raid_group_p,
                                                                    verify_flags);
    return status;
} // End fbe_raid_group_bitmap_mark_mdd_verify_block_op()


#if 0
/*!****************************************************************************
 * fbe_raid_group_mark_verify()
 ******************************************************************************
 * @brief
 *   This function will initiate error verify
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   11/01/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_mark_verify(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                    raid_group_p = NULL;   
    fbe_status_t                         status;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
 
    /* This is needed to avoid a deadlock. The usurper or monitor gets the NP lock
      and tries to mark a chunk for verify. The raid group is degraded. Since it is degraded
      first it will try to mark NR on the md of md which will try to get the NP lock and it wont get
      it since the verify has the lock. 
    */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NP_LOCK_HELD);

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    status = fbe_raid_group_bitmap_initiate_non_paged_metadata_update(packet_p, raid_group_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_raid_group_mark_verify()
#endif


/*!****************************************************************************
 * fbe_raid_group_bitmap_initiate_reinit_metadata()
 ******************************************************************************
 * @brief
 *   This function starts the process of initializing the metadata for the LUN
 * extent
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param in_context - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   09/12/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_initiate_reinit_metadata(
                                    fbe_packet_t*                   packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_raid_group_t*                       raid_group_p;
    fbe_status_t                            status;
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_raid_group_paged_metadata_t         paged_set_bits;
    fbe_chunk_count_t                       chunk_count;
    fbe_chunk_index_t                       chunk_index;
    
    payload_p           = fbe_transport_get_payload_ex(packet_p);

    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    /* Initialize the bit data with 0 */
    fbe_set_memory(&paged_set_bits, 0, sizeof(fbe_raid_group_paged_metadata_t));

     // Convert the lun start lba and block count into per disk lba and chunk count
    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          block_operation_p->lba,
                                                          block_operation_p->block_count,
                                                          &chunk_index, 
                                                          &chunk_count);

    /* Since we want to reinit all the fields, we first set the fields we want to clear */
    paged_set_bits.verify_bits = FBE_RAID_BITMAP_VERIFY_ALL;
    paged_set_bits.needs_rebuild_bits = FBE_RAID_POSITION_BITMASK_ALL_SET;

    fbe_transport_set_completion_function(packet_p, 
                                         fbe_raid_group_bitmap_paged_metadata_block_op_completion,
                                         raid_group_p); 

    status = fbe_raid_group_bitmap_update_paged_metadata(raid_group_p,
                                                         packet_p,
                                                         chunk_index,
                                                         chunk_count,
                                                         &paged_set_bits,
                                                         FBE_TRUE); /* this is a clear op */

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_bitmap_initiate_reinit_metadata()

#if 0
/*!****************************************************************************
 * fbe_raid_group_bitmap_initiate_non_paged_metadata_update()
 ******************************************************************************
 * @brief
 *   This function is called when stripe lock has been acquired.
 *   This will call the non paged metadata update to update the checkpoint   
 * 
 * @param packet_p   
 * @param raid_group_p   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/09/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_initiate_non_paged_metadata_update(fbe_packet_t *packet_p,
                                                                      fbe_raid_group_t*               raid_group_p)
{
      
    fbe_chunk_index_t                        chunk_index;    
    fbe_u64_t                                metadata_offset = FBE_LBA_INVALID; 
    fbe_chunk_count_t                        chunk_count;
    fbe_raid_verify_flags_t                  verify_flags;
    fbe_lba_t                                paged_md_start_lba = FBE_LBA_INVALID;
    fbe_lba_t                                paged_md_capacity = FBE_LBA_INVALID;
    fbe_status_t                             status;
    fbe_packet_completion_function_t         completion_function;
    fbe_lba_t                                start_lba;
    fbe_block_count_t                        block_count;
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    payload_p           = fbe_transport_get_payload_ex(packet_p);
    if(payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
    }
    else
    {
        block_operation_p   = fbe_payload_ex_get_block_operation(payload_p);
    }


    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    
    // Depending upon the type of verify update the corresponding verify checkpoint in the 
    // non paged metadata and set the corresponding completion function     
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, 
                                                               block_opcode,
                                                               &verify_flags);

    status = fbe_raid_group_metadata_get_verify_chkpt_offset(raid_group_p, verify_flags, &metadata_offset);
    if (status != FBE_STATUS_OK)
    {
         fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Unknown verify flag: 0x%x status: 0x%x\n", __FUNCTION__, verify_flags, status);
         fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
         return FBE_STATUS_OK;
    }


    // Get paged metadata start lba and its capacity.
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config),
                                                         &paged_md_capacity);

    completion_function = fbe_raid_group_bitmap_mark_paged_metadata_verify;

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_chunk_index_t paged_md_md_start_chunk_index, paged_md_md_end_chunk_index;
        fbe_chunk_count_t chunks_per_md_of_md;
        fbe_chunk_index_t end_chunk;    

        /* Only update non-paged, no paged.  Just verify entire rg since it is so small.
         */
        completion_function = fbe_raid_group_bitmap_mark_mdd_verify_block_op;
        //start_lba = 0;
        //block_count = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, block_operation_p->lba, &chunk_index);
        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, (block_operation_p->lba + block_operation_p->block_count - 1), &end_chunk);
        fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(raid_group_p, 
                                                                            chunk_index, 
                                                                            &paged_md_md_start_chunk_index);
        fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(raid_group_p, 
                                                                            end_chunk, 
                                                                            &paged_md_md_end_chunk_index);
        fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(raid_group_p, &chunks_per_md_of_md);
        chunk_index = paged_md_md_start_chunk_index * chunks_per_md_of_md;
        chunk_count = (fbe_chunk_count_t)(paged_md_md_end_chunk_index - paged_md_md_start_chunk_index + 1) * chunks_per_md_of_md;
    }
    else if(!fbe_raid_group_block_opcode_is_user_initiated_verify(block_opcode))
    {
        /* Need to mark the paged region for verify only if lba is in the paged region */
        if(block_operation_p->lba >= paged_md_start_lba)
        {
            /* We can directly update the NP region after the checkpoint is updated. */
            completion_function = fbe_raid_group_bitmap_mark_mdd_verify_block_op;
        }
        start_lba = block_operation_p->lba;
        block_count = block_operation_p->block_count;
        fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                              start_lba,
                                                              block_count,
                                                              &chunk_index, 
                                                              &chunk_count);
    }
    else
    {
        /* Any other kind of user initiated verify(R0 or RW), need to mark the Paged Metadata region 
         * also because there is no other external interface available for user to kick off verify on MD region
         * so set a completion function that gets called after the paged region is marked
         */
        fbe_transport_set_completion_function(packet_p, 
                                              fbe_raid_group_bitmap_mark_mdd_verify_block_op,
                                              raid_group_p);

        /* Since this is user intiated, set the start lba to paged MD because we first perform the verify
         * of paged region and then wrap around and do the user region
         */
        start_lba = paged_md_start_lba;
        block_count = (fbe_block_count_t)paged_md_capacity;
        fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                              start_lba,
                                                              block_count,
                                                              &chunk_index, 
                                                              &chunk_count);
    }

    status = fbe_raid_group_bitmap_set_non_paged_metadata_for_verify(packet_p,raid_group_p,
                                                                     chunk_index, chunk_count,
                                                                     completion_function,
                                                                     metadata_offset,
                                                                     verify_flags);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}// End fbe_raid_group_bitmap_initiate_non_paged_metadata_update
#endif


/*!**************************************************************
 * fbe_raid_group_allocate_block_operation_and_send_to_executor()
 ****************************************************************
 * @brief
 *  This function builds a block operation in the packet and sends it
 *  to the executor
 *  
 *
 * @param in_raid_group_p   - Pointer to the raid group
 * @param packet_p       - Pointer to the packet
 * @param in_opcode         - Block operation code
 * @param in_verify_lba     - The block's start address
 * @param in_block_count    - The block count
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  11/15/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_allocate_block_operation_and_send_to_executor(
                                fbe_raid_group_t*                       in_raid_group_p,
                                fbe_packet_t*                           packet_p,
                                fbe_lba_t                               in_verify_lba,
                                fbe_block_count_t                       in_block_count,
                                fbe_payload_control_operation_opcode_t  in_opcode)
                                
{
    fbe_payload_ex_t*                      payload_p;
    fbe_block_size_t                        optimal_block_size;
    fbe_payload_block_operation_t*          block_operation_p;    
    fbe_status_t                            status;
    fbe_raid_geometry_t*                    raid_geometry_p;
    

    // Get the SEP payload
    payload_p = fbe_transport_get_payload_ex(packet_p);

    // Setup the block operation in the packet.
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
        
    fbe_payload_block_build_operation(block_operation_p,
                                        in_opcode,
                                        in_verify_lba,
                                        in_block_count,
                                        FBE_BE_BYTES_PER_BLOCK,
                                        optimal_block_size,
                                        NULL);    

    // Increment the operation level
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s monitor I/O operation failed, status: 0x%X\n",
                        __FUNCTION__, status);
        fbe_transport_complete_packet(packet_p);
        fbe_transport_set_status(packet_p, status, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Send the packet to the RAID executor.
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)in_raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}// End fbe_raid_group_allocate_block_operation_and_send_to_executor()

/*!****************************************************************************
 * fbe_raid_group_bitmap_set_non_paged_metadata_for_verify()
 ******************************************************************************
 * @brief
 *  This function updates the non paged metadata verify checkpoint using the 
 *  metadata service 
 *
 * @param packet_p       - The pointer to the packet.
 * @param in_raid_group_p   - The pointer to the raid group.
 * @param in_chunk_index    - Chunk index to calculate the new checkpoint
 * @param in_chunk_count    - chunk count
 * @param completion function
 * @param metadata_offset   - metadaoffset for the respective checkpoint
 * @param in_verify_flag    - type of verify
 *
 * @return fbe_status_t
 *
 * @author
 *  07/16/2010 - Created. Ashwin Tamilarasan
 *  03/29/2012 - Modified for reconstruct checkpoint - Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_set_non_paged_metadata_for_verify(fbe_packet_t*   packet_p,
                                                              fbe_raid_group_t* in_raid_group_p,
                                                              fbe_chunk_index_t in_chunk_index,
                                                              fbe_chunk_count_t in_chunk_count,
                                                              fbe_packet_completion_function_t completion_function,
                                                              fbe_u64_t metadata_offset,
                                                              fbe_raid_verify_flags_t in_verify_flag)

{
    fbe_lba_t                               new_checkpoint;
    fbe_lba_t                               current_checkpoint = FBE_LBA_INVALID;
    fbe_status_t                            status;
    fbe_lba_t                               paged_md_start_lba = FBE_LBA_INVALID;
    fbe_raid_geometry_t                     *raid_geometry_p = NULL;
    fbe_u16_t                               data_disks = 0;
    fbe_bool_t                              is_paged_lba = FBE_FALSE;
  

    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p, in_chunk_index, &new_checkpoint);
          
    if (completion_function != NULL)
    {
        fbe_transport_set_completion_function(packet_p,
                                              completion_function,
                                              in_raid_group_p);
    }


    current_checkpoint = fbe_raid_group_verify_get_checkpoint(in_raid_group_p, in_verify_flag);

    /* If the existing checkpoint is invalid, then call persist checkpoint api to 
       persist the new checkpoint. We will only persist the checkpoint if we are
       going from invalid to a valid value or changing from valid to invalid
    */
    if(FBE_LBA_INVALID == current_checkpoint)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "RG psist vr chkpt: %llx/%llx fl: %d idx/cnt:%lld/%d pkt: %p\n",
                              (unsigned long long)new_checkpoint, (unsigned long long)current_checkpoint,  
                              in_verify_flag, (long long)in_chunk_index, in_chunk_count, packet_p);
        fbe_base_config_metadata_nonpaged_set_checkpoint_persist((fbe_base_config_t *)in_raid_group_p,
                                                                  packet_p,
                                                                  metadata_offset,
                                                                  0, /* There is only (1) checkpoint for verify.*/
                                                                  0);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    fbe_base_config_metadata_get_paged_record_start_lba(&(in_raid_group_p->base_config),
                                                        &paged_md_start_lba);

    paged_md_start_lba /= data_disks;

    /* We no longer reset the error_verify checkpoint on every error, since that was preventing high lbas from 
     * being verified, but instead, let the checkpoint continue and set the error_verify_required flag.
     */
    if (in_verify_flag & FBE_RAID_BITMAP_VERIFY_ERROR)
    {
        if (!fbe_raid_group_metadata_is_error_verify_required(in_raid_group_p))
        {
            /* Flag is not already set, so trace and set it now
             */
            fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "RG set vr - set error_verify_required flag\n");
            status = fbe_raid_group_metadata_set_error_verify_required(in_raid_group_p, packet_p);
        }
        else
        {
            /* Flag is already set, no need to do anything but trace and complete the packet back to the previous level
             */
            fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                 "RG set vr - error_verify_required flag already set\n");
            fbe_transport_complete_packet(packet_p);
        }
    }
    else if (in_verify_flag & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)
    {
        if (paged_md_start_lba <= new_checkpoint) {
            is_paged_lba = FBE_TRUE;
        }

        if (!fbe_raid_group_metadata_is_iw_verify_required(in_raid_group_p))
        {
            /* Flag is not already set, so trace and set it now
             */
            fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "RG set vr - set iw_verify_required flag. is_paged_md: 0x%x\n", 
                                  is_paged_lba);
            status = fbe_raid_group_metadata_set_iw_verify_required(in_raid_group_p, packet_p);
        }
        else
        {
            /* Flag is already set, no need to do anything but trace and complete the packet back to the previous level
             */
            fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                 "RG set vr - iw_verify_required flag already set. is_paged_md: 0x%x\n", 
                                 is_paged_lba);
            fbe_transport_complete_packet(packet_p);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "RG set vr chkpt: %llx/%llx fl: %d idx/cnt:%lld/%d pkt: %p\n",
                              (unsigned long long)new_checkpoint, (unsigned long long)current_checkpoint,  
                              in_verify_flag, (long long)in_chunk_index, in_chunk_count, packet_p);
        /* We want to use force set checkpoint so that the checkpoint is communicated to the peer. It is possible 
         * that peer is in middle of verifying the paged metadata but we want to start again and so use force set 
         * instead of a normal set
         */
        status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)in_raid_group_p,
                                                                        packet_p,
                                                                        metadata_offset,
                                                                        0, /* There is only (1) checkpoint for a verify */
                                                                        0 /* always 0 */);
    }

    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p,
                         FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "rg_bmp_set_non_paged_mdata_4_verify: Non paged set check point fail\n");
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}// End fbe_raid_group_bitmap_set_non_paged_metadata_for_verify


/*!****************************************************************************
 * fbe_raid_group_bitmap_mark_paged_metadata_verify()
 ******************************************************************************
 * @brief
 *   This function is called when the non-paged metadata write to set the 
 *   verify checkpoint has completed.  It will mark the paged metadata for
 *   verify.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param in_context - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   07/16/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_mark_paged_metadata_verify(
                                    fbe_packet_t*                   packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_raid_group_t                       *raid_group_p;
    fbe_status_t                            packet_status; 
    fbe_status_t                            status;
    fbe_chunk_count_t                       chunk_count;
    fbe_chunk_index_t                       chunk_index;
    fbe_raid_verify_flags_t                 verify_flag;
    fbe_payload_ex_t                       *payload_p           = NULL;
    fbe_payload_block_operation_t          *block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_group_paged_metadata_t         paged_set_bits;
    fbe_lba_t                               current_checkpoint;
    fbe_raid_group_debug_flags_t            trace_flags;

    payload_p           = fbe_transport_get_payload_ex(packet_p);

    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);

    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 
    
    packet_status = fbe_transport_get_status_code(packet_p);

    if(packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                         FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s:Non paged metadata write failed\n", __FUNCTION__);
        return FBE_STATUS_OK;
        
    }

    // Convert the lun start lba and block count into per disk lba and chunk count
    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          block_operation_p->lba,
                                                          block_operation_p->block_count,
                                                          &chunk_index, 
                                                          &chunk_count);

    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, 
                                                       block_opcode,
                                                       &verify_flag);

    current_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, verify_flag);

    /* We need to suppress this trace for these verify types which can be very frequent 
     * even one per I/O in some cases. 
     */
    if ((verify_flag & FBE_RAID_BITMAP_VERIFY_ERROR) ||
        (verify_flag & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)) {
        trace_flags = FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING;
    }
    else{
        trace_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
    }
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, trace_flags,
                         "RG mark vr paged fl:0x%x c_idx/c:0x%llX/0x%X pkt: %p cp: 0x%llx\n",
                         verify_flag, (unsigned long long)chunk_index, chunk_count, packet_p, (unsigned long long)current_checkpoint);

    /* Initialize the bit data with 0 */
    fbe_set_memory(&paged_set_bits, 0, sizeof(fbe_raid_group_paged_metadata_t));

    paged_set_bits.verify_bits = verify_flag;

    fbe_transport_set_completion_function(packet_p, 
                                         fbe_raid_group_bitmap_paged_metadata_block_op_completion,
                                         raid_group_p); 

    status = fbe_raid_group_bitmap_update_paged_metadata(raid_group_p,
                                                         packet_p,
                                                         chunk_index,
                                                         chunk_count,
                                                         &paged_set_bits,
                                                         FBE_FALSE); /* this is a set op */

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_bitmap_mark_paged_metadata_verify()


/*!***************************************************************************
 *          fbe_raid_group_bitmap_metadata_populate_stripe_lock()
 ***************************************************************************** 
 * 
 * @brief   Populate the stripe lock required for a metadata request.  The
 *          metadata service will handle the stripe locking.  For a raid group
 *          a metadata stripe is the same as a user area stripe.
 * 
 * @param   raid_group_p - Pointer to raid group
 * @param   metadata_operation_p - Pointer to metadata request (offset set)
 * @param   chunk_count - Use to set stripe range
 * 
 * @return  fbe_status_t
 * 
 * @todo    Use chunk_count to determine if lock range exceeds the metadata
 *          service capabilities.
 *
 * @author
 *  04/07/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_bitmap_metadata_populate_stripe_lock(fbe_raid_group_t *raid_group_p,
                                                                 fbe_payload_metadata_operation_t *metadata_operation_p,
                                                                 fbe_chunk_count_t chunk_count)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t           metadata_start_lba;
    fbe_block_count_t   metadata_block_count;
    fbe_u64_t           metadata_stripe_number;
    fbe_u64_t           metadata_stripe_count;
    fbe_element_size_t element_size;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_bm_md_populate_sl: Accessing paged metadata unexpectedly.");
    }

    /*! Get the metadata starting lba and number of blocks.  This
     *  assumes that the metdata.
     *  
     *  @todo Add a check for exceeding the maximum metadata blocks
     *        of 0x2000000.
     */
    FBE_UNREFERENCED_PARAMETER(chunk_count);
    status = fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s get lock range failed with status: 0x%x", 
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The metadata is accessed the same as user data.  That is
     * logically.
     */
    fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                           metadata_start_lba,
                                           metadata_block_count,
                                           &metadata_stripe_number,
                                           &metadata_stripe_count);

    /* Now simply set the stripe number and count in the metadata request
     */
    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_stripe_number);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_stripe_count);

    return FBE_STATUS_OK;
}
// end fbe_raid_group_bitmap_metadata_populate_stripe_lock


/*!****************************************************************************
 * fbe_raid_group_bitmap_clear_verify_for_chunk()
 ******************************************************************************
 * @brief
 *   This function clears all verify bits in a chunk and persists it.
 * 
 * 
 * @param packet_p           - pointer to the packet 
 * @param in_raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   11/18/2009 - Created. MEV
 *   04/15/2010 - Modified Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_clear_verify_for_chunk(
                                            fbe_packet_t*          packet_p,    
                                            fbe_raid_group_t*      in_raid_group_p)
{
    fbe_raid_group_paged_metadata_t         paged_clear_bits;
    fbe_status_t                            status;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_verify_flags_t                 verify_flags;
    fbe_chunk_count_t                       chunk_count;
    fbe_payload_block_operation_t           *block_operation_p;
    fbe_lba_t                               start_lba; 
    fbe_lba_t                               end_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_index_t                       end_chunk_index;    
    fbe_raid_iots_t*                        iots_p;

    sep_payload_p        = fbe_transport_get_payload_ex(packet_p);    
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);  

    start_lba = block_operation_p->lba;
    block_opcode = block_operation_p->block_opcode;

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine how many marks to clear. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    end_lba = start_lba + block_count - 1;

    // Clear all the verify bits in this chunk
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, start_lba, &start_chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, end_lba, &end_chunk_index);    

    //Convert the opcode into verify flags
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(in_raid_group_p, block_opcode, &verify_flags);

    fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                         "clear verify bits: 0x%x chunk %llx to %llx\n", 
                         verify_flags, (unsigned long long)start_chunk_index, (unsigned long long)end_chunk_index);

    // Setup the completion function to be called when the paged metadata write completes
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_bitmap_paged_metadata_block_op_completion,
                                          in_raid_group_p);  

    // Initialize the bit data with 0 before read operation 
    fbe_set_memory(&paged_clear_bits, 0, sizeof(fbe_raid_group_paged_metadata_t));

    chunk_count = (fbe_chunk_count_t)(end_chunk_index - start_chunk_index) + 1;
    // To clear all the verify bits OR all the verify bits which is equivalent to 0xf  
    verify_flags =  FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE
                    | FBE_RAID_BITMAP_VERIFY_ERROR
                    | FBE_RAID_BITMAP_VERIFY_SYSTEM 
                    | FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE
                    | FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
    paged_clear_bits.verify_bits = verify_flags;
 
    status = fbe_raid_group_bitmap_update_paged_metadata(in_raid_group_p,
                                                         packet_p,
                                                         start_chunk_index,
                                                         chunk_count,
                                                         &paged_clear_bits,
                                                         FBE_TRUE); /* this is a clear op */
    return status; 
} // End fbe_raid_group_bitmap_clear_verify_for_chunk()

/*!****************************************************************************
 * fbe_raid_group_verify_get_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function checks the status of the metadata operation to determine whether
 *   to proceed with the verify or not. If the verify flag is set then it proceeds
 *   with the verify. If the verify is not set it just updates the respective
 *   verify checkpoint
 * 
 * @param packet_p              - pointer to the packet
 * @param context                  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *   10/28/2010 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_get_paged_metadata_completion(fbe_packet_t*  packet_p,
                                                          fbe_packet_completion_context_t context)
{

    fbe_raid_group_t*                        raid_group_p;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;
    fbe_raid_iots_t*                        iots_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_payload_metadata_status_t           metadata_status;    
    fbe_raid_verify_flags_t                 verify_type;
    fbe_lba_t                               start_lba;
    fbe_lba_t                               new_verify_checkpoint;
    fbe_status_t                            status;
    fbe_chunk_index_t                       chunk_index; 
    fbe_chunk_index_t                       new_chunk_index;
    fbe_chunk_count_t                       exported_chunks;
    fbe_block_count_t                       block_count;
    fbe_u64_t                               metadata_offset = FBE_LBA_INVALID;
    fbe_u64_t                               current_offset = FBE_LBA_INVALID;


    raid_group_p = (fbe_raid_group_t*)context;

     // Get the metadata operation from the payload
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    // Get the status of the paged metadata operation.
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);

    /*! @todo Change to debug.
     */
    status = fbe_transport_get_status_code(packet_p);
    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)||(status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s meta op: %d status: 0x%x packet status: 0x%x \n",
                          __FUNCTION__, metadata_operation_p->opcode,
                          metadata_status, status);
    }
    // If the metadata status is bad, handle the error status
    if(metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)
    {
        fbe_raid_group_handle_metadata_error(raid_group_p, packet_p, metadata_status);
    }

    // If the status is not good set the packet status and return 
    // so that the packet gets completed
    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s paged metadata get bits failed.\n",
                              __FUNCTION__);
        // Release the metadata operation
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        
                
        /*! Is this correct way to handle? 
            If there is a metadata error, set this completion function so that
            it will clean up the iots, release the stripe lock and will clean it up
        */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion,
                                              raid_group_p);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;                                    
    }
    
    /* Current offset gives the new metadata offset for the chunk that needs to be acted upon*/
    current_offset = metadata_operation_p->u.next_marked_chunk.current_offset;

    /* calculate the new checkpoint based on the new metadata offset */
    new_chunk_index = current_offset/sizeof(fbe_raid_group_paged_metadata_t);
    exported_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
    if(new_chunk_index >= exported_chunks)
    {
        new_chunk_index = exported_chunks;
    }
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p,
                                                  new_chunk_index,
                                                  &new_verify_checkpoint);

    //  Copy the chunk info that was read to iots chunk info
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_copy_memory(&iots_p->chunk_info[0], 
                    metadata_operation_p->u.metadata.record_data, 
                    metadata_operation_p->u.metadata.record_data_size);
   
    // Get the metadata offset 
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);
    chunk_index = metadata_offset/sizeof(fbe_raid_group_paged_metadata_t);

    // Release the metadata operation
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    // Get the lba of the next chunk
    status = fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, chunk_index, &start_lba);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s invalid chunk index. 0x%llx\n",
                              __FUNCTION__, (unsigned long long)chunk_index);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion,
                                              raid_group_p);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    // Get the iots from the payload to retrieve the type of verify
    fbe_raid_iots_get_opcode(iots_p, &block_opcode);
    fbe_raid_iots_get_lba(iots_p, &start_lba);
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    // Convert the opcode into verify flags
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_type);
    
    /* If the new verify checkpoint is greater than the current lba just move the checkpoint */
    if(new_verify_checkpoint >= (start_lba + block_count))
    {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion,
                                              raid_group_p);  
        
        block_count =   new_verify_checkpoint - start_lba;  

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_verify_md_compl Verify:%d not set. current_chkpt:0x%llx,bl:0x%llx new_chkpt:0x%llx\n",
                              verify_type, (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)new_verify_checkpoint);
        status = fbe_raid_group_update_verify_checkpoint(packet_p,
                                                         raid_group_p,
                                                         start_lba,
                                                         block_count,
                                                         verify_type,
                                                         __FUNCTION__);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;        

    }

    fbe_raid_group_verify_handle_chunks(raid_group_p, packet_p); 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_verify_get_paged_metadata_completion


/*!****************************************************************************
 * fbe_raid_group_update_verify_checkpoint()
 ******************************************************************************
 * @brief
 *   This function updates the verify checkpoint depending upon the type
 *   of verify.
 * 
 * @param packet_p              - pointer to the packet
 * @param raid_group_p          - pointer to the raid group
 * @param start_lba       - Start lba of the verify
 * @param verify_flags          - verify flag to indicate the type of verify
 * @param caller
 * 
 * @return  fbe_status_t            
 *
 * @author
 *   07/17/2010 - Created. Ashwin Tamilarasan. 
 * @Modified 
 *   08/04/2011 -  Vishnu Sharma (To use incremental update). 
 ******************************************************************************/
fbe_status_t fbe_raid_group_update_verify_checkpoint(
                                                  fbe_packet_t*     packet_p,
                                                  fbe_raid_group_t* raid_group_p,
                                                  fbe_lba_t         start_lba,
                                                  fbe_block_count_t block_count,
                                                  fbe_raid_verify_flags_t verify_flags,
                                                  const char *caller)
{
    //fbe_block_count_t                             block_count;
    fbe_lba_t                                     capacity;
    fbe_lba_t                                     exported_capacity;
    fbe_lba_t                                     new_checkpoint;
    fbe_status_t                                  status; 
    fbe_u64_t                                     metadata_offset = 0;
    fbe_u64_t                                     second_metadata_offset = 0;
    fbe_lba_t                                     current_checkpoint;
    fbe_raid_group_nonpaged_metadata_t*           raid_group_non_paged_metadata_p = NULL;

    
    //block_count = fbe_raid_group_get_chunk_size(raid_group_p);

    // Update the verify checkpoint
    new_checkpoint = start_lba + block_count;
    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
   
    current_checkpoint = fbe_raid_group_verify_get_checkpoint(raid_group_p, verify_flags);
    status = fbe_raid_group_metadata_get_verify_chkpt_offset(raid_group_p, verify_flags, &metadata_offset);
    if (status != FBE_STATUS_OK)
    {
         fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Unknown verify flag: 0x%x status: 0x%x\n", __FUNCTION__, verify_flags, status);
         fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet_p);
         return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*  Get the configured and exported capacity per disk, which is the disk ending LBA + 1 */
    capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /*  If check point has reached to the capacity we will make check point value FBE_LBA_INVALID.
        Metadata service will perform the following operation to update check point to FBE_LBA_INVALID
        checkpoint = start_lba + block_count;
        checkpoint = start_lba + FBE_LBA_INVALID - start_lba;
        
    */      
    
    /* When the start_lba < exported_capacity && checkpoint >= capacity we are crossing the user data region and moving
     * to metadata region. Since we are done with verify persist the checkpoint.
     */
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_INFO, "verify inc chkpt: new_c: 0x%llx cap: 0x%llx blks: 0x%llx, lba:0x%llx\n",  
                          (unsigned long long)new_checkpoint, (unsigned long long)capacity, (unsigned long long)block_count, (unsigned long long)start_lba);
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_INFO, "verify inc chkpt: c: 0x%llx offset: %d excap:0x%llx, flags:%d\n",  
                          (unsigned long long)current_checkpoint, (int)metadata_offset, (unsigned long long)exported_capacity, verify_flags);
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                         "rg update verify chkpt: 0x%llx from: %s \n",
                         (unsigned long long)current_checkpoint, caller);
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                         "rg update verify chkpt: 0x%llx start lba: 0x%llx blks: 0x%llx flags: 0x%x \n",
                         (unsigned long long)new_checkpoint, (unsigned long long)start_lba, (unsigned long long)block_count, verify_flags);

    /* We allow a different thread to update the checkpoint?!!
     */
    if((start_lba < exported_capacity) && (new_checkpoint >= exported_capacity))
    {
        /* It is possible that someone moved the checkpoint.  If so, we should skip this step since otherwise
         * we will overwrite their value.  This check is safe since we hold the np distributed lock. 
         */
        if (start_lba != current_checkpoint)
        {
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                                  FBE_TRACE_MESSAGE_ID_INFO, "verify skip inc chk old_c: 0x%llx start_lba: 0x%llx cap: 0x%llx\n",  
                                  (unsigned long long)current_checkpoint, (unsigned long long)start_lba, (unsigned long long)capacity);
            fbe_transport_complete_packet(packet_p);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* We no longer reset the error_verify checkpoint on every error, since that was preventing high lbas from 
         * being verified, but instead, let the checkpoint continue and set the error_verify_required flag.
         * When we get here, if that flag is set, we reset the checkpoint to 0, and clear the flag.
         */
        if (fbe_raid_group_metadata_is_error_verify_required(raid_group_p) &&
            (verify_flags & FBE_RAID_BITMAP_VERIFY_ERROR))
        {
            new_checkpoint = 0;

            /* Setup a callback to clear error_verify_required flag after the checkpoint is cleared
             * This ordering means that any error which occurs between the two will see the flag set
             * and won't set it, but since the monitor is still at chkpt 0 we won't miss any errors
             */
            fbe_transport_set_completion_function(packet_p, 
                                                  fbe_raid_group_metadata_clear_error_verify_required, 
                                                  raid_group_p);            
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                                 "rg update verify chkpt restart at 0 due to error_verify_required flag\n");
        }
        else if (fbe_raid_group_metadata_is_iw_verify_required(raid_group_p) &&
                 (verify_flags & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE))
        {
            new_checkpoint = 0;

            /* Setup a callback to clear iw_verify_required flag after the checkpoint is cleared
             * This ordering means that any error which occurs between the two will see the flag set
             * and won't set it, but since the monitor is still at chkpt 0 we won't miss any errors
             */
            fbe_transport_set_completion_function(packet_p, 
                                                  fbe_raid_group_metadata_clear_iw_verify_required, 
                                                  raid_group_p);            
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                                 "rg update verify chkpt restart at 0 due to iw_verify_required flag\n");
        }
        else
        {
            new_checkpoint = FBE_LBA_INVALID;

        }
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                                 metadata_offset, 
                                                                 (fbe_u8_t*)&new_checkpoint,
                                                                 sizeof(fbe_lba_t));
    }
    else
    {
        fbe_u32_t    ms_since_checkpoint;
        fbe_time_t  last_checkpoint_time;
        fbe_u32_t   peer_update_period_ms;

        fbe_raid_group_get_last_checkpoint_time(raid_group_p, &last_checkpoint_time);
        ms_since_checkpoint = fbe_get_elapsed_milliseconds(last_checkpoint_time);
        fbe_raid_group_class_get_peer_update_checkpoint_interval_ms(&peer_update_period_ms);
        if (ms_since_checkpoint > peer_update_period_ms)
        {
            /* Periodically we will set the checkpoint to the peer. 
             * A set is needed since the peer might be out of date with us and an increment 
             * will not suffice. 
             */
            status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                                            packet_p,
                                                                            metadata_offset,
                                                                            second_metadata_offset,
                                                                            start_lba + block_count);
            fbe_raid_group_update_last_checkpoint_time(raid_group_p);
        }
        else
        {
            /* Increment checkpoint locally without sending to peer in order to not 
             * thrash the CMI. 
             */
            status = fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer((fbe_base_config_t *)raid_group_p,
                                                                               packet_p,
                                                                               metadata_offset,
                                                                               second_metadata_offset,
                                                                               start_lba,
                                                                               block_count);
        }
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}// End fbe_raid_group_update_verify_checkpoint

/*!****************************************************************************
 * fbe_raid_group_ask_upstream_for_permission()
 ******************************************************************************
 * @brief
 *   This function asks the upstream objects to see whether the lba we are trying
 *   to verify is consumed or not by the user
 * 
 * @param packet_p              - pointer to the packet
 * @param in_raid_group_p          - pointer to the raid group
 * @param verify_lba               - the lba to check
 * @param in_verify_flags          - type od verify          
 *
 * @return  fbe_status_t            
 *
 * @author
 *   07/17/2010 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_ask_upstream_for_permission(
                                    fbe_packet_t*          packet_p,
                                    fbe_raid_group_t*      in_raid_group_p,
                                    fbe_lba_t              verify_lba,                                    
                                    fbe_raid_verify_flags_t in_verify_flags)

{

    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    fbe_block_count_t block_count;
    fbe_u16_t data_disks;
    fbe_chunk_size_t chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p);
    fbe_status_t status;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_lba_t           io_block_count = FBE_LBA_INVALID;

    block_count = chunk_size * FBE_RAID_GROUP_BACKGROUND_OP_CHUNKS;
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    start_lba = (verify_lba * data_disks);
    end_lba = start_lba + (block_count * data_disks) - 1;

    /* Get the metadata positions.
     */
    fbe_raid_group_get_metadata_positions(in_raid_group_p, &raid_group_metadata_positions);

    if ((start_lba < raid_group_metadata_positions.exported_capacity) &&
        (end_lba >= raid_group_metadata_positions.exported_capacity) )
    {
        /* Request exceeds user space, truncate it.
         * We perform metadata verifies separately, so the user verify should not span into metadata. 
         */
        block_count = (raid_group_metadata_positions.exported_capacity - start_lba) / data_disks;
    }

    
    // Save the verify flags in the tracking structure
    fbe_raid_group_set_verify_flags(in_raid_group_p, in_verify_flags);

    /* make sure that the permit request IO range limit to only one client */
    if(raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
         io_block_count = block_count;
         fbe_raid_group_monitor_update_client_blk_cnt_if_needed(in_raid_group_p, verify_lba, &io_block_count);
         if((io_block_count != FBE_LBA_INVALID) && (block_count != io_block_count))
        {

            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, 
                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                "Raid Group: send verify permit s_lba 0x%llx, orig blk 0x%llx, new blk 0x%llx \n", (unsigned long long)verify_lba,
                (unsigned long long)block_count, (unsigned long long)io_block_count);

            /* update the verify block count so that permit request will limit to only one client */               
            block_count = io_block_count;
            
        }
    }    

    // Ask for permission from the upstream object
    status = fbe_raid_group_ask_verify_permission(in_raid_group_p, 
                                                  packet_p,                                                   
                                                  verify_lba,
                                                  block_count);
    if (status != FBE_STATUS_OK)
    {
        // Permission call failed. Complete the packet.
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;


}// End fbe_raid_group_ask_upstream_for_permission

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function gets the chunk info from the bitmap for a range of chunks.
 *   It issues a read to the paged metadata service.  
 *
 *   Note: since this is a generic function, a completion function should be set 
 *   by the caller. 
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param in_start_chunk_index      - index of the first chunk to read
 * @param in_chunk_count            - number of chunks to read
 *
 * @return fbe_status_t            
 *
 * @author
 *   06/07/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_get_paged_metadata(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index,
                                        fbe_chunk_count_t                   in_chunk_count)
{

    fbe_payload_ex_t*                  sep_payload_p;                      // pointer to sep payload
    fbe_payload_metadata_operation_t*   metadata_operation_p;               // pointer to metadata operation 
    fbe_u64_t                           metadata_offset;                    // offset within the metadata
    fbe_status_t                        status;                             // fbe status
    fbe_u32_t                           read_size;                          // number of bytes to read 
    fbe_lba_t                           start_lba;                          // starting LBA


    //  Allocate a metadata operation
    sep_payload_p        = fbe_transport_get_payload_ex(packet_p); 
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    //  Set up the offset of the starting chunk in the metadata (ie. where we'll read from)
    metadata_offset = in_start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    //  Set up the number of bytes to read
    read_size = in_chunk_count * sizeof(fbe_raid_group_paged_metadata_t); 

    //  Build the paged metadata read request info in the payload.  
    //  Note that even though this call takes a pointer to a buffer, it does not use it; it only stores the data 
    //  in the metadata portion of the packet.  So we are passing NULL for this buffer pointer. 
    fbe_payload_metadata_build_paged_get_bits(metadata_operation_p,
                                              &((fbe_base_config_t*) in_raid_group_p)->metadata_element,
                                              metadata_offset,
                                              (fbe_u8_t*) NULL,
                                              read_size,
                                              sizeof(fbe_raid_group_paged_metadata_t));

    //  Trace which LBA we are reading from  
    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p, in_start_chunk_index, &start_lba); 
    fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, 
        FBE_TRACE_MESSAGE_ID_INFO, "%s: issuing MD read for LBA 0x%llx, chunk count %d\n", __FUNCTION__, 
        (unsigned long long)start_lba, in_chunk_count);

    /* Now populate the metadata strip lock information */
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(in_raid_group_p,
                                                                 metadata_operation_p,
                                                                 1);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_get_paged_metadata_completion, in_raid_group_p);
    //  Issue the metadata operation to read the chunk info.  This function returns FBE_STATUS_OK if successful. 
    status = fbe_metadata_operation_entry(packet_p);

    return status; 

} // End fbe_raid_group_bitmap_get_paged_metadata()

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function is called after a paged metadata read completes. It save
 *   the read paged metadata to the iots chunk_info, checks the status,
 *   sets the proper condition and releases the metadata operation. 
 *
 *   Note: since this is a generic function, a completion function that processes
 *   the chunk_info should be set by the caller. 
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/18/2012 - Created. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_get_paged_metadata_completion(fbe_packet_t* packet_p, 
                                                                        fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*                       raid_group_p = NULL;                // pointer to raid group object 
    fbe_status_t                            packet_status;                      // overall status from the packet
    fbe_payload_ex_t*                       sep_payload_p = NULL;               // pointer to sep payload
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;        // pointer to metadata operation 
    fbe_payload_metadata_status_t           metadata_status;                    // status of the metadata operation
    fbe_u64_t                               metadata_offset;                    // offset within the metadata
    fbe_chunk_index_t                       start_chunk_index;                  // starting chunk index
    fbe_lba_t                               start_lba;                          // starting LBA
    fbe_status_t                            new_packet_status;                  // status to set in the packet

    fbe_raid_iots_t*                iots_p = NULL;
    fbe_lba_t                       lba;
    fbe_block_count_t               blocks;
    fbe_chunk_size_t                chunk_size;
    fbe_chunk_count_t               chunk_count;
    fbe_chunk_count_t               chunk_index;

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Get the status of the packet
    packet_status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    //  Get the status of the metadata operation 
    metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);

    //  Get the IOTS for this I/O
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    //  Get the LBA and block count from the IOTS
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

    //  Get the chunk size 
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p); 

    //  Determine the start chunk index and number of chunks.  This call will adjust for disk-based LBAs vs 
    //  raid-based LBAs. 
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &chunk_index, &chunk_count);

    //  Copy the chunk info that was read to iots chunk info
    fbe_copy_memory(&iots_p->chunk_info[0], 
                    metadata_operation_p->u.metadata.record_data, 
                    metadata_operation_p->u.metadata.record_data_size);

    //  Trace which LBA completed reading 
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);
    start_chunk_index = metadata_offset / sizeof(fbe_raid_group_paged_metadata_t);
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, start_chunk_index, &start_lba);
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
        "rg_bmp_get_paged_mdata_compl: completed for LBA 0x%llx\n",
    (unsigned long long)lba);

    //  Release the metadata operation
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    //  If the packet status was not a success, return the packet status as is 
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "rg_bmp_wt_paged_mdata_compl write fail,packet err %d,mdata stat %d,packet 0x%p\n", 
            packet_status, metadata_status, packet_p); 
        return packet_status;
    }

    //  If the metadata operation was successful, then we are done - return success
    if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK)
    {
        return FBE_STATUS_OK;
    }

    //  There was a failure in the metadata operation.  Set the packet to a status corresponding to the type of
    //  metadata failure. 
    fbe_raid_group_bitmap_translate_metadata_status_to_packet_status(raid_group_p, metadata_status, 
                                                                     &new_packet_status);
    fbe_transport_set_status(packet_p, new_packet_status, 0);

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
        "rg_bmp_wt_paged_mdata_compl write fail,packet stat set to %d,mdata stat %d\n",
        new_packet_status,  metadata_status);

    if (new_packet_status == FBE_STATUS_IO_FAILED_NOT_RETRYABLE){
        // set the condition to transition the RG to the ACTIVATE state.
        //  In the ACTIVATE state, we will verify the paged metadata and try to reconstruct it.
      fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                             FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);
    }

    //  Return the packet status  
    return new_packet_status; 

} // End fbe_raid_group_bitmap_get_paged_metadata_completion()
/*!****************************************************************************
 * fbe_raid_group_bitmap_get_next_marked_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function gets the next marked chunk.
 *   It issues a read to the paged metadata service.  
 *
 *   Note: since this is a generic function, a completion function should be set 
 *   by the caller. 
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to the packet
 * @param in_start_chunk_index      - index of the first chunk to read
 * @param in_chunk_count            - number of chunks to read
 *
 * @return fbe_status_t            
 *
 * @author
 *   03/26/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_next_marked_paged_metadata(
                                        fbe_raid_group_t*                   raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   start_chunk_index,
                                        fbe_chunk_count_t                   chunk_count,
                                        metadata_search_fn_t                search_fn,
                                        void*                               context)
{

    fbe_payload_ex_t*                  sep_payload_p;
    fbe_payload_metadata_operation_t*   metadata_operation_p;
    fbe_u64_t                           metadata_offset;
    fbe_status_t                        status;
    fbe_u32_t                           read_size;
    fbe_lba_t                           start_lba;

    //  Allocate a metadata operation
    sep_payload_p        = fbe_transport_get_payload_ex(packet_p); 
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    //  Set up the offset of the starting chunk in the metadata (ie. where we'll read from)
    metadata_offset = start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    //  Set up the number of bytes to read
    read_size = chunk_count * sizeof(fbe_raid_group_paged_metadata_t); 

    fbe_payload_metadata_build_paged_get_next_marked_bits(metadata_operation_p,
                                              &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                              metadata_offset,
                                              NULL,
                                              chunk_count * sizeof(fbe_raid_group_paged_metadata_t),
                                              sizeof(fbe_raid_group_paged_metadata_t),
                                              search_fn,// callback function
                                              chunk_count * sizeof(fbe_raid_group_paged_metadata_t), // search size
                                              context ); // context 

    //  Trace which LBA we are reading from  
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, start_chunk_index, &start_lba); 
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, 
        FBE_TRACE_MESSAGE_ID_INFO, "%s: issuing MD read for LBA 0x%llx, chunk count %d\n", __FUNCTION__, 
        (unsigned long long)start_lba, chunk_count);

    /* Now populate the metadata strip lock information */
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                                 metadata_operation_p,
                                                                 1);
    //  Issue the metadata operation to read the chunk info.  This function returns FBE_STATUS_OK if successful. 
    status = fbe_metadata_operation_entry(packet_p);

    return status; 

} // End fbe_raid_group_bitmap_get_marked_paged_metadata()

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_next_marked()
 ******************************************************************************
 * @brief
 *  This was copied from fbe_raid_group_bitmap_get_marked_paged_metadata()
 *  This is specifically used for a large scan of the bitmap.
 * 
 * @param raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param start_chunk_index      - index of the first chunk to read
 * @param chunk_count            - number of chunks to read
 * @param search_fn
 * @param context
 *
 * @return fbe_status_t            
 *
 * @author
 *   4/25/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_bitmap_get_next_marked(fbe_raid_group_t*                   raid_group_p,
                                      fbe_packet_t*                       packet_p,
                                      fbe_chunk_index_t                   start_chunk_index,
                                      fbe_chunk_count_t                   chunk_count,
                                      metadata_search_fn_t                search_fn,
                                      void*                               context)
{

    fbe_payload_ex_t*                  sep_payload_p = NULL;
    fbe_payload_metadata_operation_t*   metadata_operation_p = NULL;
    fbe_u64_t                           metadata_offset;
    fbe_status_t                        status;
    fbe_u32_t                           read_size;
    fbe_lba_t                           start_lba;
    fbe_sg_element_t                   *sg_p = NULL;

    sep_payload_p        = fbe_transport_get_payload_ex(packet_p); 
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    metadata_offset = start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    read_size = chunk_count * sizeof(fbe_raid_group_paged_metadata_t); 

    fbe_payload_metadata_build_paged_get_next_marked_bits(metadata_operation_p,
                                              &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                              metadata_offset,
                                              NULL,
                                              chunk_count * sizeof(fbe_raid_group_paged_metadata_t),
                                              sizeof(fbe_raid_group_paged_metadata_t),
                                              search_fn,
                                              chunk_count * sizeof(fbe_raid_group_paged_metadata_t), // search size
                                              context );

    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, start_chunk_index, &start_lba); 
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_INFO, "%s: issuing MD read for LBA 0x%llx, chunk count %d\n", __FUNCTION__, 
                          (unsigned long long)start_lba, chunk_count);
    
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                                 metadata_operation_p,
                                                                 1);
    /* Initialize cient_blob, if it is provided.  */
    fbe_payload_ex_get_sg_list(sep_payload_p, &sg_p, NULL);
    if (sg_p) {
        status = fbe_metadata_paged_init_client_blob_with_sg_list(sg_p, metadata_operation_p);
    }

    status = fbe_metadata_operation_entry(packet_p);
    return status; 
}
/**************************************
 * end fbe_raid_group_bitmap_get_next_marked
 **************************************/
/*!***************************************************************************
 *          fbe_raid_group_bitmap_validate_paged_update_request()
 *****************************************************************************

 * @brief   Before modifying the paged metadata validate that we are writing
 *          sane values.
 *
 * @param   raid_group_p           - pointer to the raid group
 * @param   start_chunk_index      - chunk index to start the write at 
 * @param   chunk_count            - number of chunks to write
 * @param   chunk_data_p           - pointer to data indicating which bit(s) of 
 *                                    the chunk info to write on disk 
 *
 * @return fbe_status_t            
 *
 * @author
 *  03/13/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_validate_paged_update_request(fbe_raid_group_t *raid_group_p,
                                                                        fbe_chunk_index_t start_chunk_index,
                                                                        fbe_chunk_count_t chunk_count,
                                                                        fbe_raid_group_paged_metadata_t *chunk_data_p) 
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t nonpaged_nr_bitmask = 0;
    fbe_bool_t                  b_is_request_in_user_area;


    /*! @note Since method does not validate the needs rebuilt information 
     *        that was obtained from the metadata of metadata 
     *        (i.e. the non-paged).
     */
    fbe_raid_group_bitmap_determine_if_chunks_in_data_area(raid_group_p,
                                                           start_chunk_index, 
                                                           chunk_count, 
                                                           &b_is_request_in_user_area);
    if (b_is_request_in_user_area == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }


    /* Determine the proper setting of the NR from the paged.
     */
    status = fbe_raid_group_bitmap_determine_nr_bits_from_nonpaged(raid_group_p, 
                                                                   start_chunk_index, 
                                                                   chunk_count,
                                                                   &nonpaged_nr_bitmask);
    if (status != FBE_STATUS_OK)
    {
        /* Report the failure
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to get nonpaged bits - status: 0x%x \n",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*! @note This flag should not be set.
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_DISABLE_UPDATE_NR_VALIDATION))
    {
        /* Trace some information and do not validate the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild bitmask: 0x%x nonpaged: 0x%x `DISABLE_UPDATE_NR_VALIDATION' debug flag set skip validation \n",
                              chunk_data_p->needs_rebuild_bits, nonpaged_nr_bitmask);
        return FBE_STATUS_OK;
    }

    /* Rekey needs to set other bits. Disabling. */
#if 0
    /*! @note Currently we only validate the needs rebuild bitmask.  We can
     *        only check for `extra' bits set. Since the update paged
     *        metadata operation `set bits' XOR in the new bits
     */
    if ((chunk_data_p->needs_rebuild_bits & ~nonpaged_nr_bitmask) != 0)
    {
        fbe_trace_level_t           trace_level = FBE_TRACE_LEVEL_ERROR;

        /* Trace a critical error in checked builds otherwise trace an error.
         */
        trace_level = (RAID_GROUP_DBG_ENABLED) ? FBE_TRACE_LEVEL_CRITICAL_ERROR : FBE_TRACE_LEVEL_ERROR;
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              trace_level,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: validate paged request chunk index: 0x%llx count: 0x%x NR bitmask: 0x%x didn't match nonpaged: 0x%x \n",
                              start_chunk_index, chunk_count, chunk_data_p->needs_rebuild_bits, nonpaged_nr_bitmask);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_raid_group_bitmap_validate_paged_update_request()
 ***********************************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_update_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function updates the chunk info in the bitmap for the given range of 
 *   chunks.  It issues a set/clear to the paged metadata service. 
 * 
 *   Note: since this is a generic function, if any specific completion action
 *   is needed, a completion function should be set by the caller. 
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param in_start_chunk_index      - chunk index to start the write at 
 * @param in_chunk_count            - number of chunks to write
 * @param in_chunk_data_p           - pointer to data indicating which bit(s) of 
 *                                    the chunk info to write on disk 
 * @param in_perform_clear_bits_b   - set to TRUE if the bits should be cleared
 *                                    instead of set 
 *
 * @return fbe_status_t            
 *
 * @author
 *   06/08/2010 - Created. Jean Montes.
 *   01/16/2012 - Change function name from write to update.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_update_paged_metadata(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   start_chunk_index,
                                        fbe_chunk_count_t                   chunk_count,
                                        fbe_raid_group_paged_metadata_t*    in_chunk_data_p, 
                                        fbe_bool_t                          in_perform_clear_bits_b)
{

    fbe_payload_ex_t*                   sep_payload_p = NULL;               // pointer to sep payload
    fbe_payload_metadata_operation_t*   metadata_operation_p = NULL;        // pointer to metadata operation
    fbe_payload_block_operation_t*      block_operation_p = NULL;           // pointer to block operation                                                                             
    fbe_u64_t                           metadata_offset;                    // offset within metadata for the chunk
    fbe_status_t                        status;                             // fbe status
    fbe_chunk_count_t                   chunk_count_per_request;
    fbe_chunk_count_t                   chunk_count_processed;
    fbe_raid_geometry_t                *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_memory_request_t *              memory_request_p = NULL;
    fbe_payload_block_operation_t*      prev_block_operation_p = NULL;           // pointer to block operation                                                                             

    //  Allocate a metadata operation
    sep_payload_p        = fbe_transport_get_payload_ex(packet_p); 
    if(sep_payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        prev_block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    }

    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    /* Save the original block and LBA values */
    fbe_payload_block_set_lba(block_operation_p, start_chunk_index);
    fbe_payload_block_set_block_count(block_operation_p, chunk_count);

    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    fbe_raid_group_get_max_allowed_metadata_chunk_count(&chunk_count_per_request);

    /* we need to break it to size of maximum chunk size request allowed*/
    chunk_count_processed = (chunk_count > chunk_count_per_request) ? chunk_count_per_request : chunk_count;
    
    //  Set up the offset of this chunk in the metadata (ie. where we'll start to write to)
    metadata_offset = start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    //  Build the paged metadata write request info in the payload.  
    //
    //  If we are clearing the bits, we use one call; if we are setting the bits, we use a different call.  Both 
    //  have the same parameters.  The mask of the chunk bits to modify and the size of the chunk info mask are 
    //  passed in the call.  The same chunk information will be written to the given number of chunks. 
    // 
    if (in_perform_clear_bits_b == FBE_TRUE)
    {
        fbe_payload_metadata_build_paged_clear_bits(metadata_operation_p,
            &((fbe_base_config_t*) in_raid_group_p)->metadata_element, metadata_offset, 
            (fbe_u8_t*) in_chunk_data_p, sizeof(fbe_raid_group_paged_metadata_t), chunk_count_processed);
    }
    else
    {
        /* Before we update the paged validate the request.
         */
        status = fbe_raid_group_bitmap_validate_paged_update_request(in_raid_group_p,
                                                                     start_chunk_index,
                                                                     chunk_count_processed,
                                                                     in_chunk_data_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s validate paged update failed - status: 0x%x \n",
                                  __FUNCTION__, status);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }
        fbe_payload_metadata_build_paged_set_bits(metadata_operation_p,
            &((fbe_base_config_t*) in_raid_group_p)->metadata_element, metadata_offset, 
            (fbe_u8_t*) in_chunk_data_p, sizeof(fbe_raid_group_paged_metadata_t), chunk_count_processed);

        if (fbe_raid_geometry_is_vault(raid_geometry_p)){
            metadata_operation_p->u.metadata.operation_flags = FBE_PAYLOAD_METADATA_OPERATION_FLAGS_REPEAT_TO_END; 
        }
    }

    /* Initialize cient_blob */
    if (prev_block_operation_p && 
        (prev_block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA))
    {
        memory_request_p = fbe_transport_get_memory_request(packet_p);
        status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, 0, FBE_FALSE);
    }

    //  Set the completion function 
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_update_paged_metadata_completion,
                (fbe_packet_completion_context_t) in_raid_group_p);

    //  Trace which LBA we are writing to 
    fbe_raid_group_trace(in_raid_group_p, 
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR, 
                         "RG: update paged clear/set:%d data:0x%x start:0x%llx curr:0x%x total:0x%x \n",  
                         in_perform_clear_bits_b, in_chunk_data_p->needs_rebuild_bits,
                         start_chunk_index, chunk_count_processed, chunk_count);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);  

    /* Now populate the metadata strip lock information */
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(in_raid_group_p,
                                                                 metadata_operation_p,
                                                                 chunk_count_processed);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    //  Issue the metadata operation to write the chunk info.  This function returns FBE_STATUS_OK if successful. 
    status = fbe_metadata_operation_entry(packet_p);

    return status; 

} // End fbe_raid_group_bitmap_update_paged_metadata()


/*!****************************************************************************
 * fbe_raid_group_bitmap_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function is called after a paged metadata update completes. It checks
 *   the status, sets the proper condition and releases the metadata operation. 
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return fbe_status_t     
 *
 * @author
 *   06/08/2010 - Created. Jean Montes.
 *   01/16/2012 - Change function name from write to update.
 *   
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_update_paged_metadata_completion(
                                            fbe_packet_t*                       packet_p,
                                            fbe_packet_completion_context_t     in_context)

{
    fbe_raid_group_t*                       raid_group_p = NULL;                // pointer to raid group object 
    fbe_status_t                            packet_status;                      // overall status from the packet
    fbe_payload_ex_t*                       sep_payload_p = NULL;               // pointer to sep payload
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;        // pointer to metadata operation 
    fbe_payload_metadata_status_t           metadata_status;                    // status of the metadata operation
    fbe_status_t                            status; 
    fbe_payload_block_operation_t*          block_operation_p = NULL;           // pointer to block operation 
    fbe_bool_t                              more_processing_required = FBE_FALSE;

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Get the status of the packet
    packet_status = fbe_transport_get_status_code(packet_p);

    //  Get the status of the metadata operation 
    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);

    metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);

    block_operation_p = fbe_payload_ex_get_prev_block_operation(sep_payload_p);

    //  If the packet status was not a success, return the packet status as is 
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "rg_bmp_wt_paged_mdata_compl paged operation fail,packet err %d,mdata stat %d,packet 0x%p\n", 
            packet_status, metadata_status, packet_p); 
                //  Release the metadata and block operation
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);
        return packet_status;
    }

    /* Handle the metadata error */
    status = fbe_raid_group_handle_metadata_error(raid_group_p,packet_p,metadata_status);

    status = fbe_raid_group_bitmap_continue_metadata_update(raid_group_p, packet_p, &more_processing_required);

    /* If we were complete immediately and we are an optimzation case, copy over the paged info and complete 
     * immediately. 
     */
    if ((metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK) &&
       !more_processing_required)
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);
        block_operation_p = fbe_payload_ex_search_prev_block_operation(sep_payload_p);

        /* If we are a write that touches only one chunk, then MDS should have sent back the special bit that indicates 
         * it copied the information on the chunk *after* the write into the payload.  This will help us avoid the read 
         * of chunk info after the write. 
         */
        if ((block_operation_p != NULL) &&
            (metadata_operation_p->opcode == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS) &&
            (metadata_operation_p->u.metadata.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_VALID_DATA) &&
            ((block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
             (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE) ||
             (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED)) )
        {
            fbe_raid_iots_t *iots_p = NULL;
            fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
            if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED)) {
                fbe_copy_memory(&iots_p->chunk_info[0], 
                                metadata_operation_p->u.metadata.record_data, 
                                metadata_operation_p->u.metadata.record_data_size);
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED);
            }
            else {
                /* It is possible to come through this case when we mark for verify
                 * after having marked for nr on a write. 
                 */
                fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED);
            }
        }
        return status;
    }
    /* If we dont need more processing or there are errors, we can complete it immediately */
    else if(((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) &&
             (metadata_status != FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE)) ||
            (!more_processing_required))
    {    
        //  Release the metadata and block operation
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);
        return status;
    }

    return status;

} // End fbe_raid_group_bitmap_update_paged_metadata_completion()

/*!****************************************************************************
 * fbe_raid_group_bitmap_continue_metadata_update()
 ******************************************************************************
 * @brief
 *   This function is called after a paged metadata update completes to see if
 * there is more paged operation that needs to be done. 
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param raid_group_p    - pointer to the raid group object
 * @param more_processing_required - FBE_TRUE if more iterations are required
 * 
 * @return fbe_status_t     
 *
 * @author
 *   10/03/2012 - Created. Ashok Tamilarasan
 *   
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_continue_metadata_update(fbe_raid_group_t* raid_group_p,
                                                                   fbe_packet_t*     packet_p,
                                                                   fbe_bool_t *more_processing_required)
{
    fbe_status_t                            packet_status;                      // overall status from the packet
    fbe_payload_ex_t*                       sep_payload_p = NULL;               // pointer to sep payload
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;        // pointer to metadata operation 
    fbe_payload_metadata_status_t           metadata_status;                    // status of the metadata operation
    fbe_u64_t                               metadata_offset;                    // offset within the metadata
    fbe_chunk_index_t                       current_chunk_index;                  // starting chunk index
    fbe_lba_t                               start_lba;                          // starting LBA
    fbe_status_t                            status; 
    fbe_u64_t                               current_chunk_count;                       
    fbe_payload_block_operation_t*          block_operation_p = NULL;           // pointer to block operation 
    fbe_chunk_index_t                       orig_chunk_index;                  // starting chunk index
    fbe_chunk_count_t                       orig_chunk_count;                       
    fbe_chunk_count_t                       remaining;
    fbe_chunk_count_t                       chunk_count_per_request;

    //  Get the status of the packet
    packet_status = fbe_transport_get_status_code(packet_p);

    if(packet_status != FBE_STATUS_OK)
    {
        *more_processing_required = FBE_FALSE;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    //  Get the status of the metadata operation 
    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);


    metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);

    block_operation_p = fbe_payload_ex_get_prev_block_operation(sep_payload_p);
    orig_chunk_index = block_operation_p->lba;
    orig_chunk_count = (fbe_chunk_count_t)block_operation_p->block_count;

     
    //  Trace which LBA completed writing 
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);
    fbe_payload_metadata_get_metadata_repeat_count(metadata_operation_p, &current_chunk_count);

    current_chunk_index = metadata_offset / sizeof(fbe_raid_group_paged_metadata_t);

    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, current_chunk_index, &start_lba);
    
    fbe_raid_group_get_max_allowed_metadata_chunk_count(&chunk_count_per_request);

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                          FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "rg_bmp_wt_paged_mdata_compl: pmd completed. LBA:0x%llx, Index:0x%llx, count:%d\n", 
                          (unsigned long long)start_lba, (unsigned long long)current_chunk_index, (int)current_chunk_count);

    /* Check if we have satisfied the request */
    if((orig_chunk_index + orig_chunk_count) > (current_chunk_index + current_chunk_count))
    {
        /* Still more to do. So kick of another request */
        remaining = (fbe_chunk_count_t)((orig_chunk_index + orig_chunk_count) - (current_chunk_index + current_chunk_count));
        current_chunk_index += current_chunk_count;

        /* We there are still more than allowed remaining, split the request otherwise this will be the last operation */
        current_chunk_count = (fbe_u64_t)((remaining > chunk_count_per_request) ? chunk_count_per_request : remaining);
        *more_processing_required = FBE_TRUE;
    }
    else
    {
        *more_processing_required = FBE_FALSE;
        return FBE_STATUS_OK;
    }

     //  Set up the offset of this chunk in the metadata (ie. where we'll start to write to)
    metadata_offset = current_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    fbe_payload_metadata_reuse_metadata_operation(metadata_operation_p,
                                                 metadata_offset, 
                                                 current_chunk_count);

    //  Set the completion function 
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_update_paged_metadata_completion,
                (fbe_packet_completion_context_t) raid_group_p);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);  

    /* Now populate the metadata strip lock information */
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                                 metadata_operation_p,
                                                                 (fbe_chunk_count_t)current_chunk_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    //  Issue the metadata operation to write the chunk info.  This function returns FBE_STATUS_OK if successful. 
    status = fbe_metadata_operation_entry(packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/*!****************************************************************************
 * fbe_raid_group_bitmap_translate_metadata_status_to_packet_status()
 ******************************************************************************
 * @brief
 *   This function will take a metadata status as input and determine the 
 *   appropriate packet status for it.  It will return that packet status in 
 *   the output parameter.
 * 
 * @param in_raid_group_p       - pointer to the raid group
 * @param in_metadata_status    - metadata status
 * @param out_packet_status_p   - pointer to data that gets set to the packet status
 *                                that corresponds to this metadata status 
 * 
 * @return fbe_status_t     
 *
 * @author
 *   03/14/2011 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_translate_metadata_status_to_packet_status(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_payload_metadata_status_t       in_metadata_status,                    
                                    fbe_status_t*                       out_packet_status_p)                  
{

    //  Determine the packet status corresponding to the type of metadata status
    switch (in_metadata_status)
    {
        //  Set packet status to ok for the success case 
        case FBE_PAYLOAD_METADATA_STATUS_OK:
        /* !@todo correctable has correct data, return ok for now */
        case FBE_PAYLOAD_METADATA_STATUS_IO_CORRECTABLE:
            *out_packet_status_p = FBE_STATUS_OK;
            break;

        //  Set packet status to "retry" for those metadata status values that imply a retry 
        case FBE_PAYLOAD_METADATA_STATUS_BUSY:
        case FBE_PAYLOAD_METADATA_STATUS_TIMEOUT:
        case FBE_PAYLOAD_METADATA_STATUS_ABORTED:
        case FBE_PAYLOAD_METADATA_STATUS_IO_RETRYABLE:
        //  A metadata status "not retryable" error is set when quiescing metadata I/O or when a "retry not possible" error
        //  is returned from RAID for metadata I/O.  This is a retryable error now.
        {
            *out_packet_status_p = FBE_STATUS_IO_FAILED_RETRYABLE;
            break;
        }

        //  Set packet status to "not retryable" for those metadata status values that imply the I/O should be 
        //  failed
        case FBE_PAYLOAD_METADATA_STATUS_IO_NOT_RETRYABLE:
        case FBE_PAYLOAD_METADATA_STATUS_FAILURE:
        case FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE:
        {
            *out_packet_status_p = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
            break;
        }

        //  Set packet status to cancelled if the metadata request was aborted 
        case FBE_PAYLOAD_METADATA_STATUS_CANCELLED:
        {
            *out_packet_status_p = FBE_STATUS_CANCELED;
            break;
        }

        //  For an unknown status, return generic failure 
        default:
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                "%s invalid metadata status %d\n", __FUNCTION__, 
                in_metadata_status);
            *out_packet_status_p = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    }

    //  Return status  
    return FBE_STATUS_OK;  

} // End fbe_raid_group_bitmap_translate_metadata_status_to_packet_status()

/*!****************************************************************************
 * fbe_raid_group_bitmap_translate_packet_status_to_block_status()
 ******************************************************************************
 * @brief
 *   This function will take a packet status as input and determine the 
 *   appropriate block status for it.  It will return that block status in 
 *   the output parameter.
 * 
 * @param in_raid_group_p       - pointer to the raid group
 * @param in_packet_status      - packet status
 * @param out_block_status_p    - pointer to data that gets set to the block status
 *                                that corresponds to this packet status 
 * @param out_block_qualifier_p - pointer to data that gets set to the block qualifier
 *                                that corresponds to this packet status 
 * 
 * 
 * @return fbe_status_t     
 *
 * @author
 *   09/24/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_translate_packet_status_to_block_status(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_payload_metadata_status_t       in_packet_status,                    
                                    fbe_payload_block_operation_status_t    *out_block_status_p,
                                    fbe_payload_block_operation_qualifier_t *out_block_qualifier_p)                  
{

    //  Determine the block status and qualifier corresponding to the type of packet status
    switch (in_packet_status)
    {
        //  Set packet status to ok for the success case 
        case FBE_STATUS_OK:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_STATUS_IO_FAILED_RETRYABLE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;
        
        case FBE_STATUS_IO_FAILED_NOT_RETRYABLE:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;        
            break;
        
        case FBE_STATUS_CANCELED:
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED;
            *out_block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED;
            break;

        //  For an unknown status, return generic failure 
        default:
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                "%s unhandled status %d\n", __FUNCTION__, 
                in_packet_status);
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *out_block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
            break;
        }
    }

    //  Return status  
    return FBE_STATUS_OK;  

} // End fbe_raid_group_bitmap_translate_metadata_status_to_packet_status()


/*!****************************************************************************
 * fbe_raid_group_bitmap_read_chunk_info_using_nonpaged()
 ******************************************************************************
 * @brief
 *   This is the function for reading the chunk info for a range of chunks that 
 *   represent the paged metadata.  
 *
 *   The nonpaged metadata service is used to read the chunk info / metadata for 
 *   the paged metadata area.  Reads of the non-paged metadata are synchronous.
 *   So the data will actually be read by the caller's completion function, when 
 *   it calls fbe_raid_group_bitmap_process_read_chunk_info_for_range_done(). 
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param in_start_chunk_index      - index of the first chunk to read 
 * @param in_chunk_count            - number of chunks to read
 *
 * @return fbe_status_t            
 *
 * @author
 *   06/16/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_read_chunk_info_using_nonpaged(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index,
                                        fbe_chunk_count_t                   in_chunk_count)

{

    fbe_payload_ex_t*                      sep_payload_p;                      // get the pointer to payload
    fbe_payload_metadata_operation_t*       metadata_operation_p;               // pointer to fake metadata operation


    //  Make sure that the number of chunks requested does not exceed the number of chunks in the paged metadata 
    //  metadata
    if (in_chunk_count > FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: 0x%x chunks is above limit 0x%x\n",
                              __FUNCTION__, in_chunk_count, FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK; 
    }

    //  To keep the completion function we need to allocate the FAKE metadata operation here.
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    //  Just complete the packet.  The read of the non-paged metadata is synchronous.  The function 
    //  fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done() will be invoked by the caller's 
    //  completion function and it will get the data. 
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    //  Return success 
    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_read_chunk_info_using_nonpaged()


/*!****************************************************************************
 * fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done()
 ******************************************************************************
 * @brief
 *   This function copies the chunk info from the non-paged metadata into the 
 *   output parameter.
 * 
 *   The buffer passed in may be for a single chunk or an array for multiple
 *   chunks. 
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to the packet
 * @param in_start_chunk_index      - index of the first chunk to read 
 * @param in_chunk_count            - number of chunks to read
 * @param out_chunk_entry_p         - pointer to data that gets filled with the chunk
 *                                    info read
 *
 * @return fbe_status_t  
 *              return error status if out_chunk_entry_p is not populated.
 *
 * @author
 *   08/02/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done(
                                            fbe_raid_group_t*                   in_raid_group_p,
                                            fbe_packet_t*                       packet_p,
                                            fbe_chunk_index_t                   in_start_chunk_index, 
                                            fbe_chunk_count_t                   in_chunk_count,
                                            fbe_raid_group_paged_metadata_t*    out_chunk_entry_p)

{
    fbe_status_t                            packet_status;                      // overall status from the packet
    fbe_payload_ex_t*                       sep_payload_p;                      // get the pointer to payload
    fbe_payload_metadata_operation_t*       metadata_operation_p;               // pointer to fake metadata operation


    //  Get the FAKE metadata operation and release it
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        return packet_status;
    }

    //  Make sure that the number of chunks requested does not exceed the number of chunks in the paged metadata 
    //  metadata
    if (in_chunk_count > FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: %d chunks is above limit\n", __FUNCTION__, in_chunk_count);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    fbe_raid_group_bitmap_populate_iots_chunk_info(in_raid_group_p, in_start_chunk_index, in_chunk_count, out_chunk_entry_p);
   
    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done()


/*!****************************************************************************
 * fbe_raid_group_bitmap_populate_iots_chunk_info()
 ******************************************************************************
 * @brief
 *   This function primarily takes care of rebuild of large raid group. When a 
 *   request comes in to populate the iots chunk info, if the chunks per md of md 
 *   is 1 then we can just read chunk info requested by the chunk count. 
 * 
 *  It gets interesting when 1 md of md chunk represents more than one physical
 *  metadata chunk. Suppose say 1 md of md chunk represents 3 physical metadata chunk.
 *  A request comes in to get chunk info on 6 chunks. Since 1 md of md chunk represents
 *  3 chunks, we might read 2 or 3 md of md chunks and have to replicate that chunk info
 *  for 6 chunks. That is taken care by this function. 
 *
 *  We need to handle an interesting USECASE
 * 1) 1 md of md chunk represents 3 physical metadata chunk
 * 2) Say we finish rebuild of 1 st physical metadata chunk
 * 3) we cannot clear the NR bits in md of md chunk since there are 2 more chunks to be rebuilt
      before we can clear the md of md chunk
 * 4) Now an IO request comes to the 1st physical metadata chunk.
 * 5) When it tries to get the chunk info, we should make sure that the chunk is not degraded
 *    even though the md of md chunk says we are degraded. If this is not handled properly
 *    we could have a data loss
 *
 *
 * @param raid_group_p  - pointer to the raid group
 * @param chunk_count_p - Number of physical chunks of metadata.
 * @param start_chunk_index - index of the first chunk to read .
 * @param out_chunk_entry_p - pointer to data that gets filled with the chunk
 *                            info read
 *
 * @return fbe_status_t
 *
 * @author
 *  01/30/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_populate_iots_chunk_info(fbe_raid_group_t *raid_group_p,
                                                            fbe_chunk_index_t  start_chunk_index,
                                                            fbe_chunk_count_t chunk_count,
                                                            fbe_raid_group_paged_metadata_t*    out_chunk_entry_p)

{
    
    fbe_chunk_index_t                       relative_start_chunk_index;
    fbe_chunk_count_t                       chunks_per_md_of_md;
    fbe_chunk_count_t                       num_exported_chunks;
    fbe_raid_group_paged_metadata_t*        paged_md_md_p = NULL;
    fbe_u32_t                               read_size;
    fbe_u32_t                               index = 0;
    fbe_chunk_index_t                       array_index = 0;
    fbe_u64_t                               address_offset = 0;
    fbe_chunk_index_t                       md_md_start_chunk_index;
    fbe_raid_group_paged_metadata_t         paged_metadata;
    fbe_raid_position_bitmask_t             pos1_bitmask = 0;
    fbe_raid_position_bitmask_t             pos2_bitmask = 0;
    fbe_raid_group_nonpaged_metadata_t*     nonpaged_metadata_p = NULL;
    fbe_u32_t                               disk_pos = FBE_RAID_INVALID_DISK_POSITION; 
    fbe_lba_t                               checkpoint_1;
    fbe_lba_t                               checkpoint_2;
    fbe_lba_t                               metadata_lba;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Raw mirror uses md of md for all user chunks, there is no paged.
         */
        num_exported_chunks = 0;
    }
    else
    {
        num_exported_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
    }
    fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(raid_group_p, &chunks_per_md_of_md);

    fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(raid_group_p, start_chunk_index, 
                                                                         &md_md_start_chunk_index);

    relative_start_chunk_index = start_chunk_index - (fbe_chunk_index_t) num_exported_chunks;

    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(raid_group_p); 

    /* If the chunks per md of md is 1, that means it is one to one mapping so just read 
       the md of md chunks equal to the chunk count 
    */
    if(chunks_per_md_of_md == 1)
    {
        read_size = chunk_count * sizeof(fbe_raid_group_paged_metadata_t); 
        fbe_copy_memory(out_chunk_entry_p, &paged_md_md_p[md_md_start_chunk_index], read_size);

        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_INFO, "%s: chunks_per_md_of_md:0x%x, start_chk_index:0x%x, chnk_cnt:0x%x\n", 
                          __FUNCTION__, chunks_per_md_of_md, (unsigned int)start_chunk_index, chunk_count);

    /* If we reach here, we know chunks per md of md is > 1 
       We are getting the checkpoint and calculating the bitmask to handle a use case
       that is described above
     */

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);
    checkpoint_1 = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[0].checkpoint;
    disk_pos    = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[0].position;
    if(disk_pos != FBE_RAID_INVALID_DISK_POSITION && checkpoint_1 != FBE_LBA_INVALID)
    {
        pos1_bitmask = 1 << disk_pos;
    }
    checkpoint_2 = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[1].checkpoint;
    disk_pos    = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[1].position;
    if(disk_pos != FBE_RAID_INVALID_DISK_POSITION && checkpoint_2 != FBE_LBA_INVALID)
    {
        pos2_bitmask = 1 << disk_pos;
    }


    out_chunk_entry_p = out_chunk_entry_p + address_offset;
    for(index = 0; index < chunk_count; index ++)
    {
        array_index = (relative_start_chunk_index + index)/chunks_per_md_of_md;
        fbe_copy_memory(&paged_metadata, &paged_md_md_p[array_index], sizeof(fbe_raid_group_paged_metadata_t));

        /* start use case - Look in the description section for the use case 
        */
        fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, (start_chunk_index + index), &metadata_lba);
        if( (checkpoint_1 != FBE_LBA_INVALID) && (metadata_lba < checkpoint_1) )
        {
            paged_metadata.needs_rebuild_bits &= ~pos1_bitmask; 
        }

        if( (checkpoint_2 != FBE_LBA_INVALID) && (metadata_lba < checkpoint_2) )
        {
            paged_metadata.needs_rebuild_bits &= ~pos2_bitmask; 
        }
        /* end use case */ 

        fbe_copy_memory(out_chunk_entry_p, &paged_metadata, sizeof(fbe_raid_group_paged_metadata_t));
        out_chunk_entry_p++;
    }
    

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_bitmap_populate_iots_chunk_info()
 ******************************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_paged_metadata_chunks_per_data_disk()
 ******************************************************************************
 * @brief
 *   Determine how many chunks of paged metadata there are on each data disk.
 *
 * @param raid_group_p  - pointer to the raid group
 * @param chunk_count_p - Number of physical chunks of metadata per disk.
 *
 * @return fbe_status_t            
 *
 * @author
 *  5/4/2011 - Created. Rob Foley
 *  2/9/2012 - Updated the name to reflect per disk
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_get_paged_metadata_chunks_per_data_disk(fbe_raid_group_t *raid_group_p,
                                                             fbe_chunk_count_t *chunk_count_p)
{
    fbe_lba_t            paged_metadata_capacity;
    fbe_raid_geometry_t* raid_geometry_p = NULL;    // pointer to geometry of the raid object
    fbe_u16_t            data_disks;
    fbe_chunk_size_t     chunk_size;   // size of each chunk in blocks

    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *) raid_group_p,
                                                          &paged_metadata_capacity);
    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p); 

    *chunk_count_p = (fbe_chunk_count_t)(paged_metadata_capacity/data_disks)/chunk_size;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_bitmap_get_paged_metadata_chunks_per_data_disk()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md()
 ******************************************************************************
 * @brief
 *   Determine how many physical chunks per md of md chunks.
 *   This is needed since the number of md of md chunks is fixed, so for very large rgs
 *   each md of md chunk will need to represent more than one physical chunk.
 *
 * @param raid_group_p  - pointer to the raid group
 * @param chunk_count_p - Number of physical chunks per md of md chunk.
 *
 * @return fbe_status_t            
 *
 * @author
 *  5/4/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(fbe_raid_group_t *raid_group_p,
                                                        fbe_chunk_count_t *chunk_count_p)

{
    fbe_chunk_count_t num_phy_chunks_per_md_of_md;
    fbe_chunk_count_t md_chunks;

    fbe_raid_group_bitmap_get_paged_metadata_chunks_per_data_disk(raid_group_p, &md_chunks);

    num_phy_chunks_per_md_of_md = md_chunks / FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
    if (md_chunks % FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
    {
        num_phy_chunks_per_md_of_md++;
    }
    *chunk_count_p = num_phy_chunks_per_md_of_md;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_md_of_md_count()
 ******************************************************************************
 * @brief
 *   Determine how many md of md is in use.
 *   This is needed since the number of md of md slots is fixed, so for very large rgs
 *   each md of md slot will need to represent more than one physical chunks.
 *
 * @param raid_group_p  - pointer to the raid group
 * @param md_of_md_count_p - Number of md of md slots in use.
 *
 * @return fbe_status_t            
 *
 * @author
 *  2/14/2012 - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_md_of_md_count(fbe_raid_group_t *raid_group_p,
                                                      fbe_u32_t *md_of_md_count_p)
{
    fbe_chunk_count_t   num_phy_chunks_per_md_of_md;
    fbe_chunk_count_t   md_chunks;
    fbe_u32_t           last_full_slot;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        md_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
    }
    else
    {
        fbe_raid_group_bitmap_get_paged_metadata_chunks_per_data_disk(raid_group_p, &md_chunks);
    }
    num_phy_chunks_per_md_of_md = md_chunks / FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
    if (md_chunks % FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
    {
        num_phy_chunks_per_md_of_md++;
    }

    /*are we initialized yet, if not, just return 0 (AR 563379)*/
    if (num_phy_chunks_per_md_of_md == 0) {
        *md_of_md_count_p = 0;
        return FBE_STATUS_OK;
    }

    last_full_slot = md_chunks / num_phy_chunks_per_md_of_md;
    if (md_chunks % num_phy_chunks_per_md_of_md)
    {
        /* Validate that we have not exceeded the non-paged metdata of metadata
         * slot count.
         */
        if ((last_full_slot + 1) > FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
        {
            /* Report an error and sent the count to the max.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s calc slot count: %d greater than max: %d \n",
                                  __FUNCTION__, (last_full_slot + 1), FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS);
            *md_of_md_count_p = FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;

            return FBE_STATUS_GENERIC_FAILURE;
        }
        *md_of_md_count_p = last_full_slot + 1;
    }
    else
    {
        /* Validate that we have not exceeded the non-paged metdata of metadata
         * slot count.
         */
        if (last_full_slot > FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
        {
            /* Report an error and sent the count to the max.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s calc slot count: %d greater than max: %d \n",
                                  __FUNCTION__, last_full_slot, FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS);
            *md_of_md_count_p = FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;

            return FBE_STATUS_GENERIC_FAILURE;
        }
        *md_of_md_count_p = last_full_slot;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_bitmap_get_md_of_md_count()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_write_chunk_info_using_nonpaged()
 ******************************************************************************
 * @brief
 *   This function updates the chunk info for a range of chunks that represent
 *   the paged metadata.  It uses the nonpaged metadata service to write the 
 *   chunk info/metadata for the paged metadata area. 
 *
 * @param raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param in_start_chunk_index      - chunk index to start the write at 
 * @param in_chunk_count            - number of chunks to write
 * @param in_chunk_data_p           - pointer to data indicating which bit(s) of 
 *                                    the chunk info to write on disk 
 * @param in_perform_clear_bits_b   - set to TRUE if the bits should be cleared
 *                                    instead of set 
 *
 * @return fbe_status_t            
 *
 * @author
 *   06/16/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(
                                        fbe_raid_group_t*                   raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index,
                                        fbe_chunk_count_t                   in_chunk_count,
                                        fbe_raid_group_paged_metadata_t*    in_chunk_data_p, 
                                        fbe_bool_t                          in_perform_clear_bits_b)

{
    fbe_chunk_index_t                   paged_md_md_chunk_index;            // chunk index relative to paged MD MD
    fbe_u64_t                           metadata_offset;                    // offset at which to start the write 
    fbe_status_t                        status;
    fbe_chunk_count_t                   num_md_chunks;
    fbe_chunk_count_t                   mdd_chunks_to_clear_nr = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Raw mirror uses md of md for all chunks.
         */
        num_md_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
    }
    else
    {
        fbe_raid_group_bitmap_get_paged_metadata_chunks_per_data_disk(raid_group_p, &num_md_chunks);
    }

    //  Make sure that the number of chunks requested does not exceed the number of chunks in the paged metadata 
    //  metadata
    if (in_chunk_count > num_md_chunks)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: %d chunks is above limit %d\n", __FUNCTION__, in_chunk_count, num_md_chunks);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK; 
    }
   
    if (in_perform_clear_bits_b == FBE_TRUE)
    {
        fbe_raid_group_bitmap_get_num_of_bits_we_can_clear_in_mdd(raid_group_p, in_start_chunk_index, in_chunk_count, &mdd_chunks_to_clear_nr);
        if (mdd_chunks_to_clear_nr)
        {
            //  The start chunk index passed in is relative to the start of the user area.  Get the chunk index relative 
            //  to the start of the chunks of the paged metadata. 
            fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(raid_group_p, in_start_chunk_index, 
                                                                                &paged_md_md_chunk_index);

            metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->paged_metadata_metadata[paged_md_md_chunk_index]);

            status = fbe_base_config_metadata_nonpaged_clear_bits((fbe_base_config_t*) raid_group_p, packet_p,
                                                                   metadata_offset, (fbe_u8_t*) in_chunk_data_p, 
                                                                  sizeof(fbe_raid_group_paged_metadata_t), mdd_chunks_to_clear_nr);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        }
        else
        {
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    
    else
    {        
        fbe_raid_group_np_set_bits(packet_p, raid_group_p,in_chunk_data_p->needs_rebuild_bits);
    }

        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_bitmap_write_chunk_info_using_nonpaged()


/*!****************************************************************************
 * fbe_raid_group_np_set_bits()
 ******************************************************************************
 * @brief
 *   This function will mark NR on the entire metadata of metadata
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   10/26/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_np_set_bits(fbe_packet_t * packet_p,                                    
                                        fbe_raid_group_t* raid_group_p,
                                        fbe_raid_position_bitmask_t   nr_pos_bitmask)

{
    
    fbe_raid_group_paged_metadata_t     chunk_data; 
    fbe_u64_t                           metadata_offset;       
    fbe_u32_t                           md_of_md_count;

    if (fbe_raid_group_is_metadata_marked_NR(raid_group_p, nr_pos_bitmask))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:NR already marked on MD of MD pos_mask: 0x%x, pkt: %p\n",
                              __FUNCTION__, nr_pos_bitmask, packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
      
    //  Set up the bits of the metadata that need to be written, which are the NR bits 
    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_data.needs_rebuild_bits = nr_pos_bitmask;

    /* get md_of_md count */
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &md_of_md_count);

    //  Set up the offset of the starting chunk in the metadata 
    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->paged_metadata_metadata[0]);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s:Marking NR on metadata of metadata. pos_mask: %d\n",
                          __FUNCTION__, nr_pos_bitmask);
    
    fbe_base_config_metadata_nonpaged_set_bits((fbe_base_config_t*) raid_group_p, packet_p,
                                               metadata_offset, (fbe_u8_t*) &chunk_data, 
                                               sizeof(fbe_raid_group_paged_metadata_t), 
                                               md_of_md_count);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_raid_group_np_set_bits()

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_num_of_bits_we_can_clear_in_mdd()
 ******************************************************************************
 * @brief
 *   This function gets the number of bits we can clear in the md of md chunk.
 *   If the chunks per md of md is 1 we can clear the bits. If the chunks per md
 *   of md is more than 1, then we can only clear it if the chunk index is the last
 *   metadata chunk or the chunk index is a multiple of chunks per md of md

 * 
 * @param raid_group_p           - pointer to the raid group
 * @param chunk_index            - input chunk index, which is relative to start
 *                                 of the raid group 
 * @param chunk_count            - Number of chunks to clear for.
 * @param clear_nr_bits_p        - Number of bits we can clear.

 *
 * @return fbe_status_t            
 *
 * @author
 *   01/26/2012 - Created. Ashwin Tamilarasan

 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_num_of_bits_we_can_clear_in_mdd(
                                    fbe_raid_group_t*      raid_group_p,
                                    fbe_chunk_index_t      chunk_index, 
                                    fbe_chunk_count_t      chunk_count,
                                    fbe_u32_t*             clear_nr_bits_p)

{
    fbe_chunk_count_t               num_exported_chunks;
    fbe_chunk_count_t               chunks_per_md_of_md;
    fbe_chunk_index_t               md_of_md_chunk_index;
    fbe_chunk_index_t               end_mdd_chunk_index;
    fbe_chunk_index_t               relative_chunk_index; 
    fbe_chunk_count_t               total_chunks;
    fbe_u32_t                       num_bits_to_clear = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* If chunks per md of md is 1 just return true. Proceed only if chunks per md of md is > 1 */
    fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(raid_group_p, &chunks_per_md_of_md);
    if(chunks_per_md_of_md == 1)
    {
        *clear_nr_bits_p = chunk_count;
        return FBE_STATUS_OK;
    }
    
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Raw mirror has all user chunks using mdd. 
         * Set exported to 0 so we consider the exported chunks as valid. 
         */
        num_exported_chunks = 0;
    }
    else
    {
        num_exported_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
    
        if (chunk_index < (fbe_chunk_index_t) num_exported_chunks) 
        {
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunk index %d not in paged metadata area\n", __FUNCTION__, 
                (int)chunk_index);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    }

    /* If the chunk index is the last paged metadata chunk or if the chunk index is a multiple of
       chunks per md of md, then we can clear the NR bits in the md of md chunk 
     */
    total_chunks = fbe_raid_group_get_total_chunks(raid_group_p);


    /* Get the start and end chunks of md of md.
     */
    md_of_md_chunk_index = (chunk_index - (fbe_chunk_index_t) num_exported_chunks) / chunks_per_md_of_md; 

    if (md_of_md_chunk_index > (fbe_chunk_index_t) FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS) 
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunk index 0x%llx exceeds paged MD MD limit\n", __FUNCTION__, 
                               (unsigned long long)md_of_md_chunk_index);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    end_mdd_chunk_index = (chunk_index + chunk_count - 1 - (fbe_chunk_index_t) num_exported_chunks) / chunks_per_md_of_md;
    if (end_mdd_chunk_index > (fbe_chunk_index_t) FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: end chunk index 0x%llx exceeds paged MD MD limit\n", __FUNCTION__, 
                              (unsigned long long)end_mdd_chunk_index);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    if (md_of_md_chunk_index > end_mdd_chunk_index)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: start mdd index 0x%llx > end mdd index 0x%llx\n", __FUNCTION__, 
                              (unsigned long long)md_of_md_chunk_index,
                  (unsigned long long)end_mdd_chunk_index);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    /* Determine how many chunks worth of bits we should clear.
     */
    num_bits_to_clear = (fbe_u32_t)(end_mdd_chunk_index - md_of_md_chunk_index);

    /* Check if the last chunk can clear.  It can only clear if it is on a chunk boundary.
     */
    relative_chunk_index = chunk_index + chunk_count - 1 - (fbe_chunk_index_t) num_exported_chunks;
    if (((chunk_index + chunk_count) == total_chunks) ||  
        ((relative_chunk_index + 1) % chunks_per_md_of_md) == 0)
    {
        num_bits_to_clear++;
    }


    *clear_nr_bits_p = num_bits_to_clear;

    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_get_num_of_bits_we_can_clear_in_mdd()


/*!****************************************************************************
 * fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index()
 ******************************************************************************
 * @brief
 *   This function takes a "standard" chunk index as an input, which is relative
 *   to the start of the RAID group.   It gets the corresponding chunk index of 
 *   this chunk in the paged metadata metadata area of the nonpaged metadata.   
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_chunk_index            - input chunk index, which is relative to start 
 *                                    of the raid group 
 * @param out_chunk_index_p         - pointer that gets filled with the output chunk 
 *                                    index.  This is the chunk index in the non-paged 
 *                                    metadata's paged metadata metadata. 
 *
 * @return fbe_status_t            
 *
 * @author
 *   08/10/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_chunk_index_t           in_chunk_index, 
                                    fbe_chunk_index_t*          out_chunk_index_p)

{
    fbe_chunk_count_t               num_exported_chunks;
    fbe_chunk_count_t               chunks_per_md_of_md;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Raw mirror uses md of md for all chunks.
         */
        num_exported_chunks = 0;
    }
    else
    {
        //  The start chunk index passed in is relative to the start of the user area.  We need to get an offset relative 
        //  to the start of the chunks for the paged metadata.  So first get the number of chunks in the user data area.
        num_exported_chunks = fbe_raid_group_get_exported_chunks(in_raid_group_p);
    
        if (in_chunk_index < (fbe_chunk_index_t) num_exported_chunks) 
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunk index %d not in paged metadata area\n", __FUNCTION__, 
                (int)in_chunk_index);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    }

    /* Each md of md chunk might represent more than one physical chunk.
     */
    fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(in_raid_group_p, &chunks_per_md_of_md);

    /* Calculate the chunk index in the nonpaged area by subtracting off the number of chunks in the user data area.
     * Then we divide by the number of physical chunks per md of md chunk.
     */
    *out_chunk_index_p = (in_chunk_index - (fbe_chunk_index_t) num_exported_chunks) / chunks_per_md_of_md; 

    if (*out_chunk_index_p > (fbe_chunk_index_t) FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS) 
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunk index %llu exceeds paged MD MD limit\n", __FUNCTION__, 
            (unsigned long long)(*out_chunk_index_p));
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    //  Return success
    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index()


/*!****************************************************************************
 * fbe_raid_group_bitmap_translate_paged_md_md_index_to_rg_chunk_index()
 ******************************************************************************
 * @brief
 *   This function takes a paged metadata metadata chunk index as an input, which 
 *   is relative to the start of paged MD MD area in the nonpaged MD.  It gets the
 *   corresponding "standard" chunk index of this chunk in RAID group. 
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_chunk_index            - input chunk index, which is relative to start 
 *                                    of the paged metadata metadata 
 * @param current_checkpoint        - Current checkpoint.  Needed when there 
 * @param out_chunk_index_p         - pointer that gets filled with the output chunk 
 *                                    index.  This is the chunk index in the RAID 
 *                                    group.
 *
 * @return fbe_status_t            
 *
 * @author
 *   03/31/2011 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_translate_paged_md_md_index_to_rg_chunk_index(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_chunk_index_t           in_chunk_index, 
                                    fbe_lba_t                   current_checkpoint,
                                    fbe_chunk_index_t*          out_chunk_index_p)

{
    fbe_chunk_count_t   num_exported_chunks;
    fbe_chunk_count_t   chunks_per_md_of_md;
    fbe_chunk_index_t   current_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_index_t   start_rg_chunk_index;
    fbe_chunk_index_t   end_rg_chunk_index;

    fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(in_raid_group_p, &chunks_per_md_of_md);
                                   
    /* Make sure the chunk index passed in is actually a paged MD MD chunk index 
     */
    if (in_chunk_index >= (fbe_chunk_index_t) FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS) 
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: chunk index %d not in paged MD MD area\n", 
                              __FUNCTION__, (int)in_chunk_index);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Get the chunk index from the current checkpoint.  If we are at the
     * end marker we will use `in_chunk_index' to determine the next lba.
     */
    if (current_checkpoint != FBE_LBA_INVALID)
    {
        fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, current_checkpoint, &current_chunk_index);
    }

    /* Get the number of chunks in the user data area
     */
    num_exported_chunks = fbe_raid_group_get_exported_chunks(in_raid_group_p);

    /* Get the start and end raid group index.
     */
    start_rg_chunk_index = (in_chunk_index * chunks_per_md_of_md) + (fbe_chunk_index_t)num_exported_chunks;
    end_rg_chunk_index = start_rg_chunk_index + (fbe_chunk_index_t)chunks_per_md_of_md - 1;

    /*! @note Since a raid group can have multiple LUNs and there is no way to
     *        know that we have already marked the non-paged metadata of
     *        metadata, there will be cases where the non-paged index passed
     *        is below the current checkpoint.  If that is the case simply
     *        trace an informational message (since we are `resetting' the
     *        metadata verify checkpoint).
     */
    if (current_chunk_index > end_rg_chunk_index)
    {
        /* Trace an informational message.
         */
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "raid_group: translate md md index: 0x%llx chkpt: 0x%llx greater than index: 0x%llx\n", 
                              (unsigned long long)in_chunk_index, (unsigned long long)current_checkpoint, (unsigned long long)end_rg_chunk_index);
    }

    /* If the current chunk index is within the non-paged metadata of metadata
     * range, use that value.
     */
    if ((current_chunk_index >= start_rg_chunk_index) &&
        (current_chunk_index <= end_rg_chunk_index)      )
    {
        /* Use hte checkpoint to determine the chunk index.
         */
        *out_chunk_index_p = current_chunk_index;
    }
    else
    {
        /* Else use the starting non-paged index.
         */
        *out_chunk_index_p = start_rg_chunk_index;
    }

    /* Return success.
     */
    return FBE_STATUS_OK; 

} // End fbe_raid_group_bitmap_translate_paged_md_md_index_to_rg_chunk_index()


/*!****************************************************************************
 * fbe_raid_group_bitmap_get_lun_edge_extent_data()
 ******************************************************************************
 * @brief
 *   This function gets the chunk index and chunk count for a given LUN edge.
 *
 * @param in_raid_group_p       - The raid group.
 * @param in_lun_edge_p         - pointer to the LUN edge
 * @param out_chunk_index_p     - pointer to first bitmap chunk in LUN extent
 * @param out_chunk_count_p     - pointer to number of chunks in LUN extent
 *
 * @return  fbe_status_t
 *
 * @author
 *   11/30/2009 - Created. MEV
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_lun_edge_extent_data(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_block_edge_t*   in_lun_edge_p,
                                    fbe_chunk_index_t*  out_chunk_index_p,
                                    fbe_chunk_count_t*  out_chunk_count_p)
{
    fbe_raid_geometry_t*            raid_geometry_p = NULL;    // pointer to geometry of the raid object
    fbe_u16_t                       data_disks;         // The number of raid data disks
    fbe_lba_t                       lun_capacity;       // capacity of the LUN extent
    fbe_lba_t                       start_lba;          // raid lba where to start marking from
    fbe_status_t                    status = FBE_STATUS_OK; 


    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    // Calculate first chunk that is in the LUN edge
    start_lba = fbe_block_transport_edge_get_offset(in_lun_edge_p) / data_disks;

    // Align the start lba too.
    start_lba += fbe_block_transport_edge_get_offset(in_lun_edge_p) % data_disks;

    status = fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, start_lba, out_chunk_index_p);

    fbe_block_transport_edge_get_capacity(in_lun_edge_p, &lun_capacity);

    *out_chunk_count_p = (fbe_chunk_count_t)((lun_capacity/data_disks) / in_raid_group_p->chunk_size);

     // The block count could be less than the number of data disks. We should align it properly
    *out_chunk_count_p += (fbe_chunk_count_t)(lun_capacity % (data_disks * in_raid_group_p->chunk_size));
    return status;
    
}   // End fbe_raid_group_bitmap_get_lun_edge_extent_data()

/*!****************************************************************************
 * fbe_raid_group_bitmap_get_chunk_index_and_chunk_count()
 ******************************************************************************
 * @brief
 *   This function gets the chunk index and chunk count given the lun offset and
 *   block count.
 *
 * @param in_raid_group_p       - The raid group.
 * @param offset                - The lun offset
 * @param block count           - The lun block count
 * @param out_chunk_index_p     - pointer to first bitmap chunk in LUN extent
 * @param out_chunk_count_p     - pointer to number of chunks in LUN extent
 *
 * @return  fbe_status_t
 *
 * @author
 *   9/17/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_lba_t           offset,
                                    fbe_lba_t           block_count,
                                    fbe_chunk_index_t*  out_chunk_index_p,
                                    fbe_chunk_count_t*  out_chunk_count_p)
{
    fbe_raid_geometry_t*            raid_geometry_p = NULL;    // pointer to geometry of the raid object
    fbe_u16_t                       data_disks;         // The number of raid data disks    
    fbe_lba_t                       start_lba;          // raid lba where to start marking from
    fbe_status_t                    status = FBE_STATUS_OK; 
    fbe_lba_t                       end_lba;
    fbe_lba_t                       end_lba_per_disk;
    fbe_chunk_index_t               end_chunk_index;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    end_lba = offset + block_count - 1;

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    
    start_lba =  offset / data_disks;
    end_lba_per_disk = end_lba /data_disks;
    // Align the start lba with the chunk size
    start_lba += offset % data_disks;

    status = fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, start_lba, out_chunk_index_p);

    status = fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, end_lba_per_disk, &end_chunk_index);
    *out_chunk_count_p = (fbe_chunk_count_t)(end_chunk_index - *out_chunk_index_p + 1);

    /* This needs to be converted to error trace once the luns are aligned and the hack in fbe_lun_usurper_send_initiate_verify_to_raid
       is taken out - Ashwin */
    if(*out_chunk_count_p == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s: chunk count is zero. 0x%x\n", __FUNCTION__, 
            *out_chunk_count_p);

    }
    
    return status;
    
}   // End fbe_raid_group_bitmap_get_chunk_index_and_chunk_count()


/*!**************************************************************
 * fbe_raid_group_get_control_buffer_for_verify()
 ****************************************************************
 * @brief
 *  This function gets the buffer pointer out of the packet
 *  and validates it. This is sloghtly different from the
 *  fbe_raid_group_usurper_get_control_buffer. This function
 *  gets the present control operation.
 * 
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param packet_p       - Pointer to the packet.
 * @param in_buffer_length  - Expected length of the buffer.
 * @param out_buffer_pp     - Pointer to the buffer pointer. 
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  11/12/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_get_control_buffer_for_verify(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_payload_control_buffer_length_t in_buffer_length,
                                        fbe_payload_control_buffer_t*       out_buffer_pp)
{
    fbe_payload_ex_t*                  payload_p = NULL;
    fbe_payload_control_operation_t*    control_operation_p = NULL;  
    fbe_payload_control_buffer_length_t buffer_length;


    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_present_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        // The buffer pointer is NULL!
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer pointer is NULL\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    };

    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if (buffer_length != in_buffer_length)
    {
        // The buffer length doesn't match!
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer length mismatch\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

} //    End fbe_raid_group_get_control_buffer_for_verify()

/*!****************************************************************************
 * fbe_raid_group_verify_read_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function will initiate a read to the paged metadata for the verify flags
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   11/10/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_read_paged_metadata(fbe_packet_t * packet_p,                                    
                                         fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;
    fbe_raid_iots_t*                     iots_p = NULL;
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_status_t                         status; 
    fbe_lba_t                            verify_lba;
    fbe_lba_t                            end_lba;
    fbe_raid_verify_flags_t              verify_flag;
    fbe_chunk_index_t                    chunk_index;
    fbe_chunk_index_t                    end_chunk_index;
    fbe_chunk_count_t                    chunk_count;
    fbe_payload_block_operation_opcode_t block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_lba_t blocks;
    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Set the iots bit to indicate we got the NP lock
       The Iots cleanup will check whether the iots bits is set or not. If it is set, then 
       it has to release the NP lock before it releases the stripe lock
    */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_current_opcode(iots_p, &block_opcode);

    fbe_raid_iots_set_np_lock(iots_p, FBE_TRUE);

    //Convert the opcode into verify flags
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_flag);

    // Get the lba from the block operation and use it to get the chunk index
    fbe_raid_iots_get_lba(iots_p, &verify_lba);
    fbe_raid_iots_get_blocks(iots_p, &blocks);
    end_lba = verify_lba + blocks;

    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, verify_lba, &chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk_index);
    chunk_count = (fbe_chunk_count_t)(end_chunk_index - chunk_index);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_get_paged_metadata_completion, raid_group_p);
    status = fbe_raid_group_bitmap_get_next_marked_paged_metadata(raid_group_p, packet_p, chunk_index, chunk_count, 
                                                                  fbe_raid_group_get_next_chunk_marked_for_verify,
                                                                  &iots_p->current_opcode);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_raid_group_read_paged_metadata()

/*!****************************************************************************
 * fbe_raid_group_bitmap_is_chunk_marked_for_verify()
 ******************************************************************************
 * @brief
 *   This function determines if a chunk is marked for verify. It compares the
 *   specified verify flag(s) against those that are currently set in the 
 *   bitmap. It returns TRUE if any are set.
 *
 * @param in_raid_group_p       - The raid group
 * @param in_chunk_index        - the index of the chunk to check 
 * @param in_verify_flag        - verify flag(s) to check
 *
 * @return  fbe_bool_t          - TRUE if verify bit(s) set
 *
 * @author
 *   11/30/2009 - Created. MEV
 *
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_bitmap_is_chunk_marked_for_verify(
                                            fbe_raid_group_t*       in_raid_group_p,
                                            fbe_chunk_index_t       in_chunk_index,
                                            fbe_raid_verify_flags_t in_verify_flags)
{
    /*!@todo This function can be removed eventually if verify report has the 
      required data to calculate the verify status of the lun */
    return FBE_FALSE;
}
// End fbe_raid_group_bitmap_is_chunk_marked_for_verify()

/*!****************************************************************************
 * fbe_raid_group_bitmap_reconstruct_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function initiates reconstructing the paged metadata at the given LBA for the given number of blocks. 
 *   It uses the nonpaged metadata to reconstruct the paged metadata.
 * 
 *   Note that the reconstruction runs on the RG in the ACTIVATE state and only on the active SP.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param in_paged_md_lba           - start LBA in the paged metadata
 * @param in_paged_md_block_count   - number of paged metadata blocks to reconstruct
 * @param fbe_payload_block_operation_opcode_t - reconstruction opcode.
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *   03/28/2012 - Modified for opcode - Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_metadata(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       packet_p,
                                        fbe_lba_t                           in_paged_md_lba,
                                        fbe_block_count_t                    in_paged_md_block_count,
                                        fbe_payload_block_operation_opcode_t in_op_code)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_chunk_index_t                           paged_md_chunk_index;
    fbe_chunk_count_t                           paged_md_chunk_count;
    fbe_bool_t                                  is_in_data_area_b;
    fbe_raid_group_nonpaged_metadata_t*         nonpaged_metadata_p = NULL;
    fbe_bool_t                                  nonpaged_md_changed_b = FBE_FALSE;
    fbe_u64_t                                   metadata_offset;
    fbe_payload_ex_t*                           payload_p = NULL;
    fbe_payload_block_operation_t*              block_operation_p = NULL;

    fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: entry\n", __FUNCTION__); 
    /* Get the chunk range we need to reconstruct based on the incoming start lba and block count.
     */
    fbe_raid_group_bitmap_get_chunk_range(in_raid_group_p, 
                                          in_paged_md_lba, 
                                          in_paged_md_block_count, 
                                          &paged_md_chunk_index, 
                                          &paged_md_chunk_count);

    /* Make sure this range is in the paged metadata region of the RG.
     */
    fbe_raid_group_bitmap_determine_if_chunks_in_data_area(in_raid_group_p, paged_md_chunk_index, paged_md_chunk_count, &is_in_data_area_b);
    if (is_in_data_area_b)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s chunks not in paged metadata; chunk index %llu, chunk count %d\n", 
                              __FUNCTION__,
                  (unsigned long long)paged_md_chunk_index,
                  paged_md_chunk_count); 
        return status;
    }

    /* Allocate a block operation to store lba, block count and opcode.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    fbe_payload_block_build_operation(block_operation_p,
                                      in_op_code,
                                      in_paged_md_lba,
                                      in_paged_md_block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      0,
                                      NULL);

    // Increment the operation level
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    // Register completion function to release the block operation.
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_process_and_release_block_operation,
                                          in_raid_group_p);

    /* Get a pointer to the non-paged metadata.
     */
    fbe_raid_group_lock(in_raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    /* Determine if we need to update the nonpaged metadata and do so.
     * This is necessary if a verify or rebuild checkpoint, for example, falls in the range of user data corresponding 
     * to the paged metadata we need to reconstruct.
     */
    fbe_raid_group_bitmap_reconstruct_paged_update_nonpaged_if_needed(in_raid_group_p, 
                                                                      paged_md_chunk_index,
                                                                      paged_md_chunk_count,
                                                                      nonpaged_metadata_p,
                                                                      &nonpaged_md_changed_b);

    if (nonpaged_md_changed_b)
    {
        fbe_raid_group_unlock(in_raid_group_p);

        /* Update the nonpaged metadata via the Metadata Service.
         * Will update the paged metadata after the nonpaged via the completion function.
         */
        fbe_transport_set_completion_function(packet_p, 
                                              fbe_raid_group_bitmap_reconstruct_paged_md_nonpaged_write_completion, 
                                              in_raid_group_p);

        metadata_offset = 0;  /* we are writing all of the nonpaged metadata */
        fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) in_raid_group_p, packet_p, 
                                                        metadata_offset, 
                                                        (fbe_u8_t*)nonpaged_metadata_p,
                                                        sizeof(fbe_raid_group_nonpaged_metadata_t));

         return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_group_unlock(in_raid_group_p);

    /* The nonpaged metadata did not change, so just update the paged metadata.
     */
    fbe_raid_group_bitmap_reconstruct_paged_md_update_paged(in_raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
// End fbe_raid_group_bitmap_reconstruct_paged_metadata()

/*!****************************************************************************
 * fbe_raid_group_bitmap_reconstruct_paged_md_nonpaged_write_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for updating the nonpaged metadata as part of
 *   paged metadata reconstruction.
 *
 * @param packet_p   - packet that is completing
 * @param in_context    - completion context, which is a pointer to 
 *                        the raid group object
 * 
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_nonpaged_write_completion(
                                            fbe_packet_t*                      packet_p,
                                            fbe_packet_completion_context_t    in_context)
{
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_status_t                            packet_status;


    /* Get a pointer to the RG from the context.
     */
    raid_group_p = (fbe_raid_group_t *) in_context;

    /* Get the status of the operation from the packet.
     */
    packet_status = fbe_transport_get_status_code(packet_p);

    /* If the packet status was not a success, log a warning and return the status.
     */
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "rg_bmp_reconstruct_nonpaged_mdata_compl write fail,packet err %d, packet 0x%p\n", 
            packet_status, packet_p); 

        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: nonpaged update complete\n", __FUNCTION__); 

    /* Update the paged metadata from the nonpaged metadata.
     */
    fbe_raid_group_bitmap_reconstruct_paged_md_update_paged(raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
// End fbe_raid_group_bitmap_reconstruct_paged_md_nonpaged_write_completion()

/*!****************************************************************************
 * fbe_raid_group_bitmap_reconstruct_paged_md_update_paged()
 ******************************************************************************
 * @brief
 *   This function reconstructs the paged metadata from the nonpaged metadata and sends
 *   the write to the Metadata Service.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * 
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_update_paged(
                                fbe_raid_group_t*                   raid_group_p,
                                fbe_packet_t*                       packet_p)
{
    fbe_status_t                                status;
    fbe_memory_request_t *                      memory_request_p = NULL;
    fbe_payload_ex_t *                          payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *             block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* allocate memory for client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   2, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_raid_group_bitmap_reconstruct_paged_md_update_paged_allocate_completion,
                                   raid_group_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_raid_group_bitmap_reconstruct_paged_md_update_paged_allocate_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function after memory allocation.
 *   It reconstructs the paged metadata from the nonpaged metadata and sends
 *   the write to the Metadata Service.
 *
 * @param memory_request_p       - pointer to the memory request
 * @param context                - pointer to the raid group
 * 
 * @return fbe_status_t            
 *
 * @author
 *   10/02/2014 - Created. Lili Chen.
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_bitmap_reconstruct_paged_md_update_paged_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                     fbe_memory_completion_context_t context)
{
    fbe_raid_group_t *                              in_raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t*                               sep_payload_p = NULL;
    fbe_lba_t                                       lba;
    fbe_block_count_t                               block_count;
    fbe_chunk_index_t                               paged_md_chunk_index;
    fbe_chunk_count_t                               paged_md_chunk_count;
    fbe_chunk_index_t                               rg_user_data_start_chunk_index;
    fbe_chunk_count_t                               rg_user_data_chunk_count;
    fbe_raid_group_paged_metadata_t                 paged_metadata_bits;
    fbe_chunk_size_t                                chunk_size_in_blocks;
    fbe_u32_t                                       num_paged_md_recs_in_block;
    fbe_u32_t                                       num_paged_md_recs_in_chunk;
    fbe_chunk_count_t                               total_chunks;
    fbe_u32_t                                       aligned;
    fbe_payload_metadata_operation_t*               metadata_operation_p = NULL;       
    fbe_u64_t                                       metadata_offset;
    fbe_payload_block_operation_t                   *block_operation_p = NULL;
    fbe_raid_geometry_t *                           raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_u16_t                                       data_disks;
    fbe_packet_t *                                  packet_p;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s memory allocation failed, state:%d\n",
                                        __FUNCTION__, memory_request_p->request_state);

        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the lba and block count from the packet.  
     * This is the area of paged metadata to reconstruct.
     */
    lba = block_operation_p->lba;
    block_count = block_operation_p->block_count;

    /* Initialize the paged metadata record.
     */
    fbe_raid_group_metadata_init_paged_metadata(&paged_metadata_bits);

    /* Get the chunk range we need to reconstruct.
     * For now, we are reconstructing 1 chunk of paged metadata at a time.
     */
    fbe_raid_group_bitmap_get_chunk_range(in_raid_group_p, lba, block_count, &paged_md_chunk_index, &paged_md_chunk_count);

    /* Determine the user data chunk range the paged metadata chunk range corresponds to.
     */
    fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range(in_raid_group_p, 
                                                                               paged_md_chunk_index, 
                                                                               paged_md_chunk_count,
                                                                               &rg_user_data_start_chunk_index,
                                                                               &rg_user_data_chunk_count);

    /* Set the paged metadata bits from the nonpaged metadata for the given RG user data chunk range.
     */
    fbe_raid_group_bitmap_set_paged_bits_from_nonpaged(in_raid_group_p, 
                                                       rg_user_data_start_chunk_index, 
                                                       rg_user_data_chunk_count,
                                                       &paged_metadata_bits);
    
    /* Change to logical LBAs. AR 552482.
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    rg_user_data_start_chunk_index *= data_disks;
    rg_user_data_chunk_count *= data_disks;

    fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "RG: Reconstruct Paged idx/cnt: 0x%llx/0x%llx bits: v: 0x%x vr: 0x%x nr: 0x%x r:0x%x\n", 
                          (unsigned long long)rg_user_data_start_chunk_index, (unsigned long long)rg_user_data_chunk_count,
                          paged_metadata_bits.valid_bit, paged_metadata_bits.verify_bits,
                          paged_metadata_bits.needs_rebuild_bits, paged_metadata_bits.reserved_bits); 

    /* Allocate a metadata operation.
     */
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /*  Get the offset of this chunk in the metadata (ie. where we'll start to write to).
     */
    metadata_offset = rg_user_data_start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    /* Get the chunk size for our calculations.
     */
    chunk_size_in_blocks = fbe_raid_group_get_chunk_size(in_raid_group_p);

    /* Make sure we are aligned by the number of blocks in a chunk.
     * We do not want to do a pre-read in the Metadata Service for our write, so we need to be aligned.
     */
    aligned = metadata_offset % (chunk_size_in_blocks * FBE_METADATA_BLOCK_DATA_SIZE);
    if (aligned != 0)
    {
         fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_WARNING, 
             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
             "rg_bmp_reconstruct_paged_mdata_update: metadata_offset not aligned: 0x%llx\n", 
             (unsigned long long)metadata_offset); 
        
         fbe_memory_free_request_entry(memory_request_p);
         fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
         fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine the number of rg user data chunks to update. We may reconstruct more than we need
     * for alignment purposes.  Again, we do not want to do a pre-read in the Metadata Service for our write,
     * so we need to be aligned.  If our RG does not use all of the paged metadata chunk, we will fill 
     * in the rest as part of reconstructing the entire paged metadata chunk. 
     */
    num_paged_md_recs_in_block = FBE_METADATA_BLOCK_DATA_SIZE / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE;
    num_paged_md_recs_in_chunk = num_paged_md_recs_in_block * chunk_size_in_blocks;
    total_chunks = paged_md_chunk_count * num_paged_md_recs_in_chunk * data_disks;

    /* Make sure we are aligned by the number of blocks in a chunk.
     */
    aligned = (total_chunks * FBE_RAID_GROUP_CHUNK_ENTRY_SIZE) % (chunk_size_in_blocks * FBE_METADATA_BLOCK_DATA_SIZE);
    if (aligned != 0)
    {
         fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_WARNING, 
             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
             "rg_bmp_reconstruct_paged_mdata_update: total_chunks not aligned: 0x%llx\n", 
             (unsigned long long)metadata_offset); 
        
         fbe_memory_free_request_entry(memory_request_p);
         fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
         fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Depending on opcode build metadata request.
     */
    if(block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
    {

        fbe_payload_metadata_build_paged_verify_write(metadata_operation_p, 
                                                      &(((fbe_base_config_t *) in_raid_group_p)->metadata_element),
                                                      metadata_offset,
                                                      (fbe_u8_t *) &paged_metadata_bits,
                                                      sizeof(fbe_raid_group_paged_metadata_t),
                                                      total_chunks);
    }
    else
    {
        fbe_payload_metadata_build_paged_write_verify(metadata_operation_p, 
                                                      &(((fbe_base_config_t *) in_raid_group_p)->metadata_element),
                                                      metadata_offset,
                                                      (fbe_u8_t *) &paged_metadata_bits,
                                                      sizeof(fbe_raid_group_paged_metadata_t),
                                                      total_chunks);
    }

    /* Initialize cient_blob */
    status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, 0, FBE_FALSE);

    /* Populate the metadata stripe lock information.
     */
    fbe_raid_group_bitmap_metadata_populate_stripe_lock(in_raid_group_p,
                                                        metadata_operation_p,
                                                        total_chunks);

    /* Set the completion function for the paged metadata write. 
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_bitmap_reconstruct_paged_md_paged_write_completion, 
                                          in_raid_group_p);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);  

    /* Issue the metadata operation to write the paged metadata chunk.
     */
    status = fbe_metadata_operation_entry(packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} 
// End fbe_raid_group_bitmap_reconstruct_paged_md_update_paged()

/*!****************************************************************************
 * fbe_raid_group_bitmap_reconstruct_paged_md_paged_write_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for writing the paged metadata for reconstruction.
 *
 * @param packet_p   - packet that is completing
 * @param in_context    - completion context, which is a pointer to 
 *                        the raid group object
 * 
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_md_paged_write_completion(
                                            fbe_packet_t*                      packet_p,
                                            fbe_packet_completion_context_t    in_context)
{
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       sep_payload_p = NULL;               
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;        
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_block_operation_t           *block_operation_p;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qual;
    fbe_memory_request_t *                  memory_request_p = NULL;

    /* Get a pointer to the RG from the context.
     */
    raid_group_p = (fbe_raid_group_t *) in_context;

    packet_status = fbe_transport_get_status_code(packet_p);

    /*  Get the status of the metadata operation.
     */
    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);    

    /*  Release the metadata operation.
     */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Release client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    /* Translate metadata status to block status and set it in block operation.
     */
    fbe_metadata_translate_metadata_status_to_block_status(metadata_status, &block_status, &block_qual);
    fbe_payload_block_set_status(block_operation_p, block_status, block_qual);    

    /*  If the packet status or MD status was not a success log a trace.
     */
    if ((packet_status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "rg_bmp_reconstruct_paged_mdata_compl write fail,packet err %d, mdata stat %d, packet 0x%p\n", 
            packet_status, metadata_status, packet_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
            "rg_bmp_reconstruct_paged_mdata_compl: completed for lba: 0x%llx blocks: 0x%llx\n", 
            (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count);
    }

    return FBE_STATUS_OK;
}
// End fbe_raid_group_bitmap_reconstruct_paged_md_paged_write_completion()


/*!****************************************************************************
 * fbe_raid_group_bitmap_reconstruct_paged_update_nonpaged_if_needed()
 ******************************************************************************
 * @brief
 *   This function updates the non paged metadata based on the paged metadata area to reconstruct.
 *   The non paged metadata includes the verify checkpoints and the rebuild checkpoints.
 *   If a checkpoint falls in the user data range corresponding to the paged metadata
 *   range we need to reconstruct, we reset the checkpoint to the beginning of that range.
 *
 * @param in_raid_group_p                       - pointer to the raid group
 * @param in_paged_md_chunk_index               - start chunk of paged metadata
 * @param in_paged_md_chunk_count               - number of chunks of paged metadata
 * @param out_nonpaged_metadata_p               - pointer to the nonpaged metadata
 * @param out_nonpaged_md_changed_b             - boolean indicating if nonpaged md changed or not
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_update_nonpaged_if_needed(
                                        fbe_raid_group_t*                    in_raid_group_p,
                                        fbe_chunk_index_t                    in_paged_md_chunk_index,
                                        fbe_chunk_count_t                    in_paged_md_chunk_count,
                                        fbe_raid_group_nonpaged_metadata_t*  out_nonpaged_metadata_p,
                                        fbe_bool_t*                          out_nonpaged_md_changed_b)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_chunk_index_t                           rg_user_data_start_chunk_index;
    fbe_chunk_count_t                           rg_user_data_chunk_count;
    fbe_lba_t                                   rg_user_data_start_lba;
    fbe_lba_t                                   rg_user_data_end_lba;
    fbe_raid_position_bitmask_t		            rl_bitmask;
    fbe_u32_t                                   width; 
    fbe_u32_t                                   disk_position;
    fbe_lba_t                                   rebuild_checkpoint;
    fbe_u32_t                                   rebuild_data_index;


    /* Determine the user data chunk range the paged metadata chunk range corresponds to.
     */
    fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range(in_raid_group_p, 
                                                                               in_paged_md_chunk_index, 
                                                                               in_paged_md_chunk_count,
                                                                               &rg_user_data_start_chunk_index,
                                                                               &rg_user_data_chunk_count);

    /* Determine the LBA range for the rg user data chunk range.
     */
    fbe_raid_group_lba_to_chunk_range(in_raid_group_p, 
                                      rg_user_data_start_chunk_index,
                                      rg_user_data_chunk_count,
                                      &rg_user_data_start_lba,
                                      &rg_user_data_end_lba);

    /* Determine if any verify checkpoints are set and in the specified user data range of the RG. 
     * If so, set the checkpoint to the beginning of the user lba range.
     */ 
    if ((rg_user_data_start_lba <= out_nonpaged_metadata_p->rw_verify_checkpoint) &&
        (rg_user_data_end_lba >= out_nonpaged_metadata_p->rw_verify_checkpoint))
    {
        out_nonpaged_metadata_p->rw_verify_checkpoint = rg_user_data_start_lba;
        *out_nonpaged_md_changed_b = FBE_TRUE;
    }
    else if ((rg_user_data_start_lba <= out_nonpaged_metadata_p->ro_verify_checkpoint) &&
            (rg_user_data_end_lba >= out_nonpaged_metadata_p->ro_verify_checkpoint))
    {
        out_nonpaged_metadata_p->ro_verify_checkpoint = rg_user_data_start_lba;
        *out_nonpaged_md_changed_b = FBE_TRUE;
    }
    else if ((rg_user_data_start_lba <= out_nonpaged_metadata_p->error_verify_checkpoint) &&
            (rg_user_data_end_lba >= out_nonpaged_metadata_p->error_verify_checkpoint))
    {
        out_nonpaged_metadata_p->error_verify_checkpoint = rg_user_data_start_lba;
        *out_nonpaged_md_changed_b = FBE_TRUE;
    }
    else if ((rg_user_data_start_lba <= out_nonpaged_metadata_p->incomplete_write_verify_checkpoint) &&
            (rg_user_data_end_lba >= out_nonpaged_metadata_p->incomplete_write_verify_checkpoint))
    {
        out_nonpaged_metadata_p->incomplete_write_verify_checkpoint = rg_user_data_start_lba;
        *out_nonpaged_md_changed_b = FBE_TRUE;
    }
    else if ((rg_user_data_start_lba <= out_nonpaged_metadata_p->system_verify_checkpoint) &&
            (rg_user_data_end_lba >= out_nonpaged_metadata_p->system_verify_checkpoint))
    {
        out_nonpaged_metadata_p->system_verify_checkpoint = rg_user_data_start_lba;
        *out_nonpaged_md_changed_b = FBE_TRUE;
    }    

    /* Get the rebuild logging bitmask from nonpaged metadata.
     */
    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rl_bitmask);

    /* Get the width of raid group so we can loop through the disks.
     */ 
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    /* See if we are rebuilding.
     */
    for (disk_position = 0; disk_position < width; disk_position++)
    {
        /* See if we have a valid rebuild checkpoint for this position.
         */ 
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, disk_position, &rebuild_checkpoint);
        if (rebuild_checkpoint == FBE_LBA_INVALID)
        {
            continue;
        }

        /* Get the rebuild data index in the nonpaged metadata for this checkpoint.
         */
        fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(in_raid_group_p, disk_position, &rebuild_data_index);

        /* See if checkpoint falls in the specified user data range of the RG.
         */
        if ((rg_user_data_start_lba <= rebuild_checkpoint) &&
            (rg_user_data_end_lba >= rebuild_checkpoint))
        {    
            /* Make sure the checkpoint is at the beginning of this data area.
             * We will be reconstructing the paged metadata corresponding to this area.
             */
            out_nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_data_index].checkpoint = rg_user_data_start_lba;
            out_nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_data_index].position = disk_position;
            *out_nonpaged_md_changed_b = FBE_TRUE;         
        }
    }

    return status;
}
// End fbe_raid_group_bitmap_reconstruct_paged_update_nonpaged_if_needed()

/*!****************************************************************************
 * fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range()
 ******************************************************************************
 * @brief
 *   This function determines the range of user data chunks that corresponds to the given
 *   paged metadata chunk range.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_paged_md_chunk_index   - start chunk index of the paged metadata
 * @param in_paged_md_chunk_count   - number of paged metadata chunks
 * @param out_rg_user_data_start_chunk_index   - start chunk index of the user data 
 * @param out_rg_user_data_chunk_count   - number of user data chunks 
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_paged_md_chunk_index,
                                        fbe_chunk_count_t                   in_paged_md_chunk_count,
                                        fbe_chunk_index_t*                  out_rg_user_data_start_chunk_index,
                                        fbe_chunk_count_t*                  out_rg_user_data_chunk_count)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_chunk_size_t                        chunk_size_in_blocks;
    fbe_u32_t                               num_paged_md_recs_in_block;
    fbe_u32_t                               num_paged_md_recs_in_chunk;
    fbe_chunk_count_t                       num_exported_chunks;
    fbe_chunk_index_t                       paged_md_chunk_index_in_paged_area;
    fbe_chunk_count_t                       max_chunk_count;


    /* Determine the number of paged metadata records in a chunk.
     * Here is an example using the current values in SEP.
     *  There are 128 records in one block (num_paged_md_recs_in_block).
     *  There are 2048 blocks in one chunk (chunk_size_in_blocks).
     *  So there are 128 * 2048 records in one chunk.
     * Each paged metadata record represents one chunk of user data.
     */
    num_paged_md_recs_in_block = FBE_METADATA_BLOCK_DATA_SIZE / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE;
    chunk_size_in_blocks = fbe_raid_group_get_chunk_size(in_raid_group_p);
    num_paged_md_recs_in_chunk = num_paged_md_recs_in_block * chunk_size_in_blocks;

    /* Determine the number of exported chunks in the RG.
     * This is the user data chunks only.
     */
    num_exported_chunks = fbe_raid_group_get_exported_chunks(in_raid_group_p);

    /* The incoming paged metadata chunk index is relative to the start chunk of the RG.
     * Get the chunk index of the paged metadata area only, so relative to the start of the paged metadata area.
     * This is for our calculations.
     */
    paged_md_chunk_index_in_paged_area = in_paged_md_chunk_index - num_exported_chunks;

    /* Based on the number of paged metadata records per chunk, determine the rg chunk index
     * and chunk count.  Each paged metadata record represents one user data chunk.
     */

    *out_rg_user_data_start_chunk_index = paged_md_chunk_index_in_paged_area * num_paged_md_recs_in_chunk;

    max_chunk_count = num_paged_md_recs_in_chunk * in_paged_md_chunk_count;
    if (num_exported_chunks < max_chunk_count)
    {
        /* The number of exported chunks in the RG is less than the max represented in the paged metadata
         * chunks, so use the exported number of chunks for the RG user data chunk count.
         */
        *out_rg_user_data_chunk_count = num_exported_chunks;
        return status;
    }

    *out_rg_user_data_chunk_count = max_chunk_count;
    return status;
}
// End fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range()

/*!****************************************************************************
 * fbe_raid_group_bitmap_determine_lba_range_for_chunk_range()
 ******************************************************************************
 * @brief
 *   This function determines the start and end LBA for the chunk range.
 *
 * @param in_raid_group_p          - pointer to the raid group
 * @param in_start_chunk_index     - start chunk index  
 * @param in_chunk_count           - number of chunks
 * @param out_start_lba            - start lba for the chunk range 
 * @param out_end_lba              - end lba for the chunk range
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_bitmap_determine_lba_range_for_chunk_range(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_chunk_index,
                                        fbe_chunk_count_t                   in_chunk_count,
                                        fbe_lba_t*                          out_start_lba,
                                        fbe_lba_t*                          out_end_lba)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_chunk_index_t           last_chunk_index;
    fbe_lba_t                   start_lba;
    fbe_lba_t                   end_lba;
    fbe_chunk_size_t            chunk_size;  

    /* Get the start lba based on the start chunk index.
     */
    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p, in_chunk_index, &start_lba);

    /* Get the last chunk index.
     */
    last_chunk_index = in_chunk_index + in_chunk_count - 1;

    /* Get the starting lba of the last chunk.
     */
    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p, last_chunk_index, &end_lba);

    /* Add the chunk size then subtract 1
     */
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p);
    end_lba += (chunk_size - 1);

    *out_start_lba = start_lba;
    *out_end_lba = end_lba;

    return status;
}
// End fbe_raid_group_bitmap_determine_lba_range_for_chunk_range()

/*!****************************************************************************
 * fbe_raid_group_lba_to_chunk_range()
 ******************************************************************************
 * @brief
 *   This function determines the start and end LBA for the chunk range.
 *   It is special because we do not check the chunk range.
 *   There are cases (like reconstructing paged), where we do not want to
 *   validate the chunk range.
 *
 * @param raid_group_p          - pointer to the raid group
 * @param start_chunk_index     - start chunk index  
 * @param chunk_count           - number of chunks
 * @param start_lba            - start lba for the chunk range 
 * @param end_lba              - end lba for the chunk range
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Rob Foley copied from fbe_raid_group_bitmap_determine_lba_range_for_chunk_range()
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_lba_to_chunk_range(fbe_raid_group_t *raid_group_p,
                                  fbe_chunk_index_t chunk_index,
                                  fbe_chunk_count_t chunk_count,
                                  fbe_lba_t        *start_lba_p,
                                  fbe_lba_t        *end_lba_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_chunk_index_t           last_chunk_index;
    fbe_lba_t                   start_lba;
    fbe_lba_t                   end_lba;
    fbe_chunk_size_t            chunk_size;  

    /* Get the start lba based on the start chunk index.
     */
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, chunk_index, &start_lba);

    /* Get the last chunk index.
     */
    last_chunk_index = chunk_index + chunk_count - 1;

    /* Add the chunk size then subtract 1
     */
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    /* Get the starting lba of the last chunk.
     */
    end_lba = (fbe_lba_t) (last_chunk_index * chunk_size); 

    end_lba += (chunk_size - 1);

    *start_lba_p = start_lba;
    *end_lba_p = end_lba;

    return status;
}
/**************************************
 * end fbe_raid_group_lba_to_chunk_range()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_set_paged_bits_from_nonpaged()
 ******************************************************************************
 * @brief
 *   This function sets the paged metadata based on the nonpaged metadata.
 *
 * @param in_raid_group_p                       - pointer to the raid group
 * @param in_rg_user_data_chunk_index           - starting chunk
 * @param in_rg_user_data_chunk_count           - number of chunks
 * @param out_paged_md_bits_p                   - pointer to paged metadata bits to set
 *
 * @return fbe_status_t            
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken.
 *   04/2012 - Modified Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_set_paged_bits_from_nonpaged(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_chunk_index_t                   in_rg_user_data_chunk_index,
                                        fbe_chunk_count_t                   in_rg_user_data_chunk_count,
                                        fbe_raid_group_paged_metadata_t*    out_paged_md_bits_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_geometry_t                            *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_lba_t                                       rg_user_data_start_lba;
    fbe_lba_t                                       rg_user_data_end_lba;
    fbe_raid_group_nonpaged_metadata_t*             nonpaged_metadata_p = NULL;
    fbe_raid_position_bitmask_t		                rl_bitmask;
    fbe_u32_t                                       width; 
    fbe_u32_t                                       disk_position;
    fbe_lba_t                                       rebuild_checkpoint;
    fbe_u16_t                                       data_disks;
    fbe_lba_t                                       paged_md_start_lba = FBE_LBA_INVALID;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);  
    fbe_base_config_metadata_get_paged_record_start_lba(&(in_raid_group_p->base_config), 
                                                        &paged_md_start_lba);
    paged_md_start_lba /= data_disks;

    /* Determine the LBA range for the rg user data chunk range.
     */
    fbe_raid_group_lba_to_chunk_range(in_raid_group_p, 
                                      in_rg_user_data_chunk_index,
                                      in_rg_user_data_chunk_count,
                                      &rg_user_data_start_lba,
                                      &rg_user_data_end_lba);

    /* Get a pointer to the non-paged metadata.
     * We will reconstruct the paged metadata from the nonpaged metadata for the specified user data range.
     */
    fbe_raid_group_lock(in_raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    if ( (nonpaged_metadata_p->rw_verify_checkpoint != FBE_LBA_INVALID) &&
         (((rg_user_data_start_lba <= nonpaged_metadata_p->rw_verify_checkpoint) &&
           (rg_user_data_end_lba >= nonpaged_metadata_p->rw_verify_checkpoint)) ||
          (paged_md_start_lba <= nonpaged_metadata_p->rw_verify_checkpoint)) )
    {
        out_paged_md_bits_p->verify_bits |= FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
    }

    if ( (nonpaged_metadata_p->ro_verify_checkpoint != FBE_LBA_INVALID) &&
         (((rg_user_data_start_lba <= nonpaged_metadata_p->ro_verify_checkpoint) &&
           (rg_user_data_end_lba >= nonpaged_metadata_p->ro_verify_checkpoint)) ||
          (paged_md_start_lba <= nonpaged_metadata_p->ro_verify_checkpoint)) )
    {
        out_paged_md_bits_p->verify_bits |= FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
    }

    if ( (nonpaged_metadata_p->error_verify_checkpoint != FBE_LBA_INVALID) &&
         (((rg_user_data_start_lba <= nonpaged_metadata_p->error_verify_checkpoint) &&
           (rg_user_data_end_lba >= nonpaged_metadata_p->error_verify_checkpoint)) ||
          (paged_md_start_lba <= nonpaged_metadata_p->error_verify_checkpoint)) )
    {
        out_paged_md_bits_p->verify_bits |= FBE_RAID_BITMAP_VERIFY_ERROR;
    }

    if ( (nonpaged_metadata_p->incomplete_write_verify_checkpoint != FBE_LBA_INVALID) &&
         (((rg_user_data_start_lba <= nonpaged_metadata_p->incomplete_write_verify_checkpoint) &&
           (rg_user_data_end_lba >= nonpaged_metadata_p->incomplete_write_verify_checkpoint)) ||
          (paged_md_start_lba <= nonpaged_metadata_p->incomplete_write_verify_checkpoint)) )
    {
        out_paged_md_bits_p->verify_bits |= FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
    }

    if ( (nonpaged_metadata_p->system_verify_checkpoint != FBE_LBA_INVALID) && 
         (((rg_user_data_start_lba <= nonpaged_metadata_p->system_verify_checkpoint) &&
           (rg_user_data_end_lba >= nonpaged_metadata_p->system_verify_checkpoint)) ||
          (paged_md_start_lba <= nonpaged_metadata_p->system_verify_checkpoint)) )
    {
        out_paged_md_bits_p->verify_bits |= FBE_RAID_BITMAP_VERIFY_SYSTEM;
    }

    /* Everything below the checkpoint is already encrypted,
     * so mark the rekey bit for everything below, but not equal to the checkpoint.
     */
    if ( (nonpaged_metadata_p->encryption.rekey_checkpoint != FBE_LBA_INVALID) &&
         (nonpaged_metadata_p->encryption.rekey_checkpoint != 0) &&
         (rg_user_data_start_lba < nonpaged_metadata_p->encryption.rekey_checkpoint) &&
         (rg_user_data_end_lba < nonpaged_metadata_p->encryption.rekey_checkpoint) )
    {
        out_paged_md_bits_p->rekey = 1;
    }

    /* Get the rebuild logging bitmask from nonpaged metadata.
     */
    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rl_bitmask);

    /* Get the width of raid group so we can loop through the disks.
     */ 
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    /* See if we are rebuilding.
     */
    for (disk_position = 0; disk_position < width; disk_position++)
    {
        /* See if we have a valid rebuild checkpoint for this position.
         */ 
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, disk_position, &rebuild_checkpoint);
        if (rebuild_checkpoint == FBE_LBA_INVALID)
        {
            continue;
        }

        /* See if we need to update the nr bits in the paged metadata. This is the rl bitmask.
         */
        if ((rebuild_checkpoint <= rg_user_data_start_lba) ||
            (rebuild_checkpoint >= paged_md_start_lba))
        {
            /* Checkpoint is below the user data area corresponding to the paged metadata area to rebuild.
             */
            out_paged_md_bits_p->needs_rebuild_bits |= (1 << disk_position);
        }
        /* When the rebuild log bitmap is set always set the nr bits.
         */
        if (rl_bitmask & (1 << disk_position))
        {
            out_paged_md_bits_p->needs_rebuild_bits |= (1 << disk_position);
        }
    }

    fbe_raid_group_unlock(in_raid_group_p);

    return status;
}
// End fbe_raid_group_bitmap_set_paged_bits_from_nonpaged()

/*!****************************************************************************
 * fbe_raid_group_mark_journal_for_verify()
 ******************************************************************************
 * @brief
 *   This function will get the NP lock and mark the journal area for verify
 * 
 * @param packet_p
 * @param raid_group_p
 * @param raid_lba
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/14/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_mark_journal_for_verify(
                                            fbe_packet_t*               packet_p,
                                            fbe_raid_group_t*           raid_group_p,
                                            fbe_lba_t                   raid_lba)
{

    fbe_raid_iots_t                  *iots_p = NULL;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_geometry_t*             raid_geometry_p = NULL;
    fbe_u16_t                        data_disks;
    fbe_lba_t                        disk_lba;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    disk_lba = raid_lba/data_disks;

    /* Get the iots to save the bitmap.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* Save the information on the in the iots. 
     * This is needed since we still need to fetch the np dist lock, which is 
     * asynchronous. 
     */
    fbe_raid_iots_set_lba(iots_p, disk_lba);

     
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Getting the NP to mark journal verify\n",
                          __FUNCTION__);

    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_journal_verify);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
} // End fbe_raid_group_mark_journal_for_verify()


/*!****************************************************************************
 * fbe_raid_group_mark_journal_for_verify_completion()
 ******************************************************************************
 * @brief
 *       This is the completion function for marking for journal verify as 
 *       part of remap request. This will remove the event from the queue
 *      and complete the event
 * 
 * @param packet_p           - pointer to a control packet from the scheduler
 * @param context
 * 
 * @return fbe_status_t        
 *
 * @author
 *   03/14/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_mark_journal_for_verify_completion(
                                                           fbe_packet_t*       packet_p,
                                                           fbe_packet_completion_context_t  context)

{
    fbe_status_t                            packet_status;
    fbe_raid_group_t*                       raid_group_p = NULL;
    fbe_event_t*                            data_event_p = NULL;
    fbe_event_stack_t*                      current_stack = NULL;


    raid_group_p = (fbe_raid_group_t*)context;

    fbe_base_config_event_queue_pop_event((fbe_base_config_t*)raid_group_p, &data_event_p);
    if(data_event_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: No event in the queue\n",
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    current_stack = fbe_event_get_current_stack(data_event_p);
    fbe_event_release_stack(data_event_p, current_stack);

    packet_status = fbe_transport_get_status_code(packet_p);

    if(packet_status != FBE_STATUS_OK)
    {
        fbe_event_set_status(data_event_p, FBE_EVENT_STATUS_GENERIC_FAILURE);
    }
    else
    {
        fbe_event_set_status(data_event_p, FBE_EVENT_STATUS_OK);
    }

    fbe_event_complete(data_event_p);

    return FBE_STATUS_OK;


} // End fbe_raid_group_mark_journal_for_verify_completion


/*!****************************************************************************
 * fbe_raid_group_journal_verify()
 ******************************************************************************
 * @brief
 *   This function will mark it for journal verify
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   03/14/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_verify(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;   
    fbe_status_t                         status;
    fbe_raid_group_nonpaged_metadata_t*  non_paged_metadata_p = NULL;
    fbe_lba_t                            journal_verify_checkpoint;
    fbe_lba_t                            new_checkpoint;
    fbe_raid_iots_t                      *iots_p = NULL;
    fbe_payload_ex_t                     *sep_payload_p;
    fbe_u64_t                            metadata_offset = FBE_LBA_INVALID;
    fbe_chunk_size_t                     chunk_size;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get the iots */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_lba(iots_p, &new_checkpoint);
    
    /* Make sure checkpoint is aligned */    
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    if ((new_checkpoint % chunk_size) != 0)  /* not aligned.  fix it.*/
    {        
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s chkpt:0x%x not aligned.  Adjusting it\n",
                              __FUNCTION__, (unsigned int)new_checkpoint);
        new_checkpoint = (new_checkpoint / chunk_size) * chunk_size;  /* adjust to previous chunk */
    }

    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->journal_verify_checkpoint);

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);  
      
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&non_paged_metadata_p);
    journal_verify_checkpoint = non_paged_metadata_p->journal_verify_checkpoint;
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Marking for journal verify. chkpt:0x%x\n",
                          __FUNCTION__, (unsigned int)new_checkpoint);
    
    if(journal_verify_checkpoint == FBE_LBA_INVALID)
    {
        fbe_base_config_metadata_nonpaged_set_checkpoint_persist((fbe_base_config_t *)raid_group_p,
                                                                  packet_p,
                                                                  metadata_offset,
                                                                  0, /* There is only (1) checkpoint for verify.*/
                                                                  new_checkpoint);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    }

    /* If the existing checkpoint is not invalid then no need to persist the checkpoint
       Just call set checkpoint api. 
    */
    status = fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                      packet_p,
                                                      metadata_offset,
                                                      0, /* There is only (1) checkpoint for a verify */
                                                      new_checkpoint);


    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_raid_group_journal_verify()


/*!****************************************************************************
 *  fbe_raid_group_plant_sgl_with_zero_buffer()
 ******************************************************************************
 * @brief
 *  This function is used to plant the sgl with data.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   03/19/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_plant_sgl_with_zero_buffer(fbe_sg_element_t * sg_list,
                                          fbe_u32_t sg_list_count,
                                          fbe_block_count_t block_count,
                                          fbe_u32_t * used_sg_elements_p)
{
    fbe_u32_t       zero_bit_bucket_size_in_blocks;
    fbe_u32_t       zero_bit_bucket_size_in_bytes;
    fbe_u8_t *      zero_bit_bucket_p = NULL;
    fbe_status_t    status;

    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_zero_bit_bucket(&zero_bit_bucket_p, &zero_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (zero_bit_bucket_p == NULL))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* convert the zero bit bucket size in bytes. */
    zero_bit_bucket_size_in_bytes = zero_bit_bucket_size_in_blocks * FBE_BE_BYTES_PER_BLOCK;

    /* update the passed zod sg list with zero bufffer if required. */
    while(block_count != 0)
    {
        if((*used_sg_elements_p) < (sg_list_count))
        {
            sg_list->address = zero_bit_bucket_p;
            if(block_count >= zero_bit_bucket_size_in_blocks)
            {
                sg_list->count = zero_bit_bucket_size_in_bytes;
                block_count -= zero_bit_bucket_size_in_blocks;
            }
            else
            {
                if((block_count * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
                {
                    /* sg count should not exceed 32bit limit
                     */
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                sg_list->count = (fbe_u32_t)(block_count * FBE_BE_BYTES_PER_BLOCK);
                block_count = 0;
            }
            sg_list++;
            (*used_sg_elements_p)++;
        }
        else
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* update the last entry with null and count as zero. */
    sg_list->address = NULL;
    sg_list->count = 0;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_plant_sgl_with_zero_buffer()
 ******************************************************************************/


/*!****************************************************************************
 *  fbe_raid_group_handle_metadata_error()
 ******************************************************************************
 * @brief
 *  This function is used handle the metdata error. If it is uncorrectable
 *  error, set the activate condition
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   03/19/2012 - Created. Ashwin Tamilarasan from
 *                fbe_raid_group_bitmap_get_paged_metadata_completion
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_handle_metadata_error(fbe_raid_group_t*    raid_group_p,
                                     fbe_packet_t*        packet_p,
                                     fbe_payload_metadata_status_t   metadata_status)
                                          
{

    fbe_status_t                 new_packet_status;

    //  There was a failure in the metadata operation.  Set the packet to a status corresponding to the type of
    //  metadata failure. 
    fbe_raid_group_bitmap_translate_metadata_status_to_packet_status(raid_group_p, metadata_status, 
                                                                     &new_packet_status);
    fbe_transport_set_status(packet_p, new_packet_status, 0);

    if(new_packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "paged I/O failed packet stat set to %d,mdata stat %d pkt: %p\n",
            new_packet_status,  metadata_status, packet_p);
    }
    if (new_packet_status == FBE_STATUS_IO_FAILED_NOT_RETRYABLE)
    {
        // set the condition to transition the RG to the ACTIVATE state.
        //  In the ACTIVATE state, we will verify the paged metadata and try to reconstruct it.
      fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                             FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);
      /* If we are already in activate then make sure we re-set the paged verify condition so 
       * we will get a chance to fix these errors. 
       */
      fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                             FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA);

      /* For drive failures we are setting eval rb logging activate since we have seen issues where it looks 
       * like the edge transition was not processed.  This will prevent the logs from filling during encryption 
       * if the iw check fails. 
       */
      fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                             FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE);
    }

    //  Return the packet status
    return FBE_STATUS_OK;

}

/******************************************************************************
 * end fbe_raid_group_handle_metadata_error()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_rg_bitmap_update_chkpt_for_unconsumed_chunks()
 ******************************************************************************
 * @brief
 *   This function will just update the checkpoint for unconsumed chunks
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   4/05/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_rg_bitmap_update_chkpt_for_unconsumed_chunks(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;   
    fbe_status_t                         status;
    fbe_payload_ex_t*                    sep_payload_p;
    fbe_raid_verify_flags_t              verify_flags;
    fbe_payload_block_operation_t        *block_operation_p;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    // Convert the opcode into verify flags
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_operation_p->block_opcode, &verify_flags);

    status = fbe_raid_group_update_verify_checkpoint(packet_p, raid_group_p, block_operation_p->lba, block_operation_p->block_count,
                                                     verify_flags, __FUNCTION__);
                                                                                
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_rg_bitmap_update_chkpt_for_unconsumed_chunks()

/*!****************************************************************************
 * fbe_raid_group_bitmap_determine_nr_bits_from_nonpaged()
 ******************************************************************************
 * @brief
 *   This function determines the correct value of the NR bitmask using the
 *   non-paged values.
 *
 * @param in_raid_group_p                       - pointer to the raid group
 * @param in_rg_user_data_chunk_index           - starting chunk
 * @param in_rg_user_data_chunk_count           - number of chunks
 * @param needs_rebuild_bits_p                  - pointer to needs rebuild mask
 *
 * @return fbe_status_t            
 *
 * @author
 *  03/13/2013  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_bitmap_determine_nr_bits_from_nonpaged(fbe_raid_group_t *in_raid_group_p,
                                                                   fbe_chunk_index_t in_rg_user_data_chunk_index,
                                                                   fbe_chunk_count_t in_rg_user_data_chunk_count,
                                                                   fbe_raid_position_bitmask_t *needs_rebuild_bits_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_u32_t                           width; 
    fbe_u32_t                           rebuild_index;
    fbe_lba_t                           rebuild_checkpoint;
    fbe_u32_t                           rebuild_position;

    /* Default to no positions that need rebuild.
     */
    *needs_rebuild_bits_p = 0;

    /* If the non-paged metadata is not initialized yet, then return the
     * default value with a status indicating such.
     */
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s nonpaged not initialized \n",
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Get the width of raid group so we can loop through the disks.
     */ 
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    /* Get a pointer to the non-paged metadata.
     * We will reconstruct the paged metadata from the nonpaged metadata for the specified user data range.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    /*! @note Currently we don't determine the physical LBA range so that we 
     *        can compare against the rebuild checkpoint.  We always set
     *        needs rebuild for any position that needs to be rebuilt.
     */
    /* 
      fbe_raid_group_bitmap_determine_lba_range_for_chunk_range(in_raid_group_p, 
                                                                in_rg_user_data_chunk_index,
                                                                in_rg_user_data_chunk_count,
                                                                &rg_user_data_start_lba,
                                                                &rg_user_data_end_lba);
     */

    /* All positions that are rebuild logging must be marked needs rebuild.
     */
    *needs_rebuild_bits_p = nonpaged_metadata_p->rebuild_info.rebuild_logging_bitmask;

    /* Loop through the rebuild checkpoints to see if the checkpoint is set for
     * any position.
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If the rebuild checkpoint is valid check if the request is above
         * or below the checkpoint.  If the checkpoint is in the paged metadata 
         * area then the rebuild position needs to be marked needs rebuild. 
         */
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint != FBE_LBA_INVALID)
        {
            /* If the position is bad report the error.
             */
            rebuild_checkpoint = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint;
            rebuild_position = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].position;
            if (rebuild_position >= width)
            {
                /* Reset the needs rebuild and trace the error.
                 */
                *needs_rebuild_bits_p = 0;
                fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s rebuild index: %d has invalid pos: %d greater than width: %d \n",
                                      __FUNCTION__, rebuild_index, rebuild_position, width);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /*! @note Currently we don't try to determine if the request is above 
             *        or below the rebuild checkpoint.  Always mark this position
             *        as needs rebuild.
             */
            *needs_rebuild_bits_p |= (1 << rebuild_position);
        }

    } /* for all rebuilding positions */

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_raid_group_bitmap_determine_nr_bits_from_nonpaged()
 **************************************************************/
/*************************
 * end file fbe_raid_group_bitmap.c
 *************************/
