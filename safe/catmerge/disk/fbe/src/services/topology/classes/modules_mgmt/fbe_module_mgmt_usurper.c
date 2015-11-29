/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management object usurper
 *  code.
 * 
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   14-April-2010:  Created. bphilbin
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_module_mgmt.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_board_interface.h"

//Local function definitions
static fbe_status_t fbe_module_mgmt_get_general_status(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
fbe_status_t fbe_module_mgmt_get_module_status(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);

static fbe_u32_t fbe_module_mgmt_get_module_number(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                   fbe_u32_t sp, 
                                                   fbe_u64_t type,
                                                   fbe_u32_t slot);

fbe_status_t fbe_module_mgmt_get_io_module_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);

static fbe_u32_t fbe_module_mgmt_get_port_number(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                   fbe_u32_t sp, 
                                                 fbe_u64_t type, 
                                                   fbe_u32_t slot,
                                                   fbe_u32_t port);
fbe_status_t fbe_module_mgmt_get_io_port_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
fbe_status_t fbe_module_mgmt_get_mezzanine_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
fbe_status_t fbe_module_mgmt_get_sfp_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_get_limits_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_set_port_config(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_config_mgmt_port_speed(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_validateMgmtPortConfigReq(fbe_module_mgmt_t *module_mgmt_ptr,
                                       fbe_esp_module_mgmt_set_mgmt_port_config_t *mgmt_port_config_req);
static fbe_status_t fbe_module_mgmt_get_mgmt_comp_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                       fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_get_requested_mgmt_port_config(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                       fbe_packet_t * packet);
static fbe_status_t fbe_module_mgm_handle_set_port_list_request(fbe_module_mgmt_t *module_mgmt, 
                                          fbe_u32_t num_ports, 
                                          fbe_esp_module_mgmt_port_config_t *new_port_config,
                                          fbe_bool_t overwrite);
static fbe_status_t fbe_module_mgm_handle_remove_port_list_request(fbe_module_mgmt_t *module_mgmt, 
                                          fbe_u32_t num_ports, 
                                          fbe_esp_module_mgmt_port_config_t *new_port_config);
static fbe_status_t fbe_module_mgmt_markIoModule(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                 fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_markIoPort(fbe_module_mgmt_t *module_mgmt_ptr, 
                                               fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_get_port_affinity(fbe_module_mgmt_t *module_mgmt_ptr,
                                                      fbe_packet_t * packet);
/*!***************************************************************
 * fbe_module_mgmt_control_entry()
 ****************************************************************
 * @brief
 *  This function is entry point for control operation for this
 *  class
 *
 * @param  object_handle - pointer to the module management object
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_module_mgmt_t                          *module_mgmt = NULL;
    
    module_mgmt = (fbe_module_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_trace((fbe_base_object_t*)module_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, control_code 0x%x\n", 
                          __FUNCTION__, control_code);
    switch(control_code) 
    {
    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_LIMITS_INFO:
        status = fbe_module_mgmt_get_limits_info(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_GENERAL_STATUS:
        status = fbe_module_mgmt_get_general_status(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS:
        status = fbe_module_mgmt_get_module_status(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_MODULE_INFO:
        status = fbe_module_mgmt_get_io_module_info(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_PORT_INFO:
        status = fbe_module_mgmt_get_io_port_info(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_SFP_INFO:
        status = fbe_module_mgmt_get_sfp_info(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MEZZANINE_INFO:
        status = fbe_module_mgmt_get_mezzanine_info(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_PORT_CONFIG:
        status = fbe_module_mgmt_set_port_config(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_MGMT_PORT_CONFIG:
        status = fbe_module_mgmt_config_mgmt_port_speed(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MGMT_COMP_INFO:
        status = fbe_module_mgmt_get_mgmt_comp_info(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_REQUESTED_MGMT_PORT_CONFIG:
        status = fbe_module_mgmt_get_requested_mgmt_port_config(module_mgmt, packet);
        break;
        
    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_MARK_IOM:
        status = fbe_module_mgmt_markIoModule(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_MARK_IOPORT:
        status = fbe_module_mgmt_markIoPort(module_mgmt, packet);
        break;

    case FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_PORT_AFFINITY:
        status = fbe_module_mgmt_get_port_affinity(module_mgmt, packet);
        break;
                
    default:
        status =  fbe_base_environment_control_entry (object_handle, packet);
        break;
    }

    return status;

}
/******************************************
 * end fbe_module_mgmt_control_entry()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_general_status()
 ****************************************************************
 * @brief
 *  This function processes the GET_GENERAL_STATUS Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  06-Mar-2013 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_general_status(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_esp_module_mgmt_general_status_t     *general_status_info_ptr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    
    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &general_status_info_ptr);
    if (general_status_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_general_status_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current general status

    general_status_info_ptr->port_configuration_loaded = module_mgmt_ptr->port_configuration_loaded;
    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Port Config Loaded %d\n", __FUNCTION__, module_mgmt_ptr->port_configuration_loaded);


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_general_status()
 ******************************************/
/*!***************************************************************
 * fbe_module_mgmt_get_module_status()
 ****************************************************************
 * @brief
 *  This function processes the GET_MODULE_STATUS Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_module_status(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_esp_module_mgmt_module_status_t     *module_status_info_ptr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               index = 0;
    SP_ID                                   sp;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &module_status_info_ptr);
    if (module_status_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_module_status_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current MODULE status

    sp = module_status_info_ptr->header.sp;
    if(sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, IOModule sp %d invalidd\n",
                              __FUNCTION__,
                              module_status_info_ptr->header.sp);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    index = fbe_module_mgmt_convert_device_type_and_slot_to_index(module_status_info_ptr->header.type, 
                                                                       module_status_info_ptr->header.slot);
    if(index >= FBE_ESP_IO_MODULE_MAX_COUNT)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, IOModule index %d invalid, IOM type: %lld, slot %d\n",
                              __FUNCTION__,
                              index,
                              module_status_info_ptr->header.type,
                              module_status_info_ptr->header.slot);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    module_status_info_ptr->state = module_mgmt_ptr->io_module_info[index].logical_info[sp].module_state;
    module_status_info_ptr->substate = module_mgmt_ptr->io_module_info[index].logical_info[sp].module_substate;
    module_status_info_ptr->type = module_mgmt_ptr->io_module_info[index].logical_info[sp].slic_type;
    module_status_info_ptr->protocol = fbe_module_mgmt_get_io_module_protocol(module_mgmt_ptr, sp, index);

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, module state 0x%x\n", 
                           __FUNCTION__, module_status_info_ptr->state);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_module_status()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_module_number()
 ****************************************************************
 * @brief
 *  This function returns a module number offset into the global array
 *  of modules in the module management object based on the specified
 *  physical location parameters.
 *
 * @param  module_mgmt_ptr - Pointer to the module management object.
 * @param  sp - SP id
 * @param  module_class - Module Class
 * @param  slot - Slot number.
 *
 * @return  module number
 *
 * @author
 *  22-April-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_u32_t fbe_module_mgmt_get_module_number(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                   fbe_u32_t sp, 
                                                   fbe_u64_t type,
                                                   fbe_u32_t slot)
{
    fbe_u32_t module_number = 0;
    for(module_number; module_number < FBE_ESP_IO_MODULE_MAX_COUNT; module_number++)
    {
        if((module_mgmt_ptr->io_module_info[module_number].physical_info[sp].module_comp_info.associatedSp == sp) &&
            (module_mgmt_ptr->io_module_info[module_number].physical_info[sp].type == type) &&
            (module_mgmt_ptr->io_module_info[module_number].physical_info[sp].module_comp_info.slotNumOnBlade == slot))
        {
            return module_number;
        }
    }
    // The specified location was not discovered in the module info list.
    return INVALID_MODULE_NUMBER;
}

/*!***************************************************************
 * fbe_module_mgmt_get_port_number()
 ****************************************************************
 * @brief
 *  This function returns a port number offset into the global array
 *  of io ports in the module management object based on the specified
 *  physical location parameters.
 *
 * @param  module_mgmt_ptr - Pointer to the module management object.
 * @param  sp - SP id
 * @param  slot - Slot number.
 * @param port - port number
 *
 * @return  module number
 *
 * @author
 *  27-May-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_u32_t fbe_module_mgmt_get_port_number(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                   fbe_u32_t sp, 
                                                 fbe_u64_t type, 
                                                   fbe_u32_t slot,
                                                   fbe_u32_t port)
{
    fbe_u32_t port_number = 0;

    for(port_number; port_number < FBE_ESP_IO_PORT_MAX_COUNT; port_number++)
    {
        if((module_mgmt_ptr->io_port_info[port_number].port_physical_info.port_comp_info[sp].associatedSp == sp) &&
            (module_mgmt_ptr->io_port_info[port_number].port_physical_info.port_comp_info[sp].slotNumOnBlade == slot) &&
            (module_mgmt_ptr->io_port_info[port_number].port_physical_info.port_comp_info[sp].portNumOnModule == port) &&
            (module_mgmt_ptr->io_port_info[port_number].port_physical_info.port_comp_info[sp].deviceType == type))
        {
            return port_number;
        }
    }
    // The specified location was not discovered in the module info list.
    return INVALID_MODULE_NUMBER;
}


/******************************************
 * end fbe_module_mgmt_get_module_number()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_io_module_info()
 ****************************************************************
 * @brief
 *  This function processes the GET_IO_MODULE_INFO Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  26-May-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_io_module_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_esp_module_io_module_info_t         *io_module_info_ptr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               index = 0;
    SP_ID                                   sp;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &io_module_info_ptr);
    if (io_module_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_io_module_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sp = io_module_info_ptr->header.sp;
    if(sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, IOModule sp %d invalid\n",
                              __FUNCTION__,
                              io_module_info_ptr->header.sp);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    index = fbe_module_mgmt_convert_device_type_and_slot_to_index(io_module_info_ptr->header.type, io_module_info_ptr->header.slot);
    if(index >= FBE_ESP_IO_MODULE_MAX_COUNT)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, IOModule index %d invalid, IOM type: %lld, slot %d\n",
                              __FUNCTION__,
                              index,
                              io_module_info_ptr->header.type,
                              io_module_info_ptr->header.slot);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memcpy(&(io_module_info_ptr->io_module_phy_info), 
        &(module_mgmt_ptr->io_module_info[index].physical_info[sp].module_comp_info),
        sizeof(fbe_board_mgmt_io_comp_info_t));

    strncpy(io_module_info_ptr->label_name, module_mgmt_ptr->io_module_info[index].physical_info[sp].label_name, 
            strlen(module_mgmt_ptr->io_module_info[index].physical_info[sp].label_name));

    io_module_info_ptr->expected_slic_type =
            fbe_module_mgmt_get_expected_slic_type(module_mgmt_ptr, io_module_info_ptr->header.sp, index);

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s,module inserted 0%x\n", 
                           __FUNCTION__, io_module_info_ptr->io_module_phy_info.inserted);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_io_module_info()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_io_port_info()
 ****************************************************************
 * @brief
 *  This function processes the GET_IO_PORT_INFO Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  27-May-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_io_port_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_esp_module_io_port_info_t           *io_port_info_ptr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               port_num = 0;
    fbe_u32_t								port_index = 0;
    fbe_u32_t                               iom_num = 0;
    SP_ID                                   sp;
    fbe_board_mgmt_io_port_info_t           *port_info;
    fbe_port_link_information_t             link_info;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &io_port_info_ptr);
    if (io_port_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_io_port_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_index = fbe_module_mgmt_get_port_number(module_mgmt_ptr, 
        io_port_info_ptr->header.sp, io_port_info_ptr->header.type, io_port_info_ptr->header.slot, io_port_info_ptr->header.port);

    if(port_index == INVALID_MODULE_NUMBER)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: PortInfo - SP:%d %s slot:%d port:%d does not exist\n",
                              io_port_info_ptr->header.sp,
                              fbe_module_mgmt_device_type_to_string(io_port_info_ptr->header.type),
                              io_port_info_ptr->header.slot,
                              io_port_info_ptr->header.port);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sp = io_port_info_ptr->header.sp;
    iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(io_port_info_ptr->header.type, io_port_info_ptr->header.slot);
    port_num = io_port_info_ptr->header.port;

    /* Fill in the port physical data. */
    port_info = &module_mgmt_ptr->io_port_info[port_index].port_physical_info.port_comp_info[sp];

    io_port_info_ptr->io_port_info.associatedSp = port_info->associatedSp;
    io_port_info_ptr->io_port_info.deviceType = port_info->deviceType;
    io_port_info_ptr->io_port_info.ioPortLED = port_info->ioPortLED;
    io_port_info_ptr->io_port_info.ioPortLEDColor = port_info->ioPortLEDColor;
    io_port_info_ptr->io_port_info.isLocalFru = port_info->isLocalFru;
    fbe_copy_memory(&io_port_info_ptr->io_port_info.pciData, 
                    &port_info->pciData, sizeof(SPECL_PCI_DATA));
    io_port_info_ptr->io_port_info.portNumOnModule = port_info->portNumOnModule;
    io_port_info_ptr->io_port_info.powerStatus = port_info->powerStatus;
    io_port_info_ptr->io_port_info.present = port_info->present;
    io_port_info_ptr->io_port_info.protocol = port_info->protocol;
    io_port_info_ptr->io_port_info.SFPcapable = port_info->SFPcapable;
    io_port_info_ptr->io_port_info.slotNumOnBlade = port_info->slotNumOnBlade;
    io_port_info_ptr->io_port_info.portal_number = port_info->portal_number;
    // workaournd to maks the 1.5 Gbps capable speed from Navisphere
    if(port_info->protocol == IO_CONTROLLER_PROTOCOL_SAS)
    {
        io_port_info_ptr->io_port_info.supportedSpeeds = port_info->supportedSpeeds & ~SPEED_ONE_FIVE_GIGABIT;
    }
    else
    {
        io_port_info_ptr->io_port_info.supportedSpeeds = port_info->supportedSpeeds;
    }

    /* Fill in the port logical data. */
    io_port_info_ptr->io_port_logical_info.port_state = module_mgmt_ptr->io_port_info[port_index].port_logical_info[sp].port_state;
    io_port_info_ptr->io_port_logical_info.port_substate = module_mgmt_ptr->io_port_info[port_index].port_logical_info[sp].port_substate;
    io_port_info_ptr->io_port_logical_info.logical_number = fbe_module_mgmt_get_port_logical_number(module_mgmt_ptr, sp, iom_num, port_num);
    io_port_info_ptr->io_port_logical_info.port_role = fbe_module_mgmt_get_port_role(module_mgmt_ptr, sp, iom_num, port_num);
    io_port_info_ptr->io_port_logical_info.port_subrole = fbe_module_mgmt_get_port_subrole(module_mgmt_ptr, sp, iom_num, port_num);
    io_port_info_ptr->io_port_logical_info.iom_group =fbe_module_mgmt_get_iom_group(module_mgmt_ptr, sp, iom_num, port_num);
    io_port_info_ptr->io_port_logical_info.is_combined_port = fbe_module_mgmt_is_port_configured_combined_port(module_mgmt_ptr,sp,iom_num,port_num);
    link_info = fbe_module_mgmt_get_port_link_info(module_mgmt_ptr, sp, iom_num, port_num);
    fbe_copy_memory(&io_port_info_ptr->io_port_info.link_info, &link_info, sizeof(fbe_port_link_information_t));

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: PortInfo - SP%d Port %d IOM:%d Port:%d state %d substate %d Role:%d Num:%d\n",
                          sp,
                          port_index,
                          iom_num,
                          port_num,
                          io_port_info_ptr->io_port_logical_info.port_state, 
                          io_port_info_ptr->io_port_logical_info.port_substate,
                          io_port_info_ptr->io_port_logical_info.port_role,
                          io_port_info_ptr->io_port_logical_info.logical_number);


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_io_port_info()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_sfp_info()
 ****************************************************************
 * @brief
 *  This function processes the GET_IO_PORT_INFO Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  27-May-2010 - Created. Nayana Chaudhari
 *  25-July-2011 - Modified. Harshal Wanjari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_sfp_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_esp_module_sfp_info_t               *sfp_info_ptr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               port_num = 0;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &sfp_info_ptr);
    if (sfp_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_sfp_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_num = fbe_module_mgmt_get_port_number(module_mgmt_ptr, 
        sfp_info_ptr->header.sp, sfp_info_ptr->header.type, sfp_info_ptr->header.slot, sfp_info_ptr->header.port);

    if(port_num == INVALID_MODULE_NUMBER)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Convert the internal sfp information structure into an 
     * external API version.
     */

    /* First zero out the structure for the return data. */
    fbe_zero_memory(&sfp_info_ptr->sfp_info,sizeof(fbe_esp_sfp_physical_info_t));

    /* Copy the pertinent values for the return data. */
    sfp_info_ptr->sfp_info.inserted = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_inserted;
    sfp_info_ptr->sfp_info.capable_speeds = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.speeds;
    sfp_info_ptr->sfp_info.cable_length = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.cable_length;
    sfp_info_ptr->sfp_info.hw_type = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.hw_type;
    sfp_info_ptr->sfp_info.media_type = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.media_type;
    sfp_info_ptr->sfp_info.condition_type = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.condition_type;
    sfp_info_ptr->sfp_info.sub_condition_type= module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.sub_condition_type;
    sfp_info_ptr->sfp_info.condition_additional_info = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.condition_additional_info;
    sfp_info_ptr->sfp_info.supported_protocols = module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.supported_protocols;

    /* Copy the resume prom strings into the return data. */
    memcpy(sfp_info_ptr->sfp_info.emc_part_number,
           module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.emc_part_number,
           (sizeof(fbe_u8_t) * FBE_PORT_SFP_EMC_DATA_LENGTH));
    memcpy(sfp_info_ptr->sfp_info.emc_serial_number,
           module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.emc_serial_number,
           (sizeof(fbe_u8_t) * FBE_PORT_SFP_EMC_DATA_LENGTH));
    memcpy(sfp_info_ptr->sfp_info.vendor_serial_number,
           module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.vendor_serial_number,
           (sizeof(fbe_u8_t) * FBE_PORT_SFP_VENDOR_DATA_LENGTH));
    memcpy(sfp_info_ptr->sfp_info.vendor_serial_number,
           module_mgmt_ptr->io_port_info[port_num].port_physical_info.sfp_info.vendor_serial_number,
           (sizeof(fbe_u8_t) * FBE_PORT_SFP_VENDOR_DATA_LENGTH));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_sfp_info()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_mezzanine_info()
 ****************************************************************
 * @brief
 *  This function processes the GET_MEZZANINE_INFO Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  26-May-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_mezzanine_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_esp_module_io_module_info_t         *io_module_info_ptr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    SP_ID                                   sp;
    fbe_u32_t                               iom_num;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &io_module_info_ptr);
    if (io_module_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_io_module_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sp = io_module_info_ptr->header.sp;
    iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(io_module_info_ptr->header.type, io_module_info_ptr->header.slot); 

    memcpy(&(io_module_info_ptr->io_module_phy_info), 
        &(module_mgmt_ptr->io_module_info[iom_num].physical_info[sp].module_comp_info),
        sizeof(fbe_board_mgmt_io_comp_info_t));

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                           "%s,module inserted 0%x\n", 
                           __FUNCTION__, io_module_info_ptr->io_module_phy_info.inserted);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_mezzanine_info()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_get_limits_info()
 ****************************************************************
 * @brief
 *  This function processes the GET_LIMITS_INFO Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Sep-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_get_limits_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_esp_module_limits_info_t            *limits_info_ptr = NULL;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &limits_info_ptr);
    if (limits_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_limits_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill in the supplied structure with the limits information */
    fbe_copy_memory(&limits_info_ptr->discovered_hw_limit, 
                    &module_mgmt_ptr->discovered_hw_limit, 
                    sizeof(fbe_esp_module_mgmt_discovered_hardware_limits_t));
    fbe_copy_memory(&limits_info_ptr->platform_hw_limit, 
                    &module_mgmt_ptr->platform_hw_limit, 
                    sizeof(fbe_environment_limits_platform_hardware_limits_t));
    fbe_copy_memory(&limits_info_ptr->platform_port_limit, 
                    &module_mgmt_ptr->platform_port_limit, 
                    sizeof(fbe_environment_limits_platform_port_limits_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_module_mgmt_get_limits_info()
 ******************************************/



/*!***************************************************************
 * fbe_module_mgmt_set_port_config()
 ****************************************************************
 * @brief
 *  This function processes the SET_PORT_CONFIG Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Nov-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_set_port_config(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_esp_module_mgmt_set_port_t          *set_port_cmd_ptr = NULL;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &set_port_cmd_ptr);
    if (set_port_cmd_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_set_port_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s received OpCode:%d \n", 
                          __FUNCTION__, set_port_cmd_ptr->opcode);

    switch(set_port_cmd_ptr->opcode)
    {
    case FBE_ESP_MODULE_MGMT_PERSIST_ALL_PORTS_CONFIG:
        status = fbe_module_mgmt_set_persist_ports_and_reboot(module_mgmt_ptr);
        break;
    case FBE_ESP_MODULE_MGMT_UPGRADE_PORTS:
        status = fbe_module_mgmt_set_upgrade_ports_and_reboot(module_mgmt_ptr);
        break;
    case FBE_ESP_MODULE_MGMT_REPLACE_PORTS:
        status = fbe_module_mgmt_reboot_to_allow_port_replace(module_mgmt_ptr);
        break;
    case FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST:
        status = fbe_module_mgm_handle_set_port_list_request(module_mgmt_ptr, set_port_cmd_ptr->num_ports, set_port_cmd_ptr->port_config, FALSE);
        break;
    case FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST_WITH_OVERWRITE:
        status = fbe_module_mgm_handle_set_port_list_request(module_mgmt_ptr, set_port_cmd_ptr->num_ports, set_port_cmd_ptr->port_config, TRUE);
        break;
    case FBE_ESP_MODULE_MGMT_REMOVE_ALL_PORTS_CONFIG:
        status = fbe_module_mgmt_remove_all_ports(module_mgmt_ptr);
        break;
    case FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST:
        status = fbe_module_mgm_handle_remove_port_list_request(module_mgmt_ptr, set_port_cmd_ptr->num_ports, set_port_cmd_ptr->port_config);
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}

/*!**************************************************************
 * fbe_module_mgm_handle_set_port_list_request()
 ****************************************************************
 * @brief
 *  This function converts the set port command data into the
 *  persistent port info that module management uses and then
 *  kicks off the process of updating the port configuration.
 *
 * @param - module_mgmt - context
 * @param - num_ports - number of ports to configure
 * @param - new_port_config - pointer to a list of new port config
 * @param - overwrite - overwrite of existing port config allowed 
 * 
 * @return - fbe_status_t
 * 
 * @author
 *  09-Sep-2011:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgm_handle_set_port_list_request(fbe_module_mgmt_t *module_mgmt, 
                                          fbe_u32_t num_ports, 
                                          fbe_esp_module_mgmt_port_config_t *new_port_config,
                                          fbe_bool_t overwrite)
{
    fbe_io_port_persistent_info_t *port_info = NULL;
    fbe_u32_t port_index = 0;
    fbe_status_t status;

    port_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, num_ports * sizeof(fbe_io_port_persistent_info_t));

    if(port_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s port_info allocate failed\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    for(port_index=0; port_index<num_ports; port_index++)
    {
        port_info[port_index].iom_group = new_port_config[port_index].iom_group;
        port_info[port_index].logical_num = new_port_config[port_index].logical_number;
        port_info[port_index].port_role = new_port_config[port_index].port_role;
        port_info[port_index].port_subrole = new_port_config[port_index].port_subrole;
        port_info[port_index].port_location.io_enclosure_number = 0;
        port_info[port_index].port_location.port = new_port_config[port_index].port_location.port;
        port_info[port_index].port_location.type_32_bit = (fbe_u32_t)new_port_config[port_index].port_location.type;
        port_info[port_index].port_location.slot = new_port_config[port_index].port_location.slot;

        fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: setport cmd details %s %d port %d: %s%d\n",
                              fbe_module_mgmt_device_type_to_string(port_info[port_index].port_location.type_32_bit),
                              port_info[port_index].port_location.slot,
                              port_info[port_index].port_location.port,
                              fbe_module_mgmt_port_role_to_string(port_info[port_index].port_role),
                              port_info[port_index].logical_num);

    }
    status = fbe_module_mgm_set_port_list(module_mgmt, num_ports, port_info, overwrite);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, port_info);
    return status;
}

/*!**************************************************************
 * fbe_module_mgm_handle_remove_port_list_request()
 ****************************************************************
 * @brief
 *  This function handles the request to remove a list of ports. 
 *
 * @param - module_mgmt - context
 * @param - num_ports - number of ports to configure
 * @param - new_port_config - pointer to a list of new port config
 * 
 * @return - fbe_status_t
 * 
 * @author
 *  09-Sep-2011:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgm_handle_remove_port_list_request(fbe_module_mgmt_t *module_mgmt, 
                                          fbe_u32_t num_ports, 
                                          fbe_esp_module_mgmt_port_config_t *new_port_config)
{
    fbe_u32_t port_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t iom_num;
    fbe_u32_t persitent_port_index;
    SP_ID sp;
    fbe_u8_t persistent_port_index;
    fbe_u8_t port_num;
    fbe_io_port_persistent_info_t *persistent_data_p;

    sp = module_mgmt->local_sp;

    for(port_index=0; port_index<num_ports; port_index++)
    {
        iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(new_port_config[port_index].port_location.type,
                                                                        new_port_config[port_index].port_location.slot);
        persitent_port_index = fbe_module_mgmt_get_io_port_index(iom_num, new_port_config[port_index].port_location.port);
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.logical_num = INVALID_PORT_U8;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.port_subrole = FBE_PORT_SUBROLE_UNINTIALIZED;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.iom_group = FBE_IOM_GROUP_UNKNOWN;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.port_location.io_enclosure_number = INVALID_ENCLOSURE_NUMBER;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.port_location.type_32_bit = FBE_DEVICE_TYPE_INVALID;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.port_location.slot = INVALID_MODULE_NUMBER;
        module_mgmt->io_port_info[persitent_port_index].port_logical_info[sp].port_configured_info.port_location.port = INVALID_PORT_U8;

        fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: removed port config for %s %d port %d.\n",
                              fbe_module_mgmt_device_type_to_string(new_port_config[port_index].port_location.type),
                              new_port_config[port_index].port_location.slot,
                              new_port_config[port_index].port_location.port);

    }

    /*
     * Write the configuration out to disk
     */
    // disabling file based persistence to instead use persistent service

    persistent_port_index = 0;
    persistent_data_p = (fbe_io_port_persistent_info_t *) ((fbe_base_environment_t *)module_mgmt)->my_persistent_data;
    for(port_num = 0; port_num < FBE_ESP_IO_PORT_MAX_COUNT; port_num++)
    {
        /* This is not a 1 to 1 mapping, we have a lot of possible port data but only a certain number of ports that can be
         * configured.  This will make use of all persistent data slots when possible.
         */
        if( (module_mgmt->io_port_info[port_num].port_physical_info.port_comp_info[module_mgmt->local_sp].present) ||
            (module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info.logical_num != INVALID_PORT_U8))
        {
            fbe_copy_memory(&persistent_data_p[persistent_port_index],
                        &module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info,
                        sizeof(fbe_io_port_persistent_info_t));
            persistent_port_index++;
        }
    }
    status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)module_mgmt);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s persistent write error, status: 0x%X",
                              __FUNCTION__, status);
    }

    /*
     * Set the flag indicating this SP needs to be rebooted.
     */
    module_mgmt->reboot_sp = REBOOT_BOTH;

    status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_CHECK_REGISTRY);
    return status;

}

/*!***************************************************************
 * fbe_module_mgmt_config_mgmt_port_speed()
 ****************************************************************
 * @brief
 *  This function process CONTROL_CODE_SET_MGMT_PORT_SPEED.
 *  Initiate the MGMT port speed config.
 *
 * @param  module_mgmt_ptr - Ptr to module_mgmt object
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request
 *
 * @return fbe_status_t
 *
 * @author 
 *  25-Mar-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t
fbe_module_mgmt_config_mgmt_port_speed(fbe_module_mgmt_t *module_mgmt_ptr, 
                                       fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_esp_module_mgmt_set_mgmt_port_config_t   *mgmt_port_config_req = NULL;
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_device_physical_location_t pLocation;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mgmt_port_config_req);
    if (mgmt_port_config_req == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_set_mgmt_port_config_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, 0x%X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
    
    pLocation.sp = module_mgmt_ptr->local_sp;
    if (module_mgmt_ptr->mgmt_module_info) {
        pLocation.slot = module_mgmt_ptr->mgmt_module_info->mgmt_port_config_op.mgmtId;
    }
    
    /* Get the device string */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_MGMT_MODULE, 
                                               &pLocation, 
                                               &(deviceStr[0]), 
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to device string for mgmt module, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt_ptr, 
                  FBE_TRACE_LEVEL_INFO,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "%s, Recieve mgmt port config request. Auto %s, speed %s, duplex %s.\n",
                  &(deviceStr[0]), 
                  fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(mgmt_port_config_req->mgmtPortRequest.mgmtPortAutoNeg),
                  fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(mgmt_port_config_req->mgmtPortRequest.mgmtPortSpeed),
                  fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(mgmt_port_config_req->mgmtPortRequest.mgmtPortDuplex));


    /* Log the event to indicate the user initiated management port speed
     * config comamnd is received.*/
    fbe_event_log_write(ESP_INFO_MGMT_PORT_CONFIG_COMMAND_RECEIVED, 
                        NULL, 
                        0, 
                        "%s %s %s %s",
                        &(deviceStr[0]),
                        fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(mgmt_port_config_req->mgmtPortRequest.mgmtPortAutoNeg),
                        fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(mgmt_port_config_req->mgmtPortRequest.mgmtPortSpeed),
                        fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(mgmt_port_config_req->mgmtPortRequest.mgmtPortDuplex));

        /* Validate the port config parameters*/
    status = fbe_module_mgmt_validateMgmtPortConfigReq(module_mgmt_ptr,
                                                      mgmt_port_config_req);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validateMgmtPortConfigReq failed, status 0x%X.\n",
                              __FUNCTION__, status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                    (fbe_base_object_t*)module_mgmt_ptr,
                                    FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't set COND_SET_MGMT_PORT_SPEED, status 0x%X.\n",
                              __FUNCTION__, status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* Enqueue usurper packet */
    fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)module_mgmt_ptr, packet);
    return FBE_STATUS_PENDING;
}
/*****************************************************
 *  end of fbe_module_mgmt_config_mgmt_port_speed()
 ****************************************************/
/*!***************************************************************
 * fbe_module_mgmt_validateMgmtPortConfigReq()
 ****************************************************************
 * @brief
 *  This function validates the mgmt port config parameters.
 *  and populates the mgmt_port_config_op of module_mgmt object.
 *
 * @param  module_mgmt_ptr - Ptr to module_mgmt object
 * @param  mgmt_port_speed_reg - Ptr to mgmt port config request
 *
 * @return fbe_status_t
 *
 * @author 
 *  25-Mar-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t
fbe_module_mgmt_validateMgmtPortConfigReq(fbe_module_mgmt_t *module_mgmt_ptr,
                                       fbe_esp_module_mgmt_set_mgmt_port_config_t *mgmt_port_config_req)
{
    fbe_u32_t           mgmt_id;
    fbe_status_t        status;
    fbe_mgmt_module_info_t  *mgmt_module_info_p;
    fbe_bool_t          invalidRequest = FBE_FALSE;

    if((mgmt_port_config_req->mgmtPortRequest.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_UNSPECIFIED) &&
       (mgmt_port_config_req->mgmtPortRequest.mgmtPortSpeed == FBE_MGMT_PORT_SPEED_UNSPECIFIED) && 
       (mgmt_port_config_req->mgmtPortRequest.mgmtPortDuplex == FBE_PORT_DUPLEX_MODE_UNSPECIFIED))
    {
        invalidRequest = FBE_TRUE;
    }
    else if((mgmt_port_config_req->mgmtPortRequest.mgmtPortAutoNeg == FBE_PORT_AUTO_NEG_OFF) &&
            ((mgmt_port_config_req->mgmtPortRequest.mgmtPortSpeed == FBE_MGMT_PORT_SPEED_UNSPECIFIED) ||
             (mgmt_port_config_req->mgmtPortRequest.mgmtPortDuplex == FBE_PORT_DUPLEX_MODE_UNSPECIFIED))) 
    {
        invalidRequest = FBE_TRUE;
    }

    if(invalidRequest) 
    {
    
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,invalid request. \n", 
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    mgmt_id = mgmt_port_config_req->phys_location.slot;
    if(mgmt_id >= module_mgmt_ptr->discovered_hw_limit.num_mgmt_modules)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Invalid MGMT module ID %d count %d\n", 
                              __FUNCTION__, mgmt_id, module_mgmt_ptr->discovered_hw_limit.num_mgmt_modules);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    mgmt_module_info_p = (&(module_mgmt_ptr->mgmt_module_info[mgmt_id]));

    /* Check for revert request */
    mgmt_module_info_p->mgmt_port_config_op.revertEnabled = mgmt_port_config_req->revert;

    /* Save the user request so that we can return the user request when needed. */
    fbe_copy_memory(&mgmt_module_info_p->user_requested_mgmt_port_config,
                    &mgmt_port_config_req->mgmtPortRequest,
                    sizeof(fbe_mgmt_port_config_info_t));

    status = fbe_module_mgmt_convert_mgmt_port_config_request(module_mgmt_ptr,
                                                              mgmt_id, 
                                                              FBE_REQUEST_SOURCE_USER,
                                                              &(mgmt_port_config_req->mgmtPortRequest));
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s,convert_mgmt_port_config_request failed.\n", __FUNCTION__);

        fbe_module_mgmt_init_mgmt_port_config_op(&mgmt_module_info_p->mgmt_port_config_op);
        return FBE_STATUS_GENERIC_FAILURE;
        
    }  
    
    return FBE_STATUS_OK;
}
/*****************************************************
 *  end of fbe_module_mgmt_validateMgmtPortConfigReq()
 ****************************************************/
/*!***************************************************************
 * fbe_module_mgmt_get_mgmt_comp_info()
 ****************************************************************
 * @brief
 *  This function process CONTROL_CODE_GET_MGMT_COMP_INFO.
 *  return mgmt comp info
 *
 * @param  module_mgmt_ptr - Ptr to module_mgmt object
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request
 *
 * @return fbe_status_t
 *
 * @author 
 *  25-Mar-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_get_mgmt_comp_info(fbe_module_mgmt_t *module_mgmt_ptr, 
                                   fbe_packet_t * packet)
{
    SP_ID       sp;
    fbe_u32_t   mgmtID = 0;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_esp_module_mgmt_get_mgmt_comp_info_t   *mgmt_comp_info = NULL;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mgmt_comp_info);
    if (mgmt_comp_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s ,fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_get_mgmt_comp_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sp = mgmt_comp_info->phys_location.sp;
    if(sp >= SP_ID_MAX)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    mgmtID = mgmt_comp_info->phys_location.slot;
    if(mgmtID >= module_mgmt_ptr->discovered_hw_limit.num_mgmt_modules)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&(mgmt_comp_info->mgmt_module_comp_info),
                    &(module_mgmt_ptr->mgmt_module_info[mgmtID].mgmt_module_comp_info[sp]),
                    sizeof(fbe_board_mgmt_mgmt_module_info_t));
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*****************************************************
 *  end of fbe_module_mgmt_get_mgmt_comp_info()
 ****************************************************/
/*!***************************************************************
 * fbe_module_mgmt_get_requested_mgmt_port_config()
 ****************************************************************
 * @brief
 *  This function process CONTROL_CODE_GET_CONFIG_MGMT_INFO.
 *  return user requested mgmt port config.
 *
 * @param  module_mgmt_ptr - Ptr to module_mgmt object
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request
 *
 * @return fbe_status_t
 *
 * @author 
 *  15-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_get_requested_mgmt_port_config(fbe_module_mgmt_t *module_mgmt_ptr, 
                                      fbe_packet_t * packet)
{
    fbe_u32_t       mgmt_id;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_esp_module_mgmt_get_mgmt_port_config_t   *mgmt_port_config = NULL;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mgmt_port_config);
    if (mgmt_port_config == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s ,fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_get_mgmt_port_config_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    mgmt_id = mgmt_port_config->phys_location.slot;
    if(mgmt_id >= FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, Invalid Mgmt ID: %d \n", __FUNCTION__, mgmt_id);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
        
    }
    
    fbe_copy_memory(&mgmt_port_config->mgmtPortConfig, 
                    &module_mgmt_ptr->mgmt_module_info[mgmt_id].user_requested_mgmt_port_config,
                    sizeof(fbe_mgmt_port_config_info_t));
    

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*****************************************************
 *  end of fbe_module_mgmt_get_requested_mgmt_port_config()
 ****************************************************/

/*!***************************************************************
 * fbe_module_mgmt_markIoModule()
 ****************************************************************
 * @brief
 *  This function processes the MARK_IOM Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  03-Mar-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_markIoModule(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                 fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_esp_module_mgmt_mark_module_t       *mark_iom_cmd_ptr = NULL;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_set_iom_port_LED_t       portLedInfo;
    fbe_u32_t                               numberOfPorts, portIndex;
    fbe_u32_t                               iom_num;
    IO_CONTROLLER_PROTOCOL                  port_protocol;
    IO_CONTROLLER_PROTOCOL                  controller_protocol;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mark_iom_cmd_ptr);
    if (mark_iom_cmd_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_mark_module_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Send request to update IOM LED (mark all ports on IOM)
     */
    numberOfPorts = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt_ptr, 
                                                                 module_mgmt_ptr->local_sp,
                                                                 mark_iom_cmd_ptr->iomHeader.slot);
    if (mark_iom_cmd_ptr->markPortOn)
    {
        portLedInfo.iom_LED.blink_rate = LED_BLINK_1HZ;
    }
    else
    {
        portLedInfo.iom_LED.blink_rate = LED_BLINK_OFF;
    }
    portLedInfo.iom_LED.sp_id = mark_iom_cmd_ptr->iomHeader.sp;
    portLedInfo.iom_LED.slot = mark_iom_cmd_ptr->iomHeader.slot;
    portLedInfo.iom_LED.device_type = mark_iom_cmd_ptr->iomHeader.type;

    iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(mark_iom_cmd_ptr->iomHeader.type, mark_iom_cmd_ptr->iomHeader.slot);

    for (portIndex = 0; portIndex < numberOfPorts; portIndex++)
    {
        port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt_ptr, module_mgmt_ptr->local_sp, iom_num, portIndex);
        controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt_ptr,module_mgmt_ptr->local_sp,iom_num,portIndex);

        if(port_protocol == IO_CONTROLLER_PROTOCOL_ISCSI || port_protocol == IO_CONTROLLER_PROTOCOL_FCOE )
        {
            //For the CNA ports in iSCSI mode, mark LED blink in 1Hz green
            if(controller_protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
            {
                portLedInfo.led_color = LED_COLOR_TYPE_GREEN;
            }
            else
            {
                portLedInfo.led_color = LED_COLOR_TYPE_AMBER_GREEN_SYNC;
            }
        }
        else
        {
            portLedInfo.led_color = LED_COLOR_TYPE_BLUE;
        }

        portLedInfo.io_port = portIndex;

        fbe_base_object_trace((fbe_base_object_t *) module_mgmt_ptr,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, set IoModuleLed, Sp %d, mod %d, port %d, color %d, blink %d\n",
                               __FUNCTION__,
                               module_mgmt_ptr->local_sp,
                               iom_num,
                               portLedInfo.io_port,
                               portLedInfo.led_color,
                               portLedInfo.iom_LED.blink_rate);

        status = fbe_api_board_set_iom_port_LED(module_mgmt_ptr->board_object_id,
                                                &portLedInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt_ptr, 
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, IoModuleLed failed, Sp %d, mod %d, status 0x%x\n",
                                   __FUNCTION__, 
                                   portLedInfo.iom_LED.sp_id, 
                                   portLedInfo.iom_LED.slot,
                                   status);
            break;
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return status;

}   // end of fbe_module_mgmt_markIoModule


/*!***************************************************************
 * fbe_module_mgmt_markIoPort()
 ****************************************************************
 * @brief
 *  This function processes the MARK_IOPORT Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  03-Mar-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_markIoPort(fbe_module_mgmt_t *module_mgmt_ptr, 
                                                 fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_esp_module_mgmt_mark_port_t         *mark_ioport_cmd_ptr = NULL;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_board_mgmt_set_iom_port_LED_t       portLedInfo;
    fbe_u32_t                               iom_num;
    IO_CONTROLLER_PROTOCOL                  port_protocol;
    IO_CONTROLLER_PROTOCOL                  controller_protocol;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mark_ioport_cmd_ptr);
    if (mark_ioport_cmd_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_module_mgmt_mark_port_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Send request to update IO Port LED
     */
    if (mark_ioport_cmd_ptr->markPortOn)
    {
        portLedInfo.iom_LED.blink_rate = LED_BLINK_1HZ;
    }
    else
    {
        portLedInfo.iom_LED.blink_rate = LED_BLINK_OFF;
    }

    portLedInfo.iom_LED.device_type = mark_ioport_cmd_ptr->iomHeader.type;
    portLedInfo.iom_LED.slot = mark_ioport_cmd_ptr->iomHeader.slot;
    portLedInfo.iom_LED.sp_id = mark_ioport_cmd_ptr->iomHeader.sp;
    portLedInfo.io_port = mark_ioport_cmd_ptr->iomHeader.port;

    iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(mark_ioport_cmd_ptr->iomHeader.type, mark_ioport_cmd_ptr->iomHeader.slot);

    port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt_ptr, module_mgmt_ptr->local_sp, iom_num, portLedInfo.io_port);
    controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt_ptr, module_mgmt_ptr->local_sp, iom_num, portLedInfo.io_port);
    if(port_protocol == IO_CONTROLLER_PROTOCOL_ISCSI || port_protocol == IO_CONTROLLER_PROTOCOL_FCOE )
    {
        //For the CNA ports in iSCSI mode, mark LED blink in 1Hz green
        if(controller_protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
        {
            portLedInfo.led_color = LED_COLOR_TYPE_GREEN;
        }
        else
        {
            portLedInfo.led_color = LED_COLOR_TYPE_AMBER_GREEN_SYNC;
        }
    }
    else
    {
        portLedInfo.led_color = LED_COLOR_TYPE_BLUE;
    }

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt_ptr,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, set IoPortLed, Sp %d, mod %d, port %d, color %d, blink %d\n",
                           __FUNCTION__,
                           module_mgmt_ptr->local_sp,
                           iom_num,
                           portLedInfo.io_port,
                           portLedInfo.led_color,
                           portLedInfo.iom_LED.blink_rate);

    status = fbe_api_board_set_iom_port_LED(module_mgmt_ptr->board_object_id,
                                            &portLedInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt_ptr, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, IoPortLed failed, Sp %d, mod %d, port %d, status 0x%x\n",
                               __FUNCTION__, 
                               portLedInfo.iom_LED.sp_id, 
                               portLedInfo.iom_LED.slot,
                               portLedInfo.io_port,
                               status);
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return status;

}   // end of fbe_module_mgmt_markIoPort
/*!***************************************************************
 * fbe_module_mgmt_get_port_affinity()
 ****************************************************************
 * @brief
 *  This function processes the GET_PORT_AFFINITYS Control Code.
 *  class
 *
 * @param  module_mgmt_ptr - pointer to the module management object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  9-May-2012 - Created. Lin Lou
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_port_affinity(fbe_module_mgmt_t *module_mgmt_ptr,
                                       fbe_packet_t * packet)
{
    fbe_esp_module_mgmt_port_affinity_t     *port_affinity = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               iom_num;

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n",
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_affinity);
    if (port_affinity == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_module_mgmt_port_affinity_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(port_affinity->header.type, port_affinity->header.slot);
    port_affinity->pciData = fbe_module_mgmt_get_pci_info(module_mgmt_ptr, port_affinity->header.sp, iom_num, port_affinity->header.port);
    port_affinity->processor_core = fbe_module_mgmt_get_port_affinity_processor_core(module_mgmt_ptr, port_affinity->pciData);

    fbe_base_object_trace((fbe_base_object_t*)module_mgmt_ptr,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, configured port %d got interrupt affinity processor core %d\n",
                           __FUNCTION__, port_affinity->header.port, port_affinity->processor_core);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_module_mgmt_get_port_affinity()
 ******************************************/
