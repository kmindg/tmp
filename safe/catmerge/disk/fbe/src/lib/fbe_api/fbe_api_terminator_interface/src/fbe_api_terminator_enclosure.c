/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_terminator_board.c
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
 *            fbe_api_terminator_insert_sas_enclosure ()
 *********************************************************************
 *
 *  Description: insert a sas enclosure in Terminator
 *
 *	Inputs: enclosure_type - type of inclosure to create.
 *
 *  Return Value: success or failure
 *
 *  History:
 *    09/09/09: guov	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_sas_enclosure(fbe_api_terminator_device_handle_t port_handle, 
												 fbe_terminator_sas_encl_info_t *encl_info, 
												 fbe_api_terminator_device_handle_t *enclosure_handle)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_insert_sas_encl_ioctl_t 	insert_enclosure;

	insert_enclosure.port_handle = port_handle;
	memcpy(&insert_enclosure.encl_info, encl_info, sizeof(fbe_terminator_sas_encl_info_t));
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_ENCLOSURE,
															&insert_enclosure,
															sizeof (fbe_terminator_insert_sas_encl_ioctl_t),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}
	*enclosure_handle = insert_enclosure.enclosure_handle;
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_get_enclosure_handle ()
 *********************************************************************
 *
 *  Description: get the handle of an existing enclosure in Terminator
 *
 *	Inputs: port_number - number of the port
 *			  enclosure_number - number of the enclosure
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/03/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_enclosure_handle(fbe_u32_t port_number,
                                                     fbe_u32_t enclosure_number,
                                                     fbe_api_terminator_device_handle_t * enclosure_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_get_enclosure_handle_ioctl_t get_enclosure_handle_ioctl;

	get_enclosure_handle_ioctl.port_number = port_number;
	get_enclosure_handle_ioctl.enclosure_number = enclosure_number;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_HANDLE,
															&get_enclosure_handle_ioctl,
															sizeof (fbe_terminator_get_enclosure_handle_ioctl_t),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}
	*enclosure_handle = get_enclosure_handle_ioctl.enclosure_handle;
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_remove_sas_enclosure ()
 *********************************************************************
 *
 *  Description: remove an existing enclosure in Terminator
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/04/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_remove_sas_enclosure(fbe_api_terminator_device_handle_t enclosure_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_remove_sas_enclosure_ioctl_t remove_sas_enclosure_ioctl;

	remove_sas_enclosure_ioctl.enclosure_handle = enclosure_handle;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_REMOVE_SAS_ENCLOSURE,
															&remove_sas_enclosure_ioctl,
															sizeof (fbe_terminator_remove_sas_enclosure_ioctl_t),
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
 *            fbe_api_terminator_enclosure_bypass_drive_slot ()
 *********************************************************************
 *
 *  Description: set the status of the specified slot on the enclosure to bypass
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *	     	  slot_number - number of the slot
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/05/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_enclosure_bypass_drive_slot (fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t slot_number)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_enclosure_bypass_drive_slot_ioctl_t enclosure_bypass_drive_slot_ioctl;

	enclosure_bypass_drive_slot_ioctl.enclosure_handle = enclosure_handle;
	enclosure_bypass_drive_slot_ioctl.slot_number = slot_number;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_BYPASS_DRIVE_SLOT,
															&enclosure_bypass_drive_slot_ioctl,
															sizeof (fbe_terminator_enclosure_bypass_drive_slot_ioctl_t),
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
 *            fbe_api_terminator_enclosure_unbypass_drive_slot ()
 *********************************************************************
 *
 *  Description: set the status of the specified slot on the enclosure to unbypass
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *	     	  slot_number - number of the slot
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/05/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_enclosure_unbypass_drive_slot (fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t slot_number)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_enclosure_unbypass_drive_slot_ioctl_t enclosure_unbypass_drive_slot_ioctl;

	enclosure_unbypass_drive_slot_ioctl.enclosure_handle = enclosure_handle;
	enclosure_unbypass_drive_slot_ioctl.slot_number = slot_number;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_UNBYPASS_DRIVE_SLOT,
															&enclosure_unbypass_drive_slot_ioctl,
															sizeof (fbe_terminator_enclosure_unbypass_drive_slot_ioctl_t),
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
 *            fbe_api_terminator_eses_increment_config_page_gen_code ()
 *********************************************************************
 *
 *  Description: increment the configuration page generation code for the enclosure
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/05/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_eses_increment_config_page_gen_code(fbe_api_terminator_device_handle_t enclosure_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_eses_increment_config_page_gen_code_ioctl_t eses_increment_config_page_gen_code_ioctl;

	eses_increment_config_page_gen_code_ioctl.enclosure_handle = enclosure_handle;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ESES_INCREMENT_CONFIG_PAGE_GEN_CODE,
															&eses_increment_config_page_gen_code_ioctl,
															sizeof (fbe_terminator_eses_increment_config_page_gen_code_ioctl_t),
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
 *            fbe_api_terminator_eses_set_unit_attention()
 *********************************************************************
 *
 *  Description: Sets the Unit attention for the enclosure
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    27/04/10: Arun S	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_eses_set_unit_attention(fbe_api_terminator_device_handle_t enclosure_handle)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_eses_set_unit_attention_ioctl_t eses_set_unit_attention_ioctl;

	eses_set_unit_attention_ioctl.enclosure_handle = enclosure_handle;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ESES_SET_UNIT_ATTENTION,
															&eses_set_unit_attention_ioctl,
															sizeof (fbe_terminator_eses_set_unit_attention_ioctl_t),
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
 *            fbe_api_terminator_set_buf_by_buf_id ()
 *********************************************************************
 *
 *  Description: set the enclosure buffer by buffer id
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  buf_id - buffer id
 *			  buf - buffer
 *			  len - buffer length
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/05/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_buf_by_buf_id(fbe_api_terminator_device_handle_t enclosure_handle,
                                                fbe_u8_t buf_id,
                                                fbe_u8_t *buf,
                                                fbe_u32_t len)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_set_buf_by_buf_id_ioctl_t set_buf_by_buf_id_ioctl;

	set_buf_by_buf_id_ioctl.enclosure_handle = enclosure_handle;
	set_buf_by_buf_id_ioctl.buf_id = buf_id;
	memcpy(set_buf_by_buf_id_ioctl.buf, buf, len);
	set_buf_by_buf_id_ioctl.len = len;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_BUF_BY_BUF_ID,
															&set_buf_by_buf_id_ioctl,
															sizeof (fbe_terminator_set_buf_by_buf_id_ioctl_t),
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
 *            fbe_api_terminator_set_buf ()
 *********************************************************************
 *
 *  Description: set the enclosure buffer
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  subencl_type - sub enclosure type
 *            side - side
 *            buf_type - buffer type
 *			  buf - buffer
 *			  len - buffer length
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/06/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_buf(fbe_api_terminator_device_handle_t enclosure_handle,
                                        ses_subencl_type_enum subencl_type,
                                        fbe_u8_t side,
                                        ses_buf_type_enum buf_type,
                                        fbe_u8_t *buf,
                                        fbe_u32_t len)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_set_buf_ioctl_t set_buf_ioctl;

	set_buf_ioctl.enclosure_handle = enclosure_handle;
	set_buf_ioctl.subencl_type = subencl_type;
	set_buf_ioctl.side = side;
	set_buf_ioctl.buf_type = buf_type;
	memcpy(set_buf_ioctl.buf, buf, len);
	set_buf_ioctl.len = len;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_BUF,
															&set_buf_ioctl,
															sizeof (fbe_terminator_set_buf_ioctl_t),
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
 *            fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc ()
 *********************************************************************
 *
 *  Description: set download micorcode status page status description
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  download_stat_desc - download status description
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/10/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(fbe_api_terminator_device_handle_t enclosure_handle,
                                                                                fbe_download_status_desc_t download_stat_desc)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl_t eses_set_download_microcode_stat_page_stat_desc_ioctl;

	eses_set_download_microcode_stat_page_stat_desc_ioctl.enclosure_handle = enclosure_handle;
	eses_set_download_microcode_stat_page_stat_desc_ioctl.download_stat_desc = download_stat_desc;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ESES_SET_DOWNLOAD_MICROCODE_STAT_PAGE_STAT_DESC,
															&eses_set_download_microcode_stat_page_stat_desc_ioctl,
															sizeof (fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl_t),
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
 *            fbe_api_terminator_get_emcEnclStatus ()
 *********************************************************************
 *
 *  Description: get enclosure EMC enclosure status.
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  emcEnclStatusPtr - new value for EMC Enclosure status
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/01/11: Joe Perry	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_emcEnclStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                  ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_enclosure_emc_encl_status_ioctl_t eses_set_shutdown_reason_ioctl;

	eses_set_shutdown_reason_ioctl.enclosure_handle = enclosure_handle;
    eses_set_shutdown_reason_ioctl.emcEnclStatus = *emcEnclStatusPtr;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_GET_EMC_ENCL_STATUS,
															&eses_set_shutdown_reason_ioctl,
															sizeof (fbe_terminator_enclosure_emc_encl_status_ioctl_t),
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
 *            fbe_api_terminator_set_emcEnclStatus ()
 *********************************************************************
 *
 *  Description: set enclosure EMC Enclosure status
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  shutdownReason - new value for EMC Enclosure status
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/01/11: Joe Perry	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_emcEnclStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                  ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_enclosure_emc_encl_status_ioctl_t eses_set_shutdown_reason_ioctl;

	eses_set_shutdown_reason_ioctl.enclosure_handle = enclosure_handle;
    eses_set_shutdown_reason_ioctl.emcEnclStatus = *emcEnclStatusPtr;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_SET_EMC_ENCL_STATUS,
															&eses_set_shutdown_reason_ioctl,
															sizeof (fbe_terminator_enclosure_emc_encl_status_ioctl_t),
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
 *            fbe_api_terminator_get_emcPsInfoStatus ()
 *********************************************************************
 *
 *  Description: get enclosure EMC PS Info status.
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  emcPsInfoStatusPtr - new value for EMC Ps Info status
 *
 *  Return Value: success or failure
 *
 *  History:
 *    01/03/12: Joe Perry	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_emcPsInfoStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                    terminator_eses_ps_id ps_id,
                                                    ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    fbe_terminator_enclosure_emc_ps_info_status_ioctl_t eses_emc_ps_info_status_ioctl;

    eses_emc_ps_info_status_ioctl.enclosure_handle = enclosure_handle;
    eses_emc_ps_info_status_ioctl.psIndex = ps_id;
    eses_emc_ps_info_status_ioctl.emcPsInfoStatus = *emcPsInfoStatusPtr;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_GET_EMC_PS_INFO_STATUS,
                                                            &eses_emc_ps_info_status_ioctl,
                                                            sizeof (fbe_terminator_enclosure_emc_ps_info_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_emcPsInfoStatus ()
 *********************************************************************
 *
 *  Description: set enclosure EMC PS Info status
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  emcPsInfoStatusPtr - new value for EMC PS Info status
 *
 *  Return Value: success or failure
 *
 *  History:
 *    01/03/12: Joe Perry	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_emcPsInfoStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                    terminator_eses_ps_id ps_id,
                                                    ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    fbe_terminator_enclosure_emc_ps_info_status_ioctl_t eses_emc_ps_info_status_ioctl;

    eses_emc_ps_info_status_ioctl.enclosure_handle = enclosure_handle;
    eses_emc_ps_info_status_ioctl.psIndex = ps_id;
    eses_emc_ps_info_status_ioctl.emcPsInfoStatus = *emcPsInfoStatusPtr;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_SET_EMC_PS_INFO_STATUS,
                                                            &eses_emc_ps_info_status_ioctl,
                                                            sizeof (fbe_terminator_enclosure_emc_ps_info_status_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_eses_set_ver_desc ()
 *********************************************************************
 *
 *  Description: set ver description
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  subencl_type - subenclosure type
 *			  side - side
 *			  comp_type - comp type
 *			  ver_desc - ver description
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/10/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_eses_set_ver_desc(fbe_api_terminator_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t  side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct ver_desc)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_eses_set_ver_desc_ioctl_t eses_set_ver_desc_ioctl;

	eses_set_ver_desc_ioctl.enclosure_handle = enclosure_handle;
	eses_set_ver_desc_ioctl.subencl_type = subencl_type;
	eses_set_ver_desc_ioctl.side = side;
	eses_set_ver_desc_ioctl.comp_type = comp_type;
	eses_set_ver_desc_ioctl.ver_desc = ver_desc;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ESES_SET_VER_DESC,
															&eses_set_ver_desc_ioctl,
															sizeof (fbe_terminator_eses_set_ver_desc_ioctl_t),
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
 *            fbe_api_terminator_eses_get_ver_desc ()
 *********************************************************************
 *
 *  Description: get ver description
 *
 *	Inputs: enclosure_handle - handle of the enclosure
 *			  subencl_type - subenclosure type
 *			  side - side
 *			  comp_type - comp type
 *			  ver_desc - buffer of ver description
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/10/09: gaob3	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_eses_get_ver_desc(fbe_api_terminator_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t  side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct *ver_desc)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_eses_get_ver_desc_ioctl_t eses_get_ver_desc_ioctl;

	eses_get_ver_desc_ioctl.enclosure_handle = enclosure_handle;
	eses_get_ver_desc_ioctl.subencl_type = subencl_type;
	eses_get_ver_desc_ioctl.side = side;
	eses_get_ver_desc_ioctl.comp_type = comp_type;
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_ESES_GET_VER_DESC,
															&eses_get_ver_desc_ioctl,
															sizeof (fbe_terminator_eses_get_ver_desc_ioctl_t),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}
	*ver_desc = eses_get_ver_desc_ioctl.ver_desc;
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_pull_enclosure ()
 *********************************************************************
 *
 *  Description: pull an enclosure from Terminator
 *
 *  Inputs:
 *      enclosure_handle - handle of the enclosure
 *
 *  Return Value: success or failure
 *
 *  History:
 *    04/02/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_pull_enclosure (fbe_api_terminator_device_handle_t enclosure_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_pull_enclosure_ioctl_t pull_enclosure_ioctl;

    pull_enclosure_ioctl.enclosure_handle = enclosure_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_PULL_ENCLOSURE,
                                                            &pull_enclosure_ioctl,
                                                            sizeof (fbe_terminator_pull_enclosure_ioctl_t),
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
 *            fbe_api_terminator_reinsert_enclosure ()
 *********************************************************************
 *
 *  Description: reinsert an existing enclosure to Terminator
 *
 *  Inputs:
 *      port_handle - handle of the port
 *      enclosure_handle - handle of the enclosure
 *
 *  Return Value: success or failure
 *
 *  History:
 *    04/02/10: gaob3   created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_reinsert_enclosure (fbe_api_terminator_device_handle_t port_handle,
                                                 fbe_api_terminator_device_handle_t enclosure_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_reinsert_enclosure_ioctl_t reinsert_enclosure_ioctl;

    reinsert_enclosure_ioctl.port_handle = port_handle;
    reinsert_enclosure_ioctl.enclosure_handle = enclosure_handle;
    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_REINSERT_ENCLOSURE,
                                                            &reinsert_enclosure_ioctl,
                                                            sizeof (fbe_terminator_reinsert_enclosure_ioctl_t),
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


