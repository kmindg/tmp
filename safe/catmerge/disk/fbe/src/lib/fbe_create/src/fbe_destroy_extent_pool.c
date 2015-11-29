/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_destroy_extent_pool.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for Raid Group destroy used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   07/02/2014:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_parity.h"
#include "fbe_database.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe_private_space_layout.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

/* Raid Group destroy library request validation function.
 */
static fbe_status_t fbe_destroy_extent_pool_validation_function (fbe_packet_t *packet_p);

/* Raid Group destroy library update configuration in memory function.
 */
static fbe_status_t fbe_destroy_extent_pool_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Raid Group destroy library persist configuration in db function.
 */
static fbe_status_t fbe_destroy_extent_pool_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Raid Group destroy library rollback function.
 */
static fbe_status_t fbe_destroy_extent_pool_rollback_function (fbe_packet_t *packet_p);

/* Raid Group destroy library commit function.
 */
static fbe_status_t fbe_destroy_extent_pool_commit_function (fbe_packet_t *packet_p);


/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_destroy_extent_pool_job_service_operation = 
{
    FBE_JOB_TYPE_DESTROY_EXTENT_POOL,
    {
        /* validation function */
        fbe_destroy_extent_pool_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_destroy_extent_pool_update_configuration_in_memory_function,

        /* persist function */
        fbe_destroy_extent_pool_persist_configuration_db_function,

        /* response/rollback function */
        fbe_destroy_extent_pool_rollback_function,

        /* commit function */
        fbe_destroy_extent_pool_commit_function,
    }
};


static void fbe_destroy_extent_pool_set_packet_status(fbe_packet_t *packet_p, 
                                                      fbe_status_t packet_status,
                                                      fbe_payload_control_status_t payload_status);
void fbe_destroy_extent_pool_set_notification_parameters(
        fbe_job_service_destroy_extent_pool_t * destroy_extent_pool_request_p,  
        fbe_status_t status,
        fbe_job_service_error_type_t error_code,
        fbe_u64_t job_number,
        fbe_notification_info_t *notification_info);
static fbe_status_t fbe_destroy_extent_pool_destroy_extent_pool(
        fbe_job_service_destroy_extent_pool_t * destroy_extent_pool_request_p);
static fbe_status_t fbe_destroy_extent_pool_destroy_extent_pool_metadata_lun(
        fbe_job_service_destroy_extent_pool_t * destroy_extent_pool_request_p);


/*!**************************************************************
 * fbe_destroy_extent_pool_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param destroy_request_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/

static fbe_status_t fbe_destroy_extent_pool_validation_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_job_service_destroy_extent_pool_t   *destroy_extent_pool_request_p = NULL;
    fbe_u32_t                            number_of_upstream_edges;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* get the parameters passed to the validation function */
    destroy_extent_pool_request_p = (fbe_job_service_destroy_extent_pool_t *)job_queue_element_p->command_data;

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Get the ext pool object id from config service */
    status = fbe_create_lib_database_service_lookup_ext_pool_object_id(destroy_extent_pool_request_p->pool_id, 
                                                                       &destroy_extent_pool_request_p->object_id);
    if(status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, Invalid extent pool id %d\n", __FUNCTION__, 
                destroy_extent_pool_request_p->pool_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_UNKNOWN_ID;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* Get the ext pool object id from config service */
    status = fbe_create_lib_database_service_lookup_ext_pool_lun_object_id(destroy_extent_pool_request_p->pool_id, 0, 
                                                                       &destroy_extent_pool_request_p->metadata_lun_object_id);
    if(status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, Invalid extent pool metadata LUN pool id %d\n", __FUNCTION__, 
                destroy_extent_pool_request_p->pool_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_UNKNOWN_ID;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* verify there are no objects above the RG object */
    number_of_upstream_edges = 0;
    status = fbe_create_lib_get_upstream_edge_count(destroy_extent_pool_request_p->object_id, 
                                                    &number_of_upstream_edges);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, could not determine number of upstream edges for pool object %d\n", 
                __FUNCTION__,
                destroy_extent_pool_request_p->object_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        return FBE_STATUS_GENERIC_FAILURE;
    } 

    if (number_of_upstream_edges != 1)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "RG Destroy Validation: RG id %d has %d upstream objects, cannot destroy\n", 
                destroy_extent_pool_request_p->object_id, number_of_upstream_edges);

        job_queue_element_p->status = status; 
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
     
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return status;
}
/**************************************************
 * end fbe_destroy_extent_pool_validation_function()
 *************************************************/

/*!**************************************************************
 * fbe_destroy_extent_pool_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_extent_pool_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_job_service_destroy_extent_pool_t *destroy_extent_pool_request_p = NULL;
    fbe_object_id_t        *pvd_list = NULL;
    fbe_u32_t               drive_count;
    fbe_u32_t               pool_id = FBE_POOL_ID_INVALID;
    fbe_u32_t               pvd_index;
    fbe_object_id_t *       pvd_object_id;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);
 
    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* get the parameters passed to the validation function */
    destroy_extent_pool_request_p = (fbe_job_service_destroy_extent_pool_t *)job_queue_element_p->command_data;

    pvd_list = (fbe_object_id_t *)(destroy_extent_pool_request_p + 1);
    status = fbe_database_get_ext_pool_configuration_info(destroy_extent_pool_request_p->object_id, &pool_id, &drive_count, pvd_list);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, get pool config failed, status %d",
                __FUNCTION__, status);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_memory_ex_release(pvd_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* fbe_database_open_transaction. Should the type of transaction be given here? */
    status = fbe_create_lib_start_database_transaction(&destroy_extent_pool_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, call to create transaction id failed, status %d",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* Destroy metadata lun */
    status = fbe_destroy_extent_pool_destroy_extent_pool_metadata_lun(destroy_extent_pool_request_p);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, call to create transaction id failed, status %d",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* Destroy extent pool */
    status = fbe_destroy_extent_pool_destroy_extent_pool(destroy_extent_pool_request_p);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, call to create transaction id failed, status %d",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /*update pvd config type*/
    /* Update PVD configuration. */
    pvd_object_id = pvd_list;
    for (pvd_index = 0; pvd_index < drive_count;  pvd_index++) {
        /* update the configuration type of the pvd object. */
        status = fbe_create_lib_database_service_update_pvd_config_type(destroy_extent_pool_request_p->transaction_id, 
                                                                        *pvd_object_id,
                                                                        FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED);
        if (status != FBE_STATUS_OK) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to update config type pvd 0x%x status %d\n",
                __FUNCTION__, *pvd_object_id, status);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "     updated config type to UNCONSUMED for PVD[%d] object.\n",
                *pvd_object_id);

        /* update the pool of the pvd object. */
        status = fbe_create_lib_database_service_update_pvd_pool_id(destroy_extent_pool_request_p->transaction_id, 
                                                                    *pvd_object_id,
                                                                    FBE_POOL_ID_INVALID);
        if (status != FBE_STATUS_OK) {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to update pool id pvd 0x%x status %d\n",
                __FUNCTION__, *pvd_object_id, status);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "     updated pool_id to %d for PVD[%d] object.\n",
                FBE_POOL_ID_INVALID, *pvd_object_id);

        pvd_object_id++;
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return status;
}
/**********************************************************
 * end fbe_destroy_extent_pool_update_configuration_in_memory_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_extent_pool_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_extent_pool_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_job_service_destroy_extent_pool_t *destroy_extent_pool_request_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* get the parameters passed to the validation function */
    destroy_extent_pool_request_p = (fbe_job_service_destroy_extent_pool_t *)job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(destroy_extent_pool_request_p->transaction_id);
    if(status != FBE_STATUS_OK) 
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_OK);

    return status;

}
/**********************************************************
 * end fbe_destroy_extent_pool_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_extent_pool_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_extent_pool_rollback_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_job_service_destroy_extent_pool_t     *destroy_extent_pool_request_p = NULL;
    fbe_notification_info_t              notification_info;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* get the parameters passed to the validation function */
    destroy_extent_pool_request_p = (fbe_job_service_destroy_extent_pool_t *)job_queue_element_p->command_data;

    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY))
    {
        /* in the memory function we register for object destroy notification,
         * on error we must unregister 
         */

        if (destroy_extent_pool_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: aborting transaction id %llu\n",
                    __FUNCTION__,
		    (unsigned long long)destroy_extent_pool_request_p->transaction_id);

            status = fbe_create_lib_abort_database_transaction(destroy_extent_pool_request_p->transaction_id);
            if(status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: Could not abort transaction id %llu with configuration service\n", 
                        __FUNCTION__,
		        (unsigned long long)destroy_extent_pool_request_p->transaction_id);

                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

                fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                return status;
            }
        }
    }

    /* fill notification parameters */
    fbe_destroy_extent_pool_set_notification_parameters(destroy_extent_pool_request_p,  
                                                       job_queue_element_p->status,
                                                       job_queue_element_p->error_code,
                                                       job_queue_element_p->job_number,
                                                       &notification_info);

    /* Send a notification for this rollback*/
    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
   }

    /* Don't set the status and error_code here. Just return what is in the job_queue_element. */

    fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_OK);
    return status;
}
/**********************************************************
 * end fbe_destroy_extent_pool_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_destroy_extent_pool_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the Raid Group destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - destroy request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_extent_pool_commit_function (fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_job_service_destroy_extent_pool_t *destroy_extent_pool_request_p = NULL;
    fbe_notification_info_t              notification_info;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);
 
    /* validate packet contents */
    status = fbe_create_lib_get_packet_contents(packet_p, &job_queue_element_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* get the parameters passed to the validation function */
    destroy_extent_pool_request_p = (fbe_job_service_destroy_extent_pool_t *)job_queue_element_p->command_data;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry, job number 0x%llx, status 0x%x, error_code %d\n",
        __FUNCTION__,
        (unsigned long long)job_queue_element_p->job_number, 
        job_queue_element_p->status,
        job_queue_element_p->error_code);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = destroy_extent_pool_request_p->object_id;
    //job_queue_element_p->need_to_wait = destroy_extent_pool_request_p->user_input.wait_destroy;
    //job_queue_element_p->timeout_msec = destroy_extent_pool_request_p->user_input.destroy_timeout_msec;

    /* fill notification parameters */
    fbe_destroy_extent_pool_set_notification_parameters(destroy_extent_pool_request_p,  
                                                       FBE_STATUS_OK,
                                                       FBE_JOB_SERVICE_ERROR_NO_ERROR,
                                                       job_queue_element_p->job_number,
                                                       &notification_info);

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(1) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(destroy_extent_pool_request_p->object_id, 
                                                                  FBE_LIFECYCLE_STATE_NOT_EXIST, 
                                                                  30000);
        if (status != FBE_STATUS_OK)
        {

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting rg object id %d destroy. status %d\n", 
                              __FUNCTION__,  destroy_extent_pool_request_p->object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            return status;
        }
    }

    /* Send a notification for this commit*/
    status = fbe_notification_send(destroy_extent_pool_request_p->object_id, notification_info);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, status: 0x%X\n",
                          __FUNCTION__, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_destroy_extent_pool_set_packet_status(packet_p, FBE_STATUS_OK, FBE_PAYLOAD_CONTROL_STATUS_OK);
    return status;
}
/**********************************************************
 * end fbe_destroy_extent_pool_commit_function()
 **********************************************************/

/*!**************************************************************
 * fbe_destroy_extent_pool_set_notification_parameters()
 ****************************************************************
 * @brief
 * This funtion sets the valus for a notification info structure
 *
 * @param destroy_extent_pool_request_p - job service request
 * @param status - status to send to receiver of notification.
 * @param error_code - if valid error, error found while destroying raid group
 * @param notification_info - extended data for receiver of notification.
 *
 * @return status - none
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/

void fbe_destroy_extent_pool_set_notification_parameters(
        fbe_job_service_destroy_extent_pool_t *destroy_extent_pool_request_p,  
        fbe_status_t status,
        fbe_job_service_error_type_t error_code,
        fbe_u64_t job_number,
        fbe_notification_info_t *notification_info)
{
    notification_info->notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info->notification_data.job_service_error_info.object_id = 
        destroy_extent_pool_request_p->object_id;

    notification_info->notification_data.job_service_error_info.status = status;
    notification_info->notification_data.job_service_error_info.error_code = error_code;
    notification_info->notification_data.job_service_error_info.job_number = job_number;
    notification_info->notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_RAID_GROUP_DESTROY;
    notification_info->class_id = FBE_CLASS_ID_INVALID;
    notification_info->object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
}
/**************************************************
 * end fbe_destroy_extent_pool_set_notification_parameters()
 **************************************************/

/*!**************************************************************
 * fbe_destroy_extent_pool_set_packet_status()
 ****************************************************************
 * @brief
 * This function sets the packet status and payload status
 *
 * @param control_operation - control operation for payload status 
 * @param fbe_payload_control_status_t - payload status
 * @param packet - packet whose status will be set
 * @param packet_status - setting of packet's status
 *
 * @return status
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static void fbe_destroy_extent_pool_set_packet_status(fbe_packet_t *packet_p, 
                                                      fbe_status_t packet_status,
                                                      fbe_payload_control_status_t payload_status)
{
    fbe_payload_ex_t                     *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, packet_status, 0);
    fbe_transport_complete_packet(packet_p);
}

/*!**************************************************************
 * fbe_destroy_extent_pool_destroy_extent_pool()
 ****************************************************************
 * @brief
 * This function destroys extent pool object.
 *
 * @param destroy_extent_pool_request_p - destroy request. 
 *
 * @return status
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_extent_pool_destroy_extent_pool(
        fbe_job_service_destroy_extent_pool_t * destroy_extent_pool_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_database_control_destroy_object_t destroy_ext_pool;

    destroy_ext_pool.object_id                              = destroy_extent_pool_request_p->object_id;
    destroy_ext_pool.transaction_id                         = destroy_extent_pool_request_p->transaction_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL,
                                                 &destroy_ext_pool,
                                                 sizeof(fbe_database_control_destroy_object_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
				"%s: extent pool metadata lun id %d\n", 
                __FUNCTION__, 
                destroy_extent_pool_request_p->object_id);
    }
     return status;
}

/*!**************************************************************
 * fbe_destroy_extent_pool_destroy_extent_pool_metadata_lun()
 ****************************************************************
 * @brief
 * This function destroys extent pool metadata lun.
 *
 * @param destroy_extent_pool_request_p - destroy request. 
 *
 * @return status
 *
 * @author
 *  07/01/2014 - Created. Lili Chen
 ****************************************************************/
static fbe_status_t fbe_destroy_extent_pool_destroy_extent_pool_metadata_lun(
        fbe_job_service_destroy_extent_pool_t * destroy_extent_pool_request_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_database_control_destroy_object_t destroy_ext_pool_lun;

    destroy_ext_pool_lun.object_id                              = destroy_extent_pool_request_p->metadata_lun_object_id;
    destroy_ext_pool_lun.transaction_id                         = destroy_extent_pool_request_p->transaction_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL_LUN,
                                                 &destroy_ext_pool_lun,
                                                 sizeof(fbe_database_control_destroy_object_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
				"%s: extent pool metadata lun id %d\n", 
                __FUNCTION__, 
                destroy_extent_pool_request_p->object_id);
    }
     return status;
}
