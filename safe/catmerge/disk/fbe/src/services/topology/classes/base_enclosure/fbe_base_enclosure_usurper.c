/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_base_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for the base enclosure class.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   11/12/2008:  Created file header. NC
 *
 ***************************************************************************/
#include "fbe_base_discovered.h"
#include "base_enclosure_private.h"
#include "fbe/fbe_eses.h"

/* Forward declaration */
static fbe_status_t base_enclosure_mgmt_get_enclosure_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_mgmt_get_port_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_attach_edge(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_detach_edge(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_enclosure_fault_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_scsi_error_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_edalLocale(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_slot_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_slot_count_per_bank(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_mgmt_basic_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet);
static fbe_status_t base_enclosure_get_component_count(fbe_base_enclosure_t *base_enclosure, 
                                                       fbe_packet_t *packet,
                                                       fbe_enclosure_component_types_t componentType);
static fbe_status_t base_enclosure_get_fan_count(fbe_base_enclosure_t *base_enclosure, 
                                                 fbe_packet_t *packet);
static fbe_status_t base_enclosure_get_ps_count(fbe_base_enclosure_t *base_enclosure, fbe_packet_t *packet);
static fbe_status_t base_enclosure_get_encl_type(fbe_base_enclosure_t *base_enclosure, 
                                                 fbe_packet_t *packet);
static fbe_status_t 
base_enclosure_get_connector_count(fbe_base_enclosure_t *base_enclosure, 
                             fbe_packet_t *packet);

static fbe_status_t 
base_enclosure_get_encl_side(fbe_base_enclosure_t *base_enclosure, 
                             fbe_packet_t *packet);
static fbe_status_t base_enclosure_get_disk_slot_present(fbe_base_enclosure_t *base_enclosure, 
                                                         fbe_packet_t *packet);
static fbe_status_t base_enclosure_get_max_drive_slot(fbe_base_enclosure_t *base_enclosure, 
                                                         fbe_packet_t *packet);

/*!**************************************************************
 * @fn fbe_base_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the base enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   10/13/2008:  Created file header. NC
 *
 ****************************************************************/
fbe_status_t 
fbe_base_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_base_enclosure_t * base_enclosure = NULL;
    fbe_trace_level_t trace_level;

    /*KvTrace("fbe_base_enclosure_main: %s entry\n", __FUNCTION__);*/
    base_enclosure = (fbe_base_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER:
            status = base_enclosure_get_slot_number( base_enclosure, packet);
            break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = base_enclosure_attach_edge( base_enclosure, packet);
            break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = base_enclosure_detach_edge( base_enclosure, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER:
            status = base_enclosure_mgmt_get_port_number(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER:
            status = base_enclosure_mgmt_get_enclosure_number(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_FAULT_INFO:
            status = base_enclosure_get_enclosure_fault_info(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SCSI_ERROR_INFO:
            status = base_enclosure_get_scsi_error_info(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO:
            status = base_enclosure_get_mgmt_basic_info(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_COMPONENT_ID:
            status = base_enclosure_get_edalLocale(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_COUNT:
            status = base_enclosure_get_ps_count(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_COUNT:
            status = base_enclosure_get_component_count(base_enclosure, packet, FBE_ENCL_LCC);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_COUNT:
            status = base_enclosure_get_fan_count(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCL_TYPE:
            status = base_enclosure_get_encl_type(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_COUNT:
            status = base_enclosure_get_max_drive_slot(base_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_COUNT_PER_BANK:
            status = base_enclosure_get_slot_count_per_bank(base_enclosure, packet);
            break;            
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DRIVE_PHY_INFO:
            /* This control code is traversable. 
             * It should have been handled if this was supported by the enclosure.
             * If it comes here, it means the enclosure does not support this code. 
             * we don't want to continue to traverse it.
             */      
            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s unsupported code CONTROL_CODE_GET_DRIVE_PHY_INFO.\n",
                                __FUNCTION__);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);            
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_COUNT:
            status = base_enclosure_get_connector_count(base_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCL_SIDE:
            status = base_enclosure_get_encl_side(base_enclosure, packet);
            break;
            
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_PRESENT:
            status = base_enclosure_get_disk_slot_present(base_enclosure, packet);
            break;

        default:
            status = fbe_base_discovering_control_entry(object_handle, packet);
            break;
    }

    // check if Enclosure trace level was updated & if so, update EDAL
    if (control_code == FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_LEVEL)
    {
        trace_level = fbe_base_object_get_trace_level((fbe_base_object_t *)base_enclosure);
        fbe_edal_setTraceLevel(trace_level);
    }

    return status;
}

/*!**************************************************************
 * @fn base_enclosure_get_slot_number(
 *               fbe_eses_enclosure_t * eses_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function retrieves slot number of a drive
 * related to a client id.
 *  The client_id is generated when creating the discovery edge
 * to the drive, which is the same as slot_number.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   10/13/2008:  Created file header. NC
 *
 ****************************************************************/
static fbe_status_t base_enclosure_get_slot_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_base_enclosure_mgmt_get_slot_number_t * base_enclosure_mgmt_get_slot_number = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_edge_index_t server_index;
    fbe_status_t status;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;  
    fbe_u8_t slot_num = FBE_ENCLOSURE_VALUE_INVALID;
    
    out_len = sizeof(fbe_base_enclosure_mgmt_get_slot_number_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&base_enclosure_mgmt_get_slot_number,out_len);
    if(status == FBE_STATUS_OK)
    {
        status = fbe_base_discovering_get_server_index_by_client_id((fbe_base_discovering_t *) base_enclosure,
                                                                    base_enclosure_mgmt_get_slot_number->client_id,
                                                                    &server_index);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s fbe_base_discovering_get_server_index_by_client_id fail, status: 0x%x.\n",
                                __FUNCTION__, status);        

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(server_index >= fbe_base_enclosure_get_number_of_slots(base_enclosure)) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader(base_enclosure),
                                    "get_slot_number, invalid slot %d.\n", 
                                    server_index);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* Translate server_index to slot_number */
        /* One to one translation */
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe(base_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,   // attribute
                                                    FBE_ENCL_DRIVE_SLOT,        // Component type
                                                    server_index,               // drive component index
                                                    &slot_num); 
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        base_enclosure_mgmt_get_slot_number->enclosure_slot_number = slot_num;
    } 
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}

/*!**************************************************************
 * @fn base_enclosure_mgmt_get_enclosure_number(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function returns enclosure number.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   11/12/2008:  Created file header. NC
 *
 ****************************************************************/
static fbe_status_t 
base_enclosure_mgmt_get_enclosure_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_enclosure_mgmt_get_enclosure_number_t * get_enclosure_number = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
   

    out_len = sizeof(fbe_base_enclosure_mgmt_get_enclosure_number_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&get_enclosure_number,out_len);
    if(status == FBE_STATUS_OK)
    {
        status = fbe_base_enclosure_get_enclosure_number(base_enclosure, &get_enclosure_number->enclosure_number);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************
 * @fn base_enclosure_mgmt_get_port_number(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function returns port number.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   01-Sept-2010: PHE -  Created file header.
 *
 ****************************************************************/
static fbe_status_t 
base_enclosure_mgmt_get_port_number(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_port_mgmt_get_port_number_t * get_port_number = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
   

    out_len = sizeof(fbe_base_port_mgmt_get_port_number_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&get_port_number,out_len);
    if(status == FBE_STATUS_OK)
    {
        status = fbe_base_enclosure_get_port_number(base_enclosure, &get_port_number->port_number);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************
 * @fn base_enclosure_attach_edge(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function attaches a discovery edge to an base enclosure object.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   10/13/2008:  Created file header. NC
 *   11/12/2008:  Move from ESES to Base. NC
 *
 ****************************************************************/
static fbe_status_t 
base_enclosure_attach_edge(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_discovery_transport_control_attach_edge_t * discovery_transport_control_attach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_edge_index_t server_index;
    fbe_bool_t power_save_supported = FALSE;
    fbe_status_t            status;
    
    len = sizeof(fbe_discovery_transport_control_attach_edge_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&discovery_transport_control_attach_edge,len);
    if(status == FBE_STATUS_OK)
    {
     
        /* Right now we do not know all the path state and path attributes rules,
           so we will do something straight forward and may be not exactly correct.
         */

        fbe_discovery_transport_get_server_index(discovery_transport_control_attach_edge->discovery_edge, &server_index);
        /* Edge validatein need to be done here */
        /* We also need to make sure the drive is inserted/logged in before we can accept this request.  use EDAL. */
        fbe_base_discovering_attach_edge((fbe_base_discovering_t *) base_enclosure, discovery_transport_control_attach_edge->discovery_edge);
        
        /* Init the discovery edge attribute to 0.*/
        fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) base_enclosure, server_index, 0 /* attr */);

        power_save_supported = fbe_base_enclosure_get_power_save_supported(base_enclosure);

        if(power_save_supported)
        {
            fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                server_index, 
                                                FBE_DISCOVERY_PATH_ATTR_POWERSAVE_SUPPORTED);
        }
      
        fbe_base_discovering_update_path_state((fbe_base_discovering_t *) base_enclosure);


        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s attach edge from slot %d.\n", 
                            __FUNCTION__, server_index);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************
 * @fn base_enclosure_detach_edge(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function detaches a discovery edge from a base enclosure object.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   10/13/2008:  Created file header. NC
 *   11/12/2008:  Move from ESES to Base. NC
 *   12/03/2009:  PHE - Retrieve the drive spin up permission.
 *
 ****************************************************************/
static fbe_status_t 
base_enclosure_detach_edge(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_edge_index_t server_index;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_u32_t curr_discovery_path_attrs = 0;

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader(base_enclosure),
                        "%s entry.\n", __FUNCTION__);

    len = sizeof(fbe_discovery_transport_control_detach_edge_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&discovery_transport_control_detach_edge,len);

    if(status == FBE_STATUS_OK)
    {
        fbe_discovery_transport_get_server_index(discovery_transport_control_detach_edge->discovery_edge, &server_index);

        if(server_index < fbe_base_enclosure_get_number_of_slots(base_enclosure))
        {
            /* The edge is to a drive. */
            status = fbe_base_discovering_get_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                server_index, 
                                                &curr_discovery_path_attrs);
            if((status == FBE_STATUS_OK) && 
                (curr_discovery_path_attrs & FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED))
            {
                /* Clear both attributes in case FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING
                * got set again due to some timing windown. 
                */
                fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *) base_enclosure, 
                                                server_index, 
                                                (FBE_DISCOVERY_PATH_ATTR_SPINUP_PENDING |
                                                FBE_DISCOVERY_PATH_ATTR_SPINUP_PERMITTED));  

                base_enclosure->number_of_drives_spinningup--;

                status = fbe_lifecycle_set_cond(&fbe_base_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)base_enclosure, 
                                        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_DRIVE_SPINUP_CTRL_NEEDED);

                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader(base_enclosure),
                                            "drive_spinup_permit, can't set DRIVE_SPINUP_CONTROL_NEEDED condition, status: 0x%x.\n",
                                            status);

                    fbe_transport_set_status(packet, status, 0);
                    fbe_transport_complete_packet(packet);
                    return status;
                }
            }
        }
        
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s detach edge from slot %d.\n", 
                            __FUNCTION__, server_index);

        status = fbe_base_discovering_detach_edge((fbe_base_discovering_t *) base_enclosure, 
                                    discovery_transport_control_detach_edge->discovery_edge);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************
 * @fn base_enclosure_get_enclosure_fault_info(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function processes FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_FAULT_INFO
 *  control code.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   04/28/2009:  Created.      Nayana Chaudhari
 *
 ****************************************************************/
static fbe_status_t 
base_enclosure_get_enclosure_fault_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_fault_t * get_enclosure_fault_info = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
  
    
    out_len = sizeof(fbe_enclosure_fault_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&get_enclosure_fault_info,out_len);
    if(status == FBE_STATUS_OK)
    {
        encl_stat = fbe_base_enclosure_get_enclosure_fault_info(base_enclosure, get_enclosure_fault_info);
    }

    fbe_base_enclosure_set_packet_payload_status(packet, encl_stat);

    fbe_transport_complete_packet(packet);
    return status;
}

/*!**************************************************************
 * @fn base_enclosure_get_scsi_error_info(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function processes FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SCSI_ERROR_INFO
 *  control code.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   04/28/2009:  Created.      Nayana Chaudhari
 *
 ****************************************************************/
static fbe_status_t 
base_enclosure_get_scsi_error_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_scsi_error_info_t * get_scsi_error_info = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
  
    
    out_len = sizeof(fbe_enclosure_scsi_error_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t *)&get_scsi_error_info,out_len);
    if(status == FBE_STATUS_OK)
    {
        // we don't really care for encl_stat here.
        encl_stat = fbe_base_enclosure_get_scsi_error_info(base_enclosure, get_scsi_error_info);
    }

    fbe_base_enclosure_set_packet_payload_status(packet, encl_stat);

    fbe_transport_complete_packet(packet);
    return status;
}



/*!****************************************************************************
 * @fn base_enclosure_get_mgmt_basic_info(
 *               fbe_base_enclosure_t * base_enclosure, 
 *               fbe_packet_t * packet)
 *
 * @brief
 * This function processes the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO
 * control code.  It fills in the default info having assumed that this enclosure 
 * is faulted.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY
 *  11/17/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
static fbe_status_t 
base_enclosure_get_mgmt_basic_info(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_OK;
    fbe_base_object_mgmt_get_enclosure_basic_info_t * get_mgmt_basic_info = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
  
    
    out_len = sizeof(fbe_base_object_mgmt_get_enclosure_basic_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t*)&get_mgmt_basic_info,out_len);
    if(status == FBE_STATUS_OK)
    {
        // we don't really care for encl_stat here.
        encl_stat = fbe_base_enclosure_get_mgmt_basic_info(base_enclosure, get_mgmt_basic_info);
    }

    fbe_base_enclosure_set_packet_payload_status(packet, encl_stat);

    fbe_transport_complete_packet(packet);
    return status;
}




/*!****************************************************************************
 * @fn      base_enclosure_get_edalLocale()
 *
 * @brief
 * This function extracts the edalLocale value from the base enclosure
 * and returns it to the caller by way of the packet control data memory.
 *
 * @param base_enclosure - Pointer to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return
 *  None
 *
 * HISTORY
 *  05/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
static fbe_status_t 
base_enclosure_get_edalLocale(fbe_base_enclosure_t * base_enclosure, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_OK;
    fbe_base_enclosure_mgmt_get_component_id_t * getComponentIdPtr = NULL;
    fbe_u8_t                            edalLocale = 0; 
    fbe_payload_control_buffer_length_t out_len = 0;

    out_len = sizeof(fbe_base_enclosure_mgmt_get_component_id_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,base_enclosure,TRUE,(fbe_payload_control_buffer_t*)&getComponentIdPtr,out_len);
    if(status == FBE_STATUS_OK)
    {
        encl_stat = fbe_base_enclosure_get_edal_locale(base_enclosure, &edalLocale);

        if(encl_stat == FBE_ENCLOSURE_STATUS_OK) 
        {
            getComponentIdPtr->component_id = edalLocale;
        }
        else
        {
            //Failed to get edalLocale.
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_base_enclosure_set_packet_payload_status(packet, encl_stat);
    }
    else
    {
        fbe_transport_set_status(packet, status, 0);
    }

    fbe_transport_complete_packet(packet);
    return status;
}


/*!*************************************************************************
* @fn base_enclosure_get_component_count()                  
***************************************************************************
* @brief
*       This function gets the count for the specified component type in the enclosure.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   26-Aug-2010     PHE - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_get_component_count(fbe_base_enclosure_t *base_enclosure, 
                            fbe_packet_t *packet,
                            fbe_enclosure_component_types_t componentType)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u32_t * pComponentCount = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pComponentCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n",
								__FUNCTION__,
                                control_buffer_length, 
                                (int)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    edalStatus = 
        fbe_edal_getSpecificComponentCount(base_enclosure->enclosureComponentTypeBlock,
                                           componentType,
                                           pComponentCount);

    if(!EDAL_STAT_OK(edalStatus))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - base_enclosure_get_component_count

/*!*************************************************************************
* @fn base_enclosure_get_slot_count_per_bank()                  
***************************************************************************
* @brief
*       This function gets the slot count per bank for enclosure.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   2-May-2012     Eric Zhou - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_get_slot_count_per_bank(fbe_base_enclosure_t *base_enclosure, 
                            fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u32_t * pSlotCountPerBank = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pSlotCountPerBank = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (int)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    *pSlotCountPerBank = base_enclosure->number_of_slots_per_bank;

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - base_enclosure_get_slot_count_per_bank


/*!*************************************************************************
* @fn base_enclosure_get_fan_count()                  
***************************************************************************
* @brief
*       This function gets the total fan count in the enclosure, including the ones
*        in the enclosure Power Supplies.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   15-Dec-2010     Rajesh V. - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_get_fan_count(fbe_base_enclosure_t *base_enclosure, 
                             fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u8_t    coolingComponentCount;
    fbe_u32_t   coolingIndex = 0;
    fbe_u8_t    containerIndex = 0;
    fbe_u8_t    sideId = 0;
    fbe_u8_t    subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u32_t * pFanCount = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    // Get the pointer to the fan count, that should be filled in.
    pFanCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    
    /* We only return the standalone fan count. 
     */ 
    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_STANDALONE;

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components(base_enclosure,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Caluclate the Fan count by the number of the containing subenclosures.
     * (Do not include the chassis)
     */
    for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)
    {
        /* Get subtype. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe(base_enclosure,
                                                               FBE_ENCL_COMP_SUBTYPE,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &subtype);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // Get the attribute which indicates if it is individual or overall element.
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe(base_enclosure,
                                                               FBE_ENCL_COMP_CONTAINER_INDEX,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &containerIndex);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get sideId */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe(base_enclosure,
                                                               FBE_ENCL_COMP_SIDE_ID,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &sideId);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if((subtype == targetSubtype) &&
           (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED) &&
           (sideId != FBE_ESES_SUBENCL_SIDE_ID_MIDPLANE))
        {
            (*pFanCount)++;
        }
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - base_enclosure_get_fan_count

/*!*************************************************************************
* @fn base_enclosure_get_ps_count()                  
***************************************************************************
* @brief
*   This functions gets the total ps count and adjusts it by only counting
*   those that have a valid side id. This will represent the fru level ps
*   container.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   1-Sep-2011     Rajesh V. - Created.
***************************************************************************/
fbe_status_t base_enclosure_get_ps_count(fbe_base_enclosure_t *base_enclosure, fbe_packet_t *packet)
{
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;   
    fbe_payload_control_buffer_t        control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u32_t                           *pPsCount = NULL;
    fbe_u32_t                           psIndex;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                            containerIndex = FBE_ENCLOSURE_CONTAINER_INDEX_INVALID;
    fbe_u8_t                            number_of_components = 0;

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__);
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pPsCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, fbe_payload_control_get_buffer_length failed.\n", 
                                __FUNCTION__);

        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)base_enclosure),
                                "get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_base_enclosure_get_number_of_components(base_enclosure,
                                                         FBE_ENCL_POWER_SUPPLY,
                                                         &number_of_components); 
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pPsCount = 0;

    // loop to count power supplies, only count the containers.
    for (psIndex = 0; psIndex < number_of_components; psIndex++)
    {
        // get the container index
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe( base_enclosure,
                                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                                FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                                psIndex,                // Component index.
                                                                &containerIndex);               // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }


        if(containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED)
        {
            (*pPsCount)++;
        }
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)base_enclosure),
                                       "%s numComponents:%d psCount:%d\n", 
                                       __FUNCTION__,
                                       number_of_components,
                                       *pPsCount);

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn base_enclosure_get_encl_type()                  
***************************************************************************
* @brief
*       This function gets the enclosure type.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   17-Jan-2011: PHE - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_get_encl_type(fbe_base_enclosure_t *base_enclosure, 
                             fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_types_t * pEnclType = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    
    /********
     * BEGIN
     ********/
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pEnclType);
    if( (pEnclType == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, pEnclType: %64p.\n",
                               __FUNCTION__, status, pEnclType);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_types_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_enclosure_types_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    enclStatus = fbe_base_enclosure_edal_getEnclosureType(base_enclosure, pEnclType);

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, enclStatus);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - base_enclosure_get_encl_type

/*!*************************************************************************
* @fn base_enclosure_get_connector_count()                  
***************************************************************************
* @brief
*       This function gets the total connector count in the enclosure, including
*       both SPA and SPB side. Note this count is for the entire connectors only.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   24-Aug-2011 PHE - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_get_connector_count(fbe_base_enclosure_t *base_enclosure, 
                             fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u8_t    componentCount;
    fbe_u32_t   index = 0;
    fbe_u32_t * pConnectorCount = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t  isLocalConnector = FBE_FALSE;
    fbe_bool_t  isEntireConnector = FBE_FALSE;
    fbe_u32_t   localCount = 0;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    // Get the pointer to the Connecotr count, that should be filled in.
    pConnectorCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    
    /* Get the total number of connector in EDAL so that we can loop throught it */
    status = fbe_base_enclosure_get_number_of_components(base_enclosure,
                                                             FBE_ENCL_CONNECTOR,
                                                             &componentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pConnectorCount = 0;

    for (index = 0; index < componentCount; index ++)
    {
        /* 
         * Check whether this is local connector. For the peer connector, 
         * FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR is not populated. 
         */
          enclStatus = fbe_base_enclosure_edal_getBool_thread_safe(base_enclosure,
                                                               FBE_ENCL_COMP_IS_LOCAL,
                                                               FBE_ENCL_CONNECTOR,
                                                               index,
                                                               &isLocalConnector);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(isLocalConnector) 
        {
            /* check whether this is entire connector.
             * We don't need to count the connector which represents the individaul lane within a connector.
             */
            enclStatus = fbe_base_enclosure_edal_getBool_thread_safe(base_enclosure,
                                                                   FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,
                                                                   FBE_ENCL_CONNECTOR,
                                                                   index,
                                                                   &isEntireConnector);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
    
            if(isEntireConnector) 
            {
                (localCount)++;
            }
        }
    }

    /* Count both local count and peer count. */
    *pConnectorCount = 2*localCount;

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - base_enclosure_get_connector_count

/*!*************************************************************************
* @fn base_enclosure_get_encl_side()                  
***************************************************************************
* @brief
*       This function gets the enclosure side(side A or B)
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   13-Dec-2011: PHE - Created.
***************************************************************************/
static fbe_status_t 
base_enclosure_get_encl_side(fbe_base_enclosure_t *base_enclosure, 
                             fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u32_t * pEnclSide = NULL;
    fbe_u8_t enclSideU8 = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    
    /********
     * BEGIN
     ********/
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pEnclSide);
    if( (pEnclSide == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x, pEnclSide: %64p.\n",
                               __FUNCTION__, status, pEnclSide);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    enclStatus = fbe_base_enclosure_edal_getEnclosureSide(base_enclosure, &enclSideU8);

    *pEnclSide = enclSideU8;

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, enclStatus);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - base_enclosure_get_encl_type

/*!*************************************************************************
* @fn base_enclosure_get_disk_slot_present()                  
***************************************************************************
* @brief
*   Check if the disk slot is owned by the base enclosure obj.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   28-Aug-2012: GB - Created.
***************************************************************************/
static fbe_status_t base_enclosure_get_disk_slot_present(fbe_base_enclosure_t *base_enclosure, 
                                                         fbe_packet_t *packet)
{
    fbe_payload_ex_t                                    * payload = NULL;
    fbe_payload_control_operation_t                     * control_operation = NULL;   
    fbe_payload_control_buffer_length_t                 control_buffer_length = 0;
    fbe_u32_t                                           index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_enclosure_mgmt_disk_slot_present_cmd_t          *pSlotPresent = NULL;
    fbe_enclosure_status_t                              enclStatus;
    fbe_status_t                                        status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pSlotPresent);
    if( (pSlotPresent == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x\n",
                               __FUNCTION__, status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_disk_slot_present_cmd_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader(base_enclosure),
                                           "%s, ActLen:%d, ExpLen:%d, status: 0x%x.\n", 
                                           __FUNCTION__,
                                           (int)control_buffer_length, 
                                           (int)sizeof(fbe_enclosure_mgmt_disk_slot_present_cmd_t), 
                                           status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    
    if(((fbe_base_object_t *)base_enclosure)->object_id != FBE_OBJECT_ID_INVALID)
    {
        // if the drive slot belongs to this enclosure/lcc obj, a valid index will be returned
        index =  fbe_base_enclosure_find_first_edal_match_U8(base_enclosure,
                                                             FBE_ENCL_DRIVE_SLOT_NUMBER,  // attribute
                                                             FBE_ENCL_DRIVE_SLOT,         // component type
                                                             0,                           // starting index
                                                             pSlotPresent->targetSlot);   // drive slot to match
    }
    if(index == FBE_ENCLOSURE_VALUE_INVALID)
    {
        pSlotPresent->slotFound = FALSE;
    }
    else
    {
        pSlotPresent->slotFound = TRUE;
    }
    enclStatus = FBE_ENCLOSURE_STATUS_OK;
    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, enclStatus);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} //base_enclosure_get_disk_slot_present
/*!*************************************************************************
* @fn base_enclosure_get_max_drive_slot()                  
***************************************************************************
* @brief
*   Get max drive slot number by the base enclosure obj.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   10-Mar-2013: Chengkai - Created.
***************************************************************************/
static fbe_status_t base_enclosure_get_max_drive_slot(fbe_base_enclosure_t *base_enclosure, 
                                                         fbe_packet_t *packet)
{
    fbe_payload_ex_t                                    * payload = NULL;
    fbe_payload_control_operation_t                     * control_operation = NULL;   
    fbe_payload_control_buffer_length_t                 control_buffer_length = 0;
    fbe_u32_t                                           *pEnclMaxSlot;
    fbe_enclosure_status_t                              enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t                                        status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader(base_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pEnclMaxSlot);
    if( (pEnclMaxSlot == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader(base_enclosure),
                               "%s, get_buffer failed, status: 0x%x\n",
                               __FUNCTION__, status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader(base_enclosure),
                                           "%s, ActLen:%d, ExpLen:%d, status: 0x%x.\n", 
                                           __FUNCTION__,
                                           (int)control_buffer_length, 
                                           (int)sizeof(fbe_u32_t), 
                                           status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    
    if(((fbe_base_object_t *)base_enclosure)->object_id != FBE_OBJECT_ID_INVALID)
    {
        enclStatus = fbe_base_enclosure_edal_getU8(base_enclosure,
                                                FBE_ENCL_MAX_SLOTS,
                                                FBE_ENCL_ENCLOSURE,
                                                0,
                                                (fbe_u8_t *)pEnclMaxSlot);
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, enclStatus);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} //base_enclosure_get_max_drive_slot

/* End of file fbe_base_enclosure_usurper.c */
