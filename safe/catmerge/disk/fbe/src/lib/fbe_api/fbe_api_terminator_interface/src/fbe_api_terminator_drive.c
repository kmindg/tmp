/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_terminator_drive.c
 ***************************************************************************
 *
 *  Description
 *      Terminator board related APIs 
 *
 *  History:
 *      09/09/09    guov - Created
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
 *            fbe_api_terminator_insert_sas_drive ()
 *********************************************************************
 *
 *  Description: insert a sas drive in Terminator
 *
 *  Inputs: port_handle - port handle
 *          enclosure_handle - enclosure handle
 *          drive_info - detail drive info to create
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/27/09: hud1    added
 *    
 *********************************************************************/

fbe_status_t fbe_api_terminator_insert_sas_drive(fbe_api_terminator_device_handle_t enclosure_handle,
                                                 fbe_u32_t slot_number,
                                                 fbe_terminator_sas_drive_info_t *drive_info,
                                                 fbe_api_terminator_device_handle_t *drive_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_sas_drive_ioctl_t insert_drive;

    insert_drive.enclosure_handle = enclosure_handle;
    insert_drive.slot_number = slot_number;

    fbe_copy_memory(&insert_drive.drive_info, drive_info, sizeof(fbe_terminator_sas_drive_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_DRIVE,
                                                            &insert_drive,
                                                            sizeof (fbe_terminator_insert_sas_drive_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_handle = insert_drive.drive_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_drive_handle ()
 *********************************************************************
 *
 *  Description: get the handle of an existing drive in Terminator
 *
 *  Inputs: port_number - number of the port
 *            enclosure_number - number of the enclosure
 *            slot_number- number of the drive
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_drive_handle(fbe_u32_t port_number,
                                                 fbe_u32_t enclosure_number,
                                                 fbe_u32_t slot_number,
                                                 fbe_api_terminator_device_handle_t * drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_drive_handle_ioctl_t get_drive_handle_ioctl;

    get_drive_handle_ioctl.port_number = port_number;
    get_drive_handle_ioctl.enclosure_number = enclosure_number;
    get_drive_handle_ioctl.slot_number = slot_number;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_HANDLE,
                                                            &get_drive_handle_ioctl,
                                                            sizeof (fbe_terminator_get_drive_handle_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_handle = get_drive_handle_ioctl.drive_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_remove_sas_drive ()
 *********************************************************************
 *
 *  Description: remove an existing sas drive in Terminator
 *
 *  Inputs: drive_handle - handle of the drive
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_remove_sas_drive (fbe_api_terminator_device_handle_t drive_handle)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_remove_sas_drive_ioctl_t remove_sas_drive_ioctl;

    remove_sas_drive_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_REMOVE_SAS_DRIVE,
                                                            &remove_sas_drive_ioctl,
                                                            sizeof (fbe_terminator_remove_sas_drive_ioctl_t),
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
 *            fbe_api_terminator_remove_sata_drive ()
 *********************************************************************
 *
 *  Description: remove an existing sata drive in Terminator
 *
 *  Inputs: drive_handle - handle of the drive
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/03/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_remove_sata_drive (fbe_api_terminator_device_handle_t drive_handle)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_remove_sata_drive_ioctl_t remove_sata_drive_ioctl;

    remove_sata_drive_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_REMOVE_SATA_DRIVE,
                                                            &remove_sata_drive_ioctl,
                                                            sizeof (fbe_terminator_remove_sata_drive_ioctl_t),
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
 *            fbe_api_terminator_pull_drive ()
 *********************************************************************
 *
 *  Description: pull an existing drive in Terminator (not destory)
 *
 *  Inputs: drive_handle - handle of the drive
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/23/09: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_pull_drive (fbe_api_terminator_device_handle_t drive_handle)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_pull_drive_ioctl_t pull_drive_ioctl;

    pull_drive_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_PULL_DRIVE,
                                                            &pull_drive_ioctl,
                                                            sizeof (fbe_terminator_pull_drive_ioctl_t),
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
 *            fbe_api_terminator_get_sas_drive_info ()
 *********************************************************************
 *
 *  Description: get the information of sas drive in Terminator
 *
 *  Inputs: drive_handle - handle of the drive
 *            drive_info - buffer of the information
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/11/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_sas_drive_info(fbe_api_terminator_device_handle_t drive_handle, fbe_terminator_sas_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_sas_drive_info_ioctl_t get_sas_drive_info_ioctl;

    get_sas_drive_info_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_SAS_DRIVE_INFO,
                                                            &get_sas_drive_info_ioctl,
                                                            sizeof (fbe_terminator_get_sas_drive_info_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_info = get_sas_drive_info_ioctl.drive_info;
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_terminator_get_sata_drive_info ()
 *********************************************************************
 *
 *  Description: get the information of sata drive in Terminator
 *
 *  Inputs: drive_handle - handle of the drive
 *            drive_info - buffer of the information
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/11/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_sata_drive_info(fbe_api_terminator_device_handle_t drive_handle, fbe_terminator_sata_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_sata_drive_info_ioctl_t get_sata_drive_info_ioctl;

    get_sata_drive_info_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_SATA_DRIVE_INFO,
                                                            &get_sata_drive_info_ioctl,
                                                            sizeof (fbe_terminator_get_sata_drive_info_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_info = get_sata_drive_info_ioctl.drive_info;
    return FBE_STATUS_OK;
}


/*TO be added*/
fbe_status_t fbe_api_terminator_insert_sata_drive(fbe_api_terminator_device_handle_t enclosure_handle,
                                                 fbe_u32_t slot_number,
                                                 fbe_terminator_sata_drive_info_t *drive_info,
                                                 fbe_api_terminator_device_handle_t *drive_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_sata_drive_ioctl_t insert_drive;

    insert_drive.enclosure_handle = enclosure_handle;
    insert_drive.slot_number = slot_number;

    fbe_copy_memory(&insert_drive.drive_info, drive_info, sizeof(fbe_terminator_sata_drive_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_SATA_DRIVE,
                                                            &insert_drive,
                                                            sizeof (fbe_terminator_insert_sata_drive_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_handle = insert_drive.drive_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_reinsert_drive ()
 *********************************************************************
 *
 *  Description: reinsert the drive into Terminator
 *
 *  Inputs: drive_handle - handle of the drive
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/24/09: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_reinsert_drive (fbe_api_terminator_device_handle_t enclosure_handle,
                                                fbe_u32_t slot_number,
                                                fbe_api_terminator_device_handle_t drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_reinsert_drive_ioctl_t reinsert_drive_ioctl;

    reinsert_drive_ioctl.enclosure_handle = enclosure_handle;
    reinsert_drive_ioctl.slot_number = slot_number;
    reinsert_drive_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_REINSERT_DRIVE,
                                                            &reinsert_drive_ioctl,
                                                            sizeof (fbe_terminator_reinsert_drive_ioctl_t),
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
 *            fbe_api_terminator_get_drive_error_count ()
 *********************************************************************
 *
 *  Description: get error count of the drive in Terminator
 *
 *  Inputs: handle - handle of the drive
 *            error_count_p - pointer to the error count
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/12/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_drive_error_count(fbe_api_terminator_device_handle_t handle, fbe_u32_t  *const error_count_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_drive_error_count_ioctl_t get_drive_error_count_ioctl;

    get_drive_error_count_ioctl.handle = handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_ERROR_COUNT,
                                                            &get_drive_error_count_ioctl,
                                                            sizeof (fbe_terminator_get_drive_error_count_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *error_count_p = get_drive_error_count_ioctl.error_count_p;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_drive_error_injection_init ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *            
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/19/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_error_injection_init(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_INIT,
                                                            NULL,
                                                            0,
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
 *            fbe_api_terminator_drive_error_injection_destroy ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *            
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/19/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_error_injection_destroy(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_DESTROY,
                                                            NULL,
                                                            0,
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
 *            fbe_api_terminator_drive_error_injection_start ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *            
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/19/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_error_injection_start(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_START,
                                                            NULL,
                                                            0,
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
 *            fbe_api_terminator_drive_error_injection_stop ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *            
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/19/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_error_injection_stop(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_STOP,
                                                            NULL,
                                                            0,
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
 *            fbe_api_terminator_drive_error_injection_add_error ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/19/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_error_injection_add_error(fbe_terminator_neit_error_record_t record)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_drive_error_injection_add_error_ioctl_t drive_error_injection_add_error_ioctl;

    drive_error_injection_add_error_ioctl.record = record;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_ADD_ERROR,
                                                            &drive_error_injection_add_error_ioctl,
                                                            sizeof (fbe_terminator_drive_error_injection_add_error_ioctl_t),
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
 *            fbe_api_terminator_drive_error_injection_init_error ()
 *********************************************************************
 *
 *  Description: 
 *
 *  Inputs:
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/19/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_error_injection_init_error(fbe_terminator_neit_error_record_t* record)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_drive_error_injection_init_error_ioctl_t drive_error_injection_init_error_ioctl;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_INIT_ERROR,
                                                            &drive_error_injection_init_error_ioctl,
                                                            sizeof (fbe_terminator_drive_error_injection_init_error_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *record = drive_error_injection_init_error_ioctl.record;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_simulated_drive_type ()
 *********************************************************************
 *
 *  Description: Set the type of simulated drive at the Terminator side.
 *
 *  Inputs: simulated_drive_type - simulated drive type
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/01/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_simulated_drive_type(terminator_simulated_drive_type_t simulated_drive_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_simulated_drive_type_ioctl_t set_simulated_drive_type_ioctl;

    set_simulated_drive_type_ioctl.simulated_drive_type = simulated_drive_type;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_TYPE,
                                                            &set_simulated_drive_type_ioctl,
                                                            sizeof (fbe_terminator_set_simulated_drive_type_ioctl_t),
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
 *            fbe_api_terminator_get_simulated_drive_type ()
 *********************************************************************
 *
 *  Description: Get the type of simulated drive at the Terminator side.
 *
 *  Inputs: simulated_drive_type - simulated drive type pointer
 *
 *  Return Value: success or failure
 *
 *  History:
 *    04/13/11: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_simulated_drive_type(terminator_simulated_drive_type_t *simulated_drive_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_simulated_drive_type_ioctl_t get_simulated_drive_type_ioctl;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_SIMULATED_DRIVE_TYPE,
                                                            &get_simulated_drive_type_ioctl,
                                                            sizeof (fbe_terminator_get_simulated_drive_type_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *simulated_drive_type = get_simulated_drive_type_ioctl.simulated_drive_type;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_simulated_drive_server_name ()
 *********************************************************************
 *
 *  Description: Set the server name of simulated drive Terminator connects to.
 *                  Remember to call it before initializing Terminator.
 *
 *  Inputs: server_name
 *
 *  Return Value: success or failure
 *
 *  History:
 *    09/14/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_simulated_drive_server_name(const char* server_name)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_simulated_drive_server_name_ioctl_t set_simulated_drive_server_name_ioctl;

    fbe_zero_memory(set_simulated_drive_server_name_ioctl.server_name, sizeof(set_simulated_drive_server_name_ioctl.server_name));
    strncpy(set_simulated_drive_server_name_ioctl.server_name, server_name, sizeof(set_simulated_drive_server_name_ioctl.server_name) - 1);
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_SERVER_NAME,
                                                            &set_simulated_drive_server_name_ioctl,
                                                            sizeof (fbe_terminator_set_simulated_drive_server_name_ioctl_t),
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
 *            fbe_api_terminator_set_simulated_drive_server_port ()
 *********************************************************************
 *
 *  Description: Set the server port of simulated drive Terminator connects to.
 *                  Remember to call it before initializing Terminator.
 *
 *  Inputs: server_port
 *
 *  Return Value: success or failure
 *
 *  History:
 *    09/14/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_simulated_drive_server_port(fbe_u16_t server_port)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_simulated_drive_server_port_ioctl_t set_simulated_drive_server_port_ioctl;

    set_simulated_drive_server_port_ioctl.server_port = server_port;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_SERVER_PORT,
                                                            &set_simulated_drive_server_port_ioctl,
                                                            sizeof (fbe_terminator_set_simulated_drive_server_port_ioctl_t),
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
 *            fbe_api_terminator_force_create_sas_drive ()
 *********************************************************************
 *
 *  Description: force create a sas drive in Terminator, not log in and insert bit set
 *
 *  Inputs: drive_handle - handle of the drive
 *          drive_info - buffer of the information
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/11/09: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_force_create_sas_drive (fbe_terminator_sas_drive_info_t *drive_info, fbe_api_terminator_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_force_create_sas_drive_ioctl_t force_create_sas_drive_ioctl;

    fbe_copy_memory(&force_create_sas_drive_ioctl.drive_info, drive_info, sizeof(fbe_terminator_sas_drive_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_FORCE_CREATE_SAS_DRIVE,
                                                            &force_create_sas_drive_ioctl,
                                                            sizeof (fbe_terminator_force_create_sas_drive_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_handle = force_create_sas_drive_ioctl.drive_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_force_create_sata_drive ()
 *********************************************************************
 *
 *  Description: force create a sata drive in Terminator, not log in and insert bit set
 *
 *  Inputs: drive_handle - handle of the drive
 *          drive_info - buffer of the information
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/11/09: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_force_create_sata_drive (fbe_terminator_sata_drive_info_t *drive_info, fbe_api_terminator_device_handle_t *drive_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_force_create_sata_drive_ioctl_t force_create_sata_drive_ioctl;

    fbe_copy_memory(&force_create_sata_drive_ioctl.drive_info, drive_info, sizeof(fbe_terminator_sata_drive_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_FORCE_CREATE_SATA_DRIVE,
                                                            &force_create_sata_drive_ioctl,
                                                            sizeof (fbe_terminator_force_create_sata_drive_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *drive_handle = force_create_sata_drive_ioctl.drive_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_drive_product_id ()
 *********************************************************************
 *
 *  Description: set the product id of an existing drive in Terminator
 *
 *  Inputs: drive_handle
 *            product_id
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    06/24/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_drive_product_id(fbe_api_terminator_device_handle_t drive_handle, const fbe_u8_t * product_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_drive_product_id_ioctl_t set_drive_product_id_ioctl;

    set_drive_product_id_ioctl.drive_handle = drive_handle;
    memset(set_drive_product_id_ioctl.product_id, 0, sizeof(set_drive_product_id_ioctl.product_id));
    memcpy(set_drive_product_id_ioctl.product_id, product_id, CSX_MIN(sizeof(set_drive_product_id_ioctl.product_id), TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_DRIVE_PRODUCT_ID,
                                                            &set_drive_product_id_ioctl,
                                                            sizeof (fbe_terminator_set_drive_product_id_ioctl_t),
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
 *            fbe_api_terminator_get_drive_product_id ()
 *********************************************************************
 *
 *  Description: get the product id of an existing drive in Terminator
 *
 *  Inputs: drive_handle
 *            product_id
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    06/24/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_drive_product_id(fbe_api_terminator_device_handle_t drive_handle, fbe_u8_t * product_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_drive_product_id_ioctl_t get_drive_product_id_ioctl;

    get_drive_product_id_ioctl.drive_handle = drive_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_PRODUCT_ID,
                                                            &get_drive_product_id_ioctl,
                                                            sizeof (fbe_terminator_get_drive_product_id_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    memcpy(product_id, get_drive_product_id_ioctl.product_id, TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_api_terminator_drive_set_state ()
 *********************************************************************
 *
 *  Description: set drive state in Terminator
 *
 *  Inputs: device_handle - handle of the drive
 *            drive_state - drive state to set
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/07/10: hud1.   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_set_state(fbe_terminator_api_device_handle_t device_handle,
                                                 terminator_sas_drive_state_t  drive_state)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_drive_set_state_ioctl_t drive_set_state_ioctl;

    drive_set_state_ioctl.device_handle = device_handle;
    drive_set_state_ioctl.drive_state = drive_state;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_STATE,
                                                            &drive_set_state_ioctl,
                                                            sizeof (drive_set_state_ioctl),
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
 *            fbe_api_terminator_sas_drive_get_default_page_info()
 *********************************************************************
 *
 *  Description: Get's the terminator's default data
 *     for a specific drive type.  These are defined in
 *     terminator_drive_sas_main_plugin.c.  
 *
 *  Inputs: drive_type - type of the drive
 *          default_info - default info that is returned.
 * 
 *  Return Value: success or failure
 *
 *  History:
 *    12/23/2013:  Wayne Garrett - created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_sas_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_drive_type_default_page_info_ioctl_t  ioctl;
    
    ioctl.drive_type = drive_type;    

    status = fbe_api_common_send_control_packet_to_service(FBE_TERMINATOR_CONTROL_CODE_DRIVE_GET_DEFAULT_PAGE_INFO,
                                                           &ioctl,
                                                           sizeof(ioctl),
                                                           FBE_SERVICE_ID_TERMINATOR,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *default_info = ioctl.default_info;

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_sas_drive_set_default_page_info()
 *********************************************************************
 *
 *  Description: Modify the terminator's default page info
 *     for a specific drive type.  These are defined in
 *     terminator_drive_sas_main_plugin.c.  
 *
 *  Inputs: drive_type - type of the drive
 *          default_info - default info that is returned.
 * 
 *  Return Value: success or failure
 *
 *  History:
 *    12/23/2013:  Wayne Garrett - created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_sas_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_drive_type_default_page_info_ioctl_t  ioctl;

    ioctl.drive_type = drive_type;   
    ioctl.default_info = *default_info; 

    status = fbe_api_common_send_control_packet_to_service(FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_DEFAULT_PAGE_INFO,
                                                           &ioctl,
                                                           sizeof(ioctl),
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
 *            fbe_api_terminator_sas_drive_set_default_field ()
 *********************************************************************
 *
 *  Description: Changes the terminator's underlying global default data
 *     structs, for a specific drive type.  These are defined in
 *     terminator_drive_sas_main_plugin.c.   Note, there is a higher level
 *     drive device object which may report back different values.  This
 *     object will intercept the inquiry data and overwrite the results
 *     with its info.   See sas_drive_process_payload_inquiry() for
 *     which values may be overwritten by this device object.
 *
 *  Inputs: drive_type - type of the drive
 *          inq_field - Inquiry field to update
 *          inq_data - new inquiry data
 *          inq_size - size of inquiry data.
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/21/2013:  Wayne Garrett - created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_sas_drive_set_default_field(fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, const fbe_u8_t *data, fbe_u32_t size)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_drive_default_ioctl_t  ioctl;
    fbe_sg_element_t sg_list[2];
    fbe_u32_t sg_list_count;


    ioctl.drive_type = drive_type;
    ioctl.field = field;

    fbe_sg_element_init(&sg_list[0], size, (void*)data);
    fbe_sg_element_terminate(&sg_list[1]);
    sg_list_count = 2;

    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_DEFAULT_FIELD,
                                                                        &ioctl,
                                                                        sizeof(ioctl),
                                                                        FBE_SERVICE_ID_TERMINATOR,
                                                                        FBE_PACKET_FLAG_NO_ATTRIB,
                                                                        sg_list,
                                                                        sg_list_count,
                                                                        &status_info,
                                                                        FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_api_terminator_set_simulated_drive_debug_flags ()
 *****************************************************************************
 *
 * @brief   Set the server port of simulated drive deug flags.
 * 
 * @param   drive_select_type - Method to select drive type (all, fru_index for b/e/s)
 * @param   first_term_drive_index - FBE_TERMINATOR_DRIVE_SELECT_ALL_DRIVES -
 *              Select all drives. Otherwise select type defines this value.
 * @param   last_term_drive_index - Last index of terminator drive array to select
 * @param   backend_bus_number - Backend bus number
 * @param   encl_number - encl_number
 * @param   slot_number - slot number
 * @param   terminator_drive_debug_flags - Terminator debug flags to set
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  10/17/2011  Ron Proulx  - Created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_terminator_set_simulated_drive_debug_flags(fbe_terminator_drive_select_type_t drive_select_type,
                                                   fbe_u32_t first_term_drive_index,
                                                   fbe_u32_t last_term_drive_index,
                                                   fbe_u32_t backend_bus_number,
                                                   fbe_u32_t encl_number,
                                                   fbe_u32_t slot_number,
                                                   fbe_terminator_drive_debug_flags_t terminator_drive_debug_flags)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_simulated_drive_debug_flags_ioctl_t set_simulated_drive_debug_flags_ioctl;

    set_simulated_drive_debug_flags_ioctl.drive_select_type      = drive_select_type;
    set_simulated_drive_debug_flags_ioctl.first_term_drive_index = first_term_drive_index;
    set_simulated_drive_debug_flags_ioctl.last_term_drive_index  = last_term_drive_index;
    set_simulated_drive_debug_flags_ioctl.backend_bus_number     = backend_bus_number;
    set_simulated_drive_debug_flags_ioctl.encl_number            = encl_number;
    set_simulated_drive_debug_flags_ioctl.terminator_drive_debug_flags = terminator_drive_debug_flags;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_DEBUG_FLAGS,
                                                            &set_simulated_drive_debug_flags_ioctl,
                                                            sizeof (fbe_terminator_set_simulated_drive_debug_flags_ioctl_t),
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


/*!***************************************************************************
 *          fbe_api_terminator_drive_disable_compression ()
 *****************************************************************************
 *
 * @brief   Disable compression in terminator drive.
 * 
 * @param   NULL
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  04/07/2011  Lili Chen  - Created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_terminator_drive_disable_compression(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_COMPRESSION_DISABLE,
                                                            NULL,
                                                            0,
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

/*!***************************************************************************
 *          fbe_api_terminator_drive_set_log_page_31_data()
 *****************************************************************************
 *
 * @brief   Set the log page 31 of a drive
 * 
 * @param   drive_handle - pointer to the drive
 * @param   log_page_31 - pointer to the log page 31 data 
 * @param   length - size of the log_page_31 buffer
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/21/2015  Deanna Heng  - Created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_set_log_page_31_data(fbe_api_terminator_device_handle_t drive_handle,
                                                           fbe_u8_t * log_page_31,
                                                           fbe_u32_t length)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_log_page_31_ioctl_t log_page_31_ioctl;

    log_page_31_ioctl.device_handle = drive_handle;
    log_page_31_ioctl.buffer_length = length;
    fbe_zero_memory(log_page_31_ioctl.log_page_31, sizeof(log_page_31_ioctl.log_page_31));
    fbe_copy_memory(&log_page_31_ioctl.log_page_31, log_page_31, length);
    
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_LOG_PAGE_31,
                                                            &log_page_31_ioctl,
                                                            sizeof (fbe_terminator_log_page_31_ioctl_t),
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

/*!***************************************************************************
 *          fbe_api_terminator_drive_get_log_page_31_data()
 *****************************************************************************
 *
 * @brief   Get the log page 31 of a drive
 * 
 * @param   drive_handle - pointer to the drive
 * @param   log_page_31 - pointer to the log page 31 data 
 * @param   length - pionter to size of the log_page_31 buffer
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/21/2015  Deanna Heng  - Created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_drive_get_log_page_31_data(fbe_api_terminator_device_handle_t drive_handle,
                                                           fbe_u8_t *log_page_31,
                                                           fbe_u32_t *length)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_log_page_31_ioctl_t log_page_31_ioctl;

    if (log_page_31 == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    log_page_31_ioctl.device_handle = drive_handle;
    log_page_31_ioctl.buffer_length = 0;
    fbe_zero_memory(log_page_31_ioctl.log_page_31, sizeof(log_page_31_ioctl.log_page_31));
    
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_LOG_PAGE_31,
                                                            &log_page_31_ioctl,
                                                            sizeof (fbe_terminator_log_page_31_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *length = log_page_31_ioctl.buffer_length;
    fbe_zero_memory(log_page_31, *length);
    fbe_copy_memory(log_page_31, &log_page_31_ioctl.log_page_31, *length);

    return FBE_STATUS_OK;
}

