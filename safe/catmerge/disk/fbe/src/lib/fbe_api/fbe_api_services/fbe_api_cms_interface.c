/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************/
/** @file fbe_api_cms_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the cluster memory interfaces for external users.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_cms_interface
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_cms.h"
#include "fbe/fbe_cms_interface.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/



/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

/****************************************************************/
/* send_control_packet_to_cms_service()
 ****************************************************************
 * @brief
 *  This function starts a transaction
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param function - pointer to the function
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
static fbe_status_t send_control_packet_to_cms(fbe_payload_control_operation_opcode_t control_code,
																	fbe_payload_control_buffer_t           buffer,
																	fbe_payload_control_buffer_length_t    buffer_length)
{
    fbe_api_control_operation_status_info_t  status_info;

    fbe_status_t status = fbe_api_common_send_control_packet_to_service (control_code, buffer, buffer_length,
                                                                         FBE_SERVICE_ID_CMS,
                                                                         FBE_PACKET_FLAG_NO_ATTRIB, &status_info,
                                                                         FBE_PACKAGE_ID_SEP_0);
    
    if (status == FBE_STATUS_NO_OBJECT && status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                       "%s:No object found!!\n",
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }else if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status,
                       status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK){
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

    return status;
}


/*****************************************************************/
/** fbe_api_cms_get_info(fbe_cms_get_info_t *get_info)
 ****************************************************************
 * @brief
 *  This function gets generic information from the cluster memory service
 *
 * @param fbe_cms_get_info_t(in) - pointer to get data
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_cms_get_info(fbe_cms_get_info_t *get_info)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    
    
    if (get_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_cms (FBE_CMS_CONTROL_CODE_GET_INFO,
																get_info, sizeof(fbe_cms_get_info_t));

    return status;
}


/*****************************************************************/
/** fbe_api_cms_get_state_machine_history(fbe_cms_control_get_sm_history_t *history)
 ****************************************************************
 * @brief
 *  This function gets the table with the latest transitions of the state machine
 *
 * @param history(in) - pointer to get data
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_cms_get_state_machine_history(fbe_cms_control_get_sm_history_t *history)
{
	fbe_status_t                            status = FBE_STATUS_OK;
    
    
    if (history == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL output buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = send_control_packet_to_cms (FBE_CMS_CONTROL_CODE_GET_STATE_MACHINE_HISTORY,
										 history, sizeof(fbe_cms_control_get_sm_history_t));

    return status;


}



