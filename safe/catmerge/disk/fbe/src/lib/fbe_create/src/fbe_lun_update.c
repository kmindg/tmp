/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_update_lun.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines to update the lun configuration 
 *  using job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   03/09/2011:  Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_base_config.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_lun_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_lun_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_lun_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_lun_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_lun_commit_function(fbe_packet_t *packet_p);

static fbe_status_t fbe_update_lun_configuration(fbe_database_control_update_lun_t update_lun);

static fbe_status_t fbe_update_lun_send_notification(fbe_job_service_lun_update_t * update_lun_p,
                                                     fbe_u64_t job_number,
                                                     fbe_job_service_error_type_t job_service_error_type,
                                                     fbe_job_action_state_t job_action_state);


/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_lun_job_service_operation = 
{
    FBE_JOB_TYPE_LUN_UPDATE,
    {
        /* validation function */
        fbe_update_lun_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_lun_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_lun_persist_function,

        /* response/rollback function */
        fbe_update_lun_rollback_function,

        /* commit function */
        fbe_update_lun_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 * fbe_update_lun_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the update provision drive configuration 
 *  job service command.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  03/09/2011 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_validation_function(fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_job_service_lun_update_t *              update_lun_p = NULL;
    fbe_bool_t                                  double_degraded;
    
    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    update_lun_p = (fbe_job_service_lun_update_t *) job_queue_element_p->command_data;
    if(update_lun_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* first found out if this is a valid object id */
    if(update_lun_p->update_lun.object_id == FBE_OBJECT_ID_INVALID)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s LUN %d does not exist.\n", __FUNCTION__, update_lun_p->update_lun.object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_UNKNOWN_ID;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Check the number of degraded db drives, if more than one drive degraded, we can't update lun*/
    status = fbe_create_lib_validate_system_triple_mirror_healthy(&double_degraded);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s check system triple mirror healthy failed.\n", 
                __FUNCTION__);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (double_degraded == FBE_TRUE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s More than one DB drive degraded.\n", 
                __FUNCTION__);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* add additional validations here. */


    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // no one uses status.

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_update_lun_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_lun_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive config type in memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  03/09/2011 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_job_service_lun_update_t *              update_lun_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    update_lun_p = (fbe_job_service_lun_update_t *) job_queue_element_p->command_data;
    if(update_lun_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_start_database_transaction(&update_lun_p->update_lun.transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start lun update config transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* update the lun. */
    status = fbe_update_lun_configuration(update_lun_p->update_lun);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_lun_update_configuration_in_memory_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_lun_rollback_function()
 ******************************************************************************
 * @brief
 *  This function is used to rollback to configuration update for the provision
 *  drive config type.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  03/09/2011 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_rollback_function(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_status_t                payload_status;
    fbe_job_service_lun_update_t *  update_lun_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    update_lun_p = (fbe_job_service_lun_update_t *)job_queue_element_p->command_data;
    if(update_lun_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_VALIDATE)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s entry, rollback not required, previous state %d, current state %d\n", __FUNCTION__, 
                job_queue_element_p->previous_state,
                job_queue_element_p->current_state);
    }

    if((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
      (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY))
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: aborting transaction id %llu\n", __FUNCTION__,
			  (unsigned long long)update_lun_p->update_lun.transaction_id);

        /* rollback the configuration transaction. */
        status = fbe_create_lib_abort_database_transaction(update_lun_p->update_lun.transaction_id);
        if(status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Could not abort transaction id %llu with configuration service\n", 
                              __FUNCTION__,
			      (unsigned long long)update_lun_p->update_lun.transaction_id);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }
    }

    /* Send a notification for this rollback. */
    status = fbe_update_lun_send_notification(update_lun_p,
                                              job_queue_element_p->job_number,
                                              job_queue_element_p->error_code,
                                              FBE_JOB_ACTION_STATE_ROLLBACK);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: status change notification failed, status:%d\n",
                __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* set the payload status to ok if it is not updated as failure. */
    fbe_payload_control_get_status(control_operation_p, &payload_status);
    if(payload_status != FBE_PAYLOAD_CONTROL_STATUS_FAILURE)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_lun_rollback_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_lun_persist_function()
 ******************************************************************************
 * @brief
 *  This function is used to persist the updated provision drive config type
 *  in configuration database.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  03/09/2011 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_lun_update_t *  update_lun_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    
    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    update_lun_p = (fbe_job_service_lun_update_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(update_lun_p->update_lun.transaction_id);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to commit, status:%d\n",
                          __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_update_lun_persist_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_lun_commit_function()
 ******************************************************************************
 * @brief
 *  This function is used to commit the configuration update to the provision
 *  drive object. It sends notification to the notification service that job
 *  is completed.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  03/09/2011 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_job_service_lun_update_t *      update_lun_p = NULL;
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_status_t                    payload_status;
    fbe_payload_control_buffer_length_t             length = 0;
    fbe_job_queue_element_t *                       job_queue_element_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    update_lun_p = (fbe_job_service_lun_update_t *) job_queue_element_p->command_data;

    /* log the trace for the config update.*/
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: lun object's config updated\n",
                      __FUNCTION__);

    /* Send a notification for this commit*/
    status = fbe_update_lun_send_notification(update_lun_p,
                                              job_queue_element_p->job_number,
                                              FBE_JOB_SERVICE_ERROR_NO_ERROR,
                                              FBE_JOB_ACTION_STATE_COMMIT);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: status change notification failed, status %d\n",
                          __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* set the payload status to ok if it is not updated as failure. */
    fbe_payload_control_get_status(control_operation_p, &payload_status);
    if(payload_status != FBE_PAYLOAD_CONTROL_STATUS_FAILURE)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    /* update job_queue_element status */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = 
        (status==FBE_STATUS_OK)? FBE_JOB_SERVICE_ERROR_NO_ERROR:FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    job_queue_element_p->object_id = update_lun_p->update_lun.object_id;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_update_lun_commit_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_lun_configuration()
 ******************************************************************************
 * @brief
 * This function is used to update the lun object.
 *
 * @param update_lun    - update lun.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  03/11/2011 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_configuration(fbe_database_control_update_lun_t update_lun)
{
    fbe_status_t                      status = FBE_STATUS_GENERIC_FAILURE; 

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: lun-obj-id: %d\n",
                      __FUNCTION__,
                      update_lun.object_id);

    /* Send the update provision drive configuration to database service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_LUN,
                                                &update_lun,
                                                sizeof(fbe_database_control_update_lun_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_LUN failed\n", __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_update_lun_configuration()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_lun_send_notification()
 ******************************************************************************
 * @brief
 * Helper function to send the notification for the spare library.
 * 
 * @param js_swap_request_p        - Job service swap request.
 * @param job_number               - job_number.
 * @param job_service_error_type   - Job service error type.
 * @param job_action_state         - Job action state.
 * 
 * @return status                   - The status of the operation.
 *
 * @author
 *  09/30/2010 - Created. 
 ******************************************************************************/
static fbe_status_t
fbe_update_lun_send_notification(fbe_job_service_lun_update_t * update_lun_p,
                                             fbe_u64_t job_number,
                                             fbe_job_service_error_type_t job_service_error_type,
                                             fbe_job_action_state_t job_action_state)
{
    fbe_notification_info_t     notification_info;
    fbe_status_t                status;

    /* fill up the notification information before sending notification. */
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = update_lun_p->update_lun.object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_service_error_type;
    notification_info.notification_data.job_service_error_info.job_number = job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_LUN_UPDATE;
    notification_info.class_id = FBE_CLASS_ID_LUN;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN; 
    //notification_info.notification_data.job_action_state = job_action_state;

    /* trace some information
     */
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "UPDATE: lun config notification lun obj: 0x%x type: %d job: 0x%llx\n",
                      update_lun_p->update_lun.object_id, update_lun_p->update_lun.update_type, 
                      (unsigned long long)job_number);


    /* send the notification to registered callers. */
    status = fbe_notification_send(update_lun_p->update_lun.object_id, notification_info);
    return status;
}
/******************************************************************************
 * end fbe_update_lun_send_notification()
 ******************************************************************************/

