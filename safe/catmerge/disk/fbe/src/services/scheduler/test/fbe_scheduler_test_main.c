
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_class.h"
#include "fbe_notification.h"
#include "fbe_scheduler.h"
#include "mut.h"
#include "fbe_base_object.h"
#include "fbe/fbe_queue.h"
#include "../../disk/fbe/src/services/topology/interface/base_object_private.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_emcutil_shell_include.h"

extern fbe_service_methods_t fbe_topology_service_methods;
extern fbe_service_methods_t fbe_notification_service_methods;
extern fbe_service_methods_t fbe_service_manager_service_methods;


#define SCHEDTEST_MIN_CLASS_ID FBE_CLASS_ID_VERTEX
#define SCHEDTEST_MAX_CLASS_ID (FBE_CLASS_ID_LAST - 1)

#define SCHEDTEST_MIN_OBJECT_ID 0
#define SCHEDTEST_MAX_OBJECT_ID (FBE_MAX_OBJECTS - 1)

static fbe_queue_head_t		schedTest_object_list = {&schedTest_object_list, &schedTest_object_list}; // empty

static fbe_queue_head_t		schedTest_workloads   = {&schedTest_workloads, &schedTest_workloads};  // empty

typedef struct schedTest_run_policy_s {
    fbe_queue_element_t     link;
    fbe_class_id_t          class_id;
    fbe_object_id_t         object_id;
    fbe_s32_t               count;
    void (*func)(struct fbe_scheduler_test_s *objectPtr, fbe_packet_t *packet, struct schedTest_run_policy_s *policy);
} schedTest_run_policy_t;

typedef struct schedTest_run_stats_s {
	fbe_u32_t               scheduleRequests;
	fbe_u32_t               runRequests;
} schedTest_run_stats_t;

typedef struct fbe_scheduler_test_s {
	fbe_base_object_t       base_object;
	// for linking to the schedTest_object_list list
	fbe_queue_element_t     object_list_element;
    // add test specific fixtures
    schedTest_run_stats_t   stats;
    schedTest_run_policy_t  policy;
	fbe_semaphore_t         executionCompletionSemaphore;
	fbe_class_id_t          class_id;
	fbe_object_id_t         object_id;
	fbe_object_id_t         real_topology_id;
	fbe_object_handle_t     handle;
} fbe_scheduler_test_t;

typedef struct schedTest_workload_stats_s {
    fbe_u32_t               created;
    fbe_u32_t               completed;
} schedTest_workload_statistics_t;


static schedTest_workload_statistics_t schedTest_workload_statistics = { 0, 0 };

/*
 * execution policies
 */

static void schedTest_execute_completion(fbe_scheduler_test_t *objectPtr,
                                         fbe_packet_t *packet,
                                         struct schedTest_run_policy_s *policy) {
    policy->count--;

    if(policy->count == 0) {
        /*
         * When count becomes zero, this objects scheduling is complete
         */
        mut_printf(MUT_LOG_HIGH,
                    "schedTest_execute_completion: object 0x%X completed execution\n", objectPtr);
        mut_printf(MUT_LOG_HIGH,
                    "schedTest_execute_completion: fbe_semaphore_release(0x%X, 1, FALSE)", 
                    &objectPtr->executionCompletionSemaphore);
        fbe_semaphore_release(&objectPtr->executionCompletionSemaphore, 0, 1, FALSE);

        schedTest_workload_statistics.completed++;
    }
    else if(policy->count > 0 ) {
        mut_printf(MUT_LOG_HIGH,
                    "schedTest_execute_completion: object 0x%X continuing execution\n", objectPtr);
        objectPtr->base_object.scheduler_element.scheduler_state |= FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST;
    }
    else {
        mut_printf(MUT_LOG_HIGH,
                    "schedTest_execute_completion: object 0x%X policy count negative  ERROR ERROR\n", objectPtr);

    }
}

static fbe_status_t 
schedTest_mgmt_release_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context; 

    mut_printf(MUT_LOG_HIGH, "schedTest_mgmt_release_completion: fbe_semaphore_release(0x%X, 1, FALSE)", sem);
	fbe_semaphore_release(sem, 0, 1, FALSE);

	return FBE_STATUS_OK;
}

/*
 * this method is not declared in any h file
 * it is needed so that the test system can access the allocated object
 */
fbe_object_handle_t fbe_topology_get_mgmt_handle(fbe_object_id_t id);

static fbe_scheduler_test_t *ask_topology_for_object(fbe_class_id_t class_id, fbe_object_id_t object_id) {
        fbe_status_t status;
	fbe_semaphore_t sem;
	fbe_packet_t * packet;
	fbe_base_object_mgmt_create_object_t request;
	fbe_object_handle_t handle;
	fbe_scheduler_test_t *objectPtr;

	fbe_semaphore_init(&sem, 0, 1);

	packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(packet);
	fbe_base_object_mgmt_create_object_init(&request, class_id, object_id);

	fbe_transport_build_control_packet(packet, 
								FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT,
								&request,
								sizeof(request),
								sizeof(request),
								schedTest_mgmt_release_completion,
								&sem);

		/* Set packet address */
	fbe_transport_set_address(	packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID);

        status = fbe_service_manager_send_mgmt_packet(packet);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	 
    mut_printf(MUT_LOG_HIGH, "ask_topology_for_object: fbe_semaphore_wait(0x%X, NUL)", sem);
	fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

	fbe_transport_release_packet(packet);

	/*
	 * Topology returned the index used to reference this object in the fbe_base_object_mgmt_create_object_t request structure
	 * This 
	 */
	handle = fbe_topology_get_mgmt_handle(request.object_id);
	objectPtr = (fbe_scheduler_test_t *)fbe_base_handle_to_pointer(handle);

	/*
	 * record the original ID for latter reference
	 */
	objectPtr->object_id = object_id;
	return objectPtr;
}

static fbe_status_t 
fbe_scheduler_test_notification_function(fbe_object_id_t object_id, 
                                         fbe_object_state_t object_state, 
                                         fbe_notification_context_t context)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if(object_state == FBE_OBJECT_STATE_DESTROY) {
        fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
        mut_printf(MUT_LOG_HIGH, "fbe_scheduler_test_notification_function: fbe_semaphore_release(0x%X, 1, FALSE)", sem);
        fbe_semaphore_release(sem, 0, 1, FALSE);
    }
    
    return FBE_STATUS_OK;
}

static void schedTest_update_execution_policy(fbe_scheduler_test_t *objectPtr) {
    if(fbe_queue_is_empty(&schedTest_workloads)) {
        /*
         * no configured workloads.
         * Program the default workload
         * This provides support for tests which were developed before workloads were implemented
         */
        mut_printf(MUT_LOG_HIGH,
                    "schedTest_update_execution_policy: initializing default workload schedTest_execute_completion for 0x%X",
                    objectPtr);

        objectPtr->policy.count = 1;
        objectPtr->policy.func  = schedTest_execute_completion;
    }
    else {
        schedTest_run_policy_t *policy  = (schedTest_run_policy_t *)fbe_queue_front(&schedTest_workloads);
        fbe_queue_remove(&policy->link);

        objectPtr->policy.count = policy->count;
        objectPtr->policy.func  = policy->func;

        mut_printf(MUT_LOG_HIGH,
                    "schedTest_update_execution_policy: initializing workload for 0x%X  count %d, func 0x%X",
                    objectPtr,
                    objectPtr->policy.count,
                    objectPtr->policy.func);
        free(policy);
    }
}

static fbe_status_t class_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle) {
	fbe_base_object_mgmt_create_object_t * request = NULL;
	fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

	status = fbe_base_object_create_object(packet, object_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	if(status == FBE_STATUS_OK) {
		fbe_scheduler_test_t *objectPtr = (fbe_scheduler_test_t *)fbe_base_handle_to_pointer(*object_handle);

        payload = fbe_transport_get_payload_ex(packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);    	     
        fbe_payload_control_get_buffer(control_operation, &request); 
		/*
		 *  objects are required to have the class_id set properly
		 */
		fbe_base_object_set_class_id((fbe_base_object_t *) objectPtr, request->class_id);

		/*
		 * Initialize Object type info  in the fbe_scheduler_test_t payload for later use
		 */
		 objectPtr->class_id		= request->class_id;
		 objectPtr->real_topology_id	= request->object_id;
		
		/* 
		 * Initialize test fixture
		 */ 
		 objectPtr->stats.scheduleRequests	= 0;
		 objectPtr->stats.runRequests     	= 0;
		 objectPtr->handle          	= *object_handle;

         /*
          * Obtain and initialize the execution policy
          */
         schedTest_update_execution_policy(objectPtr);


         /*
          * initialize executionCompletionSemaphore
          */
         mut_printf(MUT_LOG_HIGH, "class_create_object: fbe_semaphore_init(0x%X, 0, 1)", &objectPtr->executionCompletionSemaphore);
	     fbe_semaphore_init(&objectPtr->executionCompletionSemaphore, 0, 1);

		 /*
		  * finally, add the object to the object queue, for cleanup
		  */
		 fbe_queue_push(&schedTest_object_list, &objectPtr->object_list_element);
	}

	return status;
}

static fbe_status_t class_destroy_object(fbe_object_handle_t object_handle) {
	fbe_status_t status;

	fbe_scheduler_test_t *objectPtr = (fbe_scheduler_test_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_queue_remove(&objectPtr->object_list_element);

	status = fbe_base_object_destroy_object(object_handle);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


	return FBE_STATUS_OK;
}



static fbe_status_t mock_topology_control_entry(struct fbe_packet_s * packet) {

	return fbe_topology_service_methods.control_entry(packet);
}

static const fbe_service_methods_t **  schedTest_get_services() {
	fbe_service_methods_t *services = (fbe_service_methods_t *)malloc(sizeof(fbe_service_methods_t));

	static fbe_service_methods_t **servicesPtr = NULL;

	if(servicesPtr == NULL ) {
		services = (fbe_service_methods_t *)malloc(2*sizeof(fbe_service_methods_t));

		services[0].service_id = FBE_SERVICE_ID_TOPOLOGY;
		services[0].control_entry = &mock_topology_control_entry;

		services[1]= fbe_notification_service_methods;

		servicesPtr = (fbe_service_methods_t **)malloc(sizeof(fbe_service_methods_t *) * 3); 
		servicesPtr[0] =  &services[0];
		servicesPtr[1] =  &services[1];
		servicesPtr[2] = NULL; 
	}

	return servicesPtr;
}

static fbe_status_t dummy(void) {
	return FBE_STATUS_OK;
}

static fbe_status_t schedTest_run_request(fbe_scheduler_test_t *objectPtr,  fbe_packet_t *packet) {
    mut_printf(MUT_LOG_HIGH, "schedTest_monitor(0x%X, 0x%X)", objectPtr, packet);
	objectPtr->stats.runRequests++;

	/*
	 * notify semaphore to wake up test control process
	 */
    (*objectPtr->policy.func)(objectPtr, packet, &objectPtr->policy);

    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t schedTest_monitor(fbe_scheduler_test_t *objectPtr, 
                                           fbe_packet_t *packet) {

    fbe_object_state_t mgmt_state;
    fbe_status_t status;

    fbe_base_object_get_mgmt_state((fbe_base_object_t *) objectPtr, &mgmt_state);
    mut_printf(MUT_LOG_HIGH, "schedTest_monitor(0x%X, 0x%X) mgmt_state: 0x%X", objectPtr, packet, mgmt_state);
    switch(mgmt_state) {
        case FBE_OBJECT_STATE_INSTANTIATED:
        // test objects are in state instantiated
        case FBE_OBJECT_STATE_READY:
            status = schedTest_run_request(objectPtr, packet);
            break;

        case FBE_OBJECT_STATE_DESTROY_PENDING:
            fbe_base_object_set_mgmt_state((fbe_base_object_t *) objectPtr, FBE_OBJECT_STATE_DESTROY);
            fbe_base_object_run_request((fbe_base_object_t *) objectPtr);

           	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
            break;

        case FBE_OBJECT_STATE_DESTROY:
            /* Tell scheduler that object is no longer exist */
            fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
            fbe_transport_complete_packet(packet);
            
            fbe_base_object_destroy_request((fbe_base_object_t *) objectPtr);
            status = FBE_STATUS_NO_OBJECT;
            break;
        default:
            status = fbe_base_object_monitor((fbe_base_object_t *)objectPtr, packet);
            break;
    }

    return status;
}

static fbe_status_t class_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet) {
	// this has to handle the processing the schedule token
    fbe_payload_control_operation_opcode_t control_code;
	fbe_scheduler_test_t *objectPtr;
    fbe_status_t status;

    control_code = fbe_transport_get_control_code(packet);
	objectPtr = (fbe_scheduler_test_t *)fbe_base_handle_to_pointer(object_handle);

    mut_printf(MUT_LOG_HIGH, "class_control_entry(0x%X, 0x%X) control_code: 0x%X, objectPtr 0x%X ", object_handle, 
                                                                                                  packet, 
                                                                                                  control_code,
                                                                                                  objectPtr);
    switch(control_code) {
        case FBE_BASE_OBJECT_CONTROL_CODE_MONITOR:
            status = schedTest_monitor(objectPtr, packet);
            break;
        default:
            status = fbe_base_object_control_entry(object_handle, packet);
    }

	return status;
}


static fbe_status_t class_event_entry(fbe_object_handle_t object_handle, fbe_event_t event) {
	return FBE_STATUS_OK;
}


static fbe_class_methods_t *create_Mock_Class_Table(fbe_class_id_t class_id) {
	fbe_class_methods_t *class_table = (fbe_class_methods_t *)malloc(sizeof(fbe_class_methods_t));

	class_table->class_id		= class_id;
	class_table->load		    = &dummy;
	class_table->unload		    = &dummy;
	class_table->create_object	= &class_create_object;
	class_table->destroy_object	= &class_destroy_object;
	class_table->control_entry		= &class_control_entry;
	class_table->event_entry	= &class_event_entry;

	return class_table;
}

static const fbe_class_methods_t ** schedTest_get_classes() {
	fbe_u32_t number_of_classes = SCHEDTEST_MAX_CLASS_ID - SCHEDTEST_MIN_CLASS_ID + 1; // inclusive
	fbe_u32_t i, class_id;
	// add 1 to hold the terminating NULL
	static fbe_class_methods_t **classesPtrList = NULL;

	if(classesPtrList == NULL) {
		classesPtrList = (fbe_class_methods_t **)malloc(sizeof(fbe_class_methods_t **)*number_of_classes + 1); 
		
		class_id = SCHEDTEST_MIN_CLASS_ID ; 
		for(i = 0, class_id = SCHEDTEST_MIN_CLASS_ID; i < (number_of_classes); i++, class_id++) {
			classesPtrList[i] = create_Mock_Class_Table(class_id);
		}
		
		classesPtrList[number_of_classes] = NULL; 
	}

	return classesPtrList;
}


static fbe_u32_t schedTest_setup(void) {
	fbe_status_t status;

    /*
     * init schedule counters
     */
    schedTest_workload_statistics.created   = 0;
    schedTest_workload_statistics.completed = 0;
    

	fbe_queue_init(&schedTest_object_list);

    status = fbe_service_manager_init_service_table(schedTest_get_services(), FBE_PACKAGE_ID_PHYSICAL);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_memory_init_number_of_chunks(1024);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status =  fbe_transport_init();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_topology_init_class_table(schedTest_get_classes());
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TOPOLOGY);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_NOTIFICATION);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SCHEDULER);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	return FBE_STATUS_OK;
}

static fbe_u32_t schedTest_tearDown(void) {
	fbe_status_t status;

	/*
	 * Per fbe_physical_package_init.c (physical_package_destroy)
	 * This is the order of operations to shutdown the package
	 */ 

	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_NOTIFICATION);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TOPOLOGY);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SCHEDULER);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status =  fbe_transport_destroy();
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	return FBE_STATUS_OK;
}


fbe_u32_t schedTest_create_object(void) {
	fbe_scheduler_test_t *object = ask_topology_for_object(1, 0);
	MUT_ASSERT_TRUE(object != NULL);
	if (object != NULL) {
		return FBE_STATUS_OK;
	}
	else {
		return FBE_STATUS_GENERIC_FAILURE;
	}
}

static void schedTest_scheduleObject(fbe_scheduler_test_t *objectPtr) {
	objectPtr->stats.scheduleRequests++;

	fbe_scheduler_run_request(&objectPtr->base_object.scheduler_element);
}

static void schedTest_wait_for_scheduleObject(fbe_scheduler_test_t *objectPtr) {
	/*
	 * wait for sheduler to run this object
	 */
     mut_printf(MUT_LOG_HIGH, "schedTest_wait_for_scheduleObject: fbe_semaphore_wait(0x%X, NULL)", &objectPtr->executionCompletionSemaphore);
	 fbe_semaphore_wait(&objectPtr->executionCompletionSemaphore, NULL); 

	 // cleanup
	 fbe_semaphore_destroy(&objectPtr->executionCompletionSemaphore);
}

static fbe_scheduler_test_t *schedTest_allocate_Object(fbe_class_id_t class_id,
                                     				fbe_object_id_t object_id) {
	fbe_scheduler_test_t *object = ask_topology_for_object(class_id, object_id);
	MUT_ASSERT_TRUE(object != NULL);

	return object;
}

static fbe_status_t schedTest_destroy_Object(fbe_scheduler_test_t *object) {
	fbe_packet_t * 			packet = NULL;	
	fbe_semaphore_t 				sem_register_completion;
	fbe_semaphore_t 				sem_destroy_completion;
	fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;
    fbe_notification_element_t      notification_element;

    fbe_semaphore_init(&sem_register_completion, 0, 1);
    fbe_semaphore_init(&sem_destroy_completion, 0, 1);

	notification_element.notification_function = fbe_scheduler_test_notification_function;
	notification_element.notification_context = &sem_destroy_completion;

    packet = fbe_transport_allocate_packet();

    fbe_transport_initialize_packet(packet);
    fbe_transport_build_control_packet(packet, 
								FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
								&notification_element,
								sizeof(fbe_notification_element_t),
								sizeof(fbe_notification_element_t),
								schedTest_mgmt_release_completion,
								&sem_register_completion);

	/* Set packet address */
	fbe_transport_set_address(	packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_NOTIFICATION,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID);
    status = fbe_service_manager_send_mgmt_packet(packet);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    // wait for register notification to complete
    mut_printf(MUT_LOG_HIGH, "set_object_state_destroy_pending &sem_register_completion: fbe_semaphore_wait(0x%X, NULL)", &sem_register_completion);
    fbe_semaphore_wait(&sem_register_completion, NULL);

    status = fbe_transport_get_status_code (packet);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Now that fbe_scheduler_test_notification_function is registered as a
     * callback for state changes to object, start the destroy process
     */
    fbe_base_object_set_mgmt_state((fbe_base_object_t *) object, FBE_OBJECT_STATE_DESTROY_PENDING);


    // wait for destroy notification to complete
    mut_printf(MUT_LOG_HIGH, "set_object_state_destroy_pending &sem_destroy_completion: fbe_semaphore_wait(0x%X, NULL)", &sem_destroy_completion);
    fbe_semaphore_wait(&sem_destroy_completion, NULL);
    fbe_semaphore_destroy(&sem_destroy_completion);

    /*
     * Now unregister the notification callback
     *
     * IF this is NOT done, then fbe_notification_send encounters an access violation
     */

    fbe_transport_initialize_packet(packet);
    fbe_transport_build_control_packet(packet, 
								FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
								&notification_element,
								sizeof(fbe_notification_element_t),
								sizeof(fbe_notification_element_t),
								schedTest_mgmt_release_completion,
								&sem_register_completion);

	/* Set packet address */
	fbe_transport_set_address(	packet,
								FBE_PACKAGE_ID_PHYSICAL,
								FBE_SERVICE_ID_NOTIFICATION,
								FBE_CLASS_ID_INVALID,
								FBE_OBJECT_ID_INVALID);
    status = fbe_service_manager_send_mgmt_packet(packet);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    // wait for register notification to complete
    mut_printf(MUT_LOG_HIGH, "set_object_state_destroy_pending &sem_register_completion: fbe_semaphore_wait(0x%X, NUL)", &sem_register_completion);
    fbe_semaphore_wait(&sem_register_completion, NULL);

    status = fbe_transport_get_status_code (packet);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_semaphore_destroy(&sem_register_completion);

    // release packet
	fbe_transport_release_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_scheduler_test_t *schedTest_Create_Schedule_Run_object(fbe_class_id_t class_id, fbe_object_id_t object_id) {

	fbe_scheduler_test_t *objectPtr = schedTest_allocate_Object(class_id, object_id);
	MUT_ASSERT_TRUE(objectPtr != NULL);

	schedTest_scheduleObject(objectPtr);

    return objectPtr;
}

static void schedTest_Create_Schedule_Run_object_Wait_Destroy(fbe_class_id_t class_id,
                                     				fbe_object_id_t object_id) {
	fbe_status_t status;

	fbe_scheduler_test_t *object = schedTest_Create_Schedule_Run_object(class_id, object_id);
	MUT_ASSERT_TRUE(object != NULL);

	schedTest_wait_for_scheduleObject(object);

	/*
	 * these counts are incremented during the scheduling & execution process
	 */
	MUT_ASSERT_TRUE(object->stats.runRequests == object->stats.scheduleRequests);

	/*
	 * now destroy the object
	 */
	status = schedTest_destroy_Object(object);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
}

static void schedTest_add_workload(fbe_class_id_t class_id, fbe_object_id_t object_id, fbe_s32_t count,
                                    void (*func)(struct fbe_scheduler_test_s *objectPtr, 
                                                 fbe_packet_t *packet, 
                                                 struct schedTest_run_policy_s *policy)) {
    schedTest_run_policy_t *workload;

    workload = (schedTest_run_policy_t *) malloc(sizeof(schedTest_run_policy_t));

    fbe_queue_element_init(&workload->link);

    workload->class_id  = class_id;
    workload->object_id = object_id;
    workload->count     = count;
    workload->func      = func;

    fbe_queue_push(&schedTest_workloads, &workload->link);
}






fbe_u32_t schedTest_setup_tearDown_testCase(void) {
	(void)schedTest_setup();
	(void)schedTest_tearDown();

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_allocateMinObject_TestCase(void) {
	fbe_scheduler_test_t *object;

	object = schedTest_allocate_Object(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);
	MUT_ASSERT_TRUE(object != NULL);

    schedTest_destroy_Object(object);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_allocateMaxObject_TestCase(void) {
	fbe_scheduler_test_t *object;

	object = schedTest_allocate_Object(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID);

	MUT_ASSERT_TRUE(object != NULL);

    schedTest_destroy_Object(object);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_allocateMaxClass_TestCase(void) {
	fbe_scheduler_test_t *object;

	object = schedTest_allocate_Object(SCHEDTEST_MAX_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID);
	MUT_ASSERT_TRUE(object != NULL);

    schedTest_destroy_Object(object);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_allocateTwoObjects_TestCase(void) {
	fbe_scheduler_test_t *object1;
	fbe_scheduler_test_t *object2;

	object1 = schedTest_allocate_Object(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);
	object2 = schedTest_allocate_Object(SCHEDTEST_MAX_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID);

	MUT_ASSERT_TRUE(object1 != NULL);
	MUT_ASSERT_TRUE(object2 != NULL);

    schedTest_destroy_Object(object1);
    schedTest_destroy_Object(object2);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_scheduleOneObject_testCase(void) {
    schedTest_add_workload(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID, 1, schedTest_execute_completion);

	schedTest_Create_Schedule_Run_object_Wait_Destroy(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_scheduleTwoObjects_testCase(void) {
    schedTest_add_workload(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID, 1, schedTest_execute_completion);
    schedTest_add_workload(SCHEDTEST_MAX_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID, 1, schedTest_execute_completion);


	schedTest_Create_Schedule_Run_object_Wait_Destroy(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);
	schedTest_Create_Schedule_Run_object_Wait_Destroy(SCHEDTEST_MAX_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_OneObject_DefaultPoll_TestCase(void) {

	fbe_scheduler_test_t *object;

	object = schedTest_allocate_Object(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);
	MUT_ASSERT_TRUE(object != NULL);

    /*
     * wait for default (3 second) countdown to trigger scheduling of this object
     */
	schedTest_wait_for_scheduleObject(object);

    schedTest_destroy_Object(object);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_scheduleOneReacurringObject_testCase(void) {
    schedTest_add_workload(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID, 2, schedTest_execute_completion);

	schedTest_Create_Schedule_Run_object_Wait_Destroy(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);

	return FBE_STATUS_OK;
}

fbe_u32_t schedTest_scheduleTwoReacurringObject_testCase(void) {
	fbe_status_t status;
	fbe_scheduler_test_t *object1;
	fbe_scheduler_test_t *object2;

    schedTest_add_workload(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID, 3, schedTest_execute_completion);
    schedTest_add_workload(SCHEDTEST_MAX_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID, 3, schedTest_execute_completion);

	object1 = schedTest_Create_Schedule_Run_object(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID);
	object2 = schedTest_Create_Schedule_Run_object(SCHEDTEST_MAX_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID);

	MUT_ASSERT_TRUE(object1 != NULL);
	MUT_ASSERT_TRUE(object2 != NULL);

    /*
     * wait for execution completion
     */
	schedTest_wait_for_scheduleObject(object1);
	schedTest_wait_for_scheduleObject(object2);

	/*
	 * these counts are incremented during the scheduling & execution process
	 */
	MUT_ASSERT_TRUE(object1->stats.runRequests == object1->stats.scheduleRequests);
	MUT_ASSERT_TRUE(object2->stats.runRequests == object2->stats.scheduleRequests);

	/*
	 * now destroy the object
	 */
	status = schedTest_destroy_Object(object1);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	status = schedTest_destroy_Object(object2);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

	return FBE_STATUS_OK;
}


static fbe_u32_t schedTest_setup_workload1(void) {
    fbe_queue_init(&schedTest_workloads);

    schedTest_add_workload(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MIN_OBJECT_ID, 1, schedTest_execute_completion);
    schedTest_add_workload(SCHEDTEST_MIN_CLASS_ID, SCHEDTEST_MAX_OBJECT_ID, 1, schedTest_execute_completion);

    schedTest_setup();

	return FBE_STATUS_OK;
}

static void schedTest_delay() {
    
    fbe_thread_delay(20);
}

fbe_u32_t schedTest_tearDown_workload(void) {
    /*
     * need to verify that all workloads have complete, before performing standard cleanup
     */
    while(schedTest_workload_statistics.created != schedTest_workload_statistics.completed) {
        schedTest_delay();
    }

    schedTest_tearDown();

	return FBE_STATUS_OK;
}

static fbe_u32_t schedTest_workload(void) {
    /*
     * The workload was configured by schedTest_setup_workloadX()
     * During object creation, the object removes a workload from the schedTest_workloads list
     * and initializes it's workload.
     */

    while(!fbe_queue_is_empty(&schedTest_workloads)) {
        schedTest_run_policy_t *policy  = (schedTest_run_policy_t *)fbe_queue_front(&schedTest_workloads);
        fbe_scheduler_test_t *object = schedTest_Create_Schedule_Run_object(policy->class_id, policy->object_id);

        /*
         * wait for object to consume the workload
         */
        while(policy == (schedTest_run_policy_t *)fbe_queue_front(&schedTest_workloads)) {
			schedTest_delay();
        }
        schedTest_workload_statistics.created++;
    }

    /*
     * schedTest_tearDownWorkload() monitors workload threads for completion before 
     * performing the standard tearDown.
     */
     return FBE_STATUS_OK;
}

/*
__cdecl main (int argc , char ** argv)
{
	fbe_testsuite_t *suite = TEST_CREATE_SUITE("fbe_scheduler");

	TEST_ADD_TEST(suite, schedTest_setup_tearDown_testCase,		FBE_STATUS_OK, NULL, NULL);
	TEST_ADD_TEST(suite, schedTest_allocateMinObject_TestCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_allocateMaxObject_TestCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_allocateMaxClass_TestCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_allocateTwoObjects_TestCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_scheduleOneObject_testCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_scheduleTwoObjects_testCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_OneObject_DefaultPoll_TestCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_scheduleOneReacurringObject_testCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_scheduleTwoReacurringObject_testCase,	FBE_STATUS_OK, &schedTest_setup, &schedTest_tearDown);
	TEST_ADD_TEST(suite, schedTest_workload,	                FBE_STATUS_OK, &schedTest_setup_workload1, &schedTest_tearDown_workload);
	
	TEST_EXECUTE_TESTSUITE(suite, argc, argv);
	
	exit(0);
	return 0;
}
*/

__cdecl main (int argc , char ** argv)
{
    mut_testsuite_t *suite;

#include "fbe/fbe_emcutil_shell_maincode.h"

    mut_init(argc, argv);

    suite = MUT_CREATE_TESTSUITE("schedulerSuite");

	MUT_ADD_TEST(suite, schedTest_setup_tearDown_testCase,		 NULL, NULL);
	MUT_ADD_TEST(suite, schedTest_allocateMinObject_TestCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_allocateMaxObject_TestCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_allocateMaxClass_TestCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_allocateTwoObjects_TestCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_scheduleOneObject_testCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_scheduleTwoObjects_testCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_OneObject_DefaultPoll_TestCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_scheduleOneReacurringObject_testCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_scheduleTwoReacurringObject_testCase,
                                   &schedTest_setup, &schedTest_tearDown);
	MUT_ADD_TEST(suite, schedTest_workload,
                                   &schedTest_setup_workload1, &schedTest_tearDown_workload);
	
	MUT_RUN_TESTSUITE(suite)
	
	exit(0);
	return 0;
}
