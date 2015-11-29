/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ndu_commit.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for performing sep ndu commit
 * 
 * @ingroup job_lib_files
 * @created. Zhipeng Hu
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_base_config.h"
#include "fbe_database.h"
#include "fbe_private_space_layout.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_sep_ndu_commit_validation_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_sep_ndu_commit_update_configuration_in_memory_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_sep_ndu_commit_rollback_function (fbe_packet_t *packet_p);
static fbe_status_t fbe_sep_ndu_commit_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_sep_ndu_commit_commit_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_sep_ndu_commit_expand_raid_group(fbe_private_space_layout_region_t *region_p,
                                                         fbe_database_transaction_id_t transaction_id,
                                                         fbe_time_t commit_start_ms);

fbe_status_t
fbe_create_lun_configuration_service_create_system_lun_and_edges(fbe_job_service_lun_create_t *lun_create_req);


/*************************
 *   DEFINITIONS
 *************************/
#define FBE_JOB_SERVICE_SEP_NDU_COMMIT_TIMEOUT ((fbe_time_t)120000) /*ms*/
fbe_job_service_operation_t fbe_ndu_commit_job_service_operation = 
{
    FBE_JOB_TYPE_SEP_NDU_COMMIT,
    {
        /* validation function */
        fbe_sep_ndu_commit_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_sep_ndu_commit_update_configuration_in_memory_function,

        /* persist function */
        fbe_sep_ndu_commit_persist_function,

        /* response/rollback function */
        fbe_sep_ndu_commit_rollback_function,

        /* commit function */
        fbe_sep_ndu_commit_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

static fbe_status_t fbe_sep_ndu_commit_validation_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                       *                payload_p = NULL;
    fbe_payload_control_operation_t     *                control_operation_p = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;
    fbe_job_queue_element_t             *                job_queue_element_p = NULL;
    fbe_job_service_sep_ndu_commit_t *                ndu_commit_p = NULL;

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

    ndu_commit_p = (fbe_job_service_sep_ndu_commit_t *) job_queue_element_p->command_data;

    /*nothing to validate for now*/

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_sep_ndu_commit_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_job_service_sep_ndu_commit_t *           ndu_commit_p = NULL;
    fbe_payload_ex_t                       *                payload_p = NULL;
    fbe_payload_control_operation_t     *                control_operation_p = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;
    fbe_job_queue_element_t             *                job_queue_element_p = NULL;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_lun_create_t lun_create_request;
    fbe_private_space_layout_lun_info_t lun;
    fbe_private_space_layout_region_t   region;
    fbe_u32_t i;
    fbe_database_transaction_id_t     transaction_id;
    fbe_time_t                          commit_start_ms;

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

    /* Get the commit start time in milliseconds
     */
    commit_start_ms = fbe_get_time();

    /* send update system db header request to database service.
     * this will also flush the persist db out with the new layout
     */
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_COMMIT_DATABASE_TABLES,
                                                 NULL,
                                                 0,
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the user data from job command*/
    ndu_commit_p = (fbe_job_service_sep_ndu_commit_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_start_database_transaction(&transaction_id, job_queue_element_p->job_number);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        status = fbe_private_space_layout_get_region_by_index(i, &region);
        if(status != FBE_STATUS_OK)
        {
            break;
        }
        if(region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        if(region.region_type != FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
        {
            continue;
        }

        status = fbe_sep_ndu_commit_expand_raid_group(&region, transaction_id, commit_start_ms);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed to expand RG %d status: 0x%x\n",
                               __FUNCTION__, region.raid_info.raid_group_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
    {
        status = fbe_private_space_layout_get_lun_by_index(i, &lun);

        if(status != FBE_STATUS_OK)
        {
            break;
        }
        if(lun.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
        {
            break;
        }

        status = fbe_create_lib_lookup_lun_object_id(lun.lun_number, &lun_object_id);
        if((status == FBE_STATUS_NO_OBJECT) && (lun_object_id == FBE_OBJECT_ID_INVALID))
        {
            fbe_private_space_layout_get_region_by_raid_group_id(lun.raid_group_id, &region);

            job_service_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Creating System LUN %d in RG %d\n",
                              __FUNCTION__, lun.lun_number, region.raid_info.raid_group_id);

            fbe_zero_memory(&lun_create_request, sizeof(fbe_job_service_lun_create_t));
            lun_create_request.is_system_lun        = FBE_TRUE;
            lun_create_request.ndb_b                = FBE_FALSE;
            lun_create_request.noinitialverify_b    = FBE_FALSE;
            lun_create_request.lun_number           = lun.lun_number;
            lun_create_request.lun_object_id        = lun.object_id;
            lun_create_request.transaction_id       = transaction_id;
            lun_create_request.addroffset           = lun.raid_group_address_offset;
            lun_create_request.placement            = FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION;
            lun_create_request.capacity             = lun.external_capacity;
            lun_create_request.raid_group_id        = region.raid_info.raid_group_id;

            status = fbe_create_lun_configuration_service_create_system_lun_and_edges(&lun_create_request);
            if(status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: Failed to create System LUN %d in RG %d\n",
                                  __FUNCTION__, lun.lun_number, region.raid_info.raid_group_id);

                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    status = fbe_create_lib_commit_database_transaction(transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

static fbe_status_t fbe_sep_ndu_commit_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;


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

    /* nothing to do in this stage*/
    
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

}

static fbe_status_t fbe_sep_ndu_commit_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *                payload_p = NULL;
    fbe_payload_control_operation_t     *                control_operation_p = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;
    fbe_job_queue_element_t             *                job_queue_element_p = NULL;
    fbe_notification_info_t                             notification_info;

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

    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_GENERIC_FAILURE;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_SEP_NDU_COMMIT;
    notification_info.notification_data.job_service_error_info.error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, action state: %d\n",
                          __FUNCTION__, status);
    }


    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sep_ndu_commit_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *                payload_p = NULL;
    fbe_payload_control_operation_t     *                control_operation_p = NULL;
    fbe_notification_info_t                             notification_info;
    fbe_payload_control_buffer_length_t                 length = 0;
    fbe_job_queue_element_t             *                job_queue_element_p = NULL;

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

    /* Send a notification for letting users know we are done*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_SEP_NDU_COMMIT;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: job status notification failed, status: %d\n",
                          __FUNCTION__, status);
    }
        
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;

}

/*!***************************************************************************
 *          fbe_sep_ndu_commit_expand_raid_group()
 ***************************************************************************** 
 * 
 * @brief   This method sends a raid groups expand request to the raid group
 *          object.  There are cases where the raid group object may fail
 *          the request because it is currently busy.  This method will retry
 *          the expansion up-to the total time allowed.
 *
 * @param   region_p - Pointer to region that requires expansion
 * @param   transaction_id - The transaction id of the job
 * @param   commit_start_ms - The time that the 
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/20/2014  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_sep_ndu_commit_expand_raid_group(fbe_private_space_layout_region_t *region_p,
                                                         fbe_database_transaction_id_t transaction_id,
                                                         fbe_time_t commit_start_ms)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_group_get_info_t   raid_group_info;
    fbe_time_t                  elapsed_ms;
    fbe_u32_t                   wait_period_ms = ((3 * 2) * 1000);
    fbe_semaphore_t             wait_for_expand_sem;
    fbe_database_control_update_raid_t config_update;

    /* Get the raid group information.
     */
    status = fbe_create_lib_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                &raid_group_info,
                                                sizeof(fbe_raid_group_get_info_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                region_p->raid_info.object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: expansion of rg id: %d failed to get raid group info.\n",
                          __FUNCTION__, region_p->raid_info.raid_group_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Only need to do anything if expansion is required.
     */
    if (raid_group_info.capacity != region_p->raid_info.exported_capacity) {
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "commit: Extending RAID Group %d. Old size: 0x%llx New Size: 0x%llx\n",
                         region_p->raid_info.raid_group_id, 
                         raid_group_info.capacity, region_p->raid_info.exported_capacity);

        /* Initialize the semaphore and the update request.
         */
        elapsed_ms = fbe_get_elapsed_milliseconds(commit_start_ms);
        status = FBE_STATUS_TIMEOUT;
        fbe_semaphore_init(&wait_for_expand_sem, 0, 1);
        fbe_zero_memory(&config_update, sizeof(fbe_database_control_update_raid_t));
        config_update.object_id                     = region_p->raid_info.object_id;
        config_update.transaction_id                = transaction_id;
        config_update.update_type                   = FBE_UPDATE_RAID_TYPE_EXPAND_RG;
        config_update.new_rg_size                   = region_p->raid_info.exported_capacity;
        config_update.new_edge_capacity             = region_p->size_in_blocks;

        /* Wait up to timeout or success of raid group expansion.
         */
        while ((status != FBE_STATUS_OK)                             &&
               (elapsed_ms < FBE_JOB_SERVICE_SEP_NDU_COMMIT_TIMEOUT)    ) {
            /* Send the request.
             */
            status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_RAID,
                                                        &config_update,
                                                        sizeof(fbe_database_control_update_raid_t),
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_DATABASE,
                                                        FBE_CLASS_ID_INVALID,
                                                        FBE_OBJECT_ID_INVALID,
                                                        FBE_PACKET_FLAG_NO_ATTRIB);
            if (status != FBE_STATUS_OK) {
                /* Wait at least (2) monitor cycles then retry */
                job_service_trace(FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "commit: Retry commit to Extend RG %d. Old size: 0x%llx New Size: 0x%llx\n",
                                  region_p->raid_info.raid_group_id, 
                                  raid_group_info.capacity, region_p->raid_info.exported_capacity);

                /* Wait for condition to clear
                 */
                fbe_semaphore_wait_ms(&wait_for_expand_sem, wait_period_ms);
                elapsed_ms = fbe_get_elapsed_milliseconds(commit_start_ms);
            }
        }

        /* Free the semaphore.
         */
        fbe_semaphore_destroy(&wait_for_expand_sem);

        /* If we waited and the expand still failed return an error.
         */
        if (status != FBE_STATUS_OK)
        {
           job_service_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "commit: Failed to Extend RG %d. Old size: 0x%llx New Size: 0x%llx\n",
                             region_p->raid_info.raid_group_id, 
                             raid_group_info.capacity, region_p->raid_info.exported_capacity);
            return FBE_STATUS_TIMEOUT;
        }
    }

    /* No need to expand return success
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_sep_ndu_commit_expand_raid_group()
 ******************************************************************************/


/************************************* 
 * end of file fbe_sep_ndu_commit.c
 ***********************************/
