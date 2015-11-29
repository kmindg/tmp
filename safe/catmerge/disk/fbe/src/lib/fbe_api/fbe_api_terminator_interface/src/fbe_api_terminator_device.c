
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_terminator_device.c
 ***************************************************************************
 *
 *  Description
 *      Terminator device related APIs 
 *
 *  History:
 *      11/03/09    gaob3 - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_terminator_service.h"
/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/*********************************************************************
 *            fbe_api_terminator_find_device_class ()
 *********************************************************************
 *
 *  Description: find the class of the specified device in Terminator
 *
 *  Inputs: device_class_name - name of the device class
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_find_device_class(const char * device_class_name, 
                                                fbe_api_terminator_device_class_handle_t *device_class_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_find_device_class_ioctl_t find_device_class_ioctl;

    fbe_zero_memory(find_device_class_ioctl.device_class_name, sizeof(find_device_class_ioctl.device_class_name));
    strncpy(find_device_class_ioctl.device_class_name, device_class_name, sizeof(find_device_class_ioctl.device_class_name) - 1);
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_FIND_DEVICE_CLASS,
                                                            &find_device_class_ioctl,
                                                            sizeof (fbe_terminator_find_device_class_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *device_class_handle = find_device_class_ioctl.device_class_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_create_device_class_instance ()
 *********************************************************************
 *
 *  Description: create an instance of the specified device class
 *
 *  Inputs: device_class_handle - handle of the device class
 *             device_type - device type
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_create_device_class_instance(fbe_api_terminator_device_class_handle_t device_class_handle,
                                                 const char * device_type,
                                                 fbe_api_terminator_device_handle_t * device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_create_device_class_instance_ioctl_t create_device_class_instance_ioctl;

    create_device_class_instance_ioctl.device_class_handle = device_class_handle;
    /* device_type is NULL means default type */
    fbe_zero_memory(create_device_class_instance_ioctl.device_type, sizeof(create_device_class_instance_ioctl.device_type));
    strncpy(create_device_class_instance_ioctl.device_type, NULL == device_type ? "" : device_type, sizeof(create_device_class_instance_ioctl.device_type) - 1);
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_CREATE_DEVICE_CLASS_INSTANCE,
                                                            &create_device_class_instance_ioctl,
                                                            sizeof (fbe_terminator_create_device_class_instance_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *device_handle = create_device_class_instance_ioctl.device_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_device_attribute ()
 *********************************************************************
 *
 *  Description: set device attribute
 *
 *  Inputs: device_handle - handle of the device
 *            attribute_name - attribute name
 *            attribute_value - attribute value
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_device_attribute(fbe_api_terminator_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_device_attribute_ioctl_t set_device_attribute_ioctl;

    set_device_attribute_ioctl.device_handle = device_handle;
    fbe_zero_memory(set_device_attribute_ioctl.attribute_name, sizeof(set_device_attribute_ioctl.attribute_name));
    strncpy(set_device_attribute_ioctl.attribute_name, attribute_name, sizeof(set_device_attribute_ioctl.attribute_name) - 1);
    fbe_zero_memory(set_device_attribute_ioctl.attribute_value, sizeof(set_device_attribute_ioctl.attribute_value));
    strncpy(set_device_attribute_ioctl.attribute_value, attribute_value, sizeof(set_device_attribute_ioctl.attribute_value) - 1);
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_DEVICE_ATTRIBUTE,
                                                            &set_device_attribute_ioctl,
                                                            sizeof (fbe_terminator_set_device_attribute_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_insert_device ()
 *********************************************************************
 *
 *  Description: insert a device
 *
 *  Inputs: parent_device - the parent device
 *            child_device - the child device
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_device(fbe_api_terminator_device_handle_t parent_device,
                                              fbe_api_terminator_device_handle_t child_device)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_device_ioctl_t insert_device_ioctl;

    insert_device_ioctl.parent_device = parent_device;
    insert_device_ioctl.child_device = child_device;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_DEVICE,
                                                            &insert_device_ioctl,
                                                            sizeof (fbe_terminator_insert_device_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_activate_device ()
 *********************************************************************
 *
 *  Description: activate the device
 *
 *  Inputs: device_handle - handle of the device to be activated
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_activate_device(fbe_api_terminator_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_activate_device_ioctl_t activate_device_ioctl;

    activate_device_ioctl.device_handle = device_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ACTIVATE_DEVICE,
                                                            &activate_device_ioctl,
                                                            sizeof (fbe_terminator_activate_device_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_sas_enclosure_drive_slot_eses_status ()
 *********************************************************************
 *
 *  Description: set sas enclosure drive slot eses status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             slot_number - number of the slot
 *             drive_slot_stat - state of the drive slot
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_sas_enclosure_drive_slot_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                                         fbe_u32_t slot_number,
                                                                         ses_stat_elem_array_dev_slot_struct drive_slot_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl_t set_sas_enclosure_drive_slot_eses_status_ioctl;

    set_sas_enclosure_drive_slot_eses_status_ioctl.enclosure_handle = enclosure_handle;
    set_sas_enclosure_drive_slot_eses_status_ioctl.slot_number = slot_number;
    set_sas_enclosure_drive_slot_eses_status_ioctl.drive_slot_stat = drive_slot_stat;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SAS_ENCLOSURE_DRIVE_SLOT_ESES_STATUS,
                                                            &set_sas_enclosure_drive_slot_eses_status_ioctl,
                                                            sizeof (fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_force_login_device ()
 *********************************************************************
 *
 *  Description: force the device to login
 *
 *  Inputs: device_handle - handle of the device to login
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_force_login_device (fbe_api_terminator_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_force_login_device_ioctl_t force_login_device_ioctl;

    force_login_device_ioctl.device_handle = device_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_FORCE_LOGIN_DEVICE,
                                                            &force_login_device_ioctl,
                                                            sizeof (fbe_terminator_force_login_device_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_force_logout_device ()
 *********************************************************************
 *
 *  Description: force the device to logout
 *
 *  Inputs: device_handle - handle of the device to logout
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_force_logout_device (fbe_api_terminator_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_force_logout_device_ioctl_t force_logout_device_ioctl;

    force_logout_device_ioctl.device_handle = device_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_FORCE_LOGOUT_DEVICE,
                                                            &force_logout_device_ioctl,
                                                            sizeof (fbe_terminator_force_logout_device_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_remove_device ()
 *********************************************************************
 *
 *  Description: remove the specified device
 *
 *  Inputs: device_handle - handle of the device to be removed
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_remove_device (fbe_api_terminator_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_remove_device_ioctl_t remove_device_ioctl;

    remove_device_ioctl.device_handle = device_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_REMOVE_DEVICE,
                                                            &remove_device_ioctl,
                                                            sizeof (fbe_terminator_remove_device_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_device_attribute ()
 *********************************************************************
 *
 *  Description: get the specified attribute of the device
 *
 *  Inputs: device_handle - handle of the device
 *             attribute_name - attribute name
 *             attribute_value_buffer - attribute value buffer
 *             buffer_length - buffer ength
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_device_attribute(fbe_api_terminator_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value_buffer,
                                                    fbe_u32_t buffer_length)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_device_attribute_ioctl_t get_device_attribute_ioctl;

    get_device_attribute_ioctl.device_handle = device_handle;
    fbe_zero_memory(get_device_attribute_ioctl.attribute_name, sizeof(get_device_attribute_ioctl.attribute_name));
    strncpy(get_device_attribute_ioctl.attribute_name, attribute_name, sizeof(get_device_attribute_ioctl.attribute_name) - 1) ;
    get_device_attribute_ioctl.buffer_length = buffer_length;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DEVICE_ATTRIBUTE,
                                                            &get_device_attribute_ioctl,
                                                            sizeof (fbe_terminator_get_device_attribute_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(attribute_value_buffer, sizeof(get_device_attribute_ioctl.attribute_value_buffer));
    strncpy(attribute_value_buffer, get_device_attribute_ioctl.attribute_value_buffer, sizeof(get_device_attribute_ioctl.attribute_value_buffer) - 1);
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_temp_sensor_eses_status ()
 *********************************************************************
 *
 *  Description: get the temperature status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             temp_sensor_id - id of the temperature sensor
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_temp_sensor_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_temp_sensor_eses_status_ioctl_t get_temp_sensor_eses_status_ioctl;

    get_temp_sensor_eses_status_ioctl.enclosure_handle = enclosure_handle;
    get_temp_sensor_eses_status_ioctl.temp_sensor_id = temp_sensor_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_TEMP_SENSOR_ESES_STATUS,
                                                            &get_temp_sensor_eses_status_ioctl,
                                                            sizeof (fbe_terminator_get_temp_sensor_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *temp_sensor_stat = get_temp_sensor_eses_status_ioctl.temp_sensor_stat;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_temp_sensor_eses_status ()
 *********************************************************************
 *
 *  Description: set the temperature status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *            temp_sensor_id - id of the temperature sensor
 *            temp_sensor_stat - temperature sensor status
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_temp_sensor_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_temp_sensor_eses_status_ioctl_t set_temp_sensor_eses_status_ioctl;

    set_temp_sensor_eses_status_ioctl.enclosure_handle = enclosure_handle;
    set_temp_sensor_eses_status_ioctl.temp_sensor_id = temp_sensor_id;
    set_temp_sensor_eses_status_ioctl.temp_sensor_stat = temp_sensor_stat;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_TEMP_SENSOR_ESES_STATUS,
                                                            &set_temp_sensor_eses_status_ioctl,
                                                            sizeof (fbe_terminator_set_temp_sensor_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_cooling_eses_status ()
 *********************************************************************
 *
 *  Description: get the cooling status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             cooling_id - id of the cooling 
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_cooling_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct *cooling_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_cooling_eses_status_ioctl_t get_cooling_eses_status_ioctl;

    get_cooling_eses_status_ioctl.enclosure_handle = enclosure_handle;
    get_cooling_eses_status_ioctl.cooling_id = cooling_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_COOLING_ESES_STATUS,
                                                            &get_cooling_eses_status_ioctl,
                                                            sizeof (fbe_terminator_get_cooling_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *cooling_stat = get_cooling_eses_status_ioctl.cooling_stat;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_cooling_eses_status ()
 *********************************************************************
 *
 *  Description: set the cooling status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             cooling_id - id of the cooling 
 *             cooling_stat - cooling status
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_cooling_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct cooling_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_cooling_eses_status_ioctl_t set_cooling_eses_status_ioctl;

    set_cooling_eses_status_ioctl.enclosure_handle = enclosure_handle;
    set_cooling_eses_status_ioctl.cooling_id = cooling_id;
    set_cooling_eses_status_ioctl.cooling_stat = cooling_stat;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_COOLING_ESES_STATUS,
                                                            &set_cooling_eses_status_ioctl,
                                                            sizeof (fbe_terminator_set_cooling_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_ps_eses_status ()
 *********************************************************************
 *
 *  Description: get the ps status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             ps_id - id of the ps
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_ps_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct *ps_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_ps_eses_status_ioctl_t get_ps_eses_status_ioctl;

    get_ps_eses_status_ioctl.enclosure_handle = enclosure_handle;
    get_ps_eses_status_ioctl.ps_id = ps_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_PS_ESES_STATUS,
                                                            &get_ps_eses_status_ioctl,
                                                            sizeof (fbe_terminator_get_ps_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *ps_stat = get_ps_eses_status_ioctl.ps_stat;
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_terminator_set_ps_eses_status ()
 *********************************************************************
 *
 *  Description: set the ps status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             ps_id - id of the ps
 *             ps_stat - ps status
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_ps_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct ps_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_ps_eses_status_ioctl_t set_ps_eses_status_ioctl;

    set_ps_eses_status_ioctl.enclosure_handle = enclosure_handle;
    set_ps_eses_status_ioctl.ps_id = ps_id;
    set_ps_eses_status_ioctl.ps_stat = ps_stat;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_PS_ESES_STATUS,
                                                            &set_ps_eses_status_ioctl,
                                                            sizeof (fbe_terminator_set_ps_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_lcc_eses_status ()
 *********************************************************************
 *
 *  Description: get the lcc status
 *
 *  Inputs: enclosure_handle
 *             lcc_stat - struct for returned lcc status
 *
 *  Return Value: success or failue
 *
 *  History:
 *    09/02/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_lcc_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   ses_stat_elem_encl_struct *lcc_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_lcc_eses_status_ioctl_t get_lcc_eses_status_ioctl;

    get_lcc_eses_status_ioctl.enclosure_handle = enclosure_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_LCC_ESES_STATUS,
                                                            &get_lcc_eses_status_ioctl,
                                                            sizeof (fbe_terminator_get_lcc_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *lcc_stat = get_lcc_eses_status_ioctl.lcc_stat;
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_terminator_set_lcc_eses_status ()
 *********************************************************************
 *
 *  Description: set the lcc status
 *
 *  Inputs: enclosure_handle
 *             lcc_stat - lcc status struct
 *
 *  Return Value: success or failue
 *
 *  History:
 *    09/02/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_lcc_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   ses_stat_elem_encl_struct lcc_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_lcc_eses_status_ioctl_t set_lcc_eses_status_ioctl;

    set_lcc_eses_status_ioctl.enclosure_handle = enclosure_handle;
    set_lcc_eses_status_ioctl.lcc_stat = lcc_stat;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_LCC_ESES_STATUS,
                                                            &set_lcc_eses_status_ioctl,
                                                            sizeof (fbe_terminator_set_lcc_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_sas_conn_eses_status ()
 *********************************************************************
 *
 *  Description: get the connector status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             ps_id - id of the ps
 *
 *  Return Value: success or failue
 *
 *  History:
 *    24-Aug-2011: PHE  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_sas_conn_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct *sas_conn_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_sas_conn_eses_status_ioctl_t get_sas_conn_eses_status_ioctl;

    get_sas_conn_eses_status_ioctl.enclosure_handle = enclosure_handle;
    get_sas_conn_eses_status_ioctl.sas_conn_id = sas_conn_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_SAS_CONN_ESES_STATUS,
                                                            &get_sas_conn_eses_status_ioctl,
                                                            sizeof (fbe_terminator_get_sas_conn_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *sas_conn_stat = get_sas_conn_eses_status_ioctl.sas_conn_stat;
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_terminator_set_sas_conn_eses_status ()
 *********************************************************************
 *
 *  Description: set the sas connector status
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *             ps_id - id of the ps
 *             ps_stat - ps status
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_sas_conn_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct sas_conn_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_sas_conn_eses_status_ioctl_t set_sas_conn_eses_status_ioctl;

    set_sas_conn_eses_status_ioctl.enclosure_handle = enclosure_handle;
    set_sas_conn_eses_status_ioctl.sas_conn_id = sas_conn_id;
    set_sas_conn_eses_status_ioctl.sas_conn_stat = sas_conn_stat;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SAS_CONN_ESES_STATUS,
                                                            &set_sas_conn_eses_status_ioctl,
                                                            sizeof (fbe_terminator_set_sas_conn_eses_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_resume_prom_info ()
 *********************************************************************
 *
 *  Description: set information of resume prom
 *
 *  Inputs: enclosure_handle - handle of the enclosure
 *            resume_prom_id - id of resume prom
 *            buf - buffer
 *            len - length of the buffer
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/06/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_resume_prom_info(fbe_api_terminator_device_handle_t enclosure_handle,
                                                     terminator_eses_resume_prom_id_t resume_prom_id,
                                                     fbe_u8_t *buf,
                                                     fbe_u32_t len)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_resume_prom_info_ioctl_t set_resume_prom_info_ioctl;

    set_resume_prom_info_ioctl.enclosure_handle = enclosure_handle;
    set_resume_prom_info_ioctl.resume_prom_id = resume_prom_id;
    memcpy(set_resume_prom_info_ioctl.buf, buf, len);
    set_resume_prom_info_ioctl.len = len;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_RESUME_PROM_INFO,
                                                            &set_resume_prom_info_ioctl,
                                                            sizeof (fbe_terminator_set_resume_prom_info_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_io_mode ()
 *********************************************************************
 *
 *  Description: set io mode
 *
 *  Inputs: io_mode - io mode
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/12/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_io_mode(fbe_terminator_io_mode_t io_mode)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_io_mode_ioctl_t set_io_mode_ioctl;

    set_io_mode_ioctl.io_mode = io_mode;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_IO_MODE,
                                                            &set_io_mode_ioctl,
                                                            sizeof (fbe_terminator_set_io_mode_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_io_global_completion_delay ()
 *********************************************************************
 *
 *  Description: set io global_completion_delay
 *
 *  Inputs: global_completion_delay - io global_completion_delay in ms.
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/22/10: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_io_global_completion_delay(fbe_u32_t global_completion_delay)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_io_global_completion_delay_ioctl_t set_io_global_completion_deplay_ioctl;

    set_io_global_completion_deplay_ioctl.global_completion_delay = global_completion_delay;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_IO_GLOBAL_COMPLETION_DELAY,
                                                            &set_io_global_completion_deplay_ioctl,
                                                            sizeof (fbe_terminator_set_io_global_completion_delay_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_terminator_mark_eses_page_unsupported ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/20/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_mark_eses_page_unsupported_ioctl_t mark_eses_page_unsupported_ioctl;

    mark_eses_page_unsupported_ioctl.cdb_opcode = cdb_opcode;
    mark_eses_page_unsupported_ioctl.diag_page_code = diag_page_code;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_MARK_ESES_PAGE_UNSUPPORTED,
                                                            &mark_eses_page_unsupported_ioctl,
                                                            sizeof (fbe_terminator_mark_eses_page_unsupported_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_device_cpd_device_id ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/23/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_device_cpd_device_id(const fbe_api_terminator_device_handle_t device_handle, fbe_miniport_device_id_t *cpd_device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_device_cpd_device_id_ioctl_t get_device_cpd_device_id_ioctl;

    get_device_cpd_device_id_ioctl.device_handle = device_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DEVICE_CPD_DEVICE_ID,
                                                            &get_device_cpd_device_id_ioctl,
                                                            sizeof (fbe_terminator_get_device_cpd_device_id_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *cpd_device_id = get_device_cpd_device_id_ioctl.cpd_device_id;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_reserve_miniport_sas_device_table_index ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/23/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_reserve_miniport_sas_device_table_index(const fbe_u32_t port_number, const fbe_miniport_device_id_t cpd_device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_reserve_miniport_sas_device_table_index_ioctl_t reserve_miniport_sas_device_table_index_ioctl;

    reserve_miniport_sas_device_table_index_ioctl.port_number = port_number;
    reserve_miniport_sas_device_table_index_ioctl.cpd_device_id = cpd_device_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_RESERVE_MINIPORT_SAS_DEVICE_TABLE_INDEX,
                                                            &reserve_miniport_sas_device_table_index_ioctl,
                                                            sizeof (fbe_terminator_reserve_miniport_sas_device_table_index_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_miniport_sas_device_table_force_add ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/23/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_miniport_sas_device_table_force_add(const fbe_api_terminator_device_handle_t device_handle, const fbe_miniport_device_id_t cpd_device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_miniport_sas_device_table_force_add_ioctl_t miniport_sas_device_table_force_add_ioctl;

    miniport_sas_device_table_force_add_ioctl.device_handle = device_handle;
    miniport_sas_device_table_force_add_ioctl.cpd_device_id = cpd_device_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_MINIPORT_SAS_DEVICE_TABLE_FORCE_ADD,
                                                            &miniport_sas_device_table_force_add_ioctl,
                                                            sizeof (fbe_terminator_miniport_sas_device_table_force_add_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_miniport_sas_device_table_force_remove ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/23/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_miniport_sas_device_table_force_remove(const fbe_api_terminator_device_handle_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_miniport_sas_device_table_force_remove_ioctl_t miniport_sas_device_table_force_remove_ioctl;

    miniport_sas_device_table_force_remove_ioctl.device_handle = device_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_MINIPORT_SAS_DEVICE_TABLE_FORCE_REMOVE,
                                                            &miniport_sas_device_table_force_remove_ioctl,
                                                            sizeof (fbe_terminator_miniport_sas_device_table_force_remove_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_api_terminator_get_devices_count_by_type_name ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failue
 *
 *  History:
 *    07/20/2010: hud1  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_devices_count_by_type_name(const fbe_u8_t * type_name,fbe_u32_t  *const device_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_devices_count_by_type_name_ioctl_t get_devices_count_by_type_name_ioctl;
    size_t len = strlen(type_name)+1;
    if (len >sizeof(get_devices_count_by_type_name_ioctl.device_type_name)) {
        *device_count = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(get_devices_count_by_type_name_ioctl.device_type_name,type_name,(fbe_u32_t)len);
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DEVICES_COUNT_BY_TYPE_NAME,
                                                            &get_devices_count_by_type_name_ioctl,
                                                            sizeof (fbe_terminator_get_devices_count_by_type_name_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        *device_count = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *device_count = get_devices_count_by_type_name_ioctl.device_count;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_need_update_enclosure_resume_prom_checksum ()
 *********************************************************************
 *
 *  Description: set whether user wants to update enclosure resume prom checksum
 *
 *  Inputs: need_update_checksum - boolean flag.
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/18/11: Lin Lou created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_need_update_enclosure_resume_prom_checksum(fbe_bool_t need_update_checksum)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl_t set_need_update_enclosure_resume_prom_checksum_ioctl;

    set_need_update_enclosure_resume_prom_checksum_ioctl.need_update_checksum = need_update_checksum;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_NEED_UPDATE_ENCLOSURE_RESUME_PROM_CHECKSUM,
                                                            &set_need_update_enclosure_resume_prom_checksum_ioctl,
                                                            sizeof (fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_need_update_enclosure_resume_prom_checksum()
 *********************************************************************
 *
 *  Description: get whether user wants to update enclosure resume prom checksum
 *
 *  Inputs: need_update_checksum - boolean flag.
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/18/11: Lin Lou created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_need_update_enclosure_resume_prom_checksum(fbe_bool_t *need_update_checksum)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl_t get_need_update_enclosure_resume_prom_checksum_ioctl;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_NEED_UPDATE_ENCLOSURE_RESUME_PROM_CHECKSUM,
                                                            &get_need_update_enclosure_resume_prom_checksum_ioctl,
                                                            sizeof (fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *need_update_checksum = get_need_update_enclosure_resume_prom_checksum_ioctl.need_update_checksum;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_need_update_enclosure_firmware_rev ()
 *********************************************************************
 *
 *  Description: set whether user wants to update enclosure firmware revision number
 *
 *  Inputs: update_rev - boolean flag.
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/26/10: Bo Gao created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_need_update_enclosure_firmware_rev(fbe_bool_t  update_rev)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl_t set_need_update_enclosure_firmware_rev_ioctl;

    set_need_update_enclosure_firmware_rev_ioctl.need_update_rev = update_rev;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_NEED_UPDATE_ENCLOSURE_FIRMWARE_REV,
                                                            &set_need_update_enclosure_firmware_rev_ioctl,
                                                            sizeof (fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_need_update_enclosure_firmware_rev ()
 *********************************************************************
 *
 *  Description: get whether user wants to update enclosure firmware revision number
 *
 *  Inputs: update_rev - boolean flag.
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/26/10: Bo Gao created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_need_update_enclosure_firmware_rev(fbe_bool_t  *update_rev)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl_t get_need_update_enclosure_firmware_rev_ioctl;


    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_NEED_UPDATE_ENCLOSURE_FIRMWARE_REV,
                                                            &get_need_update_enclosure_firmware_rev_ioctl,
                                                            sizeof (fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *update_rev = get_need_update_enclosure_firmware_rev_ioctl.need_update_rev;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_enclosure_firmware_activate_time_interval ()
 *********************************************************************
 *
 *  Description: set time interval the enclosure wait before actual activating.
 *
 *  Inputs: enclosure_handle
 *            time_interval
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/23/10: Bo Gao created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_enclosure_firmware_activate_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t time_interval)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl_t set_enclosure_firmware_activate_time_interval_ioctl;

    set_enclosure_firmware_activate_time_interval_ioctl.enclosure_handle = enclosure_handle;
    set_enclosure_firmware_activate_time_interval_ioctl.time_interval = time_interval;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL,
                                                            &set_enclosure_firmware_activate_time_interval_ioctl,
                                                            sizeof (fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_enclosure_firmware_activate_time_interval ()
 *********************************************************************
 *
 *  Description: get time interval the enclosure wait before actual activating.
 *
 *  Inputs: enclosure_handle
 *            time_interval
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/23/10: Bo Gao created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_enclosure_firmware_activate_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t *time_interval)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl_t get_enclosure_firmware_activate_time_interval_ioctl;

    get_enclosure_firmware_activate_time_interval_ioctl.enclosure_handle = enclosure_handle;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL,
                                                            &get_enclosure_firmware_activate_time_interval_ioctl,
                                                            sizeof (fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *time_interval = get_enclosure_firmware_activate_time_interval_ioctl.time_interval;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_enclosure_firmware_reset_time_interval ()
 *********************************************************************
 *
 *  Description: set time interval the enclosure wait before actual reseting.
 *
 *  Inputs: enclosure_handle
 *            time_interval
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/23/10: Bo Gao created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_enclosure_firmware_reset_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t time_interval)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl_t set_enclosure_firmware_reset_time_interval_ioctl;

    set_enclosure_firmware_reset_time_interval_ioctl.enclosure_handle = enclosure_handle;
    set_enclosure_firmware_reset_time_interval_ioctl.time_interval = time_interval;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL,
                                                            &set_enclosure_firmware_reset_time_interval_ioctl,
                                                            sizeof (fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_enclosure_firmware_reset_time_interval ()
 *********************************************************************
 *
 *  Description: get time interval the enclosure wait before actual reseting.
 *
 *  Inputs: enclosure_handle
 *            time_interval
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/23/10: Bo Gao created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_enclosure_firmware_reset_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t *time_interval)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl_t get_enclosure_firmware_reset_time_interval_ioctl;

    get_enclosure_firmware_reset_time_interval_ioctl.enclosure_handle = enclosure_handle;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL,
                                                            &get_enclosure_firmware_reset_time_interval_ioctl,
                                                            sizeof (fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *time_interval = get_enclosure_firmware_reset_time_interval_ioctl.time_interval;
    return FBE_STATUS_OK;
}

/*!********************************************************************
 *  @fn fbe_api_terminator_get_connector_id_list_for_enclosure (fbe_api_terminator_device_handle_t enclosure_handle,
 *                                             fbe_term_encl_connector_list_t * connector_ids)
 *********************************************************************
 * @brief get the connector id list for the specified enclosure.
 *
 * @param enclosure_handle (INPUT)- handle of the enclosure
 * @param connector_ids (OUTPUT)- 
 *
 * @return success or failue
 *
 * @version
 *    26/Apr/11: PHE   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_connector_id_list_for_enclosure(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   fbe_term_encl_connector_list_t * connector_ids)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_connector_id_list_for_enclosure_ioctl_t get_connector_id_list_ioctl = {0};

    get_connector_id_list_ioctl.enclosure_handle = enclosure_handle;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_CONNECTOR_ID_LIST_FOR_ENCLOSURE,
                                                            &get_connector_id_list_ioctl,
                                                            sizeof (fbe_terminator_get_connector_id_list_for_enclosure_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *connector_ids = get_connector_id_list_ioctl.connector_ids;
    return FBE_STATUS_OK;
}


/*!********************************************************************
 *  @fn fbe_api_terminator_get_drive_slot_parent( fbe_api_terminator_device_handle_t * enclosure_handle,
 *                                                     fbe_u32_t * slot_number)
 *********************************************************************
 * @brief get the connector id list for the specified enclosure.
 *
 * @param enclosure_handle (INPUT)- handle of the enclosure
 * @param slot_number (OUTPUT)- 
 *
 * @return success or failue
 *
 * @version
 *    26/Apr/11: PHE   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_drive_slot_parent(fbe_api_terminator_device_handle_t * enclosure_handle, fbe_u32_t * slot_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_drive_slot_parent_ioctl_t get_drive_slot_parent_ioctl = {0};

    get_drive_slot_parent_ioctl.enclosure_handle = *enclosure_handle;
    get_drive_slot_parent_ioctl.slot_number = *slot_number;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_SLOT_PARENT,
                                                            &get_drive_slot_parent_ioctl,
                                                            sizeof (fbe_terminator_get_drive_slot_parent_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *enclosure_handle = get_drive_slot_parent_ioctl.enclosure_handle;
    *slot_number = get_drive_slot_parent_ioctl.slot_number;
    return FBE_STATUS_OK;
}


/*!********************************************************************
 *  @fn fbe_api_terminator_get_terminator_device_count( fbe_terminator_device_count_t * dev_count)
 *********************************************************************
 * @brief get the connector id list for the specified enclosure.
 *
 * @param dev_counts (OUTPUT)- handle of the enclosure
 *
 * @return success or failue
 *
 * @version
 *    26/Apr/11: PHE   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_terminator_device_count(fbe_terminator_device_count_t * dev_counts)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_terminator_device_count_ioctl_t get_device_count_ioctl = {0};

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_TERMINATOR_DEVICE_COUNT,
                                                            &get_device_count_ioctl,
                                                            sizeof (fbe_terminator_get_terminator_device_count_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *dev_counts = get_device_count_ioctl.dev_counts;
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_api_terminator_set_completion_dpc()
 *****************************************************************************
 *
 *  @brief  Either sets or disables the I/O completion IRQL at DPC.  Currently
 *          the default completion level is Passive (a.k.a. thread).  If the
 *          parameter is FBE_TRUE the terminator will complete I/Os at DPC
 *          which simulates the hardware.  If the parameter is FBE_FALSE
 *          this method will change the terminaotr completion IRQL to passive
 *
 *  @param  b_should_terminator_completion_be_at_dpc - FBE_TRUE - Terminator should
 *              complete I/Os at DPC IRQL.
 *                                                   - FBE_FALSE - Terminator
 *              should complete I/Os at passive (thread) IRQL.
 *
 *  @return status
 *
 *  @author
 *  02/20/2013  Ron Proulx  - Created.
 *    
 *****************************************************************************/
fbe_status_t fbe_api_terminator_set_completion_dpc(fbe_bool_t b_should_terminator_completion_be_at_dpc)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_io_completion_irql_ioctl_t set_completion_irql_ioctl;

    set_completion_irql_ioctl.b_should_completion_be_at_dpc = b_should_terminator_completion_be_at_dpc;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_IO_COMPLETION_IRQL,
                                                            &set_completion_irql_ioctl,
                                                            sizeof (fbe_terminator_set_io_completion_irql_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
