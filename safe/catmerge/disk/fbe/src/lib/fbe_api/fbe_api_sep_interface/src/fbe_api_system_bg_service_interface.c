/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_system_bg_service_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api_system_bg_service_interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_system_bg_service_interface
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_job_notification.h"
#include "fbe/fbe_api_system_bg_service_interface.h"

#define FBE_API_CONTROL_SYSTEM_BG_SERVICE_JOB_WAIT_TIMEOUT	20000

fbe_status_t FBE_API_CALL fbe_api_control_system_bg_service(fbe_base_config_control_system_bg_service_t * system_bg_service)
{
	fbe_status_t                                   		 	status;
    fbe_api_job_service_control_system_bg_service_t	        system_bg_service_control_job;
    fbe_job_service_error_type_t							error_type;
	fbe_status_t											job_status;

    if (system_bg_service == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_copy_memory(&system_bg_service_control_job.system_bg_service, system_bg_service, sizeof(fbe_base_config_control_system_bg_service_t));

    /*send the command that will eventually start the job*/
	status = fbe_api_job_service_control_system_bg_service(&system_bg_service_control_job);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
	/*Wait for the job to complete*/
	status = fbe_api_common_wait_for_job(system_bg_service_control_job.job_number, FBE_API_CONTROL_SYSTEM_BG_SERVICE_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
	if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	if (job_status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
	}
    
	return job_status;
}

/*!***************************************************************
 * @fn fbe_api_get_system_bg_service_status
 *     (fbe_base_config_control_system_bg_service_t * system_bg_service)
 ****************************************************************
 * @brief
 *  This function gets system background service enable/disable status from
 *  the system 
 *
 * @param system_bg_service - pointer to the strucutre to get system bg service information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t fbe_api_get_system_bg_service_status(fbe_base_config_control_system_bg_service_t * system_bg_service)
{
	fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (system_bg_service == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    status = fbe_api_common_send_control_packet_to_class (FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_BG_SERVICE_STATUS,
														  system_bg_service,
														  sizeof(fbe_base_config_control_system_bg_service_t),
														  FBE_CLASS_ID_BASE_CONFIG,
														  FBE_PACKET_FLAG_NO_ATTRIB,
														  &status_info,
														  FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

    return status;

}


/*!***************************************************************
 * @fn fbe_api_enable_system_sniff_verify(void)
 ****************************************************************
 * @brief
 *  This function enables the overall system sniff verify feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_system_sniff_verify(void)
{
	fbe_status_t                                    status;
    fbe_base_config_control_system_bg_service_t	    sniff_verify_info ={0,};

	sniff_verify_info.enable = FBE_TRUE;
	sniff_verify_info.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY;
    sniff_verify_info.issued_by_ndu = FBE_FALSE;

    status = fbe_api_control_system_bg_service(&sniff_verify_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to enable system sniff verify.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_disable_system_sniff_verify(void)
 ****************************************************************
 * @brief
 *  This function disables the overall system sniff verify feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_system_sniff_verify(void)
{
	fbe_status_t                                        status;
    fbe_base_config_control_system_bg_service_t			sniff_verify_info;

	sniff_verify_info.enable = FBE_FALSE;
	sniff_verify_info.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY;
    sniff_verify_info.issued_by_ndu = FBE_FALSE;

    status = fbe_api_control_system_bg_service(&sniff_verify_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to disable system sniff verify.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_system_sniff_verify_is_enabled(void)
 ****************************************************************
 * @brief
 *  This function checks if the overall system sniff verify is enabled.
 *
 * @return
 *  fbe_bool_t - FBE_TRUE(enabled) or FBE_FALSE(not enabled).
 ****************************************************************/
fbe_bool_t FBE_API_CALL fbe_api_system_sniff_verify_is_enabled(void)
{
	fbe_status_t                                        status;
    fbe_base_config_control_system_bg_service_t			sniff_verify_info;

	sniff_verify_info.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_SNIFF_VERIFY;

    status = fbe_api_get_system_bg_service_status(&sniff_verify_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system sniff verify status if it's enabled or not.\n", __FUNCTION__);
        //return FBE_STATUS_GENERIC_FAILURE;
    }

    return sniff_verify_info.enable; 

}


/*!***************************************************************
 * @fn fbe_status_t fbe_api_enable_all_bg_services(void)
 ****************************************************************
 * @brief
 *  This function enables the all system background services feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_all_bg_services(void)
{
	fbe_status_t                                        status;
    fbe_base_config_control_system_bg_service_t			bg_service_info;

	bg_service_info.enable = FBE_TRUE;
	bg_service_info.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
	bg_service_info.issued_by_ndu = FBE_FALSE;

    status = fbe_api_control_system_bg_service(&bg_service_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to enable all system background services.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_disable_all_bg_services(void)
 ****************************************************************
 * @brief
 *  This function disables the all system background services feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_all_bg_services(void)
{
	fbe_status_t                                        status;
    fbe_base_config_control_system_bg_service_t			bg_service_info;

	bg_service_info.enable = FBE_FALSE;
	bg_service_info.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
	bg_service_info.issued_by_ndu = FBE_FALSE;

    status = fbe_api_control_system_bg_service(&bg_service_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to disable all system background services.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_all_bg_services_are_enabled(void)
 ****************************************************************
 * @brief
 *  This function checks if the all system background services are enabled.
 *
 * @return
 *  fbe_bool_t - FBE_TRUE(enabled) or FBE_FALSE(not enabled).
 ****************************************************************/
fbe_bool_t FBE_API_CALL fbe_api_all_bg_services_are_enabled(void)
{
	fbe_status_t                                        status;
    fbe_base_config_control_system_bg_service_t			bg_service_info;

	bg_service_info.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;

    status = fbe_api_get_system_bg_service_status(&bg_service_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system background services status if it's enabled or not.\n", __FUNCTION__);
        //return FBE_STATUS_GENERIC_FAILURE;
    }

    return bg_service_info.enable; 

}


/*!***************************************************************
 * @fn fbe_status_t fbe_api_enable_all_bg_services_single_sp(void)
 ****************************************************************
 * @brief
 *  This function enables all the background services on this SP only
 *  it should be used by NDU only via Admin SetNDUInProgress function.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK if success
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_all_bg_services_single_sp(void)
{
	fbe_status_t                                    status;
    fbe_base_config_control_system_bg_service_t	    control_bgs;
	fbe_api_control_operation_status_info_t         status_info;

	control_bgs.enable = FBE_TRUE;
	control_bgs.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
	control_bgs.issued_by_ndu = FBE_TRUE;

	 status = fbe_api_common_send_control_packet_to_class (FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
														  &control_bgs,
														  sizeof(fbe_base_config_control_system_bg_service_t),
														  FBE_CLASS_ID_BASE_CONFIG,
														  FBE_PACKET_FLAG_NO_ATTRIB,
														  &status_info,
														  FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_disable_all_bg_services_single_sp(void)
 ****************************************************************
 * @brief
 *  This function disables all the background services on this SP only
 *  it should be used by NDU only via Admin SetNDUInProgress function.
 *  When we use it from here, it would initiate the schduler safety valve timer
 *  that checks if we did not turn it on for a long time
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK if success
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_all_bg_services_single_sp(void)
{
	fbe_status_t                                    status;
    fbe_base_config_control_system_bg_service_t	    control_bgs;
	fbe_api_control_operation_status_info_t         status_info;

	control_bgs.enable = FBE_FALSE;
	control_bgs.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_NDU;
	control_bgs.issued_by_ndu = FBE_TRUE;

	status = fbe_api_common_send_control_packet_to_class (FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
														  &control_bgs,
														  sizeof(fbe_base_config_control_system_bg_service_t),
														  FBE_CLASS_ID_BASE_CONFIG,
														  FBE_PACKET_FLAG_NO_ATTRIB,
														  &status_info,
														  FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status != FBE_STATUS_OK) {
			return status;
		}else{
			return FBE_STATUS_GENERIC_FAILURE;
		}
    }

    return status;

}
