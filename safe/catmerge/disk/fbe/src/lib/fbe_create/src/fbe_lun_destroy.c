/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_destroy.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for lun destroy used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   01/04/2010:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_notification.h"
#include "fbe/fbe_event_log_api.h"   // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h" // for message codes

/*************************
 *   FORWARD DECLARATIONS
 *************************/

/* Lun destroy library request validation function.
 */
static fbe_status_t fbe_lun_destroy_validation_function (fbe_packet_t *packet_p);
/* Lun destroy library update configuration in memory function.
 */
static fbe_status_t fbe_lun_destroy_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Lun destroy library persist configuration in db function.
 */
static fbe_status_t fbe_lun_destroy_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Lun destroy library rollback function.
 */
static fbe_status_t fbe_lun_destroy_rollback_function (fbe_packet_t *packet_p);

/* Lun destroy library commit function.
 */
static fbe_status_t fbe_lun_destroy_commit_function (fbe_packet_t *packet_p);

/* Utility function to destroy the LUN by sending the appropriate control packet to the Config Service */
static fbe_status_t fbe_lun_destroy_configuration_service_destroy_lun(fbe_object_id_t object_id, fbe_database_transaction_id_t transaction_id);

/* Utility function to get a count of the LUN Object's upstream edges from the Topology Service */
static fbe_status_t fbe_lun_destroy_get_upstream_edge_count(fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_edges);

/* Utility function to send a "prepare for destroy" usurper command to the LUN object */
static fbe_status_t fbe_lun_destroy_send_prepare_to_destroy_usurper_cmd_to_lun(fbe_object_id_t   lun_object_id);


/*************************
 *   DEFINITIONS
 *************************/

/* Job service lun destroy registration.
*/
fbe_job_service_operation_t fbe_lun_destroy_job_service_operation = 
{
    FBE_JOB_TYPE_LUN_DESTROY,
    {
        /* validation function */
        fbe_lun_destroy_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_lun_destroy_update_configuration_in_memory_function,

        /* persist function */
        fbe_lun_destroy_persist_configuration_db_function,

        /* response/rollback function */
        fbe_lun_destroy_rollback_function,

        /* commit function */
        fbe_lun_destroy_commit_function,
    }
};


/* Expected number of upstream edges for a LUN Object
*/
#define LUN_EXPECTED_NUMBER_UPSTREAM_EDGES  1


/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_lun_destroy_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/

static fbe_status_t fbe_lun_destroy_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_lun_destroy_t           *lun_destroy_request_p = NULL;
    fbe_payload_control_operation_t         *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_job_queue_element_t                 *job_queue_element_p = NULL;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_bool_t                              double_degraded;
    fbe_object_id_t                         downstream_object_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< set of downstream objects */
    fbe_u32_t                               number_of_downstream_objects;
    fbe_bool_t                              is_broken = FBE_FALSE;

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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    lun_destroy_request_p = (fbe_job_service_lun_destroy_t*)job_queue_element_p->command_data;

	/* Get the LUN object id from Config Service */
    status = fbe_create_lib_lookup_lun_object_id(lun_destroy_request_p->lun_number, 
                                                 &lun_destroy_request_p->lun_object_id);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s failed to lookup lun %d's object id; status %d\n",
                          __FUNCTION__, lun_destroy_request_p->lun_number, status);
		job_queue_element_p->status = status;
		job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ID; 
 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "%s: lookup lun %d has object id 0x%x\n",
                      __FUNCTION__, 
                      lun_destroy_request_p->lun_number, 
                      lun_destroy_request_p->lun_object_id);


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
        /*If we don't allow destroy broken lun, just return error */
        if (lun_destroy_request_p->allow_destroy_broken_lun == FBE_FALSE) {
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);

            return FBE_STATUS_GENERIC_FAILURE;
        } else { /* If we allow to destroy broken lun */ 
            /* check whether this lun is broken */
            status = fbe_create_lib_get_list_of_downstream_objects(lun_destroy_request_p->lun_object_id, 
                                        downstream_object_list,
                                        &number_of_downstream_objects);
            if (status != FBE_STATUS_OK || number_of_downstream_objects == 0)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: failed to get the downstream raid object. number of obj:%d \n", 
                        __FUNCTION__, number_of_downstream_objects);
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
            
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Check whether the raid group is broken */
            status = fbe_create_lib_get_raid_health(
                                    downstream_object_list[0], /*there is only one raid group below a lun*/
                                    &is_broken);
            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s failed to check if raid group is broken\n", 
                        __FUNCTION__);
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return status;
            }
            /* If it is not broken, we don't allow to destroy it when system 3-way mirror is double degraded*/
            if (is_broken == FBE_FALSE) {
                job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s More than one DB drive degraded.\n", 
                    __FUNCTION__);
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED;
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return status;
            }
            /* If the raid is broken,
                     * we allow to destroy this broken raid group even system 3-way mirror is double degraded */
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: system RG double degraded. but allow destroying broken LUN\n", 
                              __FUNCTION__);
        }
    }

	job_queue_element_p->status = FBE_STATUS_OK;
	job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_lun_destroy_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_destroy_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - created. guov
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_lun_destroy_t               *lun_destroy_request_p = NULL;
    fbe_payload_control_operation_t             *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t                     *job_queue_element_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_status_t                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;


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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    lun_destroy_request_p = (fbe_job_service_lun_destroy_t*)job_queue_element_p->command_data;

    lun_destroy_request_p->transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* Open a transaction with the Configuration Service */
    status = fbe_create_lib_start_database_transaction(&lun_destroy_request_p->transaction_id, job_queue_element_p->job_number);

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s cannot start destroy transaction with Config Service; LUN %d; status %d\n",
                          __FUNCTION__, lun_destroy_request_p->lun_number, status);
		
		job_queue_element_p->status = status;
		job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                      "%s: destroy LUN %d - obj0x%x\n",
                      __FUNCTION__, lun_destroy_request_p->lun_number, lun_destroy_request_p->lun_object_id);

    /* Use Configuration Service to destroy LUN object */
    status = fbe_lun_destroy_configuration_service_destroy_lun(lun_destroy_request_p->lun_object_id, lun_destroy_request_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s cannot destroy LUN %d; status %d\n",
                          __FUNCTION__, lun_destroy_request_p->lun_number, status);
		job_queue_element_p->status = status;
		job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_lun_destroy_update_configuration_in_memory_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_destroy_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_lun_destroy_t       *lun_destroy_request_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;


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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    lun_destroy_request_p = (fbe_job_service_lun_destroy_t*)job_queue_element_p->command_data;

    /* Commit the transaction */
    status = fbe_create_lib_commit_database_transaction(lun_destroy_request_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s commit transaction failed; status %d\n",
                          __FUNCTION__, status);
 		job_queue_element_p->status = status;
		job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;  
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

	/* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    
    return status;
}
/**********************************************************
 * end fbe_lun_destroy_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_destroy_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_lun_destroy_t       *lun_destroy_request_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;

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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    lun_destroy_request_p = (fbe_job_service_lun_destroy_t*)job_queue_element_p->command_data;

    /*!TODO: how will rollback know what to rollback? */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)
            && (lun_destroy_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID))
    {
        status = fbe_create_lib_abort_database_transaction(lun_destroy_request_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
    }

    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = job_queue_element_p->status;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, status: 0x%X\n",
                          __FUNCTION__, status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_lun_destroy_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_destroy_commit_function()
 ****************************************************************
 * @brief
 * This function is used to commit the Lun destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 *  11/15/2012 - Modified by Vera Wang
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_commit_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status= FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;
    fbe_job_service_lun_destroy_t       *lun_destroy_request_p = NULL;

    /*! Get the request payload for validation */
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
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get a pointer to the incoming destroy LUN request */
    lun_destroy_request_p = (fbe_job_service_lun_destroy_t*)job_queue_element_p->command_data;

	job_service_trace(FBE_TRACE_LEVEL_INFO, 
		FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
		"%s entry, job number 0x%llx, status 0x%x, error_code %d\n",
		__FUNCTION__,
		(unsigned long long)job_queue_element_p->job_number, 
		job_queue_element_p->status,
		job_queue_element_p->error_code);

	job_queue_element_p->status = FBE_STATUS_OK;
	//	job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = lun_destroy_request_p->lun_object_id;
    job_queue_element_p->need_to_wait = lun_destroy_request_p->wait_destroy;
    job_queue_element_p->timeout_msec = lun_destroy_request_p->destroy_timeout_msec;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = lun_destroy_request_p->lun_object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(lun_destroy_request_p->wait_destroy) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(lun_destroy_request_p->lun_object_id, 
                                                                  FBE_LIFECYCLE_STATE_NOT_EXIST, 
                                                                  lun_destroy_request_p->destroy_timeout_msec);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting lun object id %d destroy. status %d\n", 
                              __FUNCTION__,  lun_destroy_request_p->lun_object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    status = fbe_notification_send(lun_destroy_request_p->lun_object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so the Job Service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);


    return status;
}
/**********************************************************
 * end fbe_lun_destroy_commit_function()
 **********************************************************/


/*
 * The following functions are utility functions used by the LUN destroy Job Service code
 */


/*!**************************************************************
 * fbe_lun_destroy_lookup_lun_object_id()
 ****************************************************************
 * @brief
 * This function returns the logical object id of a LUN user id.
 *
 * !TODO consider moving this function to the appropriate utility file
 *
 * @param lun_number        - user LUN id (IN)
 * @param lun_object_id_p   - logical object id (OUT)
 *
 * @return status
 *
 * @author
 *  02/2010 - Created. rundbs
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_lookup_lun_object_id(fbe_lun_number_t lun_number, fbe_object_id_t *lun_object_id_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_lookup_lun_t      lookup_lun;

    /* Set the user LUN number for lookup */
    lookup_lun.lun_number       = lun_number;

    /* Get LUN object ID from the Config Service */
    status = fbe_create_lib_send_control_packet (
            FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_NUMBER,
            &lookup_lun,
            sizeof(fbe_database_control_lookup_lun_t),
            FBE_PACKAGE_ID_SEP_0,
            FBE_SERVICE_ID_DATABASE,
            FBE_CLASS_ID_INVALID,
            FBE_OBJECT_ID_INVALID,
            FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
    }

    /* Set the LUN object ID returned from the Config Service */
    *lun_object_id_p = lookup_lun.object_id;

    return status;
}
/******************************************************************
 * end fbe_lun_destroy_lookup_lun_object_id()
 *****************************************************************/

/*!**************************************************************
 * fbe_lun_destroy_database_service_destroy_lun()
 ****************************************************************
 * @brief
 * This function sends a control packet to the Database Service 
 * to destroy a lun object.  
 *
 * !TODO consider moving this function to the appropriate utility file
 *
 * @param object_id         - object edge connected to (IN)
 * @param transaction ID    - Config Service transaction id (IN)
 *
 * @return status - status of the operation
 *
 * @author
 *  01/2010 - Created. rundbs
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_configuration_service_destroy_lun(fbe_object_id_t object_id, fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_destroy_object_t  destroy_lun;


    /* Set the ID of the LUN to destroy */
    destroy_lun.object_id       = object_id;
    destroy_lun.transaction_id  = transaction_id;

    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_DESTROY_LUN,
												&destroy_lun,
												sizeof(fbe_database_control_destroy_object_t),
												FBE_PACKAGE_ID_SEP_0,
												FBE_SERVICE_ID_DATABASE,
												FBE_CLASS_ID_LUN,
												destroy_lun.object_id,
												FBE_PACKET_FLAG_NO_ATTRIB);

    /* If failure status returned, log an error and return immediately */
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
        return status;
    }

    return status;
}
/******************************************************************
 * end fbe_lun_destroy_configuration_service_destroy_lun()
 *****************************************************************/

/*!**************************************************************
 * fbe_lun_destroy_get_upstream_edge_count()
 ****************************************************************
 * @brief
 * This function gets a count of the number of upstream edges
 * for the given object. In this case, the object is a LUN object.
 *
 * !TODO consider moving this function to the appropriate utility file
 *
 * @param object_id                 - Object id (IN)
 * @param lun_upstream_edge_count_p - upstream object edge count (OUT)
 *
 * @return status - status of the operation
 *
 * @author
 *  01/2010 - Created. rundbs
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_get_upstream_edge_count(fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_edges)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_base_config_upstream_edge_count_t   upstream_edge_count;


    /* Initialize edge count to send to Topology Service */
    upstream_edge_count.number_of_upstream_edges = 0;

    status = fbe_create_lib_send_control_packet(
            FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_EDGE_COUNT,
            &upstream_edge_count,
            sizeof(fbe_base_config_upstream_edge_count_t),
            FBE_PACKAGE_ID_SEP_0,
            FBE_SERVICE_ID_TOPOLOGY,
            FBE_CLASS_ID_INVALID,
            object_id,
            FBE_PACKET_FLAG_NO_ATTRIB);

    /* If failure status returned, log an error and return immediately */
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
        return status;
    }

    /* Set the number of upstream edges returned from Topology Service*/
    *number_of_upstream_edges = upstream_edge_count.number_of_upstream_edges;

    return status;
}
/******************************************************************
 * end fbe_lun_destroy_get_upstream_edge_count()
 *****************************************************************/

/*!**************************************************************
 * fbe_lun_destroy_send_prepare_to_destroy_usurper_cmd_to_lun()
 ****************************************************************
 * @brief
 * This function sends a "prepare for destroy" usurper command
 * to the LUN object.  
 *
 * @param lun_object_id     - lun object id. (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/2010 - Created. rundbs
 ****************************************************************/
static fbe_status_t fbe_lun_destroy_send_prepare_to_destroy_usurper_cmd_to_lun(fbe_object_id_t   lun_object_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 


    status = fbe_create_lib_send_control_packet(FBE_LUN_CONTROL_CODE_PREPARE_TO_DESTROY_LUN,
                                                NULL,
                                                0,
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                lun_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
        return status;
    }

    return status;
}
/**********************************************************
 * end fbe_lun_destroy_send_prepare_to_destroy_usurper_cmd_to_lun()
 **********************************************************/


