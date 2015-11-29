/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_update_provision_drive.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines to update the provision drive configuration 
 *  using job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   08/25/2010:  Created. Dhaval Patel
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


/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_update_provision_drive_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_update_provision_drive_commit_function(fbe_packet_t *packet_p);

static fbe_status_t fbe_update_provision_drive_config_type_validation_function(fbe_packet_t *packet_p,
                                                                               fbe_job_service_update_provision_drive_t * update_pvd_p,
                                                                               fbe_job_service_error_type_t *error_code);

static fbe_status_t fbe_update_provision_drive_sniff_verify_state_validation_function (fbe_packet_t *packet_p,
                                                                                       fbe_job_service_update_provision_drive_t * update_pvd_p,
                                                                                       fbe_job_service_error_type_t *error_code);

static fbe_status_t fbe_update_provision_drive_pool_id_validation_function(fbe_packet_t *packet_p, 
                                                                           fbe_job_service_update_provision_drive_t * update_pvd_p, 
                                                                           fbe_job_service_error_type_t *error_code);

static fbe_status_t fbe_update_provision_drive_serial_num_validation_function(fbe_packet_t * packet_p, 
                                                                           fbe_job_service_update_provision_drive_t * update_pvd_p, 
                                                                           fbe_job_service_error_type_t * error_code);

static fbe_status_t fbe_update_provision_drive_configuration(fbe_database_transaction_id_t transaction_id, 
                                                             fbe_object_id_t vd_object_id,
                                                             fbe_provision_drive_config_type_t config_tye,
                                                             fbe_update_pvd_type_t update_type,
                                                             fbe_u8_t * opaque_data);

static fbe_status_t fbe_update_provision_drive_sniff_verify_state(fbe_database_transaction_id_t transaction_id, 
                                                             fbe_object_id_t vd_object_id,
                                                             fbe_bool_t sniff_verify_state,
                                                             fbe_update_pvd_type_t update_type);

static fbe_status_t fbe_update_provision_drive_pool_id(fbe_database_transaction_id_t transaction_id, 
                                                       fbe_object_id_t object_id, 
                                                       fbe_u32_t pool_id, 
                                                       fbe_update_pvd_type_t update_type);

static fbe_status_t fbe_update_provision_drive_serial_num(fbe_database_transaction_id_t transaction_id, 
                                                       fbe_object_id_t object_id, 
                                                       fbe_u8_t *serial_num, 
                                                       fbe_update_pvd_type_t update_type);

static fbe_status_t fbe_update_provision_drive_send_notification(fbe_job_service_update_provision_drive_t * update_pvd_p,
                                                                 fbe_u64_t job_number,
                                                                 fbe_job_service_error_type_t job_service_error_type,
                                                                 fbe_job_action_state_t job_action_state);

static fbe_status_t fbe_update_provision_drive_config_type_str(fbe_provision_drive_config_type_t config_type,
                                                               fbe_char_t ** config_type_str);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_update_provision_drive_job_service_operation = 
{
    FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE,
    {
        /* validation function */
        fbe_update_provision_drive_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_provision_drive_update_configuration_in_memory_function,

        /* persist function */
        fbe_update_provision_drive_persist_function,

        /* response/rollback function */
        fbe_update_provision_drive_rollback_function,

        /* commit function */
        fbe_update_provision_drive_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 * fbe_update_provision_drive_validation_function()
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
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_validation_function(fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_topology_mgmt_get_object_type_t         topology_mgmt_get_object_type;
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

    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;
    if(update_pvd_p == NULL)
    {

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Get the object type */
    topology_mgmt_get_object_type.object_id = update_pvd_p->object_id;

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

    job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PVD_INVALID_UPDATE_TYPE;

    switch(update_pvd_p->update_type)
    {
        case FBE_UPDATE_PVD_TYPE:
            status = fbe_update_provision_drive_config_type_validation_function(packet_p, update_pvd_p, &job_queue_element_p->error_code);
            break;
        case FBE_UPDATE_PVD_SNIFF_VERIFY_STATE:
            status = fbe_update_provision_drive_sniff_verify_state_validation_function(packet_p, update_pvd_p, &job_queue_element_p->error_code);
            break;
        case FBE_UPDATE_PVD_POOL_ID:
            status = fbe_update_provision_drive_pool_id_validation_function(packet_p, update_pvd_p, &job_queue_element_p->error_code);
            break;
        case FBE_UPDATE_PVD_SERIAL_NUMBER:
            status = fbe_update_provision_drive_serial_num_validation_function(packet_p, update_pvd_p, &job_queue_element_p->error_code);
            break;
        default:
            /* invalid update provision drive configuration request. */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s :invalid update pvd configuration request, update_type %d",
                                   __FUNCTION__, update_pvd_p->update_type);

            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PVD_INVALID_UPDATE_TYPE;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            status =  FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_config_type_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the update provision drive configuration 
 *  job service command.
 *
 * @param packet_p          - Job service packet.
 * @param update_pvd_p      - Update provision drive configuration request.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_config_type_validation_function(fbe_packet_t *packet_p,
                                                           fbe_job_service_update_provision_drive_t * update_pvd_p,
                                                           fbe_job_service_error_type_t *error_code)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_u32_t                               number_of_upstream_edges = 0;
    fbe_job_service_error_type_t            job_service_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    fbe_provision_drive_config_type_t       config_type;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;
    fbe_bool_t                              end_of_life_state = FBE_FALSE;

    /*! get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

   /* get the provision drive information to determine it's config type. */
    status = fbe_cretae_lib_get_provision_drive_config_type(update_pvd_p->object_id, &config_type ,&end_of_life_state);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error.
         */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update config pvd obj: 0x%x get config type failed. status: 0x%x \n", 
                          update_pvd_p->object_id, status);


        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*If end of life state of pvd is set it means this drive is about to die.
     We should not  configure such drive as spare.
    */
    if ((end_of_life_state == FBE_TRUE)                                                    && 
        (update_pvd_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)    )
    {
        /* Trace the error.
         */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update config pvd obj 0x%x config: %d EOL set \n", 
                          update_pvd_p->object_id, update_pvd_p->config_type);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PVD_IS_IN_END_OF_LIFE_STATE;

        *error_code = FBE_JOB_SERVICE_ERROR_PVD_IS_IN_END_OF_LIFE_STATE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    /* if config type of the pvd is same as update config type then complete the 
     * packet with specific error message.
     */
    if (config_type == update_pvd_p->config_type)
    {
        switch(update_pvd_p->config_type)
        {
            case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
                job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_UNCONSUMED;
                break;
            case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
                job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_RAID;
                break;
            case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
                job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE;
                break;
            default:
                job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_IS_NOT_CONFIGURED;
                break;
        }

        /* Trace the error.
         */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update config pvd obj: 0x%x already requested config type: %d error: %d \n", 
                          update_pvd_p->object_id, config_type, job_service_error_type);

        /* Job status is success.
         */
        job_queue_element_p->status = FBE_STATUS_OK;
        job_queue_element_p->error_code = job_service_error_type;

        *error_code = job_service_error_type;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /* get upstream edge count to determine whether pvd is already part of the raid group. */
    status = fbe_create_lib_get_upstream_edge_count(update_pvd_p->object_id, &number_of_upstream_edges);
    if (status != FBE_STATUS_OK)
    {


        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        /* Trace the error.
         */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update config pvd obj: 0x%x config: %d get upstream failed. status: 0x%x\n", 
                          update_pvd_p->object_id, update_pvd_p->config_type, status);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* verify if we can update the configuration type of the pvd object in current configuration. */
    switch(update_pvd_p->config_type)
    {
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            if((config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) &&
               (number_of_upstream_edges != 0))
            {
                /* user cannot update the config type for the pvd which is already configured as raid. */
                job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP;
            }
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            if((config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) &&
               (number_of_upstream_edges != 0))
            {
                /* user cannot configure the drive which is already part of raid group object. */
                job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP;
            }
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
            break;
        default:
            /* user has given wrong configuration type as input to update. */
            job_service_error_type = FBE_JOB_SERVICE_ERROR_PVD_INVALID_CONFIG_TYPE;
            break;
    }

    /* Check if the number of upstream edges is valid or not.
     */
    if (job_service_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {

        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update config pvd obj: 0x%x config: %d upstream edges: %d error: %d\n", 
                          update_pvd_p->object_id, config_type, number_of_upstream_edges, job_service_error_type);

        *error_code = job_service_error_type;

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = job_service_error_type;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
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
 * end fbe_update_provision_drive_config_type_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_sniff_verify_state_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the update provision drive configuration 
 *  job service command.
 *
 * @param packet_p          - Job service packet.
 * @param update_pvd_p      - Update provision drive configuration request.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  10/06/2010 - Created. Manjit Singh
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_sniff_verify_state_validation_function(fbe_packet_t *packet_p,
                                                                  fbe_job_service_update_provision_drive_t * update_pvd_p,
                                                                  fbe_job_service_error_type_t *error_code)
{
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;

    /*! get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* TODO: Add the appropriate validation before updating sniff verify state
     */

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
 * end fbe_update_provision_drive_sniff_verify_state_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_pool_id_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the update provision drive pool-id 
 *  job service command.
 *
 * @param packet_p          - Job service packet.
 * @param update_pvd_p      - Update provision drive configuration request.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/16/2011 - Created. Arun S
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_pool_id_validation_function(fbe_packet_t *packet_p, 
                                                       fbe_job_service_update_provision_drive_t * update_pvd_p, 
                                                       fbe_job_service_error_type_t *error_code)
{
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;

    /*! get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* TODO: Add the appropriate validation before updating pool-id */

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
 * end fbe_update_provision_drive_pool_id_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_serial_num_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the update provision drive serial num
 *  job service command.
 *
 * @param packet_p          - Job service packet.
 * @param update_pvd_p      - Update provision drive configuration request.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  02/23/2012 - Created. Jian Gao
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_serial_num_validation_function(fbe_packet_t *packet_p, 
                                                       fbe_job_service_update_provision_drive_t * update_pvd_p, 
                                                       fbe_job_service_error_type_t *error_code)
{
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;

    /*! get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* TODO: Add the appropriate validation before updating serial num */

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
 * end fbe_update_provision_drive_pool_id_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive config type in memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
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
    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;
    if(update_pvd_p == NULL)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_start_database_transaction(&update_pvd_p->transaction_id, job_queue_element_p->job_number);
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


    switch(update_pvd_p->update_type)
    {
        case FBE_UPDATE_PVD_TYPE:
        /* update the provision drive config type. */
             status = fbe_update_provision_drive_configuration(update_pvd_p->transaction_id,
                                                               update_pvd_p->object_id,
                                                               update_pvd_p->config_type,
                                                               update_pvd_p->update_type,
                                                               update_pvd_p->opaque_data);
            break;
        case FBE_UPDATE_PVD_SNIFF_VERIFY_STATE:
            status = fbe_update_provision_drive_sniff_verify_state(update_pvd_p->transaction_id,
                                                                   update_pvd_p->object_id,
                                                                   update_pvd_p->sniff_verify_state,
                                                                   update_pvd_p->update_type);
            break;
        case FBE_UPDATE_PVD_POOL_ID:
            status = fbe_update_provision_drive_pool_id(update_pvd_p->transaction_id,
                                                        update_pvd_p->object_id, 
                                                        update_pvd_p->pool_id, 
                                                        update_pvd_p->update_type);
            break;
        case FBE_UPDATE_PVD_SERIAL_NUMBER:
            status = fbe_update_provision_drive_serial_num(update_pvd_p->transaction_id,
                                                        update_pvd_p->object_id, 
                                                        update_pvd_p->serial_num,
                                                        update_pvd_p->update_type);
            break;
        default:
            break;
    }
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
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_provision_drive_update_configuration_in_memory_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_rollback_function()
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
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_rollback_function(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_status_t                payload_status;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "update_pvd_rollback_fun entry\n");

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
    update_pvd_p = (fbe_job_service_update_provision_drive_t *)job_queue_element_p->command_data;
    if (update_pvd_p == NULL)
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
                "update_pvd_rollback_fun, rollback not required, prev state %d, cur state %d error: %d\n", 
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
                          "update_pvd_rollback_fun: abort transaction id %llu\n",
			  (unsigned long long)update_pvd_p->transaction_id);

        /* rollback the configuration transaction. */
        status = fbe_create_lib_abort_database_transaction(update_pvd_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "update_pvd_rollback_fun: cannot abort transaction %llu with cfg service\n", 
                              (unsigned long long)update_pvd_p->transaction_id);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }
    }

    /* Send a notification for this rollback. */
    status = fbe_update_provision_drive_send_notification(update_pvd_p,
                                                          job_queue_element_p->job_number,
                                                          job_queue_element_p->error_code,
                                                          FBE_JOB_ACTION_STATE_ROLLBACK);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "update_pvd_rollback_fun: status change notification fail, status:%d\n",
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
                "update_pvd_rollback_fun, payload_status: 0x%x error: %d\n", 
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
 * end fbe_update_provision_drive_rollback_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_persist_function()
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
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
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
    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(update_pvd_p->transaction_id);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to commit the provision drive config type, status:%d, config_type:0x%x\n",
                          __FUNCTION__, status, update_pvd_p->config_type);

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
 * end fbe_update_provision_drive_persist_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_commit_function()
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
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *      update_pvd_p = NULL;
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_status_t                    payload_status;
    fbe_payload_control_buffer_length_t             length = 0;
    fbe_job_queue_element_t *                       job_queue_element_p = NULL;
    fbe_char_t *                                    config_type_str = NULL;

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
    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;

    /* log the trace for the config type update.*/
    switch(update_pvd_p->update_type)
    {
        case FBE_UPDATE_PVD_TYPE:
            fbe_update_provision_drive_config_type_str(update_pvd_p->config_type, &config_type_str);
            job_service_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "update_pvd_commit: pvd object's cfg type updated as %s,pvd_obj_id:0x%x\n",
                              config_type_str, update_pvd_p->object_id);
            break;
        case FBE_UPDATE_PVD_SNIFF_VERIFY_STATE:
            fbe_pvd_event_log_sniff_verify_enable_disable(update_pvd_p->object_id, 
                                                          update_pvd_p->sniff_verify_state);
            break;
        case FBE_UPDATE_PVD_POOL_ID:
            job_service_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "update_pvd_commit: PVD(0x%x) Pool-id: %d. Updated.\n",
                              update_pvd_p->object_id, update_pvd_p->pool_id);
            break;
        default:
            break;
    }

    /* Send a notification for this commit*/
    status = fbe_update_provision_drive_send_notification(update_pvd_p,
                                                          job_queue_element_p->job_number,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR,
                                                          FBE_JOB_ACTION_STATE_COMMIT);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "update_pvd_commit: stat change notification fail, stat %d\n", status);
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
    job_queue_element_p->object_id = update_pvd_p->object_id;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_update_provision_drive_commit_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_configuration()
 ******************************************************************************
 * @brief
 * This function is used to update the provision drive object.
 *
 * @param transaction_id          - Transaction id.
 * @param provision_drive_object  - Provision drive object.
 * @param configuration_mode      - Configuration mode which we want to update.
 * @param update_type             - Configuration update type.
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  04/27/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_configuration(fbe_database_transaction_id_t transaction_id, 
                                         fbe_object_id_t object_id,
                                         fbe_provision_drive_config_type_t config_type,
                                         fbe_update_pvd_type_t update_type,
                                         fbe_u8_t * opaque_data)
{
    fbe_status_t                       status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_pvd_t  update_provision_drive;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: pvd obj: 0x%x config_type: %d \n",
                           __FUNCTION__, object_id, config_type);

    /* Update the transaction details to update the configuration. */
    update_provision_drive.transaction_id = transaction_id;
    update_provision_drive.object_id = object_id;
    update_provision_drive.config_type = config_type;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.update_type = update_type;
    update_provision_drive.update_opaque_data = FBE_TRUE;
    fbe_copy_memory(update_provision_drive.opaque_data, opaque_data, sizeof(update_provision_drive.opaque_data));

    /* Send the update provision drive configuration to configuration service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                &update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_PVD failed\n", __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_config_type()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_sniff_verify_state()
 ******************************************************************************
 * @brief
 * This function is used to update the provision drive object.
 *
 * @param transaction_id          - Transaction id.
 * @param provision_drive_object  - Provision drive object.
 * @param sniff_verify_state      - sniff verify state.
 * @param update_type             - Configuration update type.
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  04/27/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_sniff_verify_state(fbe_database_transaction_id_t transaction_id, 
                                        fbe_object_id_t object_id,
                                        fbe_bool_t sniff_verify_state,
                                        fbe_update_pvd_type_t update_type)
{
    fbe_status_t                                          status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_pvd_t  update_provision_drive;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: pvd obj: 0x%x sniff_verify_state: %d \n",
                           __FUNCTION__, object_id, sniff_verify_state);

    /* Update the transaction details to update the configuration. */
    update_provision_drive.transaction_id = transaction_id;
    update_provision_drive.object_id = object_id;
    update_provision_drive.sniff_verify_state = sniff_verify_state;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.update_type = update_type;
    update_provision_drive.update_opaque_data = FBE_FALSE;
    fbe_zero_memory(update_provision_drive.opaque_data, sizeof(update_provision_drive.opaque_data));

    /* Send the update provision drive configuration to configuration service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                &update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_PVD failed\n", __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_sniff_verify_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_pool_id()
 ******************************************************************************
 * @brief
 * This function is used to update the provision drive object pool-id.
 *
 * @param transaction_id          - Transaction id.
 * @param object_id               - Provision drive object.
 * @param pool_id                 - Pool which the PVD belongs to.
 * @param update_type             - Configuration update type.
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  06/16/2011 - Created. Arun S
 *
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_pool_id(fbe_database_transaction_id_t transaction_id, 
                                   fbe_object_id_t object_id, 
                                   fbe_u32_t pool_id, 
                                   fbe_update_pvd_type_t update_type)
{
    fbe_status_t                       status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_pvd_t  update_provision_drive;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: pvd obj: 0x%x pool_id: %d \n",
                           __FUNCTION__, object_id, pool_id);

    /* Update the transaction details to update the configuration. */
    update_provision_drive.transaction_id = transaction_id;
    update_provision_drive.object_id = object_id;
    update_provision_drive.update_type = update_type;
    update_provision_drive.pool_id = pool_id;
    update_provision_drive.sniff_verify_state = FBE_FALSE;
    update_provision_drive.update_opaque_data = FBE_FALSE;
    fbe_zero_memory(update_provision_drive.opaque_data, sizeof(update_provision_drive.opaque_data));

    /* Send the update provision drive configuration to configuration service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                &update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_PVD failed\n", __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_pool_id()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_update_provision_drive_serial_num()
 ******************************************************************************
 * @brief
 * This function is used to update the provision drive object pool-id.
 *
 * @param transaction_id          - Transaction id.
 * @param object_id               - Provision drive object.
 * @param update_type             - Configuration update type.
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  02/23/2012 - Created. Jian G
 *
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_serial_num(fbe_database_transaction_id_t transaction_id, 
                                   fbe_object_id_t object_id,
                                   fbe_u8_t            *serial_num,
                                   fbe_update_pvd_type_t update_type)
{
    fbe_status_t                       status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_pvd_t  update_provision_drive;

    fbe_base_library_trace(FBE_LIBRARY_ID_INVALID,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: pvd obj: 0x%x Serial Num: %s.\n",
                           __FUNCTION__, object_id, serial_num);

    /* Update the transaction details to update the configuration. */
    update_provision_drive.transaction_id = transaction_id;
    update_provision_drive.object_id = object_id;
    fbe_copy_memory(update_provision_drive.serial_num, serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    update_provision_drive.update_type = update_type;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.sniff_verify_state = FBE_FALSE;
    update_provision_drive.update_opaque_data = FBE_FALSE;
    fbe_zero_memory(update_provision_drive.opaque_data, sizeof(update_provision_drive.opaque_data));

    /* Send the update provision drive configuration to configuration service. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                &update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s FBE_DATABASE_CONTROL_CODE_UPDATE_PVD failed\n", __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_serial_num()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_update_provision_drive_send_notification()
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
 *  09/30/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_send_notification(fbe_job_service_update_provision_drive_t * update_pvd_p,
                                             fbe_u64_t job_number,
                                             fbe_job_service_error_type_t job_service_error_type,
                                             fbe_job_action_state_t job_action_state)
{
    fbe_notification_info_t     notification_info;
    fbe_status_t                status;

    /* fill up the notification information before sending notification. */
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = update_pvd_p->object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_service_error_type;
    notification_info.notification_data.job_service_error_info.job_number = job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE;
    notification_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;

    /* If the request failed generate a trace.
     */
    if (job_service_error_type != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: update notification job: 0x%llx pvd obj: 0x%x failed. error: %d\n",
                          (unsigned long long)job_number, update_pvd_p->object_id, job_service_error_type);
        if (job_service_error_type == FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: update notification job: 0x%llx pvd obj: 0x%x failed. error: %d\n",
                              (unsigned long long)job_number, update_pvd_p->object_id, job_service_error_type);
        }
    }

    /* trace some information
     */
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "UPDATE: provision drive config notification pvd obj: 0x%x type: %d config: %d job: 0x%llx\n",
                      update_pvd_p->object_id, update_pvd_p->update_type, update_pvd_p->config_type, (unsigned long long)job_number);

    /* send the notification to registered callers. */
    status = fbe_notification_send(update_pvd_p->object_id, notification_info);
    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_send_notification()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_update_provision_drive_config_type_str()
 ******************************************************************************
 * @brief
 * Helper function to convert the config type to string.
 * 
 * @param config_type               - provision drive config type.
 * @param config_type_str           - string that represents the config type.
 * 
 * @return status                   - The status of the operation.
 *
 * @author
 *  09/30/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_update_provision_drive_config_type_str(fbe_provision_drive_config_type_t config_type,
                                           fbe_char_t ** config_type_str)
{
    switch(config_type)
    {
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            *config_type_str = "UNKNOWN";
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            *config_type_str = "SPARE";
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
            *config_type_str = "RAID";
            break;
        default:
            *config_type_str = "INVALID";
            break;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_provision_drive_config_type_to_str()
 ******************************************************************************/
