/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_cmi_perf.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen CMI performance testing implementation
 *
 * @version
 *   9/06/2011:  Created. Shay Harel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_cmi.h"


typedef struct fbe_rdgen_cmi_perf_context_s{
	fbe_thread_t					thread;
	fbe_atomic_t *					complete_count;
	fbe_u32_t						cpu_id;
	fbe_u32_t						run_time_msec;
	fbe_rdgen_control_cmi_perf_t *	user_request;
}fbe_rdgen_cmi_perf_context_t;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_rdgen_service_test_cmi_performance_run_test(fbe_rdgen_control_cmi_perf_t *cmi_perf_p);
static void fbe_rdgen_service_test_cmi_performance_thread(void *context);

/*!**************************************************************
 * fbe_rdgen_service_test_cmi_performance()
 ****************************************************************
 * @brief
 *  receive user packet and call the test 
 *
 * @param ts_p - packet from user              
 *
 * @return fbe status
 *
 * @author
 *  9/6/2011 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t fbe_rdgen_service_test_cmi_performance(fbe_packet_t *packet_p)
{
    fbe_payload_control_operation_t * 		control_operation_p = NULL;
    fbe_rdgen_control_cmi_perf_t * 			cmi_perf_p = NULL;
    fbe_u32_t 								len;
    fbe_status_t 							status;
	fbe_payload_ex_t  *					sep_payload_p = NULL;
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    
    fbe_payload_control_get_buffer(control_operation_p, &cmi_perf_p);
    if (cmi_perf_p == NULL){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len < sizeof(fbe_rdgen_control_cmi_perf_t)){
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu \n", __FUNCTION__, len,
			    (unsigned long long)sizeof(fbe_rdgen_control_get_object_info_t));

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_OK;
    }

	if (!fbe_cmi_is_peer_alive()) {
		 fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Peer is not there, can't run test \n",
			             __FUNCTION__);

        fbe_rdgen_complete_packet(packet_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES, FBE_STATUS_INSUFFICIENT_RESOURCES);
        return FBE_STATUS_OK;

	}

	/*let's start the test, this is blocking until we are done*/
	status = fbe_rdgen_service_test_cmi_performance_run_test(cmi_perf_p);

    fbe_rdgen_complete_packet(packet_p, status == FBE_STATUS_OK ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE, status);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_service_test_cmi_performance()
 ******************************************/


/*!**************************************************************
 * fbe_rdgen_service_test_cmi_performance_run_test()
 ****************************************************************
 * @brief
 *  run the test
 *
 * @param cmi_perf_p - structure with test data            
 *
 * @return fbe status
 *
 * @author
 *  9/6/2011 - Created. Shay Harel
 *
 ****************************************************************/
static fbe_status_t fbe_rdgen_service_test_cmi_performance_run_test(fbe_rdgen_control_cmi_perf_t *cmi_perf_p)
{
    fbe_u32_t 						thread_num;
    fbe_u32_t 						max_threads;
	EMCPAL_STATUS 						nt_status;
	fbe_rdgen_cmi_perf_context_t	cmi_perf_context[FBE_CPU_ID_MAX];
	fbe_atomic_t					completion_cout = 0;
	fbe_atomic_t					current_count = 0;
	fbe_u32_t						wait_count = 0;
	fbe_u32_t						thread_to_wait_for = 0;

	fbe_zero_memory(&cmi_perf_context, FBE_CPU_ID_MAX * sizeof(fbe_rdgen_cmi_perf_context_t));

    /*let's see if the user wanted to run with cpu count or with the 3rd argument as the cpu mask*/
	if (cmi_perf_p->with_mask) {
		fbe_u32_t core_number = 0;

		while (cmi_perf_p->thread_count != 0) {
			if (cmi_perf_p->thread_count == 0x1) {
				break;
			}
			core_number++;
			cmi_perf_p->thread_count >>= 0x1;
		}

		thread_to_wait_for = 1;
		cmi_perf_p->thread_count = core_number + 1;/*restore so the loop below can do some printouts*/

		cmi_perf_context[core_number].cpu_id = core_number;
		cmi_perf_context[core_number].complete_count = &completion_cout;
		cmi_perf_context[core_number].user_request = cmi_perf_p;
        
		nt_status = fbe_thread_init(&cmi_perf_context[core_number].thread, "fbe_rdgen_cmiperf", fbe_rdgen_service_test_cmi_performance_thread, (void *)&cmi_perf_context[core_number]);
		if (nt_status != EMCPAL_STATUS_SUCCESS) {
			fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s: fbe_thread_init fail\n", __FUNCTION__);

			/*fix me, clean all these who started*/
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}

	}else{

		max_threads = fbe_get_cpu_count();
		if (cmi_perf_p->thread_count > max_threads) {
			fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
									FBE_TRACE_MESSAGE_ID_INFO,
									"%s: we have only %d cpus and user wanted %d threads\n", 
									__FUNCTION__, max_threads, cmi_perf_p->thread_count );
	
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}

		/*let's kick off the threads*/
		for (thread_num = 0; thread_num < cmi_perf_p->thread_count; thread_num++)
		{
			cmi_perf_context[thread_num].cpu_id = thread_num;
			cmi_perf_context[thread_num].complete_count = &completion_cout;
			cmi_perf_context[thread_num].user_request = cmi_perf_p;
            thread_to_wait_for ++;
	
			nt_status = fbe_thread_init(&cmi_perf_context[thread_num].thread, "fbe_rdgen_cmiperf", fbe_rdgen_service_test_cmi_performance_thread, (void *)&cmi_perf_context[thread_num]);
			if (nt_status != EMCPAL_STATUS_SUCCESS) {
				fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s: fbe_thread_init fail\n", __FUNCTION__);
	
				/*fix me, clean all these who started*/
				return FBE_STATUS_INSUFFICIENT_RESOURCES;
			}
			
		}
	}

	/*now let's just wait for them to finish*/
	do {
		fbe_thread_delay(1000);
		fbe_atomic_exchange(&current_count, completion_cout);
		wait_count++;
	} while ((current_count != thread_to_wait_for) && (wait_count < 300));

	if (wait_count == 300) {

		fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s: not all threads finished in 5 minutes\n", __FUNCTION__);

			/*fix me, clean all these who started*/
			return FBE_STATUS_GENERIC_FAILURE;
	}

	/*copy data to structure*/
	fbe_zero_memory(&cmi_perf_p->run_time_in_msec, sizeof(fbe_u32_t) * FBE_CPU_ID_MAX);
	for (thread_num = 0; thread_num < cmi_perf_p->thread_count; thread_num++){
		cmi_perf_p->run_time_in_msec[thread_num] = cmi_perf_context[thread_num].run_time_msec;
	}


	return FBE_STATUS_OK;


}
/******************************************
 * end fbe_rdgen_service_test_cmi_performance_run_test()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_service_test_cmi_performance_thread()
 ****************************************************************
 * @brief
 *  the thread teh does teh work
 *
 * @param context - run data        
 *
 *
 * @author
 *  9/6/2011 - Created. Shay Harel
 *
 ****************************************************************/
static void fbe_rdgen_service_test_cmi_performance_thread(void *context)
{

	/*we want to be as clean as we can and use the rdgen cmi code so we'll stick a message type
	at the front of our payload*/

    fbe_cpu_affinity_mask_t 		cpu_affinity_mask = 0x1;
	fbe_rdgen_cmi_perf_context_t *	cmi_perf_context = (fbe_rdgen_cmi_perf_context_t *)context;
    fbe_u32_t						send_count = 0;
	fbe_u8_t *						send_buffer = NULL;
	fbe_semaphore_t					ack_sem;
	fbe_u32_t						min_allocation_count = sizeof(fbe_rdgen_cmi_message_type_t);
	fbe_u32_t						allocation_count = 0;
	fbe_rdgen_cmi_message_type_t *	rdgen_cmi_msg = NULL;
	fbe_time_t						start_time;
	fbe_u32_t						run_time_msec = 0;

	/*allocate a message to send in a loop*/
	if (min_allocation_count > cmi_perf_context->user_request->bytes_per_message) {
		/*user wants just a few bytes, but we need more just for house keeping, we'll have to do this minimum then*/
		allocation_count = min_allocation_count;
	}else{
		/*we can fit inside the user requested message our housekeeping code so let's just do that*/
		allocation_count = cmi_perf_context->user_request->bytes_per_message;
	}
	
	send_buffer = (fbe_u8_t *)fbe_memory_native_allocate(allocation_count);
	rdgen_cmi_msg = (fbe_rdgen_cmi_message_type_t *)send_buffer;
    *rdgen_cmi_msg = FBE_RDGEN_CMI_MESSAGE_TYPE_CMI_PERFORMANCE_TEST;

	fbe_semaphore_init(&ack_sem, 0, 1);

	/*set the thread affinity*/
	cpu_affinity_mask <<= cmi_perf_context->cpu_id;
	fbe_thread_set_affinity(&cmi_perf_context->thread, cpu_affinity_mask);

	fbe_thread_get_affinity(&cpu_affinity_mask);
	fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
				FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
				"%s Affinity 0x%llx\n", __FUNCTION__,
				(unsigned long long)cpu_affinity_mask);

	/*what is the time now*/
	start_time = fbe_get_time();
	
	/*and send the messages N times as the user requested*/
	for (send_count = 0; send_count < cmi_perf_context->user_request->number_of_messages; send_count++) {
		
		fbe_cmi_send_message(FBE_CMI_CLIENT_ID_RDGEN, 
							 allocation_count,
							 (fbe_cmi_message_t)send_buffer,
							 (fbe_cmi_event_callback_context_t)&ack_sem);

		/*wait for the semaphore which will get down on the callback*/
		fbe_semaphore_wait(&ack_sem, NULL);
	}

    run_time_msec = fbe_get_elapsed_milliseconds(start_time);

	cmi_perf_context->run_time_msec = run_time_msec;

	fbe_atomic_increment(cmi_perf_context->complete_count);

	fbe_memory_native_release(send_buffer);
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
	
}
/*************************
 * end file fbe_rdgen_cmi_perf.c
 *************************/


