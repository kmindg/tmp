/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_executor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the executer functions for the raid group object.
 * 
 *  This includes the fbe_raid_group_io_entry() function which is our entry
 *  point for functional packets.
 * 
 *  Opcodes supported:
 *  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS
 *    This performs a write operation where the raid library allocates the
 *    buffers for the write data and fills it with zeros.
 * 
 *  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS
 *    Performs a write of zeros using the "new" key.
 *    When this operation is done it will update the paged to indicate that
 *    this area is now rekeyed.
 * 
 *  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO
 *    Performs a zero using the "new" key.
 *    Used by the vault so we do not update the paged afterwards.
 * 
 *  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE
 *    Performs a write operation using the "new" key.
 *    Prior to starting this operation we will mark the write in progress.
 *    When this operation is done it will update the paged to indicate that
 *    this area is now rekeyed.
 *    
 * @ingroup raid_group_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe_raid_verify.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_library_proto.h"
#include "fbe_traffic_trace.h"
#include "EmcPAL_Misc.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t 
fbe_raid_group_update_block_sizes_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t
fbe_raid_group_update_block_sizes_memory_completion(fbe_memory_request_t *memory_request_p, 
                                                    fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_stripe_unlock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_mark_nr_for_iots_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_get_chunk_info_completion(fbe_packet_t* in_packet_p, 
                                                             fbe_packet_completion_context_t in_context);
fbe_bool_t fbe_raid_is_iots_chunk_info_clear_needed(fbe_raid_iots_t * iots_p, fbe_raid_group_t *raid_group_p);
static fbe_status_t fbe_raid_group_get_chunk_info_for_start_iots_completion(fbe_packet_t* in_packet_p,
                                                                            fbe_packet_completion_context_t in_context);
static fbe_status_t fbe_raid_group_iots_metadata_update_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_mark_for_verify_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_raid_state_status_t fbe_raid_group_retry_get_chunk_info_for_start_iots(fbe_raid_iots_t* in_iots_p);
static fbe_raid_state_status_t fbe_raid_group_retry_mark_nr_for_iots(fbe_raid_iots_t* in_iots_p);
static fbe_raid_state_status_t fbe_raid_group_iots_get_sl(fbe_raid_iots_t *iots_p);
static fbe_status_t fbe_raid_group_direct_io_sl_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_fast_write_sl_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_start_direct_io(fbe_raid_group_t *raid_group_p, 
                                                   fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_start_new_iots(fbe_raid_group_t *raid_group_p, 
                                                  fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_restart_failed_io(fbe_raid_group_t *raid_group_p, 
                                                     fbe_packet_t *packet_p,
                                                     fbe_raid_group_io_failure_type_t failure_type);
static fbe_status_t fbe_raid_group_start_direct_io_update_performance_stats(fbe_packet_t *packet_p, 
                                                                            fbe_raid_position_t position,
                                                                            fbe_block_count_t blocks,
                                                                            fbe_payload_block_operation_opcode_t opcode);
fbe_status_t fbe_mirror_optimize_determine_position(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_raid_position_t *position,
                                                    fbe_payload_block_operation_opcode_t opcode,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t blocks);
extern fbe_status_t fbe_mirror_optimize_direct_io_finish(fbe_raid_geometry_t *raid_geometry_p,
                                                  fbe_payload_block_operation_t *block_operation_p);

static fbe_status_t fbe_raid_group_mark_nr_control_operation_completion(fbe_packet_t * packet_p, 
                                                   fbe_packet_completion_context_t context);


/*!**************************************************************
 * fbe_raid_group_block_transport_entry()
 ****************************************************************
 * @brief
 *   This is the entry function that the block transport will
 *   call to pass a packet to the raid_group.
 *  
 * @param context - The raid_group ptr. 
 * @param packet_p - The packet to process.
 * 
 * @return fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_block_transport_entry(fbe_transport_entry_context_t context, 
                                                  fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_opcode_t block_opcode;

    /* Set resource priority for the packet.
     * Setting it here avoids setting at multiple other places.    
     */
    //fbe_raid_common_set_resource_priority(packet_p, fbe_raid_group_get_raid_geometry(raid_group_p));

    /* First, get the opcodes.
     */
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    /* Check to see if it is something we handle.
     */
    if (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE)
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
        fbe_lba_t lba;
        fbe_block_count_t blocks;
        fbe_bool_t b_degraded = fbe_raid_group_io_is_degraded(raid_group_p);
        fbe_memory_io_master_t *memory_master_p = NULL;

        memory_master_p = fbe_transport_memory_request_get_io_master(packet_p);

        fbe_payload_block_get_lba(block_operation_p, &lba);
        fbe_payload_block_get_block_count(block_operation_p, &blocks);

        if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) &&
            fbe_raid_geometry_is_parity_type(raid_geometry_p) &&
            b_degraded &&
            memory_master_p &&
            fbe_memory_is_reject_allowed_set(memory_master_p,
                                             FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE) &&
            !fbe_raid_geometry_is_stripe_aligned(raid_geometry_p, lba, blocks) &&
            fbe_raid_geometry_is_user_io(raid_geometry_p, lba))
        {
            /* For parity raid groups that are degraded we prefer to get full stripe writes in order to avoid the 
             * penalty of write journaling. 
             * The client said it was OK to reject this request and this is not a full stripe, so reject it now. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: non-full stripe rejected lba: 0x%llu bl: 0x%llu\n", 
                                  (unsigned long long)lba, (unsigned long long)blocks);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_FULL_STRIPE);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) || 
             (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) || 
             (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED)) &&
            fbe_base_config_is_global_path_attr_set((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED) &&
            memory_master_p &&
            fbe_memory_is_reject_allowed_set(memory_master_p,
                                             FBE_MEMORY_REJECT_ALLOWED_FLAG_NOT_PREFERRED) &&
            fbe_raid_geometry_is_user_io(raid_geometry_p, lba))
        {
            /* For any read/write I/O if we are not the preferred path and the client does not know it,
             * then reject the request with an error. 
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: non-full stripe rejected lba: 0x%llu bl: 0x%llu\n", 
                                  (unsigned long long)lba, (unsigned long long)blocks);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* When in slf, we will fail back all requests when the client is allowing us to fail them.
         */
        if (fbe_base_config_is_global_path_attr_set((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED) &&
            (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_NOT_PREFERRED)) {

            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_PENDING;
        }

        /* Only read and write non-metadata operations use direct I/O.
         */
        if (((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) || 
             (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)   )                         &&
            fbe_raid_group_is_small_io(raid_group_p, lba, blocks)                                   &&
            !b_degraded                                            &&
            ((block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CORRUPT_CRC) == 0) &&
             (raid_geometry_p->class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)                              &&
            fbe_raid_geometry_is_user_io(raid_geometry_p, lba) &&
            !fbe_raid_group_io_rekeying(raid_group_p))
        {
            /* Trace start if enabled.
             */
            if (fbe_raid_group_io_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING))
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                      FBE_TRACE_LEVEL_INFO, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: pkt:%p lba:%016llx blks:%08llx op: %d start\n",
                                      packet_p, (unsigned long long)lba,
				      (unsigned long long)blocks, block_opcode);
            }

            /*! @note In general we need a stripe lock if it is not a RAID-10 
             *        (for RAID-10 the downstream mirror raid group will take 
             *         the appropriate lock).
             *
             *        Note that this path does not support metadata I/Os.
             */
            if (raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
            {
                if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
                {
                    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
                        /* Unaligned reads cannot go down the fast path if they have extra sgs.
                         */
                        fbe_sg_element_t *sg_p = NULL;
                        fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);                        

                        if (!fbe_sg_element_bytes_is_exact_match(sg_p, (blocks*FBE_BE_BYTES_PER_BLOCK))) {
                            return fbe_raid_group_start_new_iots(raid_group_p, packet_p);
                        }
                    }
                    fbe_transport_set_completion_function(packet_p, fbe_raid_group_direct_io_sl_completion, raid_group_p);
                }
                else
                {
                    /* Writes cannot go down the fast path unless they do not have extra sgs.
                     */
                    fbe_sg_element_t *sg_p = NULL;
                    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

                    if (!fbe_sg_element_bytes_is_exact_match(sg_p, (blocks*FBE_BE_BYTES_PER_BLOCK))                         ||
                        fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY)    )
                    {
                        return fbe_raid_group_start_new_iots(raid_group_p, packet_p);
                    }
                    else
                    {
                        fbe_transport_set_completion_function(packet_p, fbe_raid_group_fast_write_sl_completion, raid_group_p);
                    }
                }
                fbe_raid_group_get_stripe_lock(raid_group_p, packet_p);
            }
            else
            {
                /* Else this is a RAID-10 and therefore no lock is required.
                 * Start the request using direct I/O.
                 */
                fbe_raid_group_start_direct_io(raid_group_p, packet_p);
            }
        }
        else if((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY)
                || (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY)
                || (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY)
                || (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY)
                || (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY))
        {
            fbe_raid_group_initiate_verify(raid_group_p, packet_p);
        }
        else if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA)
        {
            status = fbe_raid_group_process_lun_extent_init_metadata(raid_group_p, packet_p);
        }
        else if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT) {
            status = fbe_raid_group_start_set_encryption_chkpt(raid_group_p, packet_p);
        }
        else if ((fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p)) &&
                 (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) &&
                 (raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10) &&
                 (!(fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED))) &&
                 fbe_raid_geometry_is_user_io(raid_geometry_p, lba))
        {
            /* We dont want to allow Zeroing if RG's paged is not yet encrypted */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: Paged Encryption in progress \n");
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_PENDING;
        }
        else
        {
            /* Kick off the new iots.
             */
            status = fbe_raid_group_start_new_iots(raid_group_p, packet_p);
        }
    }
    else 
    {
        /* We handle this immediately without any need for an iots, siots.
         * The below function will return status immediately. 
         */
        return fbe_raid_group_negotiate_block_size(raid_group_p, packet_p);
    }
    /* Always return pending.  We will return status later once we are complete.
     */
    return FBE_STATUS_PENDING;
}
/**************************************
 * end fbe_raid_group_block_transport_entry()
 **************************************/


/*!***************************************************************************
 *          fbe_raid_group_iots_is_monitor_operation()
 *****************************************************************************
 *
 * @brief   Determines if the iots is a monitor type of operation
 *
 * @param   iots_p - Pointer to iots       
 *
 * @return  bool - FBE_TRUE - The request was issued by the raid group monitor
 *
 * @note    Need to add any new monitor operation code here.
 *
 *****************************************************************************/
static fbe_bool_t fbe_raid_group_iots_is_monitor_operation(fbe_raid_iots_t *iots_p)
{
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t  b_is_monitor_operation = FBE_FALSE;

    /* Get the block operation information from the iots
     */
    block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /*! @note Currently the raid group object simply uses the payload
     *        block operation check.
     */
    b_is_monitor_operation = fbe_payload_block_is_monitor_opcode(opcode);

    /* Return if the iots was originated from the monitor or not.
     */
    return b_is_monitor_operation;
}
/************************************************
 * end fbe_raid_group_iots_is_monitor_operation()
 ************************************************/

/*!***************************************************************
 * @fn fbe_raid_group_validate_packet()
 ****************************************************************
 * @brief
 *  This function does a sanity check of the block operation.
 *
 * @param object_handle - The raid_group handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK on success.
 *                 FBE_STATUS_GENERIC_FAILURE if something is wrong with
 *                 the block operation.
 *
 * @author
 *  1/31/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_validate_packet(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;

    if (sep_payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s payload is null packet: %p\n",
                              __FUNCTION__, packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s block operation is null packet: %p\n",
                              __FUNCTION__, packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* we just got this block operation.  Set the status of this block operation
     * to INVALID to indicate that we have not worked on it yet.  Because may set the 
     * block operation to Media Error when more than one iots are used, and we want to 
     * make sure the block operation does not have leftover status. 
     */
    fbe_payload_block_set_status(block_operation_p, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* Now check that we have a valid payload with SGs.
     */
    if (fbe_payload_block_operation_opcode_requires_sg(opcode))
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_payload_ex_get_sg_list(sep_payload_p, &sg_p, NULL);

        /* Make sure there is an sg in the payload.
         */
        if (sg_p == NULL)
        {
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s sg_p is null for opcode: %d packet: %p\n",
                                  __FUNCTION__, opcode, packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Make sure the sg list is not empty.
         */
        if ((sg_p->address == NULL) || (sg_p->count == 0))
        {
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s first sg no data opcode: %d packet: %p addr: 0x%x count: %d\n",
                                  __FUNCTION__, opcode, packet_p, (fbe_u32_t)sg_p->address, sg_p->count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_validate_packet
 **************************************/

/*!***************************************************************
 * @fn fbe_raid_group_io_entry(fbe_object_handle_t object_handle,
 *                                fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the raid_group object.
 *  The FBE infrastructure will call this function for our object.
 *
 * @param object_handle - The raid_group handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t * raid_group_p = NULL;

    raid_group_p = (fbe_raid_group_t *) fbe_base_handle_to_pointer(object_handle);

    /* Validate the block operation.
     */
    status = fbe_raid_group_validate_packet(raid_group_p, packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Simply set the cancel function and send this off to the server.
     * We need to go through the server since it is counting our I/Os. 
     * When we transition to pending the server handles keeping us in 
     * the pending state until I/Os have been drained. 
     */
    fbe_transport_set_cancel_function(packet_p, 
                                      fbe_base_object_packet_cancel_function, 
                                      (fbe_base_object_t *)raid_group_p);
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return status;
}
/* end fbe_raid_group_io_entry() */

/*!**************************************************************
 * fbe_raid_group_iots_release_stripe_lock()
 ****************************************************************
 * @brief
 *  Release this iots lock.
 *
 * @param raid_group_p - current raid group.
 * @param iots_p - Current iots.
 *
 * @return fbe_status_t.
 *
 * @author
 *  2/1/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_iots_release_stripe_lock(fbe_raid_group_t *raid_group_p, fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_status_t status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Pull the packet off the termination queue since it causes issues when the stripe
     * lock service tries to complete a packet. 
     */
    status = fbe_raid_group_remove_from_terminator_queue(raid_group_p, 
                                                         fbe_raid_iots_get_packet(iots_p));
    if (status != FBE_STATUS_OK)
    {
        /* Some thing is wrong mark the iots with unexpected error but continue
         * since we want to free the stripe lock.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: error removing from terminator queue status: 0x%x\n", 
                              __FUNCTION__, status);

        /* Fail iots
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    }

    /* Get the stripe lock operation so that we can release the stripe lock
    */
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);
    if (stripe_lock_operation_p == NULL)
    {
        /* We expected a stripe lock. Fail the iots with `unexpected error'.
         * Just finish the iots.
         * We always procedute
         * Pull off the terminator queue since we are done with the I/O. 
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: lba: 0x%llx blks: 0x%llx no stripe lock operation\n", 
                              __FUNCTION__, (unsigned long long)iots_p->lba,
			      (unsigned long long)iots_p->blocks);

        /* Fail iots and finish the request.  Since we completed the iots
         * return success.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_raid_group_iots_finished(raid_group_p, iots_p);

        status = fbe_transport_complete_packet(packet_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: error completing packet status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_STATUS_OK;
    }

    /* Trace some basic information
     */
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                          "SL Unlock iots/pkt: %p/%p Release lba: 0x%llx bl: 0x%llx opcode: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          iots_p, packet_p, iots_p->lba, iots_p->blocks, opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);
    if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: unexpected stripe_lock_status: 0x%x \n", 
                              __FUNCTION__, stripe_lock_status);
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
                              __FUNCTION__, opcode);
        /* Fail iots and finish the request.  Since we completed the iots
         * return success.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_raid_group_iots_finished(raid_group_p, iots_p);

        status = fbe_transport_complete_packet(packet_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: error completing packet status: 0x%x\n", __FUNCTION__, status);
        }
        return FBE_STATUS_OK;
    }

    /* Release the stripe lock
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_stripe_unlock_completion, raid_group_p);
    fbe_stripe_lock_operation_entry(packet_p);
    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_raid_group_iots_release_stripe_lock()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_group_stripe_unlock_completion()
 ****************************************************************
 * @brief
 *  Handle allocation of the lock.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_stripe_unlock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    /* Since we are done with the stripe lock, release the operation.
     */
    fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);

    if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)
    {
        /* For some reason the stripe lock could not be released.
         * We will display an error.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: stripe lock failed status: 0x%x\n", __FUNCTION__, stripe_lock_status);
    }
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                          "SL Unlock done i/sl:%x/%x l/b: 0x%llx/0x%llx op: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          (fbe_u32_t)iots_p, (fbe_u32_t) stripe_lock_operation_p,
                          iots_p->lba, iots_p->blocks, iots_p->block_operation_p->block_opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    /* Continue to cleanup the iots.
     */
    status = fbe_raid_group_iots_finished(raid_group_p, iots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: error finishing iots status: 0x%x\n", __FUNCTION__, status);
    }
    /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_stripe_unlock_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_handle_was_quiesced_for_next()
 ****************************************************************
 * @brief
 *  Handle the fact that was quiesced was set
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  01/29/2013  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_handle_was_quiesced_for_next(fbe_raid_group_t *raid_group_p,
                                                         fbe_raid_iots_t *iots_p)
{
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED))
    {
        /* Because this was previously quiesced, we need to update our counter.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_dec_quiesced_count(raid_group_p);
        fbe_raid_group_unlock(raid_group_p);

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                             "raid_group: next cl was quiesced iots:%8p lba:%016llx blks:%08llx fl:0x%x cnt:%d\n",
                             iots_p, iots_p->lba, iots_p->blocks, iots_p->flags, raid_group_p->quiesced_count);

        /* Clear the flag.
         */
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************** 
 * end fbe_raid_group_handle_was_quiesced_for_next()
 *****************************************************/

/*!**************************************************************
 * fbe_raid_group_iots_finished()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 *
 * @param raid_group_p - current raid group.
 * @param iots_p - The iots that is completing. This has
 *                 Status in it to tell us if it is done.
 * 
 * @notes It is the responsibility of the caller to call
 *        fbe_transport_complete_packet()
 * 
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_iots_finished(fbe_raid_group_t *raid_group_p, fbe_raid_iots_t *iots_p)
{
    fbe_status_t status;
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);

    /* If error tracing is on, we want to stop on error.
     */
    if ( fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_USER_IOTS_ERROR) &&
        (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
         (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED)                                  &&
        !fbe_raid_group_iots_is_monitor_operation(iots_p)                                                       &&
        !fbe_raid_iots_is_metadata_request(iots_p)                                                                  )
    {
        fbe_trace_level_t   trace_level = FBE_TRACE_LEVEL_ERROR;

        /* Normally we don't want to stop for the virtual drive errors.
         */
        if (fbe_raid_group_get_class_id(raid_group_p) == FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            trace_level = FBE_TRACE_LEVEL_WARNING;
        }

        /* Trace `error' on user I/O was enabled and the I/O failed.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "iots finish i/pkt: %x/%x lba/bl: 0x%llx/0x%llx op: %d flags: 0x%x st/q: 0x%x/0x%x\n", 
                          (fbe_u32_t)iots_p, (fbe_u32_t)packet_p, iots_p->packet_lba, iots_p->packet_blocks, 
                          iots_p->block_operation_p->block_opcode, iots_p->flags,
                          iots_p->error, iots_p->qualifier);
    }
    else if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR) &&
             (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)                                             &&
             (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED)                                     &&
             fbe_raid_group_iots_is_monitor_operation(iots_p)                                                             )
    {
        /* Trace `error' on monitor operation I/O was enabled and the I/O failed.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                          "iots finish i/pkt: %x/%x lba/bl: 0x%llx/0x%llx op: %d flags: 0x%x st/q: 0x%x/0x%x\n", 
                          (fbe_u32_t)iots_p, (fbe_u32_t)packet_p, iots_p->packet_lba, iots_p->packet_blocks, 
                          iots_p->block_operation_p->block_opcode, iots_p->flags,
                          iots_p->error, iots_p->qualifier);
    }
    else if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING))
    {
        /* Else if tracing is enabled.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "iots finish i/pkt: %x/%x lba/bl: 0x%llx/0x%llx op: %d flags: 0x%x st/q: 0x%x/0x%x\n", 
                          (fbe_u32_t)iots_p, (fbe_u32_t)packet_p,
			  (unsigned long long)iots_p->packet_lba,
			  (unsigned long long)iots_p->packet_blocks, 
                          iots_p->block_operation_p->block_opcode, iots_p->flags,
                          iots_p->error, iots_p->qualifier);
    }

    /* Handle case where the paged metadata is corrupt.
     */
    if ((iots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST)                &&
        (iots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS)    )
    {
        /* Invoke the method to handle this error
         */
        fbe_raid_group_util_handle_too_many_dead_positions(raid_group_p, iots_p);
    }

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED))
    {
        /* Because this was previously quiesced, we need to update our counter.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_dec_quiesced_count(raid_group_p);
        fbe_raid_group_unlock(raid_group_p);

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                             "raid_group: clear was quiesced iots:0x%x lba/bl: 0x%llx/0x%llx flags:0x%x cnt:%d\n",
                             (fbe_u32_t)iots_p,
                             (unsigned long long)iots_p->lba,
                             (unsigned long long)iots_p->blocks,
                             iots_p->flags, raid_group_p->quiesced_count);

        /* Clear the flag.
         */
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);
    }

    /* We are done.  Set the status from the iots in the packet.
     */
    status = fbe_raid_iots_set_packet_status(iots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error setting packet status: 0x%x\n", 
                              __FUNCTION__, status);
    }
#if 0
    if ((iots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        ((!fbe_queue_is_empty(&iots_p->siots_queue)) ||
         (iots_p->outstanding_requests != 0)))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: iots being freed prematurely : 0x%x\n", 
                              __FUNCTION__, status);
    }
#endif
    /* Destroy the iots now that it is no longer on a queue.
     */
    status = fbe_raid_iots_destroy(iots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error destroying iots status: 0x%x\n", 
                              __FUNCTION__, status);
    }
    return status;
}
/******************************************
 * end fbe_raid_group_iots_finished()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_is_iots_stripe_lock_required()
 ****************************************************************
 * @brief
 *  Determine if an new stripe lock is required or not.  Also if
 *  hold is allowed or not.
 * 
 * @param raid_group_p - the raid group object.
 * @param iots_p - current I/O.
 *
 * @return bool - FBE_TRUE - Stripe lock required
 *
 * @author
 *  04/11/2011  Ron Proulx  - Created
 *
 ****************************************************************/

fbe_bool_t fbe_raid_group_is_iots_stripe_lock_required(fbe_raid_group_t *raid_group_p,
                                                       fbe_raid_iots_t *iots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_lba_t lba;
    fbe_payload_block_operation_opcode_t opcode;

    /* Get the block operation information from the iots
     */
    block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* Write_log flush is performed when RG is quiesced; so locks are not required.
    */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH))
    {
        return FBE_FALSE;
    }
    else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD)
    {
        /* MARK_FOR_REBUILD does not need the stripe lock
         */
        return FBE_FALSE;
    } 
    else if (fbe_raid_geometry_is_raid10(raid_geometry_p))
    {
        /* RAID-10 does not stripe lock.
         */
        return FBE_FALSE;
    }
    
    else if (fbe_raid_group_iots_is_monitor_operation(iots_p) == FBE_TRUE)
    {
        /* Any monitor operation will need the stripe lock
         */
        return FBE_TRUE;
    }
    else if ((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) ||
             fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba))
    {
        /* If the `metadata request' flag is set or this is a RAID-10 (mirror group
         * will get lock) don't take the lock.
         */
        return FBE_FALSE;
    }

    /* Locking is required, flag it as such
     */
    return FBE_TRUE;
}
/***************************************************
 * end fbe_raid_group_is_iots_stripe_lock_required()
 ***************************************************/
/*!**************************************************************
 * fbe_raid_group_get_stripe_lock()
 ****************************************************************
 * @brief
 *  Get an iots stripe lock for this request.
 * 
 * @param raid_group_p - the raid group object.
 * @param packet_p - Packet ptr.
 * 
 * @notes Assumes we are NOT on the terminator queue.
 * 
 * @return fbe_status_t.
 *
 * @author
 *  1/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_get_stripe_lock(fbe_raid_group_t *raid_group_p,
                                                   fbe_packet_t *packet_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t sl_lba;
    fbe_block_count_t sl_blocks;
    fbe_u64_t stripe_number;
    fbe_u64_t stripe_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t b_is_lba_disk_based;
    fbe_bool_t b_allow_hold;
    fbe_element_size_t element_size;
    fbe_block_count_t disk_capacity;
    fbe_payload_block_operation_flags_t block_flags;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);

    /* Get the block operation information
     */
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
    fbe_payload_block_get_flags(block_operation_p, &block_flags);

    b_is_lba_disk_based = fbe_raid_library_is_opcode_lba_disk_based(opcode);
    /* Determine what we should be locking.
     */
    if (b_is_lba_disk_based)
    {
        /* These use a physical address.
         */
        fbe_raid_geometry_calculate_lock_range_physical(raid_geometry_p,
                                                        lba,
                                                        blocks,
                                                        &stripe_number,
                                                        &stripe_count);
    }
    else
    {
        /* These use a logical address.
         */
        if (1 || opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
        {
            fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                                   lba,
                                                   blocks,
                                                   &stripe_number,
                                                   &stripe_count);
        }
        else
        {
            /* Zeros must lock aligned to a chunk stripe.
             */
            fbe_raid_geometry_calculate_zero_lock_range(raid_geometry_p,
                                                        lba,
                                                        blocks,
                                                        FBE_RAID_DEFAULT_CHUNK_SIZE,
                                                        &stripe_number,
                                                        &stripe_count);
        }
    }

    /* We might need to align the request to a different multiple if we are 4k.
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        fbe_raid_geometry_align_io(raid_geometry_p, &stripe_number, &stripe_count);
    }
    stripe_lock_operation_p = fbe_payload_ex_allocate_stripe_lock_operation(payload_p);

    /* Determine the setting for `allow hold'
     */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) ||       
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA) ||  
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD) ||
        (fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba)) ||
        (packet_p->packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE))
    {
        /* Any metadata operation or monitor operation cannot 
         * wait for stripe locks when we are quiescing or we will deadlock. 
         */
        b_allow_hold = FBE_FALSE;
    }
    else
    {
        b_allow_hold = FBE_TRUE;
    }

    sl_lba = stripe_number;
    sl_blocks = stripe_count;

    /* Build the stripe lock operation and start it.
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation_p, 
                                                &raid_group_p->base_config.metadata_element, 
                                                sl_lba, sl_blocks); 
    }
    else
    {
        fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation_p, 
                                                 &raid_group_p->base_config.metadata_element, 
                                                 sl_lba, sl_blocks);
    }
    if (block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL) {
        /* In this case we want to skip the stripe lock.
         */
        stripe_lock_operation_p->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_DO_NOT_LOCK;
    }

	if(b_allow_hold){ /* TODO Deprecated nedd to be removed */
		stripe_lock_operation_p->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD;
	}


    fbe_payload_ex_increment_stripe_lock_operation_level(payload_p);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                          "SL get pkt: %x/%x get lba: 0x%llx bl: 0x%llx opcode: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          (fbe_u32_t)packet_p, (fbe_u32_t)stripe_lock_operation_p, lba, blocks, opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    /* If we attempted a stripe lock request that is beyond the configured
     * capacity fail the request.
     */
    disk_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    if (stripe_lock_operation_p->stripe.last > disk_capacity)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RG SL last 0x%llx > exported disk cap 0x%llx\n", 
                              stripe_lock_operation_p->stripe.last, disk_capacity);

        /*! @note Mark the stripe lock as failed and invoke the callback. 
         */
        fbe_payload_stripe_lock_set_status(stripe_lock_operation_p, FBE_PAYLOAD_STRIPE_LOCK_STATUS_ILLEGAL_REQUEST);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_stripe_lock_operation_entry(packet_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_get_stripe_lock()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_stripe_lock_completion()
 ****************************************************************
 * @brief
 *  Handle allocation of the lock.
 *
 * @param packet_p - Packet that is completing.
 * @param context - the raid group. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t packet_status;
    fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    packet_status = fbe_transport_get_status_code(packet_p);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    /* Validate that a stripe lock operaiton was requested
     */
    if (stripe_lock_operation_p == NULL)
    {
        /* Something went wrong
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "SL no stripe lock opt: packet_status: 0x%x op: %d\n", 
                              packet_status, opcode);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        status = fbe_raid_group_iots_finished(raid_group_p, iots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_sl_compl, SL error finishing iots status: 0x%x\n", status);
        }
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }

    /* Get the stripe lock operation
     */
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                          "SL done iots_p:0x%x lba:0x%llx bl:0x%llx opcode:0x%x s_n:0x%llx s_c:0x%llx\n", 
                          (fbe_u32_t)iots_p, iots_p->lba, iots_p->blocks, opcode,
                          stripe_lock_operation_p->stripe.first, 
						  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status == FBE_STATUS_OK) )
    {
        /* Terminate the I/O since we will be sending this to the library.
         */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
    }
    else if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status != FBE_STATUS_OK) )
    {
        /* We have our lock, but the packet was cancelled on the way back.
         * We only expect cancelled status here, so other status values trace with error.
         */
        if (packet_status != FBE_STATUS_CANCELED)
        {
            trace_level = FBE_TRACE_LEVEL_ERROR;
        }
        /* In this case we need to release the lock and fail the operation.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                              "SL bad status/lock held sl_status: 0x%x packet_status: 0x%x op: %d\n", 
                              stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);

        /* Below code assumes we are on the queue.
         */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        /* After we release the lock, the iots is freed, and packet completed.
         */
        fbe_raid_group_iots_release_stripe_lock(raid_group_p, iots_p);
    }
    else
    {
        /* For some reason the stripe lock was not available.
         * We will fail the request. 
         */
        if (((stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) ||
             (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED)) &&
            ((stripe_lock_operation_p->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD) != 0))
        {
            /* For dropped requests we will enqueue to the terminator queue to allow this request to
             * get processed later.  We were dropped because of a drive going away or a quiesce. 
             * Once the monitor resumes I/O, this request will get restarted. 
             *  
             * Monitor or metadata requests do not get quiesced to the terminator queue, 
             * thus we will fail those requests back instead of queuing.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL get failed queuing sl_status: 0x%x pckt_status: 0x%x op: %d\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);

            fbe_raid_iots_transition_quiesced(iots_p, fbe_raid_group_iots_get_sl, raid_group_p);
            fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
            fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        }
        if (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED)
        {
            /* We intentionally aborted this lock due to shutdown.
             */
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                                  "SL aborted sl_status: 0x%x packet_status: 0x%x op: %d\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        }
        else if ((packet_status == FBE_STATUS_CANCELED) || 
                 (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_CANCELLED))
        {
            /* We only check for cancelled.  Only cancelled means that the MDS has returned this as cancelled. 
             */
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                                  "SL cancelled iots: 0x%x sl_status: 0x%x pkt_st: 0x%x op: %d\n", 
                                  stripe_lock_status, (fbe_u32_t) iots_p, packet_status, stripe_lock_operation_p->opcode);
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_sl_compl:iots_p:%p cancelled lba:0x%llx bl:0x%llx opcode:0x%x s_n:0x%llx s_c:0x%llx\n", 
                                  iots_p, iots_p->lba, iots_p->blocks, opcode,
                                  stripe_lock_operation_p->stripe.first,
								  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);
        }
        else
        {
            /* Other errors should only get reported on metadata ops, which will specifically handle this error.
             */
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED);

            if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED)
            {
                /* We expect dropped/aborted/cancelled, but no other status values.
                 */
                trace_level = FBE_TRACE_LEVEL_ERROR;

                /* Change the status if it was an illegal request.
                 */
                if (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ILLEGAL_REQUEST)
                {
                    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
                }
            }
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL failed sl_status: 0x%x packet_status: 0x%x op: %d\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode);
        }
        fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
        status = fbe_raid_group_iots_finished(raid_group_p, iots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "rg_sl_compl, SL error finishing iots status: 0x%x\n",  status);
        }
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_stripe_lock_completion()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_iots_get_sl()
 ****************************************************************
 * @brief
 *  Restart the iots after it has been queued.
 *  It was queued just before getting a lock
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  8/8/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_group_iots_get_sl(fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)iots_p->callback_context;
    fbe_payload_ex_t *sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: restarting iots %p\n", __FUNCTION__, iots_p);

    /* Remove from the terminator queue and get the stripe lock.
     */
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_stripe_lock_completion, raid_group_p);
    fbe_raid_group_get_stripe_lock(raid_group_p, packet_p);
    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_raid_group_iots_get_sl()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_direct_io_unlock_completion()
 ****************************************************************
 * @brief
 *  Handle unlock completion for direct path.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  2/2/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_direct_io_unlock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    /* Since we are done with the stripe lock, release the operation.
     */
    fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);

    if (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)
    {
        /* For some reason the stripe lock could not be released.
         * We will display an error.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: stripe lock failed status: 0x%x\n", __FUNCTION__, stripe_lock_status);
    }

    /* If tracing is enabled.
     */
    if (fbe_raid_group_io_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING))
    {
        fbe_payload_block_operation_t  *block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
        
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "SL Unlock done b/sl:%x/%x l/b: 0x%llx/0x%llx op: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                             (fbe_u32_t)block_operation_p, (fbe_u32_t)stripe_lock_operation_p,
                             block_operation_p->lba, block_operation_p->block_count, block_operation_p->block_opcode,
                             stripe_lock_operation_p->stripe.first,
							 stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);
    }

    /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
     */
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_direct_io_unlock_completion()
 **************************************************/

static fbe_status_t fbe_raid_group_check_lba_stamp_checksum(fbe_raid_group_t *raid_group_p,
                                                            fbe_payload_block_operation_t *block_operation_p,
                                                            fbe_sg_element_t *sg_p)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps; 

    if ((block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM) == 0)
    {
        /* Only check the lba stamps.
         */
        execute_stamps.fru[0].sg = sg_p;
        execute_stamps.fru[0].seed = block_operation_p->lba;
        execute_stamps.fru[0].count = block_operation_p->block_count;
        status = fbe_xor_lib_check_lba_stamp(&execute_stamps);
    }
    else
    {
        fbe_raid_geometry_t *raid_geometry_p = &raid_group_p->geo;
        fbe_u32_t width;
        /* Check both lba stamp and checksum.
         */
        fbe_xor_lib_execute_stamps_init(&execute_stamps);
        execute_stamps.fru[0].sg = sg_p;
        execute_stamps.fru[0].seed = block_operation_p->lba;
        execute_stamps.fru[0].count = block_operation_p->block_count;
        execute_stamps.fru[0].offset = 0;
        execute_stamps.fru[0].position_mask = 0;

        /* Setup the drive count and eboard pointer.
         */
        execute_stamps.data_disks = 1;
        fbe_raid_geometry_get_width(raid_geometry_p, &width);
        execute_stamps.array_width = width;
        execute_stamps.eboard_p = NULL;
        execute_stamps.eregions_p = NULL;
        execute_stamps.option = FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS;
        status = fbe_xor_lib_execute_stamps(&execute_stamps); 
    }

    if (execute_stamps.status == FBE_XOR_STATUS_NO_ERROR)
    {
        return FBE_STATUS_OK;
    }
    /* We could have read a previously invalidated block.
     */
    if (fbe_raid_group_io_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: check lba stamp op: %d lba: 0x%llx blks: 0x%llx flags: 0x%x xor status: 0x%x\n",
                              block_operation_p->block_opcode,
			      (unsigned long long)block_operation_p->lba,
			      (unsigned long long)block_operation_p->block_count, 
                              block_operation_p->block_flags, execute_stamps.status);
    }
    
    /* There was an lba stamp failure detected.  Retry using the raid library.
     */
    return FBE_STATUS_IO_FAILED_RETRYABLE;
}
/***********************************************
 * end fbe_raid_group_check_lba_stamp_checksum()
 ***********************************************/
/*!**************************************************************
 * fbe_raid_group_direct_io_traffic_trace()
 ****************************************************************
 * @brief
 *  Print out the RG trace for this siots.
 *
 * @param siots_p - Ptr to this I/O.
 * @param b_start - TRUE if this is the start trace, FALSE otherwise.
 *
 * @return None.
 *
 * @author
 *  5/24/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_direct_io_traffic_trace(fbe_raid_group_t *raid_group_p,
                                            fbe_payload_block_operation_t *block_operation_p,
                                            fbe_u32_t position,
                                            fbe_raid_group_number_t rg_number,
                                            fbe_sg_element_t *sg_p,
                                            fbe_bool_t b_start,
                                            fbe_u16_t priority)
{
    
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_u64_t crc_info;
    fbe_u64_t rg_info = 0;
    if (rg_number == FBE_RAID_GROUP_INVALID)
    {
        rg_number = raid_group_p->base_config.base_object.object_id;
    }
    fbe_traffic_trace_get_data_crc(sg_p, block_operation_p->block_count, 0, &crc_info);

    fbe_raid_group_rebuild_get_all_rebuild_position_mask(raid_group_p, &degraded_bitmask);
    
    /* Arg2 has: 
     *   (state << 48) | (algorithm << 40) | (read_mask << 24) | (write_mask << 8) | opcode 
     */
    rg_info |= (((fbe_u64_t)1 << position) << 24); /* Just read mask*/
    rg_info |= (((fbe_u64_t)0xFF) << 40); /* Algorithm */
    rg_info |= ( ((fbe_u64_t)degraded_bitmask) << 48);

    fbe_traffic_trace_block_opcode(FBE_CLASS_ID_RAID_GROUP,
                                   block_operation_p->lba,
                                   block_operation_p->block_count,
                                   block_operation_p->block_opcode,
                                   rg_number,
                                   crc_info, /*extra information*/
                                   rg_info,
                                   b_start, /*RBA trace start*/
                                   priority);
}
/******************************************
 * end fbe_raid_group_direct_io_traffic_trace()
 ******************************************/
static fbe_status_t fbe_raid_group_direct_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_operation_t *master_block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_status_t fbe_xor_check_lba_stamp_prefetch(fbe_sg_element_t *sg_ptr, fbe_block_count_t blocks);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /*! @note Get the block status and qualifier for the completed block
     *        operation.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    /* Get the current (block not stripe) operation.
     */
    if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
    }

    /* Trace end if enabled.
     */
    if (fbe_raid_group_io_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: pkt:%p lba:%016llx blks:%08llx op: %d end sts/qual:%d/%d %d\n",
                              packet_p,
			      (unsigned long long)block_operation_p->lba,
			      (unsigned long long)block_operation_p->block_count,
                              block_operation_p->block_opcode, block_status, block_qualifier,
                              (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY) ? 1 : 0);
    }

    if((packet_p->packet_attr & FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK) && fbe_raid_geometry_is_c4_mirror(raid_geometry_p))
    {
        fbe_transport_clear_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK);
    }

    /* Check the packet and block status.
     */
    status = fbe_transport_get_status_code(packet_p);
    if ((status == FBE_STATUS_OK) && (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) && (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE))
    {
        fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
        fbe_sg_element_t *sg_p = NULL;

        if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU) ||
            fbe_traffic_trace_is_enabled(KTRC_TFBE_RG))
        {
            fbe_payload_block_operation_t *prev_block_op;
            fbe_raid_group_number_t rg_number;
            fbe_raid_position_t position;
            fbe_raid_small_read_geometry_t geo;

            /* Function declarations */
            fbe_status_t fbe_parity_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                            fbe_lba_t lba,
                                                            fbe_raid_small_read_geometry_t * geo);
            fbe_status_t fbe_striper_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_lba_t lba,
                                                             fbe_raid_small_read_geometry_t * geo);
            fbe_status_t fbe_mirror_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                            fbe_lba_t lba,
                                                            fbe_raid_small_read_geometry_t * geo);

            /* Get the previous (master) block operation (if any) */
            prev_block_op = fbe_payload_ex_search_prev_block_operation(payload_p);
            if(prev_block_op == NULL)
            {
                /* Not found. Use current */
                prev_block_op = block_operation_p;
            }

            /* Get Raid Group number */
            fbe_database_get_rg_number(raid_group_p->base_config.base_object.object_id, &rg_number);

            /* Get Fru position */
            if (fbe_raid_geometry_is_parity_type(raid_geometry_p))
            {
                status = fbe_parity_get_small_read_geometry(raid_geometry_p, prev_block_op->lba, &geo);
            }
            else if (fbe_raid_geometry_is_mirror_type(raid_geometry_p))
            {
                status = fbe_mirror_get_small_read_geometry(raid_geometry_p, prev_block_op->lba, &geo);
            }
            else if (fbe_raid_geometry_is_striper_type(raid_geometry_p))
            {
                status = fbe_striper_get_small_read_geometry(raid_geometry_p, prev_block_op->lba, &geo);
            }
            else
            {
                geo.position = 0;
                status = FBE_STATUS_OK;
            }
            position = geo.position;

            if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU)){

                fbe_traffic_trace_block_operation(FBE_CLASS_ID_RAID_GROUP,
                                                  block_operation_p,
                                                  rg_number,
                                                  position,
                                                  FBE_FALSE, /*not start RBA trace end*/
                                                  fbe_traffic_trace_get_priority(packet_p));
            }
            if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG)){

                fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
                fbe_raid_group_direct_io_traffic_trace(raid_group_p,
                                                       block_operation_p, position, rg_number, sg_p, FBE_FALSE,
                                                       fbe_traffic_trace_get_priority(packet_p));
            } 
        }
        

        /* Check the lba stamp, but only for reads.
         */
        if ((status == FBE_STATUS_OK) &&
            (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ))
        {
            fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
            fbe_xor_check_lba_stamp_prefetch(sg_p, block_operation_p->block_count);
            status = fbe_raid_group_check_lba_stamp_checksum(raid_group_p, block_operation_p, sg_p);
        }
        if (status != FBE_STATUS_OK)
        {
            /* Retry using raid library.
             */
            status = fbe_raid_group_restart_failed_io(raid_group_p, packet_p, FBE_RAID_GROUP_IO_FAILURE_TYPE_CHECKSUM_ERROR);
            if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
            {
                return status;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: restart failed_io failed with status 0x%x\n", status);
               block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST;
               block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR;
            }
        }
        /*! @note Do not release the block operation until we have determined
         *        that the operation was completely successful.  The retry
         *        handler will release the block operation.  If we are here
         *        the request was successful and this we can now release it.
         */
        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

        /* I/O and data check were successful.
         */
        if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
        {
            /* First copy the block status to the master.
             */
            master_block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
            master_block_operation_p->status = block_status;
            master_block_operation_p->status_qualifier = block_qualifier;

            /* Now release the stripe lock and complete the request.
             */
            stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_direct_io_unlock_completion, raid_group_p);
            fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation_p);
            fbe_stripe_lock_operation_entry(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            /* First copy the block status to the master.
             */
            master_block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
            master_block_operation_p->status = block_status;
            master_block_operation_p->status_qualifier = block_qualifier;
            if (payload_p->current_operation->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "payload opcode is 0x%x\n", payload_p->current_operation->payload_opcode);
            }

            /* Nothing to free, just return packet.
             */
            return FBE_STATUS_OK;
        }
    }
    else
    {
        fbe_raid_group_io_failure_type_t    failure_type;

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: pkt:%p lba:%016llx blks:%08llx op: %d end st: 0x%x sts/qual:%d/%d\n",
                              packet_p,
			      (unsigned long long)block_operation_p->lba,
			      (unsigned long long)block_operation_p->block_count,
                              block_operation_p->block_opcode, status, block_status, block_qualifier);

        /* Else start the iots.  We have taken the lock if required.
         */
        failure_type = (status != FBE_STATUS_OK) ? FBE_RAID_GROUP_IO_FAILURE_TYPE_TRANSPORT : FBE_RAID_GROUP_IO_FAILURE_TYPE_BLOCK_OPERATION;
        status = fbe_raid_group_restart_failed_io(raid_group_p, packet_p, failure_type);
        return status;
    }

    return FBE_STATUS_OK;
}
/*******************************************
 * end fbe_raid_group_direct_io_completion()
 *******************************************/

/*!**************************************************************
 * fbe_raid_group_optimize_completion()
 ****************************************************************
 * @brief
 *  When we are done with the operation finish optimize to indicate
 *  the I/O is finished.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_optimize_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_geometry_t *raid_geometry_p = &raid_group_p->geo;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_mirror_optimize_direct_io_finish(raid_geometry_p, block_operation_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_optimize_completion()
 **************************************/
static fbe_status_t fbe_raid_group_start_direct_io(fbe_raid_group_t *raid_group_p, 
                                                   fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_status_t status;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = &raid_group_p->geo;
    fbe_raid_small_read_geometry_t geo;
    fbe_raid_position_t position, new_position;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_t *master_block_operation_p = NULL;
    fbe_lun_performance_statistics_t  *performace_stats_p = NULL;
    fbe_bool_t b_optimize = FBE_FALSE;
    fbe_raid_position_bitmask_t rekey_bitmask;
    fbe_status_t fbe_parity_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_lba_t lba,
                                                    fbe_raid_small_read_geometry_t * geo);
    fbe_status_t fbe_striper_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_lba_t lba,
                                                     fbe_raid_small_read_geometry_t * geo);
    fbe_status_t fbe_mirror_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_lba_t lba,
                                                    fbe_raid_small_read_geometry_t * geo);
    
    if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        /* In this case we have stripe locks.
         */
        master_block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
    }
    else
    {
        master_block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    }
    fbe_payload_block_get_lba(master_block_operation_p, &lba);
    fbe_payload_block_get_block_count(master_block_operation_p, &blocks);
    fbe_payload_block_get_opcode(master_block_operation_p, &opcode);

    /* Determine which edge to goto.
     */
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        status = fbe_parity_get_small_read_geometry(raid_geometry_p, lba, &geo);
    }
    else if (fbe_raid_geometry_is_mirror_type(raid_geometry_p))
    {
        status = fbe_mirror_get_small_read_geometry(raid_geometry_p, lba, &geo);
        /* Mirror read optimization start.
         */
        if (status == FBE_STATUS_OK)
        {
            fbe_mirror_prefered_position_t prefered_mirror_pos_p;
            fbe_raid_geometry_get_mirror_prefered_position(raid_geometry_p, &prefered_mirror_pos_p);
            if (prefered_mirror_pos_p == FBE_MIRROR_PREFERED_POSITION_INVALID)
            {
                status = fbe_mirror_optimize_determine_position(raid_geometry_p, &new_position, opcode, lba, blocks);
                if (status == FBE_STATUS_OK)
                { 
                    if (raid_geometry_p->mirror_optimization_p->num_reads_outstanding[new_position] == 0)
                    {
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                              "raid_group: pos: %d num_reads is 0\n", new_position);
                    }
                    b_optimize = FBE_TRUE;
                    geo.position = new_position;
                }
            }
        }
    }
    else if (fbe_raid_geometry_is_striper_type(raid_geometry_p))
    {
        status = fbe_striper_get_small_read_geometry(raid_geometry_p, lba, &geo);

        /* Anything that does not get a stripe lock can just bail out with error now.
         */
        if ((raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) &&
            (status != FBE_STATUS_OK)) {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: striper get geo error 0x%x\n", status);
            fbe_payload_block_set_status(master_block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
        /* There was an invalid parameter in the, return error now.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: get lun geo error 0x%x\n", status);

        fbe_payload_block_set_status(master_block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        /* Now release the stripe lock and complete the request.
         */
        stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_direct_io_unlock_completion, raid_group_p);
        fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation_p);
        fbe_stripe_lock_operation_entry(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_direct_io_completion,
                                          raid_group_p);
    if (b_optimize)
    {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_optimize_completion, raid_group_p);
    }
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    fbe_payload_block_build_operation(block_operation_p, 
                                      opcode,
                                      geo.logical_parity_start + geo.start_offset_rel_parity_stripe,
                                      blocks,
                                      FBE_BE_BYTES_PER_BLOCK, 1, /* optimal block size */ 
                                      NULL);
    if ((master_block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM) == 0)
    {
        fbe_payload_block_clear_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }
    if (fbe_raid_group_io_rekeying(raid_group_p)){
        fbe_raid_group_rekey_get_bitmap(raid_group_p, packet_p, &rekey_bitmask);
        if ((rekey_bitmask != 0) &&
            (rekey_bitmask & (1 << geo.position))) {
            fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
        }
    }
    if (fbe_raid_geometry_is_c4_mirror(raid_geometry_p)){
        /* Anything coming out of the c4mirror does not need to get stripe locks at PVD.
         * This is because we have previously zeroed the range in ICA and the I/Os can only come from 
         * one SP.
         */
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK);
    }
    position = geo.position;
    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);

    fbe_payload_ex_get_lun_performace_stats_ptr(payload_p, (void **)&performace_stats_p);

    if ((performace_stats_p == NULL) && 
        !fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU) && 
        !fbe_traffic_trace_is_enabled(KTRC_TFBE_RG))
    {
        fbe_block_transport_send_functional_packet(block_edge_p, packet_p);
    }
    else 
    {
        /* increment the disk statistics prior to sending the packet.
         */
        fbe_raid_group_start_direct_io_update_performance_stats(packet_p, position, blocks, opcode);

        if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU) ||
            fbe_traffic_trace_is_enabled(KTRC_TFBE_RG))
        {
            fbe_raid_group_number_t rg_number;
            fbe_database_get_rg_number(raid_group_p->base_config.base_object.object_id, &rg_number);

            if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG)){
                fbe_sg_element_t *sg_p = NULL;
                fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
                fbe_raid_group_direct_io_traffic_trace(raid_group_p,
                                                       block_operation_p, position, rg_number, sg_p, FBE_TRUE, /* Start */
                                                       fbe_traffic_trace_get_priority(packet_p));
            }
            if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU)){
                fbe_traffic_trace_block_operation(FBE_CLASS_ID_RAID_GROUP,
                                                  block_operation_p, rg_number, position, FBE_TRUE, /*RBA trace start*/
                                                  fbe_traffic_trace_get_priority(packet_p));
            }
        }

        fbe_block_transport_send_functional_packet(block_edge_p, packet_p);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_raid_group_direct_io_sl_completion()
 ****************************************************************
 * @brief
 *  Handle allocation of the lock.
 *
 * @param packet_p - Packet that is completing.
 * @param context - the raid group. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  2/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_direct_io_sl_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_status_t packet_status;

    packet_status = fbe_transport_get_status_code(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);

    /* Validate that a stripe lock operaiton was requested
     */
    if (stripe_lock_operation_p != NULL)
    {
        /* Get the stripe lock operation
         */
        fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

        if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status == FBE_STATUS_OK) )
        {
            fbe_raid_group_start_direct_io(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            /* Restart the I/O using the iots.
             */
            status = fbe_raid_group_restart_failed_io(raid_group_p, packet_p, FBE_RAID_GROUP_IO_FAILURE_TYPE_STRIPE_LOCK);
            return status;
        }
    }
    else 
    {
        fbe_payload_block_operation_t *block_operation_p = NULL;
        fbe_payload_ex_release_stripe_lock_operation(payload_p, stripe_lock_operation_p);

        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        /* Something went wrong
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "SL direct no stripe lock opt: packet_status: 0x%x", 
                              packet_status);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }

}
/**************************************
 * end fbe_raid_group_direct_io_sl_completion
 **************************************/

static fbe_status_t fbe_raid_group_fast_write_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    fbe_payload_ex_get_iots(payload_p, &iots_p);
    if (fbe_raid_iots_is_any_flag_set(iots_p, (FBE_RAID_IOTS_FLAG_REMAP_NEEDED | 
                                               FBE_RAID_IOTS_FLAG_WAS_QUIESCED   )))
    {
        fbe_raid_group_iots_completion(packet_p,context);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        fbe_raid_iots_set_packet_status(iots_p);
        fbe_raid_iots_fast_destroy(iots_p);
        stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);

        /* Trace end if enabled.
         */
        if (fbe_raid_group_io_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING))
        {
            block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
            fbe_payload_block_get_status(block_operation_p, &block_status);
            fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: pkt:%08llx lba:%016llx blks:%08llx op: %d end sts/qual:%d/%d %d\n",
                                  (unsigned long long)packet_p, block_operation_p->lba, block_operation_p->block_count,
                                  block_operation_p->block_opcode, block_status, block_qualifier,
                                  (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY) ? 1 : 0);
        }

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_direct_io_unlock_completion, raid_group_p);
        fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation_p);
        fbe_stripe_lock_operation_entry(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        if (payload_p->current_operation->payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "payload opcode is 0x%x\n", payload_p->current_operation->payload_opcode);
        }

        fbe_raid_group_iots_finished(raid_group_p, iots_p);
        /* This is the metadata case. Nothing to free, just return packet.
         */
        return FBE_STATUS_OK;
    }
}

/*!**************************************************************
 * fbe_raid_group_fast_write_sl_completion()
 ****************************************************************
 * @brief
 *  Handle allocation of the lock.
 *
 * @param packet_p - Packet that is completing.
 * @param context - the raid group. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  2/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_fast_write_sl_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
    fbe_status_t packet_status;
    void fbe_parity_start_fast_write(fbe_packet_t *packet_p);
    packet_status = fbe_transport_get_status_code(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);

    /* Validate that a stripe lock operaiton was requested
     */
    if (stripe_lock_operation_p != NULL)
    {
        fbe_raid_iots_t *iots_p = NULL;
        fbe_payload_block_operation_t *block_operation_p = NULL;
        fbe_chunk_size_t chunk_size;
        fbe_lba_t lba;
        fbe_block_count_t blocks;
        if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
        {
            /* In this case we have stripe locks.
             */
            block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
        }
        else
        {
            block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        }

        fbe_payload_block_get_lba(block_operation_p, &lba);
        fbe_payload_block_get_block_count(block_operation_p, &blocks);
        fbe_payload_ex_get_iots(payload_p, &iots_p);
        fbe_raid_iots_set_geometry(iots_p, &raid_group_p->geo);
        fbe_raid_iots_set_packet(iots_p, packet_p);
        fbe_raid_iots_set_block_operation(iots_p, block_operation_p);
        fbe_raid_iots_init(iots_p, packet_p, block_operation_p, &raid_group_p->geo, lba, blocks);
        //fbe_raid_iots_fast_init(iots_p);
        chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
        fbe_raid_iots_set_chunk_size(iots_p, chunk_size);

        /* Get the stripe lock operation
         */
        fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

        if ( (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK) && (packet_status == FBE_STATUS_OK) )
        {
            if (!fbe_raid_group_io_is_degraded(raid_group_p))
            {
                /* Let's kick off a fast path write.
                 */
                fbe_raid_iots_set_callback(iots_p, (fbe_raid_iots_completion_function_t)fbe_raid_group_fast_write_completion, raid_group_p);
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED | FBE_RAID_IOTS_FLAG_FAST_WRITE);
                fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
                fbe_raid_iots_start(iots_p);
            }
            else
            {
                /* We are now degraded, rejoin the normal path.
                 */
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "SL fast write now degraded bl_status: 0x%x pkt_status: 0x%x op: %d iots_p: 0x%x\n", 
                                      stripe_lock_status, packet_status, stripe_lock_operation_p->opcode, (fbe_u32_t)iots_p);
                fbe_raid_iots_init_flags(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);
                fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
            }
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "SL fast write lock failed bl_status: 0x%x pkt_status: 0x%x op: %d iots_p: 0x%x\n", 
                                  stripe_lock_status, packet_status, stripe_lock_operation_p->opcode, (fbe_u32_t)iots_p);
            /* Just rejoin our normal stripe lock completion code to handle the error.
             */
            fbe_raid_iots_init_flags(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);
            fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);
            return fbe_raid_group_stripe_lock_completion(packet_p, raid_group_p);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else 
    {
        fbe_raid_iots_t *iots_p = NULL;
        fbe_payload_block_operation_t *block_operation_p = NULL;
        fbe_payload_ex_get_iots(payload_p, &iots_p);
        fbe_payload_ex_release_stripe_lock_operation(payload_p, stripe_lock_operation_p);

        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        /* Something went wrong
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "SL direct no stripe lock opt: packet_status: 0x%x  iots_p: 0x%x\n", 
                              packet_status, (fbe_u32_t)iots_p);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        /* Return FBE_STATUS_OK so that the packet gets completed up to the next level.
         */
        return FBE_STATUS_OK;
    }

}
/**************************************
 * end fbe_raid_group_fast_write_sl_completion
 **************************************/

/*!**************************************************************
 * fbe_raid_group_start_iots_with_lock()
 ****************************************************************
 * @brief
 *  Start an iots which now has a lock.
 *  Handles kicking off updating the bitmap if needed.
 *
 * @param raid_group_p - the raid group object.
 * @param iots_p - current I/O.
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/24/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_start_iots_with_lock(fbe_raid_group_t *raid_group_p, 
                                                 fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rebuild_logging_bitmask);

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, rebuild_logging_bitmask);

    if ((rebuild_logging_bitmask != 0) &&
        ((fbe_payload_block_operation_opcode_is_media_modify(opcode) == FBE_TRUE) ||
         ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) &&
         fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED))))
    {
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                             "mark nr pkt: 0x%x lba: 0x%llx bl: 0x%llx opcode: 0x%x status: 0x%x rl: 0x%x\n", 
                             (fbe_u32_t) packet_p,
			     (unsigned long long)iots_p->packet_lba,
			     (unsigned long long)iots_p->packet_blocks, 
                             opcode, iots_p->status, rebuild_logging_bitmask);

        /* We are rebuild logging, and this is a data modification opcode.
         * Update the metadata before proceeding.
         */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

        fbe_raid_group_mark_nr_for_iots(raid_group_p, iots_p, fbe_raid_group_mark_nr_for_iots_complete);
    }
    else
    {
        /* Lock is granted, proceed to start the I/O.
         */
        fbe_raid_group_start_iots(raid_group_p, iots_p);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_start_iots_with_lock()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_mark_nr_for_iots()
 ****************************************************************
 * @brief
 *  Get an iots stripe lock for this request.
 * 
 * @param raid_group_p - the raid group object.
 * @param iots_p - current I/O.
 * @completion_fn - Completion function.
 *
 * @return fbe_status_t.
 *
 * @author
 *  1/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_mark_nr_for_iots(fbe_raid_group_t   *raid_group_p,
                                             fbe_raid_iots_t    *iots_p,
                                             fbe_packet_completion_function_t completion_fn)
{
    fbe_status_t                        status;
    fbe_packet_t                        *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_lba_t                           lba;
    fbe_block_count_t                   blocks;
    fbe_lba_t                           packet_lba;
    fbe_block_count_t                   packet_blocks;
    fbe_chunk_count_t                   chunk_size;
    fbe_raid_position_bitmask_t         rebuild_logging_bitmask;
    fbe_lba_t                           end_lba;                        
    fbe_chunk_index_t                   start_chunk;                    
    fbe_chunk_index_t                   end_chunk;                      
    fbe_chunk_count_t                   chunk_count;
    fbe_raid_group_paged_metadata_t     chunk_data; 
    fbe_raid_geometry_t                 *raid_geometry_p;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &rebuild_logging_bitmask);
    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);

    /* Get the range of chunks affected by this iots.
     */
    status = fbe_raid_iots_get_chunk_lba_blocks(iots_p, packet_lba, packet_blocks, &lba, &blocks, chunk_size);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error getting chunk lba blocks status: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    //  Get the ending LBA to be marked, by calculating it from the start address and the block count 
    end_lba = lba + blocks - 1;
    
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk);
    
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                         "mark nr for pkt: 0x%x lba %llx to %llx chunk %llx to %llx, rl 0x%x\n", 
                         (fbe_u32_t)packet_p, (unsigned long long)lba, (unsigned long long)end_lba, (unsigned long long)start_chunk, (unsigned long long)end_chunk, rebuild_logging_bitmask);

    //  Set up the bits of the metadata that need to be written, which are the NR bits 
    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_data.needs_rebuild_bits = rebuild_logging_bitmask;

    // Set the completion function.
    fbe_transport_set_completion_function(packet_p, completion_fn, (fbe_packet_completion_context_t) raid_group_p);

    /* Clear the bit to cause us to refresh paged.
     */
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED);

    // If the IO was in metadata region then mark NR in the metadata of metadata.
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, packet_lba) ||
        !fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(raid_group_p, packet_p, start_chunk, chunk_count, &chunk_data, FALSE);
    }
    else
    {
        //  The chunks are in the user data area.  Use the paged metadata service to update them. 
        fbe_raid_group_bitmap_update_paged_metadata(raid_group_p, packet_p, start_chunk, chunk_count, &chunk_data, FALSE);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_mark_nr_for_iots
 **************************************/
/*!**************************************************************
 * fbe_raid_group_mark_nr_for_iots_complete()
 ****************************************************************
 * @brief
 *  We finished updating metadata, now kick off the I/O.
 *
 * @param packet_p - Packet that is starting.
 * @param context - the raid group. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_mark_nr_for_iots_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;
    fbe_bool_t  packet_was_failed_b;
    
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                          "rg:rg_mark_nr_4_iots_compl done marking nr lba:0x%llx bl:0x%llx status:0x%x\n", 
                          iots_p->packet_lba, iots_p->packet_blocks, iots_p->status);

    status = fbe_transport_get_status_code(packet_p);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_mark_nr_4_iots_compl write of metadata failed,packet 0x%p,status 0x%x\n", packet_p, status);

        /*  Handle the metadata error and return here */
        fbe_raid_group_executor_handle_metadata_error(raid_group_p, packet_p,
                                                      fbe_raid_group_retry_mark_nr_for_iots, &packet_was_failed_b,
                                                      status);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Terminate the I/O since the code below assumes we are terminated. 
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rebuild_logging_bitmask);

    if (iots_p->rebuild_logging_bitmask != rebuild_logging_bitmask)
    {
        /* If the rebuild log bitmask changed, and a bit was added, then we likely were quiesced at MDS. When we
         * finished the operation we saw that another bit is set. 
         */ 
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg:rg_mark_nr_4_iots_compl iots rebuild logging bitmask changed. iots bitmask:0x%x rl_bitmask:0x%x\n", 
                              iots_p->rebuild_logging_bitmask, rebuild_logging_bitmask);
        fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* Metadata is updated.  Start the I/O.
     */
    fbe_raid_group_start_iots(raid_group_p, iots_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_mark_nr_for_iots_complete()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_start_iots()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 * 
 * @param raid_group_p - the raid group object.
 * @param packet_p - packet to start the iots for.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_start_iots(fbe_raid_group_t *raid_group_p,
                                       fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_payload_block_operation_opcode_t   block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_status_t                           status;
    fbe_chunk_size_t                       chunk_size;
    fbe_time_t                             expiration_time;
    fbe_lba_t                              lba;   
    fbe_block_count_t                      blocks;    
    fbe_chunk_count_t                      start_bitmap_chunk_index;
    fbe_chunk_count_t                      num_chunks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t                              verify_disk_lba;   
    fbe_lba_t                              verify_lba;
    fbe_u16_t                              num_data_disks;

    /* If we do not have an expiration time yet, then set a default time. 
     * If we cannot complete the I/O in this default time then we will take the 
     * object out of the ready state. 
     */
    fbe_transport_get_expiration_time(packet_p, &expiration_time);
    if (expiration_time == 0)
    {
        fbe_transport_set_and_start_expiration_timer(packet_p, fbe_raid_group_get_default_user_timeout_ms(raid_group_p));
    }

    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);

    /* Limit the number of blocks if it exceeds the number of 
     * chunks that can fit into an iots. 
     */
    status = fbe_raid_iots_determine_next_blocks(iots_p, chunk_size);

    if (status != FBE_STATUS_OK)
    {
        /* Set status to unexpected and fail the I/O.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s determing next blocks error status %d", __FUNCTION__, status);
        fbe_raid_group_iots_completion(packet_p, raid_group_p);
        return status;
    }

    fbe_raid_iots_get_current_opcode(iots_p, &block_opcode);    


    //  Remove the packet from the terminator queue.  We have to do this before any paged metadata access.
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    /* Check if it is a verify operation. If it is, read in the metadata to see
       whether verify is marked for this particular chunk and start the verify.
    */
    /*!@todo Once the rebuild start using the metadata all this code can be moved 
       to a funciton called get chunk info and this function will be cleaner
    */
    if ( (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
         (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) ||
         (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY) ||
         (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) ||
         (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) )
    {
        /* Verify lba that we got is per disk lba.*/
        fbe_raid_iots_get_lba(iots_p, &verify_disk_lba);
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);
        verify_lba = verify_disk_lba * num_data_disks;

        /* Check if this lba is in the metadata region. If so, just start the request
         * as the Non Paged locked is acquired before even this request got here
         */
        if(fbe_raid_geometry_is_metadata_io(raid_geometry_p, verify_lba) ||
           !fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
        {
            /* Need to get the degraded (Needs Rebuild) information using the
             * non-paged metadata
             */
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_get_chunk_info_for_start_iots_completion,
                                              (fbe_packet_completion_context_t) raid_group_p);
            fbe_raid_group_get_chunk_info(raid_group_p, packet_p, iots_p);
        }
        else
        {
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_verify_read_paged_metadata);
        }

        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

    }
    else if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD)
    {
        /* We have limited the iots to the maximum supported chunks.
         * Get the chunk info (i.e. if the chunks needs to be rebuilt or not).
         */
        status = fbe_raid_group_rebuild_get_chunk_info(raid_group_p, packet_p);
        return status; 
    }

    else if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD)
    {        
        fbe_raid_group_mark_nr_control_operation(raid_group_p, packet_p, fbe_raid_group_paged_metadata_iots_completion);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    else if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) &&
             (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED)))
    {
        /* This is first pass of write_log flush operation where we will only perform header read
         * from the write_log's private space. Chunk-info does not apply for this region, but since the region
         * can be "rebuilding" (which means it's unconsumed and cannot be read from)
         * we will generate chunk info based on the raid_group rebuild position mask.
         * In the second pass (i.e. when iots flag is set), we will read in chunk-info for the 
         * live stripe. 
         */
        /* Pre-initialize the chunk info in the IOTS to 0, ie. the chunks are not marked for rebuild or verify 
         */
        fbe_set_memory(&iots_p->chunk_info[0], 0, FBE_RAID_IOTS_MAX_CHUNKS * sizeof(fbe_raid_group_paged_metadata_t));

        /* Add the raid_group rebuild position mask
        */
        fbe_raid_group_rebuild_get_all_rebuild_position_mask(raid_group_p,
                                                             &(iots_p->chunk_info[0].needs_rebuild_bits));

        fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* Small degraded writes optimize so that we will write the paged once
     * and the write will return the paged to us so we can avoid a paged read.
     */
    else if ( ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) || 
               (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED)) &&
              fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED))
    {
        fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }
    else if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED)
    {
        /* Get the chunk info (i.e. if the chunks needs to be rekeyed or not).
         */
        status = fbe_raid_group_rekey_read_paged(packet_p, raid_group_p);
        return status; 
    }
    else if (!fbe_raid_geometry_is_raid10(raid_geometry_p) &&
             (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE))
    {
        fbe_raid_group_rekey_write_update_paged(raid_group_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA) &&
             fbe_payload_block_is_flag_set(iots_p->block_operation_p,
                                           FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY))
    {
        /* We want to perform a verify using the new key.  Setup the chunk info for this.
         */
        fbe_raid_position_bitmask_t mask;
        fbe_set_memory(&iots_p->chunk_info[0], 0, 
                       FBE_RAID_IOTS_MAX_CHUNKS * sizeof(fbe_raid_group_paged_metadata_t));
        fbe_raid_group_get_degraded_mask(raid_group_p, &mask);
        iots_p->chunk_info[0].needs_rebuild_bits = mask;
        iots_p->chunk_info[0].rekey = 1;
        fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
        return status; 
    }
    /* Read chunk info only if raid group is degraded, or if
     * we are a mirror and a verify is in progress. 
     */
    else if (fbe_raid_group_io_is_degraded(raid_group_p) ||
             (fbe_raid_group_io_rekeying(raid_group_p) &&
              fbe_raid_group_should_read_paged_for_rekey(raid_group_p, packet_p)) )
    {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_get_chunk_info_for_start_iots_completion,
                                              (fbe_packet_completion_context_t) raid_group_p);
        fbe_raid_group_get_chunk_info(raid_group_p, packet_p, iots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED);
        /*  Get the LBA and blocks from the iots 
        */
        fbe_raid_iots_get_current_op_lba(iots_p, &lba);
        fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

        /*  Get the range of chunks affected by this iots.
        */
        fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &start_bitmap_chunk_index, &num_chunks);

        if (num_chunks > FBE_RAID_IOTS_MAX_CHUNKS)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "number of chunks %d is beyond the max for an iots %d\n",
                                  num_chunks, FBE_RAID_IOTS_MAX_CHUNKS);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Initialize the chunk info in the IOTS to 0, ie. the chunks are not marked for rebuild or verify 
        */
        fbe_set_memory(&iots_p->chunk_info[0], 0, num_chunks * sizeof(fbe_raid_group_paged_metadata_t));

        /* This is an optimization to avoid reading the paged during incomplete write verify. 
         * If the checkpoint is not at the end marker, just assume iw verify is needed. 
         */
        if (fbe_raid_group_is_iw_verify_needed(raid_group_p)) {
            fbe_u32_t chunk_index;
            for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {
                iots_p->chunk_info[0].verify_bits |= FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE;
            }
        }
        fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

}
/******************************************
 * end fbe_raid_group_start_iots()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_iots_get_first_operation()
 ****************************************************************
 * @brief
 *  Get the first opcode to issue for this iots.
 *  In some cases we issue a different operation first.
 *  For example:
 *  - Verify On-Demand does a verify and then the I/O operation.
 *  - Rebuild On-Demand does a rebuild first and then the I/O.
 *  - Rebuild does a check zeroed and then the I/O.
 * 
 * @param raid_group_p - Pointer to raid group for I/O
 * @param iots_p - Current I/O.
 *
 * @return fbe_status_t.
 *
 * @author
 *  9/21/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_iots_get_first_operation(fbe_raid_group_t *raid_group_p,
                                                            fbe_raid_iots_t *iots_p)
{
    fbe_payload_block_operation_opcode_t original_opcode;
    fbe_payload_block_operation_opcode_t current_opcode;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_raid_geometry_t *raid_geometry_p;
    fbe_raid_iots_get_opcode(iots_p, &original_opcode);
    fbe_raid_iots_get_current_opcode(iots_p, &current_opcode);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
    {
        /* The check zeroed is complete and we need to do the rebuild/verify now.
         */
        fbe_raid_iots_get_blocks(iots_p, &blocks);
        fbe_raid_iots_get_lba(iots_p, &lba);
        fbe_raid_iots_set_current_opcode(iots_p, original_opcode);

        /* If the previous I/O was quiesced we need to decrement the `quiesced' 
         * count.
         */
        fbe_raid_group_handle_was_quiesced_for_next(raid_group_p, iots_p);

        fbe_raid_iots_reinit(iots_p, lba, blocks);
    }
    else if ((current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) ||
             (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY))
    {
        /* The c4mirror is previously zeroed when ICA, do the rebuild/verify directly.
         */
        if (fbe_raid_geometry_is_c4_mirror(raid_geometry_p))
        {
            fbe_raid_iots_set_current_opcode(iots_p, current_opcode);
        }
        else
        {
            fbe_raid_iots_set_current_opcode(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED);
        }
    }
    else if ((current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) &&
             (!(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED))))
    {
        /* First perform header read operation to determine live-stripe for the flush */
        fbe_raid_iots_set_current_opcode(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_iots_get_first_operation()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_iots_start_next_operation()
 ****************************************************************
 * @brief
 *  Start the next operation for this iots.
 *  This is for cases where we do a different operation first,
 *  and then start the main operation here.
 *  Examples are:
 *  - Verify On-Demand does a verify and then the I/O operation.
 *  - Rebuild On-Demand does a rebuild first and then the I/O.
 *  - Rebuild does a check zeroed and then the I/O.
 *  - Write_log Flush does header-read first and then flush.
 * 
 * @param raid_group_p - Current raid group.
 * @param iots_p - Current I/O.
 *
 * @return fbe_status_t FBE_STATUS_OK - Caller continues processing.
 *  FBE_STATUS_MORE_PROCESSING_REQUIRED - Caller does not continue processing.
 *
 * @author
 *  9/21/2010 - Created. Rob Foley
 *  5/22/2012 - Vamsi V. Support for write_logging
 ****************************************************************/
static fbe_status_t fbe_raid_group_iots_start_next_operation(fbe_raid_group_t *raid_group_p,
                                                             fbe_raid_iots_t *iots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_opcode_t current_opcode;
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_current_opcode(iots_p, &current_opcode);

    if (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
    {
        fbe_lba_t lba;
        fbe_block_count_t blocks;
        fbe_payload_block_operation_qualifier_t block_qualifier;
        fbe_payload_block_operation_status_t block_status;
        fbe_payload_block_operation_opcode_t original_opcode;

        fbe_raid_iots_get_block_status(iots_p, &block_status);
        fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);
        fbe_raid_iots_get_opcode(iots_p, &original_opcode);

        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
            (block_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED))
        {
            /* The check zeroed is complete (we are not zeroed) and we need to do the rebuild/verify now.
             */
            fbe_raid_iots_get_blocks(iots_p, &blocks);
            fbe_raid_iots_get_lba(iots_p, &lba);
            fbe_raid_iots_get_opcode(iots_p, &original_opcode);
            fbe_raid_iots_set_current_opcode(iots_p, original_opcode);

            /* If the previous I/O was quiesced we need to decrement the `quiesced' 
             * count.
             */
            fbe_raid_group_handle_was_quiesced_for_next(raid_group_p, iots_p);

            fbe_raid_iots_reinit(iots_p, lba, blocks);
            fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);
            /* Kick off this I/O to the library.  Our callback will be executed on completion.
             */ 
            fbe_raid_iots_start(iots_p);
        }
        else if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
                 (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED))
        {
            /* The check zeroed is complete and we are zeroed.  Start the next piece of the iots by returning OK.
             */
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "check zero success op: %d block status: 0x%x block qualifier: 0x%x\n", 
                                  original_opcode, block_status, block_qualifier);
            return FBE_STATUS_OK;
        }
        else
        {
            /* Some error, just return OK so the caller handles the error.
             */
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "check zero failed op: %d block status: 0x%x block qualifier: 0x%x \n", 
                                  original_opcode, block_status, block_qualifier);
            return FBE_STATUS_OK;
        }
    }
    else if (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD)
    {
        fbe_lba_t lba;
        fbe_block_count_t blocks;
        fbe_payload_block_operation_qualifier_t block_qualifier;
        fbe_payload_block_operation_status_t block_status;
        fbe_payload_block_operation_opcode_t original_opcode;

        fbe_raid_iots_get_block_status(iots_p, &block_status);
        fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);
        fbe_raid_iots_get_opcode(iots_p, &original_opcode);

        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
            (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE))
        {
            if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED))
            {
                /* The write_log header read is complete and there is user data that needs to be flushed. 
                 * We should restart the iots with Flush operation.
                 */
                fbe_raid_iots_get_blocks(iots_p, &blocks);
                fbe_raid_iots_get_lba(iots_p, &lba);
                fbe_raid_iots_get_opcode(iots_p, &original_opcode);
                fbe_raid_iots_set_current_opcode(iots_p, original_opcode);

                /* If the previous I/O was quiesced we need to decrement the `quiesced' 
                 * count.
                */
                fbe_raid_group_handle_was_quiesced_for_next(raid_group_p, iots_p);

                fbe_raid_iots_reinit(iots_p, lba, blocks);
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED);

                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "write_log header read success. Flush is required. slot: 0x%x live-stripe lba: 0x%x blks: 0x%x ifl: 0x%x \n", 
                                      iots_p->journal_slot_id, (unsigned int)lba, (unsigned int)blocks, iots_p->flags);

                /* Kick off this I/O to first mark NR and then get the paged.
                 */ 
                fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
            }
            else
            {
                /* The write_log header read is complete and there is no data to flush. Complete the operation.
                 */
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "write_log header read success. Flush not required. slot: 0x%x ifl: 0x%x \n", 
                                      iots_p->journal_slot_id, iots_p->flags);
                return FBE_STATUS_OK;
            }

        }
        else
        {
            /* Some error, just return OK so the caller handles the error.
             */
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "write_log header read failed op: %d block status: 0x%x block qualifier: 0x%x \n", 
                                  original_opcode, block_status, block_qualifier);
            return FBE_STATUS_OK;
        }
    }
    else if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY) &&
             (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)                                 &&
             fbe_raid_group_is_verify_after_write_allowed(raid_group_p, iots_p)                              )
    {
        fbe_status_t                            status;
        fbe_payload_block_operation_status_t    block_status;
        fbe_payload_block_operation_qualifier_t block_qualifier;
        fbe_payload_block_operation_opcode_t    original_opcode;

        fbe_raid_iots_get_block_status(iots_p, &block_status);
        fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);
        fbe_raid_iots_get_opcode(iots_p, &original_opcode);

        if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
            /* The write completed now start the verify.
             */
            status = fbe_raid_group_generate_verify_after_write(raid_group_p, iots_p);
            if (status != FBE_STATUS_OK) {
                /* Some error, just return OK so the caller handles the error.
                 */
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "verify after write generate failed op: %d block status: 0x%x block qualifier: 0x%x \n", 
                                  original_opcode, block_status, block_qualifier);
                return FBE_STATUS_OK;
            }
            
            /* Set the callback */
            fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);

            /* Kick off this I/O to the library.  Our callback will be executed on completion.
             */ 
            fbe_raid_iots_start(iots_p);
        } else {
            /* Some error, just return OK so the caller handles the error.
             */
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "verify after write failed op: %d block status: 0x%x block qualifier: 0x%x \n", 
                                  original_opcode, block_status, block_qualifier);
            return FBE_STATUS_OK;
        }
    }
    else
    {
        /* Just return more processing requires, we are cleaning up this request.
         */
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s current opcode: 0x%x unexpected\n", __FUNCTION__, current_opcode);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE);
        fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_iots_start_next_operation()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_get_chunk_info_for_start_iots_completion()
 ****************************************************************
 * @brief
 *   This is the completion function for a read to the paged 
 *   metadata to get the chunk info, when called from fbe_raid_group_
 *   start_iots().  It will break up the IOTS if needed and then 
 *   actually start it. 
 * 
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  7/19/2010 - Created from fbe_raid_group_start_iots()
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_get_chunk_info_for_start_iots_completion(fbe_packet_t *packet_p,
                                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*               raid_group_p = NULL;     
    fbe_bool_t                      packet_was_failed_b;
    fbe_status_t                    status;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_raid_position_bitmask_t rg_rl_bitmask;
    fbe_raid_position_bitmask_t iots_rl_bitmask;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    raid_group_p = (fbe_raid_group_t*) context; 
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    //  Check the packet status to see if reading the chunk info succeeded 
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_chunk_info_4_start_iots_compl read mdata failed,packet 0x%p,stat 0x%x \n", 
                              packet_p, status);

        //  Handle the metadata error and return here 
        fbe_raid_group_executor_handle_metadata_error(raid_group_p, packet_p,
                                                      fbe_raid_group_retry_get_chunk_info_for_start_iots, &packet_was_failed_b,
                                                      status);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* At the start of encryption the striper needs to wait for the mirrors to get into encryption mode. 
     * Fail back requests until encryption mode has begun. 
     */
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    if (fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) &&
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS) &&
        (!fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) ||
         !fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED))) {
        fbe_raid_group_encryption_flags_t flags;
        /* Set status to unexpected and fail the I/O.
         */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_raid_group_encryption_get_state(raid_group_p, &flags);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption zero cannot proceed encryption flags: 0x%x\n",  flags);
        fbe_raid_group_iots_completion(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (fbe_raid_is_iots_chunk_info_clear_needed(iots_p, raid_group_p))
    {
        /* if we are doing ZERO opreation and if we are here then we are suppose to handle the degraded scenario
         * while no rebuild logging is on and some chunks are degraded and if we are going for zeroing then 
         * we will clear the needs rebuild bit from chunk info so that later when we come for rebuild we will not rebuild
         * these chunks as these will be already zeroed
         */
        fbe_zero_memory(&iots_p->chunk_info[0], FBE_RAID_IOTS_MAX_CHUNKS * sizeof(fbe_raid_group_paged_metadata_t));
    }
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rg_rl_bitmask); 
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &iots_rl_bitmask);

    if (rg_rl_bitmask != iots_rl_bitmask)
    {
         /* We saw the rebuild log bitmask change.  This might occur if we quiesced while we were outstanding with 
          * a metadata operation that saw a failure and remained quiesced when the rl bitmask changed. 
          * We need to restart the operation with a fresh rl bitmask. 
          */
         fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_gt_cnk_info_compl iots: 0x%x lba: 0x%llx bl: 0x%llx rg_rl:0x%x iots_rl:0x%x\n", 
                              (fbe_u32_t)iots_p, iots_p->lba, iots_p->blocks, rg_rl_bitmask, iots_rl_bitmask);

        /* Put the packet back on the termination queue since we had to take it off before reading the chunk info.
         */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
    }
    else
    {
        fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_get_chunk_info_for_start_iots_completion()
 ******************************************/

/*!***************************************************************
 *          fbe_raid_group_validate_iots_chunk_info()
 *****************************************************************
 * @brief
 *   It will break up the IOTS if needed and then 
 *   actually start it.
 * 
 * @param packet_p   
 * @param in_raid_group 
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/2/2010 - Created from fbe_raid_group_get_chunk_info_for_start_iots_completion()
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_validate_iots_chunk_info(fbe_raid_group_t *raid_group_p,
                                                            fbe_raid_iots_t *iots_p,
                                                            fbe_lba_t lba,
                                                            fbe_block_count_t blocks)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_is_request_in_user_area;
    fbe_raid_position_bitmask_t nonpaged_nr_bitmask = 0;
    fbe_chunk_count_t           start_bitmap_chunk_index;
    fbe_chunk_count_t           num_chunks;
    fbe_chunk_size_t            chunk_size;
    fbe_u32_t                   iots_chunk_index = 0;
    fbe_raid_position_bitmask_t rebuild_bits;

    /* Get the range of chunks affected by this iots.
     */
    fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &start_bitmap_chunk_index, &num_chunks);

    /*! @note Since method does not validate the needs rebuilt information 
     *        that was obtained from the metadata of metadata 
     *        (i.e. the non-paged).
     */
    fbe_raid_group_bitmap_determine_if_chunks_in_data_area(raid_group_p,
                                                           start_bitmap_chunk_index, 
                                                           num_chunks, 
                                                           &b_is_request_in_user_area);
    if (b_is_request_in_user_area == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    /* Determine the proper setting of the NR form the paged.
     */
    status = fbe_raid_group_bitmap_determine_nr_bits_from_nonpaged(raid_group_p, 
                                                                   start_bitmap_chunk_index, 
                                                                   num_chunks,
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

    /*! Loop over all the affected chunks. 
     */
    rebuild_bits = iots_p->chunk_info[0].needs_rebuild_bits;
    for (iots_chunk_index = 0; iots_chunk_index < num_chunks; iots_chunk_index++)
    {
        /* If the bits do not match the positions rebuild then something is wrong
         */
        /*! @note Currently we only validate the needs rebuild bitmask.  We can
         *        only check for `extra' bits set. Since the update paged
         *        metadata operation `set bits' XOR in the new bits
         */
        if ((rebuild_bits & ~nonpaged_nr_bitmask) != 0)
        {
            /* Generate an error trace.
             */
             fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "raid_group: validate iots chunk info lba: 0x%llx chunk index: %d iots NR: 0x%x expected NR: 0x%x \n",
                                   iots_p->lba, iots_chunk_index, rebuild_bits, nonpaged_nr_bitmask);
             return FBE_STATUS_GENERIC_FAILURE;
        }

        /* There is something wrong if any of the bits do not match the first one.
         */
        if (iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits != rebuild_bits)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: validate iots chunk info 0x%x at chunk: %d not equal rebuild_bits 0x%x\n",
                                  iots_p->chunk_info[iots_chunk_index].needs_rebuild_bits,
                                  iots_chunk_index, rebuild_bits);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
 }
/************************************************* 
 * end fbe_raid_group_validate_iots_chunk_info()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_limit_and_start_iots()
 ****************************************************************
 * @brief
 *   It will break up the IOTS if needed and then 
 *   actually start it.
 * 
 * @param packet_p   
 * @param in_raid_group 
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/2/2010 - Created from fbe_raid_group_get_chunk_info_for_start_iots_completion()
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_limit_and_start_iots(fbe_packet_t*          in_packet_p,
                                                 fbe_raid_group_t*      in_raid_group_p)
{
    fbe_status_t                status;
    fbe_payload_ex_t           *sep_payload_p; 
    fbe_raid_iots_t            *iots_p;                             
    fbe_chunk_size_t            chunk_size;
    fbe_lba_t                   lba;
    fbe_block_count_t           blocks;
    fbe_block_count_t           new_blocks;    
    fbe_raid_position_bitmask_t rg_rl_bitmask;
    fbe_raid_position_bitmask_t iots_rl_bitmask;
    fbe_trace_level_t           np_rl_iots_rl_mismatch_trace_level = FBE_TRACE_LEVEL_WARNING;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p);
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

    //  Put the packet back on the termination queue since we had to take it off before reading the chunk info
    fbe_raid_group_add_to_terminator_queue(in_raid_group_p, in_packet_p);

    if (fbe_raid_group_io_rekeying(in_raid_group_p)){
        /* In the case of a rekey we will fill out the iots with the information about what should be rekeyed.
         */
        fbe_raid_position_bitmask_t bitmap;
        fbe_raid_position_bitmask_t all_mask;
        fbe_raid_position_bitmask_t degraded_mask;
        fbe_u32_t width;

        fbe_raid_geometry_get_width(raid_geometry_p, &width);
        fbe_raid_group_rekey_get_bitmap(in_raid_group_p, in_packet_p, &bitmap);
        fbe_raid_group_get_degraded_mask(in_raid_group_p, &degraded_mask);
        fbe_raid_iots_set_rekey_bitmask(iots_p, bitmap);

        all_mask = (1 << width) - 1;
        /* During rekey iw recovery we might find all bits set.
         */
        if ((iots_p->chunk_info[0].needs_rebuild_bits & all_mask) == all_mask) {
            iots_p->chunk_info[0].needs_rebuild_bits = degraded_mask;
        }
    }
    // Break the I/O on any degraded/non-degraded chunk boundaries.     
    status = fbe_raid_iots_limit_blocks_for_degraded(iots_p, chunk_size);
    if (status != FBE_STATUS_OK)
    {
        /* Set status to unexpected and fail the I/O.
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s limit blocks for degraded error status %d \n", 
                              __FUNCTION__, status);
        fbe_raid_group_iots_completion(in_packet_p, in_raid_group_p);
        return status;
    }

    fbe_raid_iots_get_current_op_blocks(iots_p, &new_blocks);
    if (new_blocks != blocks)
    {
        //  Remove the packet from the terminator queue since this function expects it to be off the queue. 
        fbe_raid_group_remove_from_terminator_queue(in_raid_group_p, in_packet_p);
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_get_chunk_info_for_start_iots_completion,
                                              (fbe_packet_completion_context_t) in_raid_group_p);
        fbe_raid_group_get_chunk_info(in_raid_group_p, in_packet_p, iots_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Validate that we can continue.
     */
    status = fbe_raid_group_validate_iots_chunk_info(in_raid_group_p, iots_p, lba, blocks);
    if (status != FBE_STATUS_OK)
    {
        /* Set status to unexpected too many dead
         */
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS);
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: %s limit blocks for degraded error status %d \n", 
                              __FUNCTION__, status);
        fbe_raid_group_iots_completion(in_packet_p, in_raid_group_p);
        return status;
    }

    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rg_rl_bitmask); 
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &iots_rl_bitmask);

    /*! @note This is a WARNING since there will be cases where the non-paged
     *        is modified after the I/O is started and the I/O is not quiesced.
     *        If the metadata request is waiting for a lock etc, it will not
     *        be quiesced.
     */
    if (rg_rl_bitmask != iots_rl_bitmask)
    {
        /* If this I/O was quiesced it better match.
         */
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED))
        {
            np_rl_iots_rl_mismatch_trace_level = FBE_TRACE_LEVEL_ERROR;
        }
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              np_rl_iots_rl_mismatch_trace_level, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_lmit_strt_iots rg_rl_bitmask 0x%x != iots_rl_bitmask 0x%x \n", 
                              rg_rl_bitmask, iots_rl_bitmask);
    }

    /* Setup the current operation in the iots if the first op we will do is different. 
     * Such as with check zeroed and rebuild, verify on-demand or rebuild on-demand. 
     */
    fbe_raid_group_iots_get_first_operation(in_raid_group_p, iots_p);    

    /* Kick off this I/O to the library.  Our callback will be executed on completion.
     */
    fbe_raid_iots_start(iots_p);


    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/******************************************
 * end fbe_raid_group_limit_and_start_iots()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_rebuild_not_required_iots_completion()
 ****************************************************************
 * @brief
 *   This is the completion function that gets called 
 *   for cases we don't need to rebuild the chunk.
 *   This will clean up and complete the iots.
 * 
 * @param in_packet_p   - packet that is completing
 * @param in_context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_rebuild_not_required_iots_completion(fbe_packet_t *in_packet_p,
                                                    fbe_packet_completion_context_t in_context)
{
    fbe_raid_group_t*               raid_group_p;
    fbe_payload_ex_t*              sep_payload_p; 
    fbe_raid_iots_t*                iots_p;           

    raid_group_p = (fbe_raid_group_t*)in_context;

    // Put the packet back on the termination queue since we had to take it off before reading the chunk info
    fbe_raid_group_add_to_terminator_queue(raid_group_p, in_packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* We can mark the status as success since we did not issue any I/O.
     */
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    fbe_raid_group_iots_cleanup(in_packet_p,in_context);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************************
 * end fbe_raid_group_rebuild_not_required_iots_completion()
 ***********************************************************/
/*!**************************************************************
 * fbe_raid_group_mark_for_verify_completion()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_mark_for_verify_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t lba;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_lba(iots_p, &lba);

    status = fbe_transport_get_status_code(packet_p);

    /* Put it back on the termination queue since the function we call expects this.
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    if ((status == FBE_STATUS_FAILED) || (status == FBE_STATUS_IO_FAILED_RETRYABLE))
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
        fbe_bool_t b_is_lba_disk_based;

        b_is_lba_disk_based = fbe_raid_library_is_opcode_lba_disk_based(opcode);
        if (b_is_lba_disk_based ||
            fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba) ||
            (packet_p->packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE))
        {
            /* We failed marking nv for a monitor I/O or for a metadata I/O.
             * Monitor and metadata I/Os are not quiesced and need to be completed immediately.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: mark nv or recons failed iots op: 0x%x lba: 0x%llx blks: 0x%llx pkt_st: 0x%x\n",  
                                  opcode, iots_p->lba, iots_p->blocks, status);

            /* Need to populate the block status and qualifier.  For child raid groups
             * we do not want to mark it as retryable since the retries should 
             * have been performed in the lower (i.e. `child' raid group). 
             */
            if (fbe_raid_geometry_is_child_raid_group(raid_geometry_p) == FBE_TRUE)
            {
                /* Mark this as a non-retryable error
                 */
                fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
                fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
            }
            else
            {
                /* Else mark this as a retryable I/O error
                 */ 
                fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
                fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            }
            fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: mark nv or recons failed, queueing iots op: 0x%x lba: 0x%llx blks: 0x%llx pkt_st: 0x%x\n",  
                                  opcode, iots_p->lba, iots_p->blocks, status);
            fbe_raid_iots_transition_quiesced(iots_p, fbe_raid_group_iots_restart_completion, raid_group_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: mark nv failed with pkt_st: 0x%x op: 0x%x lba/bl: 0x%llx/0x%llx\n",  
                              status, opcode, iots_p->lba, iots_p->blocks);
    }

    /* Clear these flags so that when we go through iots completion we do not 
     * try to update the map again. 
     */
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE);

    /* Complete this I/O.
     */
    fbe_raid_group_iots_completion(packet_p, context);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_mark_for_verify_completion()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_iots_completion()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_iots_completion(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_opcode_t current_opcode;
    fbe_payload_block_operation_status_t block_status;
    fbe_lba_t lba;
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_current_opcode(iots_p, &current_opcode);
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_lba(iots_p, &lba);

    if (fbe_raid_iots_is_flag_set(iots_p,FBE_RAID_IOTS_FLAG_REKEY_MARK_DEGRADED)){
        /* Rekey operations will mark degraded after the reads are complete.
         */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_rekey_mark_deg_completion, raid_group_p);       
        fbe_raid_rekey_mark_degraded(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If verify after write is set start the verify.
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY) &&
        (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)                                 &&
        fbe_raid_group_is_verify_after_write_allowed(raid_group_p, iots_p)                              )
    {
        /* Start the verify.
         */
        status = fbe_raid_group_iots_start_next_operation(raid_group_p, iots_p);
        if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    /* If the current iots opcode is different from the packet, then
     * we are doing a different operation first (like rebuild/write, 
     *                                           verify/any op, check zeroed/rebuild).
     * In this case we will need to re-issue the operation.
     */
    if (current_opcode != opcode)
    {
        /* If the verify after write flag is set then complete the request.
         */
        if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY) &&
            (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY)                      &&
            (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)                                            ) {
            /* Clean up the verify and continue.
             */
            fbe_raid_group_cleanup_verify_after_write(raid_group_p, iots_p);
        } else {
            /* Else reissue the operation with a different opcode.
             */
            status = fbe_raid_group_iots_start_next_operation(raid_group_p, iots_p);
            if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
            {
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
    }

    /* Next check to see if this iots is really done (and successful) or has more pieces to kick off.
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD)
    {
        /* The return status from process iots completion will determine if we 
         * are done or not.
         */
        status = fbe_raid_group_rebuild_process_iots_completion(raid_group_p, packet_p);
        if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    /* Check whether it was a verify operation that was completed.
       if it is clear the paged metadata for that chunk and update the 
       checkpoint
    */
    if ( (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY)||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY)||           
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) )
    {
        // Take the packet off the termination queue since the packet will be forwarded on and 
        // completed to us.         
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p); 
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_iots_completion, raid_group_p);       

        // Update the verfiy metada        
        fbe_raid_verify_update_metadata(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* Check if Write_log operation has remap flag set. */
    if (((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) || 
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH)) &&
        (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED)))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg: setting cond for write_log remap. opcode:0x%x \n", opcode);

        fbe_raid_geometry_journal_set_flag(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_NEEDS_REMAP);

        /* set the journal_remap condition in the raid group object */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               (fbe_base_object_t *)raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_REMAP);
    }
    else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS))
    {
        /* If it is marked for incomplete write fail as retryable.
         */
         if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE))
         {
             fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
             fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
         }
         else
         {
             // Take the packet off the termination queue since the packet will be forwarded on and 
             // completed to us.         
             fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p); 

            fbe_transport_set_completion_function(packet_p, fbe_raid_group_rekey_iots_completion, raid_group_p);       
    
            /* Set status to good since it was invalidated prior to sending.  */
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    
            fbe_raid_rekey_update_metadata(packet_p, raid_group_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
         }
    }
    /* Verifies and rebuilds will not mark the chunk since the chunk is already marked.
     * We do not mark needs verify on Raid 10 since the mirrors do this. 
     * We don't have to do this if we are zeroing paged area (in specialize state)
     */
    else if (!fbe_raid_library_is_opcode_lba_disk_based(opcode) &&
        (raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10) &&
        (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED) ||
         fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE)) &&
        ((opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
          (!(fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba)))))
    {
        /* We need to mark this chunk as needing a verify.
         * The first step is upgrading the lock. 
         * Take it off the termination queue since the packet will be forwarded on and completed to us.
         */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

        /* We do not want this packet to get cancelled, since it must be completed.
         */
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);

        /*! @todo do we need this setting of expiration time anymore?
         * If we are already aborted or expired, set a short expiration time.
         */
        if (fbe_raid_iots_is_marked_aborted(iots_p) ||
            fbe_raid_iots_is_expired(iots_p))
        {
            fbe_transport_set_and_start_expiration_timer(packet_p, fbe_raid_group_get_paged_metadata_timeout_ms(raid_group_p));
        }

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: start mark needs verify for remap iots: %p\n", iots_p);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_mark_for_verify_completion, raid_group_p);
        fbe_raid_group_mark_for_verify(raid_group_p, packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_group_iots_start_next_piece(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_iots_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_iots_paged_metadata_update_completion()
 ****************************************************************
 * @brief
 *  Paged metadata gets updated, check the status and continue
 *  with updating nonpaged metadata if status is good else update
 *  go to next step to complete the iots with error.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/26/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_iots_paged_metadata_update_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                  raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t                        status;
    fbe_raid_iots_t *                   iots_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_lba_t                           start_lba;

    /* Verify the status of the paged metadata operation. */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        /* Put it back on the terminator queue since all the following code expects it to be there. */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

        /* Continue the IOTS. */
        fbe_raid_group_iots_start_next_piece(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the start lba from the block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);

    /* If the request is not complete place it back on the terminator queue and
     * start the next piece.
     */
    if (!fbe_raid_iots_is_request_complete(iots_p))
    {        
        /* Put it back on the terminator queue since all the following code expects it to be there. */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

        /* Continue the IOTS. */
        fbe_raid_group_iots_start_next_piece(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    }

    /* Set the completion function before we issue nonpaged metadata update request. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_iots_metadata_update_completion, raid_group_p);

    /* if paged metadata update passed successfully then update the nonpaged metadata. */
    status = fbe_raid_group_rebuild_update_checkpoint_after_io_completion(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_iots_paged_metadata_update_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_iots_metadata_update_completion()
 ****************************************************************
 * @brief
 *  Metadata is updated, continue the io.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  7/14/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_iots_metadata_update_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;

    /* Put it back on the terminator queue since all the following code expects it to be there.
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    /* Continue the IOTS.
     */
    fbe_raid_group_iots_start_next_piece(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************
 * end fbe_raid_group_iots_metadata_update_completion()
 ******************************************************/

/*!**************************************************************
 * fbe_raid_group_iots_cleanup()
 ****************************************************************
 * @brief
 *  Iots is done, cleanup and send status.
 *
 * @param packet_p - Packet that is completing.
 * @param context - raid group ptr.
 *
 * @return fbe_status_t.
 *
 * @author
 *  8/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_iots_cleanup(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_bool_t      np_lock;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* If the np lock bit is set, we must first release the NP lock before we go ahead and release the 
      stripe lock.This flag will only be set when we are doing a BV
    */
    fbe_raid_iots_get_np_lock(iots_p, &np_lock);
    if(np_lock)
    {
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, fbe_raid_iots_get_packet(iots_p));
        fbe_raid_iots_clear_np_lock(iots_p);
        fbe_transport_set_completion_function(packet_p, (fbe_packet_completion_function_t)fbe_raid_iots_proceed_cleanup, raid_group_p);
        fbe_raid_group_trace(raid_group_p, 
                             FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: iots_p: 0x%x lba: 0x%llx blks: 0x%llx releasing the np lock\n", 
                             __FUNCTION__, (fbe_u32_t)iots_p, 
                             (unsigned long long)iots_p->lba,
                             (unsigned long long)iots_p->blocks);

        fbe_base_config_release_np_distributed_lock((fbe_base_config_t*)raid_group_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If debug is enabled trace the error.
     */
    if (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        fbe_payload_block_operation_opcode_t opcode;

        fbe_raid_iots_get_opcode(iots_p, &opcode);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s iots: %p failed status: 0x%x qual: 0x%x opcode: 0x%x lba: 0x%llx bl: 0x%llx\n",
                              __FUNCTION__, iots_p, iots_p->error,
			      iots_p->qualifier, opcode,
                              (unsigned long long)iots_p->packet_lba,
			      (unsigned long long)iots_p->packet_blocks);
    }

    /* Determine if we took the stripe lock or not.
     *  
     * In some cases we also come through this path when we went shutdown before we could get our lock, 
     * so we might not have our lock yet. 
     */
    if ((fbe_raid_group_is_iots_stripe_lock_required(raid_group_p, iots_p) == FBE_FALSE) ||
        (sep_payload_p->current_operation->payload_opcode != FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION))
    {
        /* We did not get any stripe locks.
         * Just cleanup the iots and send status. 
         * Pull off the terminator queue since we are done with the I/O. 
         */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, 
                                                    fbe_raid_iots_get_packet(iots_p));
        fbe_raid_group_iots_finished(raid_group_p, iots_p);

        status = fbe_transport_complete_packet(packet_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: error completing packet status: 0x%x\n", __FUNCTION__, status);
        }
    }
    else
    {
        /* We need to cleanup.
         * The first action is to release the stripe lock if we have one.
         */
        fbe_raid_group_iots_release_stripe_lock(raid_group_p, iots_p);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_iots_cleanup()
 **************************************/

/*!**************************************************************
 * fbe_raid_iots_proceed_cleanup()
 ****************************************************************
 * @brief
 *  Iots is done, cleanup and send status.
 *
 * @param packet_p - Packet that is completing.
 * @param context - raid group ptr.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/11/2011 - Created. Ashwin Tamilarasan
 *               Created from fbe_raid_iots_cleanup
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_proceed_cleanup(void * packet_p, fbe_packet_completion_context_t context)
{
    //fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    if (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_iots_get_opcode(iots_p, &opcode);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s iots: %p failed status: 0x%x qual: 0x%x opcode: 0x%x lba: 0x%llx bl: 0x%llx\n",
                              __FUNCTION__, iots_p, iots_p->error,
			      iots_p->qualifier, opcode,
                              (unsigned long long)iots_p->packet_lba,
			      (unsigned long long)iots_p->packet_blocks);
    }

    /* Determine if we took the stripe lock or not.
     *  
     * In some cases we also come through this path when we went shutdown before we could get our lock, 
     * so we might not have our lock yet. 
     */
    if ((fbe_raid_group_is_iots_stripe_lock_required(raid_group_p, iots_p) == FBE_FALSE) ||
        (sep_payload_p->current_operation->payload_opcode != FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION))
    {
        /* We did not get any stripe locks.
         * Just cleanup the iots and send status. 
         * Pull off the terminator queue since we are done with the I/O. 
         */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, 
                                                    fbe_raid_iots_get_packet(iots_p));
        fbe_raid_group_iots_finished(raid_group_p, iots_p);
        return FBE_STATUS_OK;
    }
    else
    {
        /* We need to cleanup.
         * The first action is to release the stripe lock if we have one.
         */
        fbe_raid_group_iots_release_stripe_lock(raid_group_p, iots_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_iots_proceed_cleanup()
 **************************************/


/*!**************************************************************
 * fbe_raid_group_iots_start_next_piece()
 ****************************************************************
 * @brief
 *  Start the next piece of this iots or complete it.
 *
 * @param packet_p - Packet that is completing.
 * @param context - raid group ptr.
 *
 * @return fbe_status_t.
 *
 * @author
 *  7/14/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_iots_start_next_piece(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_opcode_t block_opcode;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_opcode(iots_p, &block_opcode);

    if (fbe_raid_iots_needs_upgrade(iots_p))
    {
        /* IOTS received an error and needs to upgrade the lock.
         */
        fbe_raid_iots_set_callback(iots_p, (fbe_raid_iots_completion_function_t)fbe_raid_group_iots_finished, raid_group_p);
        /*! @todo for now just act like the upgrade is successful.
         */
        fbe_raid_iots_lock_upgrade_complete(iots_p);
    }
    else if (fbe_raid_iots_is_request_complete(iots_p))
    {
        /* Either we are done, or we are a verify.  Verifies always finish after one iots.
         */
        fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
    }
    else
    {
        if ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
            (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) ||
            (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY) ||
            (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) ||
            (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY))
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Verify should not go to library more than once\n", __FUNCTION__);
        }
        /* Before we start the next iots, let's check if the current iots has media error.
         * If the iots has media error, let's update the block operation to record the fact
         * that media error happened.
         */
        if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
        {
            fbe_raid_iots_set_block_operation_status(iots_p);
        }
   
        /* If the previous I/O was quiesced we need to decrement the `quiesced' 
         * count.
         */
        fbe_raid_group_handle_was_quiesced_for_next(raid_group_p, iots_p);
             
        /* Initialize for the remaining blocks.
         */
        status = fbe_raid_iots_init_for_next_lba(iots_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
            fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
            return status; 
        }

        /* Kick off the iots for the new range.
         */
        fbe_raid_group_start_iots(raid_group_p, iots_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_iots_start_next_piece()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_get_chunk_info()
 ****************************************************************
 * @brief
 *   Get the chunk information for the LBAs in the current I/O 
 *   request. 
 *    
 * @param raid_group_p   - pointer to the raid group object
 * @param packet_p       - pointer to packet 
 * @param iots_p         - pointer to current I/O
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_get_chunk_info(
                                          fbe_raid_group_t*           raid_group_p,
                                          fbe_packet_t*               packet_p,
                                          fbe_raid_iots_t*            iots_p)
{
    fbe_lba_t                               lba;    // start LBA of I/O
    fbe_block_count_t                       blocks;    // block count of I/O 
    fbe_chunk_count_t                       start_bitmap_chunk_index;
    fbe_chunk_count_t                       num_chunks;
    fbe_chunk_size_t                        chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    //  Get the LBA and blocks from the iots 
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

    //  Get the range of chunks affected by this iots.
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &start_bitmap_chunk_index, &num_chunks);

    if (num_chunks > FBE_RAID_IOTS_MAX_CHUNKS)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s number of chunks is beyond the max for an iots %d\n",
                              __FUNCTION__, num_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                         "get chunk info iots: 0x%x lba: 0x%llx bl: 0x%llx rl_b: 0x%x ci: 0x%x cc: 0x%x\n",
                         (fbe_u32_t)iots_p, (unsigned long long)iots_p->lba, (unsigned long long)iots_p->blocks, iots_p->rebuild_logging_bitmask,
                         start_bitmap_chunk_index, num_chunks);

    //  We are going to read the metadata.  Set the completion function for it. 
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_get_chunk_info_completion,
                                          (fbe_packet_completion_context_t) raid_group_p);

    //  Issue the read to get the chunk info from the metadata  
    fbe_raid_group_bitmap_read_chunk_info_for_range(raid_group_p, packet_p, start_bitmap_chunk_index, num_chunks);

    //  Return more processing since we have a packet outstanding
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_get_chunk_info()
 **************************************/

/*!**************************************************************
 * fbe_raid_is_iots_chunk_info_clear_needed()
 ****************************************************************
 * @brief
 * For zero request if we are going to send the full request down the we will
 * clear the chunk info for this request. This function validates the request
 * is of expected type or not
 *
 * @param itos_p
 * @param raid_group_p
 *
 * @return fbe_bool_t.
 *
* @author
 *  21/04/2011 - Created. Mahesh Agarkar
 *
 ****************************************************************/
fbe_bool_t fbe_raid_is_iots_chunk_info_clear_needed(fbe_raid_iots_t * iots_p,
                                                    fbe_raid_group_t * raid_group_p)
{
    fbe_raid_position_bitmask_t nr_on_demand_bitmask;
    fbe_chunk_size_t chunk_size =fbe_raid_group_get_chunk_size(raid_group_p);

    /* Get the NR on demand bitmask.  If any bit is set, then the RG is degraded - return TRUE.
     */
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &nr_on_demand_bitmask);

    /* We will do three checks here:
     *       (1) This should be zero request and aligned to chunk
     *       (2) Rebuild logging is not set
     *       (3) We should be degraded
     */
    if ((fbe_raid_zero_request_aligned_to_chunk(iots_p, chunk_size)) &&
        (nr_on_demand_bitmask == 0) &&
        (fbe_raid_group_is_degraded(raid_group_p)))
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/*****************************************************
 * end fbe_raid_is_iots_chunk_info_clear_needed()
 *****************************************************/


/*!**************************************************************
 * fbe_raid_group_get_chunk_info_completion()
 ****************************************************************
 * @brief
 *   This is the completion function for a read to the metadata  
 *   to get the chunk info.  It will retrieve the chunk info and
 *   store it in the IOTS.
 *   
 *   The caller is responsible for handling any error on the metadata
 *   operation. 
 * 
 * @param in_packet_p   - packet that is completing
 * @param in_context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  08/04/2010 - Created.  Jean Montes. 
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_get_chunk_info_completion(fbe_packet_t *in_packet_p,
                                                             fbe_packet_completion_context_t in_context)
{

    fbe_raid_group_t*               raid_group_p = NULL; 
    fbe_payload_ex_t*               sep_payload_p = NULL;
    fbe_raid_iots_t*                iots_p = NULL;
    fbe_lba_t                       lba;
    fbe_block_count_t               blocks;
    fbe_chunk_size_t                chunk_size;
    fbe_chunk_count_t               chunk_count;
    fbe_chunk_count_t               chunk_index;
    fbe_status_t                    status;
    fbe_bool_t                      is_in_data_area_b; // true if chunk is in the data area
    fbe_payload_block_operation_opcode_t opcode;

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Get the IOTS for this I/O
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    //  Get the LBA and block count from the IOTS
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);
    fbe_raid_iots_get_current_opcode(iots_p, &opcode);

    //  Get the chunk size 
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p); 

    //  Determine the start chunk index and number of chunks.  This call will adjust for disk-based LBAs vs 
    //  raid-based LBAs. 
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &chunk_index, &chunk_count);

    //  Get the chunk info that was read
    //  Determine if the chunk(s) to be read are in the user data area or the paged metadata area.  If we want
    //  to read a chunk in the user area, we use the paged metadata service to do that.  If we want to read a 
    //  chunk in the paged metadata area itself, we use the nonpaged metadata to do that (in the metadata data
    //  portion of it).
    fbe_raid_group_bitmap_determine_if_chunks_in_data_area(raid_group_p, chunk_index, chunk_count, &is_in_data_area_b);

    //  If the chunk is not in the data area, then the non-paged metadata service was used to read it - check 
    //  status and get the data for a non-paged operation 
    if (is_in_data_area_b == FBE_FALSE)
    {
        status = fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done(raid_group_p, in_packet_p, 
            chunk_index, chunk_count, &iots_p->chunk_info[0]);
    }

    if ((status == FBE_STATUS_OK) &&
        (fbe_raid_is_iots_chunk_info_clear_needed(iots_p, raid_group_p)))
    {
        /* if we are doing ZERO opreation and if we are here then we are suppose to handle the degraded scenario
         * while no rebuild logging is on and some chunks are degraded and if we are going for zeroing then 
         * we will clear the needs rebuild bit from chunk info so that later when we come for rebuild we will not rebuild
         * these chunks as these will be already zeroed
         */
        fbe_zero_memory(&iots_p->chunk_info[0], FBE_RAID_IOTS_MAX_CHUNKS * sizeof(fbe_raid_group_paged_metadata_t));
    }
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                          "chunk rd end iots: 0x%x lba: 0x%llx bl: 0x%llx rl_b: 0x%x ci: 0x%x cc: 0x%x\n",
                          (fbe_u32_t)iots_p, iots_p->lba, iots_p->blocks, iots_p->rebuild_logging_bitmask, chunk_index, chunk_count);

    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR))
    {
        /* Trace all the `needs rebuild' information.
         */
        fbe_raid_group_rebuild_trace_chunk_info(raid_group_p, &iots_p->chunk_info[0], chunk_count, iots_p);
    }
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE) {
        fbe_u32_t width;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
        fbe_raid_geometry_get_width(raid_geometry_p, &width);

        if (iots_p->chunk_info[0].needs_rebuild_bits == ((1 << width) - 1)) {
            /* if we are a rekey write, we might find multiple bits set.  We should clear out all but the degraded bits  
             * to enable this recovery from an incomplete write to proceed. 
             */
            fbe_raid_position_bitmask_t degraded_mask;
            fbe_raid_group_get_degraded_mask(raid_group_p, &degraded_mask);

            /* Since all of the bits are now set, set only for drives that are not present.
             */
            iots_p->chunk_info[0].needs_rebuild_bits = degraded_mask;
        }
    }

    //  The status of the metadata read needs to be checked and handled by the caller.  It is traced there instead
    //  of here since this is a common code path and therefore the trace is not as helpful. 

    //  Return success 
    return FBE_STATUS_OK;

}
/************************************************
 * end fbe_raid_group_get_chunk_info_completion()
 ************************************************/

/*!***************************************************************************
 *          fbe_raid_group_restart_failed_io()
 *****************************************************************************
 *
 * @brief   This method restarts (via iots) an I/O that failed in the normal
 *          I/O path.  This method supports (3) failures:
 *              1) Transport failure
 *              2) Block operation failure
 *              3) Stripe lock failure
 * 
 * @param   raid_group_p - the raid group object.
 * @param   packet_p - packet to start the iots for.
 * @param   failure_type - Determines how to process the failure.
 *
 * @return  status - Unless there is an unexpected error detected this method
 *                   should return FBE_STATUS_MORE_PROCESSING_REQUIRED since we
 *                   are in the completion context and will retry the request.
 *
 * @author
 *  02/16/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_restart_failed_io(fbe_raid_group_t *raid_group_p, 
                                                     fbe_packet_t *packet_p,
                                                     fbe_raid_group_io_failure_type_t failure_type)
{
    fbe_status_t                    status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    fbe_payload_block_operation_status_t request_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t request_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    fbe_raid_iots_t                *iots_p = NULL;
    fbe_raid_geometry_t            *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t               *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t  *block_operation_p = NULL;
    fbe_payload_block_operation_t  *master_block_operation_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_lba_t                       lba;
    fbe_block_count_t               blocks;
    fbe_chunk_size_t                chunk_size;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;

    /* If we encountered a stripe lock error get the master block operation.
     */
    if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
    }
    else
    {
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    }

    /* If it is a transport failure trace some information.
     */
    switch(failure_type)
    {
        case FBE_RAID_GROUP_IO_FAILURE_TYPE_TRANSPORT:
            /* Packet never reached destination.  Since we might have sent the
             * I/O to an edge that is now disabled we treat this line a block
             * operation error.
             */
            request_status = fbe_transport_get_status_code(packet_p);
            request_qualifier = fbe_transport_get_status_qualifier(packet_p);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: restart io transport sts/qual: 0x%x/0x%x\n",
                                  request_status, request_qualifier);
            break;

        case FBE_RAID_GROUP_IO_FAILURE_TYPE_BLOCK_OPERATION:
        case FBE_RAID_GROUP_IO_FAILURE_TYPE_CHECKSUM_ERROR:
        case FBE_RAID_GROUP_IO_FAILURE_TYPE_STRIPE_LOCK:
            /* These failures will be handled below.
             */
            fbe_payload_block_get_status(block_operation_p, &request_status);
            fbe_payload_block_get_qualifier(block_operation_p, &request_qualifier);
            break;

        default:
            /* Unsupported failure type.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: restart io unsupported failure type: %d\n",
                                  failure_type);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

            /* Return Ok since we will now complete the packet.
             */
            return FBE_STATUS_OK;
    }

    /* Get the current block operation information.
     */
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* Invoke method that will update counters etc. based on failure.
     */
    status = fbe_raid_group_process_io_failure(raid_group_p, packet_p, failure_type, &master_block_operation_p);

    /* Trace failing request if enabled.
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: restart io pkt:%p lba:%012llx blks:%08llx op:%d st/q:%d/%d type:%d proc sts:%d\n",
                              packet_p,
			      (unsigned long long)lba,
			      (unsigned long long)blocks,
			      opcode, request_status, request_qualifier,
			      failure_type, status);
    }

    /* Independent of the status returned we must release the current block 
     * operation.  If it was a stripe lock request the block operation is 
     * pointer to the master so there is nothing to release.
     */
    if (failure_type != FBE_RAID_GROUP_IO_FAILURE_TYPE_STRIPE_LOCK)
    {
        /* Validate that the error was processed.
         */
        if (master_block_operation_p == NULL)
        {
            /* Something went wrong, report the error and fail the request.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: restart io process failure: %d no master found.\n",
                              failure_type);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

            /* Return Ok since we will now complete the packet.
             */
            return FBE_STATUS_OK;
        }

        /* Else release the current block operation since we will use the master
         * to either retry or fail.
         */
        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    }
    else
    {
        /* Else for a stripe lock operation the current block operation is the
         * master.
         */
        master_block_operation_p = block_operation_p;
    }
    block_operation_p = NULL;

    /* Get the master block operation status.
     */
    fbe_payload_block_get_status(master_block_operation_p, &block_status);

    /* Based on the process I/O failure status, determine if we retry the 
     * original request or not.
     */
    switch(status)
    {
        case FBE_STATUS_OK:
            /* This status indicates that it is ok to retry the request using
             * the raid library.  Just continue.  Validate that the master block
             * operation status is still `invalid' (i.e. not populated).
             */
            if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
            {
                /* We didn't correctly populate the block status.
                 */
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: restart io unexpected retry block status: 0x%x\n",
                                      block_status);
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

                /* Return Ok since we will now complete the packet.
                 */
                return FBE_STATUS_OK;
            }
            break;

        default:
            /* Something went wrong so we need to return status on this packet.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: restart io process failed status: 0x%x\n",
                                  status);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            /* fall through  */

        case FBE_STATUS_FAILED:
        case FBE_STATUS_CANCELED:
        case FBE_STATUS_IO_FAILED_NOT_RETRYABLE:
        case FBE_STATUS_TIMEOUT:
            /*! @note The above status indicate that we should not retry the
             *        request and simply fail it.  We set the packet status
             *        to `success' which assumes that we populated the block
             *        status.
             */
            if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING))
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: restart io pkt:%p process io status: 0x%x not retryable.\n",
                              packet_p, status);
            }
    
            /* Validate that the block operation status has been populated.
             */
            switch(block_status)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
                case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
                case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
                case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
                    /* Correctly populated block status.  Change packet status
                     * to success.
                     */
                    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                    break;
                default:
                    /* We didn't correctly populate the block status.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: restart io unexpected failed block status: 0x%x\n",
                                  block_status);
                    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                    fbe_payload_block_set_status(master_block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
                    break;
            }

            /* Release stripe lock operation if required.
             */
            if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
            {
                /* Now release the stripe lock and complete the request.
                 */
                stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);
                fbe_transport_set_completion_function(packet_p, fbe_raid_group_direct_io_unlock_completion, raid_group_p);

                /* Base on the opcode release the strip operation.
                 */
                if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
                {
                    fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation_p);
                }
                else
                {
                    fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation_p);
                }
                fbe_stripe_lock_operation_entry(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }

            /* Return Ok since we will now complete the packet.
             */
            return FBE_STATUS_OK;

    } /* end switch on process io failure status */

    /* Initialize and/or re-initialize packet fields.
     *    o Reset the packet resource priority.
     */
    //packet_p->resource_priority = FBE_MEMORY_DPS_RAID_BASE_PRIORITY;
    //packet_p->resource_priority_boost &= ~FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_RG;
    //fbe_raid_common_set_resource_priority(packet_p, raid_geometry_p);

    /* Initialize the iots.
     */
    fbe_payload_block_get_lba(master_block_operation_p, &lba);
    fbe_payload_block_get_block_count(master_block_operation_p, &blocks);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_raid_iots_init(iots_p, packet_p, master_block_operation_p, raid_geometry_p, lba, blocks);
    fbe_raid_iots_set_chunk_size(iots_p, chunk_size);
    fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);

    /* If it was a transport or block operation failure we are still holding 
     * the lock.  Simply add the request to the terminator queue and start the 
     * iots.
     */
    if ((failure_type == FBE_RAID_GROUP_IO_FAILURE_TYPE_TRANSPORT)       || 
        (failure_type == FBE_RAID_GROUP_IO_FAILURE_TYPE_BLOCK_OPERATION) ||
        (failure_type == FBE_RAID_GROUP_IO_FAILURE_TYPE_CHECKSUM_ERROR)     )
    {
        /* Terminate the I/O since we will be generating new packets to 
         * the next level.  If a stripe lock was required we already have it.
         */ 
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
    }
    else if (failure_type == FBE_RAID_GROUP_IO_FAILURE_TYPE_STRIPE_LOCK)
    {
        /* Else process the stripe lock failure.
         */
        return fbe_raid_group_stripe_lock_completion(packet_p, (fbe_packet_completion_context_t)raid_group_p);
    }
    else
    {
        /* Else we attempted to process an unsupported failure type.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: restart io unexpected failure type: %d\n",
                              failure_type);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

        /* Return Ok since we will now complete the packet.
         */
        return FBE_STATUS_OK;
    }

    /* Always return `more processing' since we will retry the I/O.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_restart_failed_io()
 ******************************************/

/*!***************************************************************************
 * fbe_raid_group_start_new_iots()
 *****************************************************************************
 * @brief
 *  Start a new iots that has just arrived in the object.
 * 
 * @param raid_group_p - the raid group object.
 * @param packet_p - packet to start the iots for.
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t fbe_raid_group_start_new_iots(fbe_raid_group_t *raid_group_p, 
                                           fbe_packet_t *packet_p)
{
    fbe_raid_iots_t     *iots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_chunk_size_t chunk_size;
    fbe_payload_block_operation_opcode_t opcode;

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Raid group: %s block operation is null.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    /* Initialize and kick off iots.
     * The iots is part of the payload. 
     */
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* All background ops should be a multiple of the chunk size.
     */
    if (fbe_raid_library_is_opcode_lba_disk_based(opcode) && (blocks % chunk_size))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "invalid background op size lba: 0x%llx block_count: 0x%llx\n", lba, blocks);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_COUNT);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }
    fbe_raid_iots_init(iots_p, packet_p, block_operation_p, raid_geometry_p, lba, blocks);
    fbe_raid_iots_set_chunk_size(iots_p, chunk_size);
    fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);

    /* Determine if we need to take the stripe lock or not.
     */
    if (fbe_raid_group_is_iots_stripe_lock_required(raid_group_p, iots_p) == FBE_FALSE)
    {
        /* We do not need locks since the mirror does the locking.
         * Terminate the I/O since we will be generating new packets to 
         * the next level.
         */ 
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
    }
    else
    {
        /* Else take the stripe lock (typical case)
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_stripe_lock_completion, raid_group_p);
        fbe_raid_group_get_stripe_lock(raid_group_p, packet_p);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_start_new_iots()
 ******************************************/

/*!***************************************************************
 * fbe_raid_group_negotiate_block_size()
 ****************************************************************
 * @brief
 *  This function handles the negotiate block size packet.
 * 
 *  This allows the caller to determine which block size and optimal block size
 *  they can use when sending I/O requests.
 *
 * @param raid_group_p - The raid group handle.
 * @param packet_p - The mgmt packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/22/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_negotiate_block_size(fbe_raid_group_t * const raid_group_p, 
                                    fbe_packet_t * const packet_p)
{
    fbe_block_transport_negotiate_t* negotiate_block_size_p = NULL;

    /* First get a pointer to the block command.
     */
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_sg_element_t *sg_p;
    fbe_status_t status;
    fbe_lba_t capacity;
    fbe_block_edge_t *edge_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u32_t raid_group_optimal_block_size;

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    if (sg_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "raid group: negotiate sg null. 0x%p %s\n", 
                              raid_group_p, __FUNCTION__);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Next, get a ptr to the negotiate info. 
     */
    negotiate_block_size_p = (fbe_block_transport_negotiate_t*)fbe_sg_element_address(sg_p);

    if (negotiate_block_size_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "raid group: negotiate buffer null. 0x%p %s \n", 
                              raid_group_p, __FUNCTION__);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Generate the negotiate info.  
     * Raid always exports 520 (FBE_BE_BYTES_PER_BLOCK), with an optimal block size 
     * that is set to the raid group's optimal size, which gets set by the 
     * monitor's "get block size" condition. 
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &raid_group_optimal_block_size); 
    status = FBE_STATUS_OK;
    fbe_base_config_get_capacity(&raid_group_p->base_config, &capacity);
    edge_p = (fbe_block_edge_t*)packet_p->base_edge;
    if (edge_p != NULL)
    {
        fbe_u16_t data_disks;
        fbe_u32_t stripe_size;
        fbe_element_size_t element_size;
        fbe_raid_geometry_get_element_size(&raid_group_p->geo, 
                                           &element_size);
        /* We have an edge, so fill in the optimal size and block count from the edge.
         */
        fbe_raid_geometry_get_data_disks(&raid_group_p->geo, &data_disks);
        stripe_size = data_disks * element_size;

        /* Optimal block alignment is the max of either the optimum block size or the
         * stripe size. 
         */
        negotiate_block_size_p->optimum_block_alignment = 
        FBE_MAX(raid_group_optimal_block_size, stripe_size);

        negotiate_block_size_p->block_count = edge_p->capacity;
    }
    else
    {
        negotiate_block_size_p->block_count = capacity;
        negotiate_block_size_p->optimum_block_alignment = 0;
    }
    negotiate_block_size_p->optimum_block_size = raid_group_optimal_block_size;
    negotiate_block_size_p->block_size = FBE_BE_BYTES_PER_BLOCK;
    negotiate_block_size_p->physical_block_size = FBE_BE_BYTES_PER_BLOCK;
    negotiate_block_size_p->physical_optimum_block_size = 1;

    /* Transport status is set based on the status returned from calculating.
     */
    fbe_transport_set_status(packet_p, status, 0);

    if (status != FBE_STATUS_OK)
    {
        fbe_block_edge_t *edge_p = NULL;

        fbe_base_config_get_block_edge(&raid_group_p->base_config,
                                       &edge_p,
                                       0);

        /* The only way this can fail is if the combination of block sizes are
         * not supported. 
         */
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "raid group: negotiate invalid block size. Geometry:0x%x req:0x%x "
                              "req opt size:0x%x\n",
                              edge_p->block_edge_geometry, 
                              negotiate_block_size_p->requested_block_size,
                              negotiate_block_size_p->requested_optimum_block_size);
    }
    else
    {
        /* Negotiate was successful.
         */
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/* end fbe_raid_group_negotiate_block_size */

/*!***************************************************************
 * fbe_raid_group_initiate_lun_verify()
 ****************************************************************
 * @brief
 *
 * @param raid_group_p - The raid group handle.
 * @param packet_p - The mgmt packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  04/26/12 - Created. MFerson
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_initiate_verify(fbe_raid_group_t * const raid_group_p, 
                               fbe_packet_t * const packet_p)
{
    fbe_status_t                                status;
    fbe_chunk_index_t                           chunk_index;        
    fbe_chunk_count_t                           chunk_count;        
    fbe_payload_ex_t *                          payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *             block_operation_p = NULL;
    fbe_raid_verify_type_t                      verify_type;
    fbe_raid_geometry_t *                       raid_geometry_p = NULL;
    fbe_raid_group_type_t                       raid_type;
    fbe_object_id_t object_id;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    if((((fbe_base_object_t *)raid_group_p)->class_id == FBE_CLASS_ID_STRIPER) 
       && (raid_type == FBE_RAID_GROUP_TYPE_RAID10))
    {
        status = fbe_striper_send_command_to_mirror_objects(
            packet_p, (fbe_base_object_t *)raid_group_p, raid_geometry_p);

        return status;
    }

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    switch(block_operation_p->block_opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
            verify_type = FBE_RAID_VERIFY_TYPE_USER_RO;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            verify_type = FBE_RAID_VERIFY_TYPE_USER_RW;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
            verify_type = FBE_RAID_VERIFY_TYPE_SYSTEM;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
            verify_type = FBE_RAID_VERIFY_TYPE_ERROR;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
            verify_type = FBE_RAID_VERIFY_TYPE_INCOMPLETE_WRITE;
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s:Invalid Opcode %d\n", __FUNCTION__, block_operation_p->block_opcode);
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s request type: %d op: %d\n", 
                          __FUNCTION__, verify_type, block_operation_p->block_opcode);

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
    // Mark the calculated range of chunks as needing to be verified
    status = fbe_raid_group_verify_process_verify_request(packet_p,
                                                         raid_group_p, 
                                                         chunk_index, 
                                                         verify_type, 
                                                         chunk_count);

    return status;
}
/* end fbe_raid_group_initiate_lun_verify */
/*!***************************************************************************
 * fbe_raid_group_setup_monitor_packet()
 *****************************************************************************
 *
 * @brief   This function builds a block operation in the suplied monitor
 *  packet and sends the packet to the RAID executor.
 *
 * @param raid_group_p   - Pointer to the raid group
 * @param packet_p       - Pointer to the packet
 * @param params_p - Set of parameters of monitor operation.
 *
 * @return fbe_status_t 
 *
 * @author
 *  1/10/2014 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_setup_monitor_packet(fbe_raid_group_t*  raid_group_p, 
                                                 fbe_packet_t*      packet_p,
                                                 fbe_raid_group_monitor_packet_params_t *params_p)
{
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_block_size_t                        optimal_block_size;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_raid_verify_error_counts_t*         verify_errors_p = NULL;
    fbe_status_t                            status;
    fbe_raid_geometry_t*                    raid_geometry_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);

    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);

    fbe_payload_block_build_operation(block_operation_p,
                                      params_p->opcode,
                                      params_p->lba,
                                      params_p->block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimal_block_size,
                                      NULL);
    if (params_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) {
        fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP);
    }
    if (params_p->flags != FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_INVALID) {
        fbe_payload_block_set_flag(block_operation_p, params_p->flags);
    }
    // Setup a pointer in the payload to the raid object's verify error counters.
    // Check for flush operation to determine counters to use
    if (params_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH) {
        verify_errors_p = fbe_raid_group_get_verify_error_counters_ptr(raid_group_p);
    } else {
        verify_errors_p = (fbe_raid_verify_error_counts_t*)fbe_raid_group_get_flush_error_counters_ptr(raid_group_p);
    }
    fbe_payload_ex_set_verify_error_count_ptr(payload_p, verify_errors_p);

    fbe_zero_memory(verify_errors_p, sizeof(fbe_raid_verify_error_counts_t));

    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s monitor I/O operation failed, status: 0x%X\n",
                              __FUNCTION__, status);
        fbe_transport_complete_packet(packet_p);
        fbe_transport_set_status(packet_p, status, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p)) {
        /* Raw mirror does background ops in quiesce, so we must set these bits to
         * allow the operation to make it through to the object. 
         */
        fbe_transport_set_packet_attr(packet_p, (FBE_PACKET_FLAG_DO_NOT_QUIESCE | FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK));
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_setup_monitor_packet()
 **************************************/

/*!***************************************************************************
 * fbe_raid_group_executor_send_monitor_packet()
 *****************************************************************************
 *
 * @brief   This function builds a block operation in the suplied monitor
 *  packet and sends the packet to the RAID executor.
 *
 * @param raid_group_p   - Pointer to the raid group
 * @param packet_p       - Pointer to the packet
 * @param opcode         - Block operation code
 * @param start_lba      - The block's start address
 * @param block_count    - The block count
 *
 * @return fbe_status_t 
 *
 * @author
 *  1/10/2014 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_executor_send_monitor_packet(fbe_raid_group_t*  raid_group_p, 
                                                         fbe_packet_t*      packet_p,
                                                         fbe_payload_block_operation_opcode_t opcode,
                                                         fbe_lba_t          start_lba,
                                                         fbe_block_count_t  block_count)
{
    fbe_raid_group_monitor_packet_params_t params;
    fbe_raid_group_setup_monitor_params(&params, opcode, start_lba, block_count, 
                                        FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_INVALID);

    fbe_raid_group_setup_monitor_packet(raid_group_p, packet_p, &params);

    /* There are some cases where we are using the monitor context.
     * In those cases it is ok to send the I/O immediately.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_executor_send_monitor_packet()
 **************************************/

/*!*******************************************************************************
 *          fbe_raid_group_executor_break_context_send_monitor_packet_completion()
 *********************************************************************************
 *
 * @brief   We have broken the context and are now running in the block transport
 *          context.  Now we can send the request downstream.
 *
 * @param   packet_p - Pointer to packet that completed (from block transport)
 * @param   context - Pointer to completion context information
 *
 * @return  fbe_status_t 
 *
 * @author
 *  02/08/2012  Ron Proulx - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_executor_break_context_send_monitor_packet_completion(fbe_packet_t *packet_p, 
                                                                                         fbe_packet_completion_context_t context)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_group_t   *raid_group_p;
    fbe_status_t        packet_status;

    /* Cast the context to a pointer to a raid group object.
     */ 
    raid_group_p = (fbe_raid_group_t *)context;

    /* Get the status of the operation that just completed.  We expect the
     * status to be `invalid' since it was never started.
     */
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "raid_group: break context monitor I/O failed, status: 0x%x\n",
                              packet_status);
        fbe_transport_complete_packet(packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now send the packet downstream.
     */
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
//  End fbe_raid_group_executor_break_context_send_monitor_packet_completion()

/*!***************************************************************************
 *          fbe_raid_group_executor_break_context_send_monitor_packet()
 *****************************************************************************
 *
 * @brief   This function builds a block operation in the suplied monitor
 *          packet and sends the packet to the RAID executor.
 *
 * @param   in_raid_group_p   - Pointer to the raid group
 * @param   in_packet_p       - Pointer to the packet
 * @param   b_queue_to_block_transport - FBE_TRUE - Must break context by
 *                              queuing packet to block transport.
 * @param   in_opcode         - Block operation code
 * @param   in_start_lba      - The block's start address
 * @param   in_block_count    - The block count
 *
 * @return  fbe_status_t 
 *
 * @author
 *  09/30/2009 - Created. MEV
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_executor_break_context_send_monitor_packet(fbe_raid_group_t*  in_raid_group_p, 
                                                                       fbe_packet_t*      in_packet_p,
                                                                       fbe_payload_block_operation_opcode_t     in_opcode,
                                                                       fbe_lba_t          in_start_lba,
                                                                       fbe_block_count_t  in_block_count)
{
    fbe_payload_ex_t               *payload_p;
    fbe_block_size_t                optimal_block_size;
    fbe_payload_block_operation_t  *block_operation_p;
    fbe_raid_verify_error_counts_t *verify_errors_p;
    fbe_status_t                    status;
    fbe_raid_geometry_t            *raid_geometry_p;
    fbe_queue_head_t                raid_group_packet_queue;


    // Get the SEP payload
    payload_p = fbe_transport_get_payload_ex(in_packet_p);

    // Reset the packet status to invalid
    fbe_transport_set_status(in_packet_p, FBE_STATUS_INVALID, 0);

    // Setup the block operation in the packet.
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);

    fbe_payload_block_build_operation(block_operation_p,
                                      in_opcode,
                                      in_start_lba,
                                      in_block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimal_block_size,
                                      NULL);

    // Setup a pointer in the payload to the raid object's verify error counters.
    verify_errors_p = fbe_raid_group_get_verify_error_counters_ptr(in_raid_group_p);
    fbe_payload_ex_set_verify_error_count_ptr(payload_p, verify_errors_p);

    // Clear the error counts
    fbe_zero_memory(verify_errors_p, sizeof(fbe_raid_verify_error_counts_t));

    // Increment the operation level
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "raid_group: monitor I/O block operation setup failed, status: 0x%x\n",
                              status);
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        /* Raw mirror does background ops in quiesce, so we must set these bits to
         * allow the operation to make it through to the object. 
         */
        fbe_transport_set_packet_attr(in_packet_p, (FBE_PACKET_FLAG_DO_NOT_QUIESCE | FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK));
    }

    /* In most cases it is the event thread (since we have asked for permission)
     * that this method is invoked on.  Since we don't want to tie up the event
     * thread while the I/O is executed, we must queue the packet to the block
     * transport.
     */
    fbe_queue_init(&raid_group_packet_queue);
    if (fbe_queue_is_element_on_queue(&in_packet_p->queue_element)){
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                                FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s packet %p already on the queue.\n", __FUNCTION__, in_packet_p);
    }
    fbe_queue_push(&raid_group_packet_queue, &in_packet_p->queue_element);
    fbe_transport_run_queue_push(&raid_group_packet_queue, 
                                 fbe_raid_group_executor_break_context_send_monitor_packet_completion,
                                 (fbe_packet_completion_context_t)in_raid_group_p);

	fbe_queue_destroy(&raid_group_packet_queue);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
//  End fbe_raid_group_executor_break_context_send_monitor_packet()

/*!****************************************************************************
 * fbe_raid_group_executor_handle_metadata_error()
 ******************************************************************************
 * @brief
 *   This function will handle an error from a metadata read or write that occurs
 *   in the executor (I/O path).  
 *
 *   If the I/O which had the failure was initiated by the monitor or by the meta-
 *   data service (ie. an I/O to a metadata LBA), the I/O will be failed.  The 
 *   initiator will handle the failure, by retrying or returning an error to its  
 *   caller. 
 *
 *   Also, if the metadata error indicates that the I/O can not be retried, then
 *   the I/O will be failed by this function. 
 *
 *   In all other cases, the function will set the I/O in a waiting state so 
 *   that it can be restarted at a later time.  The function to call to restart
 *   it is passed in as a parameter. 
 *
 * @notes - When this function is called, the packet is expected to be off the 
 *   terminator queue.  The function will add it to the terminator queue.  
 *
 * @param raid_group_p       - pointer to the raid group
 * @param packet_p           - packet that had an error when
 *                                reading/writing the paged MD
 * @param retry_function_p   - pointer to function to call in order to retry 
 *                                the metatdata operation / restart the I/O
 * @param packet_failed_b_p - pointer to output data that gets set to TRUE
 *                                if the packet has been failed; FALSE if the
 *                                packet should proceed in the calling function
 * @param packet_status      - Packet status equivalent of metadata element status.
 *
 * @return fbe_status_t
 *
 * @author
 *  09/01/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_executor_handle_metadata_error(fbe_raid_group_t *raid_group_p,
                                              fbe_packet_t *packet_p,
                                              fbe_raid_iots_state_t retry_function_p, 
                                              fbe_bool_t *packet_failed_b_p,
                                              fbe_status_t packet_status)
{
    fbe_payload_ex_t*                      sep_payload_p;    // pointer to sep payload
    fbe_raid_iots_t*                        iots_p;    // pointer to the IOTS of the I/O
    fbe_lba_t                               lba;    // start LBA of I/O
    fbe_payload_block_operation_opcode_t    opcode;    // opcode of the I/O
    fbe_raid_geometry_t*                    raid_geometry_p;    // pointer to raid geometry
    fbe_bool_t                              is_monitor_io_b;    // true if I/O was initiated by monitor
    fbe_bool_t                              is_paged_metadata_lba_b;    // true if 'parent' I/O is for paged MD 
    fbe_bool_t                              is_retryable_b;    // true if error indicates I/O is retryable

    //  Initialize the output parameter that the packet is not being failed
    *packet_failed_b_p = FALSE;

    //  Get the IOTS for this I/O
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    //  Get the LBA, block count, and opcode from the iots 
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    //  Determine if the "parent" I/O was to a paged metadata LBA
    is_paged_metadata_lba_b = FBE_FALSE; 
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba))
    {
        is_paged_metadata_lba_b = FBE_TRUE; 
    }

    //  Determine if this is a monitor-initiated I/O (a rebuild, verify or Flush) 
    is_monitor_io_b = fbe_transport_is_monitor_packet(packet_p, raid_geometry_p->object_id); 

    //  Determine if the metadata error is retryable or not 
    is_retryable_b = FBE_FALSE; 

    if (packet_status == FBE_STATUS_IO_FAILED_RETRYABLE)
    {
        is_retryable_b = FBE_TRUE; 
    }

    //  Fail the I/O if it was from the monitor, or an I/O to the paged metadata, or the metadata error is not
    //  retryable
    if ((is_monitor_io_b == FBE_TRUE) || 
        (is_paged_metadata_lba_b == FBE_TRUE) || 
        (is_retryable_b == FBE_FALSE) ||
        fbe_transport_is_packet_cancelled(packet_p) ||
        (packet_p->packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE))
    {
        //  Fail the I/O 
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, "rg_exec_handle_mdata_err failing I/O for packet 0x%p\n", packet_p);

        //  Set the packet status values
        if ((packet_status == FBE_STATUS_CANCEL_PENDING) || 
            (packet_status == FBE_STATUS_CANCELED) ||
            fbe_transport_is_packet_cancelled(packet_p))
        {
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
        }
        else
        {
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        }

        //  Cleanup the iots.  The call made here expects that the packet is on the terminator queue. 
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_iots_cleanup(packet_p, raid_group_p);

        //  Set the output parameter and return success  
        *packet_failed_b_p = TRUE;
        return FBE_STATUS_OK;
    }

    //! @todo - For now, if a retry function pointer has not been provided, then we just return here and let 
    //  the caller ride through the error.  The packet is not put on the terminator queue in this case.
    if (retry_function_p == NULL)
    {
        return FBE_STATUS_OK;
    }
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED)){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_exec_hndle_md_err nr for iots %p mem aborted com_fl: 0x%x\n", 
                              iots_p, iots_p->common.flags);
        fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_RESTART);
    }
    //  The I/O is a host/user I/O.  Set it up to be retried later.    
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "rg_exec_handle_mdata_err queuing packet 0x%p iots 0x%p for retry\n", packet_p, iots_p);

    //  Set all the packet status values to success 
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    //  Set up info so that the IOTS can be restarted 
    fbe_raid_iots_transition_quiesced(iots_p, retry_function_p, raid_group_p);

    //  Put the IOTS back on the terminator queue 
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    //  Return success
    return FBE_STATUS_OK;

}
/**************************************
 * end fbe_raid_group_executor_handle_metadata_error()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_retry_get_chunk_info_for_start_iots()
 ******************************************************************************
 * @brief
 *   This function will retry getting the chunk information for starting an 
 *   iots if the original metadata operation failed.  It is called after the  
 *   monitor has determined it is ready to re-try the operation.  
 *
 * @notes
 *   When this function is called, the packet is on the terminator queue. 

 * @param in_iots_p                 - pointer to the IOTS 
 *
 * @return fbe_raid_state_status_t  - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  09/02/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_raid_state_status_t fbe_raid_group_retry_get_chunk_info_for_start_iots(
                                                                                 fbe_raid_iots_t*            in_iots_p)
{
    fbe_raid_group_t*                       raid_group_p;    // pointer to raid group object 
    fbe_packet_t*                           packet_p;    // pointer to the packet 


    //  Get the raid group from where we have stored it in the iots 
    raid_group_p = (fbe_raid_group_t*) in_iots_p->callback_context; 

    //  Get the packet pointer from the iots
    packet_p = fbe_raid_iots_get_packet(in_iots_p);

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "rg_retry_chunk_info_4_start_iots - called for packet 0x%p iots 0x%p\n", 
                          packet_p, in_iots_p);

    //  Restore the iots callback function
    fbe_raid_iots_set_callback(in_iots_p, fbe_raid_group_iots_completion, raid_group_p);

    //  Unquiesce the iots 
    fbe_raid_iots_mark_unquiesced(in_iots_p);

    /* Restart from a point where we will re-evaluate the rebuild logging bitmask.
     */
    fbe_raid_group_start_iots_with_lock(raid_group_p, in_iots_p);

    //  Return a raid library status that we are waiting
    return FBE_RAID_STATE_STATUS_WAITING; 

}
/**************************************
 * end fbe_raid_group_retry_get_chunk_info_for_start_iots()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_retry_mark_nr_for_iots()
 ******************************************************************************
 * @brief
 *   This function will retry marking NR on an iots if the original metadata 
 *   operation failed.  It is called after the monitor has determined it is 
 *   ready to re-try the operation.  
 *
 * @notes
 *   When this function is called, the packet is on the terminator queue. 
 *
 * @param in_iots_p                 - pointer to the IOTS 
 *
 * @return fbe_raid_state_status_t  - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  09/07/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_raid_state_status_t fbe_raid_group_retry_mark_nr_for_iots(
                                                                    fbe_raid_iots_t*            in_iots_p)
{
    fbe_raid_group_t*                       raid_group_p;    // pointer to raid group object 
    fbe_packet_t*                           packet_p;    // pointer to the packet 


    //  Get the raid group from where we have stored it in the iots 
    raid_group_p = (fbe_raid_group_t*) in_iots_p->callback_context; 

    //  Get the packet pointer from the iots
    packet_p = fbe_raid_iots_get_packet(in_iots_p);

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - called for packet 0x%p iots 0x%p\n", __FUNCTION__, 
                          packet_p, in_iots_p);
    /* we are about to do metadata operation, we need to make sure there's no outstanding IO */
    if (!fbe_raid_iots_is_flag_set(in_iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - iots 0x%p not quiesced,  packet 0x%p\n", __FUNCTION__, in_iots_p, packet_p); 
    }

    //  Unquiesce the iots 
    fbe_raid_iots_mark_unquiesced(in_iots_p);

    //  The iots callback function was not set before we set it to NULL when queuing this packet, so it does not
    //  need to be restored 

    //  Call back into the state machine at the function which will evaluate if it needs to mark NR and if so, 
    //  it will mark it.  We have already acquired the lock and have not released it. 
    //
    //  We do not need to set a completion function here since the function below will set it itself. 
    fbe_raid_group_start_iots_with_lock(raid_group_p, in_iots_p);

    //  Return a raid library status that we are waiting
    return FBE_RAID_STATE_STATUS_WAITING; 

}
/**************************************
 * end fbe_raid_group_retry_mark_nr_for_iots
 **************************************/

/*!**************************************************************
 * fbe_raid_group_mark_nr_control_operation()
 ****************************************************************
 * @brief
 *  This function marks specified region for NR using control operation.
 * 
 * @param raid_group_p
 * @param packet_p
 * @param completion_fn
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/12/2012 - Created. Prahlad Purohit (using refactored code).
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_mark_nr_control_operation(fbe_raid_group_t*  raid_group_p,
                                                      fbe_packet_t*      packet_p,
                                                      fbe_packet_completion_function_t completion_fn)
{
    fbe_lba_t                                   start_lba;
    fbe_block_count_t                           block_count;
    fbe_status_t                                status;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p = NULL;
    fbe_bool_t                                  is_span_across_user_data_b;     
    fbe_lba_t                                   exported_disk_capacity;
    fbe_lba_t                                   end_lba;                        
    fbe_chunk_index_t                           start_chunk;                    
    fbe_chunk_index_t                           end_chunk;                      
    fbe_chunk_count_t                           chunk_count;
    fbe_raid_group_paged_metadata_t             chunk_data;
    fbe_raid_geometry_t                         *raid_geometry_p;
    fbe_lba_t                                   rg_lba;
    fbe_u16_t                                   disk_count;
    fbe_raid_position_bitmask_t                 rl_bitmask = 0;

    // This completion function will release the stripe lock that we acquired
    fbe_transport_set_completion_function(packet_p, completion_fn, raid_group_p);
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    //  Get the control buffer from the packet payload
    status = fbe_raid_group_rebuild_get_needs_rebuild_context(raid_group_p, packet_p, &needs_rebuild_context_p);       
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                         "raid_group: mark nr rl mask: 0x%x start: 0x%llx blks: 0x%llx nr mask: 0x%x\n",
                         rl_bitmask, 
                         needs_rebuild_context_p->start_lba, needs_rebuild_context_p->block_count,
                         needs_rebuild_context_p->nr_position_mask);

    //  Determine if lba range span across the user data boundary
    fbe_raid_group_bitmap_determine_if_lba_range_span_user_and_paged_metadata(raid_group_p,
                needs_rebuild_context_p->start_lba, needs_rebuild_context_p->block_count, &is_span_across_user_data_b);
    
    if(is_span_across_user_data_b)
    {
        //  Get the number of chunks per disk in the user RG data area (exported area)
        exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
        block_count = (fbe_block_count_t) (exported_disk_capacity - needs_rebuild_context_p->start_lba);        
    }
    else
    {
        block_count = needs_rebuild_context_p->block_count;
    }

    start_lba = needs_rebuild_context_p->start_lba;
    end_lba = start_lba + block_count - 1;
    
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, start_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk);
    
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);

    //  Set up the bits of the metadata that need to be written, which are the NR bits 
    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_data.needs_rebuild_bits = needs_rebuild_context_p->nr_position_mask;

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_mark_nr_control_operation_completion,
                (fbe_packet_completion_context_t) raid_group_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &disk_count);

    rg_lba = start_lba * disk_count;    

    if(fbe_raid_geometry_is_metadata_io(raid_geometry_p, rg_lba) ||
       !fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(raid_group_p, packet_p, start_chunk,
                                                              chunk_count, &chunk_data, FBE_FALSE);
    }
    else
    {
        //  The chunks are in the user data area.  Use the paged metadata service to update them. 
        fbe_raid_group_bitmap_update_paged_metadata(raid_group_p, packet_p, start_chunk,
                                                    chunk_count, &chunk_data, FBE_FALSE);
    }
 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*******************************************
 * end fbe_raid_group_get_info_and_mark_nr()
 *******************************************/


/*!**************************************************************
 * fbe_raid_group_mark_nr_control_operation_completion()
 ****************************************************************
 * @brief
 *  Completion function for mark NR using control operation. Releases metadta
 *  operation and sets appropriate control status.
 * 
 * @param packet_p
 * @param context
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/12/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_mark_nr_control_operation_completion(fbe_packet_t * packet_p, 
                                                    fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t                        *sep_payload_p = NULL;
    fbe_payload_control_operation_t         *control_operation_p = NULL;
    fbe_payload_control_status_t            control_status;
    fbe_status_t                            packet_status;

    //  Get the status of the metadata operation 
    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);

    packet_status = fbe_transport_get_status_code(packet_p);
    if (FBE_STATUS_OK == packet_status)
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_payload_control_set_status(control_operation_p, control_status);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_raid_group_start_direct_io_update_performance_stats()
 ****************************************************************
 * @brief
 *  This function will update the performance statistics
 *  for direct reads/writes.
 * 
 * @param packet_p - Pointer to packet
 * @param position - position in the RAID group
 * @param blocks - blocks of trnasfer
 * @param opcode - read or write
 *
 * @return fbe_status_t - Always return FBE_STATUS_OK
 *
 * @author
 *  02/23/2012 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_start_direct_io_update_performance_stats(fbe_packet_t *packet_p, 
                                                                            fbe_raid_position_t position,
                                                                            fbe_block_count_t blocks,
                                                                            fbe_payload_block_operation_opcode_t opcode)
{   fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;
    fbe_lun_performance_counters_t  *performance_stats_p = NULL;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);

    fbe_payload_ex_get_lun_performace_stats_ptr(payload_p, (void **)&performance_stats_p);
    if (performance_stats_p != NULL)
    {
        fbe_transport_get_cpu_id(packet_p, &cpu_id);
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        {	
            fbe_raid_perf_stats_inc_disk_reads(performance_stats_p, position, cpu_id);
            fbe_raid_perf_stats_inc_disk_blocks_read(performance_stats_p, position, blocks, cpu_id);
        }
        else
        {
            fbe_raid_perf_stats_inc_disk_writes(performance_stats_p, position, cpu_id);
            fbe_raid_perf_stats_inc_disk_blocks_written(performance_stats_p, position, blocks, cpu_id);
        }
        //timestamp
        fbe_raid_perf_stats_set_timestamp(performance_stats_p);
    }
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_raid_group_start_direct_io_update_performance_stats()
 ***************************************************************/

/*************************
 * end file fbe_raid_group_executer.c
 *************************/
