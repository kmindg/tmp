/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_rdgen_ts.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen tracking structure functions.
 *
 * @version
 *   5/28/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe/fbe_logical_drive.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_rdgen_object_dispatch_waiters(fbe_rdgen_object_t *object_p);
static fbe_bool_t fbe_rdgen_ts_gen_random_lba(fbe_rdgen_ts_t *ts_p);
static fbe_u32_t fbe_rdgen_ts_get_alignment_blocks(fbe_rdgen_ts_t *ts_p);
static fbe_bool_t fbe_rdgen_ts_is_request_aligned(fbe_rdgen_ts_t *ts_p, fbe_u32_t alignment_blocks);

/*************************
 *   GLOBAL DEFINITIONS
 *************************/
static fbe_atomic_t fbe_rdgen_ts_current_io_stamp = 0;

/*!**************************************************************
 * fbe_rdgen_ts_init()
 ****************************************************************
 * @brief
 *  Initialize the rdgen ts structure.
 *
 * @param ts_p - Current ts.
 * @param object_p - Our associated object ptr.
 * @param packet_p - The packet associated with this ts.
 * @param spec_p   - The io specification for this ts.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_init(fbe_rdgen_ts_t *ts_p,    
                       fbe_rdgen_object_t *object_p,
                       fbe_rdgen_request_t *request_p)
{
    fbe_status_t status;
    fbe_packet_t *packet_p = NULL;
    fbe_packet_t *write_packet_p = NULL;
    fbe_zero_memory(ts_p, sizeof(fbe_rdgen_ts_t));

    fbe_rdgen_ts_set_magic(ts_p);

    fbe_rdgen_ts_set_object(ts_p, object_p);
    ts_p->err_count = 0;
    ts_p->abort_err_count = 0;
    fbe_spinlock_init(&ts_p->spin);
    /* Flag this as the first request so that the pass count doesn't get 
     * incremented.
     */
    ts_p->flags = FBE_RDGEN_TS_FLAGS_FIRST_REQUEST;

    /* User has the option of specifying the starting sequence_count
     */
    if (request_p->specification.options & FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED)
    {
        /* Set the initial sequence_count to specified value
         */
        ts_p->sequence_count = request_p->specification.sequence_count_seed;
    }
    else
    {
        /* Else set the sequence count to 0
         */
        ts_p->sequence_count = 0;
    }
    ts_p->pass_count = 0;
    ts_p->io_count = 0;
    ts_p->partial_io_count = 0;
    ts_p->request_p = request_p;
    ts_p->lba = request_p->specification.min_lba;
    ts_p->blocks = request_p->specification.min_blocks;
    ts_p->pre_read_blocks = 0;
    ts_p->pre_read_lba = FBE_LBA_INVALID;
    ts_p->region_index = 0;
    fbe_rdgen_ts_update_last_send_time(ts_p);
    fbe_rdgen_ts_update_start_time(ts_p);

    /* Set values that we inherit from the request.
     */
    ts_p->operation = request_p->specification.operation;
    ts_p->io_interface = request_p->specification.io_interface;
    ts_p->priority = request_p->specification.priority;
    ts_p->block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;

    if (request_p->specification.io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_MJ ||
        request_p->specification.io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO){
        fbe_rdgen_ts_set_block_size(ts_p, FBE_BYTES_PER_BLOCK);
    }
    else {
        fbe_rdgen_ts_set_block_size(ts_p, FBE_BE_BYTES_PER_BLOCK);
    }

    /* Set status to good.  If we get any fatal error we will set this.
     */
    fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_OK);

    fbe_queue_element_init(&ts_p->queue_element);
    fbe_queue_element_init(&ts_p->thread_queue_element);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_start);

    /* We init the packet once. 
     * Each time we re-use the packet we re-init it. 
     */
    packet_p = fbe_rdgen_ts_get_packet(ts_p);
    write_packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    if (object_p->package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
        fbe_transport_initialize_packet(write_packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
        fbe_transport_initialize_sep_packet(write_packet_p);
    }

    if(request_p->specification.options & FBE_RDGEN_OPTIONS_USE_PRIORITY)
    {
        fbe_transport_set_packet_priority(packet_p, request_p->specification.priority);
        fbe_transport_set_packet_priority(write_packet_p, request_p->specification.priority);
    }

    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_CREATE_REGIONS))
    {
        fbe_data_pattern_create_region_list(&ts_p->request_p->specification.region_list[0],
                                            &ts_p->request_p->specification.region_list_length,
                                            FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH,
                                            ts_p->request_p->specification.start_lba, 
                                            ts_p->request_p->specification.max_blocks,
                                            fbe_rdgen_ts_get_max_lba(ts_p),
                                            ts_p->request_p->specification.pattern);
    }


    if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO)
    {
        status = fbe_rdgen_thread_init(&ts_p->bdev_thread, "fbe_rdgen_io", FBE_RDGEN_THREAD_TYPE_IO, FBE_U32_MAX);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: error initializing bdev thread status: 0x%x\n", 
                                    __FUNCTION__, status );
            fbe_rdgen_ts_set_status(ts_p, status);
            return;
        }
        status = fbe_rdgen_object_open_bdev(&ts_p->fp, object_p->device_name);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: error open bdev status: 0x%x\n", 
                                    __FUNCTION__, status );
            fbe_rdgen_ts_set_status(ts_p, status);
            return;
        }
    }
    else
    {
        fbe_rdgen_thread_init_flags(&ts_p->bdev_thread);
    }

    return;
}
/******************************************
 * end fbe_rdgen_ts_init()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_destroy()
 ****************************************************************
 * @brief
 *  Destroy the rdgen ts structure.
 *
 * @param ts_p - Current ts.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_ts_destroy(fbe_rdgen_ts_t *ts_p)
{
    if (!fbe_rdgen_ts_validate_magic(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: magic number is 0x%llx\n", __FUNCTION__, (unsigned long long)ts_p->magic);
    }
    fbe_rdgen_ts_free_memory(ts_p);
    fbe_spinlock_destroy(&ts_p->spin);
    fbe_rdgen_inc_ts_freed();
    return;
}
/******************************************
 * end fbe_rdgen_ts_destroy()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_lock()
 ****************************************************************
 * @brief
 *  Lock down the ts.
 *
 * @param ts_p - Current ts.
 *
 * @return None.
 *
 ****************************************************************/
void fbe_rdgen_ts_lock(fbe_rdgen_ts_t *ts_p)
{
    fbe_spinlock_lock(&ts_p->spin);
}
/**************************************
 * end fbe_rdgen_ts_lock
 **************************************/

/*!**************************************************************
 * fbe_rdgen_ts_unlock()
 ****************************************************************
 * @brief
 *  unlock the ts.
 *
 * @param ts_p - Current ts.
 *
 * @return None.
 *
 ****************************************************************/
void fbe_rdgen_ts_unlock(fbe_rdgen_ts_t *ts_p)
{
    fbe_spinlock_unlock(&ts_p->spin);
}
/**************************************
 * end fbe_rdgen_ts_unlock
 **************************************/

/*!**************************************************************
 * fbe_rdgen_ts_state()
 ****************************************************************
 * @brief
 *  Destroy the rdgen ts structure.
 *
 * @param ts_p - Current ts.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_ts_state(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t status;

    while (FBE_RDGEN_TS_STATE_STATUS_EXECUTING == (status = ts_p->state(ts_p)))
    {
        /* Continue to next state */
    }

    return;
}
/******************************************
 * end fbe_rdgen_ts_state()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_invalid_command()
 ****************************************************************
 * @brief
 *  State to handle an invalid command.
 *
 * @param ts_p - Current ts.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_invalid_command(fbe_rdgen_ts_t *ts_p)
{
    ts_p->status = FBE_STATUS_FAILED;
    fbe_rdgen_ts_inc_error_count(ts_p);
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);
    fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_invalid_command()
 ******************************************/

/*!***************************************************************************
 * fbe_rdgen_ts_set_complete_status()
 *****************************************************************************
 * @brief
 *  Mark the ts complete.  Since we could have broken up the request into
 *  multiple requests to SEP and we now continue on media errors, we must check
 *  the error counters to determine if all the sub-requests succeeded or not.
 *  The only acceptable error is media error.
 *
 * @param ts_p - Current ts.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 *****************************************************************************/
void fbe_rdgen_ts_set_complete_status(fbe_rdgen_ts_t *ts_p)
{
    /* The rdgen request completed successfully.
     */
    fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_OK);
    fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_SUCCESS);

    /* We must check for media errors.  The only allowable status are:
     *  o Invalid request
     *  o Media Error
     *  o Success
     */
    if (ts_p->invalid_request_err_count != 0)
    {
        /* Make sure the last status is set.
         */
        if (ts_p->last_packet_status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST)
        {
            /* Log the error but continue
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s: inv req cnt: %d b_sts: 0x%x doesn't match expected: 0x%x\n", 
                                    __FUNCTION__, ts_p->invalid_request_err_count,
                                    ts_p->last_packet_status.block_status,
                                    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        }
    }
    else if ((ts_p->err_count != 0)                     &&
             (ts_p->err_count != ts_p->abort_err_count)    )
    {
        /* If the media error count doesn't equal the error count, something 
         * is wrong.
         */
        if (ts_p->media_err_count != (ts_p->err_count - ts_p->abort_err_count))
        {
            /* Log the error but continue
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s: media cnt: %d doesn't equal err cnt: %d abort cnt: %d\n", 
                                    __FUNCTION__, ts_p->media_err_count, ts_p->err_count, ts_p->abort_err_count);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        }
        else
        {
            /* Else set the final status to media error.
             */
            ts_p->last_packet_status.status = FBE_STATUS_OK;
            ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
            ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST;
        }
    }
    else
    {
        /* Else there were either no errors or all the errors were aborts
         */
        ts_p->last_packet_status.status = FBE_STATUS_OK;
        ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    }
    return;
}
/*******************************************
 * end of fbe_rdgen_ts_set_complete_status()
 *******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_get_first_state()
 ****************************************************************
 * @brief
 *  Determine the first state of this ts.
 *
 * @param ts_p - Current ts.
 *
 * @return The ptr to the state we should execute.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_t fbe_rdgen_ts_get_first_state(fbe_rdgen_ts_t *ts_p)
{
    if (fbe_rdgen_ts_is_playback(ts_p)){
        /* We need to read records to find out the first state.
         */
        return fbe_rdgen_ts_generate_playback;
    }
    return fbe_rdgen_ts_get_state_for_opcode(ts_p->operation,
                                             ts_p->io_interface);
}
/******************************************
 * end fbe_rdgen_ts_get_first_state()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_get_state_for_opcode()
 ****************************************************************
 * @brief
 *  Determine the first state of this ts.
 *
 * @param ts_p - Current ts.
 *
 * @return The ptr to the state we should execute.
 *
 * @author
 *  8/2/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_t fbe_rdgen_ts_get_state_for_opcode(fbe_rdgen_operation_t opcode,
                                                       fbe_rdgen_interface_type_t io_interface)
{
    fbe_rdgen_ts_state_t state = NULL;

    switch (opcode)
    {
        case FBE_RDGEN_OPERATION_READ_ONLY:
            if (io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_DCA)
            {
                state = fbe_rdgen_ts_dca_rw_state0;
            }
            else
            {
                state = fbe_rdgen_ts_read_only_state0;
            }
            break;
        case FBE_RDGEN_OPERATION_WRITE_SAME:
        case FBE_RDGEN_OPERATION_WRITE_ONLY:
        case FBE_RDGEN_OPERATION_UNMAP:
            if (io_interface == FBE_RDGEN_INTERFACE_TYPE_IRP_DCA)
            {
                state = fbe_rdgen_ts_dca_rw_state0;
            }
            else
            {
                state = fbe_rdgen_ts_write_only_state0;
            }
            break;
        case FBE_RDGEN_OPERATION_VERIFY_WRITE:
        case FBE_RDGEN_OPERATION_ZERO_ONLY:
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY:
            state = fbe_rdgen_ts_write_only_state0;
            break;
        case FBE_RDGEN_OPERATION_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK:
        case FBE_RDGEN_OPERATION_ZERO_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK:
            state = fbe_rdgen_ts_write_read_compare_state0;
            break;
        case FBE_RDGEN_OPERATION_READ_CHECK:
            state = fbe_rdgen_ts_read_compare_state0;
            break;
        case FBE_RDGEN_OPERATION_READ_AND_BUFFER:
            state = fbe_rdgen_ts_read_buffer_state0;
            break;
        case FBE_RDGEN_OPERATION_VERIFY:
            state = fbe_rdgen_ts_verify_only_state0;
            break;
        case FBE_RDGEN_OPERATION_READ_ONLY_VERIFY:
            state = fbe_rdgen_ts_verify_only_state0;
            break;
        default:
            break;
    };
    return state;
}
/******************************************
 * end fbe_rdgen_ts_get_state_for_opcode()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_start()
 ****************************************************************
 * @brief
 *  This is the first state to execute for an rdgen ts.
 *
 * @param ts_p - Current ts.
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_start(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_t state = NULL;
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);

    if (ts_p->request_p->specification.max_blocks == 0)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: max blocks is too small\n", __FUNCTION__);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_invalid_command);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /*! @todo Make sure we don't try to start an I/O bigger than the
     * currently allocated pool.
     */
    
    
    if ( ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING )
    {
        ts_p->lba = 0;
    }

    #if 0
    if ( (ts_p->operation & FBE_RDGEN_OPERATION_CHECK_PATTERN) ||
              (ts_p->operation & FBE_RDGEN_OPERATION_SET_PATTERN) )
    {
        bCheck = fl_is_check_set_thread_outstanding( fldb_p->unit, ts_p );

        /* If bCheck flag is TRUE then it means that set/check thread is 
         *.in progress and we will not start another thread.
         */
        if ( bCheck )
        {
            success = FBE_FALSE;
            printf("rdgen: ERROR: cannot start check/set thread when another check/set thread is in progress\n");
        }
    }
    #endif

    if (ts_p->operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* The only pattern supported for a zero operation is the zero pattern.
         */
        if (ts_p->request_p->specification.pattern != FBE_RDGEN_PATTERN_ZEROS)
        {
            ts_p->request_p->specification.pattern = FBE_RDGEN_PATTERN_ZEROS;
        }
    }

    state = fbe_rdgen_ts_get_first_state(ts_p);
    if (state == NULL)
    { 
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: Unable to start thread for opcode %d\n", 
                                __FUNCTION__, ts_p->operation);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_invalid_command);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    fbe_rdgen_ts_set_state(ts_p, state);

    /* We need to hold the object lock while we do this check.
     */
    fbe_rdgen_object_lock(ts_p->object_p);
    if (fbe_rden_ts_fixed_lba_conflict_check(ts_p))
    {
        fbe_rdgen_object_unlock(ts_p->object_p);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: new request max/min lba: 0x%llx 0x%llx conflicts with already running I/O\n", __FUNCTION__,
			(unsigned long long)fbe_rdgen_ts_get_max_lba(ts_p),
			(unsigned long long)fbe_rdgen_ts_get_min_lba(ts_p));
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_invalid_command);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    fbe_rdgen_object_unlock(ts_p->object_p);
#if 0
    /* For other than normal READ, if two threads are running with
     * 'fixed lba' option then we will not allow the latest thread.
     */
    if ((!(ts_p->io_mode & FL_READ_ONLY)) &&
        (fl_gen_resolve_conflict(fldb_p, ts_p) == FL_FIXED_LBA_CONFLICT))
    {
        /* there is a fixed LBA conflict we will need to terminate this thread
         */
        printf("rdgen: ERROR: current LBAs conflicts with the I/Os going on unit %d , lba 0x%llX\r\n",
                fldb_p->unit,
                ts_p->iorb_p->lba);
        FLTS_TRANSITION(ts_p, fl_gen_cleanup_thread_state0);
        return RAID_STATE_EXECUTING;
    }
    
    FL_TS_INIT_STAMP(&ts_p->time_stamp);
    fl_ts_set_timestamp(&ts_p->time_stamp);
#endif

    /* Kick the abort thread if this request specified an abort time.
     * This will allow the abort thread to refresh its wait time. 
     */
    if (ts_p->request_p->specification.msecs_to_abort != 0)
    {
        fbe_rdgen_wakeup_abort_thread();
    }

    /* If this is from the peer use debug low. 
     * This would be very chatty when getting requests from the peer. 
     */
    fbe_rdgen_service_trace(ts_p->request_p->specification.b_from_peer ? FBE_TRACE_LEVEL_DEBUG_LOW : FBE_TRACE_LEVEL_INFO, 
                            ts_p->object_p->object_id,
                            "start ts 0x%x op: 0x%x lba: 0x%x/0x%llx/0x%llx/0x%llx bl: 0x%x/0x%llx/0x%llx, to peer %d\n", 
                            (fbe_u32_t)ts_p,
                            ts_p->operation,
                            ts_p->request_p->specification.lba_spec,
                            (unsigned long long)ts_p->request_p->specification.start_lba,
                            (unsigned long long)ts_p->request_p->specification.min_lba,
                            (unsigned long long)max_lba,
                            ts_p->request_p->specification.block_spec,
                            (unsigned long long)ts_p->request_p->specification.min_blocks,
                            (unsigned long long)ts_p->request_p->specification.max_blocks,
                            ts_p->b_send_to_peer);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_start()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_inc_historical_stats()
 ****************************************************************
 * @brief
 *  Increment the historical stats for this given ts.
 *
 * @param ts_p - current tracking structure.               
 *
 * @return None.
 *
 * @author
 *  5/22/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_inc_historical_stats(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_io_statistics_t *historical_stats_p = fbe_rdgen_get_historical_stats();

    historical_stats_p->aborted_error_count += ts_p->abort_err_count;
    historical_stats_p->bad_crc_blocks_count += ts_p->bad_crc_blocks_count;
    historical_stats_p->error_count += ts_p->err_count;
    historical_stats_p->inv_blocks_count += ts_p->inv_blocks_count;
    historical_stats_p->invalid_request_error_count += ts_p->invalid_request_err_count;
    historical_stats_p->io_count += ts_p->io_count;
    historical_stats_p->io_failure_error_count += ts_p->io_failure_err_count;
    historical_stats_p->congested_error_count += ts_p->congested_err_count;
    historical_stats_p->still_congested_error_count += ts_p->still_congested_err_count;
    historical_stats_p->media_error_count += ts_p->media_err_count;
    historical_stats_p->pass_count += ts_p->pass_count;
    return;
}
/******************************************
 * end fbe_rdgen_ts_inc_historical_stats()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_finished()
 ****************************************************************
 * @brief
 *  This is the final state of the rdgen ts.
 *
 * @param ts_p - Current ts.
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_DONE
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_finished(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_request_t *request_p = ts_p->request_p;
    fbe_u32_t request_count;
    fbe_time_t elapsed_msec = 0;
    fbe_u64_t ios_per_second = 0;
    fbe_u64_t seconds_per_io = 0;
    fbe_rdgen_object_t *object_p = ts_p->object_p;
    fbe_bool_t b_destroy_object = FBE_FALSE;
    fbe_bool_t b_from_peer = ts_p->request_p->specification.b_from_peer;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO)
    {
        fbe_rdgen_thread_t *thread_p = &ts_p->bdev_thread;
        if (!fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED))
        {
            /* Tell the thread to halt and wake it up.
             */
            fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT);
            fbe_rdgen_thread_lock(thread_p);
            fbe_rendezvous_event_set(&thread_p->event);
            fbe_rdgen_thread_unlock(thread_p);
            return FBE_RDGEN_TS_STATE_STATUS_DONE;
        }
        fbe_rdgen_object_close_bdev(&ts_p->fp);
    }

    /* Trace if an error was hit.
     */
    if (fbe_rdgen_ts_get_status(ts_p) != FBE_STATUS_OK)
    {

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                object_p->object_id,
                                "ts finish error st: 0x%x blst: 0x%x blq: 0x%x ts: 0x%x\n", 
                                ts_p->last_packet_status.status,
                                ts_p->last_packet_status.block_status,
                                ts_p->last_packet_status.block_qualifier, (fbe_u32_t)ts_p);
    }

    /* Lock the request so multiple returning requests don't overwrite each other.
     */
    fbe_rdgen_request_lock(request_p);

    /* See if we should save this status.
     */
    if (((ts_p->request_p->status.status == FBE_STATUS_INVALID) ||
         (ts_p->request_p->status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)) ||
        ((ts_p->request_p->status.status == FBE_STATUS_OK) &&
         (ts_p->request_p->status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)))
    {
        /* Save status if we have not recieved any bad completions completions yet.
         */
        fbe_rdgen_request_save_status(ts_p->request_p, &ts_p->last_packet_status);

    }
    fbe_rdgen_request_unlock(request_p);

    if (ts_p->err_count)
    {
        /*Split to two lines*/
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, object_p->object_id,
                                "ts finish 0x%x err_c:0x%x ab_c/abret_c:0x%x/0x%x fail_c:0x%x>\n", 
                                (fbe_u32_t)ts_p, ts_p->err_count, ts_p->abort_err_count, ts_p->abort_retry_count, 
                                ts_p->io_failure_err_count);
		fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, object_p->object_id,
                                "mede_c: 0x%x inverr_c: 0x%x ts: 0x%x<\n", 
                                ts_p->media_err_count, ts_p->invalid_request_err_count,
                                (fbe_u32_t)ts_p);
    }
    elapsed_msec = fbe_get_elapsed_milliseconds(ts_p->start_time);

    if (elapsed_msec != 0)
    {
        ios_per_second = (ts_p->io_count * FBE_TIME_MILLISECONDS_PER_SECOND) / elapsed_msec;
    }
    if (ts_p->io_count != 0)
    {
        seconds_per_io = elapsed_msec / (ts_p->io_count * FBE_TIME_MILLISECONDS_PER_SECOND);
    }

    /* If I/O is very slow, display seconds per I/O.
     */
    if (ios_per_second == 0)
    {
        /*Split to two lines*/
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, object_p->object_id,
                                "ts 0x%x (%d) compl lba: 0x%llx bl: 0x%llx pass: 0x%x ios: 0x%x>\n", 
                                (fbe_u32_t)ts_p, ts_p->thread_num,
				(unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				ts_p->pass_count, ts_p->io_count);
		fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, object_p->object_id,
                                "ts 0x%x msec: 0x%llx sec per io: 0x%llx errs: 0x%x<\n", 
                                (fbe_u32_t)ts_p,
				(unsigned long long)elapsed_msec,
				(unsigned long long)seconds_per_io,
				ts_p->err_count);
    }
    else
    {
        /*Split to two lines*/
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, object_p->object_id,
                                "ts 0x%x (%d) compl lba: 0x%llx bl: 0x%llx pass: 0x%x ios: 0x%x>\n", 
                                (fbe_u32_t)ts_p, ts_p->thread_num,
				(unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				ts_p->pass_count, ts_p->io_count);
		fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, object_p->object_id,
                                "ts 0x%x msec: 0x%llx ios per sec: 0x%llx errs: 0x%x<\n", 
                                (fbe_u32_t)ts_p,
				(unsigned long long)elapsed_msec,
				(unsigned long long)ios_per_second,
				ts_p->err_count);
    }

    /* Get locks.
     */
    fbe_rdgen_lock();
    fbe_rdgen_request_lock(request_p);
    fbe_rdgen_object_lock(ts_p->object_p);

    /* Decrement counters.
     */
    fbe_rdgen_dec_num_ts();
    fbe_rdgen_request_dec_num_threads(ts_p->request_p);
    fbe_rdgen_object_dec_num_threads(ts_p->object_p);
    fbe_rdgen_object_dec_current_threads(ts_p->object_p);

    if (fbe_rdgen_ts_is_playback(ts_p)){

        fbe_rdgen_object_reset_restart_time(ts_p->object_p);
        fbe_rdgen_object_set_max_threads(object_p, fbe_rdgen_object_get_num_threads(object_p));
        fbe_rdgen_object_enqueue_to_thread(object_p);
    }

    fbe_rdgen_ts_inc_historical_stats(ts_p);
    /* Add all the ts stats to the request.
     */
    fbe_rdgen_request_inc_num_errors(ts_p->request_p, ts_p->err_count);
    fbe_rdgen_request_inc_num_aborted_errors(ts_p->request_p, ts_p->abort_err_count);
    fbe_rdgen_request_inc_num_media_errors(ts_p->request_p, ts_p->media_err_count);
    fbe_rdgen_request_inc_num_io_failure_errors(ts_p->request_p, ts_p->io_failure_err_count);
    fbe_rdgen_request_inc_num_congested_errors(ts_p->request_p, ts_p->congested_err_count);
    fbe_rdgen_request_inc_num_still_congested_errors(ts_p->request_p, ts_p->still_congested_err_count);
    fbe_rdgen_request_inc_num_invalid_request_errors(ts_p->request_p, ts_p->invalid_request_err_count);
    fbe_rdgen_request_inc_num_inv_blocks(ts_p->request_p, ts_p->inv_blocks_count);
    fbe_rdgen_request_inc_num_bad_crc_blocks(ts_p->request_p, ts_p->bad_crc_blocks_count);
    fbe_rdgen_request_inc_num_passes(ts_p->request_p, fbe_rdgen_ts_get_pass_count(ts_p));
    fbe_rdgen_request_inc_num_ios(ts_p->request_p, fbe_rdgen_ts_get_io_count(ts_p));
    fbe_rdgen_request_inc_num_lock_conflicts(ts_p->request_p, fbe_rdgen_ts_get_lock_conflict_count(ts_p));
    fbe_rdgen_request_update_verify_errors(ts_p->request_p, &(ts_p->verify_errors));

    if (ios_per_second > FBE_U32_MAX)
    {
         fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, object_p->object_id,
                                 "ts ios per sec too large ios per sec: 0x%llx\n",
				 (unsigned long long)ios_per_second);
    }
    fbe_rdgen_request_save_ts_io_statistics(ts_p->request_p, ts_p->io_count, elapsed_msec, object_p->object_id);

    request_count = fbe_rdgen_request_get_num_threads(ts_p->request_p);

    /* Dequeue.
     */
    fbe_rdgen_request_dequeue_ts(request_p, ts_p);
    fbe_rdgen_object_dequeue_ts(ts_p->object_p, ts_p);

    /* Dispatch anyone that may have been waiting for us.
     */
    fbe_rdgen_object_dispatch_waiters(ts_p->object_p);

    /* Must be done prior to unlocking the request. 
     * If the request gets freed in another context then the ts memory is also freed. 
     */
    if (ts_p->object_p->device_p == NULL)
    {
        fbe_transport_destroy_packet(fbe_rdgen_ts_get_packet(ts_p));
        fbe_transport_destroy_packet(fbe_rdgen_ts_get_write_packet(ts_p));
    }

    /* While the lock is held, check if we should get rid of this object.
     * We cannot get rid of it if it is from the peer since we'd like the 
     * object to stick around for performance reasons so we don't keep re-creating it each time. 
     * We also cannot get rid of it if this is a playback, we let the thread free it. 
     */
    if ((fbe_rdgen_object_get_num_threads(object_p) == 0) &&
        fbe_rdgen_ts_is_playback(ts_p)){
        fbe_rdgen_object_enqueue_to_thread(object_p);
    }
    else if ((fbe_rdgen_object_get_num_threads(object_p) == 0) && !b_from_peer){
        b_destroy_object = FBE_TRUE;
        fbe_rdgen_dequeue_object(object_p);
        fbe_rdgen_dec_num_objects();
    }
    /* Release locks.
     */
    fbe_rdgen_object_unlock(object_p);
    fbe_rdgen_request_unlock(request_p);
    fbe_rdgen_unlock();

    fbe_rdgen_ts_destroy(ts_p);

    if (b_destroy_object)
    {
        fbe_rdgen_object_destroy(object_p);
    }

    /* Check if we need to kick off the request.
     * If we saw the number of threads get to 0, then 
     * we are the one that should kick it off. 
     */
    if (request_count == 0)
    {
        fbe_rdgen_request_check_finished(request_p);
    }

    return FBE_RDGEN_TS_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_rdgen_ts_finished()
 ******************************************/

/*!***************************************************************
 * fbe_rdgen_packet_completion
 ****************************************************************
 * @brief
 *  This function is the completion function for io packets
 *  sent out by the rdgen service.
 * 
 *  All we need to do is kick off the state machine for
 *  the io command that is completing.
 *
 * @param packet_p - The packet being completed.
 * @param context - The tracking structure being completed.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_MORE_PROCESSING_REQUIRED.
 *
 * @author
 *  10/26/07 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_rdgen_packet_completion(fbe_packet_t * packet_p, 
                            fbe_packet_completion_context_t context)
{
    fbe_rdgen_ts_t *ts_p = (fbe_rdgen_ts_t*) context;
    fbe_status_t status;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_time_t current_time;
    fbe_u32_t usec, msec;

    /* Save the status for the client state machine.
     */
    fbe_rdgen_ts_save_packet_status(ts_p);

#if 0 // Disabled until we fix raid completions in monitor context to be on correct core.
    fbe_cpu_id_t cpu_id;
	fbe_cpu_affinity_mask_t affinity;
#endif

    if (RDGEN_COND(ts_p->object_p == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_ts object is null. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RDGEN_COND(ts_p->b_outstanding == FBE_FALSE))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_ts outstanding flag 0x%x is not set to FBE_TRUE . \n", 
                                __FUNCTION__, ts_p->b_outstanding);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#if 0 // Disabled until we fix raid completions in monitor context to be on correct core.
    cpu_id  = fbe_get_cpu_id();

    if (RDGEN_COND(fbe_rdgen_ts_get_core_num(ts_p) != cpu_id))
    {
        fbe_thread_get_affinity(&affinity);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s Core = 0x%x cpu_id = 0x%llx mask = 0x%llx\n",
                                __FUNCTION__, fbe_rdgen_ts_get_core_num(ts_p), cpu_id, affinity);

    }
#endif

    /* We completed after many seconds.
     */
    usec = fbe_get_elapsed_microseconds(ts_p->last_send_time);
    if (usec >= FBE_RDGEN_SERVICE_TIME_MAX_MICROSEC)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s ts %p completed after %d usecs\n",
                                __FUNCTION__, ts_p, usec);
    }
    if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_RESET_STATS))
    {
        fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_RESET_STATS);
        fbe_rdgen_ts_reset_stats(ts_p);
    }
    /* Update our response time statistics.
     */
    fbe_rdgen_ts_set_last_response_time(ts_p, usec);
    fbe_rdgen_ts_update_cum_resp_time(ts_p, usec);
    fbe_rdgen_ts_update_max_response_time(ts_p, usec);

    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                FBE_RDGEN_OPTIONS_PERFORMANCE))
    {
        /* We want to avoid locks here.
         */
        ts_p->b_outstanding = FBE_FALSE;
    }
    else
    {
        /* We lock here since we might be getting aborted.  We want to prevent this ts from completing until 
         * the abort has completed. 
         */
        current_time = fbe_get_time();
        fbe_rdgen_ts_lock(ts_p);
        msec = fbe_get_elapsed_milliseconds(current_time);
        if (msec > 1000)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                    "%s ts_p: %p took %d msecs to get spinlock\n",
                                    __FUNCTION__, ts_p, msec);
        }
        ts_p->b_outstanding = FBE_FALSE;
        fbe_rdgen_ts_unlock(ts_p);
    }
    if ((packet_p->packet_status.code == FBE_STATUS_OK) &&
        (block_operation_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "Invalid block status %d lba: 0x%llx blocks: 0x%llx ts: 0x%x\n", 
                                block_operation_p->status, (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count,
                                (fbe_u32_t)ts_p);
    }
    if ((block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ||
        (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE))
    {
        fbe_u64_t *word_p = NULL;
        fbe_rdgen_service_t *rdgen_service_p = fbe_get_rdgen_service();
        word_p = (fbe_u64_t*)fbe_sg_element_address(ts_p->sg_ptr);

        if (fbe_base_service_get_trace_level(&rdgen_service_p->base_service) ==
            FBE_TRACE_LEVEL_DEBUG_LOW)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                    ts_p->object_p->object_id,
                                    "done ts: %p (%d) op: 0x%x lba: 0x%llx bl: 0x%llx\n",
                                    ts_p, ts_p->thread_num,
                                    block_operation_p->block_opcode, 
                                    (unsigned long long)block_operation_p->lba, 
                                    (unsigned long long)block_operation_p->block_count);
        }
        else
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                                    "done op/l/bl: %x/%llx/%llx %016llx %016llx %016llx %016llx\n", 
                                    block_operation_p->block_opcode, 
                                    (unsigned long long)block_operation_p->lba, 
                                    (unsigned long long)block_operation_p->block_count,
                    (unsigned long long)word_p[0],
                    (unsigned long long)word_p[1],
                    (unsigned long long)word_p[63],
                    (unsigned long long)word_p[64]);
        }
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                                "done op: 0x%x l/bl: %llx/%llx\n", 
                                block_operation_p->block_opcode,
				(unsigned long long)block_operation_p->lba,
				(unsigned long long)block_operation_p->block_count);
    }

#if 0
	if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE)){
		fbe_rdgen_ts_state(ts_p);
	} else 
#endif
	{
		/* Simply put this on a thread. */
		status = fbe_rdgen_ts_enqueue_to_thread(ts_p);
	}

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* end fbe_rdgen_packet_completion() */

/*!**************************************************************
 * ffbe_rdgen_ts_set_io_stamp()
 ****************************************************************
 * @brief
 * This function sets `io_stamp' which is simply a globally unique
 * tag for tracking an I/O.
 *
 * @param ts_p - current io.               
 *
 * @return fbe_status_t - Typically FB_STATUS_OK
 *
 * @author
 *  02/25/2011  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_ts_set_io_stamp(fbe_rdgen_ts_t *ts_p)
{
    fbe_atomic_t    io_stamp;

    io_stamp = fbe_atomic_increment(&fbe_rdgen_ts_current_io_stamp);
    ts_p->io_stamp = (fbe_rdgen_io_stamp_t)io_stamp;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_set_io_stamp()
 ******************************************/

/*!***************************************************************
 * fbe_rdgen_build_packet()
 ****************************************************************
 * @brief
 *  This function builds a packet with the given parameters.
 * 
 * @param packet_p - Packet to fill in.
 * @param opcode - The opcode to set in the packet.
 * @destination_package_id - This is the package we are sending
 *                           the packet to.  This determines
 *                           the payload type to use.
 * @param lba - The logical block address to put in the packet.
 * @param blocks - The block count.
 * @param block_size - The block size in bytes.
 * @param optimal_block_size - The optimal block size in blocks.
 * @param sg_p - Pointer to the sg list.
 * @param msec_to_expiration - Milliseconds packet will be allowed
 *                             to be outstanding before expiration.
 * @param io_stamp - Globally unique stamp for debugging
 * @param context - The context for the callback.
 * @param pre_read_desc_p - Ptr to the pre-read descriptor.
 * @param payload_flags - Flags to be set in the payload.
 *
 * @return none.
 *
 * @author:
 *  05/19/09 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_build_packet(fbe_packet_t * const packet_p,
                            fbe_package_id_t destination_package_id,
                            fbe_payload_block_operation_opcode_t opcode,
                            fbe_lba_t lba,
                            fbe_block_count_t blocks,
                            fbe_block_size_t block_size,
                            fbe_block_size_t optimal_block_size,
                            fbe_sg_element_t * const sg_p,
                            fbe_raid_verify_error_counts_t *verify_errors_p,
                            fbe_time_t msec_to_expiration,
                            fbe_packet_io_stamp_t io_stamp,
                            fbe_packet_completion_context_t context,
                            fbe_payload_pre_read_descriptor_t *pre_read_desc_p,
                            fbe_payload_block_operation_flags_t payload_flags)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_sg_descriptor_t *sg_desc_p = NULL;
    fbe_packet_priority_t priority;

    /* Re-initialize the packet.
     * We originally initialized the packet when we initialized the rdgen ts. 
     */
    fbe_transport_get_packet_priority(packet_p, &priority);
    fbe_transport_reuse_packet(packet_p);
    fbe_transport_set_packet_priority(packet_p, priority);

    payload_p = fbe_transport_get_payload_ex(packet_p);

	/* This will track all memory allocations for this I/O */
	fbe_payload_ex_allocate_memory_operation(payload_p); /* I was not able to find where block operation is released!!! */

    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    /* Set the completion function.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_rdgen_packet_completion,
                                          context);

    /* First build the block operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      opcode,
                                      lba,
                                      blocks,
                                      block_size,
                                      optimal_block_size,
                                      pre_read_desc_p);
    
    fbe_payload_ex_set_verify_error_count_ptr(payload_p, verify_errors_p);

    /* Set approriate flags. */
    fbe_payload_block_set_flag(block_operation_p, payload_flags);

    /* The 'Check Checksum' flag is set by default in build operation above.
     * For this flag, clear it if the caller did not specify it.
     */
    if (!(payload_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM))
    {
        fbe_payload_block_clear_flag(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM);
    }

    /* Default our repeat count to 1.
     */
    fbe_payload_block_get_sg_descriptor(block_operation_p, &sg_desc_p);
    sg_desc_p->repeat_count = 1;

    if (msec_to_expiration > 0)
    {
        /* First set then start the expiration timer
         */
        fbe_transport_set_and_start_expiration_timer(packet_p, msec_to_expiration);
    }
    else
    {
        /* Reset the expiration timer in case it has left-over value.
         */
        fbe_transport_init_expiration_time(packet_p);
    }

    return;
}
/* end fbe_rdgen_build_packet() */

/*!***************************************************************
 * fbe_rdgen_ts_send_packet()
 ****************************************************************
 * @brief
 *  This function sends a packet with the given parameters.
 * 
 * @param ts_p - tracking structure..
 * @param opcode - The opcode to set in the packet.
 * @param lba - Lba to send op to.
 * @param blocks - Blocks for operation.
 * @param sg_ptr - Ptr to the sg list to send with the packet.
 *
 * @return
 *  fbe_status_t 
 *
 * @author:
 *  06/04/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_send_packet(fbe_rdgen_ts_t *ts_p,
                                      fbe_payload_block_operation_opcode_t opcode,
                                      fbe_lba_t lba,
                                      fbe_block_count_t blocks,
                                      fbe_sg_element_t *sg_ptr,
                                      fbe_payload_block_operation_flags_t payload_flags)
{
    fbe_packet_t *packet_p = NULL;
    fbe_package_id_t package_id = ts_p->object_p->package_id;
	fbe_cpu_id_t cpu_id;
	fbe_cpu_affinity_mask_t affinity;
    fbe_memory_io_master_t *memory_io_master_p = NULL;
    fbe_u32_t alignment_blocks = 0;

    /* Set the opcode in the ts so that we know how to process errors.
     */
    fbe_rdgen_ts_set_block_opcode(ts_p, opcode);

    if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_ALLOW_FAIL_CONGESTION)
    {
        payload_flags |= FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION;
    }
    if (ts_p->object_p->device_p)
    {
        return fbe_rdgen_ts_send_disk_device(ts_p, opcode, lba, blocks, sg_ptr, payload_flags);
    }

    /* Update the last time we sent a packet for this ts.
     */
    fbe_rdgen_ts_update_last_send_time(ts_p);

    /* Get and set the io_stamp
     */
    fbe_rdgen_ts_set_io_stamp(ts_p);

    /* Make sure the packet is not outstanding.
     */
    if (RDGEN_COND(ts_p->b_outstanding == FBE_TRUE))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_ts outstanding flag 0x%x is not set to FBE_TRUE. \n", 
                                __FUNCTION__, ts_p->b_outstanding);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_rdgen_ts_set_time_to_abort(ts_p);
    ts_p->b_outstanding = FBE_TRUE;

    /* If this is a write and there is a pre-read, then fill the descriptor.
     */
    if (((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)) &&
        (ts_p->pre_read_blocks > 0))
    {
        fbe_payload_pre_read_descriptor_set_block_count(&ts_p->pre_read_descriptor, ts_p->pre_read_blocks);
        fbe_payload_pre_read_descriptor_set_lba(&ts_p->pre_read_descriptor, ts_p->pre_read_lba);
        fbe_payload_pre_read_descriptor_set_sg_list(&ts_p->pre_read_descriptor, ts_p->pre_read_sg_ptr);
    }
    if (RDGEN_COND(((lba + blocks) > ts_p->object_p->capacity) &&
                   ((ts_p->request_p->specification.max_lba == FBE_LBA_INVALID) ||
                   ((lba + blocks - 1) > ts_p->request_p->specification.max_lba))))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: lba 0x%llx + blocks 0x%llx > rdgen object's capacity 0x%llx. \n", 
                                __FUNCTION__, (unsigned long long)lba,
				(unsigned long long)blocks,
				(unsigned long long)ts_p->object_p->capacity );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Request must be aligned.
     */
    if (RDGEN_COND((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) != 0))
    {
        if (!fbe_rdgen_ts_is_request_aligned(ts_p, alignment_blocks))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: lba 0x%llx + blocks 0x%llx is not aligned to required alignment: 0x%x\n", 
                                    __FUNCTION__, (unsigned long long)lba,
                                    (unsigned long long)blocks,
                                    alignment_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* Use the write packet for media modify operations.
     */
    if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
    {
        packet_p = fbe_rdgen_ts_get_write_packet(ts_p);
    }
    else
    {
        packet_p = fbe_rdgen_ts_get_packet(ts_p);
    }
    fbe_rdgen_ts_set_packet_ptr(ts_p, packet_p);

    fbe_rdgen_build_packet(packet_p,
                           package_id,
                           opcode,
                           lba,
                           blocks,
                           ts_p->block_size,
                           ts_p->object_p->optimum_block_size,    /* opt block size. */
                           sg_ptr,
                           &ts_p->verify_errors,
                           ts_p->request_p->specification.msecs_to_expiration,
                           (fbe_packet_io_stamp_t)ts_p->io_stamp,
                           ts_p,    /* completion context */
                           &ts_p->pre_read_descriptor,
                           payload_flags);

	cpu_id  = fbe_get_cpu_id();

	if (cpu_id != fbe_rdgen_ts_get_core_num(ts_p)){
        fbe_thread_get_affinity(&affinity);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s Core = 0x%x cpu_id = 0x%llx mask = 0x%llx\n",
                                __FUNCTION__, fbe_rdgen_ts_get_core_num(ts_p),
				(unsigned long long)cpu_id,
				(unsigned long long)affinity);

	}

    /* Fill out the fields the client gave us for our memory io master.
     */
    memory_io_master_p = fbe_transport_memory_request_get_io_master(packet_p);
    memory_io_master_p->arrival_reject_allowed_flags = ts_p->request_p->specification.arrival_reject_allowed_flags;
    memory_io_master_p->client_reject_allowed_flags = ts_p->request_p->specification.client_reject_allowed_flags;

	if(fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE_BYPASS_TERMINATOR)){
		fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_COMPLETION_BY_PORT);
	}

	fbe_transport_set_cpu_id(packet_p, cpu_id);

    /* Populate the packet io_stamp */
    fbe_transport_set_io_stamp(packet_p, (fbe_packet_io_stamp_t)ts_p->io_stamp);

    if (1)
    {
        /* Send out the io packet. Send this to our only edge.
         */
		if(ts_p->object_p->edge_ptr == NULL/*ts_p->object_p->edge_ptr->capacity > 0x0fffffffff || ts_p->object_p->edge_ptr->offset > 0x0fffffffff*/){ /* Peter Puhov temporary hack */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Invalid capacity NULL object, setting to some default\n",__FUNCTION__);

			fbe_rdgen_object_initialize_capacity(ts_p->object_p);
		}

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                ts_p->object_p->object_id,
                                "send ts: %p (%d) op: 0x%x lba: 0x%llx bl: 0x%llx sg: %p\n",
                                ts_p, ts_p->thread_num, opcode, (unsigned long long)lba, (unsigned long long)blocks, sg_ptr);
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
        {
            fbe_u64_t *data_ptr = NULL;
            data_ptr = (fbe_u64_t*)sg_ptr->address;

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                    "First block: ts_p: 0x%p First 16: 0x%016llx 0x%016llx"
                                    "  Last 16: 0x%016llx 0x%016llx \n",
                                    ts_p, 
				    (unsigned long long)data_ptr[0],
				    (unsigned long long)data_ptr[1], 
                                    (unsigned long long)data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 2], 
                                    (unsigned long long)data_ptr[FBE_BE_BYTES_PER_BLOCK/sizeof(fbe_u64_t) - 1]);
        }
       if(fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, 
                                                 FBE_RDGEN_OPTIONS_DISABLE_CAPACITY_CHECK)){
           fbe_block_transport_send_funct_packet_params(ts_p->object_p->edge_ptr, packet_p, FBE_FALSE /* No capacity check */);
       } else {
           fbe_block_transport_send_funct_packet_params(ts_p->object_p->edge_ptr, packet_p, FBE_TRUE /* Yes capacity check */);
       }
    }
    else
    {
        /* We previously did not use an edge.  We're going to keep this code 
         * until we are sure we do not need it. 11/2009 
         */
        fbe_status_t status;

        /* We need to increment the block operation level, since 
         * this is what typically is done when we send the packet on the edge. 
         */
        if (package_id == FBE_PACKAGE_ID_PHYSICAL)
        {
            fbe_payload_ex_t * payload_p = NULL;
            payload_p = fbe_transport_get_payload_ex(packet_p);
            status = fbe_payload_ex_increment_block_operation_level(payload_p);
        }
        else
        {
            fbe_payload_ex_t * sep_payload_p = NULL;
            sep_payload_p = fbe_transport_get_payload_ex(packet_p);
            status = fbe_payload_ex_increment_block_operation_level(sep_payload_p);
        }

        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s unable to increment block operation level\n",
                                    __FUNCTION__);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
        /* We need to set the object id since sending to the edge typically does this for 
         * us. 
         */
        fbe_transport_set_address(packet_p,
                                  package_id,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  ts_p->object_p->object_id); 
        /* Just send this to the object directly without an edge.
         */
        fbe_topology_send_io_packet(packet_p);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_ts_send_packet
 **************************************/
/*!***************************************************************
 * fbe_rdgen_ts_save_packet_status()
 ****************************************************************
 * @brief
 *  This function saves the packet status within
 *  the rdgen ts structure.
 * 
 * @param ts_p - tracking structure.
 *
 * @return
 *  NONE
 *
 * @author:
 *  8/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_ts_save_packet_status(fbe_rdgen_ts_t *ts_p)
{
    fbe_packet_t *packet_p = fbe_rdgen_ts_get_packet_ptr(ts_p);
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Save the packet status and block payload status.
     */
    ts_p->last_packet_status.status = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p,
                                 &ts_p->last_packet_status.block_status);
    fbe_payload_block_get_qualifier(block_operation_p,
                                    &ts_p->last_packet_status.block_qualifier);
    fbe_payload_ex_get_media_error_lba(payload_p, &ts_p->last_packet_status.media_error_lba);
    return;
}
/**************************************
 * end fbe_rdgen_ts_save_packet_status
 **************************************/

/*!***************************************************************************
 *          fbe_rdgen_ts_has_uncorrectables()
 *****************************************************************************
 *
 * @brief   Using the verify error counts from the tracking structure, determine
 *          if any uncorrectable errors have occurred.  We need to determine this
 *          so that we can decide if we should proceed to the data comparison
 *          or not.
 *
 * @param   ts_p - tracking structure.
 *
 * @return  bool - FBE_TRUE - This ts has uncorrectable errors
 *                 FBE_FALSE - There are no uncorrectable errors
 *
 * @author
 *  09/30/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_bool_t fbe_rdgen_ts_has_uncorrectables(fbe_rdgen_ts_t *ts_p)
{
    fbe_bool_t  b_has_uncorrectables = FBE_FALSE;

    /* Check all the uncorrectable counters in the verify statistics.
     */
    if ((ts_p->verify_errors.u_coh_count        > 0) ||
        (ts_p->verify_errors.u_crc_multi_count  > 0) ||
        (ts_p->verify_errors.u_crc_single_count > 0) ||
        (ts_p->verify_errors.u_media_count      > 0) ||
        (ts_p->verify_errors.u_ss_count         > 0) ||
        (ts_p->verify_errors.u_ts_count         > 0) ||
        (ts_p->verify_errors.u_ws_count         > 0)    )
    {
        b_has_uncorrectables = FBE_TRUE;
    }

    /* Return if there are uncorrectables or not.
     */
    return b_has_uncorrectables;
}
/******************************************
 * end of fbe_rdgen_ts_has_uncorrectables()
 *****************************************/

/*!***************************************************************************
 *          fbe_rdgen_ts_handle_media_error()
 *****************************************************************************
 *
 * @brief   One of the SEP clients is SP Cache.  To some extent rdgen emulates
 *          a SEP client.  Thus (unless instructed otherwise by the option
 *          flags) rdgen will not treat `Media Error' as success and continue.
 *          We do increment the error count.
  
 * @param   ts_p - tracking structure.
 * @param   b_is_request_in_error_p - Pointer to bool that determine if we should
 *              the media error as an errored request or not.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  09/29/2011  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_rdgen_ts_handle_media_error(fbe_rdgen_ts_t *ts_p,
                                                    fbe_bool_t *b_is_request_in_error_p)
      
{
    /* Validate the last packet status.
     */
    if (ts_p->last_packet_status.status != FBE_STATUS_OK)
    {
        *b_is_request_in_error_p = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    /* Validate that the ts actually failed for the proper reason.
     */
    if (ts_p->last_packet_status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* We shouldn't be here.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "rdgen handle media error: unexpected block status:0x%x \n",
                                ts_p->last_packet_status.block_status);
        *b_is_request_in_error_p = FBE_TRUE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Unless the `fail on media error' flag is set we continue.
     */
    if ((ts_p->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)                  ||
        (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_FAIL_ON_MEDIA_ERROR)    )
    {
        /* Do not increment the error counter as it will be done by the caller.
         * Mark the request in error.
         */
        *b_is_request_in_error_p = FBE_TRUE;
    }
    else
    {
        /* Else we need to validate that there were no other errors for this 
         * request.
         */
        if ((ts_p->err_count != ts_p->media_err_count)          ||
            (fbe_rdgen_ts_has_uncorrectables(ts_p) == FBE_TRUE)    )
        {
            /* Some other error has occurred, treat the media error as a failure.
             */
            *b_is_request_in_error_p = FBE_TRUE;
        }
        else
        {
            /*! @note `Media Error' is a special status where SEP should have 
             *        continued to execute the request.  In-fact it is perfectly
             *        legal to return invalidated blocks to the SEP client.
             *        Therefore increment the error count and treat this as a 
             *        non-errored request.
             */
            fbe_rdgen_ts_inc_error_count(ts_p);
            ts_p->media_err_count++;
            *b_is_request_in_error_p = FBE_FALSE;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end of fbe_rdgen_ts_handle_media_error()
 ******************************************/

/*!***************************************************************
 * fbe_rdgen_ts_handle_error()
 ****************************************************************
 * @brief
 *  This function determines what to do in case of an error.
 * 
 * @param ts_p - tracking structure.
 * @param retry_state - State to transition to for retry.
 *
 * @return
 *  FBE_RDGEN_TS_STATE_STATUS_DONE - If no error processing is needed.
 *  FBE_RDGEN_TS_STATE_STATUS_EXECUTING - If more processing required.
 *
 * @author:
 *  8/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_handle_error(fbe_rdgen_ts_t *ts_p,
                                                      fbe_rdgen_ts_state_t_local retry_state)
{
    /* We mark this boolean FBE_FALSE if an error was encountered.
     */
    fbe_rdgen_ts_state_status_t ret_status;
    fbe_bool_t b_error = FBE_FALSE;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_packet_t *packet_p = fbe_rdgen_ts_get_packet_ptr(ts_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t last_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;

    /*! @todo rework to pass in the last operation opcode.
     */
    if (ts_p->object_p->device_p == NULL)
    {
        block_operation_p = fbe_transport_get_block_operation(packet_p);
        last_opcode = block_operation_p->block_opcode;
    }
    if ((ts_p->last_packet_status.status == FBE_STATUS_OK) &&
        (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE))
        {
            return FBE_RDGEN_TS_STATE_STATUS_DONE;
        }
    }
    else
    {
        if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PANIC_ON_ALL_ERRORS))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rdgen panic on IO error obj:0x%x, op:0x%x, lba:0x%llx, blk:0x%llx, curr_lba:0x%llx, curr_blk:0x%llx\n", 
                                    ts_p->object_p->object_id,
                                    ts_p->operation,
                                    (unsigned long long)ts_p->lba,
                				    (unsigned long long)ts_p->blocks,
                				    (unsigned long long)ts_p->current_lba, 
                				    (unsigned long long)ts_p->current_blocks);
        }
    }
    if (ts_p->last_packet_status.status != FBE_STATUS_OK)
    {
        b_error = FBE_TRUE;
    }
    /* We must now treat `Media Errors' as success
     */
    if (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        fbe_rdgen_ts_handle_media_error(ts_p, &b_error);
    }
    else if (ts_p->last_packet_status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        b_error = FBE_TRUE;
    }
    if ((ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        (ts_p->last_packet_status.block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED))
    {
        ts_p->still_congested_err_count++;
    }

    /* In an error was encountered, process it
     */
    if (b_error == FBE_TRUE)
    {
        fbe_bool_t b_continue_on_error = FBE_FALSE;

        switch (ts_p->last_packet_status.block_status)
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR:
                ts_p->media_err_count++;
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                        "ts compl with fatal media pk_st:0x%x bl_st:0x%x bl_q:0x%x ts: 0x%x err/ab:0x%x/0x%x\n",
                                        packet_p->packet_status.code, ts_p->last_packet_status.block_status,
                                        ts_p->last_packet_status.block_qualifier, (fbe_u32_t)ts_p,
                                        ts_p->err_count, ts_p->abort_err_count);
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED:
                ts_p->abort_err_count++;
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                        "ts compl with aborted pk_st:0x%x bl_st:0x%x bl_q:0x%x ts: 0x%x err/ab:0x%x/0x%x\n",
                                        packet_p->packet_status.code, ts_p->last_packet_status.block_status,
                                        ts_p->last_packet_status.block_qualifier, (fbe_u32_t)ts_p,
                                        ts_p->err_count, ts_p->abort_err_count);
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED:

                if (ts_p->last_packet_status.block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED)
                {
                    ts_p->congested_err_count++;
                    if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_ALLOW_FAIL_CONGESTION)
                    {
                        /* Just retry forever on a congested status.
                         */
                        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, ts_p->object_p->object_id,
                                                "ts compl with congested pk_st:0x%x bl_st:0x%x bl_q:0x%x ts: 0x%x\n",
                                                packet_p->packet_status.code, ts_p->last_packet_status.block_status,
                                                ts_p->last_packet_status.block_qualifier, (fbe_u32_t)ts_p);
                        fbe_rdgen_ts_set_state(ts_p, retry_state);
                        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
                    }
                }
                /* Fall through. */

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY:
            default:
                if ((packet_p->packet_status.code == FBE_STATUS_CANCELED) || 
                    (packet_p->packet_status.code == FBE_STATUS_CANCEL_PENDING))
                {
                    ts_p->abort_err_count++;
                    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                            "ts compl with aborted pk_st:0x%x bl_st:0x%x bl_q:0x%x ts: 0x%x err/ab:0x%x/0x%x\n",
                                            packet_p->packet_status.code, ts_p->last_packet_status.block_status,
                                            ts_p->last_packet_status.block_qualifier, (fbe_u32_t)ts_p,
                                            ts_p->err_count, ts_p->abort_err_count);
                    break;
                }
                ts_p->io_failure_err_count++;
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                        "ts compl with err pkt stat:0x%x bl stat:0x%x b_qual:0x%x \n",
                                        packet_p->packet_status.code, ts_p->last_packet_status.block_status,
                                        ts_p->last_packet_status.block_qualifier);
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST:
                /* Here instead of panicking, we will print the critical error.
                 */
                block_status = ts_p->last_packet_status.block_status;
                block_qualifier = ts_p->last_packet_status.block_qualifier;
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                        "ts compl with inv req stat:0x%x b_qual: 0x%x lba: 0x%llx blks: 0x%llx\n",
                                        block_status, block_qualifier, ts_p->lba, ts_p->blocks);
                fbe_rdgen_ts_set_unexpected_error_status(ts_p);
                ts_p->invalid_request_err_count++;
                ts_p->last_packet_status.block_status = block_status;
                ts_p->last_packet_status.block_qualifier = block_qualifier;
                break;
        }
        /* Increment all the ts error counters.
         */
        fbe_rdgen_ts_inc_error_count(ts_p);

        /* If this was a read, increment for any invalidated or bad crc blocks
         * that were returned.
         */
        if ((last_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)              &&
            (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)    )
        {
            status = fbe_rdgen_sg_count_invalidated_and_bad_crc_blocks(ts_p);
            if (status != FBE_STATUS_OK)
            {
                /* Set the status to bad in the rdgen ts.
                 */
                fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_INVALID_BLOCK_MISMATCH);
                fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
                fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
                return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
            }
        }

        /* We got an error. 
         * Determine how we want to handle this. 
         * This depends on our retry policy. 
         */
        if ( (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR) &&
             ((ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) ||
              (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
              (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST) ||
              (ts_p->last_packet_status.status == FBE_STATUS_CANCELED) ||
              (ts_p->last_packet_status.status == FBE_STATUS_CANCEL_PENDING)) )
        {
            b_continue_on_error = FBE_TRUE;
        }
        else if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS)
        {
            /* If this option is set, we retry all errors.
             */
            b_continue_on_error = FBE_TRUE;
        }
        else if ( ( (ts_p->request_p->specification.msecs_to_abort != 0) || 
                    (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                              FBE_RDGEN_OPTIONS_RANDOM_ABORT)) ) &&
                  ( (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
                    (ts_p->last_packet_status.status == FBE_STATUS_CANCELED) ||
                    (ts_p->last_packet_status.status == FBE_STATUS_CANCEL_PENDING)) )
        {
            /* If we are intentionally aborting I/Os and we are getting aborted errors,
             * then we should retry some number of times before giving up and trying a different I/O.
             */
            if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, 
                                                        FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR))
            {
                /* We only retry if the continue on error flag is set. 
                 */
                b_continue_on_error = FBE_FALSE;
            }
            else if (ts_p->abort_retry_count >= FBE_RDGEN_MAX_ABORT_RETRIES)
            {
                /* When we are retrying, Aborts that we injected get retried to a certain point.
                 */
                b_continue_on_error = FBE_TRUE;
            }
            else
            {
                /* Continue retrying the aborted request.
                 */
                ts_p->abort_retry_count++;
                fbe_rdgen_ts_set_state(ts_p, retry_state);
                return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
            }
        }

        /* Only if we `continue on error' will rdgen continue.  Otherwise this
         * rdgen request will be failed and I/O will stop.
         */
        if (b_continue_on_error)
        {
            fbe_rdgen_ts_state_t state = NULL;

            /* We will continue for media errors or aborted errors when the option to
             * continue is set, but the state machine will start over. 
             */
            state = fbe_rdgen_ts_get_first_state(ts_p);
            if (state == NULL)
            {
                fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_invalid_command);
                return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
            }
            fbe_rdgen_ts_set_state(ts_p, state);

            /* Free up memory before we start over.
             */
            fbe_rdgen_ts_free_memory(ts_p);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* No retries are being performed, 
             * just fail back to the request object.
             */
            ret_status = FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
            /* Set the status to bad in the rdgen ts.
             */
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);

            fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_IO_ERROR);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        }
    }
    else
    {
        /* If this was a read, check for bad crc's that may have been returned if we
         * were not checking for crc's. 
         */
        if ((last_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)                 &&
            ((ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC)            ||
             (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)  )   )
        {
            status = fbe_rdgen_sg_count_invalidated_and_bad_crc_blocks(ts_p);
            if (status != FBE_STATUS_OK)
            {
                /* Set the status to bad in the rdgen ts.
                 */
                fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
                fbe_rdgen_ts_inc_error_count(ts_p);
                fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_BAD_CRC_MISMATCH);
                fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
                return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
            }

            /* There is the case where we are injecting uncorrectables and the 
             * only errors are invalidation errors.  If we were not purposefully 
             * invalidating blocks (i.e. corrupted_blocks_bitmap is 0), then we 
             * must fail the request so that we don't fail the data compare due to 
             * an unexpected, invalidated block.
             */
            if ((ts_p->corrupted_blocks_bitmap == 0) &&
                (ts_p->inv_blocks_count > 0)            )
            {
                /* Flag the fact that we want to skip the data comparision phase.
                 */
                fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_SKIP_COMPARE);
            }

            /* If the block status is `media error' we should have reported at least (1) error.
             */
            if ((ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) &&
                (fbe_rdgen_ts_get_error_count(ts_p) < 1)                                                     )
            {
                 fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                        "fbe_rdgen_ts_handle_error Unexpected err_count:0x%x \n",
                                        fbe_rdgen_ts_get_error_count(ts_p));
                fbe_rdgen_ts_set_unexpected_error_status(ts_p);
                ts_p->invalid_request_err_count++;
            }

        } /* end if an error occurred on a read request */

        /* No error encountered.
         */
        ret_status = FBE_RDGEN_TS_STATE_STATUS_DONE;
    }
    return ret_status;
}
/**************************************
 * end fbe_rdgen_ts_handle_error
 **************************************/
static void fbe_rdgen_ts_handle_peer_error_bad_status(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, fbe_rdgen_object_get_object_id(ts_p->object_p),
                            "error bad status from peer ts: 0x%x local ts: 0x%x st: 0x%x bl_st: 0x%x bl_q: 0x%x\n",
                            (fbe_u32_t)ts_p->cmi_msg.peer_ts_ptr, (fbe_u32_t)ts_p,
                            ts_p->last_packet_status.status,
                            ts_p->last_packet_status.block_status,
                            ts_p->last_packet_status.block_qualifier);

    return;
}
/*!***************************************************************
 * fbe_rdgen_ts_handle_peer_error()
 ****************************************************************
 * @brief
 *  This function determines what to do in case of an error from the peer.
 * 
 * @param ts_p - tracking structure.
 *
 * @return
 *  FBE_RDGEN_TS_STATE_STATUS_DONE - If no error processing is needed.
 *  FBE_RDGEN_TS_STATE_STATUS_EXECUTING - If more processing required.
 *
 * @author:
 *  9/27/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_handle_peer_error(fbe_rdgen_ts_t *ts_p)
{
    fbe_bool_t b_error = FBE_FALSE;
    fbe_u32_t usec;

    /* Handle any error we received from the peer. 
     */
    if ((ts_p->last_packet_status.status == FBE_STATUS_OK) &&
        (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        /* No Error.
         */
    }
    else if ( (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR) &&
              ((ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) ||
               (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
               (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST) ||
               (ts_p->last_packet_status.status == FBE_STATUS_CANCELED) ||
               (ts_p->last_packet_status.status == FBE_STATUS_CANCEL_PENDING)) )
    {
        /* Peer received some error, but we are going to continue anyway.
         */
    }
    else if ( ( (ts_p->request_p->specification.msecs_to_abort != 0) || 
                (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                        FBE_RDGEN_OPTIONS_RANDOM_ABORT)) ) &&
              ( (ts_p->last_packet_status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
                (ts_p->last_packet_status.status == FBE_STATUS_CANCELED) ||
                (ts_p->last_packet_status.status == FBE_STATUS_CANCEL_PENDING)) )
    {
        /* Peer got an error, but we are injecting aborts.  Allow it to continue.
         */
    }
    else if (ts_p->last_packet_status.status != FBE_STATUS_OK)
    {

        fbe_rdgen_ts_handle_peer_error_bad_status(ts_p);

        /* Any other transport errors are not retried.
         */
        b_error = FBE_TRUE; 

        /* If we got a cmi error, then the err count will not have been incremented, so increment now.
         */
        if (ts_p->err_count == 0)
        {
            fbe_rdgen_ts_inc_error_count(ts_p);
        }
    }
    else
    {
        /* Non-retryable error.
         */
        b_error = FBE_TRUE;
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "non retryable error from peer status: 0x%x bl_status: 0x%x bl_qual: 0x%x\n",
                                ts_p->last_packet_status.status, ts_p->last_packet_status.block_status,
                                ts_p->last_packet_status.block_qualifier);
    }
    
    usec = fbe_get_elapsed_microseconds(ts_p->last_send_time);
    if (usec >= FBE_RDGEN_SERVICE_TIME_MAX_MICROSEC)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s ts %p completed after %d usecs\n",
                                __FUNCTION__, ts_p, usec);
    }

    if (b_error)
    {
        /* No retries are being performed, 
         * just fail back to the request object.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "error from peer ts: 0x%x (%u) st: 0x%x bl_st: 0x%x bl_q: 0x%x\n",
                                (fbe_u32_t)ts_p, ts_p->thread_num,
                                ts_p->last_packet_status.status,
                                ts_p->last_packet_status.block_status,
                                ts_p->last_packet_status.block_qualifier);

        /* Set the status to bad in the rdgen ts.
         */
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    }
    else
    {
        fbe_rdgen_ts_state_t state = NULL;

        /* We will continue for media errors or aborted errors when the option to
         * continue is set, but the state machine will start over. 
         */
        state = fbe_rdgen_ts_get_first_state(ts_p);
        if (state == NULL)
        {
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_invalid_command);
        }
        else
        {
            fbe_rdgen_ts_set_state(ts_p, state);
        }

        /* Free up memory before we start over.
         */
        fbe_rdgen_ts_free_memory(ts_p);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_rdgen_ts_handle_peer_error()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_ts_is_complete()
 ****************************************************************
 * @brief
 *  Determine if this ts is complete and should exit.
 *
 * @param  ts_p - current tracking structure.
 *
 * @return FBE_TRUE if we are done, FBE_FALSE otherwise.
 *
 * @author
 *  6/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_rdgen_ts_is_complete(fbe_rdgen_ts_t *ts_p)
{
    fbe_bool_t b_return = FBE_FALSE;
    fbe_time_t end_time;
    fbe_time_t current_time;

    /* Ts is complete if the request is complete.
     */
    if (fbe_rdgen_request_is_complete(ts_p->request_p))
    {
        b_return = FBE_TRUE;
    }

    /* We are complete if we finished the expected number of I/Os.
     */
    if (ts_p->request_p->specification.io_complete_flags & FBE_RDGEN_IO_COMPLETE_FLAGS_UNEXPECTED_ERRORS)
    {
        /* When this flag is set, we try to match pass count against the number of invalid request errors.
         */
        if (ts_p->invalid_request_err_count >= ts_p->request_p->specification.max_passes)
        {
            b_return = FBE_TRUE;
        }
    }
    else if ((ts_p->request_p->specification.max_passes != 0) &&
             (ts_p->pass_count >= ts_p->request_p->specification.max_passes))
    {
        b_return = FBE_TRUE;
    }

    /* We are complete if we finished the expected number of I/Os.
     */
    if ((ts_p->request_p->specification.max_number_of_ios != 0) &&
        (ts_p->io_count >= ts_p->request_p->specification.max_number_of_ios))
    {
        b_return = FBE_TRUE;
    }

    /* Only check time if the user specified a time.
     */
    if (ts_p->request_p->specification.msec_to_run != 0)
    {
        fbe_time_t request_start_time = fbe_rdgen_request_get_start_time(ts_p->request_p);

        end_time = request_start_time + ts_p->request_p->specification.msec_to_run;
        current_time = fbe_get_time();
        if (current_time >= end_time)
        {
            b_return = FBE_TRUE;
        }
    }
    return b_return;
}
/******************************************
 * end fbe_rdgen_ts_is_complete()
 ******************************************/

/*!***************************************************************
 * fbe_rdgen_ts_thread_queue_element_to_ts_ptr()
 ****************************************************************
 * @brief
 *  Convert from the fbe_rdgen_ts_t->thread_queue_element
 *  back to the fbe_rdgen_ts_t *.
 * 
 *  struct fbe_rdgen_ts_t { <-- We want to return the ptr to here.
 *  ...
 *    thread_queue_element   <-- Ptr to here is passed in.
 *  ...
 *  };
 *
 * @param thread_queue_element_p - This is the thread queue element that we wish to
 *                          get the ts ptr for.
 *
 * @return fbe_rdgen_ts_t - The ts that has the input thread queue element.
 *
 * @author
 *  06/04/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_t * 
fbe_rdgen_ts_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * rdgen ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_rdgen_ts_t * ts_p;
    ts_p = (fbe_rdgen_ts_t  *)((fbe_u8_t *)thread_queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_ts_t  *)0)->thread_queue_element));
    return ts_p;
}
/**************************************
 * end fbe_rdgen_ts_thread_queue_element_to_ts_ptr()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_ts_enqueue_to_thread()
 ****************************************************************
 * @brief
 *  Cause this rdgen ts to get scheduled on a thread.
 *
 * @param ts_p - Current ts.
 *
 * @return fbe_status_t - True if enqueue worked, False otherwise.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_ts_enqueue_to_thread(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    fbe_u32_t core_num;
    fbe_rdgen_affinity_t affinity;
    fbe_rdgen_specification_get_affinity(&ts_p->request_p->specification, &affinity);

    switch (affinity)
    {
        case FBE_RDGEN_AFFINITY_FIXED:
            /* We keep the thread fixed on a given core.
             */
            fbe_rdgen_specification_get_core(&ts_p->request_p->specification, &core_num);
            break;
        case FBE_RDGEN_AFFINITY_SPREAD:
            /* Spread the threads out to different cores.
             */
            core_num = fbe_rdgen_ts_get_thread_num(ts_p);
            break;
        case FBE_RDGEN_AFFINITY_NONE:
        default:
            /* Set the thread number based on the pass we are on for this object.
             * This causes different threads to continually use different cores.
             */
            core_num = ts_p->pass_count + fbe_rdgen_ts_get_thread_num(ts_p);
            break;
    };
    /* If the core number seems too big, just scale it down to the number 
     * of cores we have
     * We only allow the range of 0..core count
     */
    if (core_num >= fbe_get_cpu_count())
    {
        core_num = core_num % fbe_get_cpu_count();
    }

    /* Playback has fixed core number that does not change here.
     */
    if (fbe_rdgen_ts_is_playback(ts_p))
    {
        core_num = fbe_rdgen_ts_get_core_num(ts_p);
    }
    else 
    {
        fbe_rdgen_ts_set_core_num(ts_p, core_num);
    }

    if (ts_p->io_interface == FBE_RDGEN_INTERFACE_TYPE_SYNC_IO)
    {
        fbe_rdgen_thread_enqueue(&ts_p->bdev_thread, &ts_p->thread_queue_element);
        return FBE_STATUS_OK;
    }

    status = rdgen_service_send_rdgen_ts_to_thread(&ts_p->thread_queue_element, core_num);

    /* If enqueuing to a thread failed, something is very wrong. It is possible we are being destroyed.
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: cannot enqueue to thread, status: 0x%x\n", __FUNCTION__, status);
        fbe_rdgen_ts_finished(ts_p);
    }
    return status;
}
/******************************************
 * end fbe_rdgen_ts_enqueue_to_thread()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_get_alignment_blocks()
 ****************************************************************
 * @brief
 *  Returns the alignment blocks (0 indicates that no alignment is
 *  required)
 *
 * @param  ts_p - current request.            
 *
 * @return  alignment_blocks - 0 No alignment required
 *
 * @author
 *  11/26/2013 Ron Proulx - Created.
 *
 ****************************************************************/

static fbe_u32_t fbe_rdgen_ts_get_alignment_blocks(fbe_rdgen_ts_t *ts_p)
{
    fbe_u32_t   alignment_blocks;

    /* Quick check.
     */
    if ((ts_p->request_p->specification.alignment_blocks == 0) &&
        (ts_p->object_p->optimum_block_size <= 1)                 )
    {
        /* Return no alignment required.
         */
        return 0;
    }

    /* Get the maximum of alignment and optimal block size.
     */
    alignment_blocks = FBE_MAX(ts_p->request_p->specification.alignment_blocks, ts_p->object_p->optimum_block_size);
    if ((ts_p->request_p->specification.alignment_blocks != 0)                                      ||
        ( (ts_p->object_p->optimum_block_size > 1)                                           &&
         !fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                   FBE_RDGEN_OPTIONS_DO_NOT_ALIGN_TO_OPTIMAL)     )     )
    {
        /* Return the alignment blocks.
         */
        return alignment_blocks;
    }

    /* No alignment required.
     */
    return 0;
}
/****************************************** 
 * end fbe_rdgen_ts_get_alignment_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_is_request_aligned()
 ****************************************************************
 * @brief
 *  Returns the alignment blocks (0 indicates that no alignment is
 *  required)
 *
 * @param   ts_p - current request.    
 * @param   alignment_blocks - The alignment required      
 *
 * @return  bool - FBE_TRUE - Request is aligned
 *
 * @author
 *  11/27/2013 Ron Proulx - Created.
 *
 ****************************************************************/

static fbe_bool_t fbe_rdgen_ts_is_request_aligned(fbe_rdgen_ts_t *ts_p, fbe_u32_t alignment_blocks)
{
    if (ts_p->lba % alignment_blocks)
    {
        return FBE_FALSE;
    }
    if (ts_p->blocks % alignment_blocks)
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/****************************************** 
 * end fbe_rdgen_ts_is_request_aligned()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_adjust_aligned_size
 ****************************************************************
 * @brief.
 *  Adjust the size of this request so it is aligned
 *  to a pre-defined boundary.
 * 
 * @param ts_p - Current ts.
 * @param aligned_size - size to align to.
 *
 * @return
 *  none 
 *
 * @author
 *   06/19/01: Rob Foley - Created.
 *
 ****************************************************************/

void fbe_rdgen_ts_adjust_aligned_size(fbe_rdgen_ts_t *ts_p, fbe_u32_t aligned_size)
{
    fbe_u32_t stripe_size = aligned_size;
        
    if (ts_p->lba % stripe_size)
    {
        ts_p->lba += (stripe_size - (ts_p->lba % stripe_size));
    }
    if (ts_p->blocks % stripe_size)
    {
        ts_p->blocks += (stripe_size - (ts_p->blocks % stripe_size));
    }
    return;
} 
/* fbe_rdgen_ts_adjust_aligned_size() */

/*!**************************************************************
 * fbe_rdgen_ts_gen_sequential_increasing()
 ****************************************************************
 * @brief
 *  Generate the next lba and block count for a sequential
 *  request whose lbas are increasing.
 *
 * @param  ts_p - current request.               
 *
 * @return None.   
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_ts_gen_sequential_increasing(fbe_rdgen_ts_t * ts_p)
{
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);

    /* Increasing sequential.
     */
    if (ts_p->flags & FBE_RDGEN_TS_FLAGS_FIRST_REQUEST)
    {
        /* On the first request start at the specified lba.
         */
        ts_p->lba = ts_p->request_p->specification.start_lba;
    }
    else
    {
        fbe_bool_t b_beyond_capacity;

        if (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_INCREASING)
        {
            max_lba = ts_p->request_p->specification.max_lba;

            /* When both lba and block spec are increasing, only increase 
             * the lba when the block spec gets back to the start. 
             */
            if (ts_p->blocks != ts_p->request_p->specification.min_blocks)
            {
                return;
            }
        }

        /* Normal sequential increasing, just
         * increment by ts_p->blocks.
         */
        if (ts_p->request_p->specification.inc_lba == 0)
        {
            ts_p->lba += ts_p->blocks;
        }
        else
        {
            ts_p->lba += ts_p->request_p->specification.inc_lba;
        }
        b_beyond_capacity = ((ts_p->lba + ts_p->blocks - 1) > max_lba);

        if (b_beyond_capacity && 
            (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE))
        {
            /* We went beyond the capacity, but we're matching the element_size, so try to 
             * cut back to a multiple of the stripe size. 
             */
            if ((ts_p->lba + ts_p->object_p->stripe_size) <= max_lba)
            {
                ts_p->blocks = (fbe_block_count_t)(((max_lba - ts_p->lba + 1)/ ts_p->object_p->stripe_size) * ts_p->object_p->stripe_size);
                b_beyond_capacity = FBE_FALSE;
            }
        }
        if (b_beyond_capacity == FBE_FALSE)
        {
            return;
        }
        else 
        {
            /* Hit the end, starting a new pass.
             */
            fbe_rdgen_ts_inc_pass_count(ts_p);
            if (fbe_rdgen_ts_get_alignment_blocks(ts_p) > 0)
            {
                /* If aligned, reset to start.
                 */
                ts_p->lba = ts_p->request_p->specification.start_lba;;
            }
            else
            {
                /* Hit the end of the test range reset to start of test range.
                 */
                ts_p->lba = ts_p->request_p->specification.min_lba;
            }
        }
    }
    return;
}
/******************************************
 * end fbe_rdgen_ts_gen_sequential_increasing()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_gen_sequential_decreasing()
 ****************************************************************
 * @brief
 *  Generate the next lba and block count for a sequential
 *  request whose lbas are decreasing.
 *
 * @param  ts_p - current request.               
 *
 * @return None.   
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_gen_sequential_decreasing(fbe_rdgen_ts_t * ts_p)
{
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    /* Decreasing sequential.
     */
    if (ts_p->flags & FBE_RDGEN_TS_FLAGS_FIRST_REQUEST)
    {
        /* On the first request start at the end.
         */
        if (fbe_rdgen_ts_get_alignment_blocks(ts_p) > 0)
        {
            /* If aligned, reset to within blocks of the end.
             * We adjust the aligned size in case max blocks is not 
             * nicely aligned. 
             */
            ts_p->lba = max_lba - ts_p->blocks + 1;
            ts_p->lba -= (ts_p->lba % ts_p->blocks);
        }
        else
        {
            ts_p->lba = (max_lba - ts_p->blocks + 1);
        }
    }
    else
    {
        if ((ts_p->lba < ts_p->blocks) ||
            ((ts_p->lba - ts_p->blocks) < ts_p->request_p->specification.min_lba))
        {
            /* Hit the beginning, starting a new pass.
             */
            fbe_rdgen_ts_inc_pass_count(ts_p);
    
            if (fbe_rdgen_ts_get_alignment_blocks(ts_p) > 0)
            {
                /* If aligned, reset to within blocks of the end.
                 * We adjust the aligned size in case max blocks is not 
                 * nicely aligned. 
                 */
                ts_p->lba = max_lba - ts_p->blocks + 1;
                ts_p->lba -= (ts_p->lba % ts_p->blocks);
            }
            else
            {
                /* Hit the beginning of the test area.
                 * Set the lba to either the greater of max lba or blocks. 
                 */
                ts_p->lba = (max_lba - ts_p->blocks + 1);
            }
        }
        else
        {
            /* Normal sequential decreasing.
             * Just decrment by blocks.
             */
            ts_p->lba -= ts_p->blocks;
        }
    }
    return;
}
/******************************************
 * end fbe_rdgen_ts_gen_sequential_decreasing()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_gen_caterpillar_increasing()
 ****************************************************************
 * @brief
 *  Generate the next lba and block count for a caterpillar sequential
 *  request whose lbas are increasing.
 *
 * @param  ts_p - current request.               
 *
 * @return None.   
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_ts_gen_caterpillar_increasing(fbe_rdgen_ts_t * ts_p)
{
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *current_ts_p = NULL;
    fbe_block_count_t blocks;

    if (ts_p->request_p->specification.inc_lba == 0)
    {
        blocks = ts_p->blocks;
    }
    else 
    {
        blocks = ts_p->request_p->specification.inc_lba;
    }
    /* Get the lock while we modify the request.
     */
    fbe_rdgen_request_lock(ts_p->request_p);

    /* If we are beyond the test area, just round back to the start of the 
     * test area. 
     */
    if ((fbe_rdgen_request_get_caterpillar_lba(ts_p->request_p) + blocks) > 
        max_lba)
    {
        fbe_rdgen_request_set_caterpillar_lba(ts_p->request_p,
                                              ts_p->request_p->specification.min_lba);

        /* First mark everything
         */
        queue_element_p = fbe_queue_front(&ts_p->request_p->active_ts_head);
        while (queue_element_p != NULL)
        {
            current_ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
            /* Hit the end, starting a new pass for everyone.
             */
            current_ts_p->pass_count++;
            queue_element_p = fbe_queue_next(&ts_p->request_p->active_ts_head, queue_element_p);
        } /* while the queue element is not null. */
    }

    ts_p->lba = fbe_rdgen_request_get_caterpillar_lba(ts_p->request_p);

    fbe_rdgen_request_set_caterpillar_lba(ts_p->request_p, ts_p->lba + blocks);

    /* Get the lock while we modify the request.
     */
    fbe_rdgen_request_unlock(ts_p->request_p);
    return;
}
/******************************************
 * end fbe_rdgen_ts_gen_caterpillar_increasing()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_gen_caterpillar_decreasing()
 ****************************************************************
 * @brief
 *  Generate the next lba and block count for a caterpillar sequential
 *  request whose lbas are decreasing.
 *
 * @param  ts_p - current request.               
 *
 * @return None.   
 *
 * @author
 *  8/19/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_gen_caterpillar_decreasing(fbe_rdgen_ts_t * ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_lba_t cat_lba;

    /* Get the lock while we modify the request.
     */
    fbe_rdgen_request_lock(ts_p->request_p);
    cat_lba = fbe_rdgen_request_get_caterpillar_lba(ts_p->request_p);

    /* If we are already at the start of the caterpillar region. 
     * Or if using blocks we will go beyond the start of the region, 
     * then set the caterpillar to max lba. 
     */
    if ((cat_lba <= ts_p->request_p->specification.start_lba) ||
        (ts_p->blocks > cat_lba) ||
        ((cat_lba - ts_p->blocks) < ts_p->request_p->specification.start_lba))
    {
        /* Hit the beginning, starting a new pass.
         */
        fbe_rdgen_ts_inc_pass_count(ts_p);

        fbe_rdgen_request_set_caterpillar_lba(ts_p->request_p,
                                              max_lba+1);
    }
    
    if (RDGEN_COND((fbe_rdgen_request_get_caterpillar_lba(ts_p->request_p) - ts_p->blocks) >
                    max_lba))
    {
        /*! @todo This debug break is to catch an occasional issue with the caterpillar.  
         *        We will remove this once we fix this issue. 
         */
        fbe_debug_break();
        fbe_rdgen_request_unlock(ts_p->request_p);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: rdgen ts caterpillar LBA 0x%llx - ts blocks 0x%llx > maximum lba 0x%llx. \n", 
                                __FUNCTION__,
				(unsigned long long)fbe_rdgen_request_get_caterpillar_lba(ts_p->request_p),
                                (unsigned long long)ts_p->blocks,
				(unsigned long long)max_lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    ts_p->lba = fbe_rdgen_request_get_caterpillar_lba(ts_p->request_p) - ts_p->blocks;

    /* Update the caterpillar lba to our current lba.
     */
    fbe_rdgen_request_set_caterpillar_lba(ts_p->request_p, ts_p->lba);

    /* Get the lock while we modify the request.
     */
    fbe_rdgen_request_unlock(ts_p->request_p);
    return status;
}
/******************************************
 * end fbe_rdgen_ts_gen_caterpillar_decreasing()
 ******************************************/


/*!**************************************************************
 * fbe_rdgen_ts_trace_error()
 ****************************************************************
 * @brief
 *  Generate the lba and blocks info for this request.
 *
 * @param ts_p - Current ts.               
 *
 * @return fbe_bool_t FBE_TRUE if we were successful, FBE_FALSE otherwise.
 *
 * @author
 *  12/9/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_trace_error(fbe_rdgen_ts_t *ts_p,
                              fbe_trace_level_t trace_level,
                              fbe_u32_t line,
                              const char *function_p,
                              fbe_char_t * fail_string_p, ...)
                              __attribute__((format(__printf_func__,5,6)));
void fbe_rdgen_ts_trace_error(fbe_rdgen_ts_t *ts_p,
                              fbe_trace_level_t trace_level,
                              fbe_u32_t line,
                              const char *function_p,
                              fbe_char_t * fail_string_p, ...)
{
    char buffer[FBE_RDGEN_MAX_TRACE_CHARS];
    fbe_u32_t num_chars_in_string = 0;
    va_list argList;

    if (fail_string_p != NULL)
    {
        va_start(argList, fail_string_p);
        num_chars_in_string = _vsnprintf(buffer, FBE_RDGEN_MAX_TRACE_CHARS, fail_string_p, argList);
        buffer[FBE_RDGEN_MAX_TRACE_CHARS - 1] = '\0';
        va_end(argList);
    }

    fbe_rdgen_service_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                            "rdgen ts %p error\n", ts_p);
    fbe_rdgen_service_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                          "error: line: %d function: %s\n", line, function_p);
    /* Display the string, if it has some characters.
     */
    if (num_chars_in_string > 0)
    {
        fbe_rdgen_service_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                               buffer);
    }
    return;
}
/**************************************
 * end fbe_rdgen_ts_trace_error()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_random_32_fixed()
 ****************************************************************
 * @brief
 *  Return a random number based on the input seed.
 *  The random number sequence is guaranteed to be the same
 *  for a given seed.
 *
 * @param seed_p
 * 
 * @return fbe_u32_t
 *
 * @author
 *  2/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_u32_t fbe_rdgen_random_32_fixed(fbe_u32_t *seed_p)
{
    /* We use a 64 bit number so the multiply does not overflow.
     */
    fbe_u64_t random_number;

    random_number = (16807 * *seed_p) % (FBE_U32_MAX - 1);

    *seed_p = (fbe_u32_t)random_number;
    return (fbe_u32_t)random_number;
}
/******************************************
 * end fbe_rdgen_random_32_fixed()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_random_64_fixed()
 ****************************************************************
 * @brief
 *  Return a random number based on the input seed.
 *  The random number sequence is guaranteed to be the same
 *  for a given seed.
 *
 * @param seed_p
 * 
 * @return fbe_u64_t
 *
 * @author
 *  2/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_u64_t fbe_rdgen_random_64_fixed(fbe_u32_t *seed_p)
{
    fbe_u64_t low_32_bits;
    fbe_u64_t high_32_bits;
    low_32_bits = fbe_rdgen_random_32_fixed(seed_p);
    high_32_bits = fbe_rdgen_random_32_fixed(seed_p);
    high_32_bits <<= 32;
    
    /* We intentionally do not mask off the high 32 bits of low_32_bits,
     * it's just not necessary since we're creating a random number.
     */
    return (high_32_bits | low_32_bits);
}
/******************************************
 * end fbe_rdgen_random_64_fixed()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_random_64()
 ****************************************************************
 * @brief
 *  This is a Linear Congruential Random Number Generator.
 *
 * @param seed_p
 * 
 * @return fbe_u64_t
 *
 * @author
 *  10/3/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_u64_t fbe_rdgen_random_64(fbe_u64_t *seed_p)
{
    fbe_u64_t rand_num = *seed_p;
    #define LCG_A 6364136223846793005UL
    #define LCG_C 1442695040888963407UL
    #define LCG_M 0xFFFFFFFFFFFFFFFF

    rand_num = ((LCG_A * rand_num) + LCG_C) & LCG_M;
    *seed_p = rand_num;
    return (rand_num);
}
/******************************************
 * end fbe_rdgen_random_64()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_random_64_with_range()
 ****************************************************************
 * @brief
 *  Return a random number between 0 and the given max.
 *
 * @param ts_p - current rdgen tracking struct.
 * @param max_number - The highest number we should return.               
 *
 * @return fbe_u64_t the random number.   
 *
 * @author
 *  2/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_u64_t fbe_rdgen_ts_random_64_with_range(fbe_rdgen_ts_t *ts_p, fbe_u64_t max_number)
{
    /* If we are using fixed random numbers with a constant sequence use a different 
     * random number generator. 
     */
    if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_FIXED_RANDOM_NUMBERS)
    {
        if (max_number > FBE_U32_MAX)
        {
            return fbe_rdgen_random_64_fixed(&ts_p->random_context) % max_number;
        }
        else
        {
            /* We don't need a 64 bit random number so just generate a 32 bit one.
             */
            fbe_u64_t rand_num = fbe_rdgen_random_32_fixed(&ts_p->random_context);
            return rand_num % max_number;
        }
    }
    else
    {
        return fbe_random_64_with_range(&ts_p->random_context, max_number);  
    }
}
/******************************************
 * end fbe_rdgen_ts_random_64_with_range()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_object_random_64_with_range()
 ****************************************************************
 * @brief
 *  Return a random number between 0 and the given max.
 *
 * @param object_p - current rdgen object.
 * @param ts_p - current rdgen tracking struct.           
 *
 * @return fbe_u64_t the random number.   
 *
 * @author
 *  8/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_u64_t fbe_rdgen_object_random_64_with_range(fbe_rdgen_object_t *object_p,
                                                       fbe_rdgen_ts_t *ts_p)
{
    fbe_u64_t random_num;
    /* Generate the random number using the object's seed.
     *  
     * The lock is needed in order to preserve the generation of random numbers across 
     * multiple threads using this object. 
     *  
     * Then mask off the amount we calculated previously to get a random number. 
     * We need to use a mask instead of a mod in order to preserve the good period 
     * of the random number generator. 
     */
    fbe_rdgen_object_lock(object_p);
    random_num = fbe_rdgen_random_64(&object_p->random_context) & ts_p->random_mask;
    fbe_rdgen_object_unlock(object_p);
    return random_num;
}
/******************************************
 * end fbe_rdgen_object_random_64_with_range()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_generate()
 ****************************************************************
 * @brief
 *  Generate the lba and blocks info for this request.
 *
 * @param ts_p - Current ts.               
 *
 * @return fbe_bool_t FBE_TRUE if we were successful, FBE_FALSE otherwise.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_rdgen_ts_generate(fbe_rdgen_ts_t *ts_p)
{
    fbe_block_count_t blocks;
    fbe_u32_t alignment_blocks;
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_lba_t min_lba = fbe_rdgen_ts_get_min_lba(ts_p);
    fbe_block_count_t max_blocks_lba_range = (fbe_block_count_t)(max_lba - min_lba + 1);
    fbe_status_t status = FBE_STATUS_OK;

    if (fbe_rdgen_ts_is_playback(ts_p)){
        /* Playback is already generated.
         */
        return FBE_TRUE;
    }
    /* We are generating a new range, so clear our lock.
     */
    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED);

   if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION) ||
        fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE))
    {
        /* We do not want to randomize corrupt operation so we
         * will corrupt exactly for we have been asked for. And,
         * hence set all bits of  corrupted_blocks_bitmap to
         * 1 to negect the affect of it everywhere.
         */
        ts_p->corrupted_blocks_bitmap = (UNSIGNED_64_MINUS_1);
    }
    else
    {
        /* Each pass we generate this bitmap.  This cannot change within a single pass.
         */
        ts_p->corrupted_blocks_bitmap = fbe_rtl_random_64(&ts_p->random_context);

        /* Guarantee that (1) block requests have at least (1) corrupted block.
         */
        ts_p->corrupted_blocks_bitmap |= 0x0000000000000001ULL;
    }

    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_CREATE_REGIONS) ||
        fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_USE_REGIONS))
    {
        if (!fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST))
        {
            ts_p->region_index++;
            if ((ts_p->region_index >= FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH) ||
                (ts_p->region_index >= ts_p->request_p->specification.region_list_length))
            {
                ts_p->region_index = 0;
                fbe_rdgen_ts_inc_pass_count(ts_p);
            }

        }
        if (ts_p->region_index >= FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH)
        {
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                     "region index %d > max %d",
                                     ts_p->region_index, FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH);
            return FBE_FALSE;
        }
        ts_p->lba = ts_p->request_p->specification.region_list[ts_p->region_index].lba;
        ts_p->blocks = ts_p->request_p->specification.region_list[ts_p->region_index].blocks;

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                                "gen region: %d/%d lba: 0x%llx bl: 0x%llx\n",
                                ts_p->region_index,
				ts_p->request_p->specification.region_list_length,
                                (unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks);
        return FBE_TRUE;
    }

    /* First generate a random block count 0..max_blocks
     */
    if (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT)
    {
        blocks = ts_p->blocks;

        if (blocks >= max_blocks_lba_range)
        {
            blocks = (fbe_block_count_t)max_blocks_lba_range;
        }
        /* Make sure we generated a block count that is valid according to the specification.
         */
        else if (blocks < ts_p->request_p->specification.min_blocks)
        {
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_trace_error(ts_p, FBE_RDGEN_TS_TRACE_ERROR_PARAMS,
                                     "blocks %llu < spec.min_blocks %llu",
                                     (unsigned long long)blocks, (unsigned long long)ts_p->request_p->specification.min_blocks);
            return FBE_FALSE;
        }
        if (blocks > ts_p->request_p->specification.max_blocks)
        {
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_trace_error(ts_p, FBE_RDGEN_TS_TRACE_ERROR_PARAMS,
                                     "blocks %llu > spec.max_blocks %llu",
                                     (unsigned long long)blocks, (unsigned long long)ts_p->request_p->specification.max_blocks);
            return FBE_FALSE;
        }
    }
    else if (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_INCREASING)
    {
        blocks = ts_p->blocks;
        if (!fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST))
        {
            blocks = ts_p->blocks + ts_p->request_p->specification.inc_blocks;
        }
        /* Blocks will increase until we get to the top of the range, and then 
         * we will reset. 
         */
        if (blocks > ts_p->request_p->specification.max_blocks)
        {
            blocks = ts_p->request_p->specification.min_blocks;
        }
        if (blocks >= max_blocks_lba_range)
        {
            blocks = (fbe_block_count_t)max_blocks_lba_range;
        }
        /* Make sure we generated a block count that is valid according to the specification.
         */
        else if (blocks < ts_p->request_p->specification.min_blocks)
        {
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_trace_error(ts_p, FBE_RDGEN_TS_TRACE_ERROR_PARAMS,
                                     "blocks %llu < spec.min_blocks %llu",
                                     (unsigned long long)blocks, (unsigned long long)ts_p->request_p->specification.min_blocks);
            return FBE_FALSE;
        }
        if (blocks > ts_p->request_p->specification.max_blocks)
        {
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_trace_error(ts_p, FBE_RDGEN_TS_TRACE_ERROR_PARAMS,
                                     "blocks %llu > spec.max_blocks %llu",
                                     (unsigned long long)blocks, (unsigned long long)ts_p->request_p->specification.max_blocks);
            return FBE_FALSE;
        }
    }
    else if (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE)
    {
        /* If the user wants to match the stripe size, turn this into a constant stripe sized 
         * requests. 
         * We use min blocks to scale up the request. 
         */
        blocks = FBE_MIN(ts_p->object_p->stripe_size * ts_p->request_p->specification.min_blocks,
                         (fbe_block_count_t)((ts_p->object_p->capacity > FBE_U32_MAX) ? FBE_U32_MAX : ts_p->object_p->capacity));
    }
    else
    {
        fbe_block_count_t block_range = (ts_p->request_p->specification.max_blocks -
                                 ts_p->request_p->specification.min_blocks);
        if (ts_p->request_p->specification.max_blocks < 
            ts_p->request_p->specification.min_blocks)
        {
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_trace_error(ts_p, FBE_RDGEN_TS_TRACE_ERROR_PARAMS,
                                     "spec.max_blocks %llu < spec.min_blocks %llu",
                                     (unsigned long long)ts_p->request_p->specification.max_blocks,
                                     (unsigned long long)ts_p->request_p->specification.min_blocks);
            return FBE_FALSE;
        }

        /* Start out with min, then add on a random amount to this from the range. 
         */
        blocks = ts_p->request_p->specification.min_blocks;

        if (blocks > max_blocks_lba_range)
        {
            blocks = max_blocks_lba_range;
        }
        if (block_range > max_blocks_lba_range)
        {
            block_range = max_blocks_lba_range;
        }
        /* If we have a difference between max and min, then add on something random.
         */
        if (block_range > 0)
        {
            blocks += fbe_rdgen_ts_random_64_with_range(ts_p, block_range);
        }
        
        if (RDGEN_COND(blocks == 0))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: blocks 0x%llx = 0. \n", 
                                    __FUNCTION__, (unsigned long long)blocks);
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
            return FBE_FALSE;
        }

        /* Make sure we generated a block count that is valid according to the specification.
         */
        if (RDGEN_COND(blocks < ts_p->request_p->specification.min_blocks))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: blocks 0x%llx < specified request's minimum blocks 0x%llx. \n", 
                                    __FUNCTION__, (unsigned long long)blocks, (unsigned long long)ts_p->request_p->specification.min_blocks);
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
            return FBE_FALSE;
        }

        if (RDGEN_COND(blocks > ts_p->request_p->specification.max_blocks))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: blocks 0x%llx > specified request's maximum blocks 0x%llx. \n", 
                                    __FUNCTION__, (unsigned long long)blocks, (unsigned long long)ts_p->request_p->specification.max_blocks);
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
            return FBE_FALSE;
        }
    }
    
    ts_p->blocks = blocks;

    /* Next generate the lba.
     */
    if (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_FIXED)
    {
        /* Fixed lba, set the lba to the minimum, which should also be set to max.
         */
        ts_p->lba = min_lba;

        /* Only increment after the first pass.
         */
        if (!fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST))
        {
            /* Fixed lba requests increment the pass count on every new request.
             */
            fbe_rdgen_ts_inc_pass_count(ts_p);
        }
    }
    else if ((ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING) ||
             (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING) ||
             (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING) ||
             (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING))
    {
        if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0)
        {
            fbe_rdgen_ts_adjust_aligned_size(ts_p, alignment_blocks);
        }
        
        /* Sequential requests will increment the pass count in the below routines 
         * whenever we reach the start of the range. 
         */
        /* Sequential I/Os
         */
        if (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING)
        {
            fbe_rdgen_ts_gen_sequential_increasing(ts_p);
        }
        else if (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING)
        {
            fbe_rdgen_ts_gen_sequential_decreasing(ts_p);
        }
        else if (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING)
        {
            fbe_rdgen_ts_gen_caterpillar_increasing(ts_p);
        }
        else if (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING)
        {
            status = fbe_rdgen_ts_gen_caterpillar_decreasing(ts_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
                return FBE_FALSE;
            }
        }
        /* Make sure the lba is valid according to the specification.
         */
        if (RDGEN_COND((ts_p->lba < min_lba) && 
                       (ts_p->lba < ts_p->request_p->specification.start_lba)))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe rdgen ts LBA 0x%llx is either < min_lba 0x%llx or"
                                    " request's start lba 0x%llx. \n", 
                                    __FUNCTION__, (unsigned long long)ts_p->lba,
				    (unsigned long long)min_lba,
				    (unsigned long long)ts_p->request_p->specification.start_lba);
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
            return FBE_FALSE;
        }
        if (RDGEN_COND((ts_p->lba + ts_p->blocks - 1) > max_lba))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe rdgen ts LBA 0x%llx + blocks 0x%llx > max_lba 0x%llx. \n", 
                                    __FUNCTION__, (unsigned long long)ts_p->lba,
				    (unsigned long long)ts_p->blocks,
				    (unsigned long long)max_lba);
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
            return FBE_FALSE;
        }
    }
    else if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST) &&
             (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_RANDOM_START_LBA) == 0)
    {
        /* On the first request start at the specified lba.
         */
        ts_p->lba = ts_p->request_p->specification.start_lba;

        if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0)
        {
            fbe_rdgen_ts_adjust_aligned_size(ts_p, alignment_blocks);
        }
    }
    else
    {
        return fbe_rdgen_ts_gen_random_lba(ts_p);
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_rdgen_ts_generate()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_gen_random_lba()
 ****************************************************************
 * @brief
 *  Determine which lba to use when we are generating random lbas.
 *
 * @param  ts_p - Current thread.               
 *
 * @return fbe_bool_t false on error, true on success. 
 *
 * @author
 *  10/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t fbe_rdgen_ts_gen_random_lba(fbe_rdgen_ts_t *ts_p)
{
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_lba_t min_lba = fbe_rdgen_ts_get_min_lba(ts_p);
    fbe_block_count_t max_blocks_lba_range = (fbe_block_count_t)(max_lba - min_lba + 1);
    fbe_u32_t alignment_blocks;

    if (RDGEN_COND(max_blocks_lba_range == 0)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen max_blocks_lba_range 0x%llx is 0. \n", 
                                __FUNCTION__,
                                (unsigned long long)max_blocks_lba_range);
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        return FBE_FALSE;
    }

    /* Random requests increment the pass count on every new request.
     */
    fbe_rdgen_ts_inc_pass_count(ts_p);

    /* Start out with the minimum lba allowed.
     */
    ts_p->lba = min_lba;

    /* Add on a random amount that stays within the range.
     */
    if ((max_blocks_lba_range - ts_p->blocks) > 0) {
        if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, 
                                                   FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM)) {
            if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0) {
                fbe_lba_t alignment_index;
                /* First get the range in terms of the number of aligned regions. Then get a random index.
                 * This prevents us from generating multiple random lbas that hit the same region.
                 */
                alignment_index = fbe_rdgen_object_random_64_with_range(ts_p->object_p, ts_p);
                ts_p->lba += (alignment_index * alignment_blocks);
                fbe_rdgen_ts_adjust_aligned_size(ts_p, alignment_blocks);
            }
            else {
                ts_p->lba += fbe_rdgen_object_random_64_with_range(ts_p->object_p, ts_p);
            }
        }
        else {
            /* Legacy behavior for generating random numbers.
             */
            ts_p->lba += fbe_rdgen_ts_random_64_with_range(ts_p, (max_blocks_lba_range - ts_p->blocks));
            if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0){
                fbe_rdgen_ts_adjust_aligned_size(ts_p, alignment_blocks);
            }
        }
        if (ts_p->lba + ts_p->blocks > max_lba) {
            ts_p->lba = 0;
        }
    }
    /* Make sure the lba is valid according to the specification.
     */
    if (RDGEN_COND(ts_p->lba < min_lba)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen ts LBA 0x%llx < min_lba 0x%llx. \n", 
                                __FUNCTION__,
                                (unsigned long long)ts_p->lba,
                                (unsigned long long)min_lba);
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        return FBE_FALSE;
    }
    if (RDGEN_COND((ts_p->lba + ts_p->blocks - 1) > max_lba)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen ts LBA 0x%llx + blocks 0x%llx > max_lba 0x%llx. \n", 
                                __FUNCTION__,
                                (unsigned long long)ts_p->lba,
                                (unsigned long long)ts_p->blocks,
                                (unsigned long long)max_lba);
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_rdgen_ts_gen_random_lba()
 ******************************************/
/****************************************************************
 * fbe_rdgen_ts_get_breakup_count
 ****************************************************************
 * @brief
 *  Determine the correct fl_ts_p->breakup_count for this flts.
 *
 * @param ts_p - Current ts.
 *   
 * @return
 *  None
 *
 * @author
 *  06/03/2009 Rob Foley Created.
 ****************************************************************/

void fbe_rdgen_ts_get_breakup_count(fbe_rdgen_ts_t * ts_p)
{
    fbe_u32_t alignment_blocks;

    /* We should initially set the breakup count to 1.
     */
    ts_p->breakup_count = 1;

    /* If the request is not too large and the disable breakup flag is set don't
     * breakup the request.
     */
    if ((ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DISABLE_BREAKUP) &&
        (ts_p->blocks <= FBE_RDGEN_MAX_BLOCKS)                                          )
    {
        /* Don't breakup the request.
         */
    }
    else if (ts_p->request_p->specification.operation == FBE_RDGEN_OPERATION_READ_AND_BUFFER) 
    {
        /* This operation never breaks up.  We use the passed in sg list. */
        return;
    }
    else if (((ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT) ||
         (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE)) && 
        ((ts_p->blocks <= FBE_RDGEN_MAX_BLOCKS) ||
         (ts_p->operation == FBE_RDGEN_OPERATION_ZERO_ONLY)))
    {
        /* If we are doing fixed size I/Os and the size is less than
         * FBE_RDGEN_MAX_BLOCKS then don't break it up
         * since the user asked to do fixed sized transfers.
         * If it's greater than FBE_RDGEN_MAX_BLOCKS in size we need to 
         * break it up since we can't allocate more than
         * this many buffers for a request.
         */
    }
#if 0
    else if (((ts_p->operation == FBE_RDGEN_OPERATION_CHECK_PATTERN) ||
              (ts_p->operation == FBE_RDGEN_OPERATION_SET_PATTERN)) &&
             (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT))
    {
        if (ts_p->request_p->specification.alignment_blocks == 0)
        {
            /* We do not have an alignment, so simply break it up into 
             * whatever is convenient. 
             */
            fbe_u32_t breakup_blocks = FBE_MIN(ts_p->request_p->specification.max_blocks, FBE_RDGEN_MAX_BLOCKS);
            ts_p->breakup_count = (fbe_u32_t)(1 + (ts_p->blocks / breakup_blocks));
        }
        else
        {
            /* If we have an alignment, then obey it when we break the request up into 
             * pieces. 
             */
            ts_p->breakup_count = (fbe_u32_t)(ts_p->blocks / ts_p->request_p->specification.alignment_blocks);
            ts_p->breakup_count += (ts_p->blocks % ts_p->request_p->specification.alignment_blocks) ? 1 : 0;
        }
    }
#endif
    else if (ts_p->operation == FBE_RDGEN_OPERATION_WRITE_SAME || ts_p->operation == FBE_RDGEN_OPERATION_UNMAP)
#if 0
             ((ts_p->operation == FBE_RDGEN_OPERATION_CHECK_PATTERN) ||
              (ts_p->operation == FBE_RDGEN_OPERATION_SET_PATTERN)))
#endif
    {
        /* For Zeroing, the max we can read is FL_GEN_ZERO_VERIFY_BLOCKS blocks,
         * since this was the buffer allocated, and
         * the amount zeroed may be much more than our read buffer.
         *
         * For one pass I/Os we also use a similar strategy because
         * we want to break the I/O up into a predictable number of I/Os.
         */
        ts_p->breakup_count = (fbe_u32_t)(1 + (ts_p->blocks / FBE_RDGEN_ZERO_VERIFY_BLOCKS));
    }
    else if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0)
    {
        /* If we are aligned we need to be careful about how to breakup. 
         * Only breakup into aligned pieces. 
         * We only breakup if we are beyond the max blocks or 
         * every 10 times. 
         */
        if ( ts_p->blocks > FBE_RDGEN_MAX_BLOCKS ||
             ((fbe_rtl_random(&ts_p->random_context) % 10) == 0))
        {
            ts_p->breakup_count = (fbe_u32_t)(ts_p->blocks / alignment_blocks);
            if ((ts_p->blocks / alignment_blocks) > FBE_RDGEN_MAX_BLOCKS)
            {
                /* If the size we are breaking up into is still too large, then
                 * we are forced to break up further even if we are not aligned. 
                 */
                ts_p->breakup_count = (fbe_u32_t) (1 + (ts_p->blocks / FBE_RDGEN_MAX_BLOCKS));
            }
        }
    }
    else if ((ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_ALLOW_BREAKUP) ||
             (ts_p->blocks > FBE_RDGEN_MAX_BLOCKS))
    {
        /* Only breakup if the user explicitly specified this or if we exceed the max
         * allowed size. 
         */
        fbe_u32_t local_breakup_count = 0;
        fbe_u32_t random_number = fbe_rtl_random(&ts_p->random_context);
        
        /* Determine the minimum breakup count if each request
         * is a maximum of FBE_RDGEN_MAX_BLOCKS in size.
         * Only non-backfill requests may be broken up in this way.
         */
        ts_p->breakup_count = (fbe_u32_t) (1 + (ts_p->blocks / FBE_RDGEN_MAX_BLOCKS));

        /* One in 1000 times (this is arbitrary), try to break up into even more pieces.
         */
        if ((random_number % 1000) == 0)
        {
            /* the local breakup count is a random sized breakup of the request,
             * such that each request is at least FL_GEN_WR_MIN_BLOCKS.
             */
            if (ts_p->request_p->specification.min_blocks == 1)
            {
                local_breakup_count = (fbe_u32_t) (random_number % ts_p->blocks);
            }
            else
            {
                /* Make sure the pieces are a multiple of min blocks.
                 */
                local_breakup_count = (fbe_u32_t) (ts_p->blocks / ts_p->request_p->specification.min_blocks);
            }
        }
        
        /* Determine the max breakup count.
         */
        ts_p->breakup_count = FBE_MAX( ts_p->breakup_count, local_breakup_count );
    }
    return;
}
/* fbe_rdgen_ts_get_breakup_count() */

/*****************************************************************************
 * fbe_rdgen_ts_conflict_check
 *****************************************************************************
 * @brief
 *   Check an entire GEN DB's queue for conflicts with 
 *   the passed in iorb.  Skip the passed in flts.
 *
 * @param - ts_p - Current rdgen ts.
 *   
 * @return
 *   FBE_TRUE - Conflict.
 *   FBE_FALSE - No conflict.
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 *****************************************************************************/

static fbe_bool_t fbe_rdgen_ts_conflict_check(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_t *current_ts_p;
    fbe_lba_t current_start;
    fbe_lba_t current_end;
    fbe_lba_t check_start = ts_p->lba;
    fbe_lba_t check_end = ts_p->lba + ts_p->blocks - 1;
    fbe_rdgen_object_t *object_p = ts_p->object_p;
    fbe_queue_element_t *current_queue_element_p = NULL;
    fbe_bool_t b_is_check_thread = fbe_rdgen_operation_is_checking(ts_p->operation);

    if (object_p->optimum_block_size != 1)
    {
        /* Round to the optimum block size.
         */
        check_start -= FBE_MIN(check_start, (check_start % object_p->optimum_block_size));
        check_end += FBE_MIN(check_end, object_p->optimum_block_size - (check_end % object_p->optimum_block_size));
    }

    for (current_queue_element_p = fbe_queue_front(&object_p->active_ts_head);
         current_queue_element_p != NULL;
         current_queue_element_p = fbe_queue_next(&object_p->active_ts_head, current_queue_element_p))
    {
        current_ts_p = fbe_rdgen_ts_queue_element_to_ts_ptr(current_queue_element_p);
        if (ts_p == current_ts_p)
        {
            /* Self, just skip.
             */
            continue;
        }
        if (!fbe_rdgen_ts_is_flag_set(current_ts_p, FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED)) 
        {
            /* Don't check against this since it does not have a lock yet.
             */
            continue;
        }
        /* Requests that are not checking cannot conflict unless the client disables overlapped I/Os.
         * If either thread does a check then they could conflict. 
         * Disabling overlapped I/Os prevents rdgen from issuing any requests that overlap. 
         */
        if ( (b_is_check_thread == FALSE) &&
             (fbe_rdgen_operation_is_checking(current_ts_p->operation) == FALSE) &&
             ( fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, 
                                                      FBE_RDGEN_OPTIONS_DISABLE_OVERLAPPED_IOS)) )
        {
            continue;
        }

        /* Just check for overlap in their ranges.
         */
        current_start = current_ts_p->lba;
        current_end = current_ts_p->lba + current_ts_p->blocks - 1;

        if (current_ts_p->object_p->optimum_block_size != 1)
        {
            /* Round to the optimum block size.
             */
            current_start -= FBE_MIN(current_start, (current_start % current_ts_p->object_p->optimum_block_size));
            current_end += FBE_MIN(current_end, current_ts_p->object_p->optimum_block_size - 
                                   (current_end % current_ts_p->object_p->optimum_block_size));
        }
        if ((check_end < current_start) ||
            (check_start > current_end))
        {
            /* No conflict.
             */
        }
        else
        {
            /* Conflict.
             */
            return FBE_TRUE;
        }
    }

    /* No conflict found.
     */
    return FBE_FALSE;
}                               
/*******************************
 * fbe_rdgen_ts_conflict_check 
/*****************************************************************************
 * fbe_rden_ts_fixed_lba_conflict_check
 *****************************************************************************
 * @brief
 *   Check an entire object queue for conflicts with the passed rdgen ts.
 *   If the passed in rdgen ts is a fixed lba, then
 *   we see if it conflicts with any other fixed lba requests running.
 *
 * @param - ts_p - Current rdgen ts.
 *   
 * @return
 *   FBE_TRUE - Conflict.
 *   FBE_FALSE - No conflict.
 *
 * @notes
 *   The rdgen object lock must be already held.
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_bool_t fbe_rden_ts_fixed_lba_conflict_check(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_t *current_ts_p;
    fbe_lba_t current_start;
    fbe_lba_t current_end;
    fbe_lba_t check_start = fbe_rdgen_ts_get_min_lba(ts_p);
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_lba_t check_end = max_lba + ts_p->request_p->specification.max_blocks - 1;
    fbe_rdgen_object_t *object_p = ts_p->object_p;
    fbe_queue_element_t *current_queue_element_p = NULL;
    fbe_bool_t b_is_check_thread = fbe_rdgen_operation_is_checking(ts_p->operation);

    /* If this request is not fixed lba, then it will not have a fixed lba conflict.
     * A fixed lba conflict occurs when we try to start a request, which modifies the 
     * media, and conflicts with another fixed lba request. 
     * Since they are both fixed lba requests, they will always conflict, and thus 
     * the second fixed lba request should not be allowed to start. 
     * 
     * @todo add check for media modify operation.
     */
    /*! @todo We always check for other `fixed' requests.
     */
    if (FBE_FALSE && (ts_p->request_p->specification.lba_spec != FBE_RDGEN_LBA_SPEC_FIXED))
    {
        /* No conflict since the passed in ts is not fixed.
         */
        return FALSE;
    }

    /* The passed in ts is fixed lba.
     * Search for a conflict. 
     */
    for (current_queue_element_p = fbe_queue_front(&object_p->active_ts_head);
         current_queue_element_p != NULL;
         current_queue_element_p = fbe_queue_next(&object_p->active_ts_head, current_queue_element_p))
    {
        fbe_lba_t current_max_lba;

        current_ts_p = fbe_rdgen_ts_queue_element_to_ts_ptr(current_queue_element_p);
        if (ts_p == current_ts_p)
        {
            /* Self, just skip.
             */
            continue;
        }
        current_max_lba = fbe_rdgen_ts_get_max_lba(current_ts_p);

        /* Requests that are not checking cannot conflict.
         * If either thread does a check then they could conflict.
         */
        if ( (b_is_check_thread == FALSE) &&
             (fbe_rdgen_operation_is_checking(current_ts_p->operation) == FALSE) )
        {
            continue;
        }

        /* If this current ts is not fixed, we cannot have a fixed lba conflict.
         */
        if ((current_ts_p->request_p->specification.lba_spec != FBE_RDGEN_LBA_SPEC_FIXED) ||
            (current_max_lba != fbe_rdgen_ts_get_min_lba(current_ts_p)))
        {
            continue;
        }

        /* We check to see if it is possible for these threads to conflict. 
         */
        current_start = fbe_rdgen_ts_get_min_lba(current_ts_p);
        current_end = fbe_rdgen_ts_get_min_lba(current_ts_p) + current_ts_p->request_p->specification.max_blocks - 1;

        if ((check_end < current_start) ||
            (check_start > current_end))
        {
            /* No conflict.
             */
        }
        else
        {
            /* Conflict and we are both fixed lbas,
             * we will do not allow them to run together
             */
            return FBE_TRUE;
        }
    } /* end loop over all ts. */

    /* No fixed lba conflict.
     */
    return FBE_FALSE;
}                               
/*******************************
 * fbe_rden_ts_fixed_lba_conflict_check 
 *******************************/

/*!****************************************************************************
 * fbe_rdgen_object_dispatch_waiters
 *****************************************************************************
 * @brief
 *   Try to dispatch any waiting requests.
 *
 * @param - ts_p - Current rdgen ts.
 *   
 * @return fbe_status_t
 *         FBE_STATUS_OK - Done, no dispatches.
 *         FBE_STATUS_GENERIC_FAILURE - Error occurred.
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 *****************************************************************************/

static fbe_status_t fbe_rdgen_object_dispatch_waiters(fbe_rdgen_object_t *object_p)
{
    fbe_status_t status;
    fbe_rdgen_ts_t *current_ts_p = NULL;
    fbe_queue_element_t *current_queue_element_p = NULL;

    /* The passed in ts is fixed lba.
     * Search for a conflict. 
     */
    for (current_queue_element_p = fbe_queue_front(&object_p->active_ts_head);
         current_queue_element_p != NULL;
         current_queue_element_p = fbe_queue_next(&object_p->active_ts_head, current_queue_element_p))
    {
        current_ts_p = fbe_rdgen_ts_queue_element_to_ts_ptr(current_queue_element_p);

        /* If we find someone that is waiting and they do not have a conflict, then
         * clear their waiting flag and wake them up. 
         */
        if (fbe_rdgen_ts_is_flag_set(current_ts_p, FBE_RDGEN_TS_FLAGS_WAITING_LOCK) &&
            !fbe_rdgen_ts_conflict_check(current_ts_p))
        {
            /* Clear the waiting flag so others TS will treat us as granted. 
             * Set the waited flag so this TS will not try to restart waiters (and yeild). 
             */
            fbe_rdgen_ts_clear_flag(current_ts_p, FBE_RDGEN_TS_FLAGS_WAITING_LOCK);
            fbe_rdgen_ts_set_flag(current_ts_p, FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED);
            /* Simply put this on a thread to restart it.
             */
            status = fbe_rdgen_ts_enqueue_to_thread(current_ts_p);
        }
    } /* end loop over all ts. */
    return FBE_STATUS_OK;
}
/*******************************
 * fbe_rdgen_object_dispatch_waiters 
 *******************************/

/****************************************************************************
 * fbe_rdgen_ts_resolve_conflict
 ****************************************************************************
 * @brief
 *   Check an entire GEN DB's queue for conflicts with 
 *   the passed in iorb.  Skip the passed in flts.
 *
 * @param ts_p - Current ts
 *   
 * @return fbe_rdgen_ts_state_status_t
 *  FBE_STATUS_OK - No conflict continue with ts.
 *  FBE_STATUS_GENERIC_FAILURE - Error encountered, need to continue
 *                                        executing.
 *  FBE_STATUS_PENDING - Conflict wait for other TS to wake us up.
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 ****************************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_resolve_conflict(fbe_rdgen_ts_t * ts_p)
{
    fbe_status_t return_status = FBE_STATUS_OK;
    fbe_u32_t loop_count = 0;
    fbe_u32_t alignment_blocks = 0;
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_lba_t min_lba = fbe_rdgen_ts_get_min_lba(ts_p);
    fbe_u32_t max_loops = ts_p->object_p->active_ts_count;
    fbe_rdgen_object_t *object_p = ts_p->object_p;

    /* If we came from the peer, the peer has already resolved the conflict.
     * We explicitly should not try to resolve the conflict now. 
     */
    if (ts_p->request_p->specification.b_from_peer)
    {
        /* Return false to indicate no conflict.
         */
        return FBE_STATUS_OK;
    }
    /* Lock the object since we are traversing the queues.
     */
    fbe_rdgen_object_lock(object_p);

    if (fbe_rdgen_ts_is_flag_set(ts_p, FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED))
    {
        fbe_rdgen_object_unlock(object_p);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, fbe_rdgen_object_get_object_id(object_p),
                                "rslve_cnflct: lock obtained already lba: 0x%llx bl: 0x%llx, fl: 0x%x\n",
                                (unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks, ts_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Kick off any requests that are waiting for us.
     * We only kick off requests if we did not already wait for the lock 
     * since we only want to yeild the first time we come through here. 
     */
    fbe_rdgen_object_dispatch_waiters(ts_p->object_p);

    /* if we are a fixed LBA request and we are conflicting with other
    * fixed LBA thread then we will wait for our turn.
    */
    if (fbe_rden_ts_fixed_lba_conflict_check(ts_p))
    {
        fbe_rdgen_ts_mark_waiting_lock(ts_p);
        fbe_rdgen_object_unlock(object_p);
        return FBE_STATUS_PENDING;
    }

    if (fbe_rdgen_ts_conflict_check(ts_p))
    {
        if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                   FBE_RDGEN_OPTIONS_CREATE_REGIONS) ||
            fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                   FBE_RDGEN_OPTIONS_USE_REGIONS)){
            /* Region based requests need to just wait since we cannot modify the LBA.
             */
            fbe_rdgen_ts_mark_waiting_lock(ts_p);
            fbe_rdgen_object_unlock(object_p);
            return FBE_STATUS_PENDING;
        }
        /* Get the block count
         */
        if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0)
        {
            /* For aligned requests cut down to the aligned size.
             */
            ts_p->blocks = alignment_blocks;
        }
        else
        {
            /* Cut down the profile of this I/O, so that it's not likely to conflict.
             */
            ts_p->blocks = ts_p->request_p->specification.min_blocks;
        }

        /* If we are fixed 
         * or our range is equal to our min blocks (which means we are essentially fixed. 
         * Then only execute this check once. 
         */
        if ((ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_FIXED) ||
            ((max_lba - min_lba + 1) == ts_p->blocks)) 
        {
            /* We cannot change the lba.  We might have changed the block size above.
             * Return if we still have a conflict. 
             */
            fbe_rdgen_ts_mark_waiting_lock(ts_p);
            fbe_rdgen_object_unlock(object_p);
            return FBE_STATUS_PENDING;
        }

        /* We have a conflict.
         * We try to resolve the conflict by simply picking a new lba.
         */
        ts_p->lba = min_lba;
        do
        {
            fbe_lba_t max_blocks_lba_range = (max_lba - min_lba + 1);
            
            /* Get the lba randomly.
             * Note we only generate a request that fits within the capacity.
             * Subtract the current blocks from the full range so that we
             * don't exceed capacity.
             */
            if (ts_p->blocks >= max_blocks_lba_range)
            {
                /* Reduce blocks
                 */
                ts_p->blocks = (max_blocks_lba_range / 2);
            }
            max_blocks_lba_range -= ts_p->blocks;
            ts_p->lba = min_lba + (fbe_rtl_random(&ts_p->random_context) % max_blocks_lba_range);

            if ((alignment_blocks != 0)        &&
                (ts_p->lba % alignment_blocks)    )
            {
                /* If we are supposed to be aligned, align it.
                 */
                ts_p->lba -= (ts_p->lba % alignment_blocks);
            }
            if (!(fbe_rdgen_ts_conflict_check(ts_p))) 
            {
                break;  //stop adjusting if no conflict
            }
            ++loop_count;
        }
        while (loop_count < max_loops);        

        if (RDGEN_COND((ts_p->lba < min_lba) && 
                       (ts_p->lba < ts_p->request_p->specification.start_lba)))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe rdgen ts LBA 0x%llx is either < min_lba 0x%llx or"
                                    " request's start lba 0x%llx. \n", 
                                    __FUNCTION__,
				    (unsigned long long)ts_p->lba,
				    (unsigned long long)min_lba,
				    (unsigned long long)ts_p->request_p->specification.start_lba);
            ts_p->status = FBE_STATUS_GENERIC_FAILURE;
            fbe_rdgen_object_unlock(object_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (RDGEN_COND((ts_p->lba + ts_p->blocks - 1) > max_lba))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe rdgen ts LBA 0x%llx + blocks 0x%llx > max_lba 0x%llx. \n", 
                                    __FUNCTION__,
				    (unsigned long long)ts_p->lba,
				    (unsigned long long)ts_p->blocks,
				    (unsigned long long)max_lba);
            ts_p->status = FBE_STATUS_GENERIC_FAILURE;
            fbe_rdgen_object_unlock(object_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Validate that the request is aligned.
     */
    if (alignment_blocks)
    {
        if (RDGEN_COND((ts_p->lba % ts_p->request_p->specification.alignment_blocks) != 0) ||
            RDGEN_COND((ts_p->lba % ts_p->object_p->optimum_block_size) != 0)                 )
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe rdgen ts LBA 0x%llx is not aligned to request's alignment blocks 0x%x. \n", 
                                    __FUNCTION__,
				    (unsigned long long)ts_p->lba, alignment_blocks);
            ts_p->status = FBE_STATUS_GENERIC_FAILURE;
            fbe_rdgen_object_unlock(object_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (RDGEN_COND((ts_p->blocks % ts_p->request_p->specification.alignment_blocks) != 0) || 
            RDGEN_COND((ts_p->blocks % ts_p->object_p->optimum_block_size) != 0)                 )
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: fbe rdgen ts blocks 0x%llx are not aligned to request's alignment blocks 0x%x. \n", 
                                    __FUNCTION__,
				    (unsigned long long)ts_p->blocks, alignment_blocks);
            ts_p->status = FBE_STATUS_GENERIC_FAILURE;
            fbe_rdgen_object_unlock(object_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (loop_count >= max_loops)
    {
        /* Unable to resolve conflict.
         */
        return_status = FBE_STATUS_PENDING;
        fbe_rdgen_ts_mark_waiting_lock(ts_p);
    }
    else
    {
        /* Mark that we have the lock now.
         */
        fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED);
    }

    if (RDGEN_COND(((ts_p->lba + ts_p->blocks) > ts_p->object_p->capacity) &&
                   ((ts_p->request_p->specification.max_lba == FBE_LBA_INVALID) ||
                   ((ts_p->lba + ts_p->blocks - 1) > ts_p->request_p->specification.max_lba))))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen ts lba 0x%llx + blocks 0x%llx > rdgen object's capacity 0x%llx. \n", 
                                __FUNCTION__, (unsigned long long)ts_p->lba, (unsigned long long)ts_p->blocks, (unsigned long long)ts_p->object_p->capacity );
        ts_p->status = FBE_STATUS_GENERIC_FAILURE;
        fbe_rdgen_object_unlock(object_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_rdgen_object_unlock(object_p);
    return return_status;
}                               
/*******************************
 * fbe_rdgen_ts_resolve_conflict
 *******************************/
/*!**************************************************************
 * fbe_rdgen_ts_is_time_to_trace()
 ****************************************************************
 * @brief
 *  Returns true if it is time to trace for this ts.
 *
 * @param  ts_p - Current I/O.
 *
 * @return FBE_TRUE if it is time to trace.
 *
 * @author
 *  8/20/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_rdgen_ts_is_time_to_trace(fbe_rdgen_ts_t *ts_p)
{
    fbe_bool_t b_status = FBE_FALSE; /* Assume false. Set true below if time to trace. */

	/* Peter Puhov. Temporary disable trace while doing performance */
	if(fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE)){
		return FBE_FALSE;
	}

    if ((fbe_rdgen_get_trace_per_pass_count() != 0) &&
        (fbe_rdgen_ts_get_pass_count(ts_p) % fbe_rdgen_get_trace_per_pass_count()) == 0)
    {
        b_status = FBE_TRUE;
    }
    if ((fbe_rdgen_get_trace_per_io_count() != 0) &&
        (fbe_rdgen_ts_get_io_count(ts_p) % fbe_rdgen_get_trace_per_io_count()) == 0)
    {
        b_status = FBE_TRUE;
    }
    
    if (fbe_rdgen_get_seconds_per_trace() != 0)
    {
        fbe_u32_t elapsed_seconds;
        elapsed_seconds = fbe_get_elapsed_seconds(fbe_rdgen_get_last_trace_time());
        if (elapsed_seconds > fbe_rdgen_get_seconds_per_trace())
        {
            b_status = FBE_TRUE;
            fbe_rdgen_set_last_trace_time(fbe_get_time());
        }
    }
    return b_status;
}
/******************************************
 * end fbe_rdgen_ts_is_time_to_trace()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_initialize_blocks_remaining()
 ****************************************************************
 * @brief
 *  Initialize the number of blocks remaining for this pass.
 *
 * @param ts_p - current io.               
 *
 * @return None.   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_initialize_blocks_remaining(fbe_rdgen_ts_t *ts_p)
{
    ts_p->current_lba = ts_p->lba;
    ts_p->current_blocks = ts_p->blocks;
    ts_p->blocks_remaining = ts_p->blocks;
    return;
}
/******************************************
 * end fbe_rdgen_ts_initialize_blocks_remaining()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_get_current_blocks()
 ****************************************************************
 * @brief
 *  Get the number of current blocks based on our breakup count.
 *
 * @param ts_p - current io.               
 *
 * @return None.   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_ts_get_current_blocks(fbe_rdgen_ts_t *ts_p)
{
    if (ts_p->breakup_count > 1)
    {
        /* If we're breaking up this request, 
         * then set blocks for the memory allocation below.
         * Account for the fact that this I/O may be broken up even further.
         */
        fbe_lba_t new_blocks = ts_p->blocks / ts_p->breakup_count;
        /* Add on one if we are not aligned.
         */
        new_blocks += (ts_p->blocks % ts_p->breakup_count) ? 1 : 0;
        ts_p->current_blocks = FBE_MIN( new_blocks, FBE_RDGEN_MAX_BLOCKS );
    }

    /* Our current blocks can't be beyond our blocks remaining.
     */
    ts_p->current_blocks = (ts_p->blocks / ts_p->breakup_count);
    ts_p->current_blocks += (ts_p->blocks % ts_p->breakup_count) ? 1 : 0;

    ts_p->current_blocks = FBE_MIN(ts_p->blocks_remaining, ts_p->current_blocks);
    return;
}
/******************************************
 * end fbe_rdgen_ts_get_current_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_update_blocks_remaining()
 ****************************************************************
 * @brief
 *  Update the number of blocks remaining after having transferred
 *  current blocks.
 *
 * @param ts_p - current io.               
 *
 * @return .fbe_status_t
            - FBE_STATUS_GENERIC_FAILURE - For error condition.
            - FBE_STATUS_OK - No error.
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_update_blocks_remaining(fbe_rdgen_ts_t *ts_p)
{
    if (RDGEN_COND(ts_p->current_blocks > ts_p->blocks_remaining))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen ts current blocks 0x%llx > blocks_remaining 0x%llx. \n", 
                                __FUNCTION__,
				(unsigned long long)ts_p->current_blocks,
				(unsigned long long)ts_p->blocks_remaining);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ts_p->blocks_remaining -= ts_p->current_blocks;
    ts_p->current_lba += ts_p->current_blocks;
    ts_p->current_blocks = (fbe_u32_t)FBE_MIN(ts_p->blocks_remaining, ts_p->current_blocks);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_update_blocks_remaining()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_set_time_to_abort()
 ****************************************************************
 * @brief
 * This function sets time_to_abort in rdgen ts as per user entry
 * if provided or default value
 *
 * @param ts_p - current io.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/3/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_ts_set_time_to_abort(fbe_rdgen_ts_t *ts_p)
{
    fbe_time_t random_abort_time;
    fbe_time_t current_time = fbe_get_time_in_us();

    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_RANDOM_ABORT) &&
       (ts_p->request_p->specification.msecs_to_abort != 0))
    {
        /* abort the I/Os randomly using user entered value to generate the
         * random time to abort
         */
        random_abort_time = (fbe_rtl_random(&ts_p->random_context) % 
                             ts_p->request_p->specification.msecs_to_abort) + 1;
    }
    else if(fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                   FBE_RDGEN_OPTIONS_RANDOM_ABORT) &&
            (ts_p->request_p->specification.msecs_to_abort == 0))
    {
        /* User wants to abort all the I/Os immediately
         */
        random_abort_time = 0;
    }
    else if (ts_p->request_p->specification.msecs_to_abort != 0)
    {
        /*Just abort with fixed time given by user, it will not be 
         * be random abort in this case
         */
        random_abort_time = ts_p->request_p->specification.msecs_to_abort;
    }
    else
    {
        /* User not interested in aborting the I/Os randomly or
         * with fixed time, just use the rdgen default value to 
         * abort the I/Os
         */
        random_abort_time = fbe_rdgen_get_max_io_msecs();
    }

    ts_p->time_to_abort = current_time + (random_abort_time * FBE_TIME_MILLISECONDS_PER_MICROSECOND);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_ts_set_time_to_abort()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_is_playback()
 ****************************************************************
 * @brief
 *  Determine if this is a playback request.
 *
 * @param ts_p - The current thread.
 *
 * @return fbe_bool_t   
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_rdgen_ts_is_playback(fbe_rdgen_ts_t *ts_p)
{
    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_FILE)){
        return FBE_TRUE;
    }
    else {
        return FBE_FALSE;
    }
}
/******************************************
 * end fbe_rdgen_ts_is_playback()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_generate_playback()
 ****************************************************************
 * @brief
 *  Generate the information on the next I/O for the playback.
 *
 * @param ts_p - The current thread.               
 *
 * @return fbe_rdgen_ts_state_status_t  
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_generate_playback(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_t state = NULL;
    fbe_block_count_t blocks = ts_p->blocks;
    fbe_cpu_id_t cpu_id = ts_p->core_num;

    if (fbe_rdgen_request_is_complete(ts_p->request_p)){
        /* We were either stopped or we took an error on the file read.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_playback_generate_wait(ts_p)){
        /* Stay in this state until we get the next records.
         */
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }

    /* If the size of memory changes, we need to re-allocate.
     */
    if (ts_p->blocks > blocks){
        fbe_rdgen_ts_free_memory(ts_p);
    }

    state = fbe_rdgen_ts_get_state_for_opcode(ts_p->operation, ts_p->io_interface);
    if (state == NULL){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: Unable to start thread for opcode %d\n", 
                                __FUNCTION__, ts_p->operation);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_invalid_command);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    fbe_rdgen_ts_set_state(ts_p, state);

    /* If the core number changed, then change cores now.
     */
    if (cpu_id != ts_p->core_num){
        fbe_rdgen_ts_enqueue_to_thread(ts_p);
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }
    else {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
}
/******************************************
 * end fbe_rdgen_ts_generate_playback()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_timer_completion()
 ****************************************************************
 * @brief
 *  This is the timer completion function that runs when
 *  the timer expires.
 *
 * @param timer_p - pointer to the timer.
 * @param timer_completion_context - rdgen ts.
 *
 * @return none.
 *
 * @author
 *  4/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_ts_timer_completion(fbe_packet_t * packet, 
                                                  fbe_packet_completion_context_t completion_context)
{
    fbe_rdgen_ts_t *ts_p = (fbe_rdgen_ts_t*)completion_context;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                                "%s: delay complete for ts: %p thread: %d\n", 
                                __FUNCTION__, ts_p, ts_p->thread_num);
    fbe_rdgen_ts_enqueue_to_thread(ts_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_rdgen_ts_timer_completion()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_set_first_state()
 ****************************************************************
 * @brief
 *  Determine the first state for a TS.
 *  If it is a playback we go through a common state.
 *  Otherwise we just set the state passed in.
 *
 * @param ts_p
 * @param state
 * 
 * @return fbe_rdgen_ts_state_status_t
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_set_first_state(fbe_rdgen_ts_t *ts_p,
                                                         fbe_rdgen_ts_state_t state)
{
    fbe_status_t status;
    if (fbe_rdgen_ts_is_playback(ts_p)){
        /* A playback request goes through the generate state always in between requests.
         */
        state = fbe_rdgen_ts_generate_playback;
    } else if (ts_p->request_p->specification.msecs_to_delay != 0){
        fbe_u32_t msecs_to_delay = (fbe_u32_t)ts_p->request_p->specification.msecs_to_delay;
        if (fbe_rdgen_specification_extra_options_is_set(&ts_p->request_p->specification,
                                                         FBE_RDGEN_EXTRA_OPTIONS_RANDOM_DELAY)) {
            msecs_to_delay = fbe_random() % msecs_to_delay;
        }
        fbe_rdgen_ts_set_state(ts_p, state);
        if (msecs_to_delay == 0) {
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        } else {

            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id, 
                                    "%s: start delay for ts: %p thread: %d ms: %d core: %d\n", 
                                    __FUNCTION__, ts_p, ts_p->thread_num, msecs_to_delay, ts_p->core_num);
            /* Wait for some amount of time before restarting this I/O.
             */
            status = fbe_transport_set_timer(fbe_rdgen_ts_get_packet(ts_p),
                                             (fbe_u32_t) msecs_to_delay,
                                             fbe_rdgen_ts_timer_completion,
                                             ts_p);
            if (status != FBE_STATUS_OK){
                return status; 
            }
            return FBE_RDGEN_TS_STATE_STATUS_WAITING;
        }
    }
    fbe_rdgen_ts_set_state(ts_p, state);
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_set_first_state()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_init_random()
 ****************************************************************
 * @brief
 *  Determine the first state for a TS.
 *  If it is a playback we go through a common state.
 *  Otherwise we just set the state passed in.
 *
 * @param ts_p
 * @param state
 * 
 * @return fbe_rdgen_ts_state_status_t
 *
 * @author
 *  9/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_ts_init_random(fbe_rdgen_ts_t *ts_p)
{
    fbe_lba_t max_lba = fbe_rdgen_ts_get_max_lba(ts_p);
    fbe_lba_t min_lba = fbe_rdgen_ts_get_min_lba(ts_p);
    fbe_block_count_t max_blocks_lba_range = (fbe_block_count_t)(max_lba - min_lba + 1);
    fbe_lba_t capacity;
    fbe_block_count_t max_blocks = ts_p->request_p->specification.max_blocks;
    fbe_u64_t bit;
    fbe_u32_t alignment_blocks;

    if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_FIXED_RANDOM_NUMBERS)
    {
        /* We seed based on the value passed in for the seed base,
         * plus something unique, object id and the thread number. 
         * This ensures that each thread we start has a different seed, which should guarantee 
         * a different sequence of random numbers. 
         */
        ts_p->random_context = ts_p->object_p->object_id + ts_p->thread_num + ts_p->request_p->specification.random_seed_base;
    }
    else 
    {
        /* Initialize the random seed from a random number.
         */
        ts_p->random_context = fbe_rdgen_get_random_seed();
    }
    if ((alignment_blocks = fbe_rdgen_ts_get_alignment_blocks(ts_p)) > 0) {
        capacity = (max_blocks_lba_range - max_blocks) / alignment_blocks;
    }
    else {
        capacity = max_blocks_lba_range - max_blocks;
    }
    /* Determine the mask to use based on the capacity used for I/O.
     * Find first the number of the most significant bit.
     */
    bit = 1;
    while (capacity != 0){
        capacity &= ~(1 << bit) - 1;
        bit = bit + 1;
    }
    /* We create a mask which includes all but the most significant bit of capacity.
     */
    ts_p->random_mask = ((1 << (bit - 1)) - 1);
    if (ts_p->random_mask == 0){
        ts_p->random_mask = 1;
    }
}
/******************************************
 * end fbe_rdgen_ts_init_random()
 ******************************************/
/*************************
 * end file fbe_rdgen_ts.c
 *************************/


