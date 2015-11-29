/***************************************************************************
 *  fbe_terminator_miniport_api.c
 ***************************************************************************
 *
 *  Description
 *      APIs to emulate the miniport calss
 *
 *
 *  History:
 *      06/11/08    sharel  Created
 *      02/07/11    Srini M Added new function to set affinity based on
 *                          algorithm (setting affinity to highest core)
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "terminator_sas_io_api.h" /*temporary*/
#include "fbe/fbe_topology_interface.h"
#include "fbe_terminator_miniport_sas_device_table.h"
#include "fbe_terminator_common.h"
#include "fbe/fbe_winddk.h"

/**********************************/
/*        local variables         */
/**********************************/
#define TERMINATOR_MAX_IO_SLOTS 4096

typedef enum worker_thread_operation_e{
    LOGIN_ALL_DEVICES,
    LOGOUT_ALL_DEVICES,
    LOGIN_SPECIFIC_DEVICE,
    LOGOUT_SPECIFIC_DEVICE
}worker_thread_operation_t;

typedef enum terminator_worker_thread_flag_e{
    TERMINATOR_WORKER_THREAD_RUN,
    TERMINATOR_WORKER_THREAD_STOP,
    TERMINATOR_WORKER_THREAD_DONE
}terminator_worker_thread_flag_t;

typedef struct event_thread_info_s{
    fbe_u32_t       port_index;
}event_thread_info_t;

typedef struct worker_thread_info_s{
    fbe_queue_element_t queue_element;
    fbe_thread_t        thread_handle;
    terminator_worker_thread_flag_t worker_flag;
}worker_thread_info_t;

typedef struct registration_thread_info_s{
    worker_thread_info_t worker_thread;
    fbe_u32_t       port_index;
}registration_thread_info_t;

typedef struct device_reset_thread_info_s{
    worker_thread_info_t worker_thread;
    fbe_u32_t       port_index;
    fbe_terminator_device_ptr_t     device_handle;
}device_reset_thread_info_t;

static fbe_spinlock_t       worker_thread_lock;
static fbe_semaphore_t      worker_thread_semaphore;
static fbe_queue_head_t     worker_thread_cleanup_queue;
static terminator_worker_thread_flag_t  worker_cleanup_thread_flag;
static fbe_thread_t         worker_cleanup_thread_handle;

static fbe_queue_head_t     terminator_io_slots;
static fbe_mutex_t           io_slots_mutex;
static fbe_u32_t        outstanding_ios = 0;
static terminator_pmc_port_t    terminator_pmc_port_array[FBE_PMC_SHIM_MAX_PORTS];
static fbe_thread_t     events_thread_handle[FBE_PMC_SHIM_MAX_PORTS];
static terminator_worker_thread_flag_t  events_thread_flag[FBE_PMC_SHIM_MAX_PORTS];
static fbe_semaphore_t          update_semaphore[FBE_PMC_SHIM_MAX_PORTS];
static fbe_semaphore_t          update_complete_semaphore[FBE_PMC_SHIM_MAX_PORTS];
static fbe_u32_t                open_count = 0;

/******************************/
/*     local function         */
/*****************************/

static void worker_thread_cleanup_func(void * context);
static void event_thread_func(void * context);
static void login_all_thread_func(void * context);
static void miniport_api_process_events(fbe_u32_t port_index);
static void device_reset_thread_func(void *context);

static fbe_status_t terminator_miniport_api_complete_io_run_queue_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_bool_t fbe_terminator_miniport_validate_encryption_info(fbe_payload_ex_t * payload);

static void terminator_io_slots_lock(void)
{
	fbe_mutex_lock(&io_slots_mutex);
}

static void terminator_io_slots_unlock(void)
{
	fbe_mutex_unlock(&io_slots_mutex);
}

/*********************************************************************
 *            fbe_terminator_miniport_api_init ()
 *********************************************************************
 *
 *  Description: initialize the terminator miniport api specific data structures
 *
 *  Inputs:
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/12/08: sharel   created
 *
 *********************************************************************/

 fbe_status_t fbe_terminator_miniport_api_init(void)
 {
     fbe_u32_t                  i = 0;
     fbe_u32_t                  slot = 0;
     fbe_terminator_io_t *      io_slot = NULL;
     EMCPAL_STATUS              nt_status;

     for(i = 0 ; i < FBE_PMC_SHIM_MAX_PORTS; i++) {
        terminator_pmc_port_array[i].state = PMC_PORT_STATE_INITIALIZED;
        terminator_pmc_port_array[i].port_handle = NULL;
        terminator_pmc_port_array[i].payload_completion_function = NULL;
        terminator_pmc_port_array[i].payload_completion_context = NULL;
        terminator_pmc_port_array[i].callback_function = NULL;
        terminator_pmc_port_array[i].callback_context = NULL;
        /*initialize the semaphore that will control the thread*/
#if 0 /* SAFEBUG - now done elsewhere */
        fbe_semaphore_init(&update_semaphore[i], 0, FBE_SEMAPHORE_MAX);
        fbe_semaphore_init(&update_complete_semaphore[i], 0, FBE_SEMAPHORE_MAX);
#endif

        /* init the device table */
        if (FBE_STATUS_OK != terminator_miniport_sas_device_table_init(
            MAX_DEVICES_PER_SAS_PORT, &terminator_pmc_port_array[i].device_table))
        {
            terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: terminator_miniport_sas_device_table_init failed on port %d!\n",
                     __FUNCTION__, i);
        }
     }

     /*allocate io slots*/
    fbe_queue_init (&terminator_io_slots);
    fbe_mutex_init(&io_slots_mutex);
    for (slot = 0; slot <TERMINATOR_MAX_IO_SLOTS; slot++) {
        io_slot = (fbe_terminator_io_t *)fbe_terminator_allocate_memory (sizeof(fbe_terminator_io_t));
        if (io_slot == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s failed to allocate memory for IO slot at line %d.\n",
                             __FUNCTION__,
                             __LINE__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_zero_memory(io_slot, sizeof(fbe_terminator_io_t));

        fbe_queue_push (&terminator_io_slots, &io_slot->queue_element);
    }

    worker_cleanup_thread_flag = TERMINATOR_WORKER_THREAD_RUN;
    fbe_queue_init(&worker_thread_cleanup_queue);
    fbe_spinlock_init (&worker_thread_lock);
    fbe_semaphore_init(&worker_thread_semaphore, 0, FBE_SEMAPHORE_MAX);

    /*start the thread that will clean up worker thread*/
    nt_status = fbe_thread_init(&worker_cleanup_thread_handle, "fbe_term_cln", worker_thread_cleanup_func, NULL);

    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: can't start event thread %X\n", __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: success !\n", __FUNCTION__);

    open_count++;
    return FBE_STATUS_OK;

 }

fbe_status_t fbe_terminator_miniport_api_enumerate_cpd_ports(fbe_cpd_shim_enumerate_backend_ports_t * port_info)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       total_ports = 0;
    fbe_u32_t       ii;
    fbe_terminator_port_shim_backend_port_info_t port_array[FBE_TERMINATOR_PORT_SHIM_MAX_PORTS];

    if (port_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: null pointer passed in\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_enumerate_ports (port_array, &total_ports);
//  if (total_ports > port_info->number_of_io_ports) {
//      total_ports = port_info->number_of_io_ports;
//  }

    for(ii = 0; ii < total_ports; ii++) {
        port_info->io_port_array[ii].port_number = port_array[ii].port_number;
        port_info->io_port_array[ii].portal_number = port_array[ii].portal_number;
        port_info->io_port_array[ii].assigned_bus_number = port_array[ii].backend_port_number;
        port_info->io_port_array[ii].port_type = port_array[ii].port_type;
        port_info->io_port_array[ii].connect_class = port_array[ii].connect_class;
        port_info->io_port_array[ii].port_role = port_array[ii].port_role;

    }
    for(; ii < FBE_CPD_SHIM_MAX_PORTS; ii++) {
        port_info->io_port_array[ii].port_number = FBE_CPD_SHIM_INVALID_PORT_HANDLE;
        port_info->io_port_array[ii].portal_number = 0;
        port_info->io_port_array[ii].assigned_bus_number = 0;
        //port_info->io_port_array[ii].port_type = FBE_CPD_SHIM_INTERFACE_TYPE_INVALID;
    }
    port_info->number_of_io_ports = total_ports;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: returned %d ports\n", __FUNCTION__, total_ports);
    return status;
}


fbe_status_t fbe_terminator_miniport_api_port_init(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_u32_t *port_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t  port_handle;

    status = terminator_get_port_ptr_by_port_portal(io_port_number, io_portal_number, &port_handle);
    if (status != FBE_STATUS_OK)
    {
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: terminator_get_port_ptr_by_port_portal failed\n", __FUNCTION__);
    return status;
    }
    status = terminator_get_miniport_port_index(port_handle, port_index);
    if (status != FBE_STATUS_OK)
    {
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: terminator_get_miniport_port_index failed\n", __FUNCTION__);
    return status;
    }
    terminator_pmc_port_array[*port_index].port_handle = port_handle;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_port_destroy(fbe_u32_t port_index)
{
    /*
    terminator_pmc_port_array[port_index].state = PMC_PORT_STATE_INITIALIZED;
    terminator_pmc_port_array[port_index].port_handle = NULL;
    */
    return FBE_STATUS_OK;
}


fbe_status_t fbe_terminator_miniport_api_register_callback(fbe_u32_t port_index,
                                                           fbe_terminator_miniport_api_callback_function_t callback_function,
                                                           void * callback_context)
{
    EMCPAL_STATUS                   nt_status;
    registration_thread_info_t  *   thread_info = NULL;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || callback_function == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: callback registration failed for port index 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_pmc_port_array[port_index].callback_function = (fbe_cpd_shim_callback_function_t)callback_function;
    terminator_pmc_port_array[port_index].callback_context = callback_context;

    /*now we start the thread that would do the logins of all devices
    to whoever registered to the driver*/
    thread_info = (registration_thread_info_t *)fbe_allocate_nonpaged_pool_with_tag ( sizeof (registration_thread_info_t), 'mret');
    if (thread_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: thread info memory allocation failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    thread_info->port_index = port_index;

    thread_info->worker_thread.worker_flag = TERMINATOR_WORKER_THREAD_RUN;
    fbe_spinlock_lock(&worker_thread_lock);
    fbe_queue_push(&worker_thread_cleanup_queue, &thread_info->worker_thread.queue_element);
    fbe_spinlock_unlock(&worker_thread_lock);

    /*start the thread that will dispatch port events as needed*/
    nt_status = fbe_thread_init(&thread_info->worker_thread.thread_handle, "fbe_term_login", login_all_thread_func, (void *)thread_info);

    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: can't start login all thread %X\n", __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: on port 0x%X, started login thread:%p\n", __FUNCTION__, port_index, thread_info);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_unregister_callback(fbe_u32_t port_index)
{
    if (port_index >= FBE_PMC_SHIM_MAX_PORTS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: callback unregistration failed for port index 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_pmc_port_array[port_index].callback_function = NULL;
    terminator_pmc_port_array[port_index].callback_context = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: callback unregistered for port index 0x%X\n", __FUNCTION__, port_index);

    return FBE_STATUS_OK;
 }

fbe_status_t fbe_terminator_miniport_api_register_sfp_event_callback(fbe_u32_t port_index,
                                                           fbe_terminator_miniport_api_callback_function_t callback_function,
                                                           void * callback_context)
{
    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || callback_function == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: sfp event callback registration failed for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
     }

     terminator_pmc_port_array[port_index].sfp_event_callback_function = (fbe_cpd_shim_callback_function_t)callback_function;
     terminator_pmc_port_array[port_index].sfp_event_callback_context = callback_context;

     terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: sfp event callback registered for port index 0x%X\n", __FUNCTION__, port_index);

     return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_unregister_sfp_event_callback(fbe_u32_t port_index)
{
    if (port_index >= FBE_PMC_SHIM_MAX_PORTS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: sfp event callback unregistration failed for port index 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_pmc_port_array[port_index].sfp_event_callback_function = NULL;
    terminator_pmc_port_array[port_index].sfp_event_callback_context = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: sfp event callback unregistered for port index 0x%X\n", __FUNCTION__, port_index);

    return FBE_STATUS_OK;
 }

fbe_status_t fbe_terminator_miniport_api_register_payload_completion(fbe_u32_t port_index,
                                    fbe_payload_ex_completion_function_t completion_function,
                                    fbe_payload_ex_completion_context_t  completion_context)
{
    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || completion_function == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: callback registration failed for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
     }

     terminator_pmc_port_array[port_index].payload_completion_function = completion_function;
     terminator_pmc_port_array[port_index].payload_completion_context = completion_context;

     /* terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: payload_completion_function registered for port index 0x%X\n", __FUNCTION__, port_index); */

     return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_unregister_payload_completion(fbe_u32_t port_index)
{
    if (port_index >= FBE_PMC_SHIM_MAX_PORTS) {
         terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: callback un registration failed for port index 0x%X\n", __FUNCTION__, port_index);
         return FBE_STATUS_GENERIC_FAILURE;
     }

     terminator_pmc_port_array[port_index].payload_completion_function = NULL;
     terminator_pmc_port_array[port_index].payload_completion_context = NULL;

     terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: payload_completion_function un registered for port index 0x%X\n", __FUNCTION__, port_index);

     return FBE_STATUS_OK;
}

/*this thread would allow us to run in a context different than that of the caller who generated the event*/
static void event_thread_func(void * context)
{
    EMCPAL_STATUS           nt_status;
    event_thread_info_t *   event_thread_info = (event_thread_info_t *)context;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: started for port index 0x%X\n",
                     __FUNCTION__,
                     event_thread_info->port_index);

    /* If we are not building code under user or simulation mode, then we must be
     * building for VM Simulator, so set the thread priority to EMCPAL_THREAD_PRIORITY_REALTIME_LOW.
     */
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    EmcpalThreadPrioritySet(EMCPAL_THREAD_PRIORITY_REALTIME_LOW);
#endif // !(UMODE_ENV) && !(SIMMODE_ENV)

    fbe_thread_set_affinity_mask_based_on_algorithm(&events_thread_handle[event_thread_info->port_index], HIGHEST_CORE);

    while(1){
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&update_semaphore[event_thread_info->port_index], NULL);

        /*make sure we are not suppose to get out of here*/
        if(events_thread_flag[event_thread_info->port_index] == TERMINATOR_WORKER_THREAD_RUN) {
            /*do we even need to go and do anything, is there anyone registered to get events here?*/
            if (terminator_pmc_port_array[event_thread_info->port_index].callback_function != NULL){
                miniport_api_process_events(event_thread_info->port_index);
            }
            /*someone may want to block until the we are done here. takes down the semaphore*/
            fbe_semaphore_release(&update_complete_semaphore[event_thread_info->port_index], 0, 1, FALSE);

        } else {
            break;
        }
    }

    events_thread_flag[event_thread_info->port_index] = TERMINATOR_WORKER_THREAD_DONE;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: exit !, port 0x%X\n", __FUNCTION__, event_thread_info->port_index);

    fbe_release_nonpaged_pool_with_tag(context, 'mret');/*don't forget to delete the context we got*/
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*this thread would allow us to clean up worker threads */
static void worker_thread_cleanup_func(void * context)
{
    EMCPAL_STATUS           nt_status;
    fbe_queue_element_t *   queue_element;
    worker_thread_info_t *  worker_thread;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: started\n",
                     __FUNCTION__);

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    EmcpalThreadPrioritySet(EMCPAL_THREAD_PRIORITY_REALTIME_LOW);
#endif // !(UMODE_ENV) && !(SIMMODE_ENV)

    fbe_thread_set_affinity_mask_based_on_algorithm(&worker_cleanup_thread_handle, HIGHEST_CORE);

    while(1){
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&worker_thread_semaphore, NULL);

        /*make sure we are not suppose to get out of here*/
        if(worker_cleanup_thread_flag == TERMINATOR_WORKER_THREAD_RUN) {
            /* find a thread needed clean up*/
            fbe_spinlock_lock(&worker_thread_lock);
            queue_element = fbe_queue_front(&worker_thread_cleanup_queue);
            while (queue_element!=NULL) {
                worker_thread = (worker_thread_info_t *) queue_element;
                if (worker_thread->worker_flag == TERMINATOR_WORKER_THREAD_DONE) {
                    fbe_queue_remove(queue_element);
                    fbe_spinlock_unlock(&worker_thread_lock);
                    /* wait for thread and clean up the context */
                    fbe_thread_wait(&worker_thread->thread_handle);
                    fbe_thread_destroy(&worker_thread->thread_handle);
                    fbe_release_nonpaged_pool_with_tag(worker_thread, 'mret');/*don't forget to delete the context we got*/
                    /*start over */
                    fbe_spinlock_lock(&worker_thread_lock);
                    queue_element = fbe_queue_front(&worker_thread_cleanup_queue);
                }
                else {
                    queue_element = fbe_queue_next(&worker_thread_cleanup_queue, queue_element);
                }
            }
            fbe_spinlock_unlock(&worker_thread_lock);
        } else {
            break;
        }
    }

    /* wait for outstanding worker threads exiting */
    fbe_spinlock_lock(&worker_thread_lock);
    queue_element = fbe_queue_front(&worker_thread_cleanup_queue);
    while (queue_element!=NULL) {
        worker_thread = (worker_thread_info_t *) queue_element;
        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: wait for worker thread %p to exit !\n", __FUNCTION__, worker_thread);
        fbe_queue_remove(queue_element);
        fbe_spinlock_unlock(&worker_thread_lock);
        /* wait for thread and clean up the context */
        fbe_thread_wait(&worker_thread->thread_handle);
        fbe_thread_destroy(&worker_thread->thread_handle);
        fbe_release_nonpaged_pool_with_tag(worker_thread, 'mret');/*don't forget to delete the context we got*/
        /*start over */
        fbe_spinlock_lock(&worker_thread_lock);
        queue_element = fbe_queue_front(&worker_thread_cleanup_queue);
    }
    fbe_spinlock_unlock(&worker_thread_lock);

    worker_cleanup_thread_flag = TERMINATOR_WORKER_THREAD_DONE;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: exit !\n", __FUNCTION__);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


static void login_all_thread_func(void * context)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpd_shim_callback_info_t    callback_info;
    registration_thread_info_t *    thread_info = (registration_thread_info_t *)context;
    fbe_u32_t                       port = thread_info->port_index;


    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: for port 0x%X, thread started...\n", __FUNCTION__, port);

    if (terminator_pmc_port_array[port].callback_function == NULL) {
        return;
    }

    fbe_thread_set_affinity_mask_based_on_algorithm(&thread_info->worker_thread.thread_handle, HIGHEST_CORE);

    /*first of all, since it's the first registration, we need to send the port messages*/
    callback_info.callback_type = FBE_PMC_SHIM_CALLBACK_TYPE_LINK_UP;

    terminator_pmc_port_array[port].callback_function (&callback_info, terminator_pmc_port_array[port].callback_context);

    /* Terminator should have the device(s) that need the login marked login pending.
     * so call the process event thread to handle the login */
    status = fbe_terminator_miniport_api_device_state_change_notify (port);

    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Miniport state change notify failed\n");
        return;
    }

    thread_info->worker_thread.worker_flag = TERMINATOR_WORKER_THREAD_DONE;
    fbe_semaphore_release(&worker_thread_semaphore, 0, 1, FALSE);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: for port index 0x%X, thread done\n", __FUNCTION__, port);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

fbe_status_t fbe_terminator_miniport_api_remove_port(fbe_u32_t port_index)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpd_shim_callback_info_t    callback_info;

    /*do we even have a port here, if not skip this port*/
    if (terminator_pmc_port_array[port_index].port_handle == NULL) {
        // fbe_terminator_miniport_api_clean_sempahores(port_index); /* SAFEBUG - had to move this */
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*call the terminator database to remove all the objects and send them logouts,
    it would know the best order to make sure we remove the children before the root device*/
    status = fbe_terminator_logout_all_devices_on_port (terminator_pmc_port_array[port_index].port_handle);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: was not able to logout all devices for port index 0x%X\n", __FUNCTION__, port_index);
    }
    /* now start the thread to do the logout callback */
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    fbe_terminator_sleep(200); /* SAFEBUG - CGCG - this is totally busted... no locking between miniport_api_process_events and here - so sleep to work around race */

    /*now send a link down to the port*/
    if (terminator_pmc_port_array[port_index].callback_function != NULL) {
      callback_info.callback_type = FBE_PMC_SHIM_CALLBACK_TYPE_LINK_DOWN;
      terminator_pmc_port_array[port_index].callback_function (&callback_info, terminator_pmc_port_array[port_index].callback_context);
    }
    
    fbe_semaphore_wait(&update_complete_semaphore[port_index], NULL);
    /*now we also need to stop the thread that is in charge of working with this port*/
    events_thread_flag[port_index] = TERMINATOR_WORKER_THREAD_STOP;
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);
    /*wait for it to end*/
    fbe_thread_wait(&events_thread_handle[port_index]);
    fbe_thread_destroy(&events_thread_handle[port_index]);

    fbe_terminator_miniport_api_clean_sempahores(port_index); /* SAFEBUG - added this */

    status = terminator_destroy_all_devices(terminator_pmc_port_array[port_index].port_handle);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: was not able to destory all devices for port index 0x%X\n", __FUNCTION__, port_index);
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: for port index 0x%X, done\n", __FUNCTION__, port_index);
    
    /* clear the  port_handle */
    terminator_pmc_port_array[port_index].port_handle = NULL;

    /* clear the  port_handle */
    terminator_pmc_port_array[port_index].port_handle = NULL;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_destroy(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t slot;
    fbe_u32_t               i = 0;
    fbe_terminator_io_t *   terminator_io = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    open_count--;
    if (open_count != 0) {
        return FBE_STATUS_OK;
    }

    /*we might still have some outstanding ios.
    techincally, we can stay here forever since we don't count how many times we waited...*/
    terminator_io_slots_lock();
    while (outstanding_ios != 0) {
        terminator_io_slots_unlock();
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: we have more io's outstanding, waiting...!!!\n", __FUNCTION__);
        fbe_thread_delay(2000);
        terminator_io_slots_lock();
    }
    terminator_io_slots_unlock();

     for(i = 0 ; i < FBE_PMC_SHIM_MAX_PORTS; i++) {
        if (terminator_pmc_port_array[i].callback_function != NULL) {
            fbe_terminator_miniport_api_remove_port(i);
            //fbe_semaphore_destroy(&update_semaphore[i]);
            //fbe_semaphore_destroy(&update_complete_semaphore[i]);
            /*destroy all other resources*/
            terminator_pmc_port_array[i].callback_function = NULL;
            /*destroy the device table*/
            if (terminator_miniport_sas_device_table_destroy(&terminator_pmc_port_array[i].device_table)!= FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: terminator_miniport_sas_device_table_destroy failed on port %d!\n",
                         __FUNCTION__, i);
            }

        }
     }

    /*free io slots memory*/
    terminator_io_slots_lock();
    for (slot = 0; slot < TERMINATOR_MAX_IO_SLOTS; slot++) {

        if (fbe_queue_is_empty(&terminator_io_slots)) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: leaked io slots, expected %d more to free.\n",
                         __FUNCTION__, TERMINATOR_MAX_IO_SLOTS - slot);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    terminator_io = (fbe_terminator_io_t *)fbe_queue_pop (&terminator_io_slots);
    fbe_terminator_free_memory (terminator_io);
    }
    terminator_io_slots_unlock();

    fbe_queue_destroy (&terminator_io_slots);
    //fbe_spinlock_destroy(&io_slots_lock);
	fbe_mutex_destroy(&io_slots_mutex);

    worker_cleanup_thread_flag = TERMINATOR_WORKER_THREAD_STOP;
    fbe_semaphore_release(&worker_thread_semaphore, 0, 1, FALSE);
    /*wait for it to end*/
    fbe_thread_wait(&worker_cleanup_thread_handle);
    fbe_thread_destroy(&worker_cleanup_thread_handle);


    fbe_semaphore_destroy(&worker_thread_semaphore);
    fbe_queue_destroy (&worker_thread_cleanup_queue);
    fbe_spinlock_destroy(&worker_thread_lock);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: Done - Terminator miniport API exit\n", __FUNCTION__);

    return status;

}

static void miniport_api_process_events(fbe_u32_t port_index)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t         device_list[MAX_DEVICES];
    fbe_u32_t                       total_devices = 0;
    fbe_u32_t                       device_counter = 0;
    fbe_cpd_shim_callback_info_t    callback_info;
    fbe_bool_t                      login_pending = FBE_FALSE;
    fbe_bool_t                      reset_begin_set = FBE_FALSE;
    fbe_bool_t                      reset_completed_set = FBE_FALSE;
    fbe_terminator_device_ptr_t         device_id, popped_out_device_id;

    /* Reset begin logic start */
    status = terminator_get_reset_begin(terminator_pmc_port_array[port_index].port_handle, &reset_begin_set);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s:terminator_is_reset_begin_set failed for port index: 0x%X\n", __FUNCTION__, port_index);
        return;
    }
    if(reset_begin_set)
    {
        callback_info.callback_type = FBE_PMC_SHIM_CALLBACK_TYPE_DRIVER_RESET;
        callback_info.info.driver_reset.driver_reset_event_type = FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN;
        if (terminator_pmc_port_array[port_index].callback_function != NULL) {
            terminator_pmc_port_array[port_index].callback_function (&callback_info, terminator_pmc_port_array[port_index].callback_context);
        }
        status = terminator_clear_reset_begin(terminator_pmc_port_array[port_index].port_handle);
        if(status != FBE_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: terminator_clear_reset_begin_flag failed for port: 0x%X\n", __FUNCTION__, port_index);
            return;
        }
    }
    /* Reset begin logic end */

    callback_info.callback_type = FBE_PMC_SHIM_CALLBACK_TYPE_LOGOUT;

    /*1. We first get device at front of logout queue.
     *2. Then generate the logout for the device.
     *3. And then remove the device(pop) from the queue.
     * This is required as we dont want some one that
     * may be monitoring the logout queue to assume the
     * the devices log out is complete once it is taken(pop)
     * out of the queue. With this logic, as long as the
     * device exists in logout queue it is safe to assume
     * its logout is not complete.This will prevent the
     * existing timing problems during destroying the device
     * where destroy waits for the device to be taken out
     * out of the logout queue before the objects memory
     * is freed.
     */
    status = terminator_get_front_device_from_logout_queue(terminator_pmc_port_array[port_index].port_handle, &device_id);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s: failed to get next logout device on port 0x%X\n", __FUNCTION__, port_index);
        return;
    }
    while(device_id != DEVICE_HANDLE_INVALID )
    {
        /* abort all IOs of this device */
        status = terminator_abort_all_io(device_id);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to abort_all_io, device 0x%p on port 0x%X\n", __FUNCTION__, device_id, port_index);
            return;
        }
        /*send the logout*/
        status = terminator_get_device_login_data (device_id, &callback_info.info.login);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to get device login data, device 0x%p on port 0x%X\n", __FUNCTION__, device_id, port_index);
            return;
        }
        /*and issue the callback to the port object*/
        /* we dont issue login to the port but issue something like link-up, discuss with vicky */
        if (terminator_pmc_port_array[port_index].callback_function != NULL) {
            terminator_pmc_port_array[port_index].callback_function (&callback_info, terminator_pmc_port_array[port_index].callback_context);
        }

        /*set the logout_complete flag to mark that we are done for this device
         */
        status = terminator_set_device_logout_complete(device_id, FBE_TRUE);

        /* pop the device whoose logout was already generated
         */
        status = terminator_pop_device_from_logout_queue(terminator_pmc_port_array[port_index].port_handle, &popped_out_device_id);
        if(device_id != popped_out_device_id)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
             "%s: Mismatch between logout_queue_front:0x%X & logout_queue_pop:0x%p device id 0x%p\n",
             __FUNCTION__, port_index, popped_out_device_id, device_id);
            return;
        }

        /* get the next logout device id from the queue
         */
        status = terminator_get_front_device_from_logout_queue(terminator_pmc_port_array[port_index].port_handle, &device_id);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to get next logout device on port index 0x%X\n", __FUNCTION__, port_index);
            return;
        }
    }

    /* Reset completed logic start */
    status = terminator_get_reset_completed(terminator_pmc_port_array[port_index].port_handle, &reset_completed_set);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s:terminator_is_reset_completed_set failed for port: 0x%X\n", __FUNCTION__, port_index);
        return;
    }
    if(reset_completed_set)
    {
        callback_info.callback_type = FBE_PMC_SHIM_CALLBACK_TYPE_DRIVER_RESET;
        callback_info.info.driver_reset.driver_reset_event_type = FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED;
        if (terminator_pmc_port_array[port_index].callback_function != NULL) {
            terminator_pmc_port_array[port_index].callback_function (&callback_info, terminator_pmc_port_array[port_index].callback_context);
        }
        status = terminator_clear_reset_completed(terminator_pmc_port_array[port_index].port_handle);
        if(status != FBE_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s:terminator_clear_reset_completed_flag failed for port index: 0x%X\n", __FUNCTION__, port_index);
            return;
        }
    }
    /* Reset completed logic end */

    /*now that we did all the removes, let's have a new list of devices
     *ask the terminator database to give us all the devices on this port
     */
    status = terminator_enumerate_devices (terminator_pmc_port_array[port_index].port_handle, device_list, &total_devices);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s: failed to enumerate devices on port 0x%X\n", __FUNCTION__, port_index);
        return;
    }

    /*now we look for login's*/
    callback_info.callback_type = FBE_PMC_SHIM_CALLBACK_TYPE_LOGIN;

    /*we process all outstanding logins*/
    for (device_counter = 0; device_counter < total_devices; device_counter ++) {
        /*do we need to send a login to this device*/
        status = terminator_is_login_pending (device_list[device_counter], &login_pending);
        if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_DEVICE) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: failed to get device login pending status for device 0x%p on port 0x%X\n",
                             __FUNCTION__, device_list[device_counter], port_index);
            continue;
        }

        if (login_pending == 1){
            /*send the login*/
            status = terminator_get_device_login_data (device_list[device_counter], &callback_info.info.login);
            if (status != FBE_STATUS_OK) {
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: failed to get device login data, device 0x%p on port index 0x%X\n",
                                 __FUNCTION__, device_list[device_counter], port_index);
                return;
            }

            /*and issue the callback to the port object*/
            if (terminator_pmc_port_array[port_index].callback_function != NULL) {
                terminator_pmc_port_array[port_index].callback_function (&callback_info, terminator_pmc_port_array[port_index].callback_context);
            }

            /*and clear the login pending flag*/
            terminator_clear_login_pending(device_list[device_counter]);
        }
    }
    return;
}

fbe_status_t fbe_terminator_miniport_api_get_port_type (fbe_u32_t port_index, fbe_port_type_t *port_type)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_type == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port index 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
     }

    status = terminator_get_port_type (terminator_pmc_port_array[port_index].port_handle, port_type);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: returned port type:0x%X\n", __FUNCTION__, *port_type);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_get_port_address (fbe_u32_t port_index, fbe_address_t *port_address)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_address == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port index 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
     }

    status = terminator_get_port_address (terminator_pmc_port_array[port_index].port_handle, port_address);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: returned port address:0x%llX\n", __FUNCTION__,
		     (unsigned long long)port_address->sas_address);

    return status;
}

/*start a thread per port*/
fbe_status_t fbe_terminator_miniport_api_port_inserted (fbe_u32_t port_index)
{
    event_thread_info_t *   thread_info = NULL;
    EMCPAL_STATUS           nt_status;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS ) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    thread_info = (event_thread_info_t *)fbe_allocate_nonpaged_pool_with_tag ( sizeof (event_thread_info_t), 'mret');
    if (thread_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: thread info memory allocation failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    thread_info->port_index = port_index;

    events_thread_flag[port_index] = TERMINATOR_WORKER_THREAD_RUN;

    fbe_semaphore_init(&update_semaphore[port_index], 0, FBE_SEMAPHORE_MAX); /* SAFEBUG - added this */
    fbe_semaphore_init(&update_complete_semaphore[port_index], 0, FBE_SEMAPHORE_MAX); /* SAFEBUG - added this */

    /*start the thread that will dispatch port events as needed*/
    nt_status = fbe_thread_init(&events_thread_handle[port_index], "fbe_term_evt", event_thread_func, thread_info);

    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: can't start event thread %X\n", __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_set_speed (fbe_u32_t port_index, fbe_port_speed_t speed)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || speed >= FBE_PORT_SPEED_LAST) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_set_speed (terminator_pmc_port_array[port_index].port_handle, speed);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_get_port_info (fbe_u32_t port_index, fbe_port_info_t * port_info)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_port_info (terminator_pmc_port_array[port_index].port_handle, port_info);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_get_port_link_info (fbe_u32_t port_index, fbe_cpd_shim_port_lane_info_t * port_link_info)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_link_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_port_link_info (terminator_pmc_port_array[port_index].port_handle, port_link_info);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_register_keys (fbe_u32_t port_index, fbe_base_port_mgmt_register_keys_t * port_register_keys_p)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_register_keys_p == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_register_keys (terminator_pmc_port_array[port_index].port_handle, port_register_keys_p);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_reestablish_key_handle(fbe_u32_t port_index, 
                                                                fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_reestablish_key_handle_p == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_reestablish_key_handle (terminator_pmc_port_array[port_index].port_handle, port_reestablish_key_handle_p);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_unregister_keys (fbe_u32_t port_index, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_unregister_keys_p == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_unregister_keys (terminator_pmc_port_array[port_index].port_handle, port_unregister_keys_p);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_get_hardware_info (fbe_u32_t port_index, fbe_cpd_shim_hardware_info_t * hdw_info)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || hdw_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_hardware_info (terminator_pmc_port_array[port_index].port_handle, hdw_info);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_get_sfp_media_interface_info (fbe_u32_t port_index, fbe_cpd_shim_sfp_media_interface_info_t * sfp_media_interface_info)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || sfp_media_interface_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_sfp_media_interface_info (terminator_pmc_port_array[port_index].port_handle, sfp_media_interface_info);

    return status;
}

fbe_status_t fbe_terminator_miniport_api_port_configuration (fbe_u32_t port_index, fbe_cpd_shim_port_configuration_t *port_config_info)
{
    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_config_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_port_configuration (terminator_pmc_port_array[port_index].port_handle, port_config_info);

    return status;
}

//fbe_status_t fbe_terminator_miniport_api_get_hardware_info (fbe_u32_t port_index, fbe_cpd_shim_hardware_info_t * hdw_info)
//{
//    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;
//
//    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || hdw_info == NULL) {
//        terminator_trace(FBE_TRACE_LEVEL_ERROR,
//                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
//                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
//        return FBE_STATUS_GENERIC_FAILURE;
//    }
//
//    status = terminator_get_hardware_info (pmc_port_array[port_index].port_handle, hdw_info);
//
//    return status;
//}
//
//fbe_status_t fbe_terminator_miniport_api_get_sfp_media_interface_info (fbe_u32_t port_index, fbe_cpd_shim_sfp_media_interface_info_t * sfp_media_interface_info)
//{
//    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;
//
//    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || sfp_media_interface_info == NULL) {
//        terminator_trace(FBE_TRACE_LEVEL_ERROR,
//                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
//                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
//        return FBE_STATUS_GENERIC_FAILURE;
//    }
//
//    status = terminator_get_sfp_media_interface_info (pmc_port_array[port_index].port_handle, sfp_media_interface_info);
//
//    return status;
//}
//
//fbe_status_t fbe_terminator_miniport_api_port_configuration (fbe_u32_t port_index, fbe_cpd_shim_port_configuration_t *port_config_info)
//{
//    fbe_status_t        status  = FBE_STATUS_GENERIC_FAILURE;
//
//    if (port_index >= FBE_PMC_SHIM_MAX_PORTS || port_config_info == NULL) {
//        terminator_trace(FBE_TRACE_LEVEL_ERROR,
//                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
//                         "%s: ilegal arguments for port 0x%X\n", __FUNCTION__, port_index);
//        return FBE_STATUS_GENERIC_FAILURE;
//    }
//
//    status = terminator_get_port_configuration (pmc_port_array[port_index].port_handle, port_config_info);
//
//    return status;
//}

/*send payload to a device*/
fbe_status_t
fbe_terminator_miniport_api_send_payload (fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_io_t *       terminator_io = NULL;
    fbe_bool_t              logout_state = FBE_FALSE;
    fbe_payload_cdb_operation_t *   payload_cdb_operation = NULL;
     fbe_terminator_device_ptr_t  device_handle, port_handle;
	fbe_packet_t * packet = NULL;
	fbe_cpu_id_t cpu_id;
	fbe_payload_dmrb_operation_t * payload_dmrb_operation = NULL;
    fbe_bool_t                  command_valid;
    #if defined(SIMMODE_ENV)
    fbe_bool_t                      b_is_io_completion_at_dpc = FBE_FALSE;
    csx_p_execution_state_e         original_irql;
    #endif

    /* If this I/O originated as a completion we should not be running at DPC */
#if defined(SIMMODE_ENV)
    fbe_terminator_get_io_completion_at_dpc(&b_is_io_completion_at_dpc);
    if (b_is_io_completion_at_dpc == FBE_TRUE)
    {
        original_irql = EmcpalGetCurrentLevel();
        if (EMCPAL_THIS_LEVEL_IS_NOT_PASSIVE(original_irql))
        {
            EmcpalLevelLower(EMCPAL_LEVEL_PASSIVE);
        }
    }
#endif

    /* Now process the request*/
    if (payload == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    command_valid = fbe_terminator_miniport_validate_encryption_info(payload);

    if(!command_valid) 
    {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s Command Parameters not valid \n", __FUNCTION__);
    }

	packet = fbe_transport_payload_to_packet(payload);
	fbe_transport_get_cpu_id(packet, &cpu_id);
	if(cpu_id == FBE_CPU_ID_INVALID){
		terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s Invalid cpu_id\n", __FUNCTION__);
	}

	/* Check if we have enough space for DMRB operation */
	payload_dmrb_operation = fbe_payload_ex_allocate_dmrb_operation(payload);
	if(payload_dmrb_operation == NULL){
		terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s out of payload memory\n", __FUNCTION__);
	} else {
		fbe_payload_ex_increment_dmrb_operation_level(payload);
		fbe_payload_ex_release_dmrb_operation(payload, payload_dmrb_operation);
	}


    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
    if (payload_cdb_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s: payload_cdb_operation=NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (((fbe_u64_t)payload_cdb_operation%8) != 0) {
        /* Trap into debugger.*/
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"%s:payload_cdb_opt is not 64bit aligned!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check to see if the port is still active */
    if (terminator_pmc_port_array[port_index].payload_completion_function == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"terminator_miniport_send_payload:port is destroyed:0x%X\n", port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* look up the device handle from the device table*/
    fbe_terminator_miniport_api_device_table_get_device_handle(port_index, cpd_device_id, &device_handle);
    if (device_handle == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "terminator_miniport_send_payload:device not exist, abort IO:port 0x%X, device:0x%X\n",
                         port_index, cpd_device_id);
        fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }
    /*get the port handle, so that we can lock the port when enqueue an IO */
    terminator_get_port_ptr(device_handle, &port_handle);
    if (port_handle == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "terminator_miniport_send_payload:device not attach to port,abort IO:port 0x%X,device:0x%X\n",
                         port_index, cpd_device_id);
        fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }
    /* Lock the port configuration so no one else can change it while we enqueue the IO */
    terminator_lock_port(port_handle);
    /* check if the device is logging out*/
    status = terminator_is_in_logout_state(device_handle, &logout_state);
    if ((logout_state == FBE_TRUE)||(status == FBE_STATUS_GENERIC_FAILURE))
    {
        terminator_unlock_port(port_handle);
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "terminator_miniport_send_payload:device logout pending,abort IO:port 0x%X,device:0x%X\n",
                         port_index, cpd_device_id);
        fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }

    /* Check the size of I/O */
#if 0 /* this is for limited I/O size Ron will check it in soon */
    if(payload_cdb_operation->transfer_count > TERMINATOR_MAX_IO_SIZE){
        terminator_unlock_port(port_handle);
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: The I/O is too big %d bytes\n",
                         __FUNCTION__, payload_cdb_operation->transfer_count);

        fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }
#endif

    /* If we get here,it means the port exist and spinlock exist as well */
    terminator_io_slots_lock();
    /*grab the next free io slot, but check if we have some first*/
    if (fbe_queue_is_empty(&terminator_io_slots)) {
		terminator_io_slots_unlock();
        terminator_unlock_port(port_handle);
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "terminator_miniport_send_payload: io_slots queue empty IO:port 0x%X, device:0x%X\n",
                         port_index, cpd_device_id);

        fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    } else {
        /*we are good to go*/
        terminator_io = (fbe_terminator_io_t *)fbe_queue_pop (&terminator_io_slots);
        outstanding_ios++;/*don't forget to count the ios we sent*/
    }
    terminator_io_slots_unlock();

    /*fill the data in the terminator io strucutre*/
    terminator_io->payload = payload;
    terminator_io->collision_found = FBE_FALSE;
    terminator_io->is_pending = FBE_FALSE;
    terminator_io->memory_ptr = NULL;
    terminator_io->miniport_port_index = FBE_U32_MAX;

    status = terminator_send_io (device_handle, terminator_io);
    terminator_unlock_port(port_handle);
    if (status != FBE_STATUS_OK) {
        if (status == FBE_STATUS_NO_DEVICE) {
            fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        } else {
            fbe_payload_cdb_set_request_status (payload_cdb_operation, FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE);
        }
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        terminator_io_slots_lock();
        outstanding_ios--;/*we did not send the io*/
        fbe_queue_push (&terminator_io_slots, &terminator_io->queue_element);
        terminator_io_slots_unlock();
    }
    return status;
}

/*send payload to a device*/
fbe_status_t
fbe_terminator_miniport_api_send_fis (fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_io_t *       terminator_io = NULL;
    fbe_bool_t              logout_state = FBE_FALSE;
    fbe_payload_fis_operation_t *   payload_fis_operation = NULL;
    fbe_terminator_device_ptr_t  device_handle, port_handle;

    if (payload == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload_fis_operation = fbe_payload_ex_get_fis_operation(payload);
    if (payload_fis_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: payload_fis_operation == NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (((fbe_u64_t)payload_fis_operation%8) != 0) {
        /* Trap into debugger.*/
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: payload_fis_operation is not 64bit aligned!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* check to see if the port is still active */
    if (terminator_pmc_port_array[port_index].payload_completion_function == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: port has been destroyed:0x%X\n", __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_terminator_miniport_api_device_table_get_device_handle(port_index, cpd_device_id, &device_handle);
    if (device_handle == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: device does not exist anymore, abort IO:port %xX, device:0x%X\n",
                         __FUNCTION__, port_index, cpd_device_id);
        fbe_payload_fis_set_request_status (payload_fis_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }
    /*get the port handle, so that we can lock the port when enqueue an IO */
    terminator_get_port_ptr(device_handle, &port_handle);
    if (port_handle == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: device is not attached to a port, abort IO:port %xX, device:0x%X\n",
                         __FUNCTION__, port_index, cpd_device_id);
        fbe_payload_fis_set_request_status (payload_fis_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }
    /* Lock the port configuration so no one else can change it while we enqueue the IO */
    terminator_lock_port(port_handle);
    /* check if the device is logging out*/
    status = terminator_is_in_logout_state(device_handle, &logout_state);
    if ((logout_state == FBE_TRUE)||(status == FBE_STATUS_GENERIC_FAILURE))
    {
        terminator_unlock_port(port_handle);
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: device is logout pending, abort IO:port %xX, device:0x%X\n", __FUNCTION__, port_index, cpd_device_id);
        fbe_payload_fis_set_request_status(payload_fis_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        return FBE_STATUS_OK;
    }

    /* If we get here,it means the port exist and spinlock exist as well */
    terminator_io_slots_lock();
    /*grab the next free io slot, but check if we have some first*/
    if (fbe_queue_is_empty(&terminator_io_slots)) {
		terminator_io_slots_unlock();
        terminator_unlock_port(port_handle);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    } else {
        /*we are good to go*/
        terminator_io = (fbe_terminator_io_t *)fbe_queue_pop (&terminator_io_slots);
        outstanding_ios++;/*don't forget to count the ios we sent*/
    }
    terminator_io_slots_unlock();

    /*fill the data in the terminator io strucutre*/
    terminator_io->payload = payload;
    terminator_io->collision_found = FBE_FALSE;
    terminator_io->is_pending = FBE_FALSE;
    terminator_io->memory_ptr = NULL;
    terminator_io->miniport_port_index = FBE_U32_MAX;

    status = terminator_send_io (device_handle, terminator_io);
    terminator_unlock_port(port_handle);
    if (status != FBE_STATUS_OK) {
        /* TODO update with valid FIS request status
        if (status == FBE_STATUS_NO_DEVICE) {
            fbe_payload_fis_set_request_status (payload_fis_operation, FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT);
        } else {
            fbe_payload_fis_set_request_status (payload_fis_operation, FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE);
        }
        */
        terminator_pmc_port_array[port_index].payload_completion_function (payload, terminator_pmc_port_array[port_index].payload_completion_context);
        terminator_io_slots_lock();
        outstanding_ios--;/*we did not send the io*/
        fbe_queue_push (&terminator_io_slots, &terminator_io->queue_element);
        terminator_io_slots_unlock();
    }
    return status;
}



/*to be called from the terminator database when the thread completes the io*/
fbe_status_t fbe_terminator_miniport_api_complete_io (fbe_u32_t port_index, fbe_terminator_io_t *terminator_io)
{
	fbe_packet_t * packet = NULL;
	fbe_cpu_id_t cpu_id;
    fbe_queue_head_t            tmp_queue;

    fbe_queue_init(&tmp_queue);


    /*first, let's call the completion function of the miniport*/
    if(terminator_io->payload != NULL){
		packet = fbe_transport_payload_to_packet(terminator_io->payload);
		if(terminator_io->collision_found > 0){			
			fbe_debug_break();
		}
		fbe_transport_get_cpu_id(packet, &cpu_id);
		if(cpu_id == FBE_CPU_ID_INVALID){
			fbe_debug_break();
		}

		terminator_io->miniport_port_index = port_index;
		fbe_queue_push(&tmp_queue, &packet->queue_element);
		fbe_transport_run_queue_push(&tmp_queue, terminator_miniport_api_complete_io_run_queue_completion, terminator_io);
		fbe_queue_destroy(&tmp_queue);        
    }

    return FBE_STATUS_OK;
}

static fbe_status_t 
terminator_miniport_api_complete_io_run_queue_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_terminator_io_t    *terminator_io = NULL;
	fbe_u32_t               port_index;
#if defined(SIMMODE_ENV)
    fbe_bool_t              b_is_io_completion_at_dpc = FBE_FALSE;
    csx_p_execution_state_e original_irql;
#endif

	terminator_io = (fbe_terminator_io_t *)context;
	port_index = terminator_io->miniport_port_index;

    /* In the simulation environment raise to DPC before completing the packet.
     */
#if defined(SIMMODE_ENV)
    fbe_terminator_get_io_completion_at_dpc(&b_is_io_completion_at_dpc);
    if (b_is_io_completion_at_dpc == FBE_TRUE)
    {
        EmcpalLevelRaiseToDispatch(&original_irql);
    }
#endif

	terminator_pmc_port_array[port_index].payload_completion_function (terminator_io->payload,
                                                                       terminator_pmc_port_array[port_index].payload_completion_context);

    /* Now lower to the original*/
#if defined(SIMMODE_ENV)
    if (b_is_io_completion_at_dpc == FBE_TRUE)
    {
        EmcpalLevelLower(original_irql);
    }
#endif

    /*when we get back, it's safe to return the io to it's location*/
    terminator_io_slots_lock();
    outstanding_ios--;
    fbe_queue_push (&terminator_io_slots, &terminator_io->queue_element);
    terminator_io_slots_unlock();

	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*an api to mark that there are logout devices and we should do something about it*/
fbe_status_t fbe_terminator_miniport_api_device_logged_out (fbe_u32_t port_index, fbe_terminator_device_ptr_t device_id)
{
    /*we need to wake up the appropriate thread so it can go over the list
    and found the ones that are marked logout pending*/
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*an api to mark that there are login devices and we should do something about it*/
fbe_status_t fbe_terminator_miniport_api_device_state_change_notify (fbe_u32_t port_index)
{
    /*we need to wake up the appropriate thread so it can go over the list
    and found the ones that are marked login pending*/
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_start_port_reset (fbe_u32_t port_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_set_reset_begin(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;
    // Wake up the thread that sends the reset_start event to pmc shim
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t fbe_terminator_miniport_api_complete_port_reset (fbe_u32_t port_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_set_reset_completed(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;
    // Wake up the thread that sends the reset_completed event to pmc shim
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t fbe_terminator_miniport_api_auto_port_reset(fbe_u32_t port_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_set_reset_begin(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;

    // wake up the thread
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    // We will want to wait till the reset-begin flag is cleared eventhough
    // the process events thread first checks for reset-begin before checking
    // for any logouts it should generate. Because the thread may be ALREADY
    // RUNNING and past the check for reset-begin.
    status = fbe_terminator_wait_on_port_reset_clear(terminator_pmc_port_array[port_index].port_handle, FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN);
    RETURN_ON_ERROR_STATUS;

    status = fbe_terminator_logout_all_devices_on_port(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;

    // wake up the thread
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    //This is for the following situation: 1. Process events thread(assuming its already running) puts
    //logout complete on port 2. But before it goes ahead we call force-logout. 3. So the process event
    // thread now process the reset-complete before logouts which it should not. There may be other
    // scenarios like the process event thread already running and at the logout stage--etc which
    // should be ok as the process-event function always first checks for all logouts(port itself logs
    // out last after all children) before checking for reset-complete...

    // We can just commennt this to decrease complexity.
    status = fbe_terminator_wait_on_port_logout_complete(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;

    status = terminator_set_reset_completed(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;

    // wake up the thread
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    // same logic as for the same previous function call as we dont want logins to occur before
    // the thread sends a reset-clear. ( Assuming the thread already woke up and is running).
    status = fbe_terminator_wait_on_port_reset_clear(terminator_pmc_port_array[port_index].port_handle, FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED);
    RETURN_ON_ERROR_STATUS;

    status = fbe_terminator_login_all_devices_on_port(terminator_pmc_port_array[port_index].port_handle);
    RETURN_ON_ERROR_STATUS;

    // wake up the thread
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t fbe_terminator_miniport_api_all_devices_on_port_logged_out (fbe_terminator_device_ptr_t handle)
{
    fbe_u32_t i;

    for(i = 0; i < FBE_PMC_SHIM_MAX_PORTS; i++) {
        /* Locate a free entry in the table.*/
        if (terminator_pmc_port_array[i].port_handle == handle){
            /*we need to wake up the appropriate thread so it can go over the list
            and found the ones that are marked logout pending*/
            fbe_semaphore_release(&update_semaphore[i], 0, 1, FALSE);

            return FBE_STATUS_OK;

        }
    }
    /* couldn't find the right one */
    return FBE_STATUS_GENERIC_FAILURE;
    }

fbe_status_t fbe_terminator_miniport_api_all_devices_on_port_logged_in (fbe_terminator_device_ptr_t handle)
{
    fbe_u32_t i;

    for(i = 0; i < FBE_PMC_SHIM_MAX_PORTS; i++) {
        /* Locate a free entry in the table.*/
        if (terminator_pmc_port_array[i].port_handle == handle){
            /*we need to wake up the appropriate thread so it can go over the list
            and found the ones that are marked logout pending*/
            fbe_semaphore_release(&update_semaphore[i], 0, 1, FALSE);

            return FBE_STATUS_OK;

        }
    }
    /* didn't find the right one */
    return FBE_STATUS_GENERIC_FAILURE;
    }

fbe_status_t fbe_terminator_miniport_api_reset_device(fbe_u32_t port_index, fbe_miniport_device_id_t cpd_device_id)
{
    EMCPAL_STATUS nt_status;
    device_reset_thread_info_t *thread_info = NULL;

    if (port_index >= FBE_PMC_SHIM_MAX_PORTS) {
    terminator_trace(FBE_TRACE_LEVEL_ERROR,
             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
             "%s: Invalid port index 0x%X\n", __FUNCTION__, port_index);
    return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now we start the thread that would do the reset */
    thread_info = (device_reset_thread_info_t *)fbe_allocate_nonpaged_pool_with_tag ( sizeof (device_reset_thread_info_t), 'mret');
    if (thread_info == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: thread info memory allocation failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    thread_info->port_index = port_index;
    fbe_terminator_miniport_api_device_table_get_device_handle(port_index, cpd_device_id, &thread_info->device_handle);

    thread_info->worker_thread.worker_flag = TERMINATOR_WORKER_THREAD_RUN;
    fbe_spinlock_lock(&worker_thread_lock);
    fbe_queue_push(&worker_thread_cleanup_queue, &thread_info->worker_thread.queue_element);
    fbe_spinlock_unlock(&worker_thread_lock);

    /*start the thread that will do the reset*/
    nt_status = fbe_thread_init(&thread_info->worker_thread.thread_handle, "fbe_dev_reset", device_reset_thread_func, (void *)thread_info);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s: can't start device reset thread %X\n", __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: on port 0x%X, started device reset thread:%p\n", __FUNCTION__, port_index, (void *)thread_info);

    return FBE_STATUS_OK;
}

/* this thread is created whenever there is a device reset request comes in.
 * the reset is done in this thread's context
 * this thread checks if there is a customed reset callback registered by user.  If so, call it.
 * Otherwise, use the default behavior. */
static void device_reset_thread_func(void * context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    device_reset_thread_info_t *    thread_info = (device_reset_thread_info_t *)context;
    fbe_u32_t               port = thread_info->port_index;
    fbe_terminator_device_ptr_t     device_handle = thread_info->device_handle;
    fbe_terminator_device_reset_function_t reset_function = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: for port 0x%X, thread started...\n", __FUNCTION__, port);

    if (device_handle == NULL) {
    terminator_trace(FBE_TRACE_LEVEL_WARNING,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: for port 0x%X, Invalid device handle...\n", __FUNCTION__, port);
    return;
    }

    fbe_thread_set_affinity_mask_based_on_algorithm(&thread_info->worker_thread.thread_handle, HIGHEST_CORE);

    status = terminator_get_device_reset_function(device_handle, &reset_function);
    if (reset_function != NULL)
    {
    /* call the reset function of this device */
    status = reset_function(device_handle);
    }
    else
    {
    terminator_trace(FBE_TRACE_LEVEL_WARNING,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: Device reset function is not set...\n", __FUNCTION__);
    }
    /* Clean up */
    thread_info->worker_thread.worker_flag = TERMINATOR_WORKER_THREAD_DONE;
    fbe_semaphore_release(&worker_thread_semaphore, 0, 1, FALSE);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
             FBE_TRACE_MESSAGE_ID_INFO,
             "%s: for port index 0x%X, thread done\n", __FUNCTION__, port);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

fbe_status_t terminator_miniport_device_reset_common_action(fbe_terminator_device_ptr_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t    delay_in_ms = 0;
    fbe_u32_t    port_index;

    terminator_get_port_miniport_port_index(device_handle, &port_index);
    /* force logout the device.  This should cause the device to abort all pending requests. */
    terminator_logout_device(device_handle);
    /*we need to wake up the appropriate thread so it can go over the list
     *and found the ones that are marked logout pending*/
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    /* start the reset delay of this device */
    terminator_get_device_reset_delay(device_handle, &delay_in_ms);
    fbe_thread_delay(delay_in_ms);
    /* force login the device */
    terminator_login_device(device_handle);
    /*we need to wake up the appropriate thread so it can go over the list
     *and found the ones that are marked login pending*/
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    return status;
}
/*********************************************************************
*            terminator_miniport_drive_power_cycle_action ()
*********************************************************************
*
*  Description: 
*    This function power cycle (logout and login) the drive.
*
*  Inputs: 
*   device_handle - handle to device.
*
*  Return Value: 
*       success or failure
*
*  History:
*      July-03-2009: Dipak Patel created.
*    
*********************************************************************/
fbe_status_t terminator_miniport_drive_power_cycle_action(fbe_terminator_device_ptr_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t    delay_in_ms = 0;
    fbe_u32_t    port_index;
    fbe_u32_t    slot_number;
    ses_stat_elem_array_dev_slot_struct ses_stat_elem;
    fbe_terminator_device_ptr_t virtual_phy_ptr;

    terminator_get_port_miniport_port_index(device_handle, &port_index);


    status = terminator_get_drive_slot_number(device_handle, &slot_number);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s:Error !! can not get slot_number thru drive handle\n", __FUNCTION__);
        return status;
    }

    terminator_get_drive_parent_virtual_phy_ptr(device_handle, &virtual_phy_ptr);

    /* Update the status page for device off = 1 */
    terminator_get_drive_eses_status(virtual_phy_ptr,slot_number, &ses_stat_elem);
    ses_stat_elem.dev_off = TRUE;
    terminator_set_drive_eses_status(virtual_phy_ptr,slot_number, ses_stat_elem);
    
    
    /* force logout the device.  This should cause the device to abort all pending requests. */
    terminator_logout_device(device_handle);
    
    /*we need to wake up the appropriate thread so it can go over the list
     *and found the ones that are marked logout pending*/
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    /* start the powercycle delay (TBD) of this device */
    terminator_get_device_reset_delay(device_handle, &delay_in_ms);
    fbe_terminator_sleep(delay_in_ms);

    /* Update the status page for device off = 0 */
    terminator_get_drive_eses_status(virtual_phy_ptr,slot_number, &ses_stat_elem);
    ses_stat_elem.dev_off = FALSE;
    terminator_set_drive_eses_status(virtual_phy_ptr,slot_number, ses_stat_elem);

    /* force login the device */
    terminator_login_device(device_handle);
    /*we need to wake up the appropriate thread so it can go over the list
     *and found the ones that are marked login pending*/
    fbe_semaphore_release(&update_semaphore[port_index], 0, 1, FALSE);

    /* Resetting common function */
    terminator_register_device_reset_function(device_handle,
                                                       terminator_miniport_device_reset_common_action);

    /*Power cycle completed for drive */
    return status;
}
fbe_status_t fbe_terminator_miniport_api_get_cpd_device_id(fbe_u32_t port_index,
                               fbe_u32_t device_table_index,
                               fbe_miniport_device_id_t *cpd_device_id)
{
    return terminator_miniport_sas_device_table_get_device_id(&terminator_pmc_port_array[port_index].device_table,
                                   device_table_index,
                                   cpd_device_id);
}

fbe_status_t fbe_terminator_miniport_api_device_table_remove (fbe_u32_t port_index, fbe_u32_t device_index)
{
    /* remove the device from the device table */
    return terminator_miniport_sas_device_table_remove_record(&terminator_pmc_port_array[port_index].device_table, device_index);
}

fbe_status_t fbe_terminator_miniport_api_device_table_add (fbe_u32_t port_index, fbe_terminator_device_ptr_t device_handle, fbe_u32_t *device_index)
{
    /* add the device to the device table */
    return terminator_miniport_sas_device_table_add_record(&terminator_pmc_port_array[port_index].device_table, device_handle, device_index);
}

fbe_status_t fbe_terminator_miniport_api_device_table_reserve (fbe_u32_t port_index, fbe_miniport_device_id_t cpd_device_id)
{
    fbe_u32_t device_index = (fbe_u32_t) cpd_device_id & INDEX_BIT_MASK;
    return terminator_miniport_sas_device_table_reserve_record(&terminator_pmc_port_array[port_index].device_table, device_index);
}

fbe_status_t fbe_terminator_miniport_api_device_table_get_device_handle (fbe_u32_t port_index,
                                   fbe_miniport_device_id_t cpd_device_id,
                                   fbe_terminator_device_ptr_t *device_handle)
{
    fbe_u32_t device_index = (fbe_u32_t) cpd_device_id & INDEX_BIT_MASK;
    return terminator_miniport_sas_device_table_get_device_handle(&terminator_pmc_port_array[port_index].device_table, device_index, device_handle);
}

fbe_status_t fbe_terminator_miniport_api_clean_sempahores(fbe_u32_t port_index)
{
    fbe_semaphore_destroy(&update_semaphore[port_index]);
    fbe_semaphore_destroy(&update_complete_semaphore[port_index]);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_event_notify(fbe_terminator_device_ptr_t port_ptr, fbe_cpd_shim_sfp_condition_type_t event_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpd_shim_callback_info_t    callback_info;
    fbe_u32_t port_index;

    status = terminator_get_miniport_port_index(port_ptr, &port_index);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: terminator_get_miniport_port_index failed\n", __FUNCTION__);
        return status;
    }

    if(terminator_pmc_port_array[port_index].sfp_event_callback_function != NULL)
    {
        callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_SFP;
        callback_info.info.sfp_info.condition_type = event_type;
        terminator_pmc_port_array[port_index].sfp_event_callback_function(&callback_info, terminator_pmc_port_array[port_index].sfp_event_callback_context);
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_miniport_api_link_event_notify(fbe_terminator_device_ptr_t port_ptr, cpd_shim_port_link_state_t  link_state)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpd_shim_callback_info_t    callback_info;
    fbe_u32_t port_index;    

    status = terminator_get_miniport_port_index(port_ptr, &port_index);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: terminator_get_miniport_port_index failed\n", __FUNCTION__);
        return status;
    }

    if(terminator_pmc_port_array[port_index].callback_function != NULL)
    {
        switch(link_state){
            case CPD_SHIM_PORT_LINK_STATE_UP:
                callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP;
                break;
            case CPD_SHIM_PORT_LINK_STATE_DOWN:
                callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN;
                break;
            case CPD_SHIM_PORT_LINK_STATE_DEGRADED:
            case CPD_SHIM_PORT_LINK_STATE_NOT_INITIALIZED:
                callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS;
                break;
            default:
                callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS;
                break;

        }
        
        terminator_pmc_port_array[port_index].callback_function(&callback_info, terminator_pmc_port_array[port_index].callback_context);
    }
    return FBE_STATUS_OK;
}

static fbe_bool_t fbe_terminator_miniport_validate_encryption_info(fbe_payload_ex_t * payload)
{
    fbe_payload_cdb_operation_t * payload_cdb_operation = NULL;
    fbe_u8_t * cdb_opcode;
    fbe_key_handle_t key_handle;
    fbe_status_t    status;

    payload_cdb_operation = fbe_payload_ex_get_cdb_operation(payload);

    /* make sure we have the right operation */
    if (payload_cdb_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Error ! Operation must be payload_cdb_operation \n", __FUNCTION__);
        return FBE_FALSE;
    }

    status = fbe_payload_cdb_operation_get_cdb(payload_cdb_operation, &cdb_opcode);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s:Error !! fbe_payload_cdb_operation_get_cdb failed\n", __FUNCTION__);
        fbe_payload_cdb_set_request_status(payload_cdb_operation, FBE_PORT_REQUEST_STATUS_INVALID_REQUEST);
        return FBE_FALSE;
    }

    fbe_payload_ex_get_key_handle(payload, &key_handle);

    if(((*cdb_opcode == FBE_SCSI_WRITE_SAME) ||
        (*cdb_opcode == FBE_SCSI_WRITE_SAME_16)) &&
       (key_handle != FBE_INVALID_KEY_HANDLE) && 
       (payload_cdb_operation->cdb[1] != 0x08)) // if write same is an unmap it is ok
    {
        /* We dont support write same for encrypted IO */
        terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s:Write sames not supported for this IO \n", __FUNCTION__);
        return FBE_FALSE;

    }

    return FBE_TRUE;
}
