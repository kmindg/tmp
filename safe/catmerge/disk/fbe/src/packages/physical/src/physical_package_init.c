#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_notification.h"
#include "fbe_scheduler.h"
#include "fbe_port_discovery.h"
#include "fbe_service_manager.h"
#include "fbe_base_board.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_physical_package.h"
#include "physical_package_class_table.h"
#include "physical_package_service_table.h"
#include "fbe_base_discovered.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"
#include "fbe/fbe_perfstats_interface.h"


static fbe_status_t physical_package_destroy_all_objects(void);
static fbe_status_t physical_package_destroy_object(fbe_object_id_t object_id);

static fbe_status_t physical_package_destroy_board_object(void);
static fbe_status_t physical_package_get_total_objects(fbe_u32_t *total);

static fbe_object_id_t base_board_object_id = 0;

static void
physical_package_log_error(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
physical_package_log_error(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_PHYSICAL,
                     FBE_TRACE_LEVEL_ERROR,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}

static void
physical_package_log_info(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

static void
physical_package_log_info(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_PHYSICAL,
                     FBE_TRACE_LEVEL_INFO,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}

fbe_status_t 
physical_package_init(void)
{
    fbe_status_t status;
    physical_package_log_info("%s starting...\n", __FUNCTION__);

    status = physical_package_init_services();
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_init_services failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Create board object */
    status = physical_package_create_board_object();
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_create_board_object failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
physical_package_destroy(void)
{
    fbe_status_t status;

    physical_package_log_info("%s: starting...\n", __FUNCTION__);


    /* F I X  M E !!!!!!!

    In a case where the physical package is hung because memory is full,
    (for example, it wants to allocate a packet to send to an object to fulfil a request for Flare).
    If we try to run "net stop physical_package" it will not work.
    The reason is that the line below will try to allocate a packet to remove the edges, but memroy
    service is full, so it's a catch 22.........
    */

    /* Tell the board object to go into its DESTROY state. */
    (void)physical_package_destroy_board_object();

    /* Wait 100 msec. to drain all outstanding packets */
    fbe_thread_delay(100);

    /* Destroy all objects */
    physical_package_destroy_all_objects();

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_PERFSTATS);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_perfstats_service_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_ENVSTATS);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_envstats_service_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRACE);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_trace_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TRAFFIC_TRACE);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_trace_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_DRIVE_CONFIGURATION);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_destroy_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_notification_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_TOPOLOGY);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_destroy_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SCHEDULER);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_scheduler_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_EVENT_LOG);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: Event Log destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_destroy_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_memory_destroy failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_destroy_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_destroy_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    fbe_transport_destroy();
    return FBE_STATUS_OK;
}

static fbe_status_t 
physical_package_sem_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;

    fbe_semaphore_release(sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

static fbe_status_t 
physical_package_enumerate_objects(fbe_object_id_t *obj_list, fbe_u32_t total, fbe_u32_t *actual)
{
    fbe_packet_t * packet = NULL;
    fbe_sg_element_t *	sg_list = NULL;
	fbe_sg_element_t *	temp_sg_list = NULL;
	fbe_payload_ex_t	*	payload = NULL;
	fbe_topology_mgmt_enumerate_objects_t 	p_topology_mgmt_enumerate_objects;/*small enough to be on the stack*/


	sg_list = (fbe_sg_element_t *)fbe_allocate_contiguous_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/
	if (sg_list == NULL) {
		physical_package_log_error("%s: fbe_allocate_contiguous_memory failed to allocate sg list\n", __FUNCTION__);
		*actual = 0;
		return FBE_STATUS_GENERIC_FAILURE;
	}

	temp_sg_list = sg_list;
	temp_sg_list->address = (fbe_u8_t *)obj_list;
	temp_sg_list->count = total * sizeof(fbe_object_id_t);
	temp_sg_list ++;
	temp_sg_list->address = NULL;
	temp_sg_list->count = 0;

	p_topology_mgmt_enumerate_objects.number_of_objects = total;

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        physical_package_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
		fbe_release_contiguous_memory(sg_list);
		*actual = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS,
                                &p_topology_mgmt_enumerate_objects,
                                0, /* no input data */
                                sizeof(fbe_topology_mgmt_enumerate_objects_t),
                                NULL,
                                NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);

	payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_set_sg_list(payload, sg_list, 2);

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);

	*actual = p_topology_mgmt_enumerate_objects.number_of_objects_copied;

    fbe_transport_release_packet(packet);

	fbe_release_contiguous_memory(sg_list);

    return FBE_STATUS_OK;
}

static fbe_status_t 
physical_package_destroy_all_objects(void)
{
    fbe_object_id_t *						object_id_list = NULL;
    fbe_u32_t 								tenths_of_sec = 0;
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;
	fbe_u32_t								total_objects = 0;
	fbe_u32_t								actual_objects = 0;

	status = physical_package_get_total_objects(&total_objects);
	if (status != FBE_STATUS_OK) {
        physical_package_log_error("%s: can't get total objects!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (total_objects == 0) { goto skip_stuff; } /* SAFEBUG - prevent allocation of 0 bytes */
    object_id_list = (fbe_object_id_t *)fbe_allocate_contiguous_memory((sizeof(fbe_object_id_t) * total_objects));
    if (object_id_list == NULL) {
        physical_package_log_error("%s: can't allocate memory for object list!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = physical_package_enumerate_objects(object_id_list, total_objects, &actual_objects);
    if (status != FBE_STATUS_OK) {
        physical_package_log_error("%s: can't enumerate objects!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    tenths_of_sec = 0;

    while ((actual_objects != 0) &&
           (tenths_of_sec < 100)) { /* Wait ten seconds (at most) */
        /* Wait 100 msec. */
        fbe_thread_delay(100);
        tenths_of_sec++;
        status = physical_package_enumerate_objects(object_id_list ,total_objects, &actual_objects);
        if (status != FBE_STATUS_OK) {
            physical_package_log_error("%s: can't enumerate objects!\n", __FUNCTION__);
			fbe_release_contiguous_memory(object_id_list);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* print out object_id's */ 
    physical_package_log_info("%s: Found %X objects\n", __FUNCTION__, actual_objects);
    for(i = 0; i < actual_objects; i++) {
        physical_package_log_info("%s: destroy object_id %X \n", __FUNCTION__, object_id_list[i]);
        physical_package_destroy_object(object_id_list[i]);
    }

	fbe_release_contiguous_memory(object_id_list);
    skip_stuff:;
    return FBE_STATUS_OK;
}

static fbe_status_t 
physical_package_destroy_object(fbe_object_id_t object_id)
{
    fbe_packet_t * packet = NULL;
    fbe_topology_mgmt_destroy_object_t topology_mgmt_destroy_object;

    topology_mgmt_destroy_object.object_id = object_id;
    
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        physical_package_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_DESTROY_OBJECT,
                                &topology_mgmt_destroy_object,
                                sizeof(fbe_topology_mgmt_destroy_object_t),
                                0, /* no output data */
                                NULL,
                                NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);
    fbe_transport_wait_completion(packet);

    fbe_transport_release_packet(packet);
    
    return FBE_STATUS_OK;
}

fbe_status_t 
physical_package_create_board_object(void)
{    
    fbe_packet_t * packet;
    fbe_base_object_mgmt_create_object_t base_object_mgmt_create_object;
    fbe_discovery_transport_control_create_edge_t discovery_transport_control_create_edge;
    fbe_semaphore_t sem;
    fbe_status_t status;

    fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        physical_package_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    fbe_base_object_mgmt_create_object_init(&base_object_mgmt_create_object,
                                            FBE_CLASS_ID_BASE_BOARD, 
                                            FBE_OBJECT_ID_INVALID);

    fbe_transport_build_control_packet(packet, 
                                FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT,
                                &base_object_mgmt_create_object,
                                sizeof(fbe_base_object_mgmt_create_object_t),
                                sizeof(fbe_base_object_mgmt_create_object_t),
                                physical_package_sem_completion,
                                &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);

    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);

    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK) {
        physical_package_log_error("%s: can't create base board object, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    base_board_object_id = base_object_mgmt_create_object.object_id;

    /* Reuse packet */
    fbe_transport_reuse_packet(packet);
    
    discovery_transport_control_create_edge.server_id = FBE_OBJECT_ID_INVALID;
    discovery_transport_control_create_edge.server_index = 0;

    fbe_transport_build_control_packet(packet, 
                                FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE,
                                &discovery_transport_control_create_edge,
                                sizeof(fbe_discovery_transport_control_create_edge_t),
                                sizeof(fbe_discovery_transport_control_create_edge_t),
                                physical_package_sem_completion,
                                &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                base_object_mgmt_create_object.object_id);

    /* fbe_transport_send_packet(base_object_mgmt_create_object->object_id, packet); */
    fbe_service_manager_send_control_packet(packet);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
physical_package_destroy_board_object(void)
{
    fbe_packet_t * packet;
    
   packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        physical_package_log_error("%s: fbe_transport_allocate_packet fail\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_initialize_packet(packet);
    fbe_transport_build_control_packet(packet, 
                                       FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                       NULL,
                                       0,
                                       0,
                                       NULL,
                                       NULL);
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              base_board_object_id);

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);
	fbe_transport_wait_completion(packet);

    fbe_transport_release_packet(packet);
    return FBE_STATUS_OK;
}

fbe_service_control_entry_t physical_package_simulator_get_control_entry(void)
{
    return &fbe_service_manager_send_control_packet;
}

fbe_service_io_entry_t physical_package_simulator_get_io_entry(void)
{
    return &fbe_topology_send_io_packet;
}

fbe_status_t 
physical_package_init_services(void)
{
    fbe_status_t status;
    fbe_u32_t num_of_chunks;

    physical_package_log_info("%s starting...\n", __FUNCTION__);

	fbe_transport_init();

    /* Initialize service manager */
	status = fbe_service_manager_init_service_table(physical_package_service_table);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_service_manager_init_service_table failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SERVICE_MANAGER);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_init_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize the memory */
	num_of_chunks = FBE_PHYSICAL_PACKAGE_NUMBER_OF_CHUNKS;
#if defined(SIMMODE_ENV)
	num_of_chunks = FBE_MIN(FBE_PHYSICAL_PACKAGE_NUMBER_OF_CHUNKS, 2*FBE_MAX_PHYSICAL_OBJECTS);
#endif
    status =  fbe_memory_init_number_of_chunks(num_of_chunks);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_memory_init_number_of_chunks failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }
    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_MEMORY);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_memory_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status =  fbe_base_service_send_init_command(FBE_SERVICE_ID_EVENT_LOG);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: Event Log Serivce Init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize topology */
    status = fbe_topology_init_class_table(physical_package_class_table);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_topology_init_class_table failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TOPOLOGY);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: physical_package_init_service failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize notification service */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_notification_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /* Initialize scheduler */
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_SCHEDULER);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_scheduler_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    /*initialize the drive configuration service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_DRIVE_CONFIGURATION);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_drive_configuration_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

	/*initialize the drive trace service*/
	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRACE);
	if(status != FBE_STATUS_OK) {
		physical_package_log_error("%s: trace service init failed, status: %X\n",
				__FUNCTION__, status);
		return status;
	}

	/*initialize the traffic trace service*/
	status = fbe_base_service_send_init_command(FBE_SERVICE_ID_TRAFFIC_TRACE);
	if(status != FBE_STATUS_OK) {
		physical_package_log_error("%s: traffic trace service init failed, status: %X\n",
				__FUNCTION__, status);
		return status;
	}

    /*Initialize FBE perfstats service*/
    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_PERFSTATS);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_perfstats_service_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    status = fbe_base_service_send_init_command(FBE_SERVICE_ID_ENVSTATS);
    if(status != FBE_STATUS_OK) {
        physical_package_log_error("%s: fbe_envstats_service_init failed, status: %X\n",
                __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
physical_package_get_control_entry(fbe_service_control_entry_t * service_control_entry)
{
	*service_control_entry = fbe_service_manager_send_control_packet;
	return FBE_STATUS_OK;
}

fbe_status_t
physical_package_get_io_entry(fbe_io_entry_function_t * io_entry)
{
	* io_entry = fbe_topology_send_io_packet;
	return FBE_STATUS_OK;
}

static fbe_status_t physical_package_get_total_objects(fbe_u32_t *total)
{
	fbe_packet_t * 								packet = NULL;
    fbe_topology_control_get_total_objects_t	get_total;

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL) {
        physical_package_log_error("%s: fbe_transport_allocate_packet failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);

    fbe_transport_build_control_packet(packet, 
									   FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
									   &get_total,
									   sizeof(fbe_topology_control_get_total_objects_t),
									   sizeof(fbe_topology_control_get_total_objects_t),
									   NULL,
									   NULL);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_TOPOLOGY,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID);


	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_service_manager_send_control_packet(packet);

	fbe_transport_wait_completion(packet);

	*total = get_total.total_objects;

    fbe_transport_release_packet(packet);

    return FBE_STATUS_OK;

}
