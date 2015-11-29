#include "fbe/fbe_memory.h"
#include "fbe_base_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe_scheduler.h"
#include "fbe_topology.h" 
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_notification.h"
#include "fbe_transport_memory.h"
#include "base_object_private.h"
#include "fbe/fbe_trace_interface.h"

/* Class methods forward declaration */
fbe_status_t base_object_load(void);
fbe_status_t base_object_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_base_object_class_methods = {FBE_CLASS_ID_BASE_OBJECT,
                                                    base_object_load,
                                                    base_object_unload,
                                                    fbe_base_object_create_object,
                                                    fbe_base_object_destroy_object,
                                                    fbe_base_object_control_entry,
                                                    fbe_base_object_event_entry,
                                                    NULL,
                                                    NULL};

/* Forward declaration */
static void fbe_base_object_allocate_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context);
static fbe_status_t fbe_base_object_destroy_request_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_base_object_t * fbe_base_object_allocate_object(void);

fbe_status_t 
base_object_load(void)
{
    fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_base_object_t) < FBE_MEMORY_CHUNK_SIZE);

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s object size %llu\n", __FUNCTION__, (unsigned long long)sizeof(fbe_base_object_t));

    status = fbe_base_object_monitor_load_verify();
    return status;
}

fbe_status_t 
base_object_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_base_object_t * base_object = NULL;
    fbe_base_object_mgmt_create_object_t * base_object_mgmt_create_object = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_package_id_t package_id;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    /*
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    */

    fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_create_object); 

    /* Allocate memory */
    base_object = (fbe_base_object_t *)fbe_base_object_allocate_object();
    if(base_object == NULL){
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_base_object_allocate fail\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory (base_object, FBE_MEMORY_CHUNK_SIZE);
    fbe_base_object_set_class_id(base_object, FBE_CLASS_ID_BASE_OBJECT);
    /* fbe_base_set_component_type((fbe_base_t *) base_object, FBE_COMPONENT_TYPE_OBJECT); */
    fbe_base_set_magic_number((fbe_base_t *) base_object, FBE_MAGIC_NUMBER_OBJECT);

    /* Set object_id */
    base_object->object_id = base_object_mgmt_create_object->object_id;

    fbe_get_package_id(&package_id);
    /* Sep manually clears the object id in the monitor, so that SEP objects can decide when it is OK to 
     * start getting events and usurpers. 
     * All other packages clear it automatically. 
     */
    if (package_id != FBE_PACKAGE_ID_SEP_0)
    {
        fbe_topology_clear_gate(base_object->object_id);
    }
    /* Initialize trace_level */
    base_object->trace_level = fbe_trace_get_default_trace_level();

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "%X object created\n", base_object->object_id);

    /* Initialize terminator queue */
    /*
    fbe_queue_init(&base_object->initiator_queue_head);
    fbe_spinlock_init(&base_object->initiator_queue_lock);
    */
    fbe_queue_init(&base_object->terminator_queue_head);
    fbe_spinlock_init(&base_object->terminator_queue_lock);
        //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &base_object->terminator_queue_lock.lock);
        //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &base_object->terminator_queue_lock.lock, CSX_NULL);

    /* Initialize usurper queue */
    fbe_queue_init(&base_object->usurper_queue_head);
    fbe_spinlock_init(&base_object->usurper_queue_lock);
        //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &base_object->usurper_queue_lock.lock);
        //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &base_object->usurper_queue_lock.lock, CSX_NULL);

    /* Initialize object lock */
    fbe_spinlock_init(&base_object->object_lock);
        //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &base_object->object_lock.lock);
        //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &base_object->object_lock.lock, CSX_NULL);

    base_object->mgmt_attributes = 0;
    
    base_object->userper_counter = 0;

        fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_base_object_lifecycle_const, base_object);

    /* Start the Monitor. */
    fbe_scheduler_register(&base_object->scheduler_element);

    /* This functionality belong to the physical object */
    base_object->physical_object_level = FBE_PHYSICAL_OBJECT_LEVEL_INVALID; /* objects will overwrite it */

    /* Return handle */
    *object_handle = fbe_base_pointer_to_handle((fbe_base_t *) base_object);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_base_object_t * base_object;
    fbe_notification_info_t notification_info;
    fbe_object_id_t object_id;
    fbe_class_id_t class_id;
    fbe_status_t status = FBE_STATUS_FAILED;
    fbe_u32_t fbe_sched_unregister_attempts = 0;
    
    base_object = (fbe_base_object_t *)fbe_base_handle_to_pointer(object_handle);

    /* Check parent edges */

    /* Cleanup */

    /* Destroy initiator and terminator queues */
    /*
    if(!fbe_queue_is_empty(&base_object->initiator_queue_head)) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s initiator queue not empty\n", __FUNCTION__);
    }
    fbe_queue_destroy(&base_object->initiator_queue_head);
    fbe_spinlock_destroy(&base_object->initiator_queue_lock);
    */
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT, 
                          "%X object destroyed\n", base_object->object_id);


    if(!fbe_queue_is_empty(&base_object->terminator_queue_head)) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s terminator queue not empty\n", __FUNCTION__);
    }
    
    do {
        status = fbe_scheduler_unregister(&base_object->scheduler_element);
        if (fbe_sched_unregister_attempts != 0) {
            fbe_thread_delay(100);
        }

        fbe_sched_unregister_attempts++;

    } while (status != FBE_STATUS_OK && fbe_sched_unregister_attempts < 600);

    if (fbe_sched_unregister_attempts == 600) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to unregister from monitor\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_lifecycle_destroy_object((fbe_lifecycle_inst_t *)&fbe_base_object_lifecycle_const, base_object);
        
    fbe_queue_destroy(&base_object->terminator_queue_head);
    fbe_spinlock_destroy(&base_object->terminator_queue_lock);

    fbe_queue_destroy(&base_object->usurper_queue_head);
    fbe_spinlock_destroy(&base_object->usurper_queue_lock);

    fbe_spinlock_destroy(&base_object->object_lock);

    /* Collect information to fill out the notification_info structure  */
    class_id = fbe_base_object_get_class_id(object_handle);
    fbe_base_object_get_object_id(base_object, &object_id);

    notification_info.notification_type     = FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED;
    notification_info.class_id              = class_id;
    fbe_topology_class_id_to_object_type(class_id, &notification_info.object_type);

    /* Send notification */
    status = fbe_notification_send(object_id, notification_info);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to send notification, status was 0x%X\n",
                              __FUNCTION__, status);
    }

    /* Release memory */
    fbe_base_object_release_object((fbe_base_object_t *) base_object);

    return FBE_STATUS_OK;
}


static void
fbe_base_object_allocate_completion(fbe_memory_request_t * memory_request, fbe_memory_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
}

static fbe_base_object_t *
fbe_base_object_allocate_object(void)
{
    fbe_memory_request_t memory_request;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);

    memory_request.request_state = FBE_MEMORY_REQUEST_STATE_ALLOCATE_OBJECT;
    memory_request.number_of_objects = 1;
    memory_request.ptr = NULL;
    memory_request.completion_function = fbe_base_object_allocate_completion;
    memory_request.completion_context = &sem;
    fbe_memory_allocate(&memory_request);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    return (fbe_base_object_t *)memory_request.ptr;
}

void 
fbe_base_object_release_object(fbe_base_object_t * base_object)
{
    fbe_memory_release(base_object);
}

void 
fbe_base_object_mgmt_create_object_init(fbe_base_object_mgmt_create_object_t * base_object_mgmt_create_object,
                                        fbe_class_id_t	class_id, 
                                        fbe_object_id_t object_id)
{
    base_object_mgmt_create_object->class_id = class_id;
    base_object_mgmt_create_object->object_id = object_id;
    base_object_mgmt_create_object->mgmt_attributes = 0;
}

/* accessor function*/
fbe_status_t  
fbe_base_object_mgmt_get_class_id(fbe_base_object_mgmt_create_object_t * base_object_mgmt_create_object,
                                  fbe_class_id_t	* class_id)
{
    * class_id = base_object_mgmt_create_object->class_id;
    return FBE_STATUS_OK;
}


fbe_status_t  
fbe_base_object_get_object_id(fbe_base_object_t * object, fbe_object_id_t * object_id)
{
    *object_id = object->object_id;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_set_class_id(fbe_base_object_t * base_object, fbe_class_id_t class_id)
{
    if(base_object == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_object->class_id = class_id;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_change_class_id(fbe_base_object_t * base_object, fbe_class_id_t class_id)
{
    fbe_status_t status;

    if(base_object == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_object->class_id = class_id;


    status = fbe_lifecycle_set_cond((fbe_lifecycle_const_t *)&fbe_base_object_lifecycle_const,
                                    (fbe_base_object_t*)base_object,
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't set self_init condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    return status;
}

fbe_object_id_t 
fbe_base_object_scheduler_element_to_object_id(fbe_scheduler_element_t * scheduler_element)
{
    fbe_base_object_t  * base_object;
    fbe_object_id_t object_id;

    base_object = (fbe_base_object_t *)((fbe_u8_t *)scheduler_element - (fbe_u8_t *)(&((fbe_base_object_t *)0)->scheduler_element));
    fbe_base_object_get_object_id(base_object, &object_id);
    return object_id;
}

fbe_base_object_t *
fbe_base_object_scheduler_element_to_base_object(fbe_scheduler_element_t * scheduler_element)
{
    fbe_base_object_t *base_object;
    base_object = (fbe_base_object_t *)((fbe_u8_t *)scheduler_element - (fbe_u8_t *)(&((fbe_base_object_t *)0)->scheduler_element));
    return base_object;
}

fbe_status_t 
fbe_base_object_get_physical_object_level(fbe_base_object_t * base_object, fbe_physical_object_level_t * physical_object_level)
{
    *physical_object_level = base_object->physical_object_level;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_set_physical_object_level(fbe_base_object_t * base_object, fbe_physical_object_level_t physical_object_level)
{
    base_object->physical_object_level = physical_object_level;
    return FBE_STATUS_OK;
}

#if 0 /* This functionality belong to the discovered object */
/* Send FBE_BASE_OBJECT_CONTROL_CODE_GET_LEVEL packet to child and set own level as child_level + 1 */
/* Object MUST to be connected */
fbe_status_t 
fbe_base_object_init_physical_object_level(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_packet_t * new_packet = NULL;
    fbe_status_t status;
    fbe_semaphore_t sem;
    fbe_base_object_mgmt_get_level_t base_object_mgmt_get_level;

    fbe_semaphore_init(&sem, 0, 1);

    new_packet = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(new_packet);

    fbe_transport_build_control_packet(new_packet, 
                                FBE_BASE_OBJECT_CONTROL_CODE_GET_LEVEL,
                                &base_object_mgmt_get_level,
                                sizeof(fbe_base_object_mgmt_get_level_t),
                                sizeof(fbe_base_object_mgmt_get_level_t),
                                base_object_init_physical_object_level_completion,
                                &sem);

    status = fbe_base_object_send_packet(base_object, new_packet, 0); /* for now hard coded control edge */

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    if(status == FBE_STATUS_OK){
        base_object->physical_object_level = base_object_mgmt_get_level.physical_object_level + 1;
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                                 FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s physical_object_level = %X\n", __FUNCTION__, base_object->physical_object_level);
        /* The physical_object_level is changed, ask scheduler to pay attention to us. */
        fbe_base_object_run_request(base_object);
    }

    fbe_transport_copy_packet_status(new_packet, packet);
    fbe_transport_release_packet(new_packet);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t 
base_object_init_physical_object_level_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t 
base_object_get_level(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_object_mgmt_get_level_t * base_object_mgmt_get_level = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_get_level); 
    /* Check object state MUST be "connected" */

    base_object_mgmt_get_level->physical_object_level = base_object->physical_object_level;
    status = FBE_STATUS_OK;
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
#endif /* This functionality belong to the discovered object */


void
fbe_base_object_lock(fbe_base_object_t * base_object)
{
    fbe_spinlock_lock(&base_object->object_lock);
}

void
fbe_base_object_unlock(fbe_base_object_t * base_object)
{
    fbe_spinlock_unlock(&base_object->object_lock);
}

fbe_status_t
fbe_base_object_run_request(fbe_base_object_t * base_object)
{ 
    fbe_status_t status;

    status = fbe_scheduler_run_request(&base_object->scheduler_element);
    return status;
}

fbe_status_t
fbe_base_object_reschedule_request(fbe_base_object_t * base_object, fbe_u32_t msecs)
{
    return fbe_scheduler_set_time_counter(&base_object->scheduler_element, msecs);
}

fbe_status_t
fbe_base_object_drain_terminator_queue(fbe_base_object_t * base_object)
{
    fbe_queue_element_t * queue_element = NULL;
    fbe_packet_t * packet = NULL;

    fbe_spinlock_lock(&base_object->terminator_queue_lock);
    queue_element = fbe_queue_front(&base_object->terminator_queue_head);
    while(queue_element != NULL) {
        packet = fbe_transport_queue_element_to_packet(queue_element);

        /* fbe_transport_cancel_subpackets(packet); */
    }
    fbe_spinlock_unlock(&base_object->terminator_queue_lock);

    /* next_element = (fbe_scheduler_element_t *)fbe_queue_next(&scheduler_idle_queue_head, &next_element->queue_element); */

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_object_destroy_request(fbe_base_object_t * base_object)
{
    fbe_packet_t * packet;
    fbe_topology_mgmt_destroy_object_t  topology_mgmt_destroy_object;
    fbe_status_t status;
    fbe_package_id_t package_id;
    fbe_semaphore_t sem;

    /* This is temporary approach */
    fbe_base_object_trace(	(fbe_base_object_t*)base_object,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_packet fail\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);

    topology_mgmt_destroy_object.object_id = base_object->object_id;

    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT,
                                &topology_mgmt_destroy_object,
                                sizeof(fbe_topology_mgmt_destroy_object_t),
                                sizeof(fbe_topology_mgmt_destroy_object_t),
                                fbe_base_object_destroy_request_completion,
                                &sem);
    /* Set packet address */
    fbe_get_package_id(&package_id);
    fbe_transport_set_address(	packet,
                                package_id,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);

    status = fbe_service_manager_send_control_packet(packet);


    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_base_object_destroy_request_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

    /*
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    */

    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_object_add_to_terminator_queue(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_spinlock_lock(&base_object->terminator_queue_lock);
    status = fbe_transport_enqueue_packet(packet, &base_object->terminator_queue_head);
    fbe_spinlock_unlock(&base_object->terminator_queue_lock);

    return status;
}

fbe_status_t
fbe_base_object_remove_from_terminator_queue(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_spinlock_lock(&base_object->terminator_queue_lock);
    status = fbe_transport_remove_packet_from_queue(packet);
    fbe_spinlock_unlock(&base_object->terminator_queue_lock);

    return status;
}

fbe_status_t
fbe_base_object_add_to_usurper_queue(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_spinlock_lock(&base_object->usurper_queue_lock);
    status = fbe_transport_enqueue_packet(packet, &base_object->usurper_queue_head);
    fbe_spinlock_unlock(&base_object->usurper_queue_lock);

    return status;
}

fbe_status_t
fbe_base_object_remove_from_usurper_queue(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_spinlock_lock(&base_object->usurper_queue_lock);
    status = fbe_transport_remove_packet_from_queue(packet);
    fbe_spinlock_unlock(&base_object->usurper_queue_lock);

    return status;
}

fbe_status_t fbe_base_object_find_from_usurper_queue(fbe_base_object_t* base_object, fbe_packet_t** reinit_packet_p)
                                                     
{
    fbe_queue_element_t*  queue_element = NULL;
    fbe_packet_t *       packet_p = NULL;
    fbe_packet_attr_t          packet_attr;

    *reinit_packet_p = NULL;
    fbe_spinlock_lock(&base_object->usurper_queue_lock);
    queue_element = &base_object->usurper_queue_head;
    while(!fbe_queue_is_empty(&base_object->usurper_queue_head)) 
    {
        queue_element = fbe_queue_next(&base_object->usurper_queue_head, queue_element);
        packet_p = fbe_transport_queue_element_to_packet(queue_element);
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if(packet_attr & FBE_PACKET_FLAG_REINIT_PVD) 
        {
            *reinit_packet_p = packet_p;
            break;
        }
    }
    fbe_spinlock_unlock(&base_object->usurper_queue_lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_object_find_control_op_from_usurper_queue(fbe_base_object_t* base_object, fbe_packet_t** out_packet_p, 
                                                                fbe_payload_control_operation_opcode_t control_op)
{
    fbe_queue_element_t*                    queue_element = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    *out_packet_p = NULL;
    fbe_spinlock_lock(&base_object->usurper_queue_lock);
    queue_element = fbe_queue_front(&base_object->usurper_queue_head);
    while(queue_element != NULL) 
    {
        packet_p = fbe_transport_queue_element_to_packet(queue_element);
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_p = fbe_payload_ex_get_control_operation(payload_p);
        if (control_p) 
        {
            fbe_payload_control_get_opcode(control_p, &opcode);
            if (opcode == control_op) 
            {
                *out_packet_p = packet_p;
                break;
            }
        }
        queue_element = fbe_queue_next(&base_object->usurper_queue_head, queue_element);
    }
    fbe_spinlock_unlock(&base_object->usurper_queue_lock);

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_base_object_usurper_queue_drain_io(fbe_lifecycle_const_t * p_class_const, fbe_base_object_t * base_object)
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;
    fbe_status_t completion_status;
    fbe_packet_t * new_packet = NULL;
    fbe_bool_t done = FALSE;

    status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
    if(status != FBE_STATUS_OK){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X\n", __FUNCTION__, status);
        return status;
    }

    completion_status = FBE_STATUS_BUSY;
    switch(lifecycle_state){
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            break;
        /* Do not drain usurper queue in PENDING_ACTIVATE and PENDING_HIBERNATE states */
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            return FBE_STATUS_OK;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            completion_status = FBE_STATUS_FAILED;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            completion_status = FBE_STATUS_DEAD;
            break;
    }

    while(!done){
        new_packet = NULL;
        fbe_base_object_usurper_queue_lock(base_object);

        if(!fbe_queue_is_empty(&base_object->usurper_queue_head)){
            new_packet = fbe_transport_dequeue_packet(&base_object->usurper_queue_head);
        } else {
            done = TRUE;
        }
        fbe_base_object_usurper_queue_unlock(base_object);

        if(new_packet != NULL){
            fbe_transport_set_status(new_packet, completion_status, 0);
            fbe_transport_complete_packet(new_packet);
        }
    }/* while(!done) */

    fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s: ObjID=0x%x, life_cycle=0x%x, completion status: 0x%x.\n", 
                             __FUNCTION__, base_object->object_id, lifecycle_state, completion_status);

    return status;
}

fbe_status_t 
fbe_base_object_packet_cancel_function(fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_object_t * base_object = (fbe_base_object_t *)context;

    if(context != NULL)
    {
        status = fbe_lifecycle_set_cond((fbe_lifecycle_const_t *)&fbe_base_object_lifecycle_const,
                                        (fbe_base_object_t*)base_object,
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED);

        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set packet_canceled condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_OBJECT,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't set packet_canceled condition, Context is NULL\n", __FUNCTION__);
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_object_get_mgmt_attributes(fbe_base_object_t * base_object, fbe_object_mgmt_attributes_t * mgmt_attributes)
{

    *mgmt_attributes = base_object->mgmt_attributes;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_object_set_mgmt_attributes(fbe_base_object_t * base_object, fbe_object_mgmt_attributes_t mgmt_attributes)
{
    fbe_trace_level_t default_trace_level;

    if (((mgmt_attributes & FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE) != 0) &&
        ((base_object->mgmt_attributes & FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE) == 0)) {
            /* Turn on lifecycle tracing. */
            (void)fbe_lifecycle_set_trace_flags((fbe_lifecycle_const_t*)&FBE_LIFECYCLE_CONST_DATA(base_object),
                                                base_object, FBE_LIFECYCLE_TRACE_FLAGS_ALL);
            /* We may also need to adjust the trace level. */
            default_trace_level = fbe_trace_get_default_trace_level();
            if (default_trace_level < FBE_TRACE_LEVEL_DEBUG_LOW) {
                /* fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_DEBUG_LOW); */
            }
    }
    else if (((mgmt_attributes & FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE) == 0) &&
             ((base_object->mgmt_attributes & FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE) != 0)) {
            /* Turn off lifecycle tracing. */
            (void)fbe_lifecycle_set_trace_flags((fbe_lifecycle_const_t*)&FBE_LIFECYCLE_CONST_DATA(base_object),
                                                base_object, FBE_LIFECYCLE_TRACE_FLAGS_NO_TRACE);
    }

    base_object->mgmt_attributes = mgmt_attributes;

    return FBE_STATUS_OK;
}

fbe_bool_t
fbe_base_object_is_terminator_queue_empty(fbe_base_object_t * base_object)
{
    fbe_spinlock_lock(&base_object->terminator_queue_lock);
    if(fbe_queue_is_empty(&base_object->terminator_queue_head)){
        fbe_spinlock_unlock(&base_object->terminator_queue_lock);
        return TRUE;
    } else {
        fbe_spinlock_unlock(&base_object->terminator_queue_lock);
        return FALSE;
    }
}

fbe_status_t
fbe_base_object_terminator_queue_lock(fbe_base_object_t * base_object)
{
    fbe_spinlock_lock(&base_object->terminator_queue_lock);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_object_terminator_queue_unlock(fbe_base_object_t * base_object)
{
    fbe_spinlock_unlock(&base_object->terminator_queue_lock);
    return FBE_STATUS_OK;
}

fbe_queue_head_t *
fbe_base_object_get_terminator_queue_head(fbe_base_object_t * base_object)
{
    return &base_object->terminator_queue_head;
}

fbe_status_t
fbe_base_object_usurper_queue_lock(fbe_base_object_t * base_object)
{
    fbe_spinlock_lock(&base_object->usurper_queue_lock);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_object_usurper_queue_unlock(fbe_base_object_t * base_object)
{
    fbe_spinlock_unlock(&base_object->usurper_queue_lock);
    return FBE_STATUS_OK;
}

fbe_queue_head_t *
fbe_base_object_get_usurper_queue_head(fbe_base_object_t * base_object)
{
    return &base_object->usurper_queue_head;
}

fbe_status_t 
fbe_base_object_scheduler_element_is_object_valid(fbe_scheduler_element_t * scheduler_element)
{
    fbe_base_object_t  * base_object;
    fbe_u64_t  magic_number;

    base_object = (fbe_base_object_t  *)((fbe_u8_t *)scheduler_element - (fbe_u8_t *)(&((fbe_base_object_t  *)0)->scheduler_element));
    
    fbe_base_get_magic_number((fbe_base_t *) base_object, &magic_number);

    if(magic_number == FBE_MAGIC_NUMBER_OBJECT){
        return FBE_STATUS_OK;
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_status_t
fbe_base_object_set_trace_level(fbe_base_object_t * base_object,
                                fbe_trace_level_t trace_level)
{
    base_object->trace_level = trace_level;
    return FBE_STATUS_OK;
}

fbe_trace_level_t
fbe_base_object_get_trace_level(fbe_base_object_t * base_object)
{
    return base_object->trace_level;
}

void
fbe_base_object_trace(fbe_base_object_t * base_object, 
                      fbe_trace_level_t trace_level,
                      fbe_u32_t message_id,
                      const fbe_char_t * fmt, ...)
{
    va_list argList;

    if(trace_level > base_object->trace_level){
        return;
    }

    if(trace_level <= base_object->trace_level) {
        va_start(argList, fmt);
        fbe_trace_report(FBE_COMPONENT_TYPE_OBJECT,
                         base_object->object_id,
                         trace_level,
                         message_id,
                         fmt, 
                         argList);
        va_end(argList);
    }
}

void
fbe_base_object_trace_at_startup(fbe_base_object_t * base_object, 
                                 fbe_trace_level_t trace_level,
                                 fbe_u32_t message_id,
                                 const fbe_char_t * fmt, ...)
{
    va_list argList;
    if(trace_level <= base_object->trace_level) {
        va_start(argList, fmt);
        fbe_trace_at_startup(FBE_COMPONENT_TYPE_OBJECT,
                             base_object->object_id,
                             trace_level,
                             message_id,
                             fmt, 
                             argList);
        va_end(argList);
    }
}

void
fbe_base_object_customizable_trace(fbe_base_object_t * base_object, 
                      fbe_trace_level_t trace_level,
                      fbe_u8_t *header,
                      const fbe_char_t * fmt, ...)
{
    va_list argList;
    if(trace_level <= base_object->trace_level) {
        va_start(argList, fmt);
        fbe_trace_report_w_header(FBE_COMPONENT_TYPE_OBJECT,
                         base_object->object_id,
                         trace_level,
                         header,
                         fmt, 
                         argList);
        va_end(argList);
    }
}

fbe_status_t
fbe_base_object_increment_userper_counter(fbe_object_handle_t handle)
{
    fbe_base_object_t * base_object;

    base_object = (fbe_base_object_t *)fbe_base_handle_to_pointer(handle);
    fbe_atomic_increment(&base_object->userper_counter);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_object_decrement_userper_counter(fbe_object_handle_t handle)
{
    fbe_base_object_t * base_object;

    base_object = (fbe_base_object_t *)fbe_base_handle_to_pointer(handle);
    fbe_atomic_decrement(&base_object->userper_counter);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_object_check_hook()
 ******************************************************************************
 * @brief
 *   This function checks the hook status and returns the appropriate lifecycle status.
 *
 *
 * @param base_object               - pointer to the base_object
 * @param monitor_state             - the monitor state
 * @param monitor_substate          - the substate of the monitor
 * @param val2                      - a generic value that can be used (e.g. checkpoint)
 * @param status                    - pointer to the status
 *
 * @return fbe_status_t   - FBE_STATUS_OK
 * @author
 *   11/01/2011 - Copied from fbe_raid_group_check_hook. nchiu
 *
 ******************************************************************************/
fbe_status_t 
fbe_base_object_check_hook(fbe_base_object_t * base_object,
                           fbe_u32_t monitor_state,
                           fbe_u32_t monitor_substate,
                           fbe_u64_t val2,
                           fbe_scheduler_hook_status_t *status)
{
    fbe_scheduler_debug_hook_function_t hook;
    hook = base_object->scheduler_element.hook;
    *status = FBE_SCHED_STATUS_OK;

    if (hook != NULL)
    {
        // call the debug hook function
        *status = hook(base_object->object_id, monitor_state, monitor_substate, 0, val2);

        switch (*status)
        {
        // if the OK status is returned, just break
        case FBE_SCHED_STATUS_OK:
        // if the done status is returned, we are going to pause this operation
        case FBE_SCHED_STATUS_DONE:
            break;
        // if there was an error, we need to log it.
        case FBE_SCHED_STATUS_ERROR:
            fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "obj_check_hook: Debug Hook action failed...\n");
        }

        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "obj_chk_hook state:0x%X, substate 0x%X,val2 0x%llx,stat 0x%X\n",
                              monitor_state, monitor_substate, (unsigned long long)val2, *status);
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_base_object_enable_lifecycle_debug_trace()
 ****************************************************************
 * @brief
 *  This function is used to set the class level lifecycle debug
 *  tracing for all objects of the class.
 *
 * @param fbe_u32_t - Debug flag to set.  
 * 
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  02/16/2012 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_base_object_enable_lifecycle_debug_trace(fbe_base_object_t * object_p)
{
    fbe_class_id_t class_id;
    fbe_u32_t flag;
    fbe_status_t status;

    class_id = object_p->class_id;

    /* Check if lifecycle tracing is enabled */
    status = fbe_lifecycle_get_debug_trace_flag(&flag);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }
    switch(class_id)
    {
        case FBE_CLASS_ID_LUN:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_LUN_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_LUN_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_BVD_INTERFACE:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_BVD_INTERFACE_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_BVD_INTERFACE_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_MIRROR:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_MIRROR_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_MIRROR_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_STRIPER:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_MIRROR_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_MIRROR_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_PARITY:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_PARITY_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_PARITY_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_VIRTUAL_DRIVE_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_VIRTUAL_DRIVE_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_PROVISION_DRIVE_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_PROVISION_DRIVE_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
            break;
        case FBE_CLASS_ID_EXTENT_POOL:
#if 0
            if((flag & FBE_LIFECYCLE_DEBUG_FLAG_EXTENT_POOL_CLASS_TRACING)== FBE_LIFECYCLE_DEBUG_FLAG_EXTENT_POOL_CLASS_TRACING)
            {
                object_p->lifecycle.trace_flags = FBE_LIFECYCLE_TRACE_FLAGS_ALL;
            }
#endif
            break;
        case FBE_CLASS_ID_EXTENT_POOL_LUN:
            break;
        case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
            break;
        default :
            fbe_base_object_trace(object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Invalid class ID:0x%x\n",class_id);

        
    }
    return FBE_STATUS_OK;
}
