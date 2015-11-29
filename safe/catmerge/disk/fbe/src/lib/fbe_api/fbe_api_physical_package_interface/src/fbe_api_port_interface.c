/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_port_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Port related APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_port_interface
 * 
 * @version
 *   10/20/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_topology_interface.h"

/**************************************
				Local variables
**************************************/

/*******************************************
				Local functions
********************************************/

/*!***************************************************************
 * @fn fbe_api_get_port_object_id_by_location(
 *    fbe_u32_t port, 
 *    fbe_object_id_t *object_id) 
 *****************************************************************
 * @brief
 *  This function gets port object ID by location.
 *
 * @param port - port info
 * @param object_id  - The logical drive object id to send request to
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  10/14/08: sharel	created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_port_object_id_by_location(fbe_u32_t port, 
                                                                 fbe_object_id_t *object_id)
{
	fbe_topology_control_get_port_by_location_t					topology_control_get_port_by_location;
	fbe_status_t												status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t						status_info;

	if (object_id == NULL) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	topology_control_get_port_by_location.port_number = port;
	topology_control_get_port_by_location.port_object_id = FBE_OBJECT_ID_INVALID;

	 status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION,
															 &topology_control_get_port_by_location,
															 sizeof(fbe_topology_control_get_port_by_location_t),
															 FBE_SERVICE_ID_TOPOLOGY,
															 FBE_PACKET_FLAG_NO_ATTRIB,
															 &status_info,
															 FBE_PACKAGE_ID_PHYSICAL);

	  if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		 if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) && 
			 (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) {
			 /*there was not object in this location, just return invalid id (was set before)and good status*/
			  *object_id = FBE_OBJECT_ID_INVALID;
			 return FBE_STATUS_OK;
		 }

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	 }

	 *object_id = topology_control_get_port_by_location.port_object_id;

     return status;
}

/*!***************************************************************
 * @fn fbe_api_get_port_object_id_by_location_and_role(
 *    fbe_u32_t port,
 *    fbe_port_role_t role,
 *    fbe_object_id_t *object_id)
 *****************************************************************
 * @brief
 *  This function gets port object ID by bus number and port role.
 *
 * @param port - bus number
 * @param role - port role
 * @param object_id  - The logical drive object id to send request to
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *  08/30/12: Lin Lou    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_port_object_id_by_location_and_role(fbe_u32_t port,
                                                                 fbe_port_role_t role,
                                                                 fbe_object_id_t *object_id)
{
    fbe_topology_control_get_port_by_location_and_role_t        topology_control_get_port;
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t                     status_info;

    if (object_id == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_control_get_port.port_number = port;
    topology_control_get_port.port_role = role;
    topology_control_get_port.port_object_id = FBE_OBJECT_ID_INVALID;

     status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_PORT_BY_LOCATION_AND_ROLE,
                                                             &topology_control_get_port,
                                                             sizeof(fbe_topology_control_get_port_by_location_and_role_t),
                                                             FBE_SERVICE_ID_TOPOLOGY,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

      if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
         if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) &&
             (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) {
             /*there was not object in this location, just return invalid id (was set before)and good status*/
              *object_id = FBE_OBJECT_ID_INVALID;
             return FBE_STATUS_OK;
         }

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
     }

     *object_id = topology_control_get_port.port_object_id;

     return status;
}

/*!***************************************************************
 * @fn fbe_api_get_port_hardware_information(
 *    fbe_object_id_t object_id, 
 *    fbe_port_hardware_info_t *port_hardware_info) 
 *****************************************************************
 * @brief
 *  This function gets port hardware information.
 * 
 * @param object_id  - The port object id to send request to
 * @param hardware_info - Buffer for returning port harware information
 *
 * @return fbe_status_t - success or failure
 *
 * @version 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_port_hardware_information(fbe_object_id_t object_id,
                                                                fbe_port_hardware_info_t *port_hardware_info)
{
	fbe_port_hardware_info_t                                    local_port_hardware_info;
	fbe_status_t												status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t						status_info;

    if (port_hardware_info == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&local_port_hardware_info,sizeof(fbe_port_hardware_info_t));
/*
	 status = fbe_api_common_send_control_packet_to_service (FBE_BASE_PORT_CONTROL_CODE_GET_HARDWARE_INFORMATION,
															 &local_port_hardware_info,
															 sizeof(fbe_port_hardware_info_t),
															 FBE_SERVICE_ID_TOPOLOGY,
															 FBE_PACKET_FLAG_NO_ATTRIB,
															 &status_info,
															 FBE_PACKAGE_ID_PHYSICAL);
*/
    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_HARDWARE_INFORMATION,
                                                 &local_port_hardware_info,
                                                 sizeof(fbe_port_hardware_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

	  if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);		
	
      }else{
        fbe_copy_memory(port_hardware_info, &local_port_hardware_info, sizeof(fbe_port_hardware_info_t));	 
      }

     return status;
}


/*!***************************************************************
 * @fn fbe_api_get_port_sfp_information(
 *    fbe_object_id_t object_id, 
 *    fbe_port_sfp_info_t *port_sfp_info_p) 
 *****************************************************************
 * @brief
 *  This function gets port SFP information.
 * 
 * @param object_id  - The port object id to send request to
 * @param port_sfp_info_p - Buffer for returning port SFP information
 *
 * @return fbe_status_t - success or failure
 *
 * @version 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_port_sfp_information(fbe_object_id_t object_id,
                                                           fbe_port_sfp_info_t *port_sfp_info_p)
{
	fbe_port_sfp_info_t                                     local_port_sfp_info;
	fbe_status_t											status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t					status_info;

    if (port_sfp_info_p == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&local_port_sfp_info,sizeof(fbe_port_sfp_info_t));

    /*
	 status = fbe_api_common_send_control_packet_to_service (FBE_BASE_PORT_CONTROL_CODE_GET_SFP_INFORMATION,
															 &local_port_sfp_info,
															 sizeof(fbe_port_sfp_info_t),
															 FBE_SERVICE_ID_TOPOLOGY,
															 FBE_PACKET_FLAG_NO_ATTRIB,
															 &status_info,
															 FBE_PACKAGE_ID_PHYSICAL);
     */
    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_SFP_INFORMATION,
                                                 &local_port_sfp_info,
                                                 sizeof(fbe_port_sfp_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

	  if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);		
	
      }else{
        fbe_copy_memory(port_sfp_info_p, &local_port_sfp_info, sizeof(fbe_port_sfp_info_t));	 
      }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_port_get_port_role(
 *    fbe_object_id_t object_id, 
 *    fbe_port_role_t *port_role) 
 *****************************************************************
 * @brief
 *  This function gets port Role information. (FE/BE/SPECIAL/Uncommitted)
 * 
 * @param object_id  - The port object id to send request to
 * @param port_role  - Buffer for returning port role
 *
 * @return fbe_status_t - success or failure
 *
 * @version 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_get_port_role(fbe_object_id_t object_id,
                                                      fbe_port_role_t *port_role)
{	
	fbe_base_port_mgmt_get_port_role_t						get_port_role;
	fbe_status_t											status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t					status_info;

    if (port_role == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *port_role = FBE_PORT_ROLE_INVALID;

	fbe_zero_memory(&get_port_role,sizeof(fbe_base_port_mgmt_get_port_role_t));

    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_PORT_ROLE,
                                                 &get_port_role,
                                                 sizeof(fbe_base_port_mgmt_get_port_role_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

	  if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);		
	
      }else{
		*port_role = get_port_role.port_role;        
      }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_port_get_kek_handle(
 *    fbe_object_id_t object_id,
 *     fbe_key_handle_t **key_handle)
 *****************************************************************
 * @brief
 *  This function gets kek handle stored in miniport. This is INTERNAL
 *  DEBUGGING purposes ONLY
 * 
 * @param object_id  - The port object id to send request to
 * @param key_handle  - Buffer to store the key handle
 *
 * @return fbe_status_t - success or failure
 *
 * @version 
 *   10/13/2013 - Ashok Tamilarasan - Created
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_get_kek_handle(fbe_object_id_t object_id,
                                                                     fbe_key_handle_t *key_handle)
{	
	fbe_base_port_mgmt_get_kek_handle_t						kek_handle;
	fbe_status_t											status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t					status_info;

    if (key_handle == NULL || object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_KEK_HANDLE,
                                                 &kek_handle,
                                                 sizeof(fbe_base_port_mgmt_get_kek_handle_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

	  if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {

		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);		
	
      }else{
		*key_handle = kek_handle.kek_ptr;        
      }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_port_get_link_information(
 *    fbe_object_id_t object_id, 
 *    fbe_port_role_t *port_role) 
 *****************************************************************
 * @brief
 *  This function gets port Role information. (FE/BE/SPECIAL/Uncommitted)
 * 
 * @param object_id  - The port object id to send request to
 * @param port_role  - Buffer for returning port role
 *
 * @return fbe_status_t - success or failure
 *
 * @version 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_get_link_information(fbe_object_id_t object_id,
                                                            fbe_port_link_information_t *port_link_info)
{	
    fbe_port_info_t                             port_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    fbe_zero_memory(&port_info,sizeof(port_info));

    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO,
                                                 &port_info,
                                                 sizeof(port_info),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

      if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    
      }else{
        *port_link_info = port_info.port_link_info;        
      }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_port_get_port_info(
 *    fbe_object_id_t object_id, 
 *    fbe_port_info_t *port_info) 
 *****************************************************************
 * @brief
 *  This function gets port information
 * 
 * @param object_id  - The port object id to send request to
 * @param port_info  - Buffer for returning port info
 *
 * @return fbe_status_t - success or failure
 *
 * @version 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_get_port_info(fbe_object_id_t object_id,
                                                     fbe_port_info_t  *port_info)
{	
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    fbe_zero_memory(port_info,sizeof(fbe_port_info_t));

    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO,
                                                 port_info,
                                                 sizeof(fbe_port_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

      if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
    
      }

     return status;
}
/*!***************************************************************
 * @fn fbe_api_port_set_port_debug_control(
 *    fbe_object_id_t object_id,
 *    fbe_port_dbg_ctrl_t *port_debug_control)
 *****************************************************************
 * @brief
 *  This function sets port debug control to simulate the insertion/removal
 *  of enclosure 0 on the port.
 *
 * @param object_id  - The port object id to send request to
 * @param port_debug_control  - The port debug control input parameter
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_set_port_debug_control(fbe_object_id_t object_id,
                                                              fbe_port_dbg_ctrl_t *port_debug_control)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_SET_DEBUG_CONTROL,
                                                 port_debug_control,
                                                 sizeof(fbe_port_dbg_ctrl_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_port_set_port_encryption_mode(
 *    fbe_object_id_t object_id,
 *    fbe_port_encryption_mode_t mode)
 *****************************************************************
 * @brief
 *  This function sets port debug control to simulate the insertion/removal
 *  of enclosure 0 on the port.
 *
 * @param object_id  - The port object id to send request to
 * @param port_debug_control  - The port debug control input parameter
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_set_port_encryption_mode(fbe_object_id_t object_id,
                                                                fbe_port_encryption_mode_t mode)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_base_port_mgmt_set_encryption_mode_t encryption_mode;

    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    encryption_mode.encryption_mode = mode;
    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_SET_ENCRYPTION_MODE,
                                                 &encryption_mode,
                                                 sizeof(fbe_base_port_mgmt_set_encryption_mode_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

    }

     return status;
}

/*!***************************************************************
 * @fn fbe_api_port_set_port_encryption_mode(
 *    fbe_object_id_t object_id,
 *    fbe_port_encryption_mode_t mode)
 *****************************************************************
 * @brief
 *  This function sets port debug control to simulate the insertion/removal
 *  of enclosure 0 on the port.
 *
 * @param object_id  - The port object id to send request to
 * @param port_debug_control  - The port debug control input parameter
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_port_debug_register_dek(fbe_object_id_t object_id,
                                                          fbe_base_port_mgmt_debug_register_dek_t *dek_info)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_DBEUG_REGISTER_DEK,
                                                 dek_info,
                                                 sizeof(fbe_base_port_mgmt_debug_register_dek_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

    }

     return status;
}

