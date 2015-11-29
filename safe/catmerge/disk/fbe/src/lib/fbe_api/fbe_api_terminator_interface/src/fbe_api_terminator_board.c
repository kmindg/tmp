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
#include "fbe/fbe_terminator_service.h"
/**************************************
				Local variables
**************************************/

/*******************************************
				Local functions
********************************************/


/*********************************************************************
 *            fbe_api_terminator_insert_board ()
 *********************************************************************
 *
 *  Description: insert a board in Terminator
 *
 *	Inputs: board_type - type of board to create
 *
 *  Return Value: success or failue
 *
 *  History:
 *    09/09/09: guov	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_insert_board(fbe_terminator_board_info_t * board_info)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
	fbe_terminator_insert_board_ioctl_t 	insert_board;

	memcpy(&insert_board.board_info, board_info, sizeof(fbe_terminator_board_info_t));
	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INSERT_BOARD,
															&insert_board,
															sizeof (fbe_terminator_insert_board_ioctl_t),
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
 *            fbe_api_terminator_send_specl_sfi_mask_data ()
 *********************************************************************
 *
 *  Description: send SPECL SFI mask data to change device status on the board
 *
 *  Inputs: sfi_mask_data - pointer to the SPECL SFI mask data
 *
 *  Return Value: success or failue
 *
 *  History:
 *    07/28/10: Bo Gao  created
 *
 *********************************************************************/
fbe_status_t fbe_api_terminator_send_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_terminator_send_specl_sfi_mask_data_ioctl_t   send_specl_sfi_mask_data_ioctl;

    memcpy(&send_specl_sfi_mask_data_ioctl.specl_sfi_mask_data, sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SEND_SPECL_SFI_MASK_DATA,
                                                            &send_specl_sfi_mask_data_ioctl,
                                                            sizeof (fbe_terminator_send_specl_sfi_mask_data_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    memcpy(sfi_mask_data, &send_specl_sfi_mask_data_ioctl.specl_sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

	return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_terminator_set_sp_id ()
 *********************************************************************
 *
 *  Description: set the SP ID of the terminator
 *
 *	Inputs: terminator_sp_id_t - which sp
 *
 *  Return Value: success or failue
 *
 *  History:
 *    01/09/10: sharel	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_set_sp_id(terminator_sp_id_t sp_id)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
    fbe_terminator_miniport_sp_id_ioctl_t	set_sp_id;

	set_sp_id.sp_id = sp_id;

	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SP_ID,
															&set_sp_id,
															sizeof (fbe_terminator_miniport_sp_id_ioctl_t),
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
 *            fbe_api_terminator_get_sp_id ()
 *********************************************************************
 *
 *  Description: get the SP ID of the terminator
 *
 *	Inputs: terminator_sp_id_t - what is the sp id ?
 *
 *  Return Value: success or failue
 *
 *  History:
 *    01/09/10: sharel	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_get_sp_id(terminator_sp_id_t *sp_id)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;
    fbe_terminator_miniport_sp_id_ioctl_t	get_sp_id;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_GET_SP_ID,
															&get_sp_id,
															sizeof (fbe_terminator_miniport_sp_id_ioctl_t),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

	*sp_id = get_sp_id.sp_id;

	return FBE_STATUS_OK;
}

fbe_status_t fbe_api_terminator_set_single_sp(fbe_bool_t is_single)
{
    fbe_status_t                               status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t    status_info;
    fbe_terminator_sp_single_or_dual_ioctl_t   set_sp_single_or_dual = {0};

    set_sp_single_or_dual.is_single = is_single;

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_SET_SINGLE_SP,
                                                            &set_sp_single_or_dual,
                                                            sizeof (fbe_terminator_sp_single_or_dual_ioctl_t),
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

fbe_status_t fbe_api_terminator_is_single_sp_system(fbe_bool_t *is_single)
{
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_terminator_sp_single_or_dual_ioctl_t is_single_or_dual_sp = {0};

    status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_IS_SINGLE_SP_SYSTEM,
                                                            &is_single_or_dual_sp,
                                                            sizeof (fbe_terminator_sp_single_or_dual_ioctl_t),
                                                            FBE_SERVICE_ID_TERMINATOR,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *is_single = is_single_or_dual_sp.is_single;

    return FBE_STATUS_OK;
}
