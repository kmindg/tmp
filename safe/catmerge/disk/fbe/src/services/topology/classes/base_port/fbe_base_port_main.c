#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_port.h"
#include "fbe_scheduler.h"
#include "fbe_diplex.h"
#include "fbe/fbe_board.h"
#include "fbe_transport_memory.h"
#include "base_port_private.h"
#include "fbe/fbe_drive_configuration_interface.h"

/* Class methods forward declaration */
fbe_status_t base_port_load(void);
fbe_status_t base_port_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_base_port_class_methods = {FBE_CLASS_ID_BASE_PORT,
													base_port_load,
													base_port_unload,
													fbe_base_port_create_object,
													fbe_base_port_destroy_object,
													fbe_base_port_control_entry,
													fbe_base_port_event_entry,
													NULL,
													fbe_base_port_monitor_entry};

fbe_status_t 
base_port_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PORT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	return fbe_base_port_monitor_load_verify();
}

fbe_status_t 
base_port_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PORT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
	fbe_base_port_t * base_port;
	fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PORT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

	/* Call parent constructor */
	status = fbe_base_discovering_create_object(packet, object_handle);
	if(status != FBE_STATUS_OK){
		return status;
	}

	/* Set class id */
	base_port = (fbe_base_port_t *)fbe_base_handle_to_pointer(*object_handle);
	fbe_base_object_set_class_id((fbe_base_object_t *) base_port, FBE_CLASS_ID_BASE_PORT);	

	/* Set physical_object_level */
	fbe_base_object_set_physical_object_level((fbe_base_object_t *)base_port, FBE_PHYSICAL_OBJECT_LEVEL_PORT);

	base_port->port_configuration_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    base_port->maximum_transfer_length = 0;
    base_port->maximum_sg_entries = 0;
    base_port->debug_remove = FBE_FALSE;
	base_port->kek_handle = FBE_INVALID_KEY_HANDLE;
	base_port->kek_kek_handle = FBE_INVALID_KEY_HANDLE;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_destroy_object( fbe_object_handle_t object_handle)
{	
	fbe_status_t status;

    fbe_topology_class_trace(FBE_CLASS_ID_BASE_PORT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

	/* Step 1. Check parent edges */
	/* Step 2. Cleanup */
	/* Step 3. Call parent destructor */
	status = fbe_base_discovering_destroy_object(object_handle);
	return status;
}

static fbe_status_t 
base_port_sem_completion(fbe_packet_t * p_packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * p_sem = (fbe_semaphore_t*)context;
    fbe_semaphore_release(p_sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_port_set_port_info(fbe_base_port_t * p_base_port)
{
	fbe_packet_t * p_packet = NULL;
	fbe_semaphore_t sem;
	fbe_base_board_get_port_info_t port_info;
	fbe_status_t status;

	fbe_semaphore_init(&sem, 0, 1);

	p_packet = fbe_transport_allocate_packet();
	fbe_transport_initialize_packet(p_packet);
	fbe_transport_build_control_packet(p_packet, 
	                                   FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_INFO,
	                                   &port_info,
	                                   sizeof(fbe_base_board_get_port_info_t),
	                                   sizeof(fbe_base_board_get_port_info_t),
	                                   base_port_sem_completion,
	                                   &sem);

	status = fbe_base_discovered_send_control_packet((fbe_base_discovered_t*)p_base_port, p_packet);
	if (status != FBE_STATUS_OK){
		fbe_transport_release_packet(p_packet);
		fbe_base_object_trace((fbe_base_object_t*)p_base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't get board port info, status: 0x%X",
								__FUNCTION__, status);
		return status;
	}

	fbe_semaphore_wait(&sem, NULL);
	fbe_semaphore_destroy(&sem);

	fbe_transport_release_packet(p_packet);

	p_base_port->port_number = port_info.server_index;
	p_base_port->io_port_number = port_info.io_port_number;
	p_base_port->io_portal_number = port_info.io_portal_number;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_port_number(fbe_base_port_t * base_port, fbe_port_number_t port_number)
{
	base_port->port_number = port_number;
	return FBE_STATUS_OK;
}
 
fbe_status_t
fbe_base_port_get_port_number(fbe_base_port_t * base_port, fbe_port_number_t * port_number)
{
	*port_number = base_port->port_number;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_io_port_number(fbe_base_port_t * base_port, fbe_u32_t * io_port_number)
{
	*io_port_number = base_port->io_port_number;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_io_portal_number(fbe_base_port_t * base_port, fbe_u32_t * io_portal_number)
{
	*io_portal_number = base_port->io_portal_number;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_assigned_bus_number(fbe_base_port_t * base_port, fbe_u32_t assigned_bus_number)
{
	base_port->assigned_bus_number = assigned_bus_number;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_assigned_bus_number(fbe_base_port_t * base_port, fbe_u32_t * assigned_bus_number)
{
	*assigned_bus_number = base_port->assigned_bus_number;
	return FBE_STATUS_OK;
}
/*
fbe_status_t 
fbe_base_port_get_port_type(fbe_base_port_t * base_port, fbe_port_type_t   *port_type)
{
	*port_type = base_port->port_type;
	return FBE_STATUS_OK;
}
*/
fbe_status_t 
fbe_base_port_set_port_connect_class(fbe_base_port_t * base_port, fbe_port_connect_class_t   connect_class)
{
	base_port->port_connect_class = connect_class;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_port_connect_class(fbe_base_port_t * base_port, fbe_port_connect_class_t   *connect_class)
{
	*connect_class = base_port->port_connect_class;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_port_role(fbe_base_port_t * base_port, fbe_port_role_t  port_role)
{
	base_port->port_role = port_role;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_port_role(fbe_base_port_t * base_port, fbe_port_role_t   *port_role)
{
	*port_role = base_port->port_role;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_port_vendor_id(fbe_base_port_t * base_port, fbe_u32_t   *vendor_id)
{
	*vendor_id = base_port->hardware_info.vendor_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_port_device_id(fbe_base_port_t * base_port, fbe_u32_t   *device_id)
{
	*device_id = base_port->hardware_info.device_id;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_hardware_info(fbe_base_port_t * base_port, fbe_port_hardware_info_t *port_hardware_info)
{
	base_port->hardware_info = *port_hardware_info;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_hardware_info(fbe_base_port_t * base_port, fbe_port_hardware_info_t *port_hardware_info)
{
	*port_hardware_info = base_port->hardware_info;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_sfp_info(fbe_base_port_t * base_port, fbe_port_sfp_info_t *port_sfp_info)
{
	base_port->port_sfp_info = *port_sfp_info;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_sfp_info(fbe_base_port_t * base_port, fbe_port_sfp_info_t *port_sfp_info)
{
	*port_sfp_info = base_port->port_sfp_info;
	return FBE_STATUS_OK;
}
    
fbe_status_t 
fbe_base_port_set_maximum_transfer_length(fbe_base_port_t * base_port, fbe_u32_t maximum_transfer_length)
{
	base_port->maximum_transfer_length = maximum_transfer_length;
	return FBE_STATUS_OK;
}
 
fbe_status_t
fbe_base_port_get_maximum_transfer_length(fbe_base_port_t * base_port, fbe_u32_t * maximum_transfer_length)
{
	*maximum_transfer_length = base_port->maximum_transfer_length;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_maximum_sg_entries(fbe_base_port_t * base_port, fbe_u32_t maximum_sg_entries)
{
	base_port->maximum_sg_entries = maximum_sg_entries;
	return FBE_STATUS_OK;
}
 
fbe_status_t
fbe_base_port_get_maximum_sg_entries(fbe_base_port_t * base_port, fbe_u32_t * maximum_sg_entries)
{
	*maximum_sg_entries = base_port->maximum_sg_entries;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_link_info(fbe_base_port_t * base_port, fbe_port_link_information_t *port_link_info_p)
{
	*port_link_info_p = base_port->port_link_info;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_link_info(fbe_base_port_t * base_port, fbe_port_link_information_t *port_link_info_p)
{
	base_port->port_link_info = *port_link_info_p;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_set_core_affinity_info(fbe_base_port_t * base_port, fbe_bool_t affinity_enabled, fbe_u64_t active_proc_mask)
{
    base_port->multi_core_affinity_enabled = affinity_enabled;
    base_port->active_proc_mask            = active_proc_mask;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_port_get_core_affinity_info(fbe_base_port_t * base_port, fbe_bool_t *affinity_enabled, fbe_u64_t *active_proc_mask)
{
    *affinity_enabled = base_port->multi_core_affinity_enabled;
    *active_proc_mask = base_port->active_proc_mask;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_port_get_debug_remove(fbe_base_port_t *base_port, fbe_bool_t *debug_remove_p)
{
    *debug_remove_p = base_port->debug_remove;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_port_set_debug_remove(fbe_base_port_t *base_port, fbe_bool_t debug_remove)
{
    base_port->debug_remove = debug_remove;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_port_get_kek_handle(fbe_base_port_t * base_port, fbe_key_handle_t *kek_handle)
{
	*kek_handle = base_port->kek_handle;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_base_port_get_kek_kek_handle(fbe_base_port_t * base_port, fbe_key_handle_t *kek_kek_handle)
{
	*kek_kek_handle = base_port->kek_kek_handle;
	return FBE_STATUS_OK;
}

fbe_status_t fbe_base_port_set_kek_handle(fbe_base_port_t * base_port, fbe_key_handle_t kek_handle)
{
	base_port->kek_handle = kek_handle;
	return FBE_STATUS_OK;
}

