/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_rdgen_request.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen request related functions.
 *
 * @version
 *   7/21/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 
/*!**************************************************************
 * fbe_rdgen_request_init()
 ****************************************************************
 * @brief
 *  This function initializes an rdgen request.
 *
 * @param request_p - request to initialize.
 * @param object_id - The object id to init.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_request_init(fbe_rdgen_request_t *request_p,
                            fbe_packet_t *packet_p,
                            fbe_rdgen_control_start_io_t *start_p)
{
    fbe_zero_memory(request_p, sizeof(fbe_rdgen_request_t));
    fbe_rdgen_request_set_magic(request_p);
    fbe_spinlock_init(&request_p->lock);
    fbe_queue_init(&request_p->active_ts_head);
    fbe_queue_element_init(&request_p->queue_element);

    request_p->specification = start_p->specification;
    request_p->filter = start_p->filter;
    request_p->packet_p = packet_p;
    fbe_rdgen_request_update_start_time(request_p);

    fbe_rdgen_request_set_min_rate_object(request_p, FBE_OBJECT_ID_INVALID);
    fbe_rdgen_request_set_max_rate_object(request_p, FBE_OBJECT_ID_INVALID);

    /* Setup the caterpillar lba for caterpillar lba specs.
     */
    if ((request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING) ||
        (request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING))
    {
        fbe_rdgen_request_set_caterpillar_lba(request_p, request_p->specification.start_lba);
    }
    return;
}
/******************************************
 * end fbe_rdgen_request_init()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_request_stop_io()
 ****************************************************************
 * @brief
 *  This function stops all I/O on an rdgen object.
 *
 * @param request_p - Object to stop I/O on.
 * 
 * @notes assumes request lock is held.
 * 
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_request_stop_io(fbe_rdgen_request_t *request_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;

    /* Wake up all the objects for this request.
     */
    queue_element_p = fbe_queue_front(&request_p->active_ts_head);
    while (queue_element_p != NULL)
    {
        ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
        fbe_rdgen_object_lock(ts_p->object_p);
        fbe_rdgen_object_reset_restart_time(ts_p->object_p);
        fbe_rdgen_object_set_max_threads(ts_p->object_p, fbe_rdgen_object_get_num_threads(ts_p->object_p));
        fbe_rdgen_object_enqueue_to_thread(ts_p->object_p);
        fbe_rdgen_object_unlock(ts_p->object_p);
        queue_element_p = fbe_queue_next(&request_p->active_ts_head, queue_element_p);
    }
    return;
}
/******************************************
 * end fbe_rdgen_request_stop_io()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_request_finished()
 ****************************************************************
 * @brief
 *  This is the final state of the rdgen ts.
 *
 * @param request_p - Current ts.
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_DONE
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_request_finished(fbe_rdgen_request_t *request_p)
{
    fbe_bool_t err_encountered = FBE_FALSE;
    fbe_packet_t *packet_p = request_p->packet_p;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    /* Pull this request off of the global list.
     */
    fbe_rdgen_lock();
    fbe_rdgen_dec_num_requests();
    fbe_rdgen_dequeue_request(request_p);
    fbe_rdgen_unlock();

    /* If we have not already finished the master packet, do so now.
     */
    if (packet_p != NULL)
    {
		fbe_packet_attr_t packet_attr;

		fbe_transport_get_packet_attr(packet_p, &packet_attr);

		/*we copy only if we have where to copy to, in come case the buffer of this packet has been released already
		since it's sent from FBE CLI and we don't want FBE CLI to block*/
		if (!(packet_attr & FBE_PACKET_FLAG_RDGEN)) {
	
			/* First copy the stats from the request to the passed in statistics.
			 */
			fbe_payload_control_operation_t * control_operation_p = NULL;
			fbe_rdgen_control_start_io_t * start_io_p = NULL;
			fbe_payload_ex_t  *payload_p = NULL;
	
			payload_p = fbe_transport_get_payload_ex(packet_p);
			control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
			fbe_payload_control_get_buffer(control_operation_p, &start_io_p);
	
			start_io_p->status = request_p->status;
			start_io_p->statistics.error_count = fbe_rdgen_request_get_num_errors(request_p);
			start_io_p->statistics.aborted_error_count = fbe_rdgen_request_get_num_aborted_errors(request_p);
			start_io_p->statistics.media_error_count = fbe_rdgen_request_get_num_media_errors(request_p);
			start_io_p->statistics.io_failure_error_count = fbe_rdgen_request_get_num_io_failure_errors(request_p);
			start_io_p->statistics.congested_err_count = fbe_rdgen_request_get_num_congested_errors(request_p);
			start_io_p->statistics.still_congested_err_count = fbe_rdgen_request_get_num_still_congested_errors(request_p);
			start_io_p->statistics.invalid_request_err_count = fbe_rdgen_request_get_num_invalid_request_errors(request_p);
			start_io_p->statistics.inv_blocks_count = fbe_rdgen_request_get_num_inv_blocks(request_p);
			start_io_p->statistics.bad_crc_blocks_count = fbe_rdgen_request_get_num_bad_crc_blocks(request_p);
			start_io_p->statistics.pass_count = fbe_rdgen_request_get_num_passes(request_p);
			start_io_p->statistics.io_count = fbe_rdgen_request_get_num_ios(request_p);
			start_io_p->statistics.lock_conflicts = fbe_rdgen_request_get_num_lock_conflicts(request_p);
			start_io_p->statistics.verify_errors = request_p->verify_errors;
			start_io_p->statistics.elapsed_msec = fbe_get_elapsed_milliseconds(fbe_rdgen_request_get_start_time(request_p));
	
			start_io_p->statistics.min_rate_ios = fbe_rdgen_request_get_min_rate_ios(request_p);
			start_io_p->statistics.min_rate_msec = fbe_rdgen_request_get_min_rate_msec(request_p);
			start_io_p->statistics.min_rate_object_id = fbe_rdgen_request_get_min_rate_object(request_p);
	
			start_io_p->statistics.max_rate_ios = fbe_rdgen_request_get_max_rate_ios(request_p);
			start_io_p->statistics.max_rate_msec = fbe_rdgen_request_get_max_rate_msec(request_p);
			start_io_p->statistics.max_rate_object_id = fbe_rdgen_request_get_max_rate_object(request_p);
			start_io_p->specification = request_p->specification;
		}else{
			fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s FBE_PACKET_FLAG_RDGEN flag is set, not reporting statistics\n", 
									__FUNCTION__);
		}

		if (request_p->err_count > 0)
		{
			fbe_char_t *type_string = "obj_id";
			fbe_object_id_t type_id = request_p->filter.object_id;

			/* Set status into the start io request. 
			 * This includes error status and other statistics. 
			 */
			if (type_id == FBE_OBJECT_ID_INVALID)
			{
				type_string = "class_id";
				type_id = request_p->filter.class_id;
			}
			fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO,
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s complete request to %s: 0x%x with: %d errors\n", 
									__FUNCTION__, type_string, type_id, 
									request_p->err_count);

			err_encountered = FBE_TRUE;
		}
	
		/* Destroy the request before we complete the packet. 
		 * Completing the packet will free this memory. 
		 */
		fbe_rdgen_request_destroy(request_p);
		

        if(err_encountered == FBE_TRUE)
        {
            fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        }
        else
        {
            fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_OK, FBE_STATUS_OK);
        }
    }
    else
    {
        fbe_rdgen_request_destroy(request_p);
    }
    return;
}
/******************************************
 * end fbe_rdgen_request_finished()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_request_check_finished()
 ****************************************************************
 * @brief
 *  This is the final state of the rdgen ts.
 *
 * @param request_p - Current ts.
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_DONE
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_request_check_finished(fbe_rdgen_request_t *request_p)
{
    fbe_bool_t b_finished = FBE_FALSE;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    /* Check to see if we are done.
     */
    fbe_rdgen_request_lock(request_p);
    if (fbe_rdgen_request_get_num_threads(request_p) == 0)
    {
        b_finished = FBE_TRUE;
    }
    fbe_rdgen_request_unlock(request_p);

    /* If we are done, then clean up and finish this request.
     */
    if (b_finished)
    {
        fbe_rdgen_request_finished(request_p);
    }
    return;
}
/******************************************
 * end fbe_rdgen_request_check_finished()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_request_destroy()
 ****************************************************************
 * @brief
 *  This function destroys an rdgen request.
 *
 * @param request_p - request to destroy.
 *
 * @return None.
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_request_destroy(fbe_rdgen_request_t *request_p)
{
    if (!fbe_rdgen_request_validate_magic(request_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: magic number is 0x%llx\n", __FUNCTION__, (unsigned long long)request_p->magic);
    }
    fbe_spinlock_destroy(&request_p->lock);
    fbe_queue_destroy(&request_p->active_ts_head);
    return;
}
/******************************************
 * end fbe_rdgen_request_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_request_save_status()
 ****************************************************************
 * @brief
 *  This function saves status from a completing rdgen ts.
 *
 * @param request_p - Request.
 * @param status_p - Ptr to status being saved.
 * 
 * @notes We assume that the request lock is held.
 *
 * @return None.
 *
 * @author
 *  8/11/2009 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_rdgen_request_save_status(fbe_rdgen_request_t *request_p,
                                   fbe_rdgen_status_t *status_p)
{
    request_p->status = *status_p;
    return;
}

/******************************************
 * end fbe_rdgen_request_save_status()
 ******************************************/


/*!**************************************************************
 * fbe_rdgen_request_update_verify_errors()
 ****************************************************************
 * @brief
 *  This function saves status from a completing rdgen ts.
 *
 * @param request_p - Request.
 * @param status_p - Ptr to verify errors.
 * 
 *
 * @return None.
 *
 * @author
 *  03/11/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
void fbe_rdgen_request_update_verify_errors(fbe_rdgen_request_t *request_p,
                                            fbe_raid_verify_error_counts_t *in_verify_errors_p)
{
    request_p->verify_errors.u_crc_count        += in_verify_errors_p->u_crc_count;
    request_p->verify_errors.c_crc_count        += in_verify_errors_p->c_crc_count;
    request_p->verify_errors.u_crc_multi_count  += in_verify_errors_p->u_crc_multi_count;
    request_p->verify_errors.c_crc_multi_count  += in_verify_errors_p->c_crc_multi_count;
    request_p->verify_errors.u_crc_single_count  += in_verify_errors_p->u_crc_single_count;
    request_p->verify_errors.c_crc_single_count += in_verify_errors_p->c_crc_single_count;
    request_p->verify_errors.u_coh_count        += in_verify_errors_p->u_coh_count;
    request_p->verify_errors.c_coh_count        += in_verify_errors_p->c_coh_count;
    request_p->verify_errors.u_ts_count        += in_verify_errors_p->u_ts_count;
    request_p->verify_errors.c_ts_count        += in_verify_errors_p->c_ts_count;
    request_p->verify_errors.u_ws_count        += in_verify_errors_p->u_ws_count;
    request_p->verify_errors.c_ws_count        += in_verify_errors_p->c_ws_count;
    request_p->verify_errors.u_ss_count        += in_verify_errors_p->u_ss_count;
    request_p->verify_errors.c_ss_count        += in_verify_errors_p->c_ss_count;
    request_p->verify_errors.u_ls_count        += in_verify_errors_p->u_ls_count;
    request_p->verify_errors.c_ls_count        += in_verify_errors_p->c_ls_count;
    request_p->verify_errors.u_media_count     += in_verify_errors_p->u_media_count;
    request_p->verify_errors.c_media_count     += in_verify_errors_p->c_media_count;
    request_p->verify_errors.c_soft_media_count += in_verify_errors_p->c_soft_media_count;
    request_p->verify_errors.retryable_errors += in_verify_errors_p->retryable_errors;
    request_p->verify_errors.non_retryable_errors += in_verify_errors_p->non_retryable_errors;
    request_p->verify_errors.shutdown_errors += in_verify_errors_p->shutdown_errors;
    request_p->verify_errors.invalidate_count  += in_verify_errors_p->invalidate_count;

    return;
}
/**************************************
 * end fbe_rdgen_request_update_verify_errors
 **************************************/

/*!**************************************************************
 * fbe_rdgen_request_save_ts_io_statistics()
 ****************************************************************
 * @brief
 *  Save the io_count and elapsed msec for a thread that
 *  is just finishing.
 *  The request keeps the min and max across all its threads.
 *
 * @param request_p - Current request to save stats in.
 * @param io_count - Current ios to save.
 * @param elapsed_msec - Current elapsed msec to save.
 * @param object_id - Object id for the io per sec to save.
 *
 * @return None.
 *
 * @author
 *  4/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_request_save_ts_io_statistics(fbe_rdgen_request_t *request_p,
                                             fbe_u32_t io_count,
                                             fbe_time_t elapsed_msec,
                                             fbe_object_id_t object_id)
{
    fbe_object_id_t min_object_id;
    fbe_u64_t min_io_per_sec = 0;
    fbe_object_id_t max_object_id;
    fbe_u64_t max_io_per_sec = 0;
    fbe_u64_t io_per_sec = 0;

    if (elapsed_msec)
    {
        io_per_sec = (io_count * FBE_TIME_MILLISECONDS_PER_SECOND) / elapsed_msec;
    }

    /* Determine if we need to set min io per sec and min object id.
     */
    min_object_id = fbe_rdgen_request_get_min_rate_object(request_p);

    if (min_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* It has not been set yet.
         */
        fbe_rdgen_request_set_min_rate_ios(request_p, io_count);
        fbe_rdgen_request_set_min_rate_msec(request_p, elapsed_msec);
        fbe_rdgen_request_set_min_rate_object(request_p, object_id);
    }
    else
    {
        /* Set it if the new io per sec is less that the current.
         */
        if (fbe_rdgen_request_get_min_rate_msec(request_p))
        {
            min_io_per_sec = (fbe_rdgen_request_get_min_rate_ios(request_p) * FBE_TIME_MILLISECONDS_PER_SECOND) /
                             fbe_rdgen_request_get_min_rate_msec(request_p);

        }
        if ((fbe_rdgen_request_get_min_rate_ios(request_p) == 0) ||
            (fbe_rdgen_request_get_min_rate_msec(request_p) == 0) ||
            (io_per_sec < min_io_per_sec))
        {
            fbe_rdgen_request_set_min_rate_ios(request_p, io_count);
            fbe_rdgen_request_set_min_rate_msec(request_p, elapsed_msec);
            fbe_rdgen_request_set_min_rate_object(request_p, object_id);
        }
    }

    /* Determine if we need to set max io per sec and max object id.
     */
    max_object_id = fbe_rdgen_request_get_max_rate_object(request_p);

    if (max_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* It has not been set yet.
         */
        fbe_rdgen_request_set_max_rate_ios(request_p, io_count);
        fbe_rdgen_request_set_max_rate_msec(request_p, elapsed_msec);
        fbe_rdgen_request_set_max_rate_object(request_p, object_id);
    }
    else
    {
        /* Set it if the new io per sec is less that the current.
         */
        if (fbe_rdgen_request_get_max_rate_msec(request_p))
        {
            max_io_per_sec = (fbe_rdgen_request_get_max_rate_ios(request_p) * FBE_TIME_MILLISECONDS_PER_SECOND) /
                             fbe_rdgen_request_get_max_rate_msec(request_p);
        }
        if (io_per_sec > max_io_per_sec)
        {
            fbe_rdgen_request_set_max_rate_ios(request_p, io_count);
            fbe_rdgen_request_set_max_rate_msec(request_p, elapsed_msec);
            fbe_rdgen_request_set_max_rate_object(request_p, object_id);
        }
    }
    return;
}
/******************************************
 * end fbe_rdgen_request_save_ts_io_statistics()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_request_stop_for_bdev()
 ****************************************************************
 * @brief
 *  This function stops all I/O on a block device request.
 *
 * @param request_p - Request to stop I/O on.
 * 
 * @notes assumes request lock is held.
 * 
 * @return None.
 *
 * @author
*  11/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
void fbe_rdgen_request_stop_for_bdev(fbe_rdgen_request_t *request_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
    fbe_rdgen_thread_t *thread_p;

    /* Wake up all the objects for this request.
     */
    queue_element_p = fbe_queue_front(&request_p->active_ts_head);
    while (queue_element_p != NULL)
    {
        ts_p = fbe_rdgen_ts_request_queue_element_to_ts_ptr(queue_element_p);
        thread_p = &ts_p->bdev_thread;
        queue_element_p = fbe_queue_next(&request_p->active_ts_head, queue_element_p);

        while (!fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED))
        {
            fbe_thread_delay(100);
        }
        fbe_thread_wait(&thread_p->thread_handle);
        fbe_thread_destroy(&thread_p->thread_handle);
        /* Destroy the remaining members.
         */
        fbe_queue_destroy(&thread_p->ts_queue_head);
        fbe_spinlock_destroy(&thread_p->lock);
        fbe_rendezvous_event_destroy(&thread_p->event);

        fbe_rdgen_ts_finished(ts_p);
    }
    return;
}
/******************************************
 * end fbe_rdgen_request_stop_for_bdev()
 ******************************************/

/*************************
 * end file fbe_rdgen_request.c
 *************************/


