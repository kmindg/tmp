#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"

#include "fbe/fbe_board.h"
#include "magnum_board_private.h"
#include "fbe_cpd_shim.h"

/* Class methods forward declaration */
fbe_status_t magnum_board_load(void);
fbe_status_t magnum_board_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_magnum_board_class_methods = {
                                                    FBE_CLASS_ID_MAGNUM_BOARD,
                                                    magnum_board_load,
                                                    magnum_board_unload,
                                                    fbe_magnum_board_create_object,
                                                    fbe_magnum_board_destroy_object,
                                                    fbe_magnum_board_control_entry,
                                                    fbe_magnum_board_event_entry,
                                                    NULL,
                                                    fbe_magnum_board_monitor_entry
                                                    };

fbe_status_t magnum_board_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_MAGNUM_BOARD,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return fbe_magnum_board_monitor_load_verify();
}

fbe_status_t magnum_board_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_MAGNUM_BOARD,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_magnum_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_magnum_board_t * magnum_board;

    
    
    /* Call parent constructor */
    status = fbe_base_board_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    magnum_board = (fbe_magnum_board_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) magnum_board, FBE_CLASS_ID_MAGNUM_BOARD);
    fbe_base_object_trace((fbe_base_object_t*)magnum_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_magnum_board_init(magnum_board);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_magnum_board_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_magnum_board_t * magnum_board;

    magnum_board = (fbe_magnum_board_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)magnum_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Call parent destructor */
    status = fbe_base_board_destroy_object(object_handle);
    return status;
}


fbe_status_t 
fbe_magnum_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_magnum_board_init(fbe_magnum_board_t * magnum_board)
{
    fbe_status_t status;
    fbe_object_mgmt_attributes_t mgmt_attributes;
    fbe_u32_t i;

    fbe_base_object_trace((fbe_base_object_t*)magnum_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the number of io_ports in the base object. */
    fbe_base_board_set_number_of_io_ports((fbe_base_board_t *)magnum_board, FBE_MAGNUM_BOARD_NUMBER_OF_IO_PORTS); 

    /* Mark all our ports as invalid */
    for (i = 0; i < FBE_MAGNUM_BOARD_NUMBER_OF_IO_PORTS; i++){
        magnum_board->io_port_array[i].io_port_type = FBE_PORT_TYPE_INVALID;
        magnum_board->io_port_array[i].io_portal_number = FBE_BASE_BOARD_IO_PORTAL_INVALID;
        magnum_board->io_port_array[i].io_port_number = FBE_BASE_BOARD_IO_PORT_INVALID;
        magnum_board->io_port_array[i].be_port_number = FBE_BASE_BOARD_BE_PORT_INVALID;
    }

    /* Set initial flags */
    fbe_base_object_lock((fbe_base_object_t *)magnum_board);
    status = fbe_base_object_get_mgmt_attributes((fbe_base_object_t *) magnum_board, &mgmt_attributes);
    
    status = fbe_base_object_set_mgmt_attributes((fbe_base_object_t *) magnum_board, mgmt_attributes);
    fbe_base_object_unlock((fbe_base_object_t *)magnum_board);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_magnum_board_update_hardware_ports(fbe_magnum_board_t * magnum_board, fbe_bool_t *rescan_required)
{
    fbe_status_t status;
    fbe_u32_t i;
    fbe_cpd_shim_enumerate_backend_ports_t cpd_shim_enumerate_backend_ports = {0};
    
    fbe_base_object_trace((fbe_base_object_t*)magnum_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    cpd_shim_enumerate_backend_ports.number_of_io_ports = FBE_CPD_SHIM_MAX_PORTS;
    status = fbe_cpd_shim_enumerate_backend_ports(&cpd_shim_enumerate_backend_ports);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) magnum_board, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s enumerate_backend_ports failed, status: 0x%X", __FUNCTION__, status);
        *rescan_required = FBE_TRUE;
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *) magnum_board, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%X hardware ports found: \n", cpd_shim_enumerate_backend_ports.number_of_io_ports);

    fbe_base_board_set_number_of_io_ports((fbe_base_board_t *) magnum_board, cpd_shim_enumerate_backend_ports.number_of_io_ports);
    for(i = 0; i < cpd_shim_enumerate_backend_ports.number_of_io_ports; i++) {
        magnum_board->io_port_array[i].io_port_type = cpd_shim_enumerate_backend_ports.io_port_array[i].port_type;
        magnum_board->io_port_array[i].io_portal_number = (fbe_u32_t)cpd_shim_enumerate_backend_ports.io_port_array[i].portal_number; 
        magnum_board->io_port_array[i].io_port_number = (fbe_u32_t)cpd_shim_enumerate_backend_ports.io_port_array[i].port_number;
        magnum_board->io_port_array[i].be_port_number = (fbe_u32_t)cpd_shim_enumerate_backend_ports.io_port_array[i].assigned_bus_number;
        fbe_base_object_trace((fbe_base_object_t *) magnum_board, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s hardware port: 0x%X/0x%X/0x%X/0x%X\n",
                              __FUNCTION__,
                              magnum_board->io_port_array[i].io_port_type,
                              magnum_board->io_port_array[i].io_portal_number,
                              magnum_board->io_port_array[i].io_port_number,
                              magnum_board->io_port_array[i].be_port_number);
    }
    *rescan_required = cpd_shim_enumerate_backend_ports.rescan_required;
    return FBE_STATUS_OK;
}
