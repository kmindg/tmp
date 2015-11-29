/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_system_time_threshold_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api_system_time_threshold_interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_system_time_threshold_interface
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_system_time_threshold_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_job_notification.h"

#define FBE_API_TIME_THRESHOLD_JOB_WAIT_TIMEOUT	8000

fbe_status_t fbe_api_set_system_time_threshold_info(fbe_system_time_threshold_info_t * time_threshold_info)
{
	fbe_status_t                                   		 	status;
    fbe_api_job_service_set_system_time_threshold_info_t		system_time_threshold_job;
    fbe_job_service_error_type_t							error_type;
	fbe_status_t											job_status;

    if (time_threshold_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_copy_memory(&system_time_threshold_job.system_time_threshold_info, time_threshold_info, sizeof(fbe_system_time_threshold_info_t));

    /*send the command that will eventually start the job*/
	status = fbe_api_job_service_set_system_time_threshold_info(&system_time_threshold_job);
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
	/*Wait for the job to complete*/
	status = fbe_api_common_wait_for_job(system_time_threshold_job.job_number, FBE_API_TIME_THRESHOLD_JOB_WAIT_TIMEOUT, &error_type, &job_status, NULL);
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
 * @fn fbe_api_get_system_time_threshold_info()
 ****************************************************************
 * @brief
 *  This function gets system time threshold information
 *
 * @param time_threshold_info - pointer to the strucutre to get time threshold information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_system_time_threshold_info(fbe_system_time_threshold_info_t * time_threshold_info)
{
	fbe_status_t                                    status;
	fbe_database_time_threshold_t                   out_time_threshold;

    if (time_threshold_info == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL buffer pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	status = fbe_api_database_get_time_threshold(&out_time_threshold);
	
    if (status != FBE_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to get time threshold\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
		time_threshold_info->time_threshold_in_minutes = out_time_threshold.system_time_threshold_info.time_threshold_in_minutes;
        return FBE_STATUS_OK;

    }
}


