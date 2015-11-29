/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_terminator_port.c
 ***************************************************************************
 *
 *  Description
 *      Terminator port related APIs 
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
 *            fbe_api_terminator_insert_sas_port ()
 *********************************************************************
 *
 *  Description: insert a sas port in Terminator
 *
 *  Inputs: port_info
 *
 *  Return Value: success or failue
 *
 *  History:
 *    09/09/09: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_sas_port(fbe_terminator_sas_port_info_t *port_info, 
                                            fbe_api_terminator_device_handle_t *port_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_sas_port_ioctl_t      insert_port;

    memcpy(&insert_port.port_info, port_info, sizeof(fbe_terminator_sas_port_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_PORT,
                                                            &insert_port,
                                                            sizeof (fbe_terminator_insert_sas_port_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *port_handle = insert_port.port_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_insert_fc_port ()
 *********************************************************************
 *
 *  Description: insert a fc port in Terminator
 *
 *  Inputs: port_info
 *
 *  Return Value: success or failue
 *
 *  History:
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_fc_port(fbe_terminator_fc_port_info_t *port_info, 
                                            fbe_api_terminator_device_handle_t *port_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_fc_port_ioctl_t       insert_port;

    memcpy(&insert_port.port_info, port_info, sizeof(fbe_terminator_fc_port_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_FC_PORT,
                                                            &insert_port,
                                                            sizeof (fbe_terminator_insert_fc_port_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *port_handle = insert_port.port_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_insert_iscsi_port ()
 *********************************************************************
 *
 *  Description: insert a iscsi port in Terminator
 *
 *  Inputs: port_info
 *
 *  Return Value: success or failue
 *
 *  History:
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_iscsi_port(fbe_terminator_iscsi_port_info_t *port_info, 
                                            fbe_api_terminator_device_handle_t *port_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_iscsi_port_ioctl_t        insert_port;

    memcpy(&insert_port.port_info, port_info, sizeof(fbe_terminator_iscsi_port_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_ISCSI_PORT,
                                                            &insert_port,
                                                            sizeof (fbe_terminator_insert_iscsi_port_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *port_handle = insert_port.port_handle;
    return FBE_STATUS_OK;
}



/*********************************************************************
 *            fbe_api_terminator_insert_fcoe_port ()
 *********************************************************************
 *
 *  Description: insert an fcoe port in Terminator
 *
 *  Inputs: port_info
 *
 *  Return Value: success or failue
 *
 *  History:
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_fcoe_port(fbe_terminator_fcoe_port_info_t *port_info, 
                                            fbe_api_terminator_device_handle_t *port_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_insert_iscsi_port_ioctl_t        insert_port;

    memcpy(&insert_port.port_info, port_info, sizeof(fbe_terminator_fcoe_port_info_t));
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_FCOE_PORT,
                                                            &insert_port,
                                                            sizeof (fbe_terminator_insert_iscsi_port_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *port_handle = insert_port.port_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_port_handle ()
 *********************************************************************
 *
 *  Description: get the handle of an existing port in Terminator
 *
 *  Inputs: backend_number - backend number of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    10/23/09: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_port_handle(fbe_u32_t backend_number, 
                                                fbe_api_terminator_device_handle_t *port_handle)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_port_handle_ioctl_t  get_port_handle;

    get_port_handle.backend_number = backend_number;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_PORT_HANDLE,
                                                            &get_port_handle,
                                                            sizeof (fbe_terminator_get_port_handle_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    *port_handle = get_port_handle.port_handle;
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_start_port_reset ()
 *********************************************************************
 *
 *  Description: start port reset in Terminator
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_start_port_reset (fbe_api_terminator_device_handle_t port_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_start_port_reset_ioctl_t start_port_reset;

    start_port_reset.port_handle = port_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_START_PORT_RESET,
                                                            &start_port_reset,
                                                            sizeof (fbe_terminator_start_port_reset_ioctl_t),
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
 *            fbe_api_terminator_complete_port_reset ()
 *********************************************************************
 *
 *  Description: complete port reset in Terminator
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/05/09: gaob3   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_complete_port_reset (fbe_api_terminator_device_handle_t port_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_complete_port_reset_ioctl_t complete_port_reset;

    complete_port_reset.port_handle = port_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_COMPLETE_PORT_RESET,
                                                            &complete_port_reset,
                                                            sizeof (fbe_terminator_complete_port_reset_ioctl_t),
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
 *            fbe_api_terminator_get_hardware_info ()
 *********************************************************************
 *
 *  Description: get hardware info of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    05/26/10: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_hardware_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_hardware_info_ioctl_t get_hardware_info;

    get_hardware_info.port_handle = port_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_HARDWARE_INFO,
                                                            &get_hardware_info,
                                                            sizeof (fbe_terminator_get_hardware_info_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(hdw_info, &get_hardware_info.hdw_info, sizeof (fbe_cpd_shim_hardware_info_t)) ;
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_api_terminator_set_hardware_info ()
 *********************************************************************
 *
 *  Description: set hardware info of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    05/26/10: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_hardware_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_hardware_info_ioctl_t set_hardware_info;

    set_hardware_info.port_handle = port_handle;
    fbe_copy_memory(&set_hardware_info.hdw_info, hdw_info, sizeof (fbe_cpd_shim_hardware_info_t)) ;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_HARDWARE_INFO,
                                                            &set_hardware_info,
                                                            sizeof (fbe_terminator_set_hardware_info_ioctl_t),
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
 *            fbe_api_terminator_get_sfp_media_interface_info ()
 *********************************************************************
 *
 *  Description: set sfp_media_interface of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    05/26/10: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_sfp_media_interface_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_sfp_media_interface_info_ioctl_t get_sfp_info;

    get_sfp_info.port_handle = port_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_SFP_MEDIA_INTERFACE_INFO,
                                                            &get_sfp_info,
                                                            sizeof (fbe_terminator_get_sfp_media_interface_info_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(sfp_media_interface_info, &get_sfp_info.sfp_info, sizeof (fbe_cpd_shim_sfp_media_interface_info_t)) ;
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_api_terminator_set_sfp_media_interface_info ()
 *********************************************************************
 *
 *  Description: set sfp_media_interface_info of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    05/26/10: guov    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_sfp_media_interface_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_sfp_media_interface_info_ioctl_t set_sfp_info;

    set_sfp_info.port_handle = port_handle;
    fbe_copy_memory(&set_sfp_info.sfp_info, sfp_media_interface_info, sizeof (fbe_cpd_shim_sfp_media_interface_info_t)) ;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SFP_MEDIA_INTERFACE_INFO,
                                                            &set_sfp_info,
                                                            sizeof (fbe_terminator_set_sfp_media_interface_info_ioctl_t),
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
 *            fbe_api_terminator_get_port_link_info ()
 *********************************************************************
 *
 *  Description: get link information of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/25/11: aj    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_port_link_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_link_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_port_link_info_ioctl_t get_port_link_info;

    get_port_link_info.port_handle = port_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_PORT_LINK_INFO,
                                                            &get_port_link_info,
                                                            sizeof (fbe_terminator_get_port_link_info_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(port_link_info, &get_port_link_info.port_link_info, sizeof (fbe_cpd_shim_port_lane_info_t)) ;
    return FBE_STATUS_OK;
}
/*********************************************************************
 *            fbe_api_terminator_set_port_link_info ()
 *********************************************************************
 *
 *  Description: set port_link_info of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/25/11: aj    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_port_link_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_link_info)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_set_port_link_info_ioctl_t set_port_link_info;

    set_port_link_info.port_handle = port_handle;
    fbe_copy_memory(&set_port_link_info.port_link_info, port_link_info, sizeof (fbe_cpd_shim_port_lane_info_t)) ;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_PORT_LINK_INFO,
                                                            &set_port_link_info,
                                                            sizeof (fbe_terminator_set_port_link_info_ioctl_t),
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
 *            fbe_api_terminator_get_encryption_keys ()
 *********************************************************************
 *
 *  Description: set port_link_info of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/25/11: aj    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_encryption_keys(fbe_api_terminator_device_handle_t port_handle, fbe_key_handle_t key_handle, fbe_u8_t *keys)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_encryption_key_t get_encryption_key;

    get_encryption_key.port_handle = port_handle;
    get_encryption_key.key_handle = key_handle;
    fbe_zero_memory(get_encryption_key.key_buffer, FBE_ENCRYPTION_KEY_SIZE);

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_ENCRYPTION_KEY,
                                                            &get_encryption_key,
                                                            sizeof (fbe_terminator_get_encryption_key_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(keys, get_encryption_key.key_buffer, FBE_ENCRYPTION_KEY_SIZE);
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_encryption_keys ()
 *********************************************************************
 *
 *  Description: set port_link_info of a port
 *
 *  Inputs: port_handle - handle of the port
 *
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/25/11: aj    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_port_address(fbe_api_terminator_device_handle_t port_handle, fbe_address_t *address)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_get_port_address_t port_address;

    port_address.port_handle = port_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_PORT_ADDRESS,
                                                            &port_address,
                                                            sizeof (fbe_terminator_get_port_address_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(address, &port_address.address, sizeof(fbe_address_t));
    return FBE_STATUS_OK;
}

