/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_update_provision_drive_block_size.c
 ***************************************************************************
 *
 * @brief
 *  This file contains ENGINEERING ONLY routines to update the provision drive configuration 
 *  using job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   06/11/2014:  Created. Wayne Garrett
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
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_base_config.h"
#include "fbe_private_space_layout.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_provision_drive_block_size_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_block_size_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_block_size_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_block_size_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_block_size_commit_function(fbe_packet_t *packet_p);


static fbe_status_t fbe_update_provision_drive_block_size_send_notification(fbe_job_service_update_pvd_block_size_t * update_pvd_p,
                                                                 fbe_u64_t job_number,
                                                                 fbe_job_service_error_type_t job_service_error_type,
                                                                 fbe_job_action_state_t job_action_state);


/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_provision_drive_block_size_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE,
    {
        /* validation function */
        fbe_update_provision_drive_block_size_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_provision_drive_block_size_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_provision_drive_block_size_persist_function,

        /* response/rollback function */
        fbe_update_provision_drive_block_size_rollback_function,

        /* commit function */
        fbe_update_provision_drive_block_size_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 * fbe_update_provision_drive_block_size_validation_function()
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
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_block_size_validation_function(fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_pvd_block_size_t *  update_pvd_block_size_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_topology_mgmt_get_object_type_t         topology_mgmt_get_object_type;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_provision_drive_config_type_t       config_type;
    fbe_bool_t                              end_of_life_state = FBE_FALSE;

    
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

    update_pvd_block_size_p = (fbe_job_service_update_pvd_block_size_t *) job_queue_element_p->command_data;
    if(update_pvd_block_size_p == NULL)
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Get the object type */
    topology_mgmt_get_object_type.object_id = update_pvd_block_size_p->object_id;

    status = fbe_create_lib_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE,
                                                &topology_mgmt_get_object_type,
                                                sizeof(topology_mgmt_get_object_type),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK || topology_mgmt_get_object_type.topology_object_type != FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE) 
    {
        
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the provision drive information to determine it's config type. */
    status = fbe_cretae_lib_get_provision_drive_config_type(update_pvd_block_size_p->object_id, &config_type ,&end_of_life_state);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error.
         */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update pvd block size. obj: 0x%x get config type failed. status: 0x%x \n", 
                          update_pvd_block_size_p->object_id, status);


        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update pvd block size. obj: 0x%x is consumed. status: 0x%x \n", 
                          update_pvd_block_size_p->object_id, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PVD_INVALID_CONFIG_TYPE;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }


    if (fbe_private_space_layout_object_id_is_system_pvd(update_pvd_block_size_p->object_id))
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update pvd block size. obj: 0x%x is system drive. status: 0x%x \n", 
                          update_pvd_block_size_p->object_id, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_provision_drive_block_size_validation_function()
 ******************************************************************************/



/*!****************************************************************************
 * fbe_update_provision_drive_block_size_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive config type in memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_block_size_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_pvd_block_size_t *  update_pvd_block_size_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_database_control_update_pvd_block_size_t db_update_pvd_block_size;

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
    update_pvd_block_size_p = (fbe_job_service_update_pvd_block_size_t *) job_queue_element_p->command_data;
    if(update_pvd_block_size_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_start_database_transaction(&update_pvd_block_size_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to start provision drive update config type transaction %d\n", __FUNCTION__, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Update the transaction details to update the configuration. */
    db_update_pvd_block_size.transaction_id = update_pvd_block_size_p->transaction_id;
    db_update_pvd_block_size.object_id = update_pvd_block_size_p->object_id;
    db_update_pvd_block_size.configured_physical_block_size = update_pvd_block_size_p->block_size;

    /* Send the update provision drive configuration to configuration service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD_BLOCK_SIZE,
                                                &db_update_pvd_block_size,
                                                sizeof(fbe_database_control_update_pvd_block_size_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s FBE_DATABASE_CONTROL_CODE_UPDATE_PVD_BLOCK_SIZE failed\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_provision_drive_block_size_update_configuration_in_memory_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_block_size_rollback_function()
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
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_block_size_rollback_function(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_status_t                payload_status;
    fbe_job_service_update_pvd_block_size_t *   update_pvd_block_size_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "update_pvd_block_size_rollback_fun entry\n");

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
    if (length != sizeof(fbe_job_queue_element_t))
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    update_pvd_block_size_p = (fbe_job_service_update_pvd_block_size_t *)job_queue_element_p->command_data;
    if (update_pvd_block_size_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we failed during validation there is no reason to rollback.
     */
    if (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_VALIDATE)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "update_pvd_block_size_rollback_fun, rollback not required, prev state %d, cur state %d error: %d\n", 
                job_queue_element_p->previous_state,
                job_queue_element_p->current_state,
                job_queue_element_p->error_code);
    }

    /* If need be roll back the transaction.
     */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)          ||
        (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)    )
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "update_pvd_block_size_rollback_fun: abort transaction id %llu\n",
			  (unsigned long long)update_pvd_block_size_p->transaction_id);

        /* rollback the configuration transaction. */
        status = fbe_create_lib_abort_database_transaction(update_pvd_block_size_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "update_pvd_rollback_fun: cannot abort transaction %llu with cfg service\n", 
                              (unsigned long long)update_pvd_block_size_p->transaction_id);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }
    }

    /* Send a notification for this rollback. */
    status = fbe_update_provision_drive_block_size_send_notification(update_pvd_block_size_p,
                                                          job_queue_element_p->job_number,
                                                          job_queue_element_p->error_code,
                                                          FBE_JOB_ACTION_STATE_ROLLBACK);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "update_pvd_block_size_rollback_fun: status change notification fail, status:%d\n",
                          status);
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* set the payload status to ok if it is not updated as failure. */
    fbe_payload_control_get_status(control_operation_p, &payload_status);
    if (payload_status != FBE_PAYLOAD_CONTROL_STATUS_FAILURE)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        /* Trace the current error code.
         */
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "update_pvd_block_size_rollback_fun, payload_status: 0x%x error: %d\n", 
                payload_status,
                job_queue_element_p->error_code);

        /* If the error code isn't set, flag the error now.
         */
        if (job_queue_element_p->error_code == FBE_JOB_SERVICE_ERROR_NO_ERROR)
        {
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        }
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_provision_drive_block_size_rollback_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_block_size_persist_function()
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
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_block_size_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_pvd_block_size_t *  update_pvd_block_size_p = NULL;
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
    update_pvd_block_size_p = (fbe_job_service_update_pvd_block_size_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(update_pvd_block_size_p->transaction_id);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to commit the provision drive block size, status:%d, pvd:0x%x block_size:%d\n",
                          __FUNCTION__, status, update_pvd_block_size_p->object_id, update_pvd_block_size_p->block_size);

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
 * end fbe_update_provision_drive_block_size_persist_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_block_size_commit_function()
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
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_block_size_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_job_service_update_pvd_block_size_t *      update_pvd_block_size_p = NULL;
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
    update_pvd_block_size_p = (fbe_job_service_update_pvd_block_size_t *) job_queue_element_p->command_data;

    /* log the trace for the config type update.*/
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "update_pvd_block_size_commit: pvd:0x%x block_size:%d\n",
                      update_pvd_block_size_p->object_id, update_pvd_block_size_p->block_size);

    /* Send a notification for this commit*/
    status = fbe_update_provision_drive_block_size_send_notification(update_pvd_block_size_p,
                                                          job_queue_element_p->job_number,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR,
                                                          FBE_JOB_ACTION_STATE_COMMIT);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "update_pvd_block_size_commit: stat change notification fail, stat %d\n", status);
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
    job_queue_element_p->object_id = update_pvd_block_size_p->object_id;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_update_provision_drive_block_size_commit_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_block_size_send_notification()
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
 *  06/11/2014 - Created. Wayne Garrett
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_block_size_send_notification(fbe_job_service_update_pvd_block_size_t * update_pvd_block_size_p,
                                             fbe_u64_t job_number,
                                             fbe_job_service_error_type_t job_service_error_type,
                                             fbe_job_action_state_t job_action_state)
{
    fbe_notification_info_t     notification_info;
    fbe_status_t                status;

    /* fill up the notification information before sending notification. */
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = update_pvd_block_size_p->object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_service_error_type;
    notification_info.notification_data.job_service_error_info.job_number = job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE;
    notification_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;

    /* If the request failed generate a trace.
     */
    if (job_service_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update block size notification job: 0x%llx pvd obj: 0x%x failed. error: %d\n",
                          (unsigned long long)job_number, update_pvd_block_size_p->object_id, job_service_error_type);
        if (job_service_error_type == FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: update block size notification job: 0x%llx pvd obj: 0x%x failed. error: %d\n",
                              (unsigned long long)job_number, update_pvd_block_size_p->object_id, job_service_error_type);
        }
    }

    /* trace some information
     */
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "UPDATE: provision drive update block size notification pvd obj: 0x%x block_size: %d job: 0x%llx\n",
                      update_pvd_block_size_p->object_id, update_pvd_block_size_p->block_size, (unsigned long long)job_number);

    /* send the notification to registered callers. */
    status = fbe_notification_send(update_pvd_block_size_p->object_id, notification_info);
    return status;
}

