/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_board_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Board related APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_board_interface
 * 
 * @version
 *   2/19/10    Joe Perry - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_board_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

static fbe_status_t fbe_api_board_set_set_mgmt_module_vlan_config_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_board_set_mgmt_module_port_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_board_set_PostAndOrReset_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
fbe_status_t FBE_API_CALL fbe_api_board_resume_write_async_callback(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*!*******************************************************************
 * @fn fbe_api_get_board_object_id(fbe_object_id_t *object_id )
 *********************************************************************
 * @brief
 *  This function returns the object id of the Board Object.
 *
 * @param  object_id - pointer to the object ID
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    02/19/10:     Joe Perry  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_board_object_id(fbe_object_id_t *object_id)
{
	fbe_topology_control_get_board_t	        topology_control_get_board;
	fbe_status_t								status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t		status_info;

    if (object_id == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    topology_control_get_board.board_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
															 &topology_control_get_board,
															 sizeof(fbe_topology_control_get_board_t),
															 FBE_SERVICE_ID_TOPOLOGY,
															 FBE_PACKET_FLAG_NO_ATTRIB,
															 &status_info,
															 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        if ((status_info.control_operation_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE) && 
			 (status_info.control_operation_qualifier == FBE_OBJECT_ID_INVALID)) 
        {
            /*there was not object in this location, just return invalid id and good status*/
            *object_id = FBE_OBJECT_ID_INVALID;
            return FBE_STATUS_OK;
        }

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *object_id = topology_control_get_board.board_object_id;
    return status;

}

/*!***************************************************************
 * @fn fbe_api_board_get_sps_status(
 *     fbe_object_id_t object_id, 
 *     fbe_sps_get_sps_status_t *sps_status) 
 *****************************************************************
 * @brief
 *  This function gets SPS Status from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param sps_status - SPS Status passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    02/19/10:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_sps_status(fbe_object_id_t object_id, 
                             fbe_sps_get_sps_status_t *sps_status)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (sps_status == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SPS Status buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_STATUS,
                                                 sps_status,
                                                 sizeof (fbe_sps_get_sps_status_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

	if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_sps_status


/*!***************************************************************
 * @fn fbe_api_board_get_battery_status(
 *     fbe_object_id_t object_id, 
 *     fbe_sps_get_sps_status_t *battery_status) 
 *****************************************************************
 * @brief
 *  This function gets SPS Status from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param battery_status - Battery Status passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    04/18/12:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_battery_status(fbe_object_id_t object_id, 
                                 fbe_base_board_mgmt_get_battery_status_t *battery_status)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (battery_status == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Battery Status buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_STATUS,
                                                 battery_status,
                                                 sizeof (fbe_base_board_mgmt_get_battery_status_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_battery_status

/*!***************************************************************
 * @fn fbe_api_board_send_battery_command(
 *     fbe_object_id_t object_id, 
 *     fbe_base_board_mgmt_battery_command_t *batteryCommand)
 *****************************************************************
 * @brief
 *  This function sends an battery command to the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param batteryCommand - Battery command passed to the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    08/30/12:     Rui Chang  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_send_battery_command(fbe_object_id_t object_id, 
                               fbe_base_board_mgmt_battery_command_t *batteryCommand)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (batteryCommand == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Battery command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_BATTERY_COMMAND,
                                                 batteryCommand,
                                                 sizeof(fbe_base_board_mgmt_battery_command_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
							FBE_PACKAGE_ID_PHYSICAL);

	if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_send_sps_command


/*!***************************************************************
 * @fn fbe_api_board_get_sps_manuf_info(
 *     fbe_object_id_t object_id, 
 *     fbe_base_board_mgmt_get_sps_manuf_info_t *spsManufInfo) 
 *****************************************************************
 * @brief
 *  This function gets SPS Manufacturing Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param spsManufInfo - SPS Manufacturing Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    02/19/10:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_sps_manuf_info(fbe_object_id_t object_id, 
                                 fbe_sps_get_sps_manuf_info_t *spsManufInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (spsManufInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SPS Manuf Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_MANUF_INFO,
                                                 spsManufInfo,
                                                 sizeof (fbe_sps_get_sps_manuf_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

	if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_sps_manuf_info


/*!***************************************************************
 * @fn fbe_api_board_send_sps_command(
 *     fbe_object_id_t object_id, 
 *     fbe_sps_send_command_t *spsCommand) 
 *****************************************************************
 * @brief
 *  This function sends an SPS command to the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param spsCommand - SPS command passed to the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    02/19/10:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_send_sps_command(fbe_object_id_t object_id, 
                               fbe_sps_action_type_t spsCommand)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SPS_COMMAND,
                                                 &spsCommand,
                                                 sizeof (fbe_sps_action_type_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

	if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_send_sps_command

/*!***************************************************************
 * @fn fbe_api_board_get_pe_info
 *****************************************************************
 * @brief
 *  This function gets PE (Processor Enclosure) Status from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param pe_info - The pointer to the buffer which will hold the processor enclosure info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    03/16/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_pe_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_get_pe_info_t *pe_info)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (pe_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pe_info NULL pointer.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_PE_INFO,
                                                 pe_info,
                                                 sizeof (fbe_board_mgmt_get_pe_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_pe_info

/*!***************************************************************
 * @fn fbe_api_board_get_power_supply_info(
 *     fbe_object_id_t object_id, 
 *     fbe_power_supply_info_t *ps_info) 
 *****************************************************************
 * @brief
 *  This function gets Power Supply Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param ps_info - Power Supply Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  The client needs to supply the slotNumOnEncl value to 
 *  specify which power supply's info it is getting.
 *  Physical package expects the client to fill out this value.
 *
 * @version
 *    02/19/10:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_power_supply_info(fbe_object_id_t object_id, 
                                    fbe_power_supply_info_t *ps_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ps_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: PS Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_INFO,
                                                 ps_info,
                                                 sizeof (fbe_power_supply_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_ps_info

/*!***************************************************************
 * @fn fbe_api_board_get_io_port_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_io_port_info_t *io_port_info) 
 *****************************************************************
 * @brief
 *  This function gets IO Port Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param io_port_info - IO Port Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp, slotNumOnBlade,
 *  portNumOnModule and deviceType values. Physical package expects 
 *  the client to fill out these values.
 *  Currently, the deviceType value is IO module or mezzanine.
 *
 * @version
 *    05/19/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_io_port_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_io_port_info_t *io_port_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (io_port_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IO Port Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_INFO,
                                                 io_port_info,
                                                 sizeof (fbe_board_mgmt_io_port_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_io_port_info

/*!***************************************************************
 * @fn fbe_api_board_get_mgmt_module_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_mgmt_module_info_t *mgmt_module_info) 
 *****************************************************************
 * @brief
 *  This function gets Mgmt module Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param mgmt_module_info - Mgmt module Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and mgmtID values.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    05/19/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_mgmt_module_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_mgmt_module_info_t *mgmt_module_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (mgmt_module_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Mgmt module Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_INFO,
                                                 mgmt_module_info,
                                                 sizeof (fbe_board_mgmt_mgmt_module_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_mgmt_module_info

/*!***************************************************************
 * @fn fbe_api_board_get_cooling_info(
 *     fbe_object_id_t object_id, 
 *     fbe_cooling_info_t *cooling_info) 
 *****************************************************************
 * @brief
 *  This function gets Cooling Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param cooling_info - Cooling Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the slotNumOnEncl value.
 *  Physical package expects the client to fill out the value.
 *
 * @version
 *    05/19/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_cooling_info(fbe_object_id_t object_id, 
                          fbe_cooling_info_t *cooling_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (cooling_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Cooling Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_INFO,
                                                 cooling_info,
                                                 sizeof (fbe_cooling_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_cooling_info

/*!***************************************************************
 * @fn fbe_api_board_get_platform_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_platform_info_t *platform_info) 
 *****************************************************************
 * @brief
 *  This function gets Platform Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param platform_info - Platform Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *
 * @version
 *    05/17/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_platform_info(fbe_object_id_t object_id, 
                                fbe_board_mgmt_platform_info_t *platform_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (platform_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Platform Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
                                                 platform_info,
                                                 sizeof (fbe_board_mgmt_platform_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_platform_info

/*!***************************************************************
 * @fn fbe_api_board_get_bmc_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_bmc_info_t *bmc_info) 
 *****************************************************************
 * @brief
 *  This function gets Platform Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param bmc_info - BMC info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 * @version
 *    11-Sep-2012     Eric Zhou  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_bmc_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_bmc_info_t *bmc_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (bmc_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: BMC Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_INFO,
                                                 bmc_info,
                                                 sizeof (fbe_board_mgmt_bmc_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_bmc_info

/*!***************************************************************
 * @fn fbe_api_board_get_misc_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_misc_info_t *misc_info) 
 *****************************************************************
 * @brief
 *  This function gets Misc Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param misc_info - Misc info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *
 * @version
 *    05/18/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_misc_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_misc_info_t *misc_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (misc_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Misc Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_INFO,
                                                 misc_info,
                                                 sizeof (fbe_board_mgmt_misc_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_misc_info

 /*!***************************************************************
 * @fn fbe_api_board_get_suitcase_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_suitcase_info_t *suitcase_info) 
 *****************************************************************
 * @brief
 *  This function gets Suitcase Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param suitcase_info - Suitcase info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and suitcaseID values.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    05/18/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_suitcase_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_suitcase_info_t *suitcase_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (suitcase_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Suitcase Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_INFO,
                                                 suitcase_info,
                                                 sizeof (fbe_board_mgmt_suitcase_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_suitcase_info

/*!***************************************************************
 * @fn fbe_api_board_get_flt_reg_info(
 *     fbe_object_id_t object_id, 
 *     fbe_peer_boot_flt_exp_info_t *flt_reg_info) 
 *****************************************************************
 * @brief
 *  This function gets Fault Register Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param flt_exp_info - Fault reg info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and fltRegID values.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    26-July-2012:  Chengkai Hu,  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_flt_reg_info(fbe_object_id_t object_id, 
                          fbe_peer_boot_flt_exp_info_t *flt_reg_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (flt_reg_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Fault Reg Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    flt_reg_info->fltHwType = FBE_DEVICE_TYPE_FLT_REG;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_INFO,
                                                 flt_reg_info,
                                                 sizeof (fbe_peer_boot_flt_exp_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_flt_reg_info

/*!***************************************************************
 * @fn fbe_api_board_get_slave_port_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_slave_port_info_t *slave_port_info) 
 *****************************************************************
 * @brief
 *  This function gets Slave Port Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param slave_port_info - Pointer to Slave port Info.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and slavePortID values.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    05/18/10:     Arun S  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_slave_port_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_slave_port_info_t *slave_port_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (slave_port_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Slave Port Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_INFO,
                                                 slave_port_info,
                                                 sizeof (fbe_board_mgmt_slave_port_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_slave_port_info

/*!*******************************************************************
 * @fn fbe_api_board_get_io_module_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *iom_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the IO Module in PE.
 *
 * @param  iom_count - pointer to the IO Module count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/13/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_io_module_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *iom_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iom_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IO Module count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_COUNT,
                                                 iom_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_io_port_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *iop_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the IO Port in PE.
 *
 * @param  iop_count - pointer to the IO Port count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/13/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_io_port_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *iop_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iop_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IO Port count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_COUNT,
                                                 iop_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_bem_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *ioa_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the IO Annex in PE.
 *
 * @param  ioa_count - pointer to the IO Annex count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/13/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_bem_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *ioa_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ioa_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: BEM count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_COUNT,
                                                 ioa_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_mezzanine_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *mezz_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Mezzanine in PE.
 *
 * @param  mezz_count - pointer to the Mezzanine count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/13/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_mezzanine_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *mezz_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (mezz_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Mezzanine count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_COUNT,
                                                 mezz_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_ps_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *ps_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Power Supply in PE.
 *
 * @param  ps_count - pointer to the Power Supply
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/04/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_ps_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *ps_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ps_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Power Supply count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT,
                                                 ps_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_cooling_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *fan_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Cooler/Blower in PE.
 *
 * @param  fan_count - pointer to the Cooling component count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/04/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_cooling_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *fan_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (fan_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Cooling count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT,
                                                 fan_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_suitcase_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *suitcase_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Suitcase in PE.
 *
 * @param  sc_count - pointer to the suitcase component count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/12/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_suitcase_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *sc_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (sc_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Suitcase count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_COUNT,
                                                 sc_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_bmc_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *bmc_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the BMC in PE.
 *
 * @param  bmc_count - pointer to the bmc component count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    14-Sep-2012:     Eric Zhou  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_bmc_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *bmc_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (bmc_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: BMC count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_COUNT,
                                                 bmc_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_fault_reg_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *fault_reg_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the BMC in PE.
 *
 * @param  fault_reg_count - pointer to the BMC component count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/12/10:     Arun S  created
 *    07/09/14:     Chaitanya G updated to fault_reg
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_fault_reg_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *fault_reg_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (fault_reg_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Fault Register count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_COUNT,
                                                 fault_reg_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_slave_port_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *slave_port_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Slave Port in PE.
 *
 * @param  slave_port_count - pointer to the Slave port component count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/12/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_slave_port_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *slave_port_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (slave_port_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Slave Port count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_COUNT,
                                                 slave_port_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_io_module_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *iom_count) 
 *********************************************************************
 * @brief
 *  This function returns the IO Module count per blade in PE.
 *
 * @param  iom_count - pointer to the IO Module count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/14/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_io_module_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *iom_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iom_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IO Module count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_COUNT_PER_BLADE,
                                                 iom_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info, 
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_mezzanine_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *mezz_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Local Mezzanine in PE.
 *
 * @param  mezz_count - pointer to the Local Mezzanine count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/14/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_mezzanine_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *mezz_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (mezz_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Mezzanine count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_COUNT_PER_BLADE,
                                                 mezz_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_bem_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *ioa_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Local IO Module in PE.
 *
 * @param  ioa_count - pointer to the Local IO Module count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_bem_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *ioa_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ioa_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: BEM count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_COUNT_PER_BLADE,
                                                 ioa_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_io_port_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *io_port_count) 
 *********************************************************************
 * @brief
 *  This function returns the count of the Local Mezzanine in PE.
 *
 * @param  io_port_count - pointer to the Local Mezzanine count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_io_port_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *io_port_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (io_port_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IO Port count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_COUNT_PER_BLADE,
                                                 io_port_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_mgmt_module_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *mgmt_mod_count) 
 *********************************************************************
 * @brief
 *  This function returns the Mgmt module count per blade in PE.
 *
 * @param  mgmt_mod_count - pointer to the Mgmt module count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_mgmt_module_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *mgmt_mod_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (mgmt_mod_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Mgmt module count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_COUNT_PER_BLADE,
                                                 mgmt_mod_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_slave_port_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *slave_port_count) 
 *********************************************************************
 * @brief
 *  This function returns the Slave Port count per blade in PE.
 *
 * @param  slave_port_count - pointer to the slave port count
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_slave_port_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *slave_port_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (slave_port_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Slave Port count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_COUNT_PER_BLADE,
                                                 slave_port_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!*******************************************************************
 * @fn fbe_api_board_get_flt_reg_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *flt_reg_count) 
 *********************************************************************
 * @brief
 *  This function returns the Flt register count per blade in PE.
 *
 * @param  flt_reg_count - pointer to the Flt reg count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    31-July-2012:     Chengkai Hu - Created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_flt_reg_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *flt_reg_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (flt_reg_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Fault reg count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_COUNT_PER_BLADE,
                                                 flt_reg_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!*******************************************************************
 * @fn fbe_api_board_get_cooling_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *cooling_count) 
 *********************************************************************
 * @brief
 *  This function returns the Cooling count per blade in PE.
 *
 * @param  cooling_count - pointer to the Cooling count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_cooling_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *cooling_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (cooling_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Cooling count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT_PER_BLADE,
                                                 cooling_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_suitcase_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *suitcase_count) 
 *********************************************************************
 * @brief
 *  This function returns the Suitcase count per blade in PE.
 *
 * @param  suitcase_count - pointer to the Suitcase count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_suitcase_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *suitcase_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (suitcase_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Suitcase count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_COUNT_PER_BLADE,
                                                 suitcase_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_bmc_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *bmc_count) 
 *********************************************************************
 * @brief
 *  This function returns the BMC count per blade in PE.
 *
 * @param  bmc_count - pointer to the BMC count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    14-Sep-2012:   Eric Zhou  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_bmc_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *bmc_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (bmc_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: BMC count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_COUNT_PER_BLADE,
                                                 bmc_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_get_cache_card_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *cache_card_count) 
 *********************************************************************
 * @brief
 *  This function returns the cache card count per blade in PE.
 *
 * @param  bmc_count - pointer to the cache card count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    04-Feb-2013:   Rui Chang  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_cache_card_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *cache_card_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (cache_card_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Cache Card count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_COUNT,
                                                 cache_card_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_board_get_cache_card_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_cache_card_info_t *cache_card_info) 
 *****************************************************************
 * @brief
 *  This function gets cache card Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param bmc_info - cache card info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 * @version
 *    05-Feb-2013     Rui Chang  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_cache_card_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_cache_card_info_t *cache_card_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (cache_card_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Cache Card Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_INFO,
                                                 cache_card_info,
                                                 sizeof (fbe_board_mgmt_cache_card_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_bmc_info

/*!*******************************************************************
 * @fn fbe_api_board_get_dimm_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *cache_card_count) 
 *********************************************************************
 * @brief
 *  This function returns the DIMM count per blade in PE.
 *
 * @param  bmc_count - pointer to the DIMM count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    04-May-2013:   Rui Chang  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_dimm_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *dimm_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (dimm_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: DIMM count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_COUNT,
                                                 dimm_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_board_get_dimm_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_dimm_info_t *dimm_info) 
 *****************************************************************
 * @brief
 *  This function gets dimm Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param bmc_info - dimm info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 * @version
 *    05-Feb-2013     Rui Chang  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_dimm_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_dimm_info_t *dimm_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (dimm_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: DIMM Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_INFO,
                                                 dimm_info,
                                                 sizeof (fbe_board_mgmt_dimm_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_bmc_info


/*!*******************************************************************
 * @fn fbe_api_board_get_ssd_count(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *ssd_count) 
 *********************************************************************
 * @brief
 *  This function returns the SSD count per blade in PE.
 *
 * @param  ssd_count - pointer to the SSD count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05-Oct-2014:   bphilbin  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_ssd_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *ssd_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ssd_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SSD count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SSD_COUNT,
                                                 ssd_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_board_get_ssd_info(
 *     fbe_object_id_t object_id, 
 *     fbe_board_mgmt_ssd_info_t *ssd) 
 *****************************************************************
 * @brief
 *  This function gets dimm Info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param ssd_info - ssd info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 *
 * @version
 *    05-Oct-2014     bphilbin  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_ssd_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_ssd_info_t *ssd_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ssd_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SSD Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_SSD_INFO,
                                                 ssd_info,
                                                 sizeof (fbe_board_mgmt_ssd_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_bmc_info

/*!******************************************************************* 
* @fn fbe_api_board_get_power_supply_count_per_blade(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *ps_count) 
 *********************************************************************
 * @brief
 *  This function returns the PS count per blade in PE.
 *
 * @param  ps_count - pointer to the PS count per blade
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/19/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_power_supply_count_per_blade(fbe_object_id_t object_id, 
                                                      fbe_u32_t *ps_count)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ps_count == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: PS buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT_PER_BLADE,
                                                 ps_count,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_spFaultLED(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *blink_rate) 
 *********************************************************************
 * @brief
 *  This function sets the fault LED for SP in PE.
 *
 * @param  blink_rate - pointer to the SP Fault LED blink rate
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/05/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_spFaultLED(fbe_object_id_t object_id, 
                                                      LED_BLINK_RATE blink_rate,
													  fbe_u32_t status_condition)
{
    fbe_board_mgmt_set_sp_fault_LED_t          sp_fault_LED;
    fbe_api_control_operation_status_info_t    status_info;
    fbe_status_t                               status = FBE_STATUS_GENERIC_FAILURE;

    sp_fault_LED.blink_rate = blink_rate;
    sp_fault_LED.status_condition = status_condition;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_SP_FAULT_LED,
                                                 &sp_fault_LED,
                                                 sizeof (fbe_board_mgmt_set_sp_fault_LED_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*!*******************************************************************
 * @fn fbe_api_board_set_enclFaultLED(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *blink_rate) 
 *********************************************************************
 * @brief
 *  This function sets the fault LED for enclosure in PE.
 *
 * @param  enclFaultLedInfo - pointer to Encl Fault LED info
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/05/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_set_enclFaultLED(fbe_object_id_t object_id, 
                               fbe_api_board_enclFaultLedInfo_t *enclFaultLedInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (enclFaultLedInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Enclosure LED blink rate buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_ENCL_FAULT_LED,
                                                 enclFaultLedInfo,
                                                 sizeof (fbe_api_board_enclFaultLedInfo_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_UnsafetoRemoveLED(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t *blink_rate) 
 *********************************************************************
 * @brief
 *  This function sets the fault LED for Unsafe to remove in PE.
 *
 * @param  blink_rate - pointer to the LED blink rate
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/05/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_UnsafetoRemoveLED(fbe_object_id_t object_id, 
                                                      LED_BLINK_RATE blink_rate)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_UNSAFE_TO_REMOVE_LED,
                                                 &blink_rate,
                                                 sizeof (LED_BLINK_RATE),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_IOModuleFaultLED(
 *                             fbe_object_id_t object_id, 
 *                             SP_ID sp_id,
 *                             fbe_u32_t io_module,
 *                             LED_BLINK_RATE blink_rate) 
 *********************************************************************
 * @brief
 *  This function sets the fault LED for IO Module in PE.
 *
 * @param  blink_rate - pointer to the LED blink rate
 * @param  sp_id - SP for which the IO module belongs to
 * @param  io_module - IO module for which the LED to be turned on/off
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_IOModuleFaultLED(fbe_object_id_t object_id,
															 SP_ID sp_id, 
															 fbe_u32_t slot,
															 fbe_u64_t device_type,
                                                             LED_BLINK_RATE blink_rate)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_board_mgmt_set_iom_fault_LED_t              iom_fault_LED;
    
/*
    if (blink_rate == 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IO Module LED blink rate buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
*/

    iom_fault_LED.sp_id = sp_id;
    iom_fault_LED.slot = slot;
    iom_fault_LED.device_type = device_type;
    iom_fault_LED.blink_rate = blink_rate;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_FAULT_LED,
                                                 &iom_fault_LED,
                                                 sizeof (fbe_board_mgmt_set_iom_fault_LED_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    


    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_IOModulePersistedPowerState(
 *                             fbe_object_id_t object_id, 
 *                             SP_ID sp_id,
 *                             fbe_u32_t io_module,
 *                             fbe_bool_t persisted_power_enable) 
 *********************************************************************
 * @brief
 *  This function sets the persisted power state for the specified io module.
 *
 * @param  blink_rate - pointer to the LED blink rate
 * @param  sp_id - SP for which the IO module belongs to
 * @param  io_module - IO module for which the LED to be turned on/off
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    02/07/13:     bphilbin created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_IOModulePersistedPowerState(fbe_object_id_t object_id,
															 SP_ID sp_id, 
															 fbe_u32_t slot,
															 fbe_u64_t device_type,
                                                             fbe_bool_t persisted_power_enable)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_board_mgmt_set_iom_persisted_power_state_t  iom_persisted_power_state;

    iom_persisted_power_state.sp_id = sp_id;
    iom_persisted_power_state.slot = slot;
    iom_persisted_power_state.device_type = device_type;
    iom_persisted_power_state.persisted_power_enable = persisted_power_enable;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PERSISTED_POWER_STATE,
                                                 &iom_persisted_power_state,
                                                 sizeof (fbe_board_mgmt_set_iom_persisted_power_state_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);


    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}



/*!*******************************************************************
 * @fn fbe_api_setup_async_control_packet(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_set_iom_port_LED_t *iom_port) 
 *********************************************************************
 * @brief
 *  This function sets up the async control packet.
 *
 * @param  control_code 
 * @param  buffer 
 * @param  buffer_length 
 * @param  packet 
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    24-Mar-2011:     PHE - Added the function header.
 *    
 *********************************************************************/
fbe_status_t fbe_api_setup_async_control_packet(fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_packet_t **packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;

    *packet = fbe_api_get_contiguous_packet();
    
    payload = fbe_transport_get_payload_ex(*packet);

    // If the payload is NULL, exit and return generic failure.
    if (payload == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Payload is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    // If the control operation cannot be allocated, exit and return generic failure.
    if (control_operation == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate new control operation\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    return FBE_STATUS_OK;
}


/*!*******************************************************************
 * @fn fbe_api_board_set_iom_port_LED(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_set_iom_port_LED_t *iom_port) 
 *********************************************************************
 * @brief
 *  This function sets the LED for IO Module Port in PE.
 *
 * @param  fbe_board_mgmt_set_iom_port_LED_t - IOM port LED attributes
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_iom_port_LED(fbe_object_id_t object_id,
                                                      fbe_board_mgmt_set_iom_port_LED_t *iom_port)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iom_port == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: IOM port Status buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PORT_LED,
                                                 iom_port,
                                                 sizeof (fbe_board_mgmt_set_iom_port_LED_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!*******************************************************************
 * @fn fbe_api_board_set_mgmt_module_vlan_config_mode_async(
 *                             fbe_object_id_t object_id, 
 *                             SP_ID sp_id,
 *                             VLAN_CONFIG_MODE vlan_mode) 
 *********************************************************************
 * @brief
 *  This function sets the management module's vlan config mode.
 *
 * @param  sp_id - SP for which the Mgmt module belongs to
 * @param  vlan_mode - Vlan mode for Mgmt module
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_mgmt_module_vlan_config_mode_async(fbe_object_id_t object_id,
                                                             fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t                                    *packet = NULL;

    fbe_api_setup_async_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_VLAN_CONFIG_MODE,
                                       &context->command,
                                       sizeof (fbe_board_mgmt_set_mgmt_vlan_mode_t),
                                       &packet);

    status = fbe_api_common_send_control_packet_asynch(packet, 
                                                       object_id,
                                                       fbe_api_board_set_set_mgmt_module_vlan_config_mode_completion,
                                                       (fbe_packet_completion_context_t *)context,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_PENDING)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d\n", 
                       __FUNCTION__,
                       status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_set_mgmt_module_vlan_config_mode_completion(
 *                              fbe_packet_t * packet, 
 *                              fbe_packet_completion_context_t context) 
 *********************************************************************
 * @brief
 *  This function handles the asynchronous completion of setting the vlan 
 *  config mode.  It will call the specified callback function in the 
 *  fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t to get
 *  the write status back to the caller.
 *
 * @param  packet - packet allocated by fbe_api 
 * @param  context - callback context
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    02/17/11:     bphilbin - created
 *    
 *********************************************************************/
static fbe_status_t fbe_api_board_set_set_mgmt_module_vlan_config_mode_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t    *command_context;
    fbe_status_t                                            status;
    fbe_payload_control_operation_t                         *control_operation;
    fbe_payload_ex_t                                           *payload = NULL;

    command_context = (fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *) context;
    
    
    /*
     * The completion status of the set command is stored in the packet status
     * send this back to the original caller in the context provided.
     */
    status = fbe_transport_get_status_code(packet);
    command_context->status = status;

    /* 
     * Free up the memory that was allocated to send the packet.
     */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);
    
    (command_context->completion_function)(command_context);


    
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_mgmt_module_fault_LED(
 *                             fbe_object_id_t object_id, 
 *                             SP_ID sp_id,
 *                             LED_BLINK_RATE blink_rate) 
 *********************************************************************
 * @brief
 *  This function sets the Mgmt module fault LED in PE.
 *
 * @param  sp_id - SP for which the Mgmt module belongs to
 * @param  blink_rate - LED blink rate
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_mgmt_module_fault_LED(fbe_object_id_t object_id,
                                                             SP_ID sp_id, 
                                                             LED_BLINK_RATE blink_rate)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_board_mgmt_set_mgmt_fault_LED_t             mgmt_fault_led;

    mgmt_fault_led.sp_id = sp_id;
    mgmt_fault_led.blink_rate = blink_rate;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_FAULT_LED,
                                                 &mgmt_fault_led,
                                                 sizeof (fbe_board_mgmt_set_mgmt_fault_LED_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_mgmt_module_port_async(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_set_mgmt_port_t *mgmt_port) 
 *********************************************************************
 * @brief
 *  This function sets the Mgmt module Port in PE.
 *
 * @param  context - mgmt port setting attributes command context
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_mgmt_module_port_async(fbe_object_id_t object_id,
                                                      fbe_board_mgmt_set_mgmt_port_async_context_t *context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t                                    *packet = NULL;

    if (context == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Mgmt Port context buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_setup_async_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_PORT,
                                       &context->command,
                                       sizeof (fbe_board_mgmt_set_mgmt_port_t),
                                       &packet);

    status = fbe_api_common_send_control_packet_asynch(packet, 
                                                       object_id,
                                                       fbe_api_board_set_mgmt_module_port_completion,
                                                       (fbe_packet_completion_context_t *)context,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_PHYSICAL);

    
    if(status != FBE_STATUS_PENDING) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d\n", 
                       __FUNCTION__,
                       status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_mgmt_module_port_completion(
 *                              fbe_packet_t * packet, 
 *                              fbe_packet_completion_context_t context) 
 *********************************************************************
 * @brief
 *  This function handles the asynchronous completion of setting the vlan 
 *  config mode.  It will call the specified callback function in the 
 *  fbe_board_mgmt_set_mgmt_port_context_t to get
 *  the write status back to the caller.
 *
 * @param  packet - packet allocated by fbe_api 
 * @param  context - callback context
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    02/17/11:     bphilbin - created
 *    
 *********************************************************************/
static fbe_status_t fbe_api_board_set_mgmt_module_port_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_board_mgmt_set_mgmt_port_async_context_t    *command_context;
    fbe_status_t                              status;
    fbe_payload_control_operation_t           *control_operation;
    fbe_payload_ex_t                             *payload = NULL;

    command_context = (fbe_board_mgmt_set_mgmt_port_async_context_t *) context;
    
    /*
     * The completion status of the set command is stored in the packet status
     * send this back to the original caller in the context provided.
     */
    status = fbe_transport_get_status_code(packet);
    command_context->status = status;

    /* 
     * Free up the memory that was allocated to send the packet.
     */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);

    (command_context->completion_function)(command_context);
  
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_PostAndOrReset_async(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_set_PostAndOrReset_async_context_t *context) 
 *********************************************************************
 * @brief
 *  This function sets the hold in Reset or Post bit in PE.
 *
 * @param  fbe_board_mgmt_set_PostAndOrReset_t - Post Reset attributes
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_PostAndOrReset_async(fbe_object_id_t object_id,
                                                           fbe_board_mgmt_set_PostAndOrReset_async_context_t *context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t                                    *packet = NULL;

    if (context == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Post Reset Status buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_setup_async_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_CLEAR_HOLD_IN_POST_AND_OR_RESET,
                                       &context->command,
                                       sizeof (fbe_board_mgmt_set_PostAndOrReset_t),
                                       &packet);

    status = fbe_api_common_send_control_packet_asynch(packet, 
                                                       object_id,
                                                       fbe_api_board_set_PostAndOrReset_completion,
                                                       (fbe_packet_completion_context_t *)context,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_PHYSICAL);

    if(status != FBE_STATUS_PENDING) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d\n", 
                       __FUNCTION__,
                       status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_PostAndOrReset_completion(
 *                              fbe_packet_t * packet, 
 *                              fbe_packet_completion_context_t context) 
 *********************************************************************
 * @brief
 *  This function handles the asynchronous completion of setting the vlan 
 *  config mode.  It will call the specified callback function in the 
 *  fbe_board_mgmt_set_PostAndOrReset_async_context_t to get
 *  the write status back to the caller.
 *
 * @param  packet - packet allocated by fbe_api 
 * @param  context - callback context
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    02/17/11:     bphilbin - created
 *    
 *********************************************************************/
static fbe_status_t fbe_api_board_set_PostAndOrReset_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_board_mgmt_set_PostAndOrReset_async_context_t    *command_context;
    fbe_status_t                              status;
    fbe_payload_control_operation_t           *control_operation;
    fbe_payload_ex_t                             *payload = NULL;

    command_context = (fbe_board_mgmt_set_PostAndOrReset_async_context_t *) context;
    
    /*
     * The completion status of the set command is stored in the packet status
     * send this back to the original caller in the context provided.
     */
    status = fbe_transport_get_status_code(packet);
    command_context->status = status;

    /* 
     * Free up the memory that was allocated to send the packet.
     */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);
    
    (command_context->completion_function)(command_context);
  
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_board_clear_flt_reg_fault(
 *     fbe_object_id_t object_id, 
 *     fbe_peer_boot_flt_exp_info_t *flt_reg_info) 
 *****************************************************************
 * @brief
 *  This function clear Fault Register fault in VEEPROM. 
 *
 * @param object_id - The board object id to send request to.
 * @param flt_exp_info - Fault reg info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and fltRegID values.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    26-July-2012:  Chengkai Hu,  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_clear_flt_reg_fault(fbe_object_id_t object_id, 
                          fbe_peer_boot_flt_exp_info_t *flt_reg_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (flt_reg_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Fault Reg Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    flt_reg_info->fltHwType = FBE_DEVICE_TYPE_FLT_REG;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_CLEAR_FLT_REG_FAULT,
                                                 flt_reg_info,
                                                 sizeof (fbe_peer_boot_flt_exp_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_clear_flt_reg_fault

/*!*******************************************************************
 * @fn fbe_api_board_set_FlushFilesAndReg(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_set_FlushFilesAndReg_t *mgmt_port) 
 *********************************************************************
 * @brief
 *  This function issues a request to flush file buffers and registry
 *  io to disk.
 *
 * @param  fbe_board_mgmt_set_FlushFilesAndReg_t - Post Reset attributes
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    12/28/10:     Brion P  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_FlushFilesAndReg(fbe_object_id_t object_id,
                                                      fbe_board_mgmt_set_FlushFilesAndReg_t *flushOptions)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (flushOptions == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Post Reset Status buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_FLUSH_FILES_AND_REG,
                                                 flushOptions,
                                                 sizeof (fbe_board_mgmt_set_FlushFilesAndReg_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_resume(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_set_resume_t *resume_prom) 
 *********************************************************************
 * @brief
 *  This function sets the resume prom for a particular SMB device in PE.
 *
 * @param  fbe_board_mgmt_set_resume_t - Resume prom structure
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    05/10/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_resume(fbe_object_id_t object_id,
                                                      fbe_board_mgmt_set_resume_t *resume_prom)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (resume_prom == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Resume Prom buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_RESUME,
                                                 resume_prom,
                                                 sizeof (fbe_board_mgmt_set_resume_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*!***************************************************************
 * fbe_api_board_get_resume()
 ****************************************************************
 * @brief
 *  This function gets the resume prom for a particular SMB device in PE
 *  This function return data synchronously. 
 *
 * @note
 *  This api is obsolete.
 *  Please use fbe_api_board_resume_read_sync for sync read and 
 *  fbe_api_board_resume_read_async for async read.
 *
 * @param  fbe_board_mgmt_get_resume_t - Resume prom structure
 *
 * @return fbe_status_t, success or failure
 *
 * @author 
 *  18-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_get_resume(fbe_object_id_t object_id,
                                                   fbe_board_mgmt_get_resume_t *resume_prom)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (resume_prom == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Resume Prom buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME,
                                                 resume_prom,
                                                 sizeof (fbe_board_mgmt_get_resume_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************
 *  end of fbe_api_board_get_resume()
 ***************************************/
/*!*******************************************************************
 * @fn fbe_api_board_resume_read_sync(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_get_resume_t *resume_prom) 
 *********************************************************************
 * @brief
 *  This function issues SYNC resume prom read for a particular component in PE.
 *
 * @param  pResumeReadCmd - 
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    20-Jul-2011:     PHE -  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_resume_read_sync(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_cmd_t * pReadResumeCmd)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_u8_t                                    sg_count = 0;
    fbe_sg_element_t                            sg_element[2];

    if (pReadResumeCmd == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Get Resume Prom buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the element size and address for the data buffer */
    fbe_sg_element_init(&sg_element[sg_count], 
                        pReadResumeCmd->bufferSize, 
                        pReadResumeCmd->pBuffer);
    sg_count++;

    /* No more elements. Terminate it. */
    fbe_sg_element_terminate(&sg_element[sg_count]);
    sg_count++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_BOARD_CONTROL_CODE_RESUME_READ,
                                                             pReadResumeCmd,
                                                             sizeof (fbe_resume_prom_cmd_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,
                                                             sg_count,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers. In the case of user space to kernel space, 
    * the pBuffer would be overwritten by the kernel space address in base_board,
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    pReadResumeCmd->pBuffer = sg_element[0].address;

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_resume_read_async(
 *                             fbe_object_id_t object_id, 
 *                             fbe_resume_prom_get_resume_async_t *pGetResumeProm) 
 *********************************************************************
 * @brief
 *  This function issues ASYNC resume prom read for a particular component in PE.
 *
 * @param  fbe_resume_prom_get_resume_async_t - Resume prom structure
 *
 * @return fbe_status_t - success or failure
 *
 * @version
 *    05/20/10:     Arun S  created
 *    12/28/10:     Arun S  Modified to fit in the newly created common interface
 *                          for resume prom read - Asynchronous.
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_resume_read_async(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_get_resume_async_t *pGetResumeProm)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_u8_t                                    sg_count = 0;
    fbe_sg_element_t                            sg_element[2];

    if (pGetResumeProm == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Get Resume Prom buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the element size and address for the data buffer */
    fbe_sg_element_init(&sg_element[sg_count], 
                        pGetResumeProm->resumeReadCmd.bufferSize, 
                        pGetResumeProm->resumeReadCmd.pBuffer);
    sg_count++;

    /* No more elements. Terminate it. */
    fbe_sg_element_terminate(&sg_element[sg_count]);
    sg_count++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_BOARD_CONTROL_CODE_RESUME_READ,
                                                             &pGetResumeProm->resumeReadCmd,
                                                             sizeof(fbe_resume_prom_cmd_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,
                                                             sg_count,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers. In the case of user space to kernel space, 
    * the pBuffer would be overwritten by the kernel space address in base_board,
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    pGetResumeProm->resumeReadCmd.pBuffer = sg_element[0].address;

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    (pGetResumeProm->completion_function)(pGetResumeProm->completion_context, pGetResumeProm);

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_resume_write_sync(
 *                             fbe_object_id_t object_id, 
 *                             fbe_board_mgmt_get_resume_t *resume_prom) 
 *********************************************************************
 * @brief
 *  This function issues SYNC resume prom write for a particular component in PE.
 *  There is currently only the sync version, no async version.
 *
 * @param  pResumeReadCmd - 
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    20-Jul-2011:     PHE -  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_resume_write_sync(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_cmd_t * pWriteResumeCmd)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_u8_t                                    sg_count = 0;
    fbe_sg_element_t                            sg_element[2];

    if (pWriteResumeCmd == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Get Resume Prom buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the element size and address for the data buffer */
    fbe_sg_element_init(&sg_element[sg_count], 
                        pWriteResumeCmd->bufferSize, 
                        pWriteResumeCmd->pBuffer);
    sg_count++;

    /* No more elements. Terminate it. */
    fbe_sg_element_terminate(&sg_element[sg_count]);
    sg_count++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE,
                                                             pWriteResumeCmd,
                                                             sizeof (fbe_resume_prom_cmd_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,
                                                             sg_count,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_PHYSICAL);

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers. In the case of user space to kernel space, 
    * the pBuffer would be overwritten by the kernel space address in base_board,
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    pWriteResumeCmd->pBuffer = sg_element[0].address;

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_resume_write_async(
 *                             fbe_object_id_t object_id, 
 *                             fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp) 
 *********************************************************************
 * @brief
 *  This function issues ASYNC resume prom write for a particular component in PE.
 *
 * @param  object_id - Object ID.
 * @param  rpWriteAsynchOp - Pointer to fbe_resume_prom_write_resume_async_op_t structure
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    17-Dec-2012:  Dipak Patel -  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_resume_write_async(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t                                    sg_count = 0;
    fbe_packet_t *                              packet = NULL;

    if (rpWriteAsynchOp == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Pointer to WriteAsynchOp is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the element size and address for the data buffer */
    fbe_sg_element_init(&rpWriteAsynchOp->sg_element[sg_count], 
                        rpWriteAsynchOp->rpWriteAsynchCmd.bufferSize, 
                        rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer);
    sg_count++;

    /* No more elements. Terminate it. */
    fbe_sg_element_terminate(&rpWriteAsynchOp->sg_element[sg_count]);
    sg_count++;

    packet = fbe_api_get_contiguous_packet();/*no need to touch or initialize, it's taken from a pool and taken care of*/
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        fbe_api_free_memory(rpWriteAsynchOp);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                             FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE_ASYNC,
                                                             &rpWriteAsynchOp->rpWriteAsynchCmd,
                                                             sizeof (fbe_resume_prom_write_asynch_cmd_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             rpWriteAsynchOp->sg_element,
                                                             sg_count,
                                                             fbe_api_board_resume_write_async_callback,
                                                             rpWriteAsynchOp,
                                                             FBE_PACKAGE_ID_PHYSICAL);

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers. In the case of user space to kernel space, 
    * the pBuffer would be overwritten by the kernel space address in base_board,
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer = rpWriteAsynchOp->sg_element[0].address;

     if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }

    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_resume_write_async_callback(
 *                             fbe_packet_t * packet, 
 *                             fbe_packet_completion_context_t context) 
 *********************************************************************
 * @brief
 *  This function handles callback for ASYNC resume prom write 
 *  for a particular component in PE.
 *
 * @param  packet - Pointer to packet.
 * @param  context - context which was passed during async command.
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    17-Dec-2012:  Dipak Patel -  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_resume_write_async_callback(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp = (fbe_resume_prom_write_resume_async_op_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;

    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d, payload status:%d & qulf:%d\n", __FUNCTION__,
                       status, control_status, control_status_qualifier);

    }          

    /* Fill the status */
    rpWriteAsynchOp->status = status;

    fbe_api_return_contiguous_packet(packet);/*no need to destory or free it's returned to a queue and reused*/

    /* call callback fundtion */
   (rpWriteAsynchOp->completion_function)(rpWriteAsynchOp->encl_mgmt, rpWriteAsynchOp);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_board_get_iom_info
 *****************************************************************
 * @brief
 *  This function gets IOM (io module) status from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param iom_info - The pointer to the buffer which holds the
 *   io module info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and slotNumOnBlade.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    05/10/10:     Nayana Chaudhari - created
 *    05/25/10:     Arun S - Added note section
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_iom_info(fbe_object_id_t object_id,
                           fbe_board_mgmt_io_comp_info_t *iom_info)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iom_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: iom_info NULL pointer.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_INFO,
                                                 iom_info,
                                                 sizeof (fbe_board_mgmt_io_comp_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_iom_info

/*!***************************************************************
 * @fn fbe_api_board_get_bem_info
 *****************************************************************
 * @brief
 *  This function gets BEM status from the Board Object.
 *
 * @param object_id - The board object id to send request to.
 * @param iom_info - The pointer to the buffer which holds the
 *   io annex info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and slotNumOnBlade.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    05/10/10:     Nayana Chaudhari - created
 *    05/25/10:     Arun S - Added note section
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_bem_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_io_comp_info_t *iom_info)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iom_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: iom_info NULL pointer.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_INFO,
                                                 iom_info,
                                                 sizeof (fbe_board_mgmt_io_comp_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_bem_info

/*!***************************************************************
 * @fn fbe_api_board_get_mezzanine_info
 *****************************************************************
 * @brief
 *  This function gets mezzanine status from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param iom_info - The pointer to the buffer which holds the
 *   mezzanine info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  Please make sure you supply the associatedSp and slotNumOnBlade.
 *  Physical package expects the client to fill out these values.
 *
 * @version
 *    05/10/10:     Nayana Chaudhari - created
 *    05/25/10:     Arun S - Added note section.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_get_mezzanine_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_io_comp_info_t *iom_info)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (iom_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: iom_info NULL pointer.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_INFO,
                                                 iom_info,
                                                 sizeof (fbe_board_mgmt_io_comp_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_get_mezzanine_info


/*!***************************************************************
 * @fn fbe_api_board_set_ps_info(
 *     fbe_object_id_t object_id, 
 *     fbe_base_board_mgmt_set_ps_info_t *ps_info) 
 *****************************************************************
 * @brief
 *  This function sets PS info from the Board Object. 
 *
 * @param object_id - The board object id to send request to.
 * @param ps_info - PS info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    04/27/10:     Joe Perry  created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_board_set_ps_info(fbe_object_id_t object_id, 
                          fbe_base_board_mgmt_set_ps_info_t *ps_info)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info;

    if (ps_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: SPS Status buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_PS_INFO,
                                                 ps_info,
                                                 sizeof (fbe_base_board_mgmt_set_ps_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

	if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_board_set_ps_info

/*!*******************************************************************
 * @fn fbe_api_board_set_comp_max_timeout(
 *                             fbe_object_id_t object_id, 
 *                             fbe_u32_t component,
 *                             fbe_u32_t max_timeout) 
 *********************************************************************
 * @brief
 *  This function sets the max timeout for any PE component.
 *
 * @param object_id - The board object id to send request to.
 * @param component - The PE component for which max timeout has to be set.
 * @param max_timeout - maximum timeout value
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    16/08/10:     Arun S  created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_comp_max_timeout(fbe_object_id_t object_id, 
                                                      fbe_u32_t component,
                                                      fbe_u32_t max_timeout)
{
    fbe_base_board_mgmt_set_max_timeout_info_t comp_max_timeout;
    fbe_api_control_operation_status_info_t    status_info;
    fbe_status_t                               status = FBE_STATUS_GENERIC_FAILURE;

    comp_max_timeout.component = component;
    comp_max_timeout.max_timeout = max_timeout;

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_SET_COMPONENT_MAX_TIMEOUT,
                                                 &comp_max_timeout,
                                                 sizeof (fbe_base_board_mgmt_set_max_timeout_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
												 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*!***************************************************************
 * fbe_api_board_get_base_board_info()
 ****************************************************************
 * @brief
 *  This function is used display board info
 *
 * @param object_id - The board object id to send request to
 * @param base_board_info_ptr - Ptr to fbe_base_board_get_base_board_info_t
 *
 * @return fbe_status_t
 *
 * @author 
 *  11-Aug-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_board_get_base_board_info(fbe_object_id_t object_id,
                                  fbe_base_board_get_base_board_info_t *base_board_info_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    if (base_board_info_ptr == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Base board buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_FBE_BASE_BOARD_INFO,
                                                 base_board_info_ptr,
                                                 sizeof (fbe_base_board_get_base_board_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
*  end of fbe_api_board_get_base_board_info()
****************************************************/

/*!***************************************************************
 * fbe_api_board_get_eir_info()
 ****************************************************************
 * @brief
 *  This function gets EIR info from Board Object.
 *
 * @param object_id - The board object id to send request to
 * @param eir_info_ptr - Ptr to fbe_base_board_get_eir_info_t
 *
 * @return fbe_status_t
 *
 * @author 
 *  11-Feb-2011: Created  Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_board_get_eir_info(fbe_object_id_t object_id,
                           fbe_base_board_get_eir_info_t *eir_info_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    if (eir_info_ptr == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: EIR buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_BOARD_CONTROL_CODE_GET_EIR_INFO,
                                                 eir_info_ptr,
                                                 sizeof (fbe_base_board_get_eir_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************
*  end of fbe_api_board_get_eir_info()
****************************************************/

/*!*******************************************************************
 * @fn fbe_api_board_set_local_software_boot_status(
 *                             fbe_object_id_t object_id,
 *                             fbe_base_board_set_local_software_boot_status_t * pSetLocaSoftwareBootStatus) 
 *********************************************************************
 * @brief
 *  This function sets the local software boot status.
 *
 * @param  object_id - The board object id.
 * @param  pSetLocaSoftwareBootStatus - 
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    04-Aug-2011: PHE - created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_local_software_boot_status(fbe_object_id_t object_id,
                                                              fbe_base_board_set_local_software_boot_status_t * pSetLocalSoftwareBootStatus)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};
    
    if (pSetLocalSoftwareBootStatus == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: pSetLocaSoftwareBootStatus is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_SOFTWARE_BOOT_STATUS, 
                                               pSetLocalSoftwareBootStatus,
                                               sizeof(fbe_base_board_set_local_software_boot_status_t),
                                               object_id,
                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                               &status_info,
                                               FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

/*!*******************************************************************
 * @fn fbe_api_board_set_local_flt_exp_status(
 *                             fbe_object_id_t object_id,
 *                             fbe_u8_t faultStatus)
 *********************************************************************
 * @brief
 *  This function sets the local software boot status.
 *
 * @param  object_id - The board object id.
 * @param  faultStatus - The fault status to be set.
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    04-Aug-2011: PHE - created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_local_flt_exp_status(fbe_object_id_t object_id,
                                                                       fbe_u8_t faultStatus)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};
    
    status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_FLT_EXP_STATUS, 
                                               &faultStatus,
                                               sizeof(fbe_u8_t),
                                               object_id,
                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                               &status_info,
                                               FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_board_set_async_io(fbe_object_id_t object_id, fbe_bool_t async_io)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};

	if(async_io){
		status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_ASYNC_IO, 
												   NULL,
												   0,
												   object_id,
												   FBE_PACKET_FLAG_NO_ATTRIB,
												   &status_info,
												   FBE_PACKAGE_ID_PHYSICAL);
	} else {
		status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_SYNC_IO, 
												   NULL,
												   0,
												   object_id,
												   FBE_PACKET_FLAG_NO_ATTRIB,
												   &status_info,
												   FBE_PACKAGE_ID_PHYSICAL);
	}

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}


fbe_status_t FBE_API_CALL fbe_api_board_set_dmrb_zeroing(fbe_object_id_t object_id, fbe_bool_t dmrb_zeroing)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};

	if(dmrb_zeroing){
		status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_ENABLE_DMRB_ZEROING, 
												   NULL,
												   0,
												   object_id,
												   FBE_PACKET_FLAG_NO_ATTRIB,
												   &status_info,
												   FBE_PACKAGE_ID_PHYSICAL);
	} else {
		status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_DISABLE_DMRB_ZEROING, 
												   NULL,
												   0,
												   object_id,
												   FBE_PACKET_FLAG_NO_ATTRIB,
												   &status_info,
												   FBE_PACKAGE_ID_PHYSICAL);
	}

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

fbe_status_t FBE_API_CALL fbe_api_board_sim_set_psl(fbe_object_id_t object_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};

    status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SIM_SWITCH_PSL, 
                                                NULL,
                                                0,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                &status_info,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}



/*!*******************************************************************
 * @fn fbe_api_board_set_cna_mode(fbe_object_id_t object_id, SPECL_CNA_MODE cna_mode)
 *********************************************************************
 * @brief
 *  This function issues a request to set the desired CNA mode for the system.
 *
 * @param  object_id - The board object id.
 * @param  cna_mode - The desired CNA mode.
 *
 * @return fbe_status_t, success or failure
 *
 * @version
 *    04-Jan-2015: bphilbin - created
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_board_set_cna_mode(fbe_object_id_t object_id, SPECL_CNA_MODE cna_mode)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};
    fbe_base_board_set_cna_mode_t                   set_cna_mode = {0};

    set_cna_mode.cna_mode = cna_mode;
    
    status = fbe_api_common_send_control_packet(FBE_BASE_BOARD_CONTROL_CODE_SET_CNA_MODE, 
                                               &set_cna_mode,
                                               sizeof(fbe_base_board_set_cna_mode_t),
                                               object_id,
                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                               &status_info,
                                               FBE_PACKAGE_ID_PHYSICAL);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}
