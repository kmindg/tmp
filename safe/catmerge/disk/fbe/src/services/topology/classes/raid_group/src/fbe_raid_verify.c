/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group object's background verify code.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   09/17/2009:  Created. MEV
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_verify.h"
#include "fbe_raid_group_bitmap.h"      // bitmap accessor functions 
#include "fbe_raid_library.h"
#include "fbe_raid_group_logging.h"     //  for logging verify complete/lun start/lun complete 
#include "EmcPAL_Misc.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

 
static fbe_bool_t fbe_raid_verify_analyze_error_counts(fbe_raid_group_t* in_raid_group_p);

static fbe_status_t fbe_raid_verify_send_verify_report_to_lun(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_event_verify_report_t*      in_verify_event_data_p,
                                                fbe_lba_t                       verify_lba);

static fbe_status_t fbe_raid_verify_send_verify_report_to_lun_completion(fbe_event_t* in_event_p, 
                                                                        fbe_event_completion_context_t in_context);

static fbe_status_t fbe_raid_group_get_lun_edge_from_lba(fbe_raid_group_t* in_raid_group_p,
                                                         fbe_lba_t in_lba,
                                                         fbe_block_edge_t** out_lun_edge_pp);


static fbe_status_t fbe_raid_group_ask_verify_permission_completion(fbe_event_t * in_event_p,
                                                                    fbe_event_completion_context_t in_context);

static fbe_status_t fbe_raid_verify_check_errors_and_send_verify_report_event(
                                        fbe_packet_t*                   in_packet_p,
                                        fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_raid_group_verify_process_mark_verify_event_completion(fbe_packet_t* in_packet_p,
                                                                               fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_raid_group_post_clear_verify_for_chunk(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_completion(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_updated_nonpaged(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context);

fbe_status_t fbe_raid_group_verify_unconsumed_advance_checkpoint(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p, 
                                                 fbe_payload_block_operation_opcode_t    op_code,
                                                 fbe_lba_t start_lba, 
                                                 fbe_block_count_t block_count);
fbe_raid_state_status_t fbe_raid_group_restart_block_operation_packet(fbe_raid_iots_t *iots_p);

static fbe_status_t fbe_raid_group_verify_mark_metadata_of_metadata(fbe_packet_t * packet_p, 
                                                                    fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_verify_mark_metadata_of_metadata_completion(fbe_packet_t * packet_p, 
                                                                               fbe_packet_completion_context_t in_context);
static fbe_status_t fbe_raid_group_verify_mark_paged_metadata_completion(fbe_packet_t * packet_p, 
                                                                         fbe_packet_completion_context_t in_context);
static fbe_status_t fbe_raid_group_verify_mark_checkpoint(fbe_packet_t * packet_p, 
                                                          fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_bitmap_initiate_verify_checkpoint_update(fbe_packet_t *packet_p,
                                                                            fbe_raid_group_t* raid_group_p);
static fbe_status_t fbe_raid_group_verify_mark_checkpoint_completion(fbe_packet_t * packet_p, 
                                                                     fbe_packet_completion_context_t in_context);
static fbe_status_t fbe_raid_group_verify_get_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_verify_release_stripe_lock(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_verify_release_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_verify_mark_iw_verify_completion(fbe_packet_t* in_packet_p, fbe_packet_completion_context_t in_context);
static fbe_status_t fbe_raid_group_initiate_lun_extent_init_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                                         fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_initiate_lun_extent_init_metadata_completion(fbe_packet_t * packet_p,
                                                                                fbe_packet_completion_context_t context);


/***************
 * FUNCTIONS
 ***************/


/*!****************************************************************************
 * fbe_raid_run_user_verify()
 ******************************************************************************
 * @brief
 *  This function is called by the raid group monitor when the
 *  DO_VERIFY condition is set. It runs a single cycle of the 
 *  user background verify operation.
 *
 * @param in_raid_group_p   - The pointer to the raid group.
 * @param in_packet_p       - The control packet sent to us from the scheduler.
 * @param in_checkpoint     - Verify checkpoint - It could be abort verify, error verify
                              RW verify or read only checkpoint
 * @param in_verify_flag    - Verify flag to indicate the type of verify
 *
 * @return fbe_status_t     - Always returns FBE_STATUS_OK
 *
 * @author
 *  09/18/2009 - Created. MEV
 *  04/13/2010 - Modified Ashwin
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_run_user_verify(fbe_raid_group_t*   in_raid_group_p,
                                            fbe_packet_t*       in_packet_p,
                                            fbe_lba_t           in_checkpoint,
                                            fbe_raid_verify_flags_t in_verify_flag)
{
   fbe_status_t                            status;
    

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);
              
    if (in_checkpoint == 0)
    {
        in_raid_group_p->background_start_time = fbe_get_time();
    }

    // Just ask upstream whether the lun is consumed or not.
    // If it consumed send it to executor to do the verify
    status = fbe_raid_group_ask_upstream_for_permission(in_packet_p,
                                                        in_raid_group_p,
                                                        in_checkpoint,
                                                        in_verify_flag); 
   
    return status;

}  // End fbe_raid_run_user_verify()


/*!****************************************************************************
 * fbe_raid_verify_update_metadata()
 ******************************************************************************
 * @brief
 *  This is the completion function for the user verify io
 *  operation.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  09/25/2009 - Created. MEV
 *  04/13/2010 - Modified Ashwin
 *
 *****************************************************************************/
fbe_status_t fbe_raid_verify_update_metadata(fbe_packet_t* in_packet_p, 
                                             fbe_raid_group_t* in_raid_group_p)
{    
    
    fbe_payload_ex_t*                       payload_p;
    fbe_raid_iots_t*                        iots_p;
    fbe_lba_t                               verify_lba;    
    fbe_status_t                            status;
    fbe_lba_t                       end_lba;                    // end LBA to clear NR
    fbe_chunk_index_t               start_chunk;                // first chunk to clear NR 
    fbe_chunk_index_t               end_chunk;                  // last chunk to clear NR 
    fbe_chunk_count_t               chunk_count;                // number of chunks to clear
    fbe_bool_t                          is_in_data_area_b;                  // true if chunk is in the data area
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_block_count_t                       blocks;

    // Get the payload and block operation for this I/O operation
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);  
    block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_lba(iots_p, &verify_lba);

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine how many blocks to update. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &blocks);

    //  Get the ending LBA to be marked, by calculating it from the start address and the block count 
    end_lba = verify_lba + blocks - 1;

    //  Convert the starting and ending LBAs to chunk indexes 
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, verify_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, end_lba, &end_chunk);

    //  Calculate the number of chunks to be marked 
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);
    
    //  Determine if the chunk(s) to be updated are in the user data area or the paged metadata area.  If we want
    //  to update a chunk in the user area, we use the paged metadata service to do that.  If we want to update a 
    //  chunk in the paged metadata area itself, we use the nonpaged metadata to do that (in the metadata data
    //  portion of it).
    status = fbe_raid_group_bitmap_determine_if_chunks_in_data_area(in_raid_group_p,
                                                                    start_chunk,
                                                                    chunk_count,
                                                                    &is_in_data_area_b);

    if(!is_in_data_area_b)
    {
        fbe_raid_group_verify_initiate_non_paged_metadata_clear_bits(in_packet_p, in_raid_group_p, verify_lba, blocks);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    // Get the status of the operation that just completed.
    status = fbe_transport_get_status_code(in_packet_p);

    if( (status == FBE_STATUS_OK) &&
        (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
    {
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_post_clear_verify_for_chunk, in_raid_group_p);

        fbe_raid_group_bitmap_clear_verify_for_chunk(in_packet_p, in_raid_group_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    // Operation failed.
    fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s verify I/O operation failed, status: 0x%x block status: 0x%x\n",
                            __FUNCTION__, status, block_status);



    /* Complete the packet so that
      fbe_raid_verify_check_errors_and_send_verify_report_event
      will be called
    */
    //completion function into the stack
    fbe_transport_set_completion_function(in_packet_p,
                                          fbe_raid_verify_check_errors_and_send_verify_report_event,
                                          in_raid_group_p);
    fbe_transport_complete_packet(in_packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
}  // End fbe_raid_verify_update_metadata()


/*!****************************************************************************
 * fbe_raid_group_post_clear_verify_for_chunk()
 ******************************************************************************
 * @brief
 *  This is the completion function that is invoked after clearing verify in
 *  paged metadtata.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  06/13/2012 - Created. Prahlad Purohit
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_post_clear_verify_for_chunk(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t                        *raid_group_p = (fbe_raid_group_t *) in_context;
    fbe_payload_ex_t                        *sep_payload_p;
    fbe_payload_block_operation_t           *block_operation_p;
    fbe_lba_t                               start_lba; 
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_verify_flags_t                 verify_flags;
    fbe_block_count_t                       block_count;
    fbe_status_t                            status;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_iots_t*                        iots_p;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);  

    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    status = fbe_transport_get_status_code(in_packet_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Block operation packet_status: 0x%x block_status:0x%x block_qual: 0x%x",
                                __FUNCTION__, status, block_status, block_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_raid_geometry_has_paged_metadata(raid_geometry_p) &&
        (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Block operation packet_status: 0x%x block_status:0x%x block_qual: 0x%x",
                                __FUNCTION__, status, block_status, block_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    start_lba = block_operation_p->lba;
    block_opcode = block_operation_p->block_opcode;

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine the new checkpoint. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    // Convert the opcode into verify flags
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_flags);

    // Log BV complete message for LUN if needed
    fbe_raid_group_logging_log_lun_bv_start_or_complete_if_needed(raid_group_p, in_packet_p, start_lba);

    //completion function into the stack
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_raid_verify_check_errors_and_send_verify_report_event,
                                          raid_group_p);

    status = fbe_raid_group_update_verify_checkpoint(in_packet_p, raid_group_p, start_lba, block_count,
                                                     verify_flags, __FUNCTION__);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_raid_verify_check_errors_and_send_verify_report_event()
 ******************************************************************************
 * @brief
 *   This function will determine whether there was any verify errors or
 *   whether verify has been completed on a lun or not. If either of the condition
 *   is true it will send a verify report event to the lun
 *
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/12/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_verify_check_errors_and_send_verify_report_event(
                                        fbe_packet_t*                      in_packet_p,
                                        fbe_packet_completion_context_t    in_context)

{
    fbe_raid_group_t*                       raid_group_p;
    fbe_bool_t                              is_error_b;         // TRUE if any verfiy errors occurred
    fbe_bool_t                              is_pass_b;          // TRUE if a verify pass completed
    fbe_chunk_index_t                       first_chunk;        // the first chunk in the LUN edge
    fbe_chunk_count_t                       chunk_count;        // the number of chunks in the LUN edge
    fbe_chunk_count_t                       number_of_chunks;
    fbe_chunk_index_t                       current_chunk; 
    fbe_chunk_index_t                       end_chunk_index; 
    fbe_lba_t                               lba;
    fbe_lba_t                               end_lba;
    fbe_block_count_t                       block_count;
    fbe_block_edge_t*                       lun_edge_p;
    fbe_payload_ex_t*                       payload_p;
    fbe_raid_iots_t*                        iots_p;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_geometry_t*                    raid_geometry_p = NULL;
    fbe_raid_verify_tracking_t*             verify_ts_p;
    fbe_raid_group_type_t                   raid_type; 
    fbe_u16_t                               data_disks;

                  
    raid_group_p  = (fbe_raid_group_t*) in_context;

    // Initialize local variables
    is_pass_b = FALSE;

    // Get the payload and block operation for this I/O operation
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);          
    fbe_raid_iots_get_lba(iots_p, &lba);
    fbe_raid_iots_get_blocks(iots_p, &block_count);
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_opcode(iots_p, &block_opcode);  
    
    end_lba = lba + block_count;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);    
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    // Raid types other than the internal mirror 
    // are treated differently
    if(raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        // Get the current chunk that was just verified
        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, lba, &current_chunk);
        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk_index);
        number_of_chunks = (fbe_chunk_count_t)(end_chunk_index - current_chunk);

        //Determine the lun edge based on the lba
        fbe_raid_group_get_lun_edge_from_lba(raid_group_p, lba, &lun_edge_p);
    
        /* If the lun is already unbound */
        if(lun_edge_p == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "rd_verify_chkerr_sendrpt lba %llx not consumed \n",
                   (unsigned long long)lba); 
            return FBE_STATUS_OK; 
        }

        // This data disks is part of the verify event data.
        // This is used to differeentiate between RAID 10 and other types of lun
        // For all raid types other than 10, this field will not be used hence populated with 0
        data_disks = 0;
            
        // Get the bitmap chunk index and count for this LUN edge
        fbe_raid_group_bitmap_get_lun_edge_extent_data(raid_group_p, lun_edge_p, &first_chunk, &chunk_count);
    
        // Determine if a pass completed on the associated LUN edge
         if ( ((current_chunk + number_of_chunks -1 ) >= (first_chunk + chunk_count - 1)) &&
             (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
             (block_opcode!=FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) )
    
        {
            // The chunk that just complted is the last chunk in the LUN extent
            is_pass_b = FBE_TRUE;
        }
    }

    // If the raid type is RAID 10 and if we find out that this is the
    // end of lun then set the is_pass_b = true
    else
    {
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
        verify_ts_p = fbe_raid_get_verify_ts_ptr(raid_group_p);
        if( (verify_ts_p ->is_lun_end_b) &&
            (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
             (block_opcode!=FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) )
        {
            is_pass_b = FBE_TRUE;
        }
    }
    

    // Determine if there were any verify errors.
    is_error_b = fbe_raid_verify_analyze_error_counts(raid_group_p);

    if ((is_error_b == TRUE) || (is_pass_b == TRUE))
    {                                      

        fbe_raid_verify_error_counts_t* error_counts_p;
        fbe_event_verify_report_t      verify_event_data;  // verify data to send to LUN
        
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rd_verify_chkerr_sendrpt: Err or pass completed. Err %d, Pass %d, lba 0x%llx\n",
                              is_error_b, is_pass_b, (unsigned long long)lba);
        
        // Setup the event data
        error_counts_p = fbe_raid_group_get_verify_error_counters_ptr(raid_group_p);
        verify_event_data.error_counts = *error_counts_p;
        verify_event_data.pass_completed_b = is_pass_b;
        // In case of RAID 10 the data disks will let the lun object know
        // how many mirror objects are connected to the striper object
        verify_event_data.data_disks = data_disks;

        // Sending a verify report event to the lun
        fbe_raid_verify_send_verify_report_to_lun(raid_group_p, in_packet_p, &verify_event_data, lba);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } 
    
    
    return FBE_STATUS_OK; 


}  // End fbe_raid_verify_check_errors_and_send_verify_report_event()


/*!****************************************************************************
 * fbe_raid_group_verify_verify_event_completion()
 ******************************************************************************
 * @brief
 *  This is the completion method for forwarding a verify report event upstream.
 *
 * @param in_packet_p - Pointer to packet to complete             
 * @param in_context - Packet context                    
 * 
 * @return - status - The status of the event packet transmision
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_verify_verify_event_completion(fbe_packet_t *in_packet_p,
                                                             fbe_packet_completion_context_t in_context)
{
    fbe_status_t    status;

    /* retrieve the status of the packet */
    status = fbe_transport_get_status_code(in_packet_p);
    fbe_transport_release_packet(in_packet_p);

    return status;
}  // End of fbe_raid_group_verify_verify_event_completion()*/

/*!****************************************************************************
 * fbe_raid_group_set_verify_checkpoint()
 ******************************************************************************
 * @brief
 *      This function sets the verify checkpoint to specified value for given 
 *      verify type.
 * 
 * @param packet_p     - Pointer to originating packet
 * @param raid_group_p - Pointer to raid group
 * @param verify_falgs - Verify flags.
 * @param value        - LBA value to set checkpoint to.
 * 
 * @return fbe_status_t 
 *
 * @note The caller must hold NP lock.
 *
 * @author
 *   03/05/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_set_verify_checkpoint(fbe_raid_group_t        *raid_group_p,
                                                  fbe_packet_t            *packet_p,
                                                  fbe_raid_verify_flags_t verify_flags,
                                                  fbe_lba_t               value)
{
    fbe_status_t                                  status = FBE_STATUS_OK; 
    fbe_u64_t                                     metadata_offset = 0;
  
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
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&value,
                                                             sizeof(fbe_lba_t));
    if(FBE_STATUS_GENERIC_FAILURE == status)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Failed to set verify chkpt to zero\n", __FUNCTION__);
    }    

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}// End fbe_raid_group_set_verify_checkpoint

/*!****************************************************************************
 * fbe_raid_verify_np_clear_verify_bits_completion()
 ******************************************************************************
 * @brief
 *   This function is called when non paged metada verify bits clear operation
 *   has completed. Depending upon the status, it will either complete the packet
 *   or update the verify checkpoint  
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   03/05/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_verify_np_clear_verify_bits_completion(fbe_packet_t                    *in_packet_p,
                                                fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*) in_context;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_lba_t                               start_lba = FBE_LBA_INVALID;
    fbe_chunk_index_t                       next_md_of_md_index = FBE_CHUNK_INDEX_INVALID;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_block_count_t                       block_count;
    fbe_raid_verify_flags_t                 verify_flags;
    fbe_raid_iots_t*                        iots_p = NULL;

    /* Get the payload */
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* Get the status of the packet. */
    status = fbe_transport_get_status_code(in_packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: clearing non-paged metadata failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(in_packet_p, status, 0);
        return status;
    }

    /* get the start lba */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_opcode(iots_p, &block_opcode);
    fbe_raid_iots_get_lba(iots_p, &start_lba);
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    status = fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Convert of verify opcode 0x%x failed status: 0x%x\n",
                              __FUNCTION__, block_opcode, status);
    }

    /* Get the next metadata chunk to see if it is valid. If it is valid then
     * we have more chunks to verify so set verify checkpoint/ Otherwise we
     * have completed metadata verify so increment the pass count & set verify 
     * check point to zero to start verify on user data region.   */
    status = fbe_raid_group_verify_get_next_metadata_of_metadata_chunk_index(raid_group_p,
                                                                        &next_md_of_md_index,
                                                                        verify_flags);
    if (next_md_of_md_index == FBE_CHUNK_INDEX_INVALID)
    {
        fbe_raid_group_inc_paged_metadata_verify_pass_count(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Verify on paged metadata completed\n",
                             __FUNCTION__);
    
        status = fbe_raid_group_set_verify_checkpoint(raid_group_p, in_packet_p, verify_flags, 0);
    }
    else
    {
        status = fbe_raid_group_update_verify_checkpoint(in_packet_p, raid_group_p, start_lba, block_count, verify_flags, __FUNCTION__);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!****************************************************************************
 * fbe_raid_group_verify_initiate_non_paged_metadata_clear_bits()
 ******************************************************************************
 * @brief
 *  This function updates the paged metadata of metadata bitmap using the 
 *  metadata service 
 *
 * @param in_packet_p       - The pointer to the packet.
 * @param in_raid_group_p   - The pointer to the raid group.
 *
 * @return fbe_status_t
 *
 * @author
 *  01/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t  fbe_raid_group_verify_initiate_non_paged_metadata_clear_bits(fbe_packet_t*     in_packet_p,
                                                                           fbe_raid_group_t* in_raid_group_p,
                                                                           fbe_lba_t verify_lba,
                                                                           fbe_block_count_t blocks)
{
    fbe_status_t                            status;
    fbe_raid_group_paged_metadata_t         paged_metadata_metadata[FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS];
    fbe_u64_t                               metadata_offset;       
    fbe_chunk_count_t                       chunk_count;    
    fbe_payload_ex_t*                       payload_p;
    fbe_raid_iots_t*                        iots_p;   
    fbe_lba_t                               end_lba;                    // end LBA to clear MD verify
    fbe_chunk_index_t                       start_chunk;                // first chunk to clear MD Verify
    fbe_chunk_index_t                       end_chunk;                  // last chunk to clear MD verify
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_verify_flags_t                 verify_flags;
    fbe_chunk_index_t                       paged_md_md_start_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_index_t                       np_md_index;
    fbe_u32_t                               num_bits_to_clear = 0;
    fbe_raid_geometry_t                    *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_group_nonpaged_metadata_t     *non_paged_metadata_p;
    fbe_bool_t                              b_need_to_clear_mdd = FBE_FALSE;
    fbe_chunk_count_t                       chunks_per_md_of_md = 1;
    fbe_u32_t                               pmd_chunks_per_mdd_chunk;

    /* Get the nonpaged data
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)in_raid_group_p, (void **)&non_paged_metadata_p);

    // Get the payload and block operation for this I/O operation
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_raid_iots_get_opcode(iots_p, &block_opcode);

    //  Get the ending LBA to be marked, by calculating it from the start address and the block count 
    end_lba = verify_lba + blocks - 1;

    //  Convert the starting and ending LBAs to chunk indexes 
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, verify_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, end_lba, &end_chunk);

    chunk_count = (fbe_chunk_count_t)(end_chunk - start_chunk + 1);

    //  The chunk indices passed in is relative to the start of the user area.  Get the chunk index relative 
    //  to the start of the chunks of the paged metadata. 
    fbe_raid_group_bitmap_translate_rg_chunk_index_to_paged_md_md_index(in_raid_group_p, 
                                                                        start_chunk, 
                                                                        &paged_md_md_start_chunk_index);

    // Get the number of verify bits to clear in mdd.
    fbe_raid_group_bitmap_get_phy_chunks_per_md_of_md(in_raid_group_p, &chunks_per_md_of_md);
    pmd_chunks_per_mdd_chunk = chunks_per_md_of_md;
    fbe_raid_group_bitmap_get_num_of_bits_we_can_clear_in_mdd(in_raid_group_p,
                                                              start_chunk,
                                                              chunk_count,
                                                              &num_bits_to_clear);
    status = fbe_raid_group_verify_convert_block_opcode_to_verify_flags(in_raid_group_p, block_opcode, &verify_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_np_mdd_clear_bits: Convert of verify opcode 0x%x failed status: 0x%x\n",
                              block_opcode, status);
    }

    /* If there is `no' paged metadata, we still check the bits.
     */
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p) &&
         (num_bits_to_clear > 0)                                  )
    {
        for (np_md_index = paged_md_md_start_chunk_index; np_md_index < (paged_md_md_start_chunk_index + num_bits_to_clear); np_md_index++)
        {
            if ((non_paged_metadata_p->paged_metadata_metadata)[np_md_index].verify_bits & verify_flags)
            {
                b_need_to_clear_mdd = FBE_TRUE;
                break;
            }
        }
        if (!b_need_to_clear_mdd)
        {
            num_bits_to_clear = 0;
        }
    }
    
    /* If verify tracing is enabled.
     */
    fbe_raid_group_trace(in_raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                         "rg_np_mdd_clear_bits: pmd per mdd: %d clear: %d bits lba: 0x%llx blks: 0x%llx index: 0x%llx count: %d\n",
                         pmd_chunks_per_mdd_chunk, num_bits_to_clear, (unsigned long long)verify_lba, (unsigned long long)blocks, (unsigned long long)paged_md_md_start_chunk_index, chunk_count);

    /* Only if there are bits to clear.
     */
    if ((paged_md_md_start_chunk_index + num_bits_to_clear) > FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS)
    {
        /* Request is byond non-paged metadata of metadata.
         */
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_np_mdd_clear_bits: np mdd index: 0x%llx bits: %d past max: %d\n",
                              (unsigned long long)paged_md_md_start_chunk_index, num_bits_to_clear, FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (num_bits_to_clear != 0)
    {
        //  Set up the bits of the metadata that need to be written, which are the verify bits 
        fbe_set_memory(&paged_metadata_metadata, 0, sizeof(fbe_raid_group_paged_metadata_t) * FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS);

        // To clear all the verify bits OR all the verify bits which is equivalent to 0xf
        for (np_md_index = paged_md_md_start_chunk_index; np_md_index < (paged_md_md_start_chunk_index + num_bits_to_clear); np_md_index++)
        {
            paged_metadata_metadata[np_md_index].verify_bits = verify_flags;
        }

        //  Set up the offset of the starting chunk in the metadata 
        metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->paged_metadata_metadata[paged_md_md_start_chunk_index]);

        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_np_mdd_clear_bits: np md index: 0x%llx num bits to clear: %d flags: 0x%x op: %d\n",
                              (unsigned long long)paged_md_md_start_chunk_index, num_bits_to_clear, verify_flags, block_opcode);

        if (fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
        {
            fbe_transport_set_completion_function(in_packet_p, fbe_raid_verify_np_clear_verify_bits_completion,
                                                  in_raid_group_p);
        }
        else
        {
            fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_post_clear_verify_for_chunk, in_raid_group_p);
        }

        /* Conditional trace the clear bits request.
         */
        fbe_raid_group_trace(in_raid_group_p, 
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                             "rg_np_mdd_clear_bits: offset: 0x%llx slots: %d verify bits: 0x%x\n", 
                             metadata_offset, num_bits_to_clear, verify_flags);

        // Clear the required number of verify bits.
        fbe_base_config_metadata_nonpaged_clear_bits((fbe_base_config_t*) in_raid_group_p, in_packet_p,
                                                      metadata_offset, (fbe_u8_t*) &paged_metadata_metadata[paged_md_md_start_chunk_index], 
                                                      sizeof(fbe_raid_group_paged_metadata_t), 
                                                      num_bits_to_clear);

    }
    else
    {
        /* If enabled trace when we don't clear.  This can happen when (1) non-paged
         * chunk info represents multiple paged metadata chunks.
         */
        for (np_md_index = paged_md_md_start_chunk_index; np_md_index < (paged_md_md_start_chunk_index + chunk_count); np_md_index++)
        {
            if ((non_paged_metadata_p->paged_metadata_metadata)[np_md_index].verify_bits & verify_flags)
            {
                /* Trace an informational trace if enabled.
                 */
                fbe_raid_group_trace(in_raid_group_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                         "rg_np_mdd_clear_bits: bits 0 pmd per mdd: %d: mdd index: %d flags: 0x%x lba: 0x%llx blks: 0x%llx\n",
                         pmd_chunks_per_mdd_chunk, (int)np_md_index, (non_paged_metadata_p->paged_metadata_metadata)[np_md_index].verify_bits, (unsigned long long)verify_lba, (unsigned long long)blocks);
                break;
            }
        }

        // There are no verify bits to clear just update the verify check point.
        fbe_raid_group_update_verify_checkpoint(in_packet_p, in_raid_group_p, verify_lba, blocks, verify_flags, __FUNCTION__);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;    
}  // End fbe_raid_group_verify_initiate_non_paged_metadata_clear_bits()

/*!****************************************************************************
 * fbe_raid_verify_analyze_error_counts()
 ******************************************************************************
 * @brief
 *  This function analyzes the verify error counters from the
 *  previous verify I/O operation.
 *
 * @param in_raid_group_p   - The pointer to the raid group.
 *
 * @return fbe_bool_t - TRUE if any errors were found.
 *
 * @author
 *  10/19/2009 - Created. MEV
 *
 ******************************************************************************/
static fbe_bool_t fbe_raid_verify_analyze_error_counts(fbe_raid_group_t* in_raid_group_p)
{
    fbe_raid_verify_error_counts_t* verify_errors_p;
    fbe_u32_t                       sum;


    // Get a pointer to the verify error counters
    verify_errors_p = fbe_raid_group_get_verify_error_counters_ptr(in_raid_group_p);

    // Calculate the total number of verify errors
    sum  = verify_errors_p->u_crc_count;
    sum += verify_errors_p->c_crc_count;
    sum += verify_errors_p->u_coh_count;
    sum += verify_errors_p->c_coh_count;
    sum += verify_errors_p->u_ts_count;
    sum += verify_errors_p->c_ts_count;
    sum += verify_errors_p->u_ws_count;
    sum += verify_errors_p->c_ws_count;
    sum += verify_errors_p->u_ss_count;
    sum += verify_errors_p->c_ss_count;
    sum += verify_errors_p->u_ls_count;
    sum += verify_errors_p->c_ls_count;
    sum += verify_errors_p->u_media_count;
    sum += verify_errors_p->c_media_count;
    sum += verify_errors_p->c_soft_media_count;

    // Analyze the error counters
    if (sum > 0)
    {
        // Found some errors
        return TRUE;
    }

    // No errors found
    return FALSE;

}  // End fbe_raid_verify_analyze_error_counts()


/*!****************************************************************************
 * fbe_raid_verify_send_verify_report_to_lun()
 ******************************************************************************
 * @brief
 *  This function sends the verify error counts to the associated
 *  LUN object to update its verify report.
 *
 * @param in_raid_group_p           - pointer to the raid group.
 * @param in_packet_p               - control packet sent to us from the scheduler.
 * @param in_verify_event_data_p    - pointer to event data to send
 *
 * @return fbe_status_t
 *
 * @author
 *  10/20/2009 - Created. MEV
 *  03/31/2010 - Modified Ashwin 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_verify_send_verify_report_to_lun(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_event_verify_report_t*      in_verify_event_data_p,
                                                fbe_lba_t                       verify_lba)
{
    fbe_event_t *                   event_p = NULL;
    fbe_event_stack_t *             event_stack = NULL;    
    fbe_lba_t                       actual_lba;
    fbe_block_count_t               block_count; 
    fbe_raid_geometry_t*            raid_geometry_p = NULL;
    fbe_u16_t                       data_disks; 
    
    fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));

    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_base_object_trace( (fbe_base_object_t *)in_raid_group_p,
            FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s unable to allocate event",__FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, in_packet_p);
    event_stack = fbe_event_allocate_stack(event_p);
    

    // Get the chunk size
    block_count = fbe_raid_group_get_chunk_size(in_raid_group_p);

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    block_count = block_count * data_disks;

    // Convert the verify lba which is disk based to lun lba to send it up
    fbe_raid_group_convert_disk_lba_to_lun_lba(in_raid_group_p, verify_lba, &actual_lba);      

    /* Fill LBA range */
    fbe_event_stack_set_extent(event_stack, actual_lba, block_count);
    
    // Populate the event data and send the event
    fbe_event_set_verify_report_data(event_p, in_verify_event_data_p);

    fbe_event_stack_set_completion_function(event_stack, fbe_raid_verify_send_verify_report_to_lun_completion, in_raid_group_p);

    fbe_base_config_send_event((fbe_base_config_t *)in_raid_group_p, FBE_EVENT_TYPE_VERIFY_REPORT, event_p);

    return FBE_STATUS_OK;

}  // End fbe_raid_verify_send_verify_report_to_lun()


/*!****************************************************************************
 * fbe_raid_verify_send_verify_report_to_lun_completion()
 ******************************************************************************
 * @brief
 *  This is the verify report event completion function. Retrieve the packet
 *  and complete the packet 
 *
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/20/2009 - Created. mvalois
 *  03/31/2010 - Modified Ashwin 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_verify_send_verify_report_to_lun_completion(fbe_event_t * in_event_p,
                                                                         fbe_event_completion_context_t in_context)
{

    fbe_event_status_t                      event_status = FBE_EVENT_STATUS_INVALID;
    fbe_event_stack_t *                     event_stack = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_raid_group_t *                      raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t *) in_context;

    // get the data out of the event before we free it.
    event_stack = fbe_event_get_current_stack(in_event_p);
    fbe_event_get_master_packet(in_event_p, &packet_p);
    fbe_event_get_status(in_event_p, &event_status);
    
    
    fbe_event_release_stack(in_event_p, event_stack);
    fbe_event_destroy(in_event_p);
    fbe_memory_ex_release(in_event_p);

    
    /* We do not have to check for event status
       Regardless of the event status - complete the packet */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

}  // End fbe_raid_verify_send_verify_report_to_lun_completion()



/*!****************************************************************************
 * fbe_raid_group_get_lun_edge_from_lba()
 ******************************************************************************
 * @brief
 *  This function determines the LUN edge based on the LBA parameter passed in to this function.
 *
 *  It internally converts the LBA to a RAID based value 
 *  in order to properly search the LUN extents on the upper edge.
 *
 * @param in_raid_group_p   - Pointer to the raid group
 * @param in_lba            - lba to determine the lun edge.                            
 * @param out_lun_edge      - Pointer pointer to the LUN block edge                            
 *
 * @return fbe_status_t 
 *
 *
 * @todo: remove this function once support for LUN data events is added. 
 *
 * @author
 *  02/25/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_get_lun_edge_from_lba(fbe_raid_group_t* in_raid_group_p,
                                                         fbe_lba_t in_lba,
                                                         fbe_block_edge_t** out_lun_edge_pp)
{
    fbe_raid_geometry_t*            raid_geometry_p;
    fbe_block_transport_server_t*   block_transport_server_p;
    fbe_u16_t                       data_disks;
    fbe_lba_t                       actual_lba;
    fbe_object_id_t                 out_lun_object_id;

    // Get the pointer to the RAID block transport server
    block_transport_server_p = &in_raid_group_p->base_config.block_transport_server;

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    actual_lba = in_lba * data_disks;

    fbe_block_transport_find_edge_and_obj_id_by_lba(block_transport_server_p, actual_lba, &out_lun_object_id, out_lun_edge_pp);

    return FBE_STATUS_OK;

}  // End fbe_raid_group_get_lun_edge_from_lba()

/*!**************************************************************
 * fbe_raid_group_ask_verify_permission()
 ****************************************************************
 * @brief
 *  This function is used to allocate the event and send it to LUN
 *  object to see whether the lba that we are going to verfiy is 
 *  in the lun or not
 * 
 * @param raid_group_p    - raid group object.
 * @param packet_p        - FBE packet pointer.
 * @param start_lba    - lba to be verfied
 * @param block_count  - number of blocks to be verified 
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 02/23/2010 - Created. guov
 * 
 ****************************************************************/
fbe_status_t 
fbe_raid_group_ask_verify_permission(fbe_raid_group_t * raid_group_p,
                                     fbe_packet_t * packet_p,                                    
                                     fbe_lba_t start_lba,
                                     fbe_block_count_t block_count)
{
    fbe_event_t *                   event_p = NULL;
    fbe_event_stack_t *             event_stack = NULL;
    fbe_lba_t                       lun_lba; 
    fbe_block_count_t               logical_block_count;    // block count relative to the whole RG extent
    fbe_raid_geometry_t*            raid_geometry_p;        // pointer to raid geometry info 
    fbe_u16_t                       data_disks;             // number of data disks in the rg
    fbe_event_permit_request_t      permit_request_info;


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    //  Create the event
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
            FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s unable to allocate event",__FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack = fbe_event_allocate_stack(event_p);

    //  Initialize the permit request information to "no LUN found"
    fbe_event_init_permit_request_data(event_p, &permit_request_info,
                                       FBE_PERMIT_EVENT_TYPE_IS_CONSUMED_REQUEST);

    // Convert the verify lba which is disk based to lun lba to send it up
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    lun_lba   = start_lba * data_disks; 
    logical_block_count = block_count * data_disks;

    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                          "%s event_stack lba: 0x%llx blocks: 0x%llx\n",
                          __FUNCTION__, (unsigned long long)lun_lba, (unsigned long long)logical_block_count);
    /* Fill LBA range */
    fbe_event_stack_set_extent(event_stack, lun_lba, logical_block_count);

    fbe_event_stack_set_completion_function(event_stack, fbe_raid_group_ask_verify_permission_completion, raid_group_p);

    fbe_base_config_send_event((fbe_base_config_t *)raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);

    return FBE_STATUS_OK;

}  // end fbe_raid_group_ask_verify_permission()


/*!**************************************************************
 * fbe_raid_group_ask_verify_permission_completion()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the event to
 *  ask for verify permission to LUN object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 02/23/2010 - Created the intial version. guov
 * 
 ****************************************************************/
static fbe_status_t fbe_raid_group_ask_verify_permission_completion(fbe_event_t * in_event_p,
                                                                    fbe_event_completion_context_t in_context)
{
    fbe_event_stack_t *                     event_stack = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_u32_t                               event_flags = 0;
    fbe_lba_t                               start_lba;
    fbe_lba_t                               lun_lba;
    fbe_block_count_t                       logical_block_count;    
    fbe_block_count_t                       block_count;
    fbe_block_count_t                       unconsumed_block_count;
    fbe_raid_verify_flags_t                 verify_flags_from_bitmap;    
    fbe_payload_block_operation_opcode_t    op_code;
    fbe_status_t                            status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_event_status_t                      event_status = FBE_EVENT_STATUS_INVALID;
    fbe_event_permit_request_t              permit_request_info;     
    fbe_raid_verify_tracking_t*             verify_ts_p = NULL;             
    fbe_raid_geometry_t*                    raid_geometry_p = NULL; 
    fbe_u16_t                               data_disks;
    fbe_bool_t                              is_io_consumed;
    fbe_block_count_t                       io_block_count = FBE_LBA_INVALID;    

    raid_group_p = (fbe_raid_group_t *) in_context;
    // get the data out of the event before we free it.
    event_stack = fbe_event_get_current_stack(in_event_p);
    fbe_event_get_master_packet(in_event_p, &packet_p);
    fbe_event_get_flags (in_event_p, &event_flags);
    fbe_event_get_status(in_event_p, &event_status);

    // get the range
    fbe_event_stack_get_extent(event_stack, &lun_lba, &logical_block_count);

    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                          "%s event_stack lba: 0x%llx blocks: 0x%llx status: 0x%x flags: 0x%x\n",
                          __FUNCTION__, (unsigned long long)lun_lba, (unsigned long long)logical_block_count, event_status, event_flags);
    
    // Convert the verify lba which is lun based to disk lba
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    block_count   = logical_block_count / data_disks; 

    //  Get the permit request info about the LUN (object id and start/end of LUN) and store it in the
    //  verify tracking structure
    fbe_event_get_permit_request_data(in_event_p, &permit_request_info); 
    verify_ts_p = fbe_raid_get_verify_ts_ptr(raid_group_p);
    verify_ts_p->is_lun_start_b    = permit_request_info.is_start_b;
    verify_ts_p->is_lun_end_b      = permit_request_info.is_end_b;
    unconsumed_block_count         = permit_request_info.unconsumed_block_count;

    // Get the verify flags from the verify tracking structure
    verify_flags_from_bitmap = fbe_raid_group_get_verify_flags(raid_group_p); 

    // Determine the type of verify operation to perform.
    fbe_raid_group_verify_convert_verify_flags_to_opcode(raid_group_p, &op_code, verify_flags_from_bitmap);

    fbe_event_release_stack(in_event_p, event_stack);
    fbe_event_destroy(in_event_p);
    fbe_memory_ex_release(in_event_p);

    // Convert the lun lba back to verify lba which is disk based
    fbe_raid_group_convert_lun_lba_to_disk_lba(raid_group_p, lun_lba, &start_lba); 


    /* complete the packet if permit request denied or returned busy */
    if((event_flags == FBE_EVENT_FLAG_DENY) || (event_status == FBE_EVENT_STATUS_BUSY))
    {
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                              "raid_group: verify permit event request denied. lba: 0x%llx blocks: 0x%llx status: %d flags: 0x%x\n",
                             start_lba, block_count, event_status, event_flags);

        // our client is busy, we will need to try again later, we can't do the rebuild now
        // Complete the packet
        fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_OK;
    }

    if(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID == op_code)
    {
        // No valid flag bits are set.
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: %d invalid verify flags, flags: 0x%x\n",
                              __FUNCTION__, __LINE__, verify_flags_from_bitmap);

        // Complete the packet.
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }


    /* Handle all the possible event status.
     */
    switch(event_status)
    {
        case FBE_EVENT_STATUS_OK:
        
            io_block_count = block_count;

            /* check if the verify IO range is overlap with unconsumed area either at beginning or at the end */
            status = fbe_raid_group_monitor_io_get_consumed_info(raid_group_p,
                                                    unconsumed_block_count,
                                                    start_lba, 
                                                    &io_block_count,
                                                    &is_io_consumed);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO, "raid_group: verify failed to determine IO consumed area\n");
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);

                return FBE_STATUS_OK;
            }

            if(is_io_consumed)
            {
                if(block_count != io_block_count)
                {
                    /* IO range is consumed at the beginning and unconsumed at the end
                     * update the verify block count with updated block count which only cover the 
                     * consumed area
                     */

                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "verify overlap with unconsumed area at end, lba 0x%llx, " 
                            "orig_b 0x%llx, (unsigned long long)new_b 0x%llx\n",
                            start_lba, block_count, io_block_count);                    

                    block_count = io_block_count;
                    
                }

                // Set the I/O completion function and start the verify I/O operation.
                fbe_transport_set_completion_function(packet_p, fbe_raid_group_process_and_release_block_operation, raid_group_p);

                fbe_raid_group_trace(raid_group_p,
                                     FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                                     "%s lba: 0x%llx blocks: 0x%llx\n",
                                     __FUNCTION__, (unsigned long long)start_lba, (unsigned long long)block_count);

                // Send the verify packet to the raid executor
                // !@todo - the code needs to handle an error from this call 
                fbe_raid_group_executor_break_context_send_monitor_packet(raid_group_p, packet_p,
                                                                          op_code, start_lba, block_count);  

                return FBE_STATUS_OK;
            }
            else
            {
                /* IO range is unconsumed at the beginning and consumed at the end
                 * so can't continue with verify
                 * advance the verify checkpoint.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                            "verify overlap with unconsumed area at start, lba 0x%llx, " 
                            "orig_b 0x%llx, (unsigned long long)new_b 0x%llx\n",
                             start_lba, block_count, io_block_count); 
                
                block_count = io_block_count;

                /* advance the verify checkpoint */
                status = fbe_raid_group_verify_unconsumed_advance_checkpoint(raid_group_p, packet_p,  op_code, start_lba, block_count);
                return status;

            }

            break;

        case FBE_EVENT_STATUS_NO_USER_DATA:

            /* We can have this event status if lun was unbound in the middle of BV */
            fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                             "%s: LBA 0x%llx has No User Data\n", __FUNCTION__, (unsigned long long)start_lba);


            /* find out the unconsumed block counts that needs to skip with advance verify checkpoint */
            status = fbe_raid_group_monitor_io_get_unconsumed_block_counts(raid_group_p, unconsumed_block_count, 
                                                start_lba, &io_block_count);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO, "_io_get_unconsumed_blk failed, start lba 0x%llx blks 0x%llx\n",
                        (unsigned long long)start_lba, (unsigned long long)block_count);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }
            if (io_block_count == 0) {
                /* The edge may have been attached after the event was returned with no user data.  
                 * The result is an io_block_count of zero.  Let the monitor run again to retry.
                 */
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "Verify no user data io block count: 0x%llx, start lba 0x%llx blks 0x%llx ublks: 0x%llx\n",
                                      (unsigned long long)io_block_count, (unsigned long long)start_lba, 
                                      (unsigned long long)block_count, (unsigned long long)unconsumed_block_count);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
            }
            /* set the updated block count to advance verify checkpoint */
            block_count = io_block_count;

            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "raid_group: Verify,  No user data: lba: 0x%llx  new blocks: 0x%llx unconsumed: 0x%llx \n",
                          (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)unconsumed_block_count);

            status = fbe_raid_group_verify_unconsumed_advance_checkpoint(raid_group_p, packet_p, op_code, start_lba, block_count);
            return status;

        break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s received bad status from client:%d , can't proceed with verify\n",
                                    __FUNCTION__, event_status);

            //! @todo need to decise to skip this trunk or retry this trunk later.
            // Complete the packet
            fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;

    } /* end if switch on event_status*/            

        
    return FBE_STATUS_OK;
}  // end fbe_raid_group_ask_verify_permission_completion()


/*!**************************************************************
 * fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_completion()
 ****************************************************************
 * @brief
 *  This completion function is used to release the block operation we allocated
 *  for clear verify bits in paged metadata.
 * 
 * @param in_packet_p - Packet pointer
 * @param in_context  - Pointer to Raid group.
 *
 * @return fbe_status_t
 *
 * @author
 * 06/12/2012 - Created Prahlad Purohit
 * 
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_completion(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context)
{
    fbe_payload_ex_t  *sep_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_updated_nonpaged()
 ****************************************************************
 * @brief
 *  This completion function is invoked after clearing verify bits in paged 
 *  metadata for unconsumed chunks . If clear paged was success it goes ahead 
 *  and updates the checkpoint in non paged metadta.
 * 
 * @param in_packet_p - Packet pointer
 * @param in_context  - Pointer to Raid group.
 *
 * @return fbe_status_t
 *
 * @author
 * 06/12/2012 - Created Prahlad Purohit
 * 
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_updated_nonpaged(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context)
{
    fbe_payload_ex_t                        *sep_payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_status_t                            packet_status;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    packet_status = fbe_transport_get_status_code(in_packet_p);
    
    fbe_payload_block_get_status(block_operation_p, &block_status);
    
    if((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t *)in_context,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s Paged metadata block operation status: 0x%x status: 0x%x",
                                 __FUNCTION__, block_status, packet_status);

         return FBE_STATUS_GENERIC_FAILURE;
     }

    fbe_raid_group_get_NP_lock((fbe_raid_group_t *)in_context, in_packet_p, fbe_rg_bitmap_update_chkpt_for_unconsumed_chunks);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!****************************************************************************
 * fbe_raid_group_verify_check_permission_based_on_load 
 ******************************************************************************
 * @brief
 *   This function will check if a verify I/O is allowed based on the current
 *   I/O load and scheduler credits available for it.
 * 
 * @param raid_group_p - pointer to the raid group
 * @param ok_to_proceed_b_p - pointer to output that gets set to FBE_TRUE
 *                            if the operation can proceed 
 *
 * @return  fbe_status_t           
 *
 * @author
 *   05/06/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_verify_check_permission_based_on_load(fbe_raid_group_t *raid_group_p,
                                                     fbe_bool_t *ok_to_proceed_b_p)
{
    
    fbe_base_config_operation_permission_t      permission_data;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    *ok_to_proceed_b_p = FBE_FALSE;
    permission_data.operation_priority = fbe_raid_group_get_verify_priority(raid_group_p);
    permission_data.credit_requests.cpu_operations_per_second = FBE_RAID_GROUP_VERIFY_CPUS_PER_SECOND;
    permission_data.credit_requests.mega_bytes_consumption     = FBE_RAID_GROUP_VERIFY_MBYTES_CONSUMPTION;

    /* IO credits is width since we know the rebuild will touch the entire width typically.
     */
    permission_data.io_credits = raid_geometry_p->width;

    fbe_base_config_get_operation_permission((fbe_base_config_t*) raid_group_p, &permission_data, FBE_TRUE, 
                                             ok_to_proceed_b_p);
    if(*ok_to_proceed_b_p == FBE_FALSE){
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 500);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_verify_check_permission_based_on_load()
 **************************************/


/*!****************************************************************************
 * fbe_raid_group_verify_convert_block_opcode_to_verify_flags_without_obj_ptr()
 ******************************************************************************
 * @brief
 *  This function converts the block opcode to verify flags. Onlt difference
 *  between this function and the above function is that this one doesn't take
 *  the raid group as argument. This is needed for fbe_raid_group_get_next_chunk_marked_for_verify
 *  since the function do not have the raid group pointer
 *
 *  It internally converts the LBA to a RAID based value 
 *  in order to properly search the LUN extents on the upper edge.
 *
 * @param in_opcode          - verify opcde to convert
 * @param out_verify_type    - respective verify bitmap flags        
 * 
 * @return fbe_status_t 
 *
 * @author
 *  3/27/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_convert_block_opcode_to_verify_flags_without_obj_ptr(fbe_payload_block_operation_opcode_t opcode,
                                                                          fbe_raid_verify_flags_t*   verify_flags_p)
                                                                                                    
{
    fbe_status_t                 status =  FBE_STATUS_OK;

    *verify_flags_p = FBE_RAID_BITMAP_VERIFY_NONE;

    switch(opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
             *verify_flags_p = FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
             break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
            *verify_flags_p = FBE_RAID_BITMAP_VERIFY_ERROR;
            break;
        
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
             *verify_flags_p = FBE_RAID_BITMAP_VERIFY_SYSTEM;
             break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            *verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
            *verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

    }
        
    return status;
}  // End fbe_raid_group_verify_convert_block_opcode_to_verify_flags_without_obj_ptr()

/*!****************************************************************************
 * fbe_raid_group_verify_convert_block_opcode_to_verify_flags()
 ******************************************************************************
 * @brief
 *
 * @param raid_group_p - Current input raid group.
 * @param opcode          - verify opcde to convert
 * @param verify_type    - respective verify bitmap flags        
 * 
 * @return fbe_status_t 
 *
 * @author
 *  10/26/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_convert_block_opcode_to_verify_flags(fbe_raid_group_t*    raid_group_p,
                                                                  fbe_payload_block_operation_opcode_t opcode,
                                                                  fbe_raid_verify_flags_t*   verify_flags_p)                                                                  
                                                                                                    
{
    FBE_UNREFERENCED_PARAMETER(raid_group_p);

    return fbe_raid_group_verify_convert_block_opcode_to_verify_flags_without_obj_ptr(opcode, verify_flags_p);

}  // End fbe_raid_group_verify_convert_block_opcode_to_verify_flags()



/*!****************************************************************************
 * fbe_raid_group_verify_convert_type_to_verify_flags()
 ******************************************************************************
 * @brief
 *  This function converts Verify type to verify flahs
 *
 *
 * @param verify_type          - verify type to convert
 * @param verify_flags_p    - respective verify bitmap flags        
 * 
 * @return fbe_status_t 
 *
 * @author
 *  01/10/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_convert_type_to_verify_flags(fbe_raid_group_t*    in_raid_group_p,
                                                                fbe_raid_verify_type_t verify_type,
                                                                fbe_raid_verify_flags_t*   verify_flags_p)                                                                  
                                                                                                    
{
    switch(verify_type)
    {
        case FBE_RAID_VERIFY_TYPE_INCOMPLETE_WRITE:
             *verify_flags_p = FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
             break;

       case FBE_RAID_VERIFY_TYPE_ERROR:
            *verify_flags_p = FBE_RAID_BITMAP_VERIFY_ERROR;
            break;
        
        case FBE_RAID_VERIFY_TYPE_SYSTEM: 
             *verify_flags_p = FBE_RAID_BITMAP_VERIFY_SYSTEM;
             break;

       case FBE_RAID_VERIFY_TYPE_USER_RW: 
            *verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
            break;

       case FBE_RAID_VERIFY_TYPE_USER_RO: 
            *verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
            break;
            

        default:
            fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Unrecognized type %d\n", __FUNCTION__, verify_type);
            EmcpalDebugBreakPoint();
            break;

    }
        
    return FBE_STATUS_OK;
}  // End fbe_raid_group_verify_convert_verify_type_to_verify_flags()


/*!****************************************************************************
 * fbe_raid_group_verify_convert_verify_flags_to_opcode()
 ******************************************************************************
 * @brief
 *  This function determines the LUN edge based on the LBA parameter passed in to this function.
 *
 *  It internally converts the LBA to a RAID based value 
 *  in order to properly search the LUN extents on the upper edge.
 *
 * @param raid_group_p - Current input raid group.
 * @param opcode          - verify opcde to convert
 * @param verify_type    - respective verify bitmap flags        
 * 
 * @return fbe_status_t 
 *
 * @author
 *  10/26/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_convert_verify_flags_to_opcode(fbe_raid_group_t*    raid_group_p,
                                                                  fbe_payload_block_operation_opcode_t *opcode,
                                                                  fbe_raid_verify_flags_t verify_type)

{
    *opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;

    // Determine the type of verify operation to perform. Since multiple types may be 
    // concurently marked for the chunk, select the type in a hierarchical fashion.
    if ((verify_type & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE) != 0)
    {
        *opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY;
    }
    else if ((verify_type & FBE_RAID_BITMAP_VERIFY_ERROR) != 0)
    {
        *opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY;
    }
    else if ((verify_type & FBE_RAID_BITMAP_VERIFY_SYSTEM) != 0)
    {
        *opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY;
    }
    else if ((verify_type & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE) != 0)
    {
        *opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY;
    }
    else if ((verify_type & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY) != 0)
    {
        *opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY;
    }
    else
    {
        // No valid flag bits are set.
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:%d invalid verify flags, flags: 0x%x",
                              __FUNCTION__, __LINE__, verify_type);
    }
        
    return FBE_STATUS_OK;
}  // End fbe_raid_group_verify_convert_verify_flags_to_opcode()


/*!****************************************************************************
 * fbe_raid_group_metadata_get_verify_chkpt_offset()
 ******************************************************************************
 * @brief
 *  Fetch the offset of the particular flavor of verify checkpoint.
 * 
 * @param packet_p     - Pointer to originating packet
 * @param raid_group_p - Pointer to raid group
 * @param verify_flags - Verify flags.
 * 
 * @return fbe_status_t 
 *
 *
 * @author
 *  3/26/2012 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t fbe_raid_group_metadata_get_verify_chkpt_offset(fbe_raid_group_t        *raid_group_p,
                                                             fbe_raid_verify_flags_t verify_flags,
                                                             fbe_u64_t *offset_p)
{
    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE) != 0)
    {
        *offset_p = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->incomplete_write_verify_checkpoint);
    }
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_ERROR) != 0)
    {
        *offset_p = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->error_verify_checkpoint);
    }
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_SYSTEM) != 0)
    {
        *offset_p = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->system_verify_checkpoint);
    }     
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE) != 0)
    {
       *offset_p = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->rw_verify_checkpoint);
    }
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY) != 0)
    {
        *offset_p = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->ro_verify_checkpoint); 
    }
    else
    {
        // No valid flag bits are set.
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:%d No valid verify flags are set. flags: 0x%x",
                              __FUNCTION__, __LINE__, verify_flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_raid_group_metadata_get_verify_chkpt_offset()
 *******************************************************/

/*!**************************************************************
 * fbe_raid_group_background_op_is_journal_verify_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks whether journal verify needs to run or not
 *
 * @param in_raid_group_p           - pointer to the raid group
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  03/14/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_background_op_is_journal_verify_need_to_run(fbe_raid_group_t*    raid_group_p)
{

    fbe_raid_group_nonpaged_metadata_t*      non_paged_metadata_p = NULL;
    fbe_lba_t                               journal_verify_checkpoint;
    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&non_paged_metadata_p);
    journal_verify_checkpoint = non_paged_metadata_p->journal_verify_checkpoint;

    if(journal_verify_checkpoint != FBE_LBA_INVALID)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;

}
/******************************************************************************
 * end fbe_raid_group_background_op_is_journal_verify_need_to_run()
 ******************************************************************************/


/*!**************************************************************
 * fbe_raid_group_background_op_is_metadata_verify_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks metadata of metadata and determine 
 *  if metadata verify background operation needs to run or not.
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param verify_flags              - Verify flags to check for.
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  01/06/2012 - Created. Ashok Tamilarasan
 *  04/21/2012 - Modified to use check points - Prahlad Purohit
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_background_op_is_metadata_verify_need_to_run(fbe_raid_group_t* raid_group_p,
                                                            fbe_raid_verify_flags_t verify_flags)
{
    fbe_lba_t                            ro_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                            rw_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                            error_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                            incomplete_write_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                            system_verify_checkpoint = FBE_LBA_INVALID;
    fbe_raid_group_nonpaged_metadata_t*  raid_group_non_paged_metadata_p = NULL;
    fbe_lba_t                            metadata_start_lba = FBE_LBA_INVALID;
    fbe_raid_geometry_t                  *raid_geometry_p = NULL;
    fbe_u16_t                            num_data_disks = 0;
    fbe_u32_t                            chunk_count = 0;
    fbe_u32_t                            cur_chunk_count;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Raw mirrors do not use paged metadata.  They use non-paged mdd for all 
         * their user area. 
         */
        return FBE_FALSE;
    }

    fbe_raid_group_lock(raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
    error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint; 
    rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;   
    ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;   
    incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
    system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;
    fbe_raid_group_unlock(raid_group_p);

    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);

    metadata_start_lba /= num_data_disks;    

    if(((verify_flags & FBE_RAID_BITMAP_VERIFY_ERROR) && (error_verify_checkpoint >= metadata_start_lba) && (error_verify_checkpoint != FBE_LBA_INVALID)) ||
       ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE) && (rw_verify_checkpoint >= metadata_start_lba) && (rw_verify_checkpoint != FBE_LBA_INVALID)) ||
       ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY) && (ro_verify_checkpoint >= metadata_start_lba) && (ro_verify_checkpoint != FBE_LBA_INVALID)) ||
       ((verify_flags & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE) && (incomplete_write_verify_checkpoint >= metadata_start_lba) && (incomplete_write_verify_checkpoint != FBE_LBA_INVALID)) ||
       ((verify_flags & FBE_RAID_BITMAP_VERIFY_SYSTEM) && (system_verify_checkpoint >= metadata_start_lba) && (system_verify_checkpoint != FBE_LBA_INVALID)))
    {
        return FBE_TRUE;
    }

    /*  Loop though the paged metadata metadata to find the first chunk that is marked for verify 
     */
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &chunk_count);
    for (cur_chunk_count = 0; cur_chunk_count < chunk_count; cur_chunk_count++)
    {
        if (raid_group_non_paged_metadata_p->paged_metadata_metadata[cur_chunk_count].verify_bits & verify_flags)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: MD Verify set.Chunk:%d,Vr:0x%08x \n",__FUNCTION__, chunk_count,
                                    raid_group_non_paged_metadata_p->paged_metadata_metadata[cur_chunk_count].verify_bits);
            return FBE_TRUE;
        }
    }
    
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_background_op_is_metadata_verify_need_to_run()
 ******************************************************************************/
/*!**************************************************************
 * fbe_raid_group_background_op_is_verify_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks verify checkpoint and determine if verify 
 *  background operation needs to run or not.
 *
 * @param in_raid_group_p           - pointer to the raid group
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  07/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_background_op_is_verify_need_to_run(fbe_raid_group_t*       raid_group_p)
{
    fbe_lba_t                            ro_verify_checkpoint;
    fbe_lba_t                            rw_verify_checkpoint;
    fbe_lba_t                            error_verify_checkpoint;
    fbe_lba_t                            incomplete_write_verify_checkpoint;
    fbe_lba_t                            system_verify_checkpoint;    
    fbe_raid_group_nonpaged_metadata_t*  raid_group_non_paged_metadata_p = NULL;
    fbe_lba_t                            exported_disk_capacity;

    fbe_raid_group_lock(raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
    error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint; 
    rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;   
    ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;   
    incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
    system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;    
    fbe_raid_group_unlock(raid_group_p);

    //  Get the exported capacity of raid group object
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /* check if verify checkpoint needs to be driven forward
     */
    if(((error_verify_checkpoint < exported_disk_capacity) ||
       (rw_verify_checkpoint < exported_disk_capacity) ||
       (ro_verify_checkpoint < exported_disk_capacity) ||
       (system_verify_checkpoint < exported_disk_capacity) ||       
       (incomplete_write_verify_checkpoint < exported_disk_capacity)) && 
       !fbe_raid_group_is_degraded(raid_group_p) &&
       fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_VERIFY))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_background_op_is_verify_need_to_run()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_raid_group_verify_get_checkpoint
 ******************************************************************************
 * @brief
 *   This function is used to get verify checkpoint for specified verify type.
 *
 * @param   raid_group_p             - pointer to a raid group object
 * @param   verify_flags                - Verify flags.
 * 
 * @return  fbe_lba_t                   - Verify check point.
 *
 * @author
 *   02/29/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_lba_t
fbe_raid_group_verify_get_checkpoint(fbe_raid_group_t*         raid_group_p,
                                     fbe_raid_verify_flags_t   verify_flags)
{
    fbe_raid_group_nonpaged_metadata_t*     nonpaged_metadata_p = NULL;

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);    

    /* We need to check the verify flags in order of highest priority verify.
     */
    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE) != 0)
    {
        return nonpaged_metadata_p->incomplete_write_verify_checkpoint;
    }     
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_ERROR) != 0)
    {
        return nonpaged_metadata_p->error_verify_checkpoint;
    }
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_SYSTEM) != 0)
    {
        return nonpaged_metadata_p->system_verify_checkpoint;
    }
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE) != 0)
    {
        return nonpaged_metadata_p->rw_verify_checkpoint;
    }
    else if ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY) != 0)
    {
        return nonpaged_metadata_p->ro_verify_checkpoint;
    }
    else
    {
        // No valid flag bits are set.
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:%d No valid verify flags are set.",
                              __FUNCTION__, __LINE__);

        return FBE_LBA_INVALID;
    }
}

/*!**************************************************************
 * fbe_raid_group_get_verify_bits_for_md_chkpt()
 ****************************************************************
 * @brief
 *  Return the verify bits for the checkpoint that
 *  seems to be within the paged metadata.
 *
 * @param raid_group_p - pointer to the raid group
 * @param verify_flags_p - verify bits.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_get_verify_bits_for_md_chkpt(fbe_raid_group_t *raid_group_p,
                                            fbe_raid_verify_flags_t *verify_flags_p)
{
    fbe_lba_t                            ro_verify_checkpoint;
    fbe_lba_t                            rw_verify_checkpoint;
    fbe_lba_t                            error_verify_checkpoint;
    fbe_lba_t                            incomplete_write_verify_checkpoint;
    fbe_lba_t                            system_verify_checkpoint;    
    fbe_raid_group_nonpaged_metadata_t*  raid_group_non_paged_metadata_p = NULL;
    fbe_lba_t                            exported_disk_capacity;

    fbe_raid_group_lock(raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
    error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint; 
    rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;   
    ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;    
    incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
    system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;    
    fbe_raid_group_unlock(raid_group_p);

    //  Get the exported capacity of raid group object
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /* check if verify checkpoint needs to be driven forward
     */
    if ((incomplete_write_verify_checkpoint != FBE_LBA_INVALID) &&
             (incomplete_write_verify_checkpoint >= exported_disk_capacity))
    {
        *verify_flags_p = FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
    }     
    else if ((error_verify_checkpoint != FBE_LBA_INVALID) &&
        (error_verify_checkpoint >= exported_disk_capacity))
    {
        *verify_flags_p = FBE_RAID_BITMAP_VERIFY_ERROR;
    }
    else if ((system_verify_checkpoint != FBE_LBA_INVALID) &&
             (system_verify_checkpoint >= exported_disk_capacity))
    {
        *verify_flags_p = FBE_RAID_BITMAP_VERIFY_SYSTEM;
    }
    else if ((rw_verify_checkpoint != FBE_LBA_INVALID) &&
             (rw_verify_checkpoint >= exported_disk_capacity))
    {
        *verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
    }
    else if ((ro_verify_checkpoint != FBE_LBA_INVALID) &&
             (ro_verify_checkpoint >= exported_disk_capacity))
    {
        *verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_get_verify_bits_for_md_chkpt()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_raid_group_verify_get_next_verify_type
 ******************************************************************************
 * @brief
 *   This function is used to get the next verify type to be run.
 *
 * @param   raid_group_p              - pointer to a raid group object.
 * @param   metadata_region           - TRUE indicates metadata region.
 * @param   out_verify_flags          - Pointer to get next exligible verify type.
 * 
 * @return fbe_status_t
 *
 * @note The priority order of verifies must be kept consistent with 
 * fbe_medic_action_t.
 *
 * @author
 *   04/22/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_get_next_verify_type(fbe_raid_group_t*       in_raid_group_p,
                                                        fbe_bool_t              metadata_region,
                                                        fbe_raid_verify_flags_t *out_verify_flags_p)
{
    fbe_raid_group_nonpaged_metadata_t  *raid_group_non_paged_metadata_p = NULL;
    fbe_lba_t                           error_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                           ro_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                           rw_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                           incomplete_write_verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                           system_verify_checkpoint = FBE_LBA_INVALID;    
    fbe_bool_t                          is_rw_verify_enabled = FALSE;
    fbe_bool_t                          is_ro_verify_enabled = FALSE;
    fbe_bool_t                          is_error_verify_enabled = FALSE;
    fbe_bool_t                          is_iw_verify_enabled = FALSE;
    fbe_bool_t                          is_sys_verify_enabled = FALSE;
    fbe_raid_geometry_t                 *raid_geometry_p = NULL;
    fbe_lba_t                           metadata_start_lba = FBE_LBA_INVALID;
    fbe_u16_t                           num_data_disks = 0;

    /*  Initialize the metadata type as invalid */
    *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_NONE;

    /* Get verify checkpoints. */
    fbe_raid_group_lock(in_raid_group_p);    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)in_raid_group_p, (void **)&raid_group_non_paged_metadata_p);    
    error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint; 
    rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;   
    ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;    
    incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
    system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;    
    fbe_raid_group_unlock(in_raid_group_p);    

    /* Get verify enabled states. */
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) in_raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY, &is_error_verify_enabled);
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) in_raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_INCOMPLETE_WRITE_VERIFY, &is_iw_verify_enabled);
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) in_raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY, &is_sys_verify_enabled);    
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) in_raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY, &is_rw_verify_enabled);
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) in_raid_group_p, FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY, &is_ro_verify_enabled);

    fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "rg_monitor_run_verify opt enable(1) ERR:%d,RW:%d,RO:%d,IW:%d,SYS:%d\n", 
                          is_error_verify_enabled, is_rw_verify_enabled, 
                          is_ro_verify_enabled, is_iw_verify_enabled,
                          is_sys_verify_enabled);    

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);

    metadata_start_lba /= num_data_disks;

    if(metadata_region)
    {
        /* Defines the hierarchical order for verifies. Read note above before
         * changing this order.
         */
        if((fbe_raid_group_background_op_is_metadata_verify_need_to_run(in_raid_group_p,FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE))
           && is_iw_verify_enabled)
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
        }
        else if((fbe_raid_group_background_op_is_metadata_verify_need_to_run(in_raid_group_p,FBE_RAID_BITMAP_VERIFY_ERROR))
                && is_error_verify_enabled)
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_ERROR;
        }
        else if((fbe_raid_group_background_op_is_metadata_verify_need_to_run(in_raid_group_p,FBE_RAID_BITMAP_VERIFY_SYSTEM))
                && is_sys_verify_enabled)
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_SYSTEM;
        }
        else if((fbe_raid_group_background_op_is_metadata_verify_need_to_run(in_raid_group_p,FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE))
                && is_rw_verify_enabled)
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
        }
        else if((fbe_raid_group_background_op_is_metadata_verify_need_to_run(in_raid_group_p,FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY))
                && is_ro_verify_enabled)
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
        }
    }
    else
    {
        /* Defines the hierarchical order for verifies. Read note above before
         * changing this order.
         */    
        if((incomplete_write_verify_checkpoint < metadata_start_lba) && is_iw_verify_enabled)
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
        }
        else if((error_verify_checkpoint < metadata_start_lba) && is_error_verify_enabled) 
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_ERROR;
        }
        else if((system_verify_checkpoint < metadata_start_lba) && is_sys_verify_enabled) 
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_SYSTEM;
        }
        else if((rw_verify_checkpoint < metadata_start_lba) && is_rw_verify_enabled) 
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE;
        }
        else if((ro_verify_checkpoint < metadata_start_lba) && is_ro_verify_enabled) 
        {
            *out_verify_flags_p = FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY;
        }
    }

    /*  Return success  */
    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_verify_get_next_metadata_verify_lba
 ******************************************************************************
 * @brief
 *   This function is used to get the next metadata lba which needs to be verified
 *
 * @param   raid_group_p              - pointer to a raid group object
 * @param   in_verify_flags           - Verify flag bitmask caller is interested in.
 * @param   out_metadata_verify_lba_p - Pointer to the next lba which needs to be 
 *                                      verified
 * 
 * @return fbe_status_t  
 *
 * @author
 *   01/10/2012 - Created. Ashok Tamilarasan
 *   03/28/2012 - Modified to specify verify_flags - Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_verify_get_next_metadata_verify_lba(fbe_raid_group_t*       in_raid_group_p,
                                                   fbe_raid_verify_flags_t in_verify_flags,
                                                   fbe_lba_t*              out_metadata_verify_lba_p)
{
    fbe_chunk_index_t                   paged_md_md_index;    // index relative to paged MD MD
    fbe_chunk_index_t                   rg_chunk_index;       // chunk index relative to the RG 
    fbe_lba_t                           verify_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t                           mdd_chunk_lba = FBE_LBA_INVALID;
    //fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_lba_t           configured_capacity;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_INFO;

    /*  Initialize the metadata rebuild LBA as invalid */
    *out_metadata_verify_lba_p = FBE_LBA_INVALID;

    /* we need to figure out which lba to verify and the kind of verify
     */
    fbe_raid_group_verify_get_next_metadata_of_metadata_chunk_index(in_raid_group_p,
                                                               &paged_md_md_index,
                                                               in_verify_flags);

    /*  If no chunks are marked needs verify, then return here with success  */
    if (paged_md_md_index == FBE_CHUNK_INDEX_INVALID)
    {
        return FBE_STATUS_OK;
    }

    // Get verify checkpoint for running verify operation.
    verify_checkpoint = fbe_raid_group_verify_get_checkpoint(in_raid_group_p, in_verify_flags);

    /*  Translate the paged MD chunk to a RG-based chunk */
    fbe_raid_group_bitmap_translate_paged_md_md_index_to_rg_chunk_index(in_raid_group_p, 
                                                                        paged_md_md_index,
                                                                        verify_checkpoint,
                                                                        &rg_chunk_index); 

    /*  Get the LBA corresponding to the given chunk  */
    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p, rg_chunk_index, &mdd_chunk_lba);

    /*! @note Since a raid group can have multiple LUNs and there is no way to
     *        know that we have already marked the non-paged metadata of
     *        metadata, three will be cases where the non-paged index (which is
     *        the first non-paged slot that indicated that a verify was required)
     *        is below the current checkpoint.  If that is the case simply
     *        trace an informational message (since we are `resetting' the
     *        metadata verify checkpoint).
     */
    *out_metadata_verify_lba_p = mdd_chunk_lba;

    // Check for upper limit (configured capacity) on lba range.
    configured_capacity = fbe_raid_group_get_disk_capacity(in_raid_group_p);
    if(*out_metadata_verify_lba_p >= configured_capacity)
    {
        trace_level = FBE_TRACE_LEVEL_ERROR;
    }

    fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                          trace_level, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: verify get next metadata pmdd: 0x%llx index: 0x%llx lba: 0x%llx type: 0x%x\n",
                          (unsigned long long)paged_md_md_index, (unsigned long long)rg_chunk_index,
                          (unsigned long long)*out_metadata_verify_lba_p, in_verify_flags);

    /*  Return success  */
    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_verify_get_next_metadata_of_metadata_chunk_index()
 ******************************************************************************
 * @brief
 *   This function is used to get the next metadata chunk index that is marked 
 *    for verify
 *
 * @param   raid_group_p             - pointer to a raid group object
 * @param   out_chunk_index_p        - next chunk index which needs to be 
 *                                     verified
 * @param   verify_flags             - Verify operation flags.
 * 
 * @return fbe_status_t  
 *
 * @author
 *   01/10/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_verify_get_next_metadata_of_metadata_chunk_index(
                                        fbe_raid_group_t*           raid_group_p,
                                        fbe_chunk_index_t*          out_chunk_index_p,
                                        fbe_raid_verify_flags_t     verify_flags)
{
    fbe_chunk_count_t                   chunk_count;
    fbe_chunk_count_t                   cur_chunk_count;            // chunk number currently processing    
    fbe_raid_group_paged_metadata_t*    paged_md_md_p;              // pointer to paged metadata metadata 
    
    /*  Initialize the metadata rebuild LBA as invalid */
    *out_chunk_index_p = FBE_CHUNK_INDEX_INVALID;

    /* we need to figure out which index is marked for verify 
     */
    /*  Get a pointer to the paged metadata metadata  */
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(raid_group_p); 

    /*  Loop though the paged metadata metadata to find the first chunk that is marked for verify 
     */
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &chunk_count);

    for (cur_chunk_count = 0; cur_chunk_count < chunk_count; cur_chunk_count++)
    {
        if (paged_md_md_p[cur_chunk_count].verify_bits & verify_flags)
        {
            *out_chunk_index_p = cur_chunk_count;
            break;
        }
    }
    /*  Return success  */
    return FBE_STATUS_OK;
}  
/***********************************************************************
 * end fbe_raid_group_verify_get_next_metadata_of_metadata_chunk_index()
 ***********************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_get_opcode_for_verify
 ******************************************************************************
 * @brief
 *   This function is used to get the block operation opcode for the specified
 *   verify operation.
 *
 * @param   raid_group_p             - pointer to a raid group object
 * @param   verify_flags             - verify operation
 * 
 * @return  fbe_raid_group_background_op_t  
 *
 * @author
 *   01/2012 - Created. Susan Rundbaken
 *
 ******************************************************************************/
fbe_raid_group_background_op_t
fbe_raid_group_verify_get_opcode_for_verify(fbe_raid_group_t*           raid_group_p,
                                            fbe_raid_verify_flags_t     verify_flags)
{
    /* Determine the type of verify operation to perform. 
     */
    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)!= 0) 
    {
        return FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY;            
    }

    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_ERROR) != 0) 
    {
        return FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY;            
    }    

    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_SYSTEM)!= 0) 
    {
        return FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY;            
    }    

    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE) != 0) 
    {
        return FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY;            
    } 

    if ((verify_flags & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY) != 0) 
    {
        return FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY;            
    } 

    /* No valid flag bits are set.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: invalid verify flags, flags: 0x%x\n",
                          __FUNCTION__,verify_flags);

    return FBE_RAID_GROUP_BACKGROUND_OP_NONE;
}  

/*!****************************************************************************
 * fbe_raid_group_verify_process_mark_verify_event()
 ******************************************************************************
 * @brief
 *  This function processes mark verify event from virtual drive.
 * 
 * @param raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_data_event_p       - data event to mark needs rebuild
 * 
 * @return fbe_lifecycle_status_t        
 *
 * @author
 *   03/09/2012 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_verify_process_mark_verify_event(
                                    fbe_raid_group_t*   raid_group_p,
                                    fbe_event_t*        in_data_event_p,
                                    fbe_packet_t*       in_packet_p)
{
    fbe_lba_t                           start_lba;                          // start lba of the event extent
    fbe_block_count_t                   block_count;                        // block count of the event extent
    fbe_event_stack_t                  *event_stack_p;                      // event stack
    //fbe_edge_index_t                            edge_index;                         // edge index
    //fbe_base_edge_t*                            base_edge_p;                        // pointer to the base edge
    fbe_status_t                        status;                             // status of the operation
    fbe_chunk_index_t                   chunk_index;        
    fbe_chunk_count_t                   chunk_count;        
    fbe_payload_ex_t                   *sep_payload_p;
    fbe_payload_block_operation_t      *block_operation_p = NULL;
    fbe_optimum_block_size_t            optimum_block_size;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_raid_verify_type_t      verify_type;
    fbe_event_data_request_t    data_request;

    fbe_event_get_data_request_data(in_data_event_p, &data_request);
        
    // Get information from the event 
    event_stack_p = fbe_event_get_current_stack(in_data_event_p);
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &block_count);
    //fbe_event_get_edge(in_data_event_p, &base_edge_p);
    //fbe_base_transport_get_client_index(base_edge_p, (fbe_edge_index_t *) &edge_index);

    /* Validate that the request doesn't exceed the consumed capacity.
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);
    if ((start_lba + block_count) > raid_group_metadata_positions.raid_capacity)
    {
        /* Request extents beyond protected capacity.  If it is within the journal
         * area just write 0s (0s is a special pattern that the journaling uses).
         * Journal geometry is a physical address, need the width to convert to logical.
         */
        fbe_u16_t            data_disks;
        fbe_raid_geometry_t *raid_geometry_p;        

        raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

        if ((start_lba >= (raid_group_metadata_positions.journal_write_log_pba * data_disks)) &&
            (block_count <= (raid_group_metadata_positions.journal_write_log_physical_capacity * data_disks)))
        {
            /* Trace an informational message and mark journal for verify.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s journal verify for lba: 0x%llx blks: 0x%llx required.\n", 
                                  __FUNCTION__, (unsigned long long)start_lba, (unsigned long long)block_count);

            fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_mark_journal_for_verify_completion, raid_group_p);
            fbe_raid_group_mark_journal_for_verify(in_packet_p, raid_group_p, start_lba);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

        /* Otherwise validate that it doesn't exceed the imported capacity.
         */
        else if ((start_lba + block_count) > raid_group_metadata_positions.imported_capacity)
        {
            /* Trace an error and return.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s start: 0x%llx blks: 0x%llx beyond imported: 0x%llx \n", 
                                  __FUNCTION__, (unsigned long long)start_lba, (unsigned long long)block_count,
                                  (unsigned long long)raid_group_metadata_positions.imported_capacity);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* Else the request is spanning between protected and non-protected areas, which is unexpected.
         */
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s start: 0x%llx blks: 0x%llx spanning rc: 0x%llx ic: 0x%llx w%d wlp0x%llx \n",
                                  __FUNCTION__, (unsigned long long)start_lba, (unsigned long long)block_count, 
                                  (unsigned long long)raid_group_metadata_positions.raid_capacity,
                                  (unsigned long long)raid_group_metadata_positions.imported_capacity,
                                  data_disks, (unsigned long long)raid_group_metadata_positions.journal_write_log_physical_capacity);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* allocate a block operation */
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p); 
    block_operation_p   = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    
    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(
        ((fbe_base_config_t *)raid_group_p)->block_edge_p, 
        &optimum_block_size);
    /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY,
                                      start_lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);

    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(in_packet_p,
                                          fbe_raid_group_verify_process_mark_verify_event_completion,
                                          raid_group_p);

    // Convert the lun start lba and block count to per disk chunk index and chunk count
    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          start_lba,
                                                          block_count,
                                                          &chunk_index, 
                                                          &chunk_count);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: mark verify slba 0x%llx bl 0x%llx ch_index: 0x%llx ch_count: 0x%x\n", 
                          __FUNCTION__, (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)chunk_index, chunk_count);

    if (chunk_count == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO, "%s: chunk count is 0\n", __FUNCTION__);
    }
    if (data_request.data_event_type == FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY)
    {
        verify_type = FBE_RAID_VERIFY_TYPE_INCOMPLETE_WRITE;
    }
    else
    {
        verify_type = FBE_RAID_VERIFY_TYPE_ERROR;
    }
    // Mark the calculated range of chunks as needing to be verified
    status = fbe_raid_group_verify_process_verify_request(in_packet_p,
                                                         raid_group_p, 
                                                         chunk_index, 
                                                         verify_type, 
                                                         chunk_count);

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_verify_process_mark_verify_event()

/*!****************************************************************************
 * fbe_raid_group_verify_process_mark_verify_event_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for mark verify event from virtual drive.
 * 
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_context            - pointer to a raid group object
 * 
 * @return fbe_status_t        
 *
 * @author
 *   03/09/2012 - Created. chenl6
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_verify_process_mark_verify_event_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*                       raid_group_p;          
    fbe_status_t                            status;
    fbe_event_t*                            event_p;
    fbe_payload_ex_t *                      payload_p;
    fbe_payload_block_operation_t *         block_operation_p;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    // Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    // Free the resources we allocated previously
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
        
    status = fbe_transport_get_status_code(in_packet_p);
    if ((status == FBE_STATUS_OK) && (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {

        // Pop the the data event and complete the event
        fbe_base_config_event_queue_pop_event((fbe_base_config_t *)raid_group_p, &event_p);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: mark verify failed packet_status: 0x0%x block_status: 0x%x block_qual: 0x%x\n", 
                              __FUNCTION__, status, block_status, block_qualifier);
    }

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);    

    /* We have some work to do and so reschedule the monitor immediately */

        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0);

    return FBE_STATUS_OK;
} // End fbe_raid_group_verify_process_mark_verify_event_completion()


/*!****************************************************************************
 *  fbe_raid_verify_initiate_error_verify_event()
 ******************************************************************************
 * @brief
 *  This function will create an event and put it in the base config event queue.
 *  The background monitor condition will dequeue the event and mark the chunk
 *  for error verify
 * 
 * @param raid_group_p
 * @param packet_p     
 * @param chunk_index 
 * @param chunk_count
 * @param event_type
 *
 * @return fbe_status_t.
 *
 * @author
 *  03/27/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_verify_initiate_error_verify_event(fbe_raid_group_t*    raid_group_p,
                                                         fbe_packet_t*          packet_p,
                                                         fbe_chunk_index_t      chunk_index,
                                                         fbe_chunk_count_t      chunk_count,
                                                         fbe_data_event_type_t  event_type)
                                           
{
    fbe_event_t *                       event_p = NULL;
    fbe_event_stack_t *                 event_stack = NULL;
    fbe_lba_t                           raid_lba; 
    fbe_block_count_t                   raid_block_count;  
    fbe_block_count_t                   disk_block_count;
    fbe_chunk_index_t                   end_chunk_index;
    fbe_lba_t                           start_lba;
    fbe_lba_t                           end_lba;  
    fbe_raid_geometry_t*                raid_geometry_p;        
    fbe_u16_t                           data_disks;
    fbe_u64_t                           chunk_range;
    fbe_bool_t                          queued = FBE_FALSE;             
    fbe_event_data_request_t            error_verify_data_request;
    fbe_chunk_size_t                    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
            FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s unable to allocate event",__FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_event_init(event_p);
    event_stack = fbe_event_allocate_stack(event_p);

    error_verify_data_request.data_event_type = event_type;
    fbe_event_set_data_request_data( event_p, &error_verify_data_request );
 
    /* Calculate the start and end lba of this error verify request */
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, chunk_index, &start_lba);

    end_chunk_index = chunk_index + chunk_count - 1;
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, end_chunk_index, &end_lba);

    /*calculat the disk based block count from the start and end lba */
    disk_block_count = end_lba - start_lba + chunk_size;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    raid_lba   = start_lba * data_disks; 
    raid_block_count = disk_block_count * data_disks;

    /* Fill LBA range */
    fbe_event_stack_set_extent(event_stack, raid_lba, raid_block_count);

    if (raid_block_count == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: block count is zero\n", __FUNCTION__);
    }
    fbe_event_stack_set_completion_function(event_stack, fbe_raid_verify_initiate_error_verify_event_completion,
                                            raid_group_p);

    chunk_range = raid_block_count * FBE_RAID_GROUP_COALESCE_CHUNK_RANGE;

    /* Combine the events and then push it in. Don't allow coalescing past the raid capacity (into the journal).
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);
    fbe_base_config_event_queue_coalesce_and_push_event((fbe_base_config_t*)raid_group_p, event_p, 
                                                        chunk_range, &queued, raid_group_metadata_positions.exported_capacity);

    /* If the event was not queued because we coalesced it, then free up the memory*/
    if(!queued)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: event is not queued\n", __FUNCTION__);
        fbe_event_complete(event_p);
    }

    /* We have some work to do and so reschedule the monitor immediately */

        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0);

    return FBE_STATUS_OK;
}

/*******************************************************
 * end fbe_raid_verify_initiate_error_verify_event()
 ******************************************************/

/*!****************************************************************************
 *  fbe_raid_verify_initiate_error_verify_event_completion()
 ******************************************************************************
 * @brief
 *  This function will just free the memory allocated for the event
 * 
 * @param event_p
 * @param context
 *
 * @return fbe_status_t.
 *
 * @author
 *  03/27/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_verify_initiate_error_verify_event_completion(fbe_event_t*   event_p,
                                                               fbe_packet_completion_context_t context)
                                           
{
    fbe_event_stack_t*             event_stack_p = NULL;
    fbe_raid_group_t*              raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t*)context;

    event_stack_p = fbe_event_get_current_stack( event_p );

    // free all associated resources before releasing event
    fbe_event_release_stack( event_p, event_stack_p );
    fbe_event_destroy( event_p );
    fbe_memory_ex_release(event_p); 


    return FBE_STATUS_OK;
}

/*******************************************************
 * end fbe_raid_verify_initiate_error_verify_event_completion()
 ******************************************************/

/*!****************************************************************************
 *  fbe_raid_group_get_next_chunk_marked_for_verify()
 ******************************************************************************
 * @brief
 *  This function will go over the search data and find out if the chunk
 *  is marked for verify or not
 * 
 * @param search_data -       pointer to the search data
 * @param search_size  -      search data size
 * @param raid_group_p  -     pointer to the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  03/27/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_get_next_chunk_marked_for_verify(fbe_u8_t*     search_data,
                                                            fbe_u32_t    search_size,
                                                            context_t    context)
                                           
{
    fbe_payload_block_operation_opcode_t*    block_opcode_p = NULL;
    fbe_raid_group_paged_metadata_t          paged_metadata;
    fbe_raid_verify_flags_t                  verify_flag;
    fbe_u8_t*                                source_ptr = NULL;
    fbe_bool_t                               is_chunk_marked_for_processing = FBE_FALSE;
    fbe_u32_t                                index;

    block_opcode_p = (fbe_payload_block_operation_opcode_t*)context;
    //Convert the opcode into verify flags
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags_without_obj_ptr(*block_opcode_p, &verify_flag);

    source_ptr = search_data;

    for(index = 0; index < search_size; index += sizeof(fbe_raid_group_paged_metadata_t))
    {
        fbe_copy_memory(&paged_metadata, source_ptr, sizeof(fbe_raid_group_paged_metadata_t));
        source_ptr += sizeof(fbe_raid_group_paged_metadata_t);

        if(paged_metadata.verify_bits & verify_flag)
        {
            is_chunk_marked_for_processing = FBE_TRUE;
            break;
        }
    }

    return is_chunk_marked_for_processing;
}

/*******************************************************
 * end fbe_raid_group_get_next_chunk_marked_for_verify()
 ******************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_type_is_user_initiated()
 ******************************************************************************
 * @brief
 *  This function checks if the verify type belongs to class that is initiated by 
 *  the user or not
 *
 *
 * @param in_verify_type          - verify type to check
 * 
 * @return fbe_bool_t  - TRUE if that is the case, FALSE otherwise
 *
 * @author
 *  01/10/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_block_opcode_is_user_initiated_verify(fbe_payload_block_operation_opcode_t block_opcode)
                                                                                                    
{
    fbe_bool_t user_initiated = FBE_FALSE;

    switch(block_opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
            user_initiated = FBE_TRUE;
            break;
    }
        
    return user_initiated;
}  // End fbe_raid_group_verify_type_is_user_initiated()

/*!**************************************************************
 * fbe_raid_group_verify_get_marked_blocks()
 ****************************************************************
 * @brief
 *  Get the number of contiguous blocks that are marked
 *  for a particular verify.
 *
 * @param raid_group_p
 * @param iots_p - The chunk info of the iots is what we check for
 *                 the verify flags.
 * @param verify_flags - The type of verify to look for.
 * @param block_p - Block count of contiguous blocks not marked for verify.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/27/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_verify_get_marked_blocks(fbe_raid_group_t *raid_group_p,
                                                     fbe_raid_iots_t *iots_p,
                                                     fbe_raid_verify_flags_t verify_flags,
                                                     fbe_block_count_t *block_p)
{
    fbe_u32_t chunk_index;
    fbe_block_count_t blocks = 0;
    fbe_block_count_t iots_blocks;
    fbe_chunk_size_t chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_chunk_count_t chunk_count;

    fbe_raid_iots_get_current_op_blocks(iots_p, &iots_blocks);
    chunk_count = (fbe_chunk_count_t) (iots_blocks / chunk_size);
    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        /* Count the blocks that have verify marked.
         * Break out when we find an unmarked chunk. 
         */
        if ((iots_p->chunk_info[chunk_index].verify_bits & verify_flags) == 0)
        {
            break;
        }
        else
        {
            blocks += chunk_size;
        }
    }
    *block_p = blocks;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_verify_get_marked_blocks()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_verify_get_unmarked_blocks()
 ****************************************************************
 * @brief
 *  Get the number of contiguous blocks that are not marked
 *  for a particular verify.
 *
 * @param raid_group_p
 * @param iots_p - The chunk info of the iots is what we check for
 *                 the verify flags.
 * @param verify_flags - The type of verify to look for the absence of.
 * @param block_p - Block count of contiguous blocks not marked for verify.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/27/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_verify_get_unmarked_blocks(fbe_raid_group_t *raid_group_p,
                                                       fbe_raid_iots_t *iots_p,
                                                       fbe_raid_verify_flags_t verify_flags,
                                                       fbe_block_count_t *block_p)
{
    fbe_u32_t chunk_index;
    fbe_block_count_t iots_blocks;
    fbe_block_count_t blocks = 0;
    fbe_chunk_size_t chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_chunk_count_t chunk_count;

    fbe_raid_iots_get_current_op_blocks(iots_p, &iots_blocks);
    chunk_count = (fbe_chunk_count_t) (iots_blocks / chunk_size);

    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        /* Count the blocks that do not have verify marked.
         * Break out when we find a marked chunk. 
         */
        if (iots_p->chunk_info[chunk_index].verify_bits & verify_flags)
        {
            break;
        }
        else
        {
            blocks += chunk_size;
        }
    }
    *block_p = blocks;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_verify_get_unmarked_blocks()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_verify_iots_completion()
 ****************************************************************
 * @brief
 *   This is the completion function that gets called 
 *   when we find that the chunk need not be verified.
 *   This will clean up and complete the iots.
 * 
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/25/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_verify_iots_completion(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL; 
    fbe_raid_iots_t *iots_p = NULL;
    raid_group_p = (fbe_raid_group_t*)context;

    /* Put the packet back on the termination queue since we had to take it off before reading the chunk info
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* We can return success since:
     *  o Either not I/O was performed
     *      OR
     *  o The packet status will be processed by another method
     */
    /* AR598712: we could not clear the error FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS
       since fbe_raid_group_iots_finished would check that to set proper conditions so as to recover the full paged metadtata
     */
    if (!(iots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST && iots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS))
    {
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_COMPLETE);
    }

    fbe_raid_group_iots_cleanup(packet_p, raid_group_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**********************************************************
 * end fbe_raid_group_verify_iots_completion()
 **********************************************************/

/*!***************************************************************************
 * fbe_raid_group_verify_handle_chunks()
 *****************************************************************************
 * @brief
 *  This function simply moves the checkpoint if there are
 *  any leading chunks that are not marked.
 *  This function also limits the verify blocks if it find any trailing chunks 
 *  that are not marked for verify.
 *
 * @param   raid_group_p - Pointer for raid group for this rebuild request.
 * @param   packet_p   - Pointer to a packet for rebuld request.
 *
 * @return  fbe_status_t            
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_verify_handle_chunks(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p) 
{
    fbe_status_t                        status = FBE_STATUS_OK;   
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_raid_iots_t*                    iots_p = NULL; 
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_raid_verify_flags_t verify_flags;
    fbe_block_count_t blocks;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p); 
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_raid_iots_get_opcode(iots_p, &block_opcode);
    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_flags);

    if ((iots_p->chunk_info[0].verify_bits & verify_flags) == 0)
    {
        fbe_lba_t start_lba;
        fbe_raid_iots_get_lba(iots_p, &start_lba);
        /* Skip over the unmarked chunks by moving the checkpoint. 
         * We will perform the actual verify on this next monitor cycle. 
         */
        fbe_raid_group_verify_get_unmarked_blocks(raid_group_p, iots_p, verify_flags, &blocks);

        /* We will move the checkpoint for this number of blocks and complete the iots.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_iots_completion, raid_group_p); 

        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                             "raid_group vr type %d is not set. current_chkpt:0x%llx, bl: 0x%llx\n",
                               verify_flags, (unsigned long long)start_lba, (unsigned long long)blocks);
        status = fbe_raid_group_update_verify_checkpoint(packet_p,
                                                         raid_group_p,
                                                         start_lba,
                                                         blocks,
                                                         verify_flags,
                                                         __FUNCTION__);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_block_count_t iots_blocks;
        fbe_raid_iots_get_current_op_blocks(iots_p, &iots_blocks);

        /* We definately have something marked.  See how far it goes.
         */
        fbe_raid_group_verify_get_marked_blocks(raid_group_p, iots_p, verify_flags, &blocks);

        if (blocks < iots_blocks)
        {
            /* We have something unmarked.  Let's do the verify for the area that is marked only.
             * Another monitor cycle will handle the unmarked area.
             */
            fbe_raid_iots_set_blocks(iots_p, blocks);
            fbe_raid_iots_set_blocks_remaining(iots_p, blocks);
            fbe_raid_iots_set_current_op_blocks(iots_p, blocks);
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);

            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                 "raid_group: vr limit unmarked orig_blks: 0x%llx new_blks: 0x%llx vr_fl: 0x%x\n",
                                 (unsigned long long)iots_blocks, (unsigned long long)blocks, verify_flags);
        }
    }


    fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* end of fbe_raid_group_verify_handle_chunks() */



/*!***************************************************************************
 * fbe_raid_group_verify_unconsumed_advance_checkpoint()
 *****************************************************************************
 * @brief
 *   This is a helper function which sends request to advance the verify checkpoint. 
 *
 * @param   raid_group_p - Pointer for raid group for this verify request.
 * @param   packet_p    - Pointer to a packet for rebuld request.
 * @param   op_code     - verify type.
 * @param   start_lba   - verify IO start LBA.
 * @param   block_count - block counts for unconsumed range.
 *
 * @return  fbe_status_t            
 *
 * @author
 *  7/16/2012 - Created. Amit Dhaduk
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_verify_unconsumed_advance_checkpoint(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p, 
                                                 fbe_payload_block_operation_opcode_t    op_code,
                                                 fbe_lba_t start_lba, 
                                                 fbe_block_count_t block_count)
{

    fbe_raid_geometry_t*                    raid_geometry_p = NULL; 
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_raid_iots_t*                        iots_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_block_size_t                        optimal_block_size;

    /* use the iots to store the lba, block count and opcode
     * We need these when we update the checkpoint
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);        
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);


    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);        

    fbe_payload_block_build_operation(block_operation_p,
                                     op_code,
                                     start_lba,
                                     block_count,
                                     FBE_BE_BYTES_PER_BLOCK,
                                     optimal_block_size,
                                     NULL);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_set_current_opcode(iots_p, op_code);
    fbe_raid_iots_set_block_operation(iots_p, block_operation_p);

    /* Checkpoint update used to use block_operation block count
     * but now correctly uses iots block count (which might be different
     * in other paths), so set iots block count per above comment. AR 536292
     */
    fbe_raid_iots_set_blocks(iots_p, block_count);

    fbe_payload_block_set_status(block_operation_p, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_payload_ex_increment_block_operation_level(sep_payload_p); 


    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_completion, raid_group_p);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_raid_group_verify_initiate_non_paged_metadata_clear_bits(packet_p, raid_group_p, start_lba, block_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_clear_verify_for_unconsumed_chunk_updated_nonpaged, raid_group_p);

    fbe_raid_group_bitmap_clear_verify_for_chunk(packet_p, raid_group_p);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_raid_group_metadata_block_operation_completion()
 ****************************************************************
 * @brief
 *  The purpose of this function is to handle failures of the metadata
 *  operation due to a normal quiesce event.  This causes us to queue the
 *  request and mark it quiesced so that it can get restarted.
 *  The effect from the outside is that the request will not 
 *  fail back unless the raid group goes out of READY.
 *
 * @param packet_p
 * @param context
 *
 * @return fbe_status_t
 *
 * @author
 *  8/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_metadata_block_operation_completion(fbe_packet_t *packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = NULL;
    fbe_status_t status;
    fbe_status_t packet_status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    raid_group_p = (fbe_raid_group_t*)context;

    packet_status = fbe_transport_get_status_code(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    if ((packet_status == FBE_STATUS_IO_FAILED_RETRYABLE) ||
        ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
         (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE)))
    {
        fbe_raid_iots_t *iots_p = NULL;
        fbe_raid_geometry_t *raid_geometry_p = NULL;
        fbe_lba_t lba;
        fbe_block_count_t blocks;

        fbe_payload_ex_get_iots(payload_p, &iots_p);
        raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
        fbe_payload_block_get_lba(block_operation_p, &lba);
        fbe_payload_block_get_block_count(block_operation_p, &blocks);
        fbe_raid_iots_init(iots_p, packet_p, block_operation_p, raid_geometry_p, lba, blocks);
        fbe_raid_iots_transition_quiesced(iots_p, fbe_raid_group_restart_block_operation_packet, raid_group_p);

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_mrk_vr_cmp: pk_st: 0x%x queuing. bl_st: %d bl_q: %d pkt: %p\n", 
                              packet_status, block_status, block_qualifier, packet_p);
        status = fbe_base_object_add_to_terminator_queue((fbe_base_object_t *) raid_group_p, packet_p);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_mrk_vr_cmp: failed add termq status: 0x%x pk_st: 0x%x bl_st: 0x%x bl_q: 0x%x\n", 
                                  status, packet_status, block_status, block_qualifier);
            return status; 
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }
    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_mrk_vr_cmp: failed with pk_st: 0x%x bl_status: 0x%x\n", packet_status, block_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_mark_verify_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_restart_block_operation_packet()
 ******************************************************************************
 * @brief
 *   This function restarts the block operation that was previously queued
 * 
 * @param iots_p - pointer to iots
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   08/17/2012 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_raid_state_status_t fbe_raid_group_restart_block_operation_packet(fbe_raid_iots_t * iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)iots_p->callback_context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    /* If the previous I/O was quiesced we need to decrement the `quiesced' 
     * count.
     */
    fbe_raid_group_handle_was_quiesced_for_next(raid_group_p, iots_p);

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid group: iots %p restart packet %p op: %d\n",  
                          iots_p, packet_p, opcode);

    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) raid_group_p, packet_p);

    /* Mark complete since it was marked this way before we were queued.
     */
    fbe_raid_iots_destroy(iots_p);

    /* Set a completion function so that if the mark verify fails with an error due to us 
     * starting to quiesce, then we will retry the mark. 
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_block_operation_completion, raid_group_p);

    if(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA)
    {
        fbe_raid_group_initiate_lun_extent_init_metadata(raid_group_p, packet_p);
    }
    else
    {
        /* Just complete the iots now.
         */
        fbe_raid_group_initiate_verify(raid_group_p, packet_p);
    }
    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_raid_group_restart_block_operation_packet()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_process_lun_extent_init_metadata()
 ******************************************************************************
 * @brief
 *   This function process init metadata block operation from the LUN
 * 
 * @param raid_group_p - pointer to raid group
 * @param packet_p - pointer of verify packet
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   09/13/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_process_lun_extent_init_metadata(fbe_raid_group_t * raid_group_p, 
                                                             fbe_packet_t * packet_p)
{
    fbe_status_t                                status;
    fbe_raid_geometry_t *                       raid_geometry_p = NULL;
    fbe_raid_group_type_t                       raid_type;
    fbe_payload_ex_t *                          payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *             block_operation_p = NULL;
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Since this RG does not have paged MD, there is not much to do and so 
         * return success
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s No MD\n", __FUNCTION__);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

    /* Process R10 striper requests */
    if((((fbe_base_object_t *)raid_group_p)->class_id == FBE_CLASS_ID_STRIPER) 
       && (raid_type == FBE_RAID_GROUP_TYPE_RAID10))
    {
        status = fbe_striper_send_command_to_mirror_objects(
            packet_p, (fbe_base_object_t *)raid_group_p, raid_geometry_p);

        return status;
    }

   /* Set a completion function so that if the mark verify fails with an error due to us 
    * starting to quiesce, then we will retry the mark. 
    */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_metadata_block_operation_completion, raid_group_p);
    fbe_raid_group_initiate_lun_extent_init_metadata(raid_group_p, packet_p);
    return FBE_STATUS_PENDING;
    
}
/******************************************************************************
 * end fbe_raid_group_process_lun_extent_init_metadata()
 ******************************************************************************/

/*!***************************************************************
 * fbe_raid_group_initiate_lun_extent_init_metadata()
 ****************************************************************
 * @brief Starts the process of initing the metadata region for the
 * LUN extent
 *
 * @param raid_group_p - The raid group handle.
 * @param packet_p - The mgmt packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  09/14/12 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_initiate_lun_extent_init_metadata(fbe_raid_group_t * const raid_group_p, 
                                                 fbe_packet_t * const packet_p)
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
                                   (fbe_memory_completion_function_t)fbe_raid_group_initiate_lun_extent_init_metadata_allocate_completion,
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
/* end fbe_raid_group_initiate_lun_extent_init_metadata */

/*!***************************************************************
 * fbe_raid_group_initiate_lun_extent_init_metadata_allocate_completion()
 ****************************************************************
 * @brief This is the completion function for memory allocation.
 *
 * @param memory_request_p - Pointer to memory request.
 * @param context  - Completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/15/14 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_initiate_lun_extent_init_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                     fbe_memory_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_packet_t *                                  packet_p;
    fbe_chunk_index_t                               chunk_index;        
    fbe_chunk_count_t                               chunk_count;        
    fbe_payload_ex_t *                              payload_p;
    fbe_payload_block_operation_t *                 block_operation_p = NULL;
    fbe_object_id_t                                 object_id;
    fbe_raid_group_t *                              raid_group_p = (fbe_raid_group_t *)context;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s memory allocation failed, state:%d\n",
                                        __FUNCTION__, memory_request_p->request_state);

        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_completion_function(packet_p, 
                                         fbe_raid_group_initiate_lun_extent_init_metadata_completion,
                                         raid_group_p); 

    // Convert the start lba and block count to per disk chunk index and chunk count
    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          block_operation_p->lba,
                                                          block_operation_p->block_count,
                                                          &chunk_index, 
                                                          &chunk_count);

    /* Treat this like a monitor op since we are getting the np lock and cannot wait in the raid library.
     */
    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_MONITOR_OP);        
    fbe_transport_set_monitor_object_id(packet_p, object_id);

    /* This request only initialize paged metadata so no need to grab the NP lock
     * AR 526409
     */

    /* start reinit metadata */
    status = fbe_raid_group_bitmap_initiate_reinit_metadata(packet_p, raid_group_p);

    return status;
}
/******************************************************************************
 * end fbe_raid_group_initiate_lun_extent_init_metadata_allocate_completion()
 ******************************************************************************/

/*!***************************************************************
 * fbe_raid_group_initiate_lun_extent_init_metadata_completion()
 ****************************************************************
 * @brief This is the completion function to release memory request.
 *
 * @param packet_p - Pointer to the packet.
 * @param context  - Completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/15/14 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_initiate_lun_extent_init_metadata_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t                        *raid_group_p;
    fbe_memory_request_t                    *memory_request_p = NULL;

    /* Get the raid group pointer from the input context */
    raid_group_p = (fbe_raid_group_t *)context;   

    /* release client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_initiate_lun_extent_init_metadata_completion()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_raid_group_verify_process_verify_request()
 ******************************************************************************
 * @brief
 *  This function processes verify request. It starts the following process:
 *   1) Lock the region that needs to be verified(Incomplete Write Verify only);
 *   2) Get a NP lock (If paged verify is needed);
 *   3) Mark verify for the paged region in NP;
 *   4) Release the NP lock;
 *   5) Mark the user region that needs to be verified in the paged;
 *   6) Get a NP lock;
 *   7) Move the checkpoint;
 *   8) Release the NP Lock;
 *   9) Unlock the user region(If locked before).
 *
 * @param packet_p - The pointer to the packet.
 * @param raid_group_p  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
fbe_status_t 
fbe_raid_group_verify_process_verify_request(fbe_packet_t*               packet_p,
                                             fbe_raid_group_t*           raid_group_p,
                                             fbe_chunk_index_t           chunk_index,
                                             fbe_raid_verify_type_t      verify_type,
                                             fbe_chunk_count_t           chunk_count)          
{
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_lba_t                               paged_md_start_lba = FBE_LBA_INVALID;
    fbe_lba_t                               paged_md_capacity  = FBE_LBA_INVALID;    
    fbe_lba_t                               verify_lba;
    fbe_block_count_t                       block_count;
    fbe_status_t                            status;
    fbe_raid_geometry_t *                   raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, chunk_index, &verify_lba);
    block_count = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: verify request type: %d for: %d chunks from lba: 0x%llx\n",
                          verify_type, chunk_count, (unsigned long long)verify_lba);   

    /* For all other verify types */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config),
                                                         &paged_md_capacity);

    if (verify_type == FBE_RAID_VERIFY_TYPE_INCOMPLETE_WRITE)
    {
        /**
         * The logging severity of I/O depends on IW checkpoint.
         * We need to hold user I/O here to reduce logging severity.
         *
         * For paged metadata I/O, we always report 'Expected' logs.
         */
        if (block_operation_p->lba < paged_md_start_lba)
        {
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_get_stripe_lock_completion, raid_group_p);
            fbe_raid_group_get_stripe_lock(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }


    /* If lba is not in paged region, no need to mark mdd. 
     */
    if (fbe_raid_geometry_has_paged_metadata(raid_geometry_p) &&
        !fbe_raid_group_block_opcode_is_user_initiated_verify(block_opcode) &&
        (block_operation_p->lba < paged_md_start_lba))
    {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_paged_metadata_completion, raid_group_p);        
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_mark_paged_metadata_verify, raid_group_p);        
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the NP lock to update metadata of metadata. 
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_metadata_of_metadata_completion, raid_group_p);
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_verify_mark_metadata_of_metadata);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_raid_group_verify_process_verify_request()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_get_stripe_lock_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for getting stripe lock. It starts to get 
 *  NP lock for MDD changes.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_get_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_status_t packet_status;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    packet_status = fbe_transport_get_status_code(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);

    /* Validate that a stripe lock operaiton was requested
     */
    if (stripe_lock_operation_p == NULL)
    {
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        /* Something went wrong
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s stripe lock opt: packet_status: 0x%x\n", 
                              __FUNCTION__, packet_status);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }

    /* Get the stripe lock operation status.
     */
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status == FBE_STATUS_OK) )
    {
        /* Push a completion on stack to release stripe lock. 
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_release_stripe_lock, raid_group_p);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_metadata_of_metadata_completion, raid_group_p);
        /* Get the NP lock to update metadata of metadata. 
         */
        fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_verify_mark_metadata_of_metadata);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
        /* Something went wrong
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s stripe lock opt: packet_status: 0x%x lock_status: 0x%x\n", 
                              __FUNCTION__, packet_status, stripe_lock_status);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        if (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)
        {
            /* Push a completion on stack to release NP lock. 
             */
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_release_stripe_lock, raid_group_p);
            return FBE_STATUS_CONTINUE;
        }

        fbe_payload_ex_release_stripe_lock_operation(payload_p, stripe_lock_operation_p);
        return FBE_STATUS_OK;
    }
}
/******************************************************************************
 * end fbe_raid_group_verify_get_stripe_lock_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_release_stripe_lock()
 ******************************************************************************
 * @brief
 *  This function release strip lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_release_stripe_lock(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    //fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);

    /* Get the stripe lock operation so that we can release the stripe lock
    */
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);
    if (stripe_lock_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: no stripe lock operation\n", 
                              __FUNCTION__);

        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        return FBE_STATUS_OK;
    }

    /* Generate the proper unlock operation
     */
    if (stripe_lock_operation_p->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK)
    {
        fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation_p);
    }
    else if (stripe_lock_operation_p->opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK)
    {
        fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation_p);
    }
    else
    {
        /* Else this stripe lock operation isn't supported
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: unexpected opcode: 0x%x not supported\n", 
                              __FUNCTION__, stripe_lock_operation_p->opcode);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        return FBE_STATUS_OK;
    }

    /* Release the stripe lock
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_release_stripe_lock_completion, raid_group_p);
    fbe_stripe_lock_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_verify_release_stripe_lock()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_release_stripe_lock_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for releasing stripe lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_release_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);
    if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)
    {
        /* For some reason the stripe lock could not be released.
         * We will display an error.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: stripe lock failed status: 0x%x\n", 
                              __FUNCTION__, stripe_lock_status);
    }

    /* Since we are done with the stripe lock, release the operation.
     */
    fbe_payload_ex_release_stripe_lock_operation(payload_p, stripe_lock_operation_p);

    /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_verify_release_stripe_lock_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_metadata_of_metadata()
 ******************************************************************************
 * @brief
 *  This function marks metadata of metadata, after obtaining NP lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_mark_metadata_of_metadata(fbe_packet_t * packet_p, 
                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t    *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t        status;
    fbe_raid_verify_flags_t                 verify_flags;
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Fetched NP lock.\n",
                          __FUNCTION__);

    //fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NP_LOCK_HELD);

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);        

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    /* Set the block operation status to good since we might not even fill it in if we are not 
     * going to write the paged also. 
     */
    fbe_payload_block_set_status(block_operation_p, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    fbe_raid_group_verify_convert_block_opcode_to_verify_flags(raid_group_p, block_opcode, &verify_flags);

    status = fbe_raid_group_bitmap_mark_metadata_of_metadata_verify(packet_p,
                                                                    raid_group_p,
                                                                    verify_flags);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_metadata_of_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_metadata_of_metadata_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for marking metadata of metadata. 
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_mark_metadata_of_metadata_completion(fbe_packet_t * packet_p, 
                                                           fbe_packet_completion_context_t in_context)
{

    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)in_context;
    fbe_status_t                            status;
    fbe_raid_geometry_t *                   raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_lba_t                               paged_md_start_lba = FBE_LBA_INVALID;

    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s mark mdd failed, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p) ||
        !fbe_raid_group_block_opcode_is_user_initiated_verify(block_opcode) && (block_operation_p->lba >= paged_md_start_lba))
    {
        /* Only update non-paged, no paged.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_checkpoint_completion, raid_group_p);
        /* Get the NP lock for updating checkpoint. 
         */
        fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_verify_mark_checkpoint);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Update paged metadata.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_paged_metadata_completion, raid_group_p);        
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_mark_paged_metadata_verify, raid_group_p);        
    return FBE_STATUS_CONTINUE;
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_metadata_of_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for marking paged metadata. It starts to
 *  get NP lock for checkpoint update.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_mark_paged_metadata_completion(fbe_packet_t * packet_p, 
                                                     fbe_packet_completion_context_t in_context)
{

    fbe_raid_group_t*           raid_group_p = (fbe_raid_group_t*)in_context;
    fbe_status_t                status;

    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s mark paged metadata failed, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_checkpoint_completion, raid_group_p);

    /* Get the NP lock for updating checkpoint. 
     */
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_verify_mark_checkpoint);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_checkpoint()
 ******************************************************************************
 * @brief
 *  This function initiate verify checkpoint update after obtaining NP lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_mark_checkpoint(fbe_packet_t * packet_p, 
                                      fbe_packet_completion_context_t context)
{
    fbe_raid_group_t    *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t        status;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_NP_LOCK_TRACING,
                         "%s: Fetched NP lock.\n", __FUNCTION__);

    //fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NP_LOCK_HELD);

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);        

    status = fbe_raid_group_bitmap_initiate_verify_checkpoint_update(packet_p, raid_group_p);

    return status;
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_initiate_verify_checkpoint_update()
 ******************************************************************************
 * @brief
 *  This function initiate verify checkpoint update.
 *
 * @param packet_p - The pointer to the packet.
 * @param raid_group_p  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_bitmap_initiate_verify_checkpoint_update(fbe_packet_t *packet_p,
                                                        fbe_raid_group_t* raid_group_p)
{
      
    fbe_chunk_index_t                        chunk_index;    
    fbe_u64_t                                metadata_offset = FBE_LBA_INVALID; 
    fbe_chunk_count_t                        chunk_count;
    fbe_raid_verify_flags_t                  verify_flags;
    fbe_lba_t                                paged_md_start_lba = FBE_LBA_INVALID;
    fbe_lba_t                                paged_md_capacity = FBE_LBA_INVALID;
    fbe_status_t                             status;
    fbe_lba_t                                start_lba;
    fbe_block_count_t                        block_count;
    fbe_payload_ex_t *                       payload_p           = NULL;
    fbe_payload_block_operation_t *          block_operation_p   = NULL;
    fbe_payload_block_operation_opcode_t     block_opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    payload_p           = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    
    /* Depending upon the type of verify update the corresponding verify checkpoint in the 
     * non paged metadata.
     */     
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

    /* Get paged metadata start lba and its capacity.
     */
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config),
                                                         &paged_md_capacity);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_chunk_index_t paged_md_md_start_chunk_index, paged_md_md_end_chunk_index;
        fbe_chunk_count_t chunks_per_md_of_md;
        fbe_chunk_index_t end_chunk;    

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
                                                                     NULL,
                                                                     metadata_offset,
                                                                     verify_flags);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/******************************************************************************
 * end fbe_raid_group_bitmap_initiate_verify_checkpoint_update()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_checkpoint_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for checkpoint update.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_mark_checkpoint_completion(fbe_packet_t * packet_p, 
                                                 fbe_packet_completion_context_t in_context)
{

    fbe_raid_group_t*           raid_group_p = (fbe_raid_group_t*)in_context;
    fbe_status_t                status;

    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s mark checkpoint failed, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_checkpoint_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_iw_verify()
 ******************************************************************************
 * @brief
 *  This function processes incomplete write verify request.
 *
 * @param packet_p - The pointer to the packet.
 * @param raid_group_p  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
fbe_status_t 
fbe_raid_group_verify_mark_iw_verify(fbe_packet_t*               packet_p,
                                     fbe_raid_group_t*           raid_group_p)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_lba_t                               paged_md_start_lba = FBE_LBA_INVALID;
    fbe_lba_t                               paged_md_capacity  = FBE_LBA_INVALID;
    fbe_chunk_index_t                       chunk_index;        
    fbe_chunk_count_t                       chunk_count;        
    fbe_optimum_block_size_t                optimum_block_size;
    
    /* Get paged metadata start lba and its capacity. */
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);

    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config),
                                                         &paged_md_capacity);


    /* Convert the metadata start lba and block count into per disk chunk index and chunk count */
    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          paged_md_start_lba,
                                                          paged_md_capacity,
                                                          &chunk_index, 
                                                          &chunk_count);

    /* allocate a block operation */
    payload_p = fbe_transport_get_payload_ex(packet_p); 
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    
    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(((fbe_base_config_t *)raid_group_p)->block_edge_p, 
                                                    &optimum_block_size);
    /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY,
                                      paged_md_start_lba,
                                      paged_md_capacity,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);
    fbe_payload_ex_increment_block_operation_level(payload_p);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_verify_mark_iw_verify_completion,
                                          raid_group_p);

    /* Mark the calculated range of chunks as needing to be verified */
    status = fbe_raid_group_verify_process_verify_request(packet_p,
                                                          raid_group_p, 
                                                          chunk_index, 
                                                          FBE_RAID_VERIFY_TYPE_INCOMPLETE_WRITE, 
                                                          chunk_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_iw_verify()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_mark_iw_verify_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for incomplete write verify.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_mark_iw_verify_completion(fbe_packet_t*                   in_packet_p,
                                                fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*                       raid_group_p;          
    fbe_status_t                            status;
    fbe_payload_ex_t *                      payload_p;
    fbe_payload_block_operation_t *         block_operation_p;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    /* Get the raid group pointer from the input context */
    raid_group_p = (fbe_raid_group_t*) in_context; 

    /* Free the resources we allocated previously */
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
        
    status = fbe_transport_get_status_code(in_packet_p);
    if ((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: mark verify failed packet_status: 0x0%x block_status: 0x%x block_qual: 0x%x\n", 
                              __FUNCTION__, status, block_status, block_qualifier);
    }

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);    

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_verify_mark_iw_verify_completion()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_setup_verify_payload()
 ****************************************************************
 * @brief
 *  We add a block payload to track the marking of verify.
 * 
 *  The later completion functions need this operation to know
 *  exactly what is getting marked and for what kind of verify (opcode).
 *
 * @param raid_group_p   - The raid group.
 * @param packet_p       - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/3/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_setup_verify_payload(fbe_raid_group_t *raid_group_p,
                                                        fbe_packet_t *packet_p)
{
    fbe_raid_geometry_t                *raid_geometry_p = NULL;
    fbe_lba_t                           lba;
    fbe_block_count_t                   blocks;
    fbe_lba_t                           start_lba;
    fbe_lba_t                           end_lba;
    fbe_block_count_t                   block_count;           
    fbe_chunk_size_t                    chunk_size;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_block_operation_t      *block_operation_p = NULL;
    fbe_optimum_block_size_t            optimum_block_size;
    fbe_raid_iots_t*                    iots_p = NULL;
    fbe_payload_block_operation_opcode_t verify_opcode;
    fbe_u16_t                           data_disks;
    fbe_u32_t                           chunk_stripe_blocks;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);
    fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p); 
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    /* Take the host lba and block count and round it up to a full chunk stripe.
     * This is simply to be consistent with a true mark verify operation. 
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    chunk_stripe_blocks = (fbe_u32_t)(data_disks * chunk_size);
    end_lba = lba + blocks;
    start_lba = lba;
    if (start_lba % chunk_stripe_blocks)
    {
        start_lba -= (start_lba % chunk_stripe_blocks);
    }
    if (end_lba % chunk_stripe_blocks)
    {
        end_lba += chunk_stripe_blocks - (end_lba % chunk_stripe_blocks);
    }
    block_count = end_lba - start_lba;
    fbe_block_transport_edge_get_optimum_block_size(((fbe_base_config_t *)raid_group_p)->block_edge_p, 
                                                    &optimum_block_size);

    /* We only expect two different flavors of mark since 
     * there are only two different iots flags to indicate the mark type.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED)) {
        verify_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY;
    }
    else {
        verify_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY;
    }
    fbe_payload_block_build_operation(block_operation_p,
                                      verify_opcode,
                                      start_lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);
    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_raid_group_setup_verify_payload()
 *******************************************************************/

/*!**************************************************************
 * fbe_raid_group_mark_needs_verify_block_op_completion()
 ****************************************************************
 * @brief
 *  Handle a completion where we need to remove the
 *  payload that we added for the mark for verify.
 *
 * @param packet_p - Packet that is completing.
 * @param context - This is the raid group ptr.
 *
 * @return fbe_status_t.
 *
 * @author
 *  5/8/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_mark_needs_verify_block_op_completion(fbe_packet_t * packet_p, 
                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_lba(iots_p, &lba);
    fbe_raid_iots_get_blocks(iots_p, &blocks);

    status = fbe_transport_get_status_code(packet_p);

    /* The block operation was allocated when we started this operation. 
     * In all cases any block operation status was moved up to the packet status, 
     * so we do not need this block operation further, release it. 
     */
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    if (block_operation_p != NULL) {
        fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);
    }
    else {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: mark nv no block op iots op: 0x%x lba: 0x%llx blks: 0x%llx pkt_st: 0x%x\n",  
                              opcode, lba, blocks, status);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_mark_needs_verify_block_op_completion()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_mark_mdd_for_verify()
 ****************************************************************
 * @brief
 *  Mark the metadata of metadata as needing
 *  a verify for this object.
 * 
 *  We hand this off to the monitor to allow it to do the mark.
 *  This is needed in some cases since if a monitor op is
 *  performing a paged op and gets an error, it already has the
 *  NP lock and cannot mark the paged.  
 *
 * @param raid_group_p   - The raid group.
 * @param packet_p       - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/19/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_mark_mdd_for_verify(fbe_raid_group_t*   raid_group_p,
                                                       fbe_packet_t*       packet_p)
{
    fbe_status_t                        status;
    fbe_chunk_count_t                   chunk_count;
    fbe_chunk_index_t                   chunk_index;
    fbe_lba_t                           lba;
    fbe_block_count_t                   blocks;
    fbe_chunk_size_t                    chunk_size;
    fbe_raid_geometry_t*                raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t*                  sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t*                    iots_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);
    fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);

    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s metadata error at lba: %llx bl: 0x%x\n", __FUNCTION__, lba, (unsigned int)blocks);

        /* For Remap required read errors mark metadata region for error verify.
         */
        if(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE))
        {
            /* For Incomplete write errors set the glitching drive bitmask in
             * non paged metadata. Eval rebuild logging will trigger incomplete
             * write verify if required.
             */
            fbe_u64_t   metadata_offset = 0;

            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Glitching disk bitmask: 0x%x\n",
                                  __FUNCTION__,
                                  iots_p->dead_err_bitmap);

            metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->glitching_disks_bitmask);

            fbe_base_config_metadata_nonpaged_set_bits((fbe_base_config_t*) raid_group_p, 
                                                       packet_p,
                                                       metadata_offset,
                                                       (fbe_u8_t*) &(iots_p->dead_err_bitmap),
                                                       sizeof(iots_p->dead_err_bitmap), 
                                                       1);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    /* Determine the range on the raid group this lba and block count uses. 
     */
    status = fbe_raid_geometry_calculate_chunk_range(raid_geometry_p,
                                                     lba, blocks, chunk_size, &chunk_index, &chunk_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s calculate chunk range error status %d\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (chunk_count == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s chunk count: 0x%x chunk_index: 0x%llx\n", 
                              __FUNCTION__, chunk_count,
                  (unsigned long long)chunk_index);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, packet %p, lba:0x%llx, bl:0x%llx, ch_idx:0x%llx, ch_count:%d\n",
                          __FUNCTION__, packet_p, (unsigned long long)lba, (unsigned long long)blocks, 
                          (unsigned long long)chunk_index, chunk_count );

    /* We do not want this packet to get cancelled, since this must complete.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL); 

    /* Mark the LUN extent as needing to be verified.  First setp is nonpaged. 
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED))
    {
        fbe_raid_verify_initiate_error_verify_event(raid_group_p, packet_p, chunk_index, chunk_count,
                                                    FBE_DATA_EVENT_TYPE_MARK_VERIFY);
    }
    else
    {
        fbe_raid_verify_initiate_error_verify_event(raid_group_p, packet_p, chunk_index, chunk_count,
                                                    FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY);
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/**************************************
 * End fbe_raid_group_mark_mdd_for_verify()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_mark_for_verify()
 ****************************************************************
 * @brief
 *  This function initiates a mark of verify for this I/O
 *  that just completed.
 *  
 *  Note that this mark is done inline by marking the paged for the user area.
 *  Once that mark is done we will update the checkpoint.
 *
 *  The mark of the md of md (for paged errors) is handed off to the
 *  monitor to for simplicity since there are some cases where we already
 *  hold the NP lock.
 *
 *  The reason this needs to be done inline for the user area
 *  is because theoretically every single I/O could get an error 
 *  and need to mark.  Thus dispatching this to a thread will not work 
 *  since the memory required to dispatch the work will not scale.  
 *
 *  The prior approach we tried was using the object
 *  event queue to queue work to mark the paged.  However, the number of 
 *  events per raid group was on the order of thousands
 *  and was not bounded in any way as the dispatch thread (monitor) could
 *  not keep up with the incoming rate of events to mark.
 *  We still use this method for the paged since the number of events is
 *  bounded to 1 (to mark the entire paged).
 *
 * @param raid_group_p   - The raid group.
 * @param packet_p       - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_mark_for_verify(fbe_raid_group_t *raid_group_p,
                                                        fbe_packet_t *packet_p)
{
    fbe_status_t                        status;
    fbe_lba_t                           lba;
    fbe_block_count_t                   blocks;
    fbe_raid_geometry_t*                raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t*                   sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t*                    iots_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                         "%s, packet %p, lba:0x%llx, bl:0x%llx\n",
                         __FUNCTION__, packet_p, (unsigned long long)lba, (unsigned long long)blocks );

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p) ||
        fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba)) {

        /* This path does not do the mark inline, it hands off to the monitor. 
         * Why?  Because in some cases background ops that touch the metadata already have 
         * the NP lock and cannot get it again.  We could special case this here 
         * to not get the NP lock, but it is cleaner to just use the monitor context to mark in all cases.
         */
        return fbe_raid_group_mark_mdd_for_verify(raid_group_p, packet_p);
    }

    /* We do not want this packet to get cancelled, since this must complete.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL); 

   /* Setup a block operation as the following code we will be traversing expects our
    * block opcode to reflect the type of mark we need to make. 
    */
    status = fbe_raid_group_setup_verify_payload(raid_group_p, packet_p);
    if (status == FBE_STATUS_OK) {

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_mark_needs_verify_block_op_completion, raid_group_p);
        /* If we previously got an error and are now restarting, we need to set status to good,
         * Otherwise the following states will think we had an error and will bail out. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        /* In this case we only mark paged since we took the error on the user area. 
         * Normally when we start a verify on the user area, we always mark the paged. 
         * However, we do not want to verify the paged, since we might have a high incoming rate of errors.
         * With a high incoming rate of errors we would spend all of our time marking and verifying the paged. 
         * Also keep in mind that marking the md of md constantly is very expensive as it consumes the system 
         * drives. 
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_mark_paged_metadata_completion, raid_group_p);        
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_mark_paged_metadata_verify, raid_group_p);        
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }
    else {
        /* Our setup for mark of verify failed for an unknown reason.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, __LINE__);
        fbe_transport_complete_packet(packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * End fbe_raid_group_mark_for_verify()
 **************************************/

/******************************
 * end fbe_raid_verify.c
 ******************************/
