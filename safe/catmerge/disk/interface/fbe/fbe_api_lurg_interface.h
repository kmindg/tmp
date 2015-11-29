#ifndef FBE_API_LURG_INTERFACE_H
#define FBE_API_LURG_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_lurg_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_api interface for the LUN/RG creation 
 *  and deletion.
 *
 * @ingroup fbe_api_neit_package_interface_class_files
 *
 * @version
 *  4/21/2010 - Created. Sanjay Bhave
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_types.h"
#include "fbe_service_manager.h"
#include "fbe_topology_interface.h"

/** Job completion callback data. */
/* @todo there's no way to attach this to a specific job? */
typedef struct fbe_lurg_job_s {
	fbe_semaphore_t                     sem;
	fbe_notification_registration_id_t  reg_id;
	fbe_job_service_info_t              job_service_error_info;
} fbe_lurg_job_t;


/* Kick the job's semaphore on completion. */
static void job_callback(update_object_msg_t *update_object_msg, void *context)
{
	fbe_lurg_job_t *job = context;
	fbe_notification_type_t type = update_object_msg->notification_info.notification_type;

	fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:\tnotification %llu\n",
		      __FUNCTION__, (unsigned long long)job->reg_id);
	if (type != FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:\tspurious notification %llu expected %d\n", __FUNCTION__,
			(unsigned long long)type,
			FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED);
		return;
	}
	job->job_service_error_info = update_object_msg->notification_info.notification_data.job_service_error_info;
	fbe_semaphore_release(&job->sem, 0, 1, FALSE);
}

/** Initialize a job ticket. */
static fbe_status_t job_init(fbe_lurg_job_t *job)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

	fbe_semaphore_init(&job->sem, 0, 1);
	job->job_service_error_info.status = FBE_STATUS_INVALID;
	job->job_service_error_info.error_code = FBE_JOB_SERVICE_ERROR_INVALID;
	
	status = fbe_api_notification_interface_register_notification(
		FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
		FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
		FBE_TOPOLOGY_OBJECT_TYPE_ALL,
		job_callback,
		job,
		&job->reg_id);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:\tregistration fail %d\n", __FUNCTION__, status);
		return status;
	}
	return status;
}

/* Wait on a job completion. */
static fbe_status_t job_destroy(fbe_lurg_job_t *job)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
	status = fbe_semaphore_wait_ms(&job->sem, 10000);
	if (status != FBE_STATUS_OK) {
		if (status == FBE_STATUS_TIMEOUT) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s:\ttimed out waiting for job\n", __FUNCTION__);
		}else{
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s:\twait fail %d\n", __FUNCTION__, status);
		}

		status = fbe_api_notification_interface_unregister_notification(job_callback, job->reg_id);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s:\tunregistration fail %d\n", __FUNCTION__, status);
		}

		return FBE_STATUS_GENERIC_FAILURE;
	}
	fbe_semaphore_destroy(&job->sem);

	status = fbe_api_notification_interface_unregister_notification(job_callback, job->reg_id);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s:\tunregistration fail %d\n", __FUNCTION__, status);
		return status;
	}

	if ((job->job_service_error_info.status != FBE_STATUS_OK) && job->job_service_error_info.error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s:\t got %d %d expected %d\n", __FUNCTION__,
			job->job_service_error_info.status,
			job->job_service_error_info.error_code,
			FBE_STATUS_OK);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:\tdone\n", __FUNCTION__);
	return FBE_STATUS_OK;
}
#endif /* FBE_API_LURG_INTERFACE_H */

/*************************
 * end file fbe_api_lurg_interface.h
 *************************/
