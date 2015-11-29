/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_utils.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *  02/22/2012  Ron Proulx  - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_raid_library.h"
#include "fbe_transport_memory.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************************
 *          fbe_raid_group_handle_transport_error()
 *****************************************************************************
 *
 * @brief   Increments the appropriate counter in the verify counter structure.
 *
 * @param   raid_group_p - Pointer to raid group that encountered error
 * @param   transport_status - Transport status
 * @param   transport_qualifier - Transport qualifier
 * @param   block_status_p - Pointer to block status to update
 * @param   block_qualifier_p - Pointer to block qualifier to update  
 * @param   error_counters_p - Pointer to error counter structure
 * 
 * @return  status
 *
 * @author
 *  02/22/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_handle_transport_error(fbe_raid_group_t *raid_group_p,
                                                   fbe_status_t transport_status,
                                                   fbe_u32_t transport_qualifier,
                                                   fbe_payload_block_operation_status_t *block_status_p,
                                                   fbe_payload_block_operation_qualifier_t *block_qualifier_p,
                                                   fbe_raid_verify_error_counts_t *error_counters_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Initialize the block status and qualifier to invalid.
     */
    *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;

    /* Now switch on the transport status.
     */
    switch(transport_status)
    {
         case FBE_STATUS_OK:
            /* No transport error. 
             */
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
            break;

        case FBE_STATUS_IO_FAILED_RETRYABLE:
            /* Retryable
             */
            status = FBE_STATUS_IO_FAILED_RETRYABLE;
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            error_counters_p->retryable_errors++;
            break;

        case FBE_STATUS_CANCEL_PENDING:
        case FBE_STATUS_CANCELED:
            /* Canceled signifies the I/O has been aborted.  Currently we do not
             * update the error counters but we may need to update the I/O
             * statistics.  Return `non-retryable' since the client has aborted
             * the request.
             */
            status = FBE_STATUS_CANCELED;
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED;
            break;

        case FBE_STATUS_BUSY:   
            /* The object is in activate or activate pending state.
             */
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            error_counters_p->retryable_errors++;
            break;

        case FBE_STATUS_DEAD:   
            /* The object is in destroy or destroy pending state.
             * Set the block status to raid group is shutting down. 
             * Set the status to `non-retryable'.
             */ 
            status = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
            error_counters_p->non_retryable_errors++;
            break;

        case FBE_STATUS_QUIESCED:   
            /* This IO, coming from a monitor operation, is rejected when the 
             * object is quiesced. This can happen when the VD is quiesced.
             */

        case FBE_STATUS_IO_FAILED_NOT_RETRYABLE:   
            /* Set the status to `non-retryable'.
             */ 
            status = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
            error_counters_p->non_retryable_errors++;
            break;

        case FBE_STATUS_FAILED:
            /* FBE_STATUS_FAILED means that the object below us drained I/O 
             * because it went to failed. We will treat this just like an 
             * other dead error. 
             */
        case FBE_STATUS_EDGE_NOT_ENABLED: 
            /* Since we might be able to use redundancy to complete the request
             * we allow the retry (by returning Ok).  Also since the raid library
             * will retry the request do not increment the `non-retryable' count.
             */
            status = FBE_STATUS_IO_FAILED_RETRYABLE;
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE;
            break;

        default:
            /* No other status is expected.
             */
            *block_status_p = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            *block_qualifier_p = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR;
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s status 0x%x unexpected.\n",
                                  __FUNCTION__, transport_status);
            return FBE_STATUS_GENERIC_FAILURE;

    }; /* end switch on transport status */

    /* Return the executuon status.
     */
    return status;
}
/**********************************************
 * end fbe_raid_group_handle_transport_error()
 **********************************************/

/*!***************************************************************************
 *          fbe_raid_group_handle_block_operation_error()
 *****************************************************************************
 *
 * @brief   Increments the appropriate counter in the verify counter structure.
 *
 * @param   raid_group_p - Pointer to raid group that encountered error
 * @param   block_status - Block operation status
 * @param   block_qualifier - Block operation qualifier
 * @param   error_counters_p - Pointer to error counter structure
 * 
 * @return  status
 *
 * @author
 *  02/22/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_handle_block_operation_error(fbe_raid_group_t *raid_group_p,
                                                                fbe_payload_block_operation_status_t block_status,
                                                                fbe_payload_block_operation_qualifier_t block_qualifier,
                                                                fbe_raid_verify_error_counts_t *error_counters_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* Increment the error counts depending on the error
     * we received on this request.
     */
    switch (block_status)
    { 
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            /* Block operation was successful but the qualifier indicates
             * that there is some action to take (i.e. retry thru the raid 
             * library).
             */
            status = FBE_STATUS_OK;
            switch(block_qualifier)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED:
                    /* This was a soft media error.
                     */
                    error_counters_p->c_soft_media_count++;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED:
                    /* These should not be encountered for raid group requests.
                     */
                default:
                    /* Else unsupported qualifier encountered.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                          FBE_TRACE_LEVEL_ERROR, 
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "raid_group: block error status 0x%x qual: 0x%x unexpected.\n",
                                          block_status, block_qualifier);
                    return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            /*! @note An I/O failed status no longer implies a non-retryable
             *        error. Look at the qualifier to determine if it is 
             *        retryable or not.
             */
            status = FBE_STATUS_IO_FAILED_RETRYABLE;
            switch(block_qualifier)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE:
                    /*! @note We have encountered a `non-retryable' (for a 
                     *        particular position) error.  Therefore we need to
                     *        increment the counter.
                     */ 
                    status = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
                    error_counters_p->non_retryable_errors++;
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED:
                    error_counters_p->retryable_errors++;
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_FULL_STRIPE:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR:
                    status = FBE_STATUS_FAILED;
                    break;
                default:
                    /* Else unsupported qualifier encountered.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                          FBE_TRACE_LEVEL_ERROR, 
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "raid_group: block error status 0x%x qual: 0x%x unexpected.\n",
                                          block_status, block_qualifier);
                    return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            /* Media error encountered.
             */
            status = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
            switch(block_qualifier)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST:
                    /* The sector was either previously invalidated or is now
                     * invalidated.  May succeed so don't increment counters.
                     */
                    break;
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_NO_REMAP:
                    /* Add the current error to the MENR bitmap.
                     * Media error no remap bitmap contains every MENR position.
                     * May succeed so don't increment counters.
                     */
                    break;
                default:
                    /* Else unsupported qualifier encountered.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                          FBE_TRACE_LEVEL_ERROR, 
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "raid_group: block error status 0x%x qual: 0x%x unexpected.\n",
                                          block_status, block_qualifier);
                    return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
    
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            /*! @note Although we may need to increment a performance 
             *        counter, this status doesn't impact the error counters.
             *        But we return `cancelled' so that we don't
             *        retry the request.
             */
            status = FBE_STATUS_CANCELED;
            switch(block_qualifier)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_OPTIONAL_ABORTED_LEGACY:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED:
                case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED:
                    /* All of the above are valid qualifiers for an aborted status.
                     */
                    break;
                default:
                    /* Else unsupported qualifier encountered.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: block error status 0x%x qual: 0x%x unexpected.\n",
                                      block_status, block_qualifier);
                    return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
    
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            /* The packet expired while it was outstanding.  Return timeout
             * since we already timed-out.
             */
            status = FBE_STATUS_TIMEOUT;
            break;
    
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
        default:
            /* Else unsupported status encountered.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: block error status 0x%x unexpected qual: 0x%x \n",
                                  block_status, block_qualifier);
            return FBE_STATUS_GENERIC_FAILURE;

    } /* end of switch on block status */
          
    /* Return the execution status.
     */
    return status;
}
/***************************************************
 * end fbe_raid_group_handle_block_operation_error()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_process_io_failure()
 ***************************************************************************** 
 * 
 * @brief   Using the packet or block status, determine if we need to update
 *          any counters etc for a failed block operation.
 * 
 * @param   raid_group_p - Pointer to raid group structure
 * @param   packet_p - Pointer to packet that failed
 * @param   failure_type - Type of failure.
 * @param   master_block_operation_pp - Address of master block operation
 *              pointer to udpate.
 * 
 * @return  status - 
 *
 * @author
 *  02/22/2012  Ron Proulx  - Created from fbe_raid_fruts_chain_get_eboard().
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_process_io_failure(fbe_raid_group_t *raid_group_p,
                                               fbe_packet_t *packet_p,
                                               fbe_raid_group_io_failure_type_t failure_type,
                                               fbe_payload_block_operation_t  **master_block_operation_pp)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_payload_ex_t               *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t  *block_operation_p = NULL;
    fbe_payload_block_operation_t  *master_block_operation_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_memory_request_t           *memory_request_p = NULL;
    fbe_raid_verify_error_counts_t  local_error_counters;
    fbe_raid_verify_error_counts_t *error_counters_p = NULL;
    fbe_status_t                    transport_status = FBE_STATUS_INVALID;
    fbe_u32_t                       transport_qualifier = 0;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;

    /* First set the master block operation to invalid.
     */
    *master_block_operation_pp = NULL;

    /* Get the transport status.
     */
    transport_status = fbe_transport_get_status_code(packet_p);
    transport_qualifier = fbe_transport_get_status_qualifier(packet_p);

    /* Now get the current block operation.  If this is a stripe lock operation
     * get the master block operation.
     */
    if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
    }
    else
    {
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        if (block_operation_p != NULL)
        {
            if (payload_p->current_operation->prev_header != NULL)
            {
                if (payload_p->current_operation->prev_header->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
                {
                    /* Get the stripe lock and then the master.
                     */
                    stripe_lock_operation_p = fbe_payload_ex_get_prev_stripe_lock_operation(payload_p);
                    master_block_operation_p = (fbe_payload_block_operation_t *)stripe_lock_operation_p->operation_header.prev_header;

                }
                else
                {
                    /* Else no stripe lock was required.
                     */
                    master_block_operation_p = fbe_payload_ex_get_prev_block_operation(payload_p);
                }
            }
            if ((master_block_operation_p == NULL)                                                                ||
                (master_block_operation_p->operation_header.payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)    )
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: process io failure master block operation is null.\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: process io failure block operation is null.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the current block status and opcode.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* Now that we have the current block operation status, set the return
     * master block operation pointer.
     */
    *master_block_operation_pp = master_block_operation_p;

    /*! @todo Currently the only operations that we support are:
     *          o Read
     *          o Write
     */
    if ((opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)            &&
        (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)           &&
        (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED)           &&
        (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE)    )
    {
        /* Trace an error and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: process io failure unsupported opcode: %d \n",
                              opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @todo This method currently doesn't expect the memory request to be 
     *        in use.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    if (fbe_memory_request_is_in_use(memory_request_p) == FBE_TRUE)
    {
        /* Trace an error and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: process io failure packet: 0x%p unexpected in-use memory request.\n",
                              packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Currently this method simply updates the appropriate verify error 
     *        counters.  If none are supplied we use local value to keep error
     *        handling logic simple.
     */
    fbe_payload_ex_get_verify_error_count_ptr(payload_p, (void **)&error_counters_p);
    if (error_counters_p == NULL)
    {
        /*! @note Although this waste some stack space we need to validate the 
         *        I/O status below and this make the processing simpler.
         */
        error_counters_p = &local_error_counters;
        fbe_zero_memory(error_counters_p, sizeof(*error_counters_p));
    }

    /* If it is a transport failure trace some information.
     */
    switch(failure_type)
    {
        case FBE_RAID_GROUP_IO_FAILURE_TYPE_TRANSPORT:
            /* Invoke method to handle transport errors.  This will
             * update the error counters as required.
             */
            status = fbe_raid_group_handle_transport_error(raid_group_p,
                                                           transport_status,
                                                           transport_qualifier,
                                                           &block_status,
                                                           &block_qualifier,
                                                           error_counters_p);

            /* If the status is not success or generic failure we need to
             * populate the master block operation status and qualifier since 
             * we will be completing this request.
             */
            switch(status)
            {
                case FBE_STATUS_OK:
                    /* Allow retry.
                     */
                    break;
                case FBE_STATUS_IO_FAILED_RETRYABLE:
                    /* Allow retry.
                     */
                    status = FBE_STATUS_OK;
                    break;
                case FBE_STATUS_CANCELED:
                    /* Request aborted by client.
                     */
                    fbe_payload_block_set_status(master_block_operation_p,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, 
                                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
                    break;
                case FBE_STATUS_IO_FAILED_NOT_RETRYABLE:
                    /* Raid group is going shutdown.
                     */
                     fbe_payload_block_set_status(master_block_operation_p,
                                                  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
                     break;
                default:
                    /* Logic error.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: process io failure unsupported transport status: 0x%x\n",
                                  status);
                    return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_RAID_GROUP_IO_FAILURE_TYPE_BLOCK_OPERATION:
            /* Invoke the method to process the block status.
             */
            status = fbe_raid_group_handle_block_operation_error(raid_group_p,
                                                                 block_status,
                                                                 block_qualifier,
                                                                 error_counters_p);

            /* If the status is not success or generic failure we need to
             * populate the master block operation status and qualifier since 
             * we will be completing this request.
             */
            switch(status)
            {
                case FBE_STATUS_OK:
                    /* Allow retry.
                     */
                    break;
                case FBE_STATUS_IO_FAILED_RETRYABLE:
                case FBE_STATUS_IO_FAILED_NOT_RETRYABLE:
                    /*! @note We do not know if the request maybe be completed using 
                     *        redundancy so return Ok so that the request is retried 
                     *        using the raid library.  Since it may succeed we cannot
                     *        mark the request as uncorrectable media error.
                     */ 
                    /* Allow retry.
                     */
                    status = FBE_STATUS_OK;
                    break;
                case FBE_STATUS_FAILED:
                case FBE_STATUS_CANCELED:
                    /* Request aborted by client.
                     */
                    // Fall thru
                case FBE_STATUS_TIMEOUT:
                    /* Request took too long.  Don't retry.
                     */
                    /* Copy the block status from the failed request into the
                     * master packet.
                     */
                     fbe_payload_block_set_status(master_block_operation_p,
                                                  block_status,
                                                  block_qualifier);
                     break;
                default:
                    /* Logic error.
                     */
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group:process io failure unsupported blk err status:0x%x, set blk status to INVALID_REQUEST\n",
                                  status);

					fbe_payload_block_set_status(master_block_operation_p,
												 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
												 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

                    return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_RAID_GROUP_IO_FAILURE_TYPE_CHECKSUM_ERROR:
            /*! @note Increment counters associated with checksum error. 
             *        Since we will be retrying this request thru the library
             *        we always mark it as a `correctable' error.
             */
            error_counters_p->c_crc_count++;
            break;

        case FBE_RAID_GROUP_IO_FAILURE_TYPE_STRIPE_LOCK:
            /* Nothing to do for a stripe lock failure.
             */
            break;

        default:
            /* Unsupported failure type.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: process io failure unsupported failure type: %d\n",
                                  failure_type);
            return FBE_STATUS_GENERIC_FAILURE;

    } /* end switch on failure type */

    /* Return success.
     */
    return status;
}
/******************************************
 * end fbe_raid_group_process_io_failure()
 ******************************************/


/*!****************************************************************************
 * fbe_raid_group_utils_is_sys_rg()
 ******************************************************************************
 * @brief
 *  Check whether the raid group is a system RG or not
 *
 * @param raid_group_p
 * @param sys_rg
 * 
 * @return fbe_status_t
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_utils_is_sys_rg(fbe_raid_group_t*   raid_group_p,
                                            fbe_bool_t*         sys_rg)
{
    fbe_object_id_t              object_id;


    object_id = fbe_raid_group_get_object_id(raid_group_p);
    *sys_rg = fbe_database_is_object_system_rg(object_id);
    
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_raid_group_utils_is_sys_rg()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_util_is_raid_group_in_use()
 *****************************************************************************
 *
 * @brief   This function determinbe if the raid group is `in-use'.. In use
 *          means that (1) or more LUs are bound on this raid group.   
 *
 * @param   raid_group_p   - Pointer to the raid group
 * @param   b_is_raid_group_in_use_p - Address of `in-use' variable to update.
 *
 * @return fbe_status_t 
 *
 *
 * @todo: remove this function once support for LUN data events is added. 
 *
 * @author
 *  05/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_util_is_raid_group_in_use(fbe_raid_group_t *raid_group_p,
                                                      fbe_bool_t *b_is_raid_group_in_use_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_block_transport_server_t   *block_transport_server_p = NULL;
    fbe_edge_index_t                client_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                 client_object_id = FBE_OBJECT_ID_INVALID;
   
    /* By default we assume there is an upstream object.
     */
    *b_is_raid_group_in_use_p = FBE_TRUE;

    /* Get the pointer to the RAID block transport server.
     */
    block_transport_server_p = &raid_group_p->base_config.block_transport_server;

    /* Now get client id and index (if any).
     */
    status = fbe_block_transport_find_first_upstream_edge_index_and_obj_id(block_transport_server_p, 
                                                                           &client_index, 
                                                                           &client_object_id);
    if (status != FBE_STATUS_OK)
    {
        /* Generate an error trace and report that there is not a upstream object.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get upstream object. status: 0x%x \n",
                              __FUNCTION__, status);
        *b_is_raid_group_in_use_p = FBE_FALSE;
        return status;
    }

    /* If there is no object located trace an informational message.
     */
    if (client_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Trace an informational message and return False.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: is in use is False. \n");
        *b_is_raid_group_in_use_p = FBE_FALSE;
    }

    return FBE_STATUS_OK;

}
/*************************************************
 * end fbe_raid_group_util_is_raid_group_in_use()
 *************************************************/
 
/*!****************************************************************************
 * fbe_raid_group_utils_get_block_operation_error_precedece()
 ******************************************************************************
 * @brief
 *  This function determines how to rate the block status error.
 *
 * @param   status               - block operation status
 *
 * @return
 *  A number with a relative precedence value.
 *  This number may be compared with other precedence values from this
 *  function.
 *
 * @author
 *   06/18/2012 - Prahlad Purohit - Created using simillar function in PVD & RAID Lib. 
 *
 ******************************************************************************/
fbe_raid_group_error_precedence_t 
fbe_raid_group_utils_get_block_operation_error_precedece(fbe_payload_block_operation_status_t status)
{

    fbe_raid_group_error_precedence_t priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_NO_ERROR;

    /* Determine the priority of this error
     * Certain errors are more significant than others.
     */
    switch (status)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS:
            /* Set precedence.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_NO_ERROR;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID:
            /* The only way this could occur is if the I/O was never
             * executed.  Assumption is that it is retryable.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_INVALID_REQUEST;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
            /* Request was not properly formed, etc. Retryable.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_INVALID_REQUEST;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
            /* Set precedence.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_MEDIA_ERROR;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            /* With support of drive spin-down this may occur and depending
             * on the situation (i.e. if there is redundancy etc.) the I/O
             * may be retried.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_NOT_READY;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
            /* The request was purposfully aborted.  It is up to the
             * originator to retry the request.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_ABORTED;
            break;


        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_TIMEOUT:
            /* May not want to retry this request.  Up to the client.
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_TIMEOUT;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:
            /* Device not usable therefore don't retry (at least until the
             * device is replaced).
             */
            priority = FBE_RAID_GROUP_ERROR_PRECEDENCE_DEVICE_DEAD;
            break;

        default:
            /* We don't expect this so use the highest priority error.
             */
            priority = FBE_PROVISION_DRIVE_ERROR_PRECEDENCE_UNKNOWN;
            break;
    }

    return priority;
}
/******************************************************************************
 * end fbe_raid_group_utils_get_block_operation_error_precedece()
 ******************************************************************************/


/*!****************************************************************************
 *   fbe_raid_group_monitor_io_get_consumed_info()
 ******************************************************************************
 * @brief
 *  This function determines if IO has any overlap with unconsumed area.
 *
 * @param   raid_group_p -         -IN - Pointer to raid group
 * @param   unconsumed_block_count -IN - unconsumed block count return received
 *                                       from permit event completion
 * @param   io_start_lba -          IN- IO start lba
 * @param   io_block_count -        IN/OUT - IO block count, it will be modified if
 *                                           IO has overlap unconsumed area
 * @param   is_consumed  -          OUT - to determine if IO is consumed at beginning 
 *                                        or not
 * 
 * @return  fbe status
 *
 * @author
 *   07/13/2012 - Amit Dhaduk 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_io_get_consumed_info(fbe_raid_group_t* raid_group_p, 
                                fbe_block_count_t     unconsumed_block_count,
                                fbe_lba_t io_start_lba, fbe_block_count_t* io_block_count,                                 
                                fbe_bool_t* is_consumed)
{

    fbe_lba_t                           next_consumed_lba;
    fbe_block_count_t                   logical_block_count; 
    fbe_lba_t                           logical_lba;
    fbe_block_count_t                   new_block_count = FBE_LBA_INVALID;
    fbe_raid_geometry_t*                raid_geometry_p = NULL; 
    fbe_u16_t                           data_disks;
    fbe_lba_t                           end_lba = FBE_LBA_INVALID;

    /* unconsumed area determine logic and possible cases
     * A. If IO is align with consumed area at the beginning
     *     A.1 If IO end extent is not align with consumed area
     *      - calculate the new block count which cover consumed space and return it             
     * B. If IO is not align with consumed area at the beginning
     *     - calculate the unconsume block count and return
     * C. If the raid type is _MIRROR_UNDER_STRIPER then consider unconsumed block counts
     *    which was received from event permit completion to calculate total block counts
     *    in both above cases.
     */


    /* Convert the disk base lba to logical lba */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_block_count = *io_block_count * data_disks;
    logical_lba = io_start_lba * data_disks;
    
    fbe_block_transport_server_find_next_consumed_lba(&((fbe_base_config_t *) raid_group_p)->block_transport_server,
                                                    logical_lba,
                                                    &next_consumed_lba);


    if(next_consumed_lba != FBE_LBA_INVALID)
    {
        /* next consumed lba is logical, so make it relative to a single drive. */
        next_consumed_lba = next_consumed_lba / data_disks;
    }
    else
    {
        next_consumed_lba = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    }

    if(next_consumed_lba == io_start_lba)
    {
        *is_consumed = FBE_TRUE;

        /* IO range is consumed at the beginning
         * Now check the IO end extent is align with consumed area or not 
         */

        /* for R1_0 case, mirror object consider unconsumed block counts returned by the striper
         * object from upstream. If the unconsumed_block_count returned by striper in event
         * permit completion is 0 then new block counts remain unchanged */
        if(raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
        {
            new_block_count = *io_block_count - unconsumed_block_count;    
        }
        else if(raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_SPARE)
        {
            new_block_count = *io_block_count - unconsumed_block_count;
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                                 "VD: get cns info nw bl:0x%llx old bl: 0x%llx uncons: 0x%llx\n", 
                                 new_block_count, *io_block_count, unconsumed_block_count);
        }
        else
        {
            fbe_block_transport_server_get_end_of_extent(&((fbe_base_config_t *) raid_group_p)->block_transport_server,
                                                         logical_lba, logical_block_count, &end_lba);

            if (end_lba == (logical_lba + logical_block_count - 1))
            {
                /* end lba is align with consumed area, consider the original block count */
                new_block_count = *io_block_count;

            }
            else
            {

                /* end extent has unconsumed area, recalculate the block counts */
                fbe_block_count_t new_blocks = (fbe_block_count_t)end_lba + 1 - logical_lba;

                new_block_count = new_blocks / data_disks;

                /* validate new blocks count */
                if (new_block_count > *io_block_count)
                {
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "_consumed_info:blk cnt is large, new blk 0x%llx, orig blks 0x%llx, end_lba 0x%llx\n", 
                                          (unsigned long long)new_block_count, (unsigned long long)*io_block_count, (unsigned long long)end_lba);
                    
                    new_block_count = fbe_raid_group_get_chunk_size(raid_group_p);
                }
                if(new_block_count == 0)
                {
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "_consumed_info:block count is zero, end lba 0x%llx, new blocks 0x%llx, orig blks 0x%llx \n",
                                          (unsigned long long)end_lba, (unsigned long long)new_blocks, (unsigned long long)*io_block_count);
                    return FBE_STATUS_GENERIC_FAILURE;
                }            
            }
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                                 "RG: get cns info nw bl:0x%llx lba: 0x%llx io_bl: 0x%llx uncons: 0x%llx elba:0x%llx\n", 
                                 new_block_count, io_start_lba, *io_block_count, unconsumed_block_count, end_lba);
        }

        /* update the new block count */
        if(new_block_count != *io_block_count)
        {

            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "unconsumed at end, orig blk 0x%llx, new_blk 0x%llx, unconsumed  0x%llx\n",
                *io_block_count, new_block_count, unconsumed_block_count);  

            /* update the new block counts which is consumed IO range */
            *io_block_count = new_block_count;
        }

    }
    else
    {
        /* IO has unconsumed area at the beginning
        * calculate the unconsumed block counts and return
        */
        *is_consumed = FBE_FALSE;

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "unconsumed in beginning, orig blk 0x%llx, new_blk 0x%llx, unconsumed  0x%llx\n",
                    (unsigned long long)*io_block_count, (unsigned long long)next_consumed_lba-io_start_lba, (unsigned long long)unconsumed_block_count); 

        /* for R1_0 case, mirror object consider unconsumed block counts returned by the striper
         * object from upstream object */
        if(raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
        {
            *io_block_count = unconsumed_block_count;
        }
        else
        {
            *io_block_count = next_consumed_lba - io_start_lba;
        }        

   }

     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_monitor_io_get_consumed_info()
 ******************************************************************************/

/*!****************************************************************************
 *   fbe_raid_group_monitor_io_get_unconsumed_block_counts()
 ******************************************************************************
 * @brief
 *  This function determines the next consumed LBA and return the updated block 
 *  counts for the unconsumed LBA range.
 *
 * @param   raid_group_p -         -IN  - Pointer to raid group object
 * @param   unconsumed_block_count -IN  - unconsumed block count received
 *                                       from permit event completion
 * @param   io_start_lba -          IN  - IO start lba
 * @param   updated_block_count -   OUT - IO block counts, which cover unconsumed
 *                                        LBA range
 * 
 * @return  fbe status
 *
 * @author
 *   07/16/2012 - Amit Dhaduk 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_io_get_unconsumed_block_counts(fbe_raid_group_t* raid_group_p, 
                                fbe_block_count_t     unconsumed_block_count,
                                fbe_lba_t io_start_lba, fbe_block_count_t* updated_block_count)
{

    fbe_lba_t                           next_consumed_lba;
    fbe_lba_t                           logical_lba;
    fbe_raid_geometry_t*                raid_geometry_p = NULL; 
    fbe_u16_t                           data_disks;


    /* Convert the disk base lba to logical lba */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_lba = io_start_lba * data_disks;

    if (raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) 
    {
        
        fbe_block_transport_server_find_next_consumed_lba(&((fbe_base_config_t *) raid_group_p)->block_transport_server,
                                                          logical_lba,
                                                          &next_consumed_lba);
        if(next_consumed_lba != FBE_LBA_INVALID)
        {
            // next consumed lba is logical, so make it relative to a single drive.
            next_consumed_lba = next_consumed_lba / data_disks;
        }
        else
        {
            next_consumed_lba = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
        }
        /* With a vd it is possible that we can see a block count of 0 here, so
         * simply don't set it. 
         */
        if ((next_consumed_lba - io_start_lba) != 0)
        {
            *updated_block_count = (fbe_block_count_t) (next_consumed_lba - io_start_lba);
        }
        else
        {
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                                 "RG: find next consumed returns next_consumed_lba: 0x%llx io_start_lba: 0x%llx\n",
                                 unconsumed_block_count, io_start_lba);
            *updated_block_count = unconsumed_block_count;
        }
    }
    else 
    {
        //  R10 does not need to calculate.  Just get the permit request info's block count and 
        //  assign it to the rebuild context data structure
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "RG: R10 get_unconsumed, start lba: 0x%llx unconsumed blks: 0x%llx \n",
                              io_start_lba, unconsumed_block_count);

        /* if unconsumed block count is zero, the stripper is probably gone */
       if (unconsumed_block_count == 0)
       {
            return FBE_STATUS_GENERIC_FAILURE;
       }

        *updated_block_count = unconsumed_block_count;
    }

    return FBE_STATUS_OK;
    
}
/******************************************************************************
 * end fbe_raid_group_monitor_io_get_unconsumed_block_counts()
 ******************************************************************************/

/*!****************************************************************************
 *   fbe_raid_group_monitor_update_client_blk_cnt_if_needed()
 ******************************************************************************
 * @brief
 *  This function determines if permit IO is crossing client range then return 
 *  the updated block counts so that permit request will go only to one client. 
 *  If it returns FBE_LBA_INVALID means there is no user data and caller has to
 *  do nothing.
 *
 * @param   raid_group_p -         -IN  - Pointer to raid group
 * @param   io_start_lba -          IN  - IO start lba
 * @param   io_block_count -        IN  - IO block count, 
 *                                  OUT - updated block counts to send permit request
 *                                        to only one client if IO is across clients 
 *                                        
 * 
 * @return  fbe status
 *
 * @author
 *   07/13/2012 - Amit Dhaduk 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_update_client_blk_cnt_if_needed(fbe_raid_group_t* raid_group_p, 
                                fbe_lba_t io_start_lba, fbe_block_count_t* io_block_count)
{

    fbe_block_count_t                   logical_block_count; 
    fbe_lba_t                           logical_lba;
    fbe_block_count_t                   new_block_count = FBE_LBA_INVALID;
    fbe_raid_geometry_t*                raid_geometry_p = NULL; 
    fbe_u16_t                           data_disks;
    fbe_lba_t                           end_lba = FBE_LBA_INVALID;


    /* Convert the disk base lba to logical lba */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_block_count = *io_block_count * data_disks;
    logical_lba = io_start_lba * data_disks;


    fbe_block_transport_server_get_end_of_extent(&((fbe_base_config_t *) raid_group_p)->block_transport_server,
                                                logical_lba, logical_block_count, &end_lba);

    if(end_lba == FBE_LBA_INVALID)
    {
        /* There is no user data, so return */
        return FBE_STATUS_OK;
    }

    /* check if IO range is across more than one client or has unconsumed at end */
    if (end_lba != (logical_lba + logical_block_count - 1))
    {

        /* end extent is across clients or has unconsumed area, recalculate the block counts */
        fbe_block_count_t new_blocks = (fbe_block_count_t)end_lba + 1 - logical_lba;

        new_block_count = new_blocks / data_disks;

        /* validate new blocks count */
        if (new_block_count > *io_block_count)
        {

            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "_update_client_blk_cnt: blk cnt is large, s_lba 0x%llx, orig blk 0x%llx, new blk 0x%llx, end_lba 0x%llx\n", 
                                  (unsigned long long)io_start_lba, (unsigned long long)*io_block_count, (unsigned long long)new_block_count, (unsigned long long)end_lba);

            
            return FBE_STATUS_OK;
        }
        if(new_block_count == 0)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "_update_client_blk_cnt:blk cnt is zero, s_lba 0x%llx, orig blk 0x%llx, end lba 0x%llx, new blk 0x%llx\n",
                                  (unsigned long long)io_start_lba, (unsigned long long)*io_block_count, (unsigned long long)end_lba, (unsigned long long)new_blocks);
            return FBE_STATUS_OK;
        }   

        /* update the new block counts so we limit IO to one client only */
        *io_block_count = new_block_count;
             
    }
  
     return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_monitor_update_client_blk_cnt_if_needed()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_util_count_bits_set_in_bitmask()
 ******************************************************************************
 * @brief   Simply count the number if bits set.
 *
 * @param   fbe_raid_bitmask_t
 * 
 * @return  count of bits set
 *
 ******************************************************************************/
fbe_u32_t fbe_raid_group_util_count_bits_set_in_bitmask(fbe_raid_position_bitmask_t bitmask)
{
    fbe_u32_t   bits_set = 0;
    fbe_u32_t   bit_index;
    fbe_u32_t   total_bits = (sizeof(fbe_raid_position_bitmask_t) * FBE_BYTE_NUM_BITS);

    for (bit_index = 0; bit_index < total_bits; bit_index++)
    {
        if (((1 << bit_index) & bitmask) != 0)
        {
            bits_set++;
        }
    }
    return bits_set;
}
/*****************************************************
 * end fbe_raid_group_util_count_bits_set_in_bitmask()
 *****************************************************/

/*!****************************************************************************
 *          fbe_raid_group_util_handle_too_many_dead_positions()
 ****************************************************************************** 
 * 
 * @brief   This function compares the non-paged metadata against the data
 *          returned from the iots (paged metadata).  If there is a discrepancy
 *          it sets the condition that wil reconstruct the paged metadata.
 * 
 * @param   raid_group_p - Pointer to raid group object
 * @param   iots_p - Pointer to iots that failed
 * 
 * @return  fbe status
 * 
 * @note    Do not generate the `paged metadata reconstruct' event from this
 *          method since it may be invoked in the I/O path.
 *
 * @author
 *  03/06/2013  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_util_handle_too_many_dead_positions(fbe_raid_group_t *raid_group_p,
                                                                fbe_raid_iots_t *iots_p)
{
    fbe_raid_geometry_t                *raid_geometry_p = NULL;
    fbe_class_id_t                      class_id;
    fbe_raid_group_type_t               raid_type;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t                           lba;
    fbe_block_count_t                   blocks;
    fbe_chunk_size_t                    chunk_size;
    fbe_chunk_count_t                   chunk_count;
    fbe_chunk_count_t                   chunk_index;

    /* Get the class id and geometry.
     */
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p); 
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Get the LBA and block count from the IOTS
     */
    fbe_raid_iots_get_current_op_lba(iots_p, &lba);
    fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

    /* Determine the start chunk index and number of chunks.  This call will
     * adjust for disk-based LBAs vs raid-based LBAs.
     */
    fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &chunk_index, &chunk_count);

    /* Display a warning trace
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: handle too many dead lba: 0x%llx blocks: 0x%llx op: %d chunk index: 0x%x count: %d\n",
                          (unsigned long long)lba,
                          (unsigned long long)blocks,
                          opcode, chunk_index, chunk_count);

    /* Display the chunk information
     */
    fbe_raid_group_rebuild_trace_chunk_info(raid_group_p, &iots_p->chunk_info[0], chunk_count, iots_p);

    /* Always trace as an error because it is not expected.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: handle too many dead lba: 0x%llx blocks: 0x%llx op: %d class_id: %d raid type: %d\n",
                          (unsigned long long)lba,
                          (unsigned long long)blocks,
                          opcode, class_id, raid_type);

    /* A raid 10 does not have paged metadata.
     */
    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Currently no recovery for RAID-10
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_CRITICAL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "raid_group: handle too many dead unexpected raid type: %d\n",
                              raid_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the condition to mark `paged metadata reconstruct required' in the
     * non-paged metadata.
     */
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p,
                           FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_PAGED_METADATA_RECONSTRUCT);
    
    /* Set the condition to get the RG out of ready to activate on both side */
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_util_handle_too_many_dead_positions()
 **********************************************************/

/*!***************************************************************************
 *          fbe_raid_group_process_and_release_block_operation
 ***************************************************************************** 
 * 
 * @brief   This function processes (converts the block operation status and
 *          qualifier to a packet status) and then releases the block operation.
 *
 * @param   packet_p  - The packet requesting this operation.
 * @param   context   - The raid group
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  03/27/2013  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_process_and_release_block_operation(fbe_packet_t *packet_p, 
                                                                fbe_packet_completion_context_t context)
{
    fbe_status_t                    packet_status = FBE_STATUS_OK;
    fbe_payload_block_operation_t  *block_operation_p;
    fbe_payload_ex_t               *payload_p;
    fbe_raid_group_t               *raid_group_p;
    fbe_raid_verify_error_counts_t  local_error_counters;
    fbe_raid_verify_error_counts_t *error_counters_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    
    /* Get the raid group and payload
     */
    raid_group_p = (fbe_raid_group_t *)context;
    payload_p = fbe_transport_get_payload_ex(packet_p);    

    /* If there is no block operation something is wrong.
     */
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);    
    if (block_operation_p == NULL)
    {
        /* This is an error.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Block operation is null \n",
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_OK;
    }

    /* Get the packet status.
     */
    packet_status = fbe_transport_get_status_code(packet_p);

    /* Get the current block status and opcode.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* First check the packet status.
     */
    if (packet_status != FBE_STATUS_OK)
    {
        /* Trace some information and set the packet status
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s op: %d sts: 0x%x block sts/qual: %d/%d failed\n",
                              __FUNCTION__, opcode, packet_status, block_status, block_qualifier);
    }
    else if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* Else invoke the method to process the block status (if not success)
         */
         
        /*! @note Although this waste some stack space we need to validate the 
         *        I/O status below and this make the processing simpler.
         */
        error_counters_p = &local_error_counters;
        fbe_zero_memory(error_counters_p, sizeof(*error_counters_p));

        /* Determine the packet status based on the block status
         */
        packet_status = fbe_raid_group_handle_block_operation_error(raid_group_p,
                                                                    block_status,
                                                                    block_qualifier,
                                                                    error_counters_p);

        /* Trace some information and set the packet status
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s op: %d block sts/qual: %d/%d return status: 0x%x\n",
                              __FUNCTION__, opcode, block_status, block_qualifier, packet_status);
        fbe_transport_set_status(packet_p, packet_status, 0);
    }

    /* Now release the block operation and return success
     */
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_process_and_release_block_operation()
 **********************************************************/

/*!***************************************************************************
 *          fbe_raid_group_is_verify_after_write_allowed()
 ***************************************************************************** 
 * 
 * @brief   Check if we can set the `verify after write' iots flag or not.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   iots_p - Pointer to IOTS
 *
 * @return  bool - FBE_TRUE - Verify after write is allowed
 *                 FBE_FALSE - Verify after write is not allowed
 *
 * @author
 *  04/11/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_group_is_verify_after_write_allowed(fbe_raid_group_t *raid_group_p,
                                                        fbe_raid_iots_t *iots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_bool_t          b_is_system_rg = FBE_FALSE;

    /*! @note Currently we don't allow verify after write for:
     *          1. Spare or RAID-10
     *          2. System raid groups
     */
    fbe_raid_group_utils_is_sys_rg(raid_group_p, &b_is_system_rg);
    if (fbe_raid_geometry_is_sparing_type(raid_geometry_p) ||
        fbe_raid_geometry_is_raid10(raid_geometry_p)          ) {
        return FBE_FALSE;
    }
    if (b_is_system_rg == FBE_TRUE) {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/****************************************************
 * end fbe_raid_group_is_verify_after_write_allowed()
 ****************************************************/

/*!***************************************************************************
 *          fbe_raid_group_generate_verify_after_write()
 ***************************************************************************** 
 * 
 * @brief   The write has completed successfully. Generate the verify portion
 *          of the request.
 *
 * @param   raid_group_p - Raid group object pointer
 * @param   iots_p - IOTS 
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/10/2014  Ron Proulx  - Created. 
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_generate_verify_after_write(fbe_raid_group_t *raid_group_p, 
                                                        fbe_raid_iots_t *iots_p)        
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t           lba;
    fbe_block_count_t   blocks;
    fbe_lba_t           verify_lba;
    fbe_block_count_t   verify_blocks;

    /* The write completed now start the verify.
     */
    fbe_raid_iots_get_blocks(iots_p, &blocks);
    fbe_raid_iots_get_lba(iots_p, &lba);

    /* If the previous I/O was quiesced we need to decrement the `quiesced' 
     * count.
     */
    fbe_raid_group_handle_was_quiesced_for_next(raid_group_p, iots_p);

    /* Convert the request to physical.
     */
    fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                           lba,
                                           blocks,
                                           &verify_lba,
                                           &verify_blocks);

    /* Update the current opcode */
    fbe_raid_iots_set_current_opcode(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY);
    fbe_raid_iots_reinit(iots_p, verify_lba, verify_blocks);

    /* Mark this as the `original' iots.
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************** 
 * end fbe_raid_group_generate_verify_after_write()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_cleanup_verify_after_write()
 ***************************************************************************** 
 * 
 * @brief   Check the current status and update the original request accordingly.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   iots_p - Pointer to IOTS
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/10/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_cleanup_verify_after_write(fbe_raid_group_t *raid_group_p,
                                                       fbe_raid_iots_t *iots_p)
{
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    fbe_raid_iots_get_block_status(iots_p, &block_status);
    fbe_raid_iots_get_block_qualifier(iots_p, &block_qualifier);

    /* If it failed other trace flags can enable the tracing.
     */
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                             "verify after write i/pkt: %x/%x lba/bl: 0x%llx/0x%llx op: %d flags: 0x%x st/q: 0x%x/0x%x\n", 
                             (fbe_u32_t)iots_p, (fbe_u32_t)iots_p->packet_p, iots_p->lba, iots_p->blocks, 
                             iots_p->current_opcode, iots_p->flags,
                             block_status, block_qualifier);
    }

    /* Initialize fields specific to Block operations
     */
    fbe_raid_iots_set_current_opcode(iots_p, iots_p->block_operation_p->block_opcode);
    iots_p->blocks_transferred = iots_p->block_operation_p->block_count;
    fbe_raid_iots_set_blocks(iots_p, iots_p->block_operation_p->block_count);
    fbe_raid_iots_set_lba(iots_p, iots_p->block_operation_p->lba);
    fbe_raid_iots_set_current_lba(iots_p, (iots_p->block_operation_p->lba + iots_p->block_operation_p->block_count));
    fbe_raid_iots_set_blocks_remaining(iots_p, 0);
    fbe_raid_iots_set_current_op_lba(iots_p, iots_p->block_operation_p->lba);
    fbe_raid_iots_set_current_op_blocks(iots_p, iots_p->block_operation_p->block_count);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************* 
 * end fbe_raid_group_cleanup_verify_after_write()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_should_refresh_block_sizes()
 ****************************************************************
 * @brief
 *  Determine if we need to refresh the block sizes.
 *
 * @param raid_group_p - Current rg object.          
 * @param b_refresh_p - TRUE refresh needed, FALSE not needed.    
 *
 * @return fbe_status_t
 *
 * @author
 *  6/4/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_should_refresh_block_sizes(fbe_raid_group_t *raid_group_p,
                                                       fbe_bool_t *b_refresh_p)
{
    fbe_u32_t index;
    fbe_block_edge_t *edge_p = NULL;
    fbe_raid_position_bitmask_t new_bitmask_4k = 0;
    fbe_u32_t width;
    fbe_raid_position_bitmask_t orig_bitmask_4k;
    fbe_raid_position_bitmask_t update_needed_bitmask = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_position_bitmask_t rl_bitmask;
    
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &orig_bitmask_4k);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    for ( index = 0; index < width; index++) {
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &edge_p, index);
        if (edge_p->block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160) {
            new_bitmask_4k |= (1 << index);
            /* If the bit is changing and it is not rebuild logging, 
             * then we need to update the block size of this position. 
             *  
             * This check is needed to prevent the monitor from trying to refresh prior to 
             * clear rebuild logging. 
             */
            if (((orig_bitmask_4k & (1 << index)) == 0) &&
                (rl_bitmask & (1 << index))== 0 ) {
                /* Save this bit as it needs to change. */
                update_needed_bitmask |= (1 << index);
            }
        } else if (((orig_bitmask_4k & (1 << index)) != 0) &&
                   (rl_bitmask & (1 << index))== 0 ) {
            /* Save this bit as it needs to change. */
            update_needed_bitmask |= (1 << index);
        }
    }

    /* If there is a different set of 4k drives, 
     * and at least one of these is not rebuild logging, then refresh. 
     */
    if (update_needed_bitmask) {

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "refresh bl/s needed old: 0x%x new: 0x%x update: 0x%x\n",
                              orig_bitmask_4k, new_bitmask_4k, update_needed_bitmask);
        *b_refresh_p = FBE_TRUE;
    } else {
        *b_refresh_p = FBE_FALSE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_should_refresh_block_sizes()
 ******************************************/


fbe_bool_t fbe_raid_group_is_c4_mirror_raid_group(fbe_raid_group_t*in_raid_group_p)
{
    fbe_raid_geometry_t     *raid_geometry_p     = &in_raid_group_p->geo;
    return fbe_raid_geometry_is_c4_mirror(raid_geometry_p);
}

fbe_status_t fbe_raid_group_c4_mirror_get_new_pvd(fbe_raid_group_t *raid_group_p, fbe_u32_t *bitmask)
{
    fbe_object_id_t object_id;
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state;

    *bitmask = 0;
    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *)raid_group_p, &lifecycle_state);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to get lifecycle state\n",
                              __FUNCTION__);
        /* Always return OK to avoid blocking something */
        return FBE_STATUS_OK;
    }
    /* In specialize rotary, we need wait path ready.
     * If we mark the path as broken (new pvd) here, raid group will stuck in downstream_health_not_optimal condition.
     */
    if(lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE) {
        /* Let this raid group broken if both its drives are replaced */
        return FBE_STATUS_OK;
    }

    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    status = fbe_c4_mirror_check_new_pvd(object_id, bitmask);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to get new pvd bitmask\n",
                              __FUNCTION__);
    }
    return status;
}

fbe_status_t fbe_raid_group_c4_mirror_set_new_pvd(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p,
                                                 fbe_u32_t edge)
{
    fbe_object_id_t object_id;

    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    fbe_c4_mirror_rginfo_update(packet_p, object_id, edge);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t fbe_raid_group_c4_mirror_load_config(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_object_id_t object_id;
    fbe_status_t status;

    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    status = fbe_c4_mirror_load_rginfo(object_id, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t fbe_raid_group_c4_mirror_write_default(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_object_id_t object_id;
    fbe_status_t status;

    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    status = fbe_c4_mirror_write_default(packet_p, object_id);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t fbe_raid_group_c4_mirror_is_config_loadded(fbe_raid_group_t *raid_group_p, fbe_bool_t *is_loadded)
{
    fbe_status_t status;
    fbe_object_id_t object_id;

    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    status = fbe_c4_mirror_is_load(object_id, is_loadded);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to check c4mirror\n",
                              __FUNCTION__);
    }
    return status;
}

/************************************************************
 *  fbe_raid_group_get_valid_bitmask()
 ************************************************************
 *
 * @brief   Generate the bitmask.
 *
 * @param   raid_group_p - pointer to raid group object
 * @param   valid_bitmask_p - bitmask of positions for this width
 * 
 * @return  status
 *    
 ************************************************************/
fbe_status_t fbe_raid_group_get_valid_bitmask(fbe_raid_group_t *raid_group_p, fbe_raid_position_bitmask_t *valid_bitmask_p)
{
    fbe_u32_t                   position, max_position;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width;

    /* Get the width.
     */
    *valid_bitmask_p = 0;
    fbe_raid_group_get_width(raid_group_p, &width);

    /* Simply set a bit for each position.
     */
    max_position = (sizeof(*valid_bitmask_p) * 8);
    if (width > max_position)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: width 0x%x > max_position 0x%x \n",
                              __FUNCTION__, width, max_position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    max_position = width;
    for (position = 0; position < max_position; position++)
    {
        *valid_bitmask_p |= (1 << position);
    }

    /* Return the valid bitmask.
     */
    return status;
}
/*****************************************
 * fbe_raid_group_get_valid_bitmask()
 *****************************************/

/*!****************************************************************************
 *          fbe_raid_group_send_to_downstream_positions_completion()
 ******************************************************************************
 * @brief
 *   This function is the completion function for sending a usurper to the
 *   specified downstream positions.  It releases any allocated structures
 *   from the payload stack.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  03/04/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_send_to_downstream_positions_completion(fbe_packet_t *packet_p,
                                                                    fbe_packet_completion_context_t context_p)

{
    fbe_status_t                        status;                      
    fbe_raid_group_t                   *raid_group_p = (fbe_raid_group_t *)context_p;                   
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_buffer_operation_t     *buffer_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_payload_control_operation_opcode_t opcode;

    /* Get the status.
     */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control status.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_opcode(control_operation_p, &opcode);    
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    
    /* Trace a warning on failure.
     */
    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: send to downstream failed - status: %d control status: %d\n",
                              status, control_status);
    }   

    /* Determine if we allocated the downstream usurper or not.
     */
    if (opcode != FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS) {
        /* Release the send to downstream control operation
         */
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 
        fbe_payload_control_get_opcode(control_operation_p, &opcode);    
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

        /* If this is not the send to downstream something is wrong.
         */
        if (opcode != FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS) {
            fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: send to downstream completion opcode: %d not expected: %d\n",
                                  opcode, FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS);
        }
    }

    /* Now release the payload buffer.
     */
    buffer_operation_p = fbe_payload_ex_get_buffer_operation(payload_p);
    if (buffer_operation_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: send to downstream completion buffer operation not found\n");
    } else {
        fbe_payload_ex_release_buffer_operation(payload_p, buffer_operation_p);
    }

    /* Now complete the packet.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
} 
/**************************************************************
 * end fbe_raid_group_send_to_downstream_positions_completion()
 **************************************************************/

/*****************************************************************************
 *          fbe_raid_group_build_send_to_downstream_positions()
 *****************************************************************************
 *
 * @brief   Using the supplied packet add the `send to downstream positions'
 *          usurper.
 *
 * @param   raid_group_p - pointer to raid group object
 * @param   positions_to_send_to - Bitmask of positions to send to
 * @param   size_of_request - Size of request to send to each position
 * 
 * @return  status
 * 
 * @author
 *  03/04/2015  Ron Proulx  - Created.
 *    
 ******************************************************************************/
fbe_status_t fbe_raid_group_build_send_to_downstream_positions(fbe_raid_group_t *raid_group_p, 
                                                               fbe_packet_t *packet_p, 
                                                               fbe_raid_position_bitmask_t positions_to_send_to,
                                                               fbe_u32_t size_of_request)
{
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_buffer_operation_t                 *buffer_operation_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_raid_group_send_to_downstream_positions_t  *send_to_positions_p = NULL;
    fbe_u32_t                                       buffer_size;

    /* Allocate a buffer from the packet for the send downstream usurper.
     */
    buffer_size = sizeof(*send_to_positions_p) + size_of_request;
    payload_p = fbe_transport_get_payload_ex(packet_p);
    buffer_operation_p = fbe_payload_ex_allocate_buffer_operation(payload_p, buffer_size);
    if (buffer_operation_p == NULL) {
        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: pkt: %p failed to allocate buffer operation for: %d bytes\n",
                              __FUNCTION__, packet_p, buffer_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Buid the send downstream buffer.
     */
    send_to_positions_p = (fbe_raid_group_send_to_downstream_positions_t *)&buffer_operation_p->buffer;
    send_to_positions_p->positions_to_send_to = positions_to_send_to;
    send_to_positions_p->positions_sent_to = 0;
    send_to_positions_p->position_in_progress = FBE_EDGE_INDEX_INVALID;

    /* Now allocate a control request.
     */
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (control_operation_p == NULL) {
        /* Release the buffer operation
         */
        fbe_payload_ex_release_buffer_operation(payload_p, buffer_operation_p);

        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: pkt: %p failed to allocate control operation\n",
                              __FUNCTION__, packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Build the control operation.
     */
    fbe_payload_control_build_operation(control_operation_p,  
                                        FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS,
                                        send_to_positions_p, 
                                        sizeof(*send_to_positions_p)); 
    fbe_payload_ex_increment_control_operation_level(payload_p);

    /* Set the completion so that allocated resource are freed.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_send_to_downstream_positions_completion, 
                                          raid_group_p);

    /* Return success*/
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_build_send_to_downstream_positions()
 **********************************************************/

/*****************************************************************************
 *          fbe_raid_group_get_send_to_downsteam_request()
 *****************************************************************************
 *
 * @brief   Using the supplied packet get the `send to downstream' buffer
 *          (request).
 *
 * @param   raid_group_p - pointer to raid group object
 * @param   packet_p - pointer to packet with send to dowstream request
 * @param   send_to_downstream_pp - Address of pointer to populate
 * 
 * @return  status
 * 
 * @author
 *  03/05/2015  Ron Proulx  - Created.
 *    
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_send_to_downsteam_request(fbe_raid_group_t *raid_group_p, 
                                                         fbe_packet_t *packet_p, 
                                                         fbe_raid_group_send_to_downstream_positions_t **send_to_downstream_pp)
{
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    /* Initialize the return value.
     */
    *send_to_downstream_pp = NULL;

    /* Get the current control request and check if it is the send downstream.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 
    fbe_payload_control_get_opcode(control_operation_p, &opcode); 
    
    /* In most cases the send downstream will be the previous request.
     */  
    if (opcode != FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS) {
        /* Get the previous request.
         */ 
        control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);
        fbe_payload_control_get_opcode(control_operation_p, &opcode); 

        /* This should be the send to downstream.
         */
        if (opcode != FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS) {
            /* Trace and return error.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: unexpected opcode: %d \n",
                                  __FUNCTION__, opcode);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Get the buffer for the control code.
     */
    fbe_payload_control_get_buffer(control_operation_p, send_to_downstream_pp);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_get_send_to_downsteam_request()
 **********************************************************/

/*!****************************************************************************
 *          fbe_raid_group_send_to_downstream_position_completion()
 ******************************************************************************
 * @brief
 *   This function is the completion function for sending a usurper to the
 *   specified downstream position.  It check of the send to downstream positions
 *   is complete and if so completes the packet which will call the completion
 *   to free resources and continue (above).
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  03/04/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_send_to_downstream_position_completion(fbe_packet_t *packet_p,
                                                                   fbe_packet_completion_context_t context_p)

{
    fbe_status_t                        status = FBE_STATUS_INVALID;                      
    fbe_raid_group_t                   *raid_group_p = (fbe_raid_group_t *)context_p;                   
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_u32_t                           width;
    fbe_raid_group_send_to_downstream_positions_t  *send_to_downstream_p = NULL;
    fbe_edge_index_t                    position = FBE_EDGE_INDEX_INVALID;
    fbe_raid_position_bitmask_t         position_bitmask = 0;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_block_edge_t                   *block_edge_p = NULL;

    /* Get the payload and width.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_group_get_width(raid_group_p, &width);

    /* Get the completion status.
     */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control status.
     */
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_opcode(control_operation_p, &opcode);  

    /* Get the send downstream request.
     */
    fbe_raid_group_get_send_to_downsteam_request(raid_group_p, packet_p, &send_to_downstream_p);
    if (send_to_downstream_p == NULL) {
        /* If we could not get the send to downstream buffer trace an error
         * and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed to get send to downstream \n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Check if this is the first `send' request (i.e. we haven't sent a request yet)
     */
    if (send_to_downstream_p->position_in_progress != FBE_EDGE_INDEX_INVALID) {

        /* Mark the request as complete.
         */
        position = send_to_downstream_p->position_in_progress;
        send_to_downstream_p->positions_sent_to |= (1 << position);


        /* Trace a info on failure.
         */
        if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
            /* Trace an info and set the failed bit.
             */
            fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid group: send to downstream position: %d opcode: 0x%x failed - status: %d control status: %d\n",
                              position, opcode, status, control_status);
        }
    } else {
        /* Set packets status as good.*/
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    }

    /* Check if there is more work.
     */
    if (send_to_downstream_p->positions_sent_to != send_to_downstream_p->positions_to_send_to) {
        /* There is more work.  Determine the next position, set the completions and
         * send the request.
         */
        for (position = 0; position < width; position++) {
            position_bitmask = (1 << position);
            if (((position_bitmask & send_to_downstream_p->positions_to_send_to) != 0) &&
                ((position_bitmask & send_to_downstream_p->positions_sent_to) == 0)       ) {
                break;
            }
        }

        /* Validate new position to send to.
         */
        if (position >= width) {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: Could not location position to send (mask: 0x%x) sent: 0x%x\n", 
                                  __FUNCTION__, send_to_downstream_p->positions_to_send_to,
                                  send_to_downstream_p->positions_sent_to);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* Validate that the request completion has been set.
         */
        if (send_to_downstream_p->request_completion == NULL) {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: request completion is NULL\n", 
                                  __FUNCTION__);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* Set position to send to, completions and send it.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        send_to_downstream_p->position_in_progress = position;
        fbe_transport_set_completion_function(packet_p, 
                                              fbe_raid_group_send_to_downstream_position_completion, 
                                              raid_group_p);
        fbe_transport_set_completion_function(packet_p, 
                                              send_to_downstream_p->request_completion, 
                                              raid_group_p);
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, position);
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* All requests complete. Just complete packet.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} 
/**************************************************************
 * end fbe_raid_group_send_to_downstream_position_completion()
 **************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_send_to_downstream_positions()
 ******************************************************************************
 * @brief
 *   This function send the specified usurper to the previously specified
 *   positions.  It gets the `send to downstream' buffer and populates the
 *   request with the information supplied.
 * 
 * @param raid_group_p - Pointer to raid group object
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param request_opcode - The opcode for the request
 * @param request_p - The `temporary' copy of the request
 * @param request_size - The size of the request
 * @param completion_function - The completion function for the downstream request
 * 
 * @return  fbe_status_t
 * 
 * @note    This routine is called from a completion and therefore must complete
 *          the packet on error.
 *
 * @author
 *  03/05/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_send_to_downstream_positions(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p,
                                                         fbe_u32_t request_opcode,
                                                         fbe_u8_t *request_p,
                                                         fbe_u32_t request_size,
                                                         fbe_packet_completion_function_t completion_function,
                                                         fbe_packet_completion_context_t completion_context)
{
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_raid_group_send_to_downstream_positions_t  *send_to_downstream_p = NULL;
    fbe_u8_t                                       *request_buffer_p = NULL;
    fbe_payload_buffer_operation_t                 *buffer_operation_p = NULL;

    /* Get the send downstream request.
     */
    fbe_raid_group_get_send_to_downsteam_request(raid_group_p, packet_p, &send_to_downstream_p);
    if (send_to_downstream_p == NULL) {
        /* If we could not get the send to downstream buffer trace an error
         * and return failure.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed to get send to downstream \n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Validate sufficient space remains.
     */ 
    if ((sizeof(*send_to_downstream_p) + request_size) > FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE) {
        /* Exceeded buffer operation buffer size.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: send size: %d plus request size: %d exceeds buffer: %d\n", 
                              __FUNCTION__, (fbe_u32_t)sizeof(*send_to_downstream_p), request_size, FBE_PAYLOAD_BUFFER_OPERATION_BUFFER_SIZE);  
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the buffer operation so that we can allocate a buffer for the new request.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    buffer_operation_p = fbe_payload_ex_get_any_buffer_operation(payload_p);
    if (buffer_operation_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Buffer operation not found\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Allocate a control operation for the request.
     */
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (control_operation_p == NULL) {
        /* Generate an error trace.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: pkt: %p failed to allocate control operation\n",
                              __FUNCTION__, packet_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now populate the send to downstream with the information requested.
     */
    send_to_downstream_p->request_completion = completion_function;
    send_to_downstream_p->request_context = completion_context;
    request_buffer_p = &buffer_operation_p->buffer[0] + sizeof(*send_to_downstream_p);
    fbe_copy_memory(request_buffer_p, request_p, request_size);

    /* Build the control operation.
     */
    fbe_payload_control_build_operation(control_operation_p,  
                                        request_opcode,
                                        request_buffer_p, 
                                        request_size); 
    fbe_payload_ex_increment_control_operation_level(payload_p);

    /* Now call the method to send the first which will also check for completion.
     */
    fbe_raid_group_send_to_downstream_position_completion(packet_p, (fbe_packet_completion_context_t)raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************************************
 * end fbe_raid_group_send_to_downstream_positions()
 **************************************************************/


/*********************************
 * end file fbe_raid_group_utils.c
 *********************************/
