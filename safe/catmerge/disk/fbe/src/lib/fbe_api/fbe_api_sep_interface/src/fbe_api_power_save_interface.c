/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_power_save_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api_power_save_interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_power_save_interface
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_job_notification.h"
#include "fbe/fbe_api_database_interface.h"

#define FBE_API_POWER_SAVE_JOB_WAIT_TIMEOUT	30000 

static fbe_status_t fbe_api_set_system_power_save_info(fbe_system_power_saving_info_t * power_save_info)
{
	fbe_status_t                                   		 	status;
    fbe_api_job_service_change_system_power_save_info_t		system_ps_change_job;
    fbe_job_service_error_type_t							error_type;
	fbe_status_t											job_status;

    if (power_save_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_copy_memory(&system_ps_change_job.system_power_save_info, power_save_info, sizeof(fbe_system_power_saving_info_t));

    /*send the command that will eventually start the job*/
	status = fbe_api_job_service_set_system_power_save_info(&system_ps_change_job);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
	/*Wait for the job to complete*/
	status = fbe_api_common_wait_for_job(system_ps_change_job.job_number, FBE_API_POWER_SAVE_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
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
 * @fn fbe_api_get_system_power_save_info(fbe_system_power_saving_info_t * power_save_info)
 ****************************************************************
 * @brief
 *  This function gets power save information related information from
 *  the systm
 *
 * @param power_save_info - pointer to the strucutre to get power save information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_system_power_save_info(fbe_system_power_saving_info_t * power_save_info)
{
	fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (power_save_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    status = fbe_api_common_send_control_packet_to_class (FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_POWER_SAVING_INFO,
														  power_save_info,
														  sizeof(fbe_system_power_saving_info_t),
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
 * @fn fbe_status_t fbe_api_enable_object_power_save(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function enables the power saving in the object level
 *
 * @param object_id - object to enable power saving on
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_object_power_save(fbe_object_id_t object_id)
{
	fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_class_id_t									class_id;

    /*verify teh user is not trying to to change a RAID object, this one has a seperate function because it needs to persist*/
    status = fbe_api_get_object_class_id(object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (class_id > FBE_CLASS_ID_RAID_FIRST && class_id < FBE_CLASS_ID_RAID_LAST) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Wrong function for enabling RG\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_ENABLE_OBJECT_POWER_SAVE,
											     NULL,
												 0,
												 object_id,
												 FBE_PACKET_FLAG_NO_ATTRIB,
												 &status_info,
												 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
 * @fn fbe_status_t fbe_api_disable_object_power_save(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function disables the power saving in the object level
 *
 * @param object_id - object to disable power saving on
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_object_power_save(fbe_object_id_t object_id)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_class_id_t									class_id;

    /*verify teh user is not trying to to change a RAID object, this one has a seperate function because it needs to persist*/
    status = fbe_api_get_object_class_id(object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (class_id > FBE_CLASS_ID_RAID_FIRST && class_id < FBE_CLASS_ID_RAID_LAST) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Wrong function for disabling RG\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_OBJECT_POWER_SAVE,
												 NULL,
												 0,
												 object_id,
												 FBE_PACKET_FLAG_NO_ATTRIB,
												 &status_info,
												 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
 * @fn fbe_status_t fbe_api_get_object_power_save_info(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function returns power saving related information about the object
 *
 * @param object_id - object to disable power saving on
  *@param info - the information about the object power saving
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_power_save_info(fbe_object_id_t object_id, fbe_base_config_object_power_save_info_t *info)
{
	fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    if (info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_OBJECT_POWER_SAVE_INFO,
												 info,
												 sizeof(fbe_base_config_object_power_save_info_t),
												 object_id,
											     FBE_PACKET_FLAG_NO_ATTRIB,
												 &status_info,
												 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
 * @fn fbe_api_enable_system_power_save(void)
 ****************************************************************
 * @brief
 *  This function enables the overall system power saving feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_system_power_save(void)
{
	fbe_status_t                                    status;
    fbe_system_power_saving_info_t					power_save_info;

	status = fbe_api_get_system_power_save_info(&power_save_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	power_save_info.enabled = FBE_TRUE;
	
    status = fbe_api_set_system_power_save_info(&power_save_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_disable_system_power_save(void)
 ****************************************************************
 * @brief
 *  This function diasbales the overall system power saving feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_system_power_save(void)
{
	fbe_status_t                                    status;
    fbe_system_power_saving_info_t					power_save_info;

	status = fbe_api_get_system_power_save_info(&power_save_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	power_save_info.enabled = FBE_FALSE;
	
    status = fbe_api_set_system_power_save_info(&power_save_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_set_power_save_periodic_wakeup(fbe_u64_t minutes)
 ****************************************************************
 * @brief
 *  This function returns power saving related information about the object
 *
 * @param minutes - how often (in minutes) to wake up, do a sniff and get back to sleep
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_power_save_periodic_wakeup(fbe_u64_t minutes)
{
	fbe_status_t                                    status;
    fbe_system_power_saving_info_t					power_save_info;

	if (minutes == 0) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: 0 is not an acceptable value for wake up time\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_get_system_power_save_info(&power_save_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	power_save_info.hibernation_wake_up_time_in_minutes = minutes;
	
    status = fbe_api_set_system_power_save_info(&power_save_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_set_object_power_save_idle_time(fbe_object_id_t object_id,
											   fbe_u64_t idle_time_in_seconds)
 ****************************************************************
 * @brief
 *  This function sets the time it takes an object to start power saving
 *
 * @param idle_time_in_seconds - how long in seconds to wait before hibernating
 * @param object_id - object to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_object_power_save_idle_time(fbe_object_id_t object_id, fbe_u64_t idle_time_in_seconds)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_base_config_set_object_idle_time_t			set_ps_time;
    fbe_class_id_t									class_id;

    if (idle_time_in_seconds == 0){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: 0 is too small\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*verify teh user is not trying to to change a RAID object, this one has a seperate function because it needs to persist*/
    status = fbe_api_get_object_class_id(object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id for object %u\n", __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((class_id > FBE_CLASS_ID_RAID_FIRST) && (class_id < FBE_CLASS_ID_RAID_LAST)) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Wrong function for setting RG idle time\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_ps_time.power_save_idle_time_in_seconds = idle_time_in_seconds;
	
    status = fbe_api_common_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_SET_OBJECT_POWER_SAVE_IDLE_TIME,
												 &set_ps_time,
												 sizeof(fbe_base_config_set_object_idle_time_t),
												 object_id,
											     FBE_PACKET_FLAG_NO_ATTRIB,
												 &status_info,
												 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
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
 * @fn fbe_api_enable_raid_group_power_save(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function enables the RG power saving
 *
 * @param rg_object_id - RAID object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_raid_group_power_save(fbe_object_id_t rg_object_id)
{
	fbe_class_id_t                             class_id = FBE_CLASS_ID_INVALID;
	fbe_status_t                               status = FBE_STATUS_INVALID;
	fbe_api_job_service_change_rg_info_t       change_info;
    fbe_job_service_error_type_t               error_type;
	fbe_status_t                               job_status = FBE_STATUS_INVALID;
	fbe_u64_t                                  job_number = 0;
    fbe_database_raid_group_info_t             rg_info = {0};

    /*verify the user is not trying to to change a non RAID object, this one has a seperate function because it needs to persist*/
	status = fbe_api_get_object_class_id(rg_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if (class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= FBE_CLASS_ID_RAID_LAST) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, The object id is not RG\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    /*verify if the rg is power saving capable. If not, return error to the user. */
    status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, can't get rg info\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    if (rg_info.power_save_capable != FBE_TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to enable power saving on raid group. \n", __FUNCTION__);
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
	change_info.power_save_enabled = FBE_TRUE;
	change_info.update_type = FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE;
	change_info.object_id = rg_object_id;
    
	/*issue the command to RAID*/
	status = fbe_api_job_service_change_rg_info(&change_info, &job_number);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to start update job\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(job_number, FBE_API_POWER_SAVE_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
	if (status == FBE_STATUS_GENERIC_FAILURE) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

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
 * @fn fbe_api_disable_raid_group_power_save(fbe_object_id_t object_id)
 ****************************************************************
 * @brief
 *  This function disable the RG power saving
 *
 * @param rg_object_id - RAID object ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_raid_group_power_save(fbe_object_id_t rg_object_id)
{
	fbe_class_id_t									class_id;
	fbe_status_t									status;
	fbe_api_job_service_change_rg_info_t			change_info;
    fbe_job_service_error_type_t					error_type;
	fbe_status_t									job_status;
	fbe_u64_t										job_number;

    /*verify the user is not trying to to change a non RAID object, this one has a seperate function because it needs to persist*/
	status = fbe_api_get_object_class_id(rg_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if (class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= FBE_CLASS_ID_RAID_LAST) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, The object id is not RG\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	change_info.power_save_enabled = FBE_FALSE;
	change_info.update_type = FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE;
	change_info.object_id = rg_object_id;

    /*issue command to RAID*/
	status = fbe_api_job_service_change_rg_info(&change_info, &job_number);
	if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to start update job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	/*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(job_number, FBE_API_POWER_SAVE_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
	if (status == FBE_STATUS_GENERIC_FAILURE) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

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
 * @fn fbe_api_set_raid_group_power_save_idle_time(fbe_object_id_t rg_object_id,
													 fbe_u64_t idle_time_in_seconds)
 ****************************************************************
 * @brief
 *  This function sets the time it takes an object to start power saving
 *
 * @param idle_time_in_seconds - how long in seconds to wait before hibernating
 * @param rg_object_id - RAID object ID 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_raid_group_power_save_idle_time(fbe_object_id_t rg_object_id, fbe_u64_t idle_time_in_seconds)
{
	fbe_class_id_t									class_id;
	fbe_status_t									status;
	fbe_api_job_service_change_rg_info_t			change_info;
	fbe_job_service_error_type_t					error_type;
	fbe_status_t									job_status;
	fbe_u64_t										job_number;

    /*verify the user is not trying to to change a non RAID object, this one has a seperate function because it needs to persist*/
	status = fbe_api_get_object_class_id(rg_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if (class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= FBE_CLASS_ID_RAID_LAST) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, The object id is not RG\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    change_info.power_save_idle_time_in_sec = idle_time_in_seconds;
	change_info.update_type = FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME;
	change_info.object_id = rg_object_id;

	/*issue command to RAID*/
    status = fbe_api_job_service_change_rg_info(&change_info, &job_number);
	if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to start update job\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(job_number, FBE_API_POWER_SAVE_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
	if (status == FBE_STATUS_GENERIC_FAILURE) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
	}

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
 * @fn fbe_api_enable_system_power_save_statistics(void)
 ****************************************************************
 * @brief
 *  This function enables the overall system power saving statistics
 *  feature.
 *  Note: The statistics itself is not persisted and not available
 *  on both SPs for now.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @author
 *  7/09/2012  Vera Wang - created.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enable_system_power_save_statistics(void)
{
	fbe_status_t                                    status;
    fbe_system_power_saving_info_t					power_save_info;

	status = fbe_api_get_system_power_save_info(&power_save_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	power_save_info.stats_enabled = FBE_TRUE;
	
    status = fbe_api_set_system_power_save_info(&power_save_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_status_t fbe_api_disable_system_power_save_statistics(void)
 ****************************************************************
 * @brief
 *  This function diasbales the overall system power saving statistics
 *  feature
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @author
 *  7/09/2012  Vera Wang - created.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_disable_system_power_save_statistics(void)
{
	fbe_status_t                                    status;
    fbe_system_power_saving_info_t					power_save_info;

	status = fbe_api_get_system_power_save_info(&power_save_info);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	power_save_info.stats_enabled = FBE_FALSE;
	
    status = fbe_api_set_system_power_save_info(&power_save_info);
	if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to set system info\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}


