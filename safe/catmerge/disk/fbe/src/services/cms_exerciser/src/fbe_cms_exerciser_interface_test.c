/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_exerciser_interface_test.c
 ***************************************************************************
 *
 *  Description
 *      implementation of interface tests
 *    
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_cms_exerciser_interface.h"
#include "fbe_base.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_cms_interface.h"
#include "fbe_cms_exerciser_private.h"
#include "fbe_notification.h"


/*local definitions*/
typedef enum exclusive_alloc_test_steps_e{
	EXCLUSIVE_TEST_ALLOC,
	EXCLUSIVE_TEST_COMMIT,
	EXCLUSIVE_TEST_RELEASE,
	EXCLUSIVE_TEST_DONE
}exclusive_alloc_test_steps_t;

typedef struct buffer_allocation_context_s{
	fbe_queue_element_t				queue_element;
	fbe_cms_buff_req_t				buffer_request;
	fbe_sg_element_t				sg_element[2];
	exclusive_alloc_test_steps_t	next_step;
	fbe_spinlock_t	*				lock;
	fbe_semaphore_t	*				sem;
	fbe_queue_head_t *				queue_head;
}buffer_allocation_context_t;


typedef struct exclusive_alloc_test_context_s{
    fbe_cms_exerciser_exclusive_alloc_test_t 	test_info;
	fbe_cpu_affinity_mask_t						cpu_to_affine;
	fbe_thread_t								thread_handle;
	fbe_u16_t									thread_number;
	buffer_allocation_context_t *				buffer_array;
}exclusive_alloc_test_context_t;


/*local stuff*/
static fbe_atomic_t		exclusive_alloc_test_in_progress = 0;
static fbe_atomic_t		exclusive_alloc_test_thread_done_count = 0;
static fbe_u64_t		exclusive_alloc_test_threads_to_run = 0;
static fbe_bool_t		exclusive_alloc_test_has_errors = FBE_FALSE;

/*local functions*/
static fbe_cms_exerciser_start_exclusive_alloc_test(fbe_cms_exerciser_exclusive_alloc_test_t *test_info);
static void fbe_cms_exerciser_exclusive_alloc_test_thread(void * context);
static fbe_status_t fbe_cms_exerciser_exclusive_alloc_test_completion(fbe_cms_buff_req_t * req_p, void * context);
static fbe_status_t fbe_cms_exerciser_exclusive_alloc_test_commit_completion(fbe_cms_buff_req_t * req_p, void * context);
static __forceinline buffer_allocation_context_t * cms_request_to_buffer_allocation_context(fbe_cms_buff_req_t * req_p);
static fbe_status_t fbe_cms_exerciser_exclusive_alloc_test_release_completion(fbe_cms_buff_req_t * req_p, void * context);
static fbe_cms_exerciser_exclusive_alloc_test_mark_thread_done(void);


/*******************************************************************************************************************************/

fbe_status_t fbe_cms_exerciser_control_run_exclusive_alloc_tests(fbe_packet_t * packet)
{
	fbe_status_t                				status = FBE_STATUS_OK;
    fbe_cms_exerciser_exclusive_alloc_test_t *	run_test_params = NULL;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_exerciser_get_payload(packet, (void **)&run_test_params, sizeof(fbe_cms_exerciser_exclusive_alloc_test_t));
    if ( status != FBE_STATUS_OK ){
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_exerciser_complete_packet(packet, status);
        return status;
    }

	/*and start the test which will start some threads that will do the work
	at the end of the tests, we'll send a notification to say the test is done*/
	status = fbe_cms_exerciser_start_exclusive_alloc_test(run_test_params);
	if ( status != FBE_STATUS_OK ){
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't start test\n", __FUNCTION__);
        cms_exerciser_complete_packet(packet, status);
        return status;
    }

	cms_exerciser_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cms_exerciser_start_exclusive_alloc_test(fbe_cms_exerciser_exclusive_alloc_test_t *test_info)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
	fbe_cpu_id_t 							i, n;
	fbe_cpu_id_t 							cpu_count = fbe_get_cpu_count();
    fbe_atomic_t							test_count = 0;
	fbe_cpu_affinity_mask_t					mask = 0x1;
	exclusive_alloc_test_context_t *		test_context = NULL;
	fbe_u16_t								thread_number = 0;
    fbe_cms_control_get_access_func_ptrs_t *func_ptr = cms_exerciser_get_access_lib_func_ptrs();
    
	test_count = fbe_atomic_exchange(&exclusive_alloc_test_in_progress, 1);
	if (test_count == 1) {
		cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: test already runs\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	exclusive_alloc_test_has_errors = FBE_FALSE;

    status = func_ptr->register_client_func(FBE_CMS_CLIENT_INTERNAL,
                                            FBE_CMS_CLIENT_POLICY_EXPLICIT,
                                            NULL,    /* Event calback  func*/
                                            0,       /* mem reservation */
                                            0,       /* mem limit */
                                            0,       /* min owners */
                                            0,       /* max owners */
                                            0);      /* avg owners */

	/*calculate how many we are going to have, we have to set the number ahead of time*/
	for(i = 0; i < cpu_count; i++){
		/*do we want to use this cpu ?*/
		if (mask & test_info->cpu_mask) {
			exclusive_alloc_test_threads_to_run += test_info->threads_per_cpu;
		}

		mask <<= 1;
	}
	
    mask = 0x1;
    /* Start thread*/
	for(i = 0; i < cpu_count; i++){
		/*do we want to use this cpu ?*/
		if (mask & test_info->cpu_mask) {
			/*how many threads per cpu ?*/
			for (n = 0; n < test_info->threads_per_cpu; n++) {
				test_context = (exclusive_alloc_test_context_t *)fbe_memory_ex_allocate(sizeof(exclusive_alloc_test_context_t));
				if (test_context == NULL) {
					/*We will have a leak here for the threads that already started so let's fix it later wehn we have time :)*/
					return FBE_STATUS_GENERIC_FAILURE;
				}

				test_context->cpu_to_affine = mask;
				test_context->thread_number = thread_number;
				fbe_copy_memory(&test_context->test_info, test_info, sizeof(fbe_cms_exerciser_exclusive_alloc_test_t));
				fbe_thread_init(&test_context->thread_handle, fbe_cms_exerciser_exclusive_alloc_test_thread, (void*)(fbe_ptrhld_t)test_context);
                thread_number++;
			}
		}

		mask <<= 1;
	}

	return FBE_STATUS_OK;

}

static void fbe_cms_exerciser_exclusive_alloc_test_thread(void * context)
{
	exclusive_alloc_test_context_t *			test_context = (exclusive_alloc_test_context_t *)context;
	fbe_u32_t									alloc_count = 0;
	fbe_cms_control_get_access_func_ptrs_t * 	func_ptr = cms_exerciser_get_access_lib_func_ptrs();
    fbe_u64_t									owner_id;
	fbe_cms_alloc_id_t							alloc_id;
	fbe_status_t								status;
    fbe_queue_head_t							queue_head;
	fbe_semaphore_t								sem;
	fbe_spinlock_t								lock;
	buffer_allocation_context_t *				current_buffer;
	fbe_atomic_t								allocs_done = 0;
	fbe_bool_t									run = FBE_TRUE;

	fbe_queue_init(&queue_head);
	fbe_semaphore_init(&sem, 0, FBE_SEMAPHORE_MAX);
	fbe_spinlock_init(&lock);

    owner_id  = CMS_OWNER_ID_CREATE(FBE_CMS_CLIENT_INTERNAL, test_context->thread_number);

    status = func_ptr->register_owner_func(FBE_CMS_CLIENT_INTERNAL, owner_id, 0, 0, 1);

    if (status != FBE_STATUS_OK ) {
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to register owner id\n", __FUNCTION__);
        exclusive_alloc_test_has_errors = FBE_TRUE;
    }

	fbe_thread_set_affinity(&test_context->thread_handle, test_context->cpu_to_affine);

	test_context->buffer_array = (buffer_allocation_context_t *)fbe_memory_ex_allocate(sizeof(buffer_allocation_context_t) * test_context->test_info.allocations_per_thread);
    
	/*let's see how many allocations we need to do and run them*/
	for (alloc_count = 0; alloc_count < test_context->test_info.allocations_per_thread; alloc_count++) {
		alloc_id.owner_id = owner_id;
		alloc_id.buff_id = alloc_count;
		test_context->buffer_array[alloc_count].lock = &lock;
		test_context->buffer_array[alloc_count].sem = &sem;
		test_context->buffer_array[alloc_count].queue_head = &queue_head;
        
		status = func_ptr->allocate_execlusive_func(&test_context->buffer_array[alloc_count].buffer_request,
													alloc_id,
													(1024 * (test_context->thread_number + 1)),
													fbe_cms_exerciser_exclusive_alloc_test_completion,
													context,
													test_context->buffer_array[alloc_count].sg_element,
													2);

		if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
			cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to allocate exclusive\n", __FUNCTION__);
			exclusive_alloc_test_has_errors = FBE_TRUE;
			fbe_atomic_increment(&allocs_done);
		}

		fbe_thread_delay(test_context->test_info.msec_delay_between_allocations);
	}

	/*we'll have to process things in the completion function but we'll wait here until things are done
	meanwhile, we just process what's on the queue as they are processed in completion*/
	while (run) {
		fbe_semaphore_wait(&sem, 0);
		current_buffer = (buffer_allocation_context_t *)fbe_queue_pop(&queue_head);

		switch (current_buffer->next_step) {
		case EXCLUSIVE_TEST_COMMIT:
            status = func_ptr->commit_exclusive_func(&current_buffer->buffer_request,
											current_buffer->buffer_request.m_alloc_id,
											0,
											100,
											fbe_cms_exerciser_exclusive_alloc_test_commit_completion,
											context);

			if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
				cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to commit exclusive\n", __FUNCTION__);
				exclusive_alloc_test_has_errors = FBE_TRUE;
			}
			break;
		case EXCLUSIVE_TEST_RELEASE:
			func_ptr->free_exclusive_func(&current_buffer->buffer_request,
										  current_buffer->buffer_request.m_alloc_id,
										  fbe_cms_exerciser_exclusive_alloc_test_release_completion,
										  context);

			if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) {
				cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to free exclusive\n", __FUNCTION__);
				exclusive_alloc_test_has_errors = FBE_TRUE;
			}
		case EXCLUSIVE_TEST_DONE:
			fbe_atomic_increment(&allocs_done);
			if (allocs_done == test_context->test_info.allocations_per_thread) {
				run = FBE_FALSE;
			}

			break;
		}

	}

	/*when we get here, all the allocations+commit+free we did are done*/

    status = func_ptr->unregister_owner_func(FBE_CMS_CLIENT_INTERNAL, owner_id);

    if (status != FBE_STATUS_OK ) {
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to unregister owner id\n", __FUNCTION__);
        exclusive_alloc_test_has_errors = FBE_TRUE;
    }

	fbe_memory_ex_release(test_context->buffer_array);
	fbe_memory_ex_release(test_context);

	fbe_cms_exerciser_exclusive_alloc_test_mark_thread_done();




}

fbe_cms_exerciser_control_get_interface_tests_resutls(fbe_packet_t *packet)
{
	fbe_status_t                						status = FBE_STATUS_OK;
    fbe_cms_exerciser_get_interface_test_results_t *	get_results = NULL;
    
    /* Verify packet contents and get pointer to the user's buffer*/
    status = cms_exerciser_get_payload(packet, (void **)&get_results, sizeof(fbe_cms_exerciser_get_interface_test_results_t));
    if ( status != FBE_STATUS_OK ){
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Payload failed.\n", __FUNCTION__);
        cms_exerciser_complete_packet(packet, status);
        return status;
    }

	get_results->passed = !exclusive_alloc_test_has_errors;

	cms_exerciser_complete_packet(packet, status);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_cms_exerciser_exclusive_alloc_test_completion(fbe_cms_buff_req_t * req_p, void * context)
{
	/*cast from CMs to our test structure*/
	buffer_allocation_context_t * buffer_req = (buffer_allocation_context_t *)cms_request_to_buffer_allocation_context(req_p);

	if (req_p->m_status != FBE_STATUS_OK) {
		exclusive_alloc_test_has_errors = FBE_TRUE;
		buffer_req->next_step = EXCLUSIVE_TEST_DONE;
		cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: error during exclusive allocation\n", __FUNCTION__);
    }

    /*once we got a buffer we want to write some data in it and then commit it*/
	fbe_set_memory(req_p->m_sg_p->address, 'X', 100);

	/*and commit*/
	buffer_req->next_step = EXCLUSIVE_TEST_COMMIT;

	fbe_spinlock_lock(buffer_req->lock);
	fbe_queue_push(buffer_req->queue_head, &buffer_req->queue_element);
	fbe_spinlock_unlock(buffer_req->lock);

	fbe_semaphore_release(buffer_req->sem, 0, 1, FALSE); 
    
	return FBE_STATUS_OK;
}

static fbe_status_t fbe_cms_exerciser_exclusive_alloc_test_commit_completion(fbe_cms_buff_req_t * req_p, void * context)
{
   /*cast from CMs to our test structure*/
	buffer_allocation_context_t * buffer_req = (buffer_allocation_context_t *)cms_request_to_buffer_allocation_context(req_p);

	if (req_p->m_status != FBE_STATUS_OK) {
		exclusive_alloc_test_has_errors = FBE_TRUE;
		buffer_req->next_step = EXCLUSIVE_TEST_DONE;
		cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: error during commit\n", __FUNCTION__);
    }

    /*and release*/
	buffer_req->next_step = EXCLUSIVE_TEST_RELEASE;

	fbe_spinlock_lock(buffer_req->lock);
	fbe_queue_push(buffer_req->queue_head, &buffer_req->queue_element);
	fbe_spinlock_unlock(buffer_req->lock);

	fbe_semaphore_release(buffer_req->sem, 0, 1, FALSE); 
    
	return FBE_STATUS_OK;
}

static __forceinline buffer_allocation_context_t * cms_request_to_buffer_allocation_context(fbe_cms_buff_req_t * req_p)
{
    buffer_allocation_context_t * buffer;
    buffer = (buffer_allocation_context_t  *)((fbe_u8_t *)req_p - (fbe_u8_t *)(&((buffer_allocation_context_t  *)0)->buffer_request));
    return buffer;
}

static fbe_status_t fbe_cms_exerciser_exclusive_alloc_test_release_completion(fbe_cms_buff_req_t * req_p, void * context)
{
	buffer_allocation_context_t * buffer_req = (buffer_allocation_context_t *)cms_request_to_buffer_allocation_context(req_p);

	if (req_p->m_status != FBE_STATUS_OK) {
		exclusive_alloc_test_has_errors = FBE_TRUE;
		cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: error during exclusive release\n", __FUNCTION__);
    }

    /*and release*/
	buffer_req->next_step = EXCLUSIVE_TEST_DONE;

	fbe_spinlock_lock(buffer_req->lock);
	fbe_queue_push(buffer_req->queue_head, &buffer_req->queue_element);
	fbe_spinlock_unlock(buffer_req->lock);

	fbe_semaphore_release(buffer_req->sem, 0, 1, FALSE); 
    
	return FBE_STATUS_OK;

}

static fbe_cms_exerciser_exclusive_alloc_test_mark_thread_done(void)
{
    fbe_atomic_increment(&exclusive_alloc_test_thread_done_count);

	if (exclusive_alloc_test_threads_to_run == exclusive_alloc_test_thread_done_count) {
		/*test is done, let's generate notification so the user knows*/
		fbe_notification_info_t notification;
		fbe_atomic_t			test_enabled;

		test_enabled = fbe_atomic_exchange(&exclusive_alloc_test_in_progress, 0);
		if (test_enabled == 0) {
			cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: got callback from a disabled test...????\n", __FUNCTION__);
        }

        notification.notification_type = FBE_NOTIFICATION_TYPE_CMS_TEST_DONE;
		notification.class_id = FBE_CLASS_ID_INVALID;
		notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
        
		fbe_notification_send(FBE_OBJECT_ID_INVALID, notification);
	}
}
