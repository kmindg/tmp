#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"

#include "fbe/fbe_board.h"
#include "armada_board_private.h"
#include "fbe_cpd_shim.h"

/* Class methods forward declaration */
fbe_status_t armada_board_load(void);
fbe_status_t armada_board_unload(void);
static fbe_port_connect_class_t armada_board_get_port_type(fbe_cpd_shim_connect_class_t cpd_shim_port_type);
static fbe_port_role_t armada_board_get_port_role(fbe_cpd_shim_port_role_t cpd_shim_port_role);
/* Export class methods  */
fbe_class_methods_t fbe_armada_board_class_methods = {
                                                    FBE_CLASS_ID_ARMADA_BOARD,
                                                    armada_board_load,
                                                    armada_board_unload,
                                                    fbe_armada_board_create_object,
                                                    fbe_armada_board_destroy_object,
                                                    fbe_armada_board_control_entry,
                                                    fbe_armada_board_event_entry,
                                                    NULL,
                                                    fbe_armada_board_monitor_entry
                                                    };

fbe_status_t armada_board_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_ARMADA_BOARD,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return fbe_armada_board_monitor_load_verify();
}

fbe_status_t armada_board_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_ARMADA_BOARD,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_armada_board_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_armada_board_t * armada_board;

    
    /* Call parent constructor */
    status = fbe_base_board_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    armada_board = (fbe_armada_board_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_trace_level((fbe_base_object_t *) armada_board, FBE_TRACE_LEVEL_DEBUG_HIGH);
    fbe_base_object_set_class_id((fbe_base_object_t *) armada_board, FBE_CLASS_ID_ARMADA_BOARD);
    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_armada_board_init(armada_board);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_armada_board_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_armada_board_t * armada_board;

    armada_board = (fbe_armada_board_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Call parent destructor */
    status = fbe_base_board_destroy_object(object_handle);
    return status;
}


fbe_status_t 
fbe_armada_board_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_armada_board_init(fbe_armada_board_t * armada_board)
{
#if 0
    fbe_status_t status;
    fbe_object_mgmt_attributes_t mgmt_attributes;
#endif
    fbe_u32_t i;

    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the number of io_ports in the base object. */
    fbe_base_board_set_number_of_io_ports((fbe_base_board_t *)armada_board, FBE_ARMADA_BOARD_NUMBER_OF_IO_PORTS); 

    /* Mark all our ports as invalid */
    for (i = 0; i < FBE_ARMADA_BOARD_NUMBER_OF_IO_PORTS; i++){
        armada_board->io_port_array[i].client_instantiated = FBE_FALSE;
        armada_board->io_port_array[i].connect_class = FBE_PORT_CONNECT_CLASS_INVALID;
        armada_board->io_port_array[i].port_role = FBE_PORT_ROLE_INVALID;
        armada_board->io_port_array[i].io_portal_number = FBE_BASE_BOARD_IO_PORTAL_INVALID;
        armada_board->io_port_array[i].io_port_number = FBE_BASE_BOARD_IO_PORT_INVALID;
        armada_board->io_port_array[i].be_port_number = FBE_BASE_BOARD_BE_PORT_INVALID;
    }
    armada_board->rescan_required = FBE_FALSE;
    armada_board->rescan_countdown = 0;

#if 0
    /* Set initial flags */
    fbe_base_object_lock((fbe_base_object_t *)armada_board);
    status = fbe_base_object_get_mgmt_attributes((fbe_base_object_t *) armada_board, &mgmt_attributes);
    mgmt_attributes &= ~FBE_BASE_OBJECT_FLAG_INIT_REQUIRED;
    status = fbe_base_object_set_mgmt_attributes((fbe_base_object_t *) armada_board, mgmt_attributes);
    fbe_base_object_unlock((fbe_base_object_t *)armada_board);
#endif

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_armada_board_update_hardware_ports(fbe_armada_board_t * armada_board, fbe_bool_t *rescan_required)
{
    fbe_status_t status;
    fbe_u32_t ii,jj,empty_slot_index;
    fbe_bool_t duplicate_entry_found = FBE_FALSE, be_rescan_required = FALSE;
    fbe_cpd_shim_backend_port_info_t *current_be_port = NULL;
    fbe_armada_board_io_port_t *current_armada_io_port = NULL;
    fbe_cpd_shim_enumerate_io_modules_t    cpd_shim_enumerate_io_modules;
    fbe_cpd_shim_enumerate_backend_ports_t port_info;    
    
    fbe_base_object_trace((fbe_base_object_t*)armada_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    memset(&cpd_shim_enumerate_io_modules, 0, sizeof(cpd_shim_enumerate_io_modules)); // SAFEBUG - uninitialized fields seen to be used
    memset(&port_info, 0, sizeof(port_info)); // SAFEBUG - uninitialized fields seen to be used
    status = fbe_cpd_shim_enumerate_backend_io_modules(&cpd_shim_enumerate_io_modules);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) armada_board, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s enumerate_backend_io_modules failed, status: 0x%X", __FUNCTION__, status);
        *rescan_required = FBE_TRUE;
    }

    port_info.number_of_io_ports = FBE_CPD_SHIM_MAX_PORTS;

    status = fbe_cpd_shim_enumerate_backend_ports(&port_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) armada_board, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s enumerate_backend_ports failed, status: 0x%X", __FUNCTION__, status);
        *rescan_required = FBE_TRUE;
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *) armada_board, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%X hardware ports found: \n", port_info.number_of_io_ports);


    fbe_base_board_set_number_of_io_ports((fbe_base_board_t *) armada_board, port_info.number_of_io_ports);    

    for(ii = 0; ii < port_info.number_of_io_ports; ii++) {    
        current_be_port = &(port_info.io_port_array[ii]);
        duplicate_entry_found = FBE_FALSE;
        empty_slot_index = FBE_CPD_SHIM_MAX_PORTS;

        for(jj = 0; jj < FBE_CPD_SHIM_MAX_PORTS; jj++) {
            /* Identify possible duplicate entry & the first empty slot*/
            current_armada_io_port = &(armada_board->io_port_array[jj]);
            if (current_armada_io_port->io_port_number != FBE_BASE_BOARD_IO_PORT_INVALID){

                if ((current_armada_io_port->be_port_number == current_be_port->assigned_bus_number) &&
                    (current_armada_io_port->io_portal_number == current_be_port->portal_number) &&
                    (current_armada_io_port->io_port_number == current_be_port->port_number)){
                        duplicate_entry_found = FBE_TRUE;
                        break;
                }
            }else{
                /*Empty slot found.*/
                empty_slot_index = ((empty_slot_index > jj) ? jj : empty_slot_index);
            }
        }

        if (!duplicate_entry_found){
            if (empty_slot_index >= FBE_CPD_SHIM_MAX_PORTS){
                fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s no empty slot in io_port_array , index: 0x%X\n",
                                       __FUNCTION__, empty_slot_index);
                fbe_base_object_trace((fbe_base_object_t*)armada_board, 
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s skipping entry for be: 0x%X port: 0x%X portal : 0x%X \n",
                                       __FUNCTION__, current_be_port->assigned_bus_number,
                                       current_be_port->port_number,current_be_port->portal_number);

            }else{
                current_armada_io_port = &(armada_board->io_port_array[empty_slot_index]);

                current_armada_io_port->client_instantiated = FBE_FALSE;
                current_armada_io_port->connect_class = armada_board_get_port_type(current_be_port->connect_class);
                current_armada_io_port->port_role = armada_board_get_port_role(current_be_port->port_role);
                current_armada_io_port->io_portal_number = (fbe_u32_t)current_be_port->portal_number; 
                current_armada_io_port->io_port_number = (fbe_u32_t)current_be_port->port_number;
                current_armada_io_port->be_port_number = (fbe_u32_t)current_be_port->assigned_bus_number;

#if 0
                current_armada_io_port->bus = (fbe_u32_t)current_be_port->hdw_info.bus;
                current_armada_io_port->slot = (fbe_u32_t)current_be_port->hdw_info.slot;
                current_armada_io_port->function = (fbe_u32_t)current_be_port->hdw_info.function;
                current_armada_io_port->vendor = (fbe_u32_t)current_be_port->hdw_info.vendor;
                current_armada_io_port->device = (fbe_u32_t)current_be_port->hdw_info.device;
#endif

        fbe_base_object_trace((fbe_base_object_t *) armada_board, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s hardware port: 0x%X/0x%X/0x%X/0x%X\n",
                                    __FUNCTION__,
                                    current_armada_io_port->connect_class,
                                    current_armada_io_port->io_port_number,
                                    current_armada_io_port->io_portal_number,                                    
                                    current_armada_io_port->be_port_number);
    }
        }
    }
    be_rescan_required = ((cpd_shim_enumerate_io_modules.rescan_required)
           || (port_info.rescan_required)
           || (cpd_shim_enumerate_io_modules.total_enumerated_io_ports != port_info.total_discovered_io_ports));
    *rescan_required = be_rescan_required;
    if (be_rescan_required){
        /* Trace reasons for rescan*/
        if (cpd_shim_enumerate_io_modules.total_enumerated_io_ports 
            != port_info.total_discovered_io_ports){
                fbe_base_object_trace((fbe_base_object_t *) armada_board, 
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Rescan triggered due to port mismatch. Module scan : %d. Port scan %d.\n",
                                    __FUNCTION__,
                                    cpd_shim_enumerate_io_modules.total_enumerated_io_ports,
                                    port_info.total_discovered_io_ports);
            }
        fbe_base_object_trace((fbe_base_object_t *) armada_board, 
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Rescan triggered. Module scan : %d. Port scan %d.\n",
                            __FUNCTION__,
                            cpd_shim_enumerate_io_modules.rescan_required,
                            port_info.rescan_required);            
    }
    return FBE_STATUS_OK;
}

static fbe_port_connect_class_t 
armada_board_get_port_type(fbe_cpd_shim_connect_class_t cpd_shim_port_type)
{
    fbe_port_connect_class_t port_type = FBE_PORT_CONNECT_CLASS_INVALID;

    switch(cpd_shim_port_type){
        case FBE_CPD_SHIM_CONNECT_CLASS_SAS:
            port_type = FBE_PORT_CONNECT_CLASS_SAS;
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_FC:
            port_type = FBE_PORT_CONNECT_CLASS_FC;
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_ISCSI:
            port_type = FBE_PORT_CONNECT_CLASS_ISCSI;
            break;
            
        case FBE_CPD_SHIM_CONNECT_CLASS_FCOE:
            port_type = FBE_PORT_CONNECT_CLASS_FCOE;
            break;
            
    }

    return port_type;
}
static fbe_port_role_t 
armada_board_get_port_role(fbe_cpd_shim_port_role_t cpd_shim_port_role)
{
    fbe_port_role_t  port_role = FBE_PORT_ROLE_INVALID;

    switch (cpd_shim_port_role){
        case FBE_CPD_SHIM_PORT_ROLE_FE:
            port_role = FBE_PORT_ROLE_FE;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_BE:
            port_role = FBE_PORT_ROLE_BE;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_UNC:
            port_role = FBE_PORT_ROLE_UNC;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_BOOT:
            port_role = FBE_PORT_ROLE_BOOT;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_SPECIAL:
            port_role = FBE_PORT_ROLE_SPECIAL;
            break;
    }

    return port_role;
}
