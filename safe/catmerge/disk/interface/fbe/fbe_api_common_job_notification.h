#ifndef FBE_API_COMMON_JOB_NOTIFICATION_H
#define FBE_API_COMMON_JOB_NOTIFICATION_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_common_job_notifiation.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions used to get notifications from the job
 *  service and let FBE APIs wait for these notifications
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"

FBE_API_CPP_EXPORT_START
fbe_status_t FBE_API_CALL fbe_api_common_init_job_notification(void);
fbe_status_t FBE_API_CALL fbe_api_common_destroy_job_notification(void);

/* When the job is LUN/RG creation, the notification will send back the object_id */
fbe_status_t FBE_API_CALL fbe_api_common_wait_for_job(fbe_u64_t job_number,
										 fbe_u32_t timeout_in_ms,
										 /*OUT*/fbe_job_service_error_type_t *job_error_code,
										 /*OUT*/fbe_status_t * job_status,
                                         /*OUT*/fbe_object_id_t *object_id);

void FBE_API_CALL fbe_api_common_clean_job_notification_queue(void);

FBE_API_CPP_EXPORT_END

#endif/*FBE_API_COMMON_JOB_NOTIFICATION_H*/
