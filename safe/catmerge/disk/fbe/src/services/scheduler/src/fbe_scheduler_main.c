#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

#include "fbe_testability.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_scheduler_private.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe_job_service.h"



void
scheduler_trace_object(fbe_object_id_t object_id,
                       fbe_trace_level_t trace_level,
                       fbe_trace_message_id_t message_id,
                       const char * fmt, ...) __attribute__((format(__printf_func__,4,5)));


/* Idle timer triggers idle queue dispatch. The resolution in milliseconds */
#define FBE_SCHEDULER_IDLE_TIMER 100L /* 100 ms. */
#define FBE_SCHEDULER_DEFAULT_TIMER 3000 /* 3 sec. by default */
#define FBE_SCHEDULER_CREDIT_LOAD_INTERVAL_MS 1000
#define FBE_SCHEDULER_CREDIT_LOAD_TIMER_COUNT_MS (FBE_SCHEDULER_CREDIT_LOAD_INTERVAL_MS / FBE_SCHEDULER_IDLE_TIMER)

#if defined(SIMMODE_ENV)
#define FBE_SCHEDULER_PACKET_COUNT 500
#else
#define FBE_SCHEDULER_PACKET_COUNT 2000
#endif
#define FBE_SCHEDULER_SEC_TO_WAIT_BEFORE_BG_ENABLE              5400 /*wait up to 90 minutes before we give up on upper layers reenabling background operations */
#define FBE_SCHEDULER_SEC_TO_WAIT_BEFORE_QUEUE_DEPTH_RESTORE    5400 /*wait up to 90 minutes before we give up on upper layers restoring the queue depth */

typedef enum scheduler_thread_flag_e{
    SCHEDULER_THREAD_RUN,
    SCHEDULER_THREAD_STOP,
    SCHEDULER_THREAD_DONE
}scheduler_thread_flag_t;

enum fbe_scheduler_constants_e {
    FBE_SCHEDULER_PP_THREADS_MAX = 1,
    FBE_SCHEDULER_ESP_THREADS_MAX = 1,
    FBE_SCHEDULER_SEP_THREADS_MAX = FBE_CPU_ID_MAX,
};

/* Scheduler Thread Info */
typedef struct fbe_scheduler_thread_info_s{
    fbe_object_id_t     object_id;
    fbe_time_t          dispatch_timestamp; /* Timestamp for the last dispatch */
    fbe_u32_t           exceed_counter; /* Increment it only when the thread exceeds the 3sec limit */
    fbe_cpu_id_t		cpu_id;
}fbe_scheduler_thread_info_t;

/* Global tick count Ito track the no. of idle threads. */
static fbe_atomic_t run_thread_tick_count = 0;
static fbe_time_t idle_thread_time; /*! Time used so we can compare I/O times against this in debug macros. */
static fbe_scheduler_thread_info_t scheduler_thread_info_pool[FBE_CPU_ID_MAX];

static fbe_spinlock_t		scheduler_lock; 
/*
static fbe_u32_t			job_in_progress = 0;
static fbe_u64_t            job_number;
*/
static fbe_queue_head_t	    scheduler_idle_queue_head;
static fbe_thread_t			scheduler_idle_queue_thread_handle;
static scheduler_thread_flag_t  scheduler_idle_queue_thread_flag;

static fbe_thread_t             scheduler_run_queue_thread_handle[FBE_CPU_ID_MAX];
static scheduler_thread_flag_t  scheduler_run_queue_thread_flag;

static fbe_u32_t			credit_load_timer_count = 0;

static fbe_cpu_id_t cpu_id = 0;

static fbe_multicore_queue_t scheduler_run_queue;
static fbe_u32_t scheduler_run_queue_counter[FBE_CPU_ID_MAX];
static fbe_queue_head_t tmp_queue_array[FBE_CPU_ID_MAX];
static fbe_rendezvous_event_t scheduler_run_queue_event[FBE_CPU_ID_MAX];

static fbe_u8_t * fbe_scheduler_packet_buffer = NULL;
static fbe_multicore_queue_t scheduler_packet_queue;
static fbe_u32_t scheduler_packet_queue_counter[FBE_CPU_ID_MAX];

typedef struct fbe_scheduler_service_s{
    fbe_base_service_t	base_service;
}fbe_scheduler_service_t;

static fbe_scheduler_service_t scheduler_service;
static fbe_cpu_id_t fbe_scheduler_cpu_count; 

static fbe_time_t scheduler_bgs_monitor_start_time;
static fbe_u32_t  sec_to_wait_before_bg_enable = FBE_SCHEDULER_SEC_TO_WAIT_BEFORE_BG_ENABLE;
static fbe_bool_t check_for_bgs_enable = FBE_FALSE;

static fbe_time_t scheduler_queue_depth_monitor_start_time;
static fbe_u32_t  sec_to_wait_before_queue_depth_restore = FBE_SCHEDULER_SEC_TO_WAIT_BEFORE_QUEUE_DEPTH_RESTORE;
static fbe_bool_t check_for_queue_depth_restore = FBE_FALSE;

/* Forward declaration */ 
static fbe_status_t fbe_scheduler_control_entry(fbe_packet_t * packet);

fbe_service_methods_t fbe_scheduler_service_methods = {FBE_SERVICE_ID_SCHEDULER, fbe_scheduler_control_entry};

static void scheduler_idle_queue_thread_func(void * context);
static void scheduler_run_queue_thread_func(void * context);

static void scheduler_dispatch_idle_queue(void);
static void scheduler_dispatch_run_queue(void *context);


static fbe_status_t scheduler_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static void fbe_scheduler_init_debug_hooks(void);
static fbe_status_t set_scheduler_debug_hook(fbe_u64_t hook);
static fbe_status_t get_scheduler_debug_hook(fbe_scheduler_debug_hook_t *hook);
static void del_scheduler_debug_hook(fbe_u64_t hook);
static fbe_bool_t fbe_scheduler_compare_debug_hook(fbe_scheduler_debug_hook_t *hook,
                                                   fbe_object_id_t object_id,
                                                   fbe_u32_t monitor_state,
                                                   fbe_u32_t monitor_substate,
                                                   fbe_u64_t val2);
static fbe_u32_t has_scheduler_table_entry(fbe_scheduler_element_t * scheduler_element);
static fbe_status_t fbe_scheduler_send_control_operation(
                                fbe_object_id_t                         object_id,  
                                fbe_payload_control_operation_opcode_t  in_op_code);

static void scheduler_dispatch_run_queue_sep(void *context);
static void scheduler_dispatch_idle_queue_sep(void);
static fbe_status_t scheduler_sep_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static void fbe_scheduler_check_is_need_to_restart_bgs(void);
static fbe_status_t fbe_scheduler_send_control_operation_to_base_config_class(fbe_payload_control_operation_opcode_t  in_op_code,
																			  fbe_base_config_control_system_bg_service_t *bg_operation);
static void fbe_scheduler_check_need_to_restore_queue_depth(void);


/*
static fbe_job_service_update_encryption_mode_t job_service_update_encryption_mode;
*/
static fbe_status_t scheduler_plugin_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_scheduler_plugin_t scheduler_encryption_plugin = { NULL, NULL , NULL };

/*!**************************************************************
 * fbe_scheduler_init_debug_hooks()
 ****************************************************************
 * @brief
 *  Initialize the scheduler debug hook array.
 *
 * @param none
 *
 * @return none
 *
 * @author
 *  8/9/2011 - Created. Jason White
 *
 ****************************************************************/
static void fbe_scheduler_init_debug_hooks(void)
{
    fbe_u32_t i;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    for (i=0; i<MAX_SCHEDULER_DEBUG_HOOKS; i++)
    {
        scheduler_debug_hooks[i].action = 0;
        scheduler_debug_hooks[i].check_type = 0;
        scheduler_debug_hooks[i].monitor_state = 0;
        scheduler_debug_hooks[i].monitor_substate = 0;
        scheduler_debug_hooks[i].object_id = FBE_OBJECT_ID_INVALID;
        scheduler_debug_hooks[i].val1 = 0;
        scheduler_debug_hooks[i].val2 = 0;
        scheduler_debug_hooks[i].counter = 0;
    }
} // End fbe_scheduler_init_debug_hooks()

void
scheduler_trace(fbe_trace_level_t trace_level,
                fbe_trace_message_id_t message_id,
                const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&scheduler_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&scheduler_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_SCHEDULER,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

void
scheduler_trace_object(fbe_object_id_t object_id,
                       fbe_trace_level_t trace_level,
                       fbe_trace_message_id_t message_id,
                       const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&scheduler_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&scheduler_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_OBJECT,
                     object_id,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

static fbe_status_t 
fbe_scheduler_init(fbe_packet_t * packet)
{
	fbe_u32_t      thread_pool;
    fbe_status_t			status;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;
	EMCPAL_STATUS       nt_status;
	fbe_u32_t i;
	fbe_packet_t * packet_ptr;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(FBE_SCHEDULER_CREDIT_LOAD_INTERVAL_MS >= FBE_SCHEDULER_IDLE_TIMER)

    /* Initialize scheduler lock */
    fbe_spinlock_init(&scheduler_lock);

    /* Initialize scheduler queues */
    fbe_queue_init(&scheduler_idle_queue_head);    
    
	fbe_multicore_queue_init(&scheduler_run_queue);
	for(i = 0; i < FBE_CPU_ID_MAX; i++){
		fbe_queue_init(&tmp_queue_array[i]);
		fbe_rendezvous_event_init(&scheduler_run_queue_event[i]);
		scheduler_run_queue_counter[i] = 0;
	}

    fbe_get_package_id(&package_id);

	if (package_id == FBE_PACKAGE_ID_SEP_0) {
		/*initialize the scheduler credit logic, we need to do this prior to start any thread*/
		status = fbe_scheduler_credit_init();
		if (status != FBE_STATUS_OK) {
			scheduler_trace(FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s: failed to init credit logic\n", __FUNCTION__);
	
		}

		if(scheduler_encryption_plugin.plugin_init){
			scheduler_encryption_plugin.plugin_init();
		}

		fbe_scheduler_start_to_monitor_background_service_enabled();/*when we first start sep, background services will be disbaled so we want to make sure they were re-enabled*/
	}

    /* Start idle thread */
    scheduler_idle_queue_thread_flag = SCHEDULER_THREAD_RUN;
    nt_status = fbe_thread_init(&scheduler_idle_queue_thread_handle, "fbe_sched_idle", scheduler_idle_queue_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        scheduler_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: fbe_thread_init fail\n", __FUNCTION__);
    }
	

    fbe_scheduler_cpu_count = fbe_get_cpu_count();

    if(fbe_scheduler_cpu_count > FBE_CPU_ID_MAX){
        fbe_scheduler_cpu_count = FBE_CPU_ID_MAX;
    }

    switch(package_id){
        case FBE_PACKAGE_ID_PHYSICAL:
            if(fbe_scheduler_cpu_count > FBE_SCHEDULER_PP_THREADS_MAX){
                fbe_scheduler_cpu_count = FBE_SCHEDULER_PP_THREADS_MAX;
            }
            break;
        case FBE_PACKAGE_ID_ESP:
            if(fbe_scheduler_cpu_count > FBE_SCHEDULER_ESP_THREADS_MAX){
                fbe_scheduler_cpu_count = FBE_SCHEDULER_ESP_THREADS_MAX;
            }
            break;
        case FBE_PACKAGE_ID_SEP_0:
            if(fbe_scheduler_cpu_count > FBE_SCHEDULER_SEP_THREADS_MAX){
                fbe_scheduler_cpu_count = FBE_SCHEDULER_SEP_THREADS_MAX;
            }
            break;
    }

    if(package_id == FBE_PACKAGE_ID_SEP_0)
    {
        /* Allocate memory for scheduler packet */
		scheduler_trace(FBE_TRACE_LEVEL_INFO, 
					   FBE_TRACE_MESSAGE_ID_INFO, 
					   "MCRMEM: Sched: %d \n", (int)(FBE_SCHEDULER_PACKET_COUNT * sizeof(fbe_packet_t)));

        fbe_scheduler_packet_buffer = fbe_memory_allocate_required(FBE_SCHEDULER_PACKET_COUNT * sizeof(fbe_packet_t));

        /* Check if we have the required memory. If not, don't proceed at all. Just PANIC. */
        if(fbe_scheduler_packet_buffer == NULL)
        {
            scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                            FBE_TRACE_MESSAGE_ID_INFO, 
                            "%s: Could not allocate the required memory: 0x%x. PANIC..\n", 
                            __FUNCTION__, 
                            (unsigned int)(FBE_SCHEDULER_PACKET_COUNT * sizeof(fbe_packet_t)));

            return FBE_STATUS_GENERIC_FAILURE;
        }

		packet_ptr = (fbe_packet_t *)fbe_scheduler_packet_buffer;
		fbe_multicore_queue_init(&scheduler_packet_queue);
		for(thread_pool = 0; thread_pool < fbe_scheduler_cpu_count; thread_pool++) {
			scheduler_packet_queue_counter[thread_pool] = 0;
			for(i = 0; i < FBE_SCHEDULER_PACKET_COUNT / fbe_scheduler_cpu_count; i++){
				fbe_queue_element_init(&packet_ptr->queue_element);
				fbe_multicore_queue_push(&scheduler_packet_queue, &packet_ptr->queue_element, thread_pool);
				scheduler_packet_queue_counter[thread_pool]++;
				packet_ptr++;
			}
		}
	}

    /* Temporary to find CSX resource queue corruption */
    //fbe_scheduler_cpu_count = 1;
	for(thread_pool = 0; thread_pool < fbe_scheduler_cpu_count; thread_pool++)
	{
		/* Start run_queue thread */
		scheduler_run_queue_thread_flag = SCHEDULER_THREAD_RUN;
		scheduler_thread_info_pool[thread_pool].cpu_id = thread_pool;
		nt_status = fbe_thread_init(&scheduler_run_queue_thread_handle[thread_pool], "fbe_runq", scheduler_run_queue_thread_func, &scheduler_thread_info_pool[thread_pool]);
		if (nt_status != EMCPAL_STATUS_SUCCESS) {
			scheduler_trace(FBE_TRACE_LEVEL_ERROR,
					FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					"%s: fbe_thread_init fail\n", __FUNCTION__);
		}
	}

	// initialize the scheduler debug hook array
	fbe_scheduler_init_debug_hooks();

    fbe_base_service_set_service_id((fbe_base_service_t *) &scheduler_service, FBE_SERVICE_ID_SCHEDULER);

    fbe_base_service_init((fbe_base_service_t *) &scheduler_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_scheduler_destroy(fbe_packet_t * packet)
{
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;
    fbe_u32_t thread_pool;
	fbe_u32_t i;

    scheduler_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    fbe_get_package_id(&package_id);
    if (package_id == FBE_PACKAGE_ID_SEP_0) {
		if(scheduler_encryption_plugin.plugin_destroy){
			scheduler_encryption_plugin.plugin_destroy();
		}

        fbe_scheduler_credit_destroy();
    }

    /* Stop idle_queue thread */
    scheduler_idle_queue_thread_flag = SCHEDULER_THREAD_STOP;  
    fbe_thread_wait(&scheduler_idle_queue_thread_handle);
    fbe_thread_destroy(&scheduler_idle_queue_thread_handle);

    fbe_queue_destroy(&scheduler_idle_queue_head);

    /* Stop run_queue thread */
    scheduler_run_queue_thread_flag = SCHEDULER_THREAD_STOP;  

    for(thread_pool = 0; thread_pool < fbe_scheduler_cpu_count; thread_pool++)
    {
		fbe_rendezvous_event_set(&scheduler_run_queue_event[thread_pool]);
        fbe_thread_wait(&scheduler_run_queue_thread_handle[thread_pool]);
        fbe_thread_destroy(&scheduler_run_queue_thread_handle[thread_pool]);
    }

	fbe_multicore_queue_destroy(&scheduler_run_queue);
	for(i = 0; i < FBE_CPU_ID_MAX; i++){
		fbe_queue_destroy(&tmp_queue_array[i]);
		fbe_rendezvous_event_destroy(&scheduler_run_queue_event[i]);
	}

    fbe_spinlock_destroy(&scheduler_lock);

    if(fbe_scheduler_packet_buffer != NULL)
    {
        fbe_memory_release_required(fbe_scheduler_packet_buffer);
        fbe_scheduler_packet_buffer = NULL;
    }

    fbe_base_service_destroy((fbe_base_service_t *) &scheduler_service);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_scheduler_control_entry(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);

    /* Don't trace frequently occurring control codes */
    if (control_code != FBE_SCHEDULER_CONTROL_CODE_GET_DEBUG_HOOK)
    {
        scheduler_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry - control_code: 0x%x\n", __FUNCTION__, control_code);
    }

    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = fbe_scheduler_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &scheduler_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_scheduler_destroy( packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_PER_CORE:
            status = fbe_scheduler_remove_credits_per_core(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_PER_CORE:
            status = fbe_scheduler_add_credits_per_core(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_REMOVE_CPU_CREDITS_ALL_CORES:
            status = fbe_scheduler_remove_credits_all_core_cores(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_ADD_CPU_CREDITS_ALL_CORES:
            status = fbe_scheduler_add_credits_all_cores(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_ADD_MEMORY_CREDITS:
            status = fbe_scheduler_add_memory_credits(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_REMOVE_MEMORY_CREDITS:
            status = fbe_scheduler_remove_memory_credits(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_SET_CREDITS:
            status = fbe_scheduler_set_credits(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_GET_CREDITS:
            status = fbe_scheduler_get_credits(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_SET_SCALE:
            status = fbe_scheduler_set_scale_usurper(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_GET_SCALE:
            status = fbe_scheduler_get_scale_usurper(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_SET_DEBUG_HOOK:
            status = fbe_scheduler_set_debug_hook(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_GET_DEBUG_HOOK:
            status = fbe_scheduler_get_debug_hook(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_DELETE_DEBUG_HOOK:
            status = fbe_scheduler_del_debug_hook(packet);
            break;
        case FBE_SCHEDULER_CONTROL_CODE_CLEAR_DEBUG_HOOKS:
            status = fbe_scheduler_clear_debug_hooks(packet);
            break;
        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&scheduler_service, packet);
            break;
    }

    return status;
}

fbe_status_t 
fbe_scheduler_register(fbe_scheduler_element_t * scheduler_element)
{
    /* We want to run immediately after registration */ 
    fbe_spinlock_lock(&scheduler_lock);

    /* Temporary hack */
    scheduler_element->time_counter = FBE_SCHEDULER_DEFAULT_TIMER;
    
    scheduler_element->scheduler_state = 0;

    /* set the schduler debug hook function if there is an
       entry for this scheduler_element's object in the 
       scheduler_debug_hooks table */
    scheduler_element->hook_counter = has_scheduler_table_entry(scheduler_element);
    if (scheduler_element->hook_counter!=0) {
        scheduler_element->hook = fbe_scheduler_debug_hook;
    } else {
        scheduler_element->hook = NULL;
    }

    fbe_queue_push(&scheduler_idle_queue_head, &scheduler_element->queue_element);
    scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_IDLE;

    fbe_spinlock_unlock(&scheduler_lock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_scheduler_unregister(fbe_scheduler_element_t * scheduler_element)
{
    /* Lock all queues */
    fbe_spinlock_lock(&scheduler_lock);

    /* if the scheduler element is on pending queue the packet MUST be NULL */
    if(scheduler_element->current_queue != FBE_SCHEDULER_QUEUE_TYPE_DESTROYED){
        
        scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s:  monitor still busy, please wait\n", __FUNCTION__);

        fbe_spinlock_unlock(&scheduler_lock);

        return FBE_STATUS_BUSY;
    }

    //fbe_queue_remove(&scheduler_element->queue_element);

    /* Unlock all queues */
    fbe_spinlock_unlock(&scheduler_lock);

    return FBE_STATUS_OK;
}

static void 
scheduler_idle_queue_thread_func(void * context)
{
    fbe_package_id_t package_id = FBE_PACKAGE_ID_INVALID;
    
    fbe_u32_t thread_pool;
    fbe_time_t thread_age_ms = 0;
    fbe_time_t current_time;
    fbe_time_t dispatch_time;
    fbe_u32_t check_bgs_enable_minute_resolution = 0;
    fbe_u32_t check_queue_depth_restore_minute_resolution = 0;
    
    FBE_UNREFERENCED_PARAMETER(context);

    scheduler_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);
      
    /* first time */
    idle_thread_time = fbe_get_time();

    fbe_get_package_id(&package_id);

    while(1){
        fbe_thread_delay(FBE_SCHEDULER_IDLE_TIMER);
        /* Get time so we can compare to this in debug macros for I/O times. */
        current_time = fbe_get_time();
        if ((current_time - idle_thread_time) > FBE_SCHEDULER_DEFAULT_TIMER)
        {
            scheduler_trace(FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                            "%s: idle thread didn't get dispatched for %d ms.\n", 
                            __FUNCTION__, 
                            (int)(current_time - idle_thread_time));
        }
        idle_thread_time = current_time;
        fbe_transport_set_coarse_time((fbe_u32_t)current_time);
        if(scheduler_idle_queue_thread_flag == SCHEDULER_THREAD_RUN) {
            /*do we need to reload the credit table ? (each 1 sec.), also, we do that only in SEP pacakge*/
            if (package_id == FBE_PACKAGE_ID_SEP_0){
                scheduler_dispatch_idle_queue_sep();
				credit_load_timer_count++;

				if (credit_load_timer_count >= FBE_SCHEDULER_CREDIT_LOAD_TIMER_COUNT_MS) {
					credit_load_timer_count = 0;
					scheduler_credit_reset_master_memory();
					scheduler_credit_reload_table();

                    /*we use the 1 second interval to also check (once a minute) if we need to re-enable the background services.
					They were disabled at start time via the driver entry and the start of the managament stack is supposed to re-enable them 
					after we backed off the system drive for the whole boot time. If they fail to do so, we'll do it here*/
					if (check_for_bgs_enable && ((check_bgs_enable_minute_resolution++) == 60)) {
						fbe_scheduler_check_is_need_to_restart_bgs();
						check_bgs_enable_minute_resolution = 0;
					}
				}
            }  
            else {
                scheduler_dispatch_idle_queue();
                if(package_id == FBE_PACKAGE_ID_ESP)
                {
                    if(check_for_queue_depth_restore && ((check_queue_depth_restore_minute_resolution++) == 60)) {
                        fbe_scheduler_check_need_to_restore_queue_depth();
                        check_queue_depth_restore_minute_resolution = 0;
                    }
                }
            }
        } 
        else {
            break;
        }

        current_time = fbe_get_time();

        /* Go thru the list of thread_info, trace any one with timestamp 3 sec old */
        for(thread_pool = 0; thread_pool < fbe_scheduler_cpu_count; thread_pool++)
        {
            dispatch_time = scheduler_thread_info_pool[thread_pool].dispatch_timestamp;

            /* We are not locking dispatch_time, so we need to make sure it is after our current time. */
            if ((dispatch_time != 0) && (current_time > dispatch_time))
            {
                thread_age_ms = (current_time - dispatch_time);
                if (thread_age_ms > FBE_SCHEDULER_DEFAULT_TIMER)
                {
                    /* Increment exceed_counter each time when the thread is greater than 3 second */
                    scheduler_thread_info_pool[thread_pool].exceed_counter++;

                    /* Log a message once every 1000 times */
                    if(scheduler_thread_info_pool[thread_pool].exceed_counter % 1000 == 1)
                    {
                        scheduler_trace(FBE_TRACE_LEVEL_WARNING, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                    "%s: OBJ: %4X - Thread exceeded 3 sec limit (%llu ms) for %d times.\n", 
                                __FUNCTION__, 
                                scheduler_thread_info_pool[thread_pool].object_id,
                                (unsigned long long)thread_age_ms, 
                                scheduler_thread_info_pool[thread_pool].exceed_counter);
                    }
                }
            }
        }
    }

    scheduler_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s done\n", __FUNCTION__);

    scheduler_idle_queue_thread_flag = SCHEDULER_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/* Dispatch first element on the queue */
static void 
scheduler_dispatch_run_queue_sep(void *context)
{
    fbe_packet_t * packet;
    fbe_scheduler_element_t *scheduler_element = NULL;
    fbe_scheduler_thread_info_t *scheduler_thread_info = (fbe_scheduler_thread_info_t *) context;
    fbe_object_id_t object_id;
    fbe_status_t status;	
    fbe_package_id_t package_id;
	fbe_queue_head_t tmp_queue;
	fbe_queue_element_t  * queue_element;

    fbe_queue_init(&tmp_queue);

	fbe_get_package_id(&package_id);

	fbe_multicore_queue_lock(&scheduler_run_queue, scheduler_thread_info->cpu_id);  
	fbe_rendezvous_event_clear(&scheduler_run_queue_event[scheduler_thread_info->cpu_id]);
	while((scheduler_packet_queue_counter[scheduler_thread_info->cpu_id] > 0) &&
			(scheduler_element = (fbe_scheduler_element_t *)fbe_multicore_queue_pop(&scheduler_run_queue, scheduler_thread_info->cpu_id))){
		scheduler_run_queue_counter[scheduler_thread_info->cpu_id]--;
		scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_PENDING;
		fbe_queue_push(&tmp_queue, &scheduler_element->queue_element);


		queue_element = fbe_multicore_queue_pop(&scheduler_packet_queue, scheduler_thread_info->cpu_id);
		scheduler_packet_queue_counter[scheduler_thread_info->cpu_id]--;
		packet = fbe_transport_queue_element_to_packet(queue_element);

		scheduler_element->packet = packet;

	}
	fbe_multicore_queue_unlock(&scheduler_run_queue, scheduler_thread_info->cpu_id);

	while(scheduler_element = (fbe_scheduler_element_t *)fbe_queue_pop(&tmp_queue)){
		//packet = fbe_transport_allocate_packet();
		packet = scheduler_element->packet;
	    object_id = fbe_base_object_scheduler_element_to_object_id(scheduler_element);

		scheduler_thread_info->object_id = object_id;
		scheduler_thread_info->dispatch_timestamp = fbe_get_time();

		fbe_transport_initialize_packet(packet);    
		fbe_transport_set_cpu_id(packet, scheduler_thread_info->cpu_id);

        fbe_transport_set_monitor_object_id(packet, object_id);
		fbe_payload_ex_allocate_memory_operation(&packet->payload_union.payload_ex);

		fbe_transport_build_control_packet(packet, 
									FBE_BASE_OBJECT_CONTROL_CODE_MONITOR,
									scheduler_element,
									sizeof(fbe_scheduler_element_t),
									sizeof(fbe_scheduler_element_t),
									scheduler_sep_completion,
									scheduler_element);

        /* We mark the packet as Monitor Op and propagate the attribute to sub-packets.
         * One example of how we use this: 
         * So that on drive errors, Raid library can recognize that its a Monitor OP and 
         * fail back the request to avoid deadlock instead of waiting for shutdown-continue.
         */
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_MONITOR_OP);
		fbe_transport_set_address(packet, package_id, FBE_SERVICE_ID_TOPOLOGY, FBE_CLASS_ID_INVALID, object_id);

		scheduler_element->packet = packet;

		status = fbe_topology_send_monitor_packet(packet);

		/* Clear out the exceed_counter and timestamp in the thread_info */
		scheduler_thread_info->dispatch_timestamp = 0;
		scheduler_thread_info->exceed_counter = 0;
	}
	
	fbe_queue_destroy(&tmp_queue);

}


static fbe_status_t 
scheduler_sep_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_scheduler_element_t * scheduler_element = (fbe_scheduler_element_t *)context;
    fbe_status_t status;
    fbe_object_id_t object_id;
	fbe_cpu_id_t cpu_id;
    fbe_payload_ex_t * payload;
	fbe_payload_control_operation_t * control_operation;


	fbe_transport_get_cpu_id(packet, &cpu_id);

	if(cpu_id >= fbe_scheduler_cpu_count){
        scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Invalid cpu_id \n", __FUNCTION__);
		cpu_id = 0;
	}

    status = fbe_transport_get_status_code(packet);
    if(status == FBE_STATUS_NO_OBJECT) { /* The object is no longer exist */
		fbe_transport_destroy_packet(packet);

		fbe_multicore_queue_lock(&scheduler_run_queue, cpu_id);
		fbe_queue_element_init(&packet->queue_element);
		fbe_multicore_queue_push(&scheduler_packet_queue, &packet->queue_element, cpu_id);
		scheduler_packet_queue_counter[cpu_id]++;
		fbe_multicore_queue_unlock(&scheduler_run_queue, cpu_id);
		
		/* Reset packet pointer */
        scheduler_element->packet = NULL;
        scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_DESTROYED;
        return FBE_STATUS_OK;
    }

    object_id = fbe_base_object_scheduler_element_to_object_id(scheduler_element);

	//fbe_transport_destroy_packet(packet);

    fbe_spinlock_lock(&scheduler_lock);

    /* Remove element from pending queue */
    //fbe_queue_remove(&scheduler_element->queue_element);

    /* Reset packet pointer */
    scheduler_element->packet = NULL;

    if(scheduler_element->scheduler_state & FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST){
        /* remove flag */
        scheduler_element->scheduler_state &= ~FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST;
        /* Push element to run queue */
        scheduler_element->time_counter = 0;
        scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_RUN;
        fbe_spinlock_unlock(&scheduler_lock);
		
		fbe_transport_destroy_packet(packet);

		fbe_multicore_queue_lock(&scheduler_run_queue, cpu_id);
		fbe_multicore_queue_push(&scheduler_run_queue, &scheduler_element->queue_element, cpu_id);
		scheduler_run_queue_counter[cpu_id]++;

        if (fbe_queue_is_element_on_queue(&packet->queue_element))
        {
            scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "packet %p for object 0x%x is already on some queue\n", packet, object_id);
        }
		fbe_queue_element_init(&packet->queue_element);
		fbe_multicore_queue_push(&scheduler_packet_queue, &packet->queue_element, cpu_id);
		scheduler_packet_queue_counter[cpu_id]++;

		fbe_rendezvous_event_set(&scheduler_run_queue_event[cpu_id]);
		fbe_multicore_queue_unlock(&scheduler_run_queue, cpu_id);

        /* wake up the run_queue thread */
        //fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    } 
	
    /* If object did not set the time_counter we will set it to three cicles */
    if(scheduler_element->time_counter == 0) {
        scheduler_element->time_counter = FBE_SCHEDULER_DEFAULT_TIMER;
    }

    /* Push element to idle queue */
    fbe_queue_push(&scheduler_idle_queue_head, &scheduler_element->queue_element);
    scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_IDLE;

	fbe_spinlock_unlock(&scheduler_lock);


	payload = fbe_transport_get_payload_ex(packet);
	//control_operation = fbe_payload_ex_get_control_operation(payload); /* Monitor control operation */
	control_operation = fbe_payload_ex_get_present_control_operation(payload);

	if((control_operation != NULL) && (control_operation->status == FBE_PAYLOAD_CONTROL_STATUS_UNIT_ATTENTION)){
        scheduler_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Object 0x%x control status_qualifier %d \n", object_id, control_operation->status_qualifier);

		/* This will return packet to packet queue */
		fbe_transport_set_completion_function(packet, scheduler_plugin_completion, NULL);

		/* We MUST to make sure that object_id in packet is correct */
		fbe_transport_set_object_id(packet, object_id);

		/* This will ALWAYS complete packet */

		if(scheduler_encryption_plugin.plugin_entry){
			scheduler_encryption_plugin.plugin_entry(packet);
		} else {
			fbe_transport_complete_packet(packet);
		}

		return FBE_STATUS_MORE_PROCESSING_REQUIRED;
	}

	fbe_transport_destroy_packet(packet);

    if (fbe_queue_is_element_on_queue(&packet->queue_element))
    {
        scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "packet %p for object 0x%xis already on some queue\n", packet, object_id);
    }

	fbe_multicore_queue_lock(&scheduler_run_queue, cpu_id);
	fbe_queue_element_init(&packet->queue_element);
	fbe_multicore_queue_push(&scheduler_packet_queue, &packet->queue_element, cpu_id);
	scheduler_packet_queue_counter[cpu_id]++;
	fbe_multicore_queue_unlock(&scheduler_run_queue, cpu_id);

    return FBE_STATUS_OK;
}

static void 
scheduler_dispatch_idle_queue(void)
{
    fbe_scheduler_element_t * scheduler_element = NULL;
    fbe_scheduler_element_t * next_element = NULL;
    fbe_status_t status;
	fbe_u32_t i;
	fbe_u32_t counter;

    fbe_spinlock_lock(&scheduler_lock);

    /* Loop over all elements and push the to run_queue if needed */
	counter = 0;
    next_element = (fbe_scheduler_element_t *)fbe_queue_front(&scheduler_idle_queue_head);
    while(next_element != NULL) {
        scheduler_element = next_element;
        next_element = (fbe_scheduler_element_t *)fbe_queue_next(&scheduler_idle_queue_head, &next_element->queue_element);

        /* Sanity checking */
        status = fbe_base_object_scheduler_element_is_object_valid(scheduler_element);
        if(status != FBE_STATUS_OK) {
			fbe_spinlock_unlock(&scheduler_lock);
            scheduler_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: scheduler_idle_queue_head corrupted\n", __FUNCTION__);
            return;
        }

        if(scheduler_element->time_counter <= FBE_SCHEDULER_IDLE_TIMER){
            scheduler_element->time_counter = 0;

            /* Move element to run_queue */
            fbe_queue_remove(&scheduler_element->queue_element);
            fbe_queue_push(&tmp_queue_array[cpu_id], &scheduler_element->queue_element);
			counter++;
			cpu_id++;
			if(cpu_id >= fbe_scheduler_cpu_count){
				cpu_id = 0;
			}

            scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_RUN;

            /* wake up the run_queue thread */
            //fbe_semaphore_release(&scheduler_run_queue_semaphore, 0, 1, FALSE);
        } else {
            scheduler_element->time_counter -= FBE_SCHEDULER_IDLE_TIMER;
        }
    } /* while(next_element != NULL) */

    fbe_spinlock_unlock(&scheduler_lock);

	for(i = 0; i < fbe_scheduler_cpu_count; i++){
		fbe_multicore_queue_lock(&scheduler_run_queue, i);
		while(scheduler_element = (fbe_scheduler_element_t *)fbe_queue_pop(&tmp_queue_array[i])){
			fbe_multicore_queue_push(&scheduler_run_queue, &scheduler_element->queue_element, i);
			scheduler_run_queue_counter[i]++;
			counter --;
		}
		fbe_rendezvous_event_set(&scheduler_run_queue_event[i]);
		fbe_multicore_queue_unlock(&scheduler_run_queue, i);
	}

	if(counter != 0){
		scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s: counter != 0 \n", __FUNCTION__);
	}
	return;
}

static void 
scheduler_dispatch_idle_queue_sep(void)
{
    fbe_scheduler_element_t * scheduler_element = NULL;
    fbe_scheduler_element_t * next_element = NULL;
    fbe_status_t status;
	fbe_u32_t i;
	fbe_u32_t counter;
    fbe_u32_t tmp_packet_queue_counter[FBE_CPU_ID_MAX] = {0};
	fbe_u32_t available_cpu;

    fbe_spinlock_lock(&scheduler_lock);

    /* Loop over all elements and push the to run_queue if needed */
	counter = 0;
    next_element = (fbe_scheduler_element_t *)fbe_queue_front(&scheduler_idle_queue_head);
    while(next_element != NULL) {
        scheduler_element = next_element;
        next_element = (fbe_scheduler_element_t *)fbe_queue_next(&scheduler_idle_queue_head, &next_element->queue_element);

        /* Sanity checking */
        status = fbe_base_object_scheduler_element_is_object_valid(scheduler_element);
        if(status != FBE_STATUS_OK) {
			fbe_spinlock_unlock(&scheduler_lock);
            scheduler_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: scheduler_idle_queue_head corrupted\n", __FUNCTION__);
            return;
        }

        if(scheduler_element->time_counter <= FBE_SCHEDULER_IDLE_TIMER){
            /* we need to find a core which has spare packet, otherwise, we should stop */
            available_cpu = fbe_scheduler_cpu_count;
            while (((scheduler_packet_queue_counter[cpu_id] - tmp_packet_queue_counter[cpu_id]) == 0) &&
                   (available_cpu > 0)) {
                available_cpu --;
                cpu_id++;
                if(cpu_id >= fbe_scheduler_cpu_count) {
                    cpu_id = 0;
                }
            }
            if (available_cpu > 0) {
                tmp_packet_queue_counter[cpu_id] ++;
            }
            else {
                scheduler_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: scheduler idle queue no more available packet\n", __FUNCTION__);
                break;  /* we don't have free packet to do anything */
            }

            scheduler_element->time_counter = 0;

            /* Move element to run_queue */
            fbe_queue_remove(&scheduler_element->queue_element);
            fbe_queue_push(&tmp_queue_array[cpu_id], &scheduler_element->queue_element);
			counter++;
			cpu_id++;
			if(cpu_id >= fbe_scheduler_cpu_count){
				cpu_id = 0;
			}

            scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_RUN;

            /* wake up the run_queue thread */
            //fbe_semaphore_release(&scheduler_run_queue_semaphore, 0, 1, FALSE);
        } else {
            scheduler_element->time_counter -= FBE_SCHEDULER_IDLE_TIMER;
        }
    } /* while(next_element != NULL) */

    fbe_spinlock_unlock(&scheduler_lock);

	for(i = 0; i < fbe_scheduler_cpu_count; i++){
		fbe_multicore_queue_lock(&scheduler_run_queue, i);
		while(scheduler_element = (fbe_scheduler_element_t *)fbe_queue_pop(&tmp_queue_array[i])){
			fbe_multicore_queue_push(&scheduler_run_queue, &scheduler_element->queue_element, i);
			scheduler_run_queue_counter[i]++;
			counter --;
		}
		fbe_rendezvous_event_set(&scheduler_run_queue_event[i]);
		fbe_multicore_queue_unlock(&scheduler_run_queue, i);
	}

	if(counter != 0){
		scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s: counter != 0 \n", __FUNCTION__);
	}
	return;
}


static void 
scheduler_run_queue_thread_func(void * context)
{
    fbe_scheduler_thread_info_t *scheduler_thread_info = (fbe_scheduler_thread_info_t *) context;
    fbe_cpu_affinity_mask_t affinity = 0x1;
	fbe_package_id_t package_id;

    fbe_get_package_id(&package_id);


    fbe_thread_set_affinity(&scheduler_run_queue_thread_handle[scheduler_thread_info->cpu_id], 
                            affinity << scheduler_thread_info->cpu_id);


    fbe_thread_get_affinity(&affinity);
    scheduler_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry; affinity %llX\n", __FUNCTION__,
		    (unsigned long long)affinity);

    while(1)    
    {
        /* Clear the run_thread_tick_count */
        fbe_atomic_exchange(&run_thread_tick_count, 0);

        //fbe_semaphore_wait(&scheduler_run_queue_semaphore, NULL);		
		fbe_rendezvous_event_wait(&scheduler_run_queue_event[scheduler_thread_info->cpu_id], 0);

        if(scheduler_run_queue_thread_flag == SCHEDULER_THREAD_RUN) {
			if(package_id == FBE_PACKAGE_ID_SEP_0){
				scheduler_dispatch_run_queue_sep(context);
			} else {
				scheduler_dispatch_run_queue(context);
			}
        } else {
            break;
        }
#if !defined(ALAMOSA_WINDOWS_ENV) && !defined(CSX_BO_USER_PREEMPTION_DISABLE_SUPPORTED)
        // Don't let the scheduler dominate here
        csx_p_thr_superyield();
#endif
    }

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s done\n", __FUNCTION__);

    scheduler_run_queue_thread_flag = SCHEDULER_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

fbe_status_t 
fbe_scheduler_run_request(fbe_scheduler_element_t * scheduler_element)
{
	fbe_u32_t    available_cpu;
	fbe_cpu_id_t local_cpu_id;
    fbe_package_id_t package_id;

    fbe_spinlock_lock(&scheduler_lock);

    if(scheduler_element->current_queue == FBE_SCHEDULER_QUEUE_TYPE_IDLE){
        fbe_get_package_id(&package_id);

        if (package_id == FBE_PACKAGE_ID_SEP_0) {
            /* scheduler_packet_queue_counter is only available to sep */
            available_cpu = fbe_scheduler_cpu_count;
            while ((scheduler_packet_queue_counter[cpu_id]== 0) && (available_cpu > 0)) {
                available_cpu --;
                cpu_id++;
                if(cpu_id >= fbe_scheduler_cpu_count) {
                    cpu_id = 0;
                }
            }
            if (available_cpu == 0) {
                scheduler_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "no more available packet for obj 0x%x, schedule to cpu %d\n", 
                                fbe_base_object_scheduler_element_to_object_id(scheduler_element), cpu_id);
            }
        }
        local_cpu_id = cpu_id;
        cpu_id++;
        if(cpu_id >= fbe_scheduler_cpu_count){
            cpu_id = 0;
        }
        scheduler_element->time_counter = 0;
        fbe_queue_remove(&scheduler_element->queue_element);
        scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_RUN;		
        fbe_spinlock_unlock(&scheduler_lock);

		fbe_multicore_queue_lock(&scheduler_run_queue, local_cpu_id);
		fbe_multicore_queue_push(&scheduler_run_queue, &scheduler_element->queue_element, local_cpu_id);
		scheduler_run_queue_counter[local_cpu_id]++;
		fbe_rendezvous_event_set(&scheduler_run_queue_event[local_cpu_id]);
		fbe_multicore_queue_unlock(&scheduler_run_queue, local_cpu_id);

        /* wake up the run_queue thread */
        //fbe_semaphore_release(&scheduler_run_queue_semaphore, 0, 1, FALSE);
        return FBE_STATUS_OK;
    }

    if(scheduler_element->current_queue == FBE_SCHEDULER_QUEUE_TYPE_PENDING){
        scheduler_element->scheduler_state |= FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST;
    }

    fbe_spinlock_unlock(&scheduler_lock);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_scheduler_set_time_counter(fbe_scheduler_element_t * scheduler_element, fbe_u32_t time_counter)
{
    scheduler_element->time_counter = time_counter;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * set_scheduler_debug_hook()
 ****************************************************************
 * @brief
 *  Set the debug hook in the scheduler debug hook array and setup
 *  the monitor with a function pointer to the hook function.
 *
 * @param hook - A pointer to the debug hook
 *
 * @return fbe_status_t
 *
 * @author
 *  8/9/2011 - Created. Jason White
 *
 ****************************************************************/
static fbe_status_t set_scheduler_debug_hook(fbe_u64_t hook)
{
    fbe_u32_t i;
    fbe_scheduler_debug_hook_t *dbg_hook;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    dbg_hook = (fbe_scheduler_debug_hook_t *)(fbe_ptrhld_t)hook;

    for (i=0; i<MAX_SCHEDULER_DEBUG_HOOKS; i++)
    {
        // find and set the first empty element in the scheduler debug hook array
        if (scheduler_debug_hooks[i].object_id == FBE_OBJECT_ID_INVALID)
        {
            scheduler_debug_hooks[i].action = dbg_hook->action;
            scheduler_debug_hooks[i].check_type = dbg_hook->check_type;
            scheduler_debug_hooks[i].monitor_state = dbg_hook->monitor_state;
            scheduler_debug_hooks[i].monitor_substate = dbg_hook->monitor_substate;
            scheduler_debug_hooks[i].object_id = dbg_hook->object_id;
            scheduler_debug_hooks[i].val1 = dbg_hook->val1;
            scheduler_debug_hooks[i].val2 = dbg_hook->val2;

            status = fbe_scheduler_send_control_operation(dbg_hook->object_id, FBE_BASE_OBJECT_CONTROL_CODE_SET_DEBUG_HOOK);

            if (status != FBE_STATUS_OK) {
                scheduler_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to send the hook to obj %d\n", __FUNCTION__, dbg_hook->object_id);
            }
            return status;
        }
    }
    scheduler_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,"%s: could not find an empty slot!\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;
} // End set_scheduler_debug_hook()

/*!**************************************************************
 * get_scheduler_debug_hook()
 ****************************************************************
 * @brief
 *  Get the given debug hook and fill in the stats (counters)
 *
 * @param hook - A pointer to the debug hook
 *
 * @return fbe_status_t
 *
 * @author
 *  8/9/2011 - Created. Jason White
 *
 ****************************************************************/
static fbe_status_t get_scheduler_debug_hook(fbe_scheduler_debug_hook_t *hook)
{
    fbe_u32_t i;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    for (i=0; i<MAX_SCHEDULER_DEBUG_HOOKS; i++)
    {
        // find the hook in the scheduler debug hook array and NULL it out
        if (hook->object_id != 0 &&
            hook->object_id != FBE_OBJECT_ID_INVALID &&
            scheduler_debug_hooks[i].action == hook->action &&
            scheduler_debug_hooks[i].check_type == hook->check_type &&
            scheduler_debug_hooks[i].monitor_state == hook->monitor_state &&
            scheduler_debug_hooks[i].monitor_substate == hook->monitor_substate &&
            scheduler_debug_hooks[i].object_id == hook->object_id &&
            scheduler_debug_hooks[i].val1 == hook->val1 &&
            scheduler_debug_hooks[i].val2 == hook->val2 )
        {
            fbe_atomic_exchange(&hook->counter, scheduler_debug_hooks[i].counter);

            return FBE_STATUS_OK;
        }
    }
    scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: could not find the hook!\n", __FUNCTION__);
    return FBE_STATUS_GENERIC_FAILURE;
} // End get_scheduler_debug_hook()

/*!**************************************************************
 * del_scheduler_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the given debug hook and remove the hook function
 *  pointer from the monitor.
 *
 * @param hook - A pointer to the debug hook
 *
 * @return none
 *
 * @author
 *  8/9/2011 - Created. Jason White
 *
 ****************************************************************/
static void del_scheduler_debug_hook(fbe_u64_t hook)
{
    fbe_u32_t i;
    fbe_scheduler_debug_hook_t *dbg_hook = NULL;
    fbe_scheduler_debug_hook_t *sched_hook = NULL;
    fbe_status_t status;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    dbg_hook = (fbe_scheduler_debug_hook_t *)(fbe_ptrhld_t)hook;

    for (i=0; i<MAX_SCHEDULER_DEBUG_HOOKS; i++)
    {
        // find the hook in the scheduler debug hook array and NULL it out
        if (scheduler_debug_hooks[i].action == dbg_hook->action &&
            scheduler_debug_hooks[i].check_type == dbg_hook->check_type &&
            scheduler_debug_hooks[i].monitor_state == dbg_hook->monitor_state &&
            scheduler_debug_hooks[i].monitor_substate == dbg_hook->monitor_substate &&
            scheduler_debug_hooks[i].object_id == dbg_hook->object_id &&
            scheduler_debug_hooks[i].val1 == dbg_hook->val1 &&
            scheduler_debug_hooks[i].val2 == dbg_hook->val2 )
        {
            // we always send the clear to the object, and let the object to track the count.
            status = fbe_scheduler_send_control_operation(dbg_hook->object_id, FBE_BASE_OBJECT_CONTROL_CODE_CLEAR_DEBUG_HOOK);
            if (status == FBE_STATUS_OK) {
            sched_hook = &scheduler_debug_hooks[i];

            scheduler_debug_hooks[i].action = 0;
            scheduler_debug_hooks[i].check_type = 0;
            scheduler_debug_hooks[i].monitor_state = 0;
            scheduler_debug_hooks[i].monitor_substate = 0;
            scheduler_debug_hooks[i].object_id = FBE_OBJECT_ID_INVALID;
            scheduler_debug_hooks[i].val1 = 0;
            scheduler_debug_hooks[i].val2 = 0;
            scheduler_debug_hooks[i].counter = 0;
        }
            else {
                scheduler_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to delete the hook from obj %d\n", __FUNCTION__, dbg_hook->object_id);
            }
            break;
        }
    }

    if (sched_hook  == NULL) {
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: could not find the hook!\n", __FUNCTION__);
    }
    return ;
} // End del_scheduler_debug_hook()

fbe_status_t fbe_scheduler_set_debug_hook(fbe_packet_t *packet)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_scheduler_debug_hook_t*                 hook = NULL;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &hook);
    if (hook == NULL){
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* If length of the buffer does not match, return an error*/
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_scheduler_debug_hook_t)){
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    scheduler_trace_object(hook->object_id, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: hook state: %d substate: %d type: %d action: %d val1: 0x%llx val2: 0x%llx\n", 
                           __FUNCTION__, hook->monitor_state, hook->monitor_substate,
                           hook->check_type, hook->action, hook->val1, hook->val2);

    status = set_scheduler_debug_hook((fbe_u64_t)hook);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_get_debug_hook(fbe_packet_t *packet)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_scheduler_debug_hook_t*                 hook = NULL;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &hook);
    if (hook == NULL){
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* If length of the buffer does not match, return an error*/
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_scheduler_debug_hook_t)){
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = get_scheduler_debug_hook(hook);
    if (status != FBE_STATUS_OK) 
    { 
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: get scheduler hook for obj: 0x%x failed. status: 0x%x\n", 
                        __FUNCTION__, hook->object_id, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        scheduler_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                        "scheduler: get debug hook obj: 0x%x state: %d substate: %d check: %d action: %d counter: 0x%llx\n", 
                        hook->object_id, hook->monitor_state, hook->monitor_substate,
                        hook->check_type, hook->action, (unsigned long long)hook->counter);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_del_debug_hook(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_scheduler_debug_hook_t*                 hook = NULL;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &hook);
    if (hook == NULL){
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* If length of the buffer does not match, return an error*/
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_scheduler_debug_hook_t)){
        scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: fbe_payload_control_get_buffer_length failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    scheduler_trace_object(hook->object_id, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: hook state: %d substate: %d type: %d action: %d val1: 0x%llx val2: 0x%llx\n", 
                           __FUNCTION__, hook->monitor_state, hook->monitor_substate,
                           hook->check_type, hook->action, hook->val1, hook->val2);

    del_scheduler_debug_hook((fbe_u64_t)hook);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_scheduler_clear_debug_hooks(fbe_packet_t *packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.*/
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: Clearing all of the debug hooks!\n", __FUNCTION__);
    fbe_scheduler_init_debug_hooks();

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_scheduler_debug_hook()
 ****************************************************************
 * @brief
 *  This is the scheduler debug hook function that will be called
 *  by the monitors that have the function pointer setup.
 *
 * @param object_id - The object id of the monitor
 * @param monitor_state - The current state of the monitor
 * @param monitor_substate - The current substate of the monitor
 * @param val1 - A generic value that can be used for comparisons
 * @param val2 - A generic value that can be used for comparisons
 *
 * @return fbe_scheduler_hook_status_t
 *
 * @author
 *  8/9/2011 - Created. Jason White
 *
 ****************************************************************/
fbe_scheduler_hook_status_t fbe_scheduler_debug_hook(fbe_object_id_t object_id,
                                                     fbe_u32_t monitor_state,
                                                     fbe_u32_t monitor_substate,
                                                     fbe_u64_t val1,
                                                     fbe_u64_t val2)
{
    fbe_u32_t           hook_index;
    fbe_scheduler_hook_status_t status = FBE_SCHED_STATUS_OK;
    fbe_trace_level_t   trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;

    scheduler_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry - obj id 0x%x\n", __FUNCTION__, object_id);

    for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++)
    {
        // find the debug hook in the scheduler debug hook array
        if (fbe_scheduler_compare_debug_hook(&scheduler_debug_hooks[hook_index], object_id, monitor_state, monitor_substate, val2))
        {
            // increment the counter since we hit the hook.
            scheduler_debug_hooks[hook_index].counter++;

            // If the counter transitioned from 0 to 1 increase the trace level
            if (scheduler_debug_hooks[hook_index].counter == 1)
            {
                trace_level = FBE_TRACE_LEVEL_INFO;
            }

            // Trace the hook information
            scheduler_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,"%s: hit index: %d obj: 0x%x state: %d substate: %d count: 0x%llx\n", 
                            __FUNCTION__, hook_index, scheduler_debug_hooks[hook_index].object_id, scheduler_debug_hooks[hook_index].monitor_state,
                            scheduler_debug_hooks[hook_index].monitor_substate, (unsigned long long)scheduler_debug_hooks[hook_index].counter);

            switch (scheduler_debug_hooks[hook_index].action)
            {
            case SCHEDULER_DEBUG_ACTION_LOG:
                // We just need to log the values here...
                scheduler_trace_object(scheduler_debug_hooks[hook_index].object_id, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: debug hook state: %d substate: %d action: LOG(%d)\n", 
                                       __FUNCTION__, scheduler_debug_hooks[hook_index].monitor_state, scheduler_debug_hooks[hook_index].monitor_substate, scheduler_debug_hooks[hook_index].action); 
                scheduler_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,"%s: debug hook val1 = 0x%llx\n", __FUNCTION__, (unsigned long long)scheduler_debug_hooks[hook_index].val1);
                scheduler_trace(trace_level, FBE_TRACE_MESSAGE_ID_INFO,"%s: debug hook val2 = 0x%llx\n", __FUNCTION__, (unsigned long long)scheduler_debug_hooks[hook_index].val2);
                break;
            case SCHEDULER_DEBUG_ACTION_CONTINUE:
                // If this is a continue, we dont need to do anything
                break;
            case SCHEDULER_DEBUG_ACTION_PAUSE:
                // If we want to pause a monitor, we just need to return the done status.
                scheduler_trace_object(scheduler_debug_hooks[hook_index].object_id, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: debug hook state: %d substate: %d action: PAUSE (%d)\n",
                                        __FUNCTION__, scheduler_debug_hooks[hook_index].monitor_state, scheduler_debug_hooks[hook_index].monitor_substate, scheduler_debug_hooks[hook_index].action);
                status = FBE_SCHED_STATUS_DONE;
                break;
            case SCHEDULER_DEBUG_ACTION_CLEAR:
                // If we want to clear out the hook, then we need to delete the debug hook
                scheduler_trace_object(scheduler_debug_hooks[hook_index].object_id, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: deleting debug hook state: %d substate: %d action: %d counter: 0x%llx\n", 
                                       __FUNCTION__, scheduler_debug_hooks[hook_index].monitor_state, scheduler_debug_hooks[hook_index].monitor_substate,
                                       scheduler_debug_hooks[hook_index].action, scheduler_debug_hooks[hook_index].counter);
                del_scheduler_debug_hook((fbe_u64_t)&scheduler_debug_hooks[hook_index]);
                break;
            case SCHEDULER_DEBUG_ACTION_ONE_SHOT:
                // If this is the first time set the status to done.  Otherwise continue.
                scheduler_trace_object(scheduler_debug_hooks[hook_index].object_id, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: debug hook state: %d substate: %d action: ONE SHOT (%d)\n",
                                        __FUNCTION__, scheduler_debug_hooks[hook_index].monitor_state, scheduler_debug_hooks[hook_index].monitor_substate, scheduler_debug_hooks[hook_index].action);
                if (scheduler_debug_hooks[hook_index].counter == 1)
                {
                    status = FBE_SCHED_STATUS_DONE;
                }
                break;
            default:
                if (scheduler_debug_hooks[hook_index].object_id==FBE_OBJECT_ID_INVALID)
                {
                    // possibly object deleted
                    scheduler_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: hook %d deleted for object ID: 0x%X\n", __FUNCTION__, hook_index, object_id);
                    /* object is deleted is not an error case, it's just a race that should be handled gracefully, so we return OK status */
                }
                else
                {
                    // An unknown action was supplied, we need to print an error level trace...
                    scheduler_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: unknown debug hook action: %d\n", __FUNCTION__, scheduler_debug_hooks[hook_index].action);
                    status = FBE_SCHED_STATUS_ERROR;
                }
                break;
            }
            return status;
        }
    }
    return status;
} // End fbe_scheduler_debug_hook()

/*!**************************************************************
 * fbe_scheduler_compare_debug_hook()
 ****************************************************************
 * @brief
 *  This function will compare to see if a given hook is set.
 *
 * @param hook - pointer to the hook
 * @param object_id - The object id
 * @param monitor_state - The current state of the monitor
 * @param monitor_substate - The current substate of the monitor
 * @param val2 - A generic value that can be used for comparisons
 *
 * @return fbe_bool_t
 *
 * @author
 *  8/31/2011 - Created. Jason White
 *
 ****************************************************************/
static fbe_bool_t fbe_scheduler_compare_debug_hook(fbe_scheduler_debug_hook_t *hook,
                                                   fbe_object_id_t object_id,
                                                   fbe_u32_t monitor_state,
                                                   fbe_u32_t monitor_substate,
                                                   fbe_u64_t val2)
{
    fbe_bool_t hook_found = FBE_FALSE;

    if (hook->check_type == SCHEDULER_CHECK_STATES &&
        hook->monitor_state == monitor_state &&
        hook->monitor_substate == monitor_substate &&
        hook->object_id == object_id)
    {
        hook_found = FBE_TRUE;
    }
    else if (hook->check_type == SCHEDULER_CHECK_VALS_EQUAL &&
             hook->monitor_state == monitor_state &&
             hook->monitor_substate == monitor_substate &&
             hook->object_id == object_id &&
             hook->val1 == val2)
    {
        hook_found = FBE_TRUE;
    }
    else if (hook->check_type == SCHEDULER_CHECK_VALS_LT &&
             hook->monitor_state == monitor_state &&
             hook->monitor_substate == monitor_substate &&
             hook->object_id == object_id &&
             hook->val1 <= val2)
    {
        hook_found = FBE_TRUE;
    }
    else if (hook->check_type == SCHEDULER_CHECK_VALS_GT &&
             hook->monitor_state == monitor_state &&
             hook->monitor_substate == monitor_substate &&
             hook->object_id == object_id &&
             val2 >= hook->val1)
    {
        hook_found = FBE_TRUE;
    }

    return hook_found;
}

/*!**************************************************************
 * fbe_scheduler_set_startup_hooks()
 ****************************************************************
 * @brief
 *  This function will populate the scheduler debug hooks table
 *  with the given debug hooks. This is used for inserting hooks
 *  as the SP is booting up.
 *
 * @param hook - pointer to the hooks
 *
 * @return 
 *
 * @author
 *  8/31/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void
fbe_scheduler_set_startup_hooks(fbe_scheduler_debug_hook_t *dbg_hooks) 
{
    fbe_u32_t i;

    for (i=0; i<MAX_SCHEDULER_DEBUG_HOOKS; i++)
    {
        if (dbg_hooks[i].object_id == FBE_OBJECT_ID_INVALID) 
        {
            return;
        } else {
            scheduler_debug_hooks[i].action = dbg_hooks[i].action;
            scheduler_debug_hooks[i].check_type = dbg_hooks[i].check_type;
            scheduler_debug_hooks[i].monitor_state = dbg_hooks[i].monitor_state;
            scheduler_debug_hooks[i].monitor_substate = dbg_hooks[i].monitor_substate;
            scheduler_debug_hooks[i].object_id = dbg_hooks[i].object_id;
            scheduler_debug_hooks[i].val1 = dbg_hooks[i].val1;
            scheduler_debug_hooks[i].val2 = dbg_hooks[i].val2;
        }
    }
} // End fbe_scheduler_set_startup_hooks()

/*!**************************************************************
 * has_scheduler_table_entry()
 ****************************************************************
 * @brief
 *  This function will check if the object id of the scheduler 
 *  element is in the scheduler debug hooks table.
 *
 * @param scheduler_element - a scheduler element that is being
 *   registered by the scheduler.
 *
 * @return fbe_bool_t
 *
 * @author
 *  9/8/2011 - Created. Deanna Heng
 *
 ****************************************************************/
static fbe_u32_t 
has_scheduler_table_entry(fbe_scheduler_element_t * scheduler_element) 
{
    fbe_u32_t i = 0;
    fbe_u32_t num_object_hooks = 0;
    fbe_object_id_t sched_obj_id = FBE_OBJECT_ID_INVALID;

    sched_obj_id = fbe_base_object_scheduler_element_to_object_id(scheduler_element);

    for (i=0; i<MAX_SCHEDULER_DEBUG_HOOKS; i++)
    {
        if (scheduler_debug_hooks[i].object_id == sched_obj_id) {
            num_object_hooks ++;
        }
        } 

    return num_object_hooks;
} // End has_scheduler_table_entry()

/*!**************************************************************
 * fbe_scheduler_send_control_operation()
 ****************************************************************
 * @brief
 *  This function allocates and builds a control operation in the packet.
 *  It returns error status if the payload pointer is invalid or 
 *  a control operation cannot be allocated.
 *
 * @param object_id         - target object id. 
 * @param in_packet_p       - Pointer to the packet.
 * @param in_op_code        - Operation code
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  11/1/2011 - Created. nchiu
 *
 ****************************************************************/
static fbe_status_t fbe_scheduler_send_control_operation(fbe_object_id_t                         object_id,  
                                                         fbe_payload_control_operation_opcode_t  in_op_code)
{
    fbe_packet_t *                      packet_p;
    fbe_package_id_t                    package_id;
    fbe_status_t                        status;

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        scheduler_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to alloc packet to delete debug hook for object ID: 0x%X\n", 
                        __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet_p);
    fbe_transport_build_control_packet(packet_p, 
                                        in_op_code,
                                        NULL,
                                        0,
                                        0,
                                        NULL,
                                        NULL);
    /* Set packet address */
    fbe_get_package_id(&package_id);
    fbe_transport_set_address(packet_p,
                            package_id,
                            FBE_SERVICE_ID_TOPOLOGY,
                            FBE_CLASS_ID_INVALID,
                            object_id);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    fbe_transport_release_packet(packet_p);

    return status;

}   // End fbe_scheduler_send_control_operation()


static fbe_status_t 
scheduler_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_scheduler_element_t * scheduler_element = (fbe_scheduler_element_t *)context;
    fbe_status_t status;
    fbe_object_id_t object_id;
	fbe_cpu_id_t cpu_id;

/* SAFEMESS - need a knob to enable this */
#if 0
    /* This has a huge performance impact, but we need it to detect CSX resource queue corruption */
    {
        csx_p_resource_list_validate(CSX_MY_MODULE_CONTEXT());
    }

#endif
        
    status = fbe_transport_get_status_code(packet);
    if(status == FBE_STATUS_NO_OBJECT) { /* The object is no longer exist */
        fbe_transport_release_packet(packet);
        /* Reset packet pointer */
        scheduler_element->packet = NULL;
        scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_DESTROYED;
        return FBE_STATUS_OK;
    }

#if 0
    /* We expect scheduler element is on some queue, otherwise, it's destroyed. */
    if (!fbe_queue_is_element_on_queue(&scheduler_element->queue_element)) {
        scheduler_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: %llx double completion detected and object already destroyed. \n", __FUNCTION__, scheduler_element);
        fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    }
#endif

    object_id = fbe_base_object_scheduler_element_to_object_id(scheduler_element);

    fbe_spinlock_lock(&scheduler_lock);

    /* Remove element from pending queue */
    //fbe_queue_remove(&scheduler_element->queue_element);

    /* Reset packet pointer */
    scheduler_element->packet = NULL;

    if(scheduler_element->scheduler_state & FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST){
        /* remove flag */
        scheduler_element->scheduler_state &= ~FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST;
        /* Push element to run queue */
        scheduler_element->time_counter = 0;
        //fbe_queue_push(&scheduler_run_queue_head, &scheduler_element->queue_element);
        scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_RUN;
        fbe_spinlock_unlock(&scheduler_lock);
		
		fbe_transport_get_cpu_id(packet, &cpu_id);

		if(cpu_id >= fbe_scheduler_cpu_count){
	        scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: Invalid cpu_id \n", __FUNCTION__);
			cpu_id = 0;
		}

		fbe_multicore_queue_lock(&scheduler_run_queue, cpu_id);
		fbe_multicore_queue_push(&scheduler_run_queue, &scheduler_element->queue_element, cpu_id);
		scheduler_run_queue_counter[cpu_id]++;
		fbe_rendezvous_event_set(&scheduler_run_queue_event[cpu_id]);
		fbe_multicore_queue_unlock(&scheduler_run_queue, cpu_id);

        /* wake up the run_queue thread */
        //fbe_semaphore_release(&scheduler_run_queue_semaphore, 0, 1, FALSE);
        fbe_transport_release_packet(packet);
        return FBE_STATUS_OK;
    } else {
        /* If object did not set the time_counter we will set it to three cicles */
        if(scheduler_element->time_counter == 0) {
            scheduler_element->time_counter = FBE_SCHEDULER_DEFAULT_TIMER;
        }

        /* Push element to idle queue */
        fbe_queue_push(&scheduler_idle_queue_head, &scheduler_element->queue_element);
        scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_IDLE;
    }

    fbe_spinlock_unlock(&scheduler_lock);

    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

/* Dispatch first element on the queue */
static void 
scheduler_dispatch_run_queue(void *context)
{
    fbe_packet_t * packet;
    fbe_scheduler_element_t *scheduler_element = NULL;
    fbe_scheduler_thread_info_t *scheduler_thread_info = (fbe_scheduler_thread_info_t *) context;
    fbe_object_id_t object_id;
    fbe_status_t status;	
    fbe_package_id_t package_id;
	fbe_queue_head_t tmp_queue;

    fbe_queue_init(&tmp_queue);

	fbe_multicore_queue_lock(&scheduler_run_queue, scheduler_thread_info->cpu_id);  
	fbe_rendezvous_event_clear(&scheduler_run_queue_event[scheduler_thread_info->cpu_id]);

	while(scheduler_element = (fbe_scheduler_element_t *)fbe_multicore_queue_pop(&scheduler_run_queue, scheduler_thread_info->cpu_id)){
		scheduler_run_queue_counter[scheduler_thread_info->cpu_id]--;
		scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_PENDING;
		fbe_queue_push(&tmp_queue, &scheduler_element->queue_element);
	}
	fbe_multicore_queue_unlock(&scheduler_run_queue, scheduler_thread_info->cpu_id);

	while(scheduler_element = (fbe_scheduler_element_t *)fbe_queue_pop(&tmp_queue)){
		packet = fbe_transport_allocate_packet();

	    object_id = fbe_base_object_scheduler_element_to_object_id(scheduler_element);

		scheduler_thread_info->object_id = object_id;
		scheduler_thread_info->dispatch_timestamp = fbe_get_time();

		fbe_transport_initialize_packet(packet);    
		fbe_transport_set_cpu_id(packet, scheduler_thread_info->cpu_id);

		fbe_transport_build_control_packet(packet, 
									FBE_BASE_OBJECT_CONTROL_CODE_MONITOR,
									scheduler_element,
									sizeof(fbe_scheduler_element_t),
									sizeof(fbe_scheduler_element_t),
									scheduler_completion,
									scheduler_element);

		fbe_get_package_id(&package_id);
		fbe_transport_set_address(packet, package_id, FBE_SERVICE_ID_TOPOLOGY, FBE_CLASS_ID_INVALID, object_id);

		scheduler_element->packet = packet;

#if 0    
		fbe_spinlock_lock(&scheduler_lock);
		fbe_queue_push(&scheduler_pending_queue_head, &scheduler_element->queue_element);
		//scheduler_element->current_queue = FBE_SCHEDULER_QUEUE_TYPE_PENDING;
		fbe_spinlock_unlock(&scheduler_lock);
#endif

		status = fbe_topology_send_monitor_packet(packet);

		/* Clear out the exceed_counter and timestamp in the thread_info */
		scheduler_thread_info->dispatch_timestamp = 0;
		scheduler_thread_info->exceed_counter = 0;
	}
	
	fbe_queue_destroy(&tmp_queue);

}


void fbe_scheduler_get_control_mem_use_mb(fbe_u32_t *memory_use_mb_p)
{
    fbe_u32_t memory_mb = 0;

    memory_mb += FBE_SCHEDULER_PACKET_COUNT * sizeof(fbe_packet_t);
    memory_mb = ((memory_mb + (1024*1024) - 1) / (1024*1024));
    *memory_use_mb_p = memory_mb;
}

static void fbe_scheduler_check_is_need_to_restart_bgs(void)
{
	fbe_time_t 	time_since_bgs_disabled = 0;
    fbe_status_t status;
	
	time_since_bgs_disabled = fbe_get_elapsed_seconds(scheduler_bgs_monitor_start_time);
	if ((time_since_bgs_disabled > sec_to_wait_before_bg_enable)) {

		fbe_base_config_control_system_bg_service_t control_bgs;

		check_for_bgs_enable = FBE_FALSE;/*one time only, since after we enable, we are all done with this check*/
		 
		/*and enable them if no one did so far. We don't want to blindly do so because if someone enabled/disbaled some
		of them early after the reboot for some reason, we don't want to overide that, we pass the stucture w/o any memberss iniazlied, this is ok for this usurper*/
		fbe_scheduler_send_control_operation_to_base_config_class(FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_BG_SERVICE_STATUS, &control_bgs);

		if (!control_bgs.enabled_externally) {
			control_bgs.enable = FBE_TRUE;
			control_bgs.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
			control_bgs.issued_by_ndu = FBE_FALSE;
            control_bgs.issued_by_scheduler = FBE_TRUE;
			status = fbe_scheduler_send_control_operation_to_base_config_class(FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE, &control_bgs);
			if (status != FBE_STATUS_OK) {
				/*we better panic here, since if not, we may lose data due to rebuilds or verifies not working*/
				scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to restart background operations\n", __FUNCTION__ );
			}else{
				/* we want this to be visible in traces*/
				scheduler_trace(FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
					"%s: No one enabled BGS, we had to do it\n", __FUNCTION__);
			}
		}

	}
	
}

static fbe_status_t fbe_scheduler_restore_queue_depth(void)
{
    fbe_packet_t *                                      packet_p;
    fbe_status_t                                        status;
    fbe_topology_control_get_object_id_of_singleton_t   obj_id;

    packet_p = fbe_transport_allocate_packet();
    if(packet_p == NULL)
    {
        scheduler_trace(FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to alloc packet\n", 
                        __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    obj_id.object_id    = FBE_OBJECT_ID_INVALID;
    obj_id.class_id     = FBE_CLASS_ID_DRIVE_MGMT;

    fbe_transport_initialize_packet(packet_p);
    fbe_transport_build_control_packet(packet_p, 
                                       FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_ID_OF_SINGLETON_CLASS,
                                       &obj_id,
                                       sizeof(fbe_topology_control_get_object_id_of_singleton_t),
                                       sizeof(fbe_topology_control_get_object_id_of_singleton_t),
                                       NULL,
                                       NULL);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_ESP,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_reuse_packet(packet_p);

    fbe_transport_build_control_packet(packet_p, 
                                       FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESET_SYSTEM_DRIVE_QUEUE_DEPTH,
                                       NULL,
                                       0,
                                       0,
                                       NULL,
                                       NULL);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_ESP,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              obj_id.object_id);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);

    fbe_transport_release_packet(packet_p);

    return status;
}

static void fbe_scheduler_check_need_to_restore_queue_depth(void)
{
    fbe_time_t time_since_reduced = 0;
    fbe_status_t status;

    time_since_reduced = fbe_get_elapsed_seconds(scheduler_queue_depth_monitor_start_time);
    if(time_since_reduced > sec_to_wait_before_queue_depth_restore)
    {
        status = fbe_scheduler_restore_queue_depth();
        scheduler_trace(FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: No one restored sys drive Q depth, we had to do it\n",
                        __FUNCTION__);
    }
}

static fbe_status_t fbe_scheduler_send_control_operation_to_base_config_class(fbe_payload_control_operation_opcode_t  in_op_code,
																			  fbe_base_config_control_system_bg_service_t *bg_operation)
{
    fbe_packet_t *                      packet_p;
    fbe_status_t                        status;

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        scheduler_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to alloc packet\n", __FUNCTION__ );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(packet_p);
    fbe_transport_build_control_packet(packet_p, 
									   in_op_code,
                                        bg_operation,
                                        sizeof(fbe_base_config_control_system_bg_service_t),
                                        sizeof(fbe_base_config_control_system_bg_service_t),
                                        NULL,
                                        NULL);
    /* Set packet address */
    fbe_transport_set_address(packet_p,
                            FBE_PACKAGE_ID_SEP_0,
                            FBE_SERVICE_ID_TOPOLOGY,
                            FBE_CLASS_ID_BASE_CONFIG,
                            FBE_OBJECT_ID_INVALID);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    fbe_transport_release_packet(packet_p);

    return status;

} 

/*issued when starting up or when NDU process disabled bgs to start NDU or Commit*/
void fbe_scheduler_start_to_monitor_background_service_enabled(void)
{
	scheduler_bgs_monitor_start_time = fbe_get_time();
	/*usally we don't like to do it but there is no point of having a whole kernel implementation for the schduller only for this one*/
	/*these defines say that on simulation or checked builds we don't do this check.The reason is that these are targets that are used
	for testing and the tester may want to disable the background services for some purpoes so we don't want to override it*/
	#if defined(UMODE_ENV) || defined(SIMMODE_ENV)|| defined DBG
	check_for_bgs_enable = FBE_FALSE;
	#else
	check_for_bgs_enable = FBE_TRUE;
	#endif
}

/*issued when starting up or when NDU process disabled bgs to start NDU or Commit*/
void fbe_scheduler_start_to_monitor_system_drive_queue_depth(void)
{
    scheduler_queue_depth_monitor_start_time    = fbe_get_time();
    check_for_queue_depth_restore               = FBE_TRUE;
}

/*issued when starting up or when NDU process disabled bgs to start NDU or Commit*/
void fbe_scheduler_stop_monitoring_system_drive_queue_depth(void)
{
    scheduler_queue_depth_monitor_start_time    = 0;
    check_for_queue_depth_restore               = FBE_FALSE;
}

static fbe_status_t 
scheduler_plugin_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_cpu_id_t cpu_id;

	fbe_transport_get_cpu_id(packet, &cpu_id);

    scheduler_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Entry ...\n", __FUNCTION__);

	fbe_transport_destroy_packet(packet);

    if (fbe_queue_is_element_on_queue(&packet->queue_element))
    {
        scheduler_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "packet %p is already on some queue\n", packet);
    }

	fbe_multicore_queue_lock(&scheduler_run_queue, cpu_id);
	fbe_queue_element_init(&packet->queue_element);
	fbe_multicore_queue_push(&scheduler_packet_queue, &packet->queue_element, cpu_id);
	scheduler_packet_queue_counter[cpu_id]++;
	fbe_multicore_queue_unlock(&scheduler_run_queue, cpu_id);

	return FBE_STATUS_OK;
}


fbe_status_t fbe_scheduler_register_encryption_plugin(fbe_scheduler_plugin_t * plugin)
{
	scheduler_encryption_plugin.plugin_init = plugin->plugin_init;
	scheduler_encryption_plugin.plugin_destroy = plugin->plugin_destroy;
	scheduler_encryption_plugin.plugin_entry = plugin->plugin_entry;

	return FBE_STATUS_OK;
}
