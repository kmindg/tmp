/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_metadata.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the LUN object's s metadata related functionality.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   02/04/2010:  Created. Dhaval Patel
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe_lun_private.h"
#include "fbe_transport_memory.h"

/*****************************************
 * LOCAL FUNCTION FORWARD DECLARATION
 *****************************************/
static fbe_status_t 
fbe_lun_metadata_nonpaged_set_generation_num_completion(fbe_packet_t * packet, 
                                                                                 fbe_packet_completion_context_t context);


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
/*!**************************************************************
 * fbe_lun_metadata_is_initial_verify_needed()
 ****************************************************************
 * @brief
 *  Determine if the initial verify is needed.
 *
 * @param lun_p - current lun object ptr.               
 *
 * @return FBE_TRUE if the initial verify is needed at this time.
 *         FBE_FALSE if no initial verify is needed (or was already run).
 *
 * @author
 *  3/27/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_lun_metadata_is_initial_verify_needed(fbe_lun_t *lun_p)
{
    /* If they want the initial verify (meaning they did not set the no initial verify), 
     * and if the initial verify has not been run, then we should kick it off. 
     *  
     */
    if (!fbe_lun_get_noinitialverify_b(lun_p) &&
        !fbe_lun_metadata_was_initial_verify_run(lun_p))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/******************************************
 * end fbe_lun_metadata_is_initial_verify_needed()
 ******************************************/

/*!**************************************************************
 * fbe_lun_metadata_was_initial_verify_run()
 ****************************************************************
 * @brief
 *  Determine if the initial verify already ran.
 *
 * @param lun_p - current lun object ptr.               
 *
 * @return FBE_TRUE initial verify already ran.
 *         FBE_FALSE initial verify did not run.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_lun_metadata_was_initial_verify_run(fbe_lun_t *lun_p)
{
    fbe_lun_nonpaged_metadata_t *lun_nonpaged_metadata_p;
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) lun_p, (void **)&lun_nonpaged_metadata_p);

    return ((lun_nonpaged_metadata_p->flags & FBE_LUN_NONPAGED_FLAGS_INITIAL_VERIFY_RAN) != 0);
}
/******************************************
 * end fbe_lun_metadata_was_initial_verify_run()
 ******************************************/

/*!****************************************************************************
 * fbe_lun_metadata_initialize_metadata_element()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata element for the lun
 *   object.
 *
 * @param lun_p         - pointer to the provision drive
 * @param packet_p      - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   12/16/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_lun_metadata_initialize_metadata_element(fbe_lun_t * lun_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload_p           = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_lba_t                           imported_capacity   = FBE_LBA_INVALID;
    fbe_lba_t                           exported_capacity   = FBE_LBA_INVALID;
    fbe_lba_t                           metadata_capacity   = FBE_LBA_INVALID;
    fbe_lba_t                           blocks_per_chunk    = 2048;
    fbe_block_edge_t *                  block_edge_p        = NULL; 
    
    /* Get the imported capacity for the LUN object. */
    fbe_base_config_get_block_edge((fbe_base_config_t *) lun_p, &block_edge_p, 0);
    fbe_block_transport_edge_get_capacity(block_edge_p, &imported_capacity);

    /* Get the exported capacity of the LUN object. */
    fbe_base_config_get_capacity((fbe_base_config_t *) lun_p, &exported_capacity);

    /* Round the exported capacity up to the nearest 1MB chunk */
    if(exported_capacity % blocks_per_chunk) {
        exported_capacity += blocks_per_chunk - (exported_capacity % blocks_per_chunk);
    }

    metadata_capacity   = imported_capacity - (exported_capacity + FBE_LUN_DIRTY_FLAGS_SIZE_BLOCKS);

    if(metadata_capacity == 0) {
        EmcpalDebugBreakPoint();
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_base_config_metadata_set_paged_record_start_lba((fbe_base_config_t *) lun_p,
                                                        exported_capacity);

    fbe_base_config_metadata_set_paged_mirror_metadata_offset((fbe_base_config_t *) lun_p,
                                                              FBE_LBA_INVALID);

    fbe_base_config_metadata_set_paged_metadata_capacity((fbe_base_config_t *) lun_p,
                                                        metadata_capacity);

    /*! @todo clean up */
    lun_p->dirty_flags_start_lba = exported_capacity + metadata_capacity;
    
    /* Simply set the number of stripes to the lu capacity divided by chunk size */
    fbe_base_config_metadata_set_number_of_stripes((fbe_base_config_t *) lun_p,
                                                   (imported_capacity / FBE_LUN_CHUNK_SIZE));

    /* complete the packet with good status here. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_metadata_initialize_metadata_element()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_nonpaged_metadata_init()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the nonpaged metadata memory of the
 *  lun object.
 *
 * @param lun_p         - pointer to the lun
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_nonpaged_metadata_init(fbe_lun_t * lun_p, fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* call the base config nonpaged metadata init to initialize the metadata. */
    status = fbe_base_config_metadata_nonpaged_init((fbe_base_config_t *) lun_p,
                                                    sizeof(fbe_lun_nonpaged_metadata_t),
                                                    packet_p);
    return status;
}
/******************************************************************************
 * end fbe_lun_nonpaged_metadata_init()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_write_default_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged metadata.
 *
 * @param lun_p         - pointer to the provision drive
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_metadata_write_default_nonpaged_metadata(fbe_lun_t * lun_p,
                                                 fbe_packet_t *packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_is_active;
    fbe_lun_nonpaged_metadata_t lun_nonpaged_metadata;
 
    /*! @note We should not be here if this is not the active SP.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)lun_p);
    if (b_is_active == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected passive set.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
          
    /* We must zero the local structure since it will be used to set all the
     * default metadata values.
     */
    fbe_zero_memory(&lun_nonpaged_metadata, sizeof(fbe_lun_nonpaged_metadata_t));

    /* Now set the default non-paged metadata.
     */
    status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)lun_p,
                                                           (fbe_base_config_nonpaged_metadata_t *)&lun_nonpaged_metadata);
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                             FBE_TRACE_LEVEL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Set default base config nonpaged failed status: 0x%x\n",
                             __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

       return status;
    }

    /* Now set the lun specific fields.
     */
    lun_nonpaged_metadata.zero_checkpoint = 0;
    lun_nonpaged_metadata.has_been_written = FBE_FALSE;
    lun_nonpaged_metadata.flags = FBE_LUN_NONPAGED_FLAGS_NONE;
   
    /* Send the nonpaged metadata write request to metadata service. 
     */
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) lun_p,
                                                    packet_p,
                                                    0,
                                                    (fbe_u8_t *) &lun_nonpaged_metadata, /* non paged metadata memory. */
                                                    sizeof(fbe_lun_nonpaged_metadata_t)); /* size of the non paged metadata. */

    /* Return status of write default non-paged.
     */
    return status;
}
/******************************************************************************
 * end fbe_lun_metadata_write_default_nonpaged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_write_default_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the paged metadata with default values
 *
 * @param lun_p         - pointer to the lun object
 * @param packet_p      - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/21/2012 - Created. MFerson.
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_metadata_write_default_paged_metadata(fbe_lun_t * lun_p,
                                              fbe_packet_t *packet_p)
{

    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u64_t                                       repeat_count = 0;
    fbe_lun_paged_metadata_t                        paged_metadata;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_metadata_operation_t *              metadata_operation_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_lba_t                                       paged_metadata_capacity = 0;

    /* get the payload and metadata operation. */
    sep_payload_p       = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Set the repeat count as number of records in paged metadata capacity.
     * This is the number of chunks of user data + paddings for alignment.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)lun_p,
                                                       &paged_metadata_capacity);

    /* Limit the size of paged metadata we initialize */
    /*
    if(paged_metadata_capacity > FBE_LUN_CHUNK_SIZE)
    {
        paged_metadata_capacity = FBE_LUN_CHUNK_SIZE;
    } 
    */

    if (fbe_lun_get_export_lun_b(lun_p)) {
        /* Also zero out the dirty flag (align paged metadata to chunk size) */
        paged_metadata_capacity = (paged_metadata_capacity + FBE_LUN_CHUNK_SIZE - 1) / FBE_LUN_CHUNK_SIZE * FBE_LUN_CHUNK_SIZE;
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s zero ext lun pagedmetadata, size: 0x%llx\n",
                              __FUNCTION__, paged_metadata_capacity);
    }

    repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE) / sizeof(fbe_lun_paged_metadata_t);

    /* Initialize the paged metadata with default values. */
    fbe_zero_memory(&paged_metadata, sizeof(paged_metadata));

    /* Allocate the metadata operation. */
    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p    = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) lun_p)->metadata_element),
                                                    0, /* Offset */
                                                    (fbe_u8_t *)&paged_metadata,
                                                    sizeof(paged_metadata),
                                                    repeat_count);
    if(status != FBE_STATUS_OK)
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_lun_metadata_write_default_paged_metadata_completion,
                                          lun_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);

    /* Issue the metadata operation. */
    status =  fbe_metadata_operation_entry(packet_p);

    return status;
}
/******************************************************************************
 * end fbe_lun_metadata_write_default_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_write_default_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the paged metadata with default values
 *
 * @param lun_p         - pointer to the lun
 * @param packet_p      - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/21/2012 - Created. MFerson.
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_lun_t *                         lun_p                   = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p    = NULL;
    fbe_payload_control_operation_t *   control_operation_p     = NULL;
    fbe_payload_ex_t *                  sep_payload_p           = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;
    
    lun_p = (fbe_lun_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
        
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "lun_mdata_wt_def_paged_mdata_compl write def paged metadata failed,stat:0x%x, mdata_stat:0x%x.\n",
                              status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_metadata_write_default_paged_metadata_completion()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_lun_metadata_get_zero_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to get the zero checkpoint for the lun object.
 *
 * @param   lun_p                   - pointer to the LUN Object
 * @param   zero_checkpoint_p       - pointer to the zero checkpoint.
 *
 * @return  fbe_status_t            - status of the operation.
 *
 * @author
 *   12/16/2009 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_metadata_get_zero_checkpoint(fbe_lun_t * lun_p,
                                     fbe_lba_t * zero_checkpoint_p)
{
    fbe_lun_nonpaged_metadata_t * lun_nonpaged_metadata = NULL;

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) lun_p, (void **)&lun_nonpaged_metadata);

    if (lun_nonpaged_metadata == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "lun_md_get_zero_checkpoint: non-paged MetaData is NULL for LUN object.\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *zero_checkpoint_p = lun_nonpaged_metadata->zero_checkpoint;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_metadata_get_zero_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_set_zero_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to set the zero checkpoint for the lun object.
 *
 * @param lun_p              - pointer to the provision drive
 * @param packet_p           - pointer to a monitor packet.
 * @param zero_checkpoint    - zero checkpoint.
 * @param persist_checkpoint - TRUE indicates a persist request.
 *
 * @return  fbe_status_t    - status of the operation.
 *
 * @author
 *   12/16/2009 - Created. Dhaval Patel
 *   07/02/2012 - Modified - Added persist option.
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_metadata_set_zero_checkpoint(fbe_lun_t * lun_p, 
                                     fbe_packet_t * packet_p,
                                     fbe_lba_t zero_checkpoint,
                                     fbe_bool_t persist_checkpoint)
{
    fbe_status_t    status;
    fbe_lba_t       imported_capacity = FBE_LBA_INVALID;

    /* get the paged metadatas start lba. */
    fbe_lun_get_imported_capacity(lun_p, &imported_capacity);

    /* check if disk zeroing completed */
    if((zero_checkpoint != FBE_LBA_INVALID) && 
       (zero_checkpoint  >= imported_capacity))
    {
        /* lun zeroing gets completed */
        fbe_base_object_trace((fbe_base_object_t *)lun_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:checkpoint cannot be updated beyond import capacity, zero_chk:0x%llx, export_cap:0x%llx\n",
                              __FUNCTION__,
			      (unsigned long long)zero_checkpoint,
			      (unsigned long long)imported_capacity);
        zero_checkpoint = FBE_LBA_INVALID;
    }

    /*!@todo we might need to handle the nonpaged metadata write explicitly to
     * pass the error message to the caller.
     */

    if(persist_checkpoint)
    {
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) lun_p,
                                                         packet_p,
                                                         (fbe_u64_t)(&((fbe_lun_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                         (fbe_u8_t *)&zero_checkpoint,
                                                         sizeof(fbe_lba_t));


    }
    else
    {
        status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) lun_p,
                                                         packet_p,
                                                         (fbe_u64_t)(&((fbe_lun_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                         (fbe_u8_t *)&zero_checkpoint,
                                                         sizeof(fbe_lba_t));
    }
    return status;
}
/******************************************************************************
 * end fbe_lun_metadata_set_zero_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_set_lun_has_been_written()
 ******************************************************************************
 * @brief
 *   This function is used to set the fact the lun has user data on it now.
 *
 * @param lun_p             - pointer to the provision drive
 * @param packet_p          - pointer to a monitor packet.
 *
 * @return  fbe_status_t    - status of the operation.
 *
 ******************************************************************************/
fbe_status_t 
fbe_lun_metadata_set_lun_has_been_written(fbe_lun_t * lun_p, 
                                          fbe_packet_t * packet_p)
{
    fbe_status_t    status;
    fbe_bool_t		lun_has_data = FBE_TRUE;
    
    /*!@todo we might need to handle the nonpaged metadata write explicitly to
     * pass the error message to the caller.
     */

    
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) lun_p,
                                                         packet_p,
                                                         (fbe_u64_t)(&((fbe_lun_nonpaged_metadata_t*)0)->has_been_written),
                                                         (fbe_u8_t *)&lun_has_data,
                                                         sizeof(fbe_bool_t));

    return status;
}

/******************************************************************************
 * end fbe_lun_metadata_set_lun_has_been_written()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_metadata_nonpaged_set_generation_num_completion()
 ******************************************************************************
 * @brief
 * This is the complete function of setting the lun generation number.
 *
 * @param packet    - pointer to the input packet.
 * @param context   - input context.
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  02/27/2012 - Created.   He, Wei
 ******************************************************************************/
 
 static fbe_status_t 
 fbe_lun_metadata_nonpaged_set_generation_num_completion(fbe_packet_t * packet, 
                                                                                  fbe_packet_completion_context_t context)
 {
     fbe_payload_ex_t * sep_payload = NULL;
     fbe_payload_metadata_operation_t * metadata_operation = NULL;
     fbe_status_t                        status;
     fbe_payload_metadata_status_t       metadata_status;
 
     sep_payload = fbe_transport_get_payload_ex(packet);
     metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
     
     status = fbe_transport_get_status_code(packet);
     fbe_payload_metadata_get_status(metadata_operation, &metadata_status);
 
     fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);
 
     if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
     {
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);    
         return FBE_STATUS_GENERIC_FAILURE;
     }
 
     return FBE_STATUS_OK;
 }
 
 /******************************************************************************
  * end fbe_lun_metadata_nonpaged_set_generation_num_completion()
  ******************************************************************************/

 /*!****************************************************************************
  * fbe_lun_metadata_nonpaged_set_generation_num()
  ******************************************************************************
  * @brief
  * This function is used to set the lun generation number in disk of nonpaged metadata.
  *
  * @param packet    - pointer to the input packet.
  * @param context    - input context.
  *
  * @return status           - The status of the operation.
  *
  * @author
  *  02/27/2012 - Created.   He, Wei
  ******************************************************************************/
  
 fbe_status_t fbe_lun_metadata_nonpaged_set_generation_num(fbe_base_config_t * base_config,
                                                                               fbe_packet_t * packet,
                                                                               fbe_u64_t   metadata_offset,
                                                                               fbe_u8_t *  metadata_record_data,
                                                                               fbe_u32_t metadata_record_data_size)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
    
    fbe_payload_metadata_build_nonpaged_preset(metadata_operation, 
                                               metadata_record_data,
                                               FBE_PAYLOAD_METADATA_MAX_DATA_SIZE,
                                               base_config->base_object.object_id);

    fbe_transport_set_completion_function(packet, fbe_lun_metadata_nonpaged_set_generation_num_completion, base_config);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}
 
 /******************************************************************************
  * end fbe_lun_metadata_nonpaged_set_generation_num()
  ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_is_peer_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if the peer has set this flag.
 * 
 * @param lun_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/5/2012 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t fbe_lun_is_peer_clustered_flag_set(fbe_lun_t * lun_p, fbe_lun_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_lun_metadata_memory_t * lun_metadata_memory_p = NULL;

    if (lun_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    lun_metadata_memory_p = (fbe_lun_metadata_memory_t *)lun_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;

    if (lun_metadata_memory_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if (((lun_metadata_memory_p->flags & flags) == flags))
    {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/**************************************
 * end fbe_lun_is_peer_clustered_flag_set()
 **************************************/

/*!**************************************************************
 * fbe_lun_set_clustered_flag()
 ****************************************************************
 * @brief
 *  Set the local clustered flags and update peer if needed.
 *
 * @param lun_p - The object.
 * @param flags - The flags to clear.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/5/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_lun_set_clustered_flag(fbe_lun_t * lun_p, fbe_lun_clustered_flags_t flags)
{
    fbe_lun_metadata_memory_t * lun_metadata_memory = NULL;

    if (lun_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_metadata_memory = (fbe_lun_metadata_memory_t *)lun_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (lun_metadata_memory == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!((lun_metadata_memory->flags & flags) == flags))
    {
        lun_metadata_memory->flags |= flags;
        fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const, (fbe_base_object_t*)lun_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_lun_set_clustered_flag()
 ******************************************/
/*!**************************************************************
 * fbe_lun_clear_clustered_flag()
 ****************************************************************
 * @brief
 *  Clear the local clustered flags and update peer if needed.
 *
 * @param lun_p - The object.
 * @param flags - The flags to clear.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/5/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_lun_clear_clustered_flag(fbe_lun_t * lun_p, fbe_lun_clustered_flags_t flags)
{
    fbe_lun_metadata_memory_t * lun_metadata_memory = NULL;

    if (lun_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_metadata_memory = (fbe_lun_metadata_memory_t *)lun_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (lun_metadata_memory == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((lun_metadata_memory->flags & flags) != 0)
    {
        lun_metadata_memory->flags &= ~flags;        
        fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const, 
                               (fbe_base_object_t*)lun_p, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_lun_clear_clustered_flag()
 ******************************************/

/*!****************************************************************************
 * fbe_lun_is_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if this flag is set locally.
 * 
 * @param lun_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/6/2012 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t fbe_lun_is_clustered_flag_set(fbe_lun_t * lun_p, fbe_lun_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_lun_metadata_memory_t * lun_metadata_memory_p = NULL;

    if (lun_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    lun_metadata_memory_p = (fbe_lun_metadata_memory_t *)lun_p->base_config.metadata_element.metadata_memory.memory_ptr;

    if (lun_metadata_memory_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if ((lun_metadata_memory_p->flags & flags) == flags)
    {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/**************************************
 * end fbe_lun_is_clustered_flag_set()
 **************************************/

fbe_status_t lun_process_memory_update_message(fbe_lun_t * lun_p)
{
    fbe_lun_metadata_memory_t * metadata_memory_ptr = NULL;
    fbe_lun_metadata_memory_t * metadata_memory_peer_ptr = NULL;
    fbe_lifecycle_state_t peer_state = FBE_LIFECYCLE_STATE_INVALID;

    if (lun_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        /* Nothing can be done - we are not initialized yet */
        return FBE_STATUS_OK;
    }

    fbe_metadata_element_get_metadata_memory(&lun_p->base_config.metadata_element, (void **)&metadata_memory_ptr);
    fbe_metadata_element_get_peer_metadata_memory(&lun_p->base_config.metadata_element, (void **)&metadata_memory_peer_ptr);

    fbe_base_object_trace((fbe_base_object_t*)lun_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "lun_proc_mem_updt_msg local flags: 0x%llx peer flags: 0x%llx\n",  
                          (unsigned long long)(metadata_memory_ptr) ? metadata_memory_ptr->flags : 0, 
                          (unsigned long long)(metadata_memory_peer_ptr) ? metadata_memory_peer_ptr->flags : 0);
    fbe_lun_lock(lun_p);

    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t*)lun_p, &peer_state);

    if ((peer_state == FBE_LIFECYCLE_STATE_ACTIVATE) &&
        fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED) &&
        fbe_lun_is_unexpected_error_limit_hit(lun_p))
    {
        /* The peer is trying to come up, we might need to bring ourselves out of FAIL.
         * Clear our unexpected errors and we'll bring outselves out of FAIL automatically. 
         */ 
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN peer err threshold cleared exit FAIL local: 0x%llx peer: 0x%llx pr_alive: %d\n",                               
                              lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              fbe_cmi_is_peer_alive());
        fbe_lun_reset_unexpected_errors(lun_p);
        fbe_lifecycle_reschedule(&fbe_lun_lifecycle_const, (fbe_base_object_t *) lun_p, (fbe_lifecycle_timer_msec_t) 0);
    }
    if (fbe_lun_is_peer_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST)&&
        !fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST))
    {
        /* Peer is trying to negotiate with us on errors.
         */ 
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN peer eval err req local: 0x%llx peer: 0x%llx pr_alive: %d\n",                               
                              lun_p->lun_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.flags,
                              fbe_cmi_is_peer_alive());

        /* Start running the same condition so we will negotiate with the peer.
         */
        fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const, 
                               (fbe_base_object_t*)lun_p, 
                               FBE_LUN_LIFECYCLE_COND_UNEXPECTED_ERROR);
    }
    fbe_lun_unlock(lun_p);
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_lun_metadata.c
 *******************************/
