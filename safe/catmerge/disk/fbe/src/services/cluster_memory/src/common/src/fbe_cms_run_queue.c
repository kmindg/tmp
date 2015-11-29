#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe_cms_private.h"

typedef enum fbe_cms_run_queue_thread_flag_e{
    CMS_RUN_QUEUE_THREAD_RUN,
    CMS_RUN_QUEUE_THREAD_STOP,
    CMS_RUN_QUEUE_THREAD_DONE
}fbe_cms_run_queue_thread_flag_t;

static fbe_thread_t						fbe_cms_run_queue_thread_handle[FBE_CPU_ID_MAX];
static fbe_cms_run_queue_thread_flag_t  fbe_cms_run_queue_thread_flag[FBE_CPU_ID_MAX];
static fbe_semaphore_t 					fbe_cms_run_queue_semaphore[FBE_CPU_ID_MAX];
static fbe_cpu_id_t 					fbe_cms_run_queue_cpu_count;
static fbe_multicore_queue_t 			fbe_cms_run_multicore_queue;
static fbe_atomic_t 					fbe_cms_run_queue_init_count = 0;

/* Forward declaration */ 
static void fbe_cms_run_queue_thread_func(void * context);

fbe_status_t 
fbe_cms_run_queue_init(void)
{
	fbe_cpu_id_t i;
	fbe_atomic_t init_done;

	fbe_atomic_increment(&fbe_cms_run_queue_init_count);

	fbe_atomic_exchange(&init_done, fbe_cms_run_queue_init_count);
	if (init_done != 1) {
		return FBE_STATUS_OK;
	}

	fbe_cms_run_queue_cpu_count = fbe_get_cpu_count();
       
	cms_trace(FBE_TRACE_LEVEL_INFO, "%s fbe_cms_run_queue_cpu_count = %d\n", __FUNCTION__, fbe_cms_run_queue_cpu_count);

	fbe_multicore_queue_init(&fbe_cms_run_multicore_queue);

    /* Start thread's */
	for(i = 0; i < fbe_cms_run_queue_cpu_count; i++){
		fbe_semaphore_init(&fbe_cms_run_queue_semaphore[i], 0, FBE_SEMAPHORE_MAX);
		fbe_cms_run_queue_thread_flag[i] = CMS_RUN_QUEUE_THREAD_RUN;
		fbe_thread_init(&fbe_cms_run_queue_thread_handle[i], "fbe_cms_runq", fbe_cms_run_queue_thread_func, (void*)(fbe_ptrhld_t)i);
	}

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_cms_run_queue_destroy(void)
{
	fbe_cpu_id_t i;
	fbe_atomic_t init_done;

    fbe_atomic_exchange(&init_done, fbe_cms_run_queue_init_count);
	if (init_done == 0) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s init not done\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    cms_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s entry\n", __FUNCTION__);

	/* Stop thread's */
	for(i = 0; i < fbe_cms_run_queue_cpu_count; i++){
		fbe_cms_run_queue_thread_flag[i] = CMS_RUN_QUEUE_THREAD_STOP;
	    fbe_semaphore_release(&fbe_cms_run_queue_semaphore[i], 0, 1, FALSE); 
	}

    /* notify all run queueu thread*/
	for(i = 0; i < fbe_cms_run_queue_cpu_count; i++){
		fbe_thread_wait(&fbe_cms_run_queue_thread_handle[i]);
		fbe_thread_destroy(&fbe_cms_run_queue_thread_handle[i]);
		fbe_semaphore_destroy(&fbe_cms_run_queue_semaphore[i]);
	}

	
	fbe_multicore_queue_destroy(&fbe_cms_run_multicore_queue);

    return FBE_STATUS_OK;
}

static void 
fbe_cms_run_queue_thread_func(void * context)
{
    fbe_cpu_id_t 					thread_number;
    fbe_cpu_affinity_mask_t 		affinity = 0x1;
	fbe_cms_run_queue_client_t *	client_struct;
    
	thread_number = (fbe_cpu_id_t)context;

    fbe_thread_set_affinity(&fbe_cms_run_queue_thread_handle[thread_number], 
							affinity << thread_number);

    cms_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s thread %d entry \n", __FUNCTION__, thread_number);

    while(1)    
	{
		fbe_semaphore_wait(&fbe_cms_run_queue_semaphore[thread_number], 0);
		if(fbe_cms_run_queue_thread_flag[thread_number] == CMS_RUN_QUEUE_THREAD_RUN) {
			fbe_multicore_queue_lock(&fbe_cms_run_multicore_queue, thread_number);
			client_struct = (fbe_cms_run_queue_client_t *)fbe_multicore_queue_pop(&fbe_cms_run_multicore_queue, thread_number);
			fbe_multicore_queue_unlock(&fbe_cms_run_multicore_queue, thread_number);
			/*call the client*/
			client_struct->callback(client_struct->context);
		} else {
			break;
		}
    }

    cms_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s %lld done\n", __FUNCTION__,
	      (long long)thread_number);

    fbe_cms_run_queue_thread_flag[thread_number] = CMS_RUN_QUEUE_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*user is forced to pass via function so we make sure we fill up the structure, otherwise, bad things can happen.
use FBE_CPU_ID_INVALID if you don't want a specific CPU for completion and prefer to be called at the same core
you call this function. ** Assumption is caller is core affined *** */
fbe_status_t fbe_cms_run_queue_push(fbe_cms_run_queue_client_t *waiter_struct,
									fbe_cms_run_queue_completion_t callback,
									fbe_cpu_id_t requested_cpu_id,
									void * context,
									fbe_cms_runs_queue_flags_t run_queue_flags)
{
	
    fbe_cpu_id_t current_cpu_id = FBE_CPU_ID_INVALID;
	fbe_atomic_t init_done;

    fbe_atomic_exchange(&init_done, fbe_cms_run_queue_init_count);
	if (init_done == 0) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s init not done\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if (waiter_struct == NULL || callback == NULL) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s NULL pointer\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    if (requested_cpu_id == FBE_CPU_ID_INVALID) {
		current_cpu_id = fbe_get_cpu_id();
	}else if (requested_cpu_id < fbe_cms_run_queue_cpu_count){
		current_cpu_id = requested_cpu_id;
	}else{
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s illeal CPU ID passed: %d\n", __FUNCTION__, requested_cpu_id);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	waiter_struct->callback = callback;
    waiter_struct->context = context;
	waiter_struct->run_queue_flags = run_queue_flags;/*currently not used*/
    
    fbe_multicore_queue_lock(&fbe_cms_run_multicore_queue, current_cpu_id);
	fbe_multicore_queue_push(&fbe_cms_run_multicore_queue, &waiter_struct->queue_element, current_cpu_id );
	fbe_multicore_queue_unlock(&fbe_cms_run_multicore_queue, current_cpu_id);

	fbe_semaphore_release(&fbe_cms_run_queue_semaphore[current_cpu_id], 0, 1, FALSE); 

	return FBE_STATUS_OK;
}

