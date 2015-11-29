/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the drive swap service related functionality.
 *
 * @version
 *  8/26/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_spare.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_spare_lib_private.h"
#include "fbe_job_service.h"
#include "fbe_private_space_layout.h"

/************************** 
 *  GLOBALS
 ***************************/
/* The is the amount of time we allow a single (each transaction has multiple 
 * operations) operation to take before we fail the job.  Any modification
 * to this value is not persisted.  Thus it will be restored to the default
 * value if the job service moves or is rebooted.
 */
static fbe_u32_t  fbe_spare_lib_operation_timeout_seconds = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS;
static fbe_bool_t fbe_spare_lib_operation_confirmation_enable = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_CONFIRMATION_ENABLE;

/* Drive swap library request validation function.
 */
static fbe_status_t fbe_spare_lib_drive_swap_request_validation_function(fbe_packet_t *packet_p);

/* Drive swap library select spare function.
 */
static fbe_status_t fbe_spare_lib_drive_swap_request_select_spare_function(fbe_packet_t *packet_p);

/* Drive swap library update configuration in memory function.
 */
static fbe_status_t fbe_spare_lib_drive_swap_request_update_configuration_in_memory_function(fbe_packet_t *packet_p);

/* Drive swap library persist configuration function.
 */
static fbe_status_t fbe_spare_lib_drive_swap_request_persist_configuration_db_function(fbe_packet_t *packet_p);

/* Drive swap library rollback function.
 */
static fbe_status_t fbe_spare_lib_drive_swap_request_rollback_function(fbe_packet_t *packet_p);

/* Drive swap library commit function.
 */
static fbe_status_t fbe_spare_lib_drive_swap_request_commit_function(fbe_packet_t *packet_p);

/* Update spare library configuration validation function */
static fbe_status_t fbe_update_spare_library_config_validation_function(fbe_packet_t *packet_p);

/* Update spare library configuration in-memory function */
static fbe_status_t fbe_update_spare_library_config_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Update spare library configuration rollback */
static fbe_status_t fbe_update_spare_library_config_rollback_function (fbe_packet_t *packet_p);

/* Job service swap request registration.
*/
fbe_job_service_operation_t fbe_spare_job_service_operation = 
{
    FBE_JOB_TYPE_DRIVE_SWAP,
    {
        /* validation function */
        fbe_spare_lib_drive_swap_request_validation_function,

        /* selection function */
        fbe_spare_lib_drive_swap_request_select_spare_function,

        /* update in memory function */
        fbe_spare_lib_drive_swap_request_update_configuration_in_memory_function,

        /* persist function */
        fbe_spare_lib_drive_swap_request_persist_configuration_db_function,

        /* rollback function */
        fbe_spare_lib_drive_swap_request_rollback_function,

        /* commit function */
        fbe_spare_lib_drive_swap_request_commit_function,
    }
};

/* Update the spare library configuration (time to wait for requests, etc) 
 */
fbe_job_service_operation_t fbe_update_spare_library_config_operation = 
{
    FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG,
    {
        /* validation function */
        fbe_update_spare_library_config_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_update_spare_library_config_update_configuration_in_memory_function,

        /* persist function */
        NULL,

        /* response/rollback function */
        fbe_update_spare_library_config_rollback_function,

        /* commit function */
        NULL,
    }
};

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 * fbe_spare_lib_drive_swap_request_select_spare_function()
 ******************************************************************************
 * @brief
 * This function is used to validate the drive swap request coming from the 
 * VD object.
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param swap_request_p - Drive swap request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/13/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_drive_swap_request_validation_function(fbe_packet_t  *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_drive_swap_request_t   *js_swap_request_p = NULL;
    fbe_spare_swap_command_t                swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_virtual_drive_get_info_t            virtual_drive_info;
    fbe_object_id_t                         swap_request_orig_pvd_id = FBE_OBJECT_ID_INVALID;
 
    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    status = fbe_spare_lib_utils_get_control_buffer(packet_p,
                                                    &control_operation_p,
                                                    (void **)&job_queue_element_p,
                                                    sizeof(*job_queue_element_p), __FUNCTION__);
    if (status != FBE_STATUS_OK)
    {
        /* packet has been already completed. */
        return status;
    }
    
    /* Initialize the job queue element status.
     */
    job_queue_element_p->status = FBE_STATUS_INVALID;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID;

    /* Get the job swap request.
     */
    js_swap_request_p = (fbe_job_service_drive_swap_request_t *) job_queue_element_p->command_data;
    if (js_swap_request_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s job service queue element command data is null\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);
    job_queue_element_p->object_id = vd_object_id;
    job_queue_element_p->class_id = FBE_CLASS_ID_VIRTUAL_DRIVE;
    
    /* Initialize the swap status to invalid.
     */
    fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID);
    fbe_job_service_drive_swap_request_set_internal_error(js_swap_request_p, FBE_SPARE_INTERNAL_ERROR_INVALID);

    /* Now perform basic validation using the virtual drive object id supplied.
     */
    status = fbe_spare_lib_validation_validate_virtual_drive_object_id(js_swap_request_p);
    if (status != FBE_STATUS_OK)
    {
        /* Mark the job in error so that we rollback the transaction and
         * generate the proper event log etc.
         */
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine if the operation confirmation mechanism has been disabled.
     * By default it should be enabled.
     */
    if (fbe_spare_main_is_operation_confirmation_enabled())
    {
        /* This is the normal case.  Enable confirmation for the entire request.
         */
        fbe_job_service_drive_swap_request_set_confirmation_enable(js_swap_request_p, FBE_TRUE);
    }
    else
    {
        /* Else generate a warning and disable confirmation (i.e. don't wait
         * for the virtual drive monitor).  This should only be done for testing!
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s vd obj: 0x%x job: %lld confirmation disabled !\n",
                               __FUNCTION__, vd_object_id, job_queue_element_p->job_number);

        /* Disable confirmation for this job.
         */
        fbe_job_service_drive_swap_request_set_confirmation_enable(js_swap_request_p, FBE_FALSE);
    }

    /* Based on drive swap request command type drive swap service library 
     * will perform the different validation.
     */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command_type);
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s :swap request command type: %d.\n", 
                           __FUNCTION__, swap_command_type);

    /* Get the virtual drive information.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                     &virtual_drive_info,
                                                     sizeof (fbe_virtual_drive_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: get vd info failed - status:%d\n", 
                               __FUNCTION__, status);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);


        /* Mark the job in error so that we rollback the transaction and
         * generate the proper event log etc.
         */
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate the original pvd object id supplied (unless it is an 
     * abort copy operation)
     */
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(js_swap_request_p, &swap_request_orig_pvd_id);
    if (swap_command_type != FBE_SPARE_ABORT_COPY_COMMAND)
    {
        /* Validate the original pvd id supplied.
         */
        if (swap_request_orig_pvd_id != virtual_drive_info.original_pvd_object_id)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: request orig: 0x%x doesnt match actual: 0x%x\n",
                                   __FUNCTION__, swap_request_orig_pvd_id, virtual_drive_info.original_pvd_object_id);
    
            /* Set the proper status in the job request.
             */
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID);
    
            /* Mark the job in error so that we rollback the transaction and
             * generate the proper event log etc.
             */
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Now set the swap index from the virtual drive information for user requests.
     */
    if ((swap_command_type == FBE_SPARE_INITIATE_USER_COPY_COMMAND)    ||
        (swap_command_type == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )
    {
        /* Set the swap edge index.
         */
        fbe_job_service_drive_swap_request_set_swap_edge_index(js_swap_request_p, virtual_drive_info.swap_edge_index);

        /* For user request validate the swap-in edge index.
         */
        status = fbe_spare_lib_validation_validate_user_request_swap_edge_index(js_swap_request_p);
        if (status != FBE_STATUS_OK)
        {
            /* Mark the job in error so that we rollback the transaction and
             * generate the proper event log etc.
             */
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Based on the command type perform the validation.
     */
    switch (swap_command_type)
    {
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* Validate the incoming drive swap in spare request. */
            status = fbe_spare_lib_validation_swap_in_request(js_swap_request_p);
            break;
        case FBE_SPARE_COMPLETE_COPY_COMMAND:
        case FBE_SPARE_ABORT_COPY_COMMAND:
            /* Validate the incoming drive swap out request. */
            status = fbe_spare_lib_validation_swap_out_request(js_swap_request_p);
            break;
        default:
            /* invalid swap command type. */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s :invalid swap command type %d",__FUNCTION__, swap_command_type);

            /* Mark the job in error so that we rollback the transaction and
             * generate the proper event log etc.
             */
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            status =  FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Based on the status update the job element status.
     */
    if (status == FBE_STATUS_OK)
    {
        /* If the validation succeeded change the swap status to `success'.
         */
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_NO_ERROR);

        /* Update job_queue_element status 
         */
        job_queue_element_p->status = FBE_STATUS_OK;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
    }
    else
    {
        /* Else mark the job in error so that we rollback the transaction and
         * generate the proper event log etc.
         */
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_drive_swap_request_validation_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_drive_swap_request_select_spare_function()
 ******************************************************************************
 * @brief
 * This function is used to find the best suitable spare using sparing algorithm.
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param swap_request_p - Drive swap request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/13/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_spare_lib_drive_swap_request_select_spare_function(fbe_packet_t *packet_p)
{
    fbe_job_service_drive_swap_request_t *  js_swap_request_p = NULL;
    fbe_spare_swap_command_t                swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    status = fbe_spare_lib_utils_get_control_buffer(packet_p,
                                                    &control_operation_p,
                                                    (void **)&job_queue_element_p,
                                                    sizeof(*job_queue_element_p), __FUNCTION__);
    if(status != FBE_STATUS_OK)
    {
        /* packet has been already completed. */
        return status;
    }

    js_swap_request_p = (fbe_job_service_drive_swap_request_t *) job_queue_element_p->command_data;
    if (js_swap_request_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s job service queue element command data is null\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Based on drive swap request command type drive swap service library 
     * will perform different operations like SWAP_IN, SWAP_OUT etc.
     */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command_type);

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s :swap request command type: %d.\n", 
                           __FUNCTION__, swap_command_type);

    job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND;

    switch (swap_command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            /* Process drive swap-in request to select the best suitable hot spare/proactive spare. */
            status = fbe_spare_lib_selection_find_best_suitable_spare(packet_p, js_swap_request_p);
            break;
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
        case FBE_SPARE_COMPLETE_COPY_COMMAND:
        case FBE_SPARE_ABORT_COPY_COMMAND:
            /* These commands are not applicable for selection of the spare. */
            job_queue_element_p->status = FBE_STATUS_OK;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            break;
        default:
            /* Invalid command type.*/
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s :invalid swap command type %d",
                                   __FUNCTION__, swap_command_type);

            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            break;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_drive_swap_request_select_spare_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_drive_swap_request_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 * This function is used to update the configuration in memory
 * based on specific swap request.
 * 
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param swap_request_p - Drive swap request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/31/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_spare_lib_drive_swap_request_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_job_service_drive_swap_request_t *  js_swap_request_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_spare_swap_command_t                swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    status = fbe_spare_lib_utils_get_control_buffer(packet_p,
                                                    &control_operation_p,
                                                    (void **)&job_queue_element_p,
                                                    sizeof(*job_queue_element_p), __FUNCTION__);
    if(status != FBE_STATUS_OK)
    {
        /* packet has been already completed. */
        return status;
    }

    js_swap_request_p = (fbe_job_service_drive_swap_request_t *) job_queue_element_p->command_data;
    if (js_swap_request_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s job service queue element command data is null\n", __FUNCTION__);
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Based on drive swap request command type drive swap service library 
     * will perform different configuration update in memory.
     */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command_type);

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s :swap request command type: %d.\n", 
                           __FUNCTION__, swap_command_type);

    /* initialize to default status */
    job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND;

    switch (swap_command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            /* swap-in permanent spare drive. */
            status = fbe_spare_lib_swap_in_permanent_spare(packet_p, js_swap_request_p);
            break;
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* Invoke method to initiate the copy operation.
             */
            status = fbe_spare_lib_initiate_copy_operation(packet_p, js_swap_request_p);
            break;
        case FBE_SPARE_COMPLETE_COPY_COMMAND:
            /* Invoke method to complete the copy operation.
             */
            status = fbe_spare_lib_complete_copy_operation(packet_p, js_swap_request_p);
            break;
        case FBE_SPARE_ABORT_COPY_COMMAND:
            /* Invoke the method to complete the aborted copy operation.
             */
            status = fbe_spare_lib_complete_aborted_copy_operation(packet_p, js_swap_request_p);
            break;
        default:            
            /* Invalid command type.*/
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: invalid swap command type %d \n", 
                                   __FUNCTION__, swap_command_type);

            status =  FBE_STATUS_GENERIC_FAILURE;
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            break;
    }

    return status;
}
/*******************************************************************************
 * end fbe_spare_lib_drive_swap_request_update_configuration_in_memory_function()
 *******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_drive_swap_request_persist_configuration_db_function()
 ******************************************************************************
 * @brief
 * This function is used to persist the configuration in database.
 * 
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param swap_request_p - Drive swap request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/13/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_spare_lib_drive_swap_request_persist_configuration_db_function(fbe_packet_t *packet_p)
{
    fbe_job_service_drive_swap_request_t *  js_swap_request_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_queue_element_t *               job_queue_element_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_spare_swap_command_t                swap_command_type;
    fbe_system_encryption_mode_t            encryption_mode;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    status = fbe_spare_lib_utils_get_control_buffer(packet_p,
                                                    &control_operation_p,
                                                    (void **)&job_queue_element_p,
                                                    sizeof(*job_queue_element_p), __FUNCTION__);
    if (status != FBE_STATUS_OK)
    {
        /* packet has been already completed. */
        return status;
    }

    js_swap_request_p = (fbe_job_service_drive_swap_request_t *) job_queue_element_p->command_data;
    if (js_swap_request_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s job service queue element command data is null\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Commit the transaction while persisting the configuration changes. */
    status = fbe_spare_lib_utils_commit_database_transaction(js_swap_request_p->transaction_id);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to commit the drive swap request, status:%d, tran_id:0x%llx, swap_cmd:0x%x\n",
                          __FUNCTION__, status,
              (unsigned long long)js_swap_request_p->transaction_id,
              js_swap_request_p->command_type);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the swap command type of the swap request. */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command_type);

    /* Determine if encryption mode is enabled.
     */
    fbe_database_get_system_encryption_mode(&encryption_mode);

    /* swap request failed, return appropriate status to Virtual Drive object */
    switch (swap_command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* For internally generated request send the status of the operation
             * to the virtual drive.  Do not wait for confirmation.
             */
            js_swap_request_p->b_operation_confirmation = FBE_FALSE;
            if (swap_command_type == FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
            {
                /* wait for permanent spare confirmation is NEEDED since many Freddie test will fail without
                 */
                js_swap_request_p->b_operation_confirmation = FBE_TRUE;
            }
            status = fbe_spare_lib_utils_send_swap_completion_status(js_swap_request_p,
                                                                     js_swap_request_p->b_operation_confirmation);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = (status == FBE_STATUS_OK) ? 
                                                FBE_JOB_SERVICE_ERROR_NO_ERROR : FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            break;

        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* For user initiated requests the notification contains the status
             * of the request. Do not wait for confirmation.
             */
            js_swap_request_p->b_operation_confirmation = FBE_FALSE;
            status = fbe_spare_lib_utils_send_swap_completion_status(js_swap_request_p,
                                                                     js_swap_request_p->b_operation_confirmation);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = js_swap_request_p->status_code;
            break;

        case FBE_SPARE_COMPLETE_COPY_COMMAND:
        case FBE_SPARE_ABORT_COPY_COMMAND:
            /* For internally generated request send the status of the operation
             * to the virtual drive.
             */
            status = fbe_spare_lib_utils_send_swap_completion_status(js_swap_request_p,
                                                                     js_swap_request_p->b_operation_confirmation);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = (status == FBE_STATUS_OK) ? 
                                                FBE_JOB_SERVICE_ERROR_NO_ERROR : FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            /* If encryption is enabled and this was a copy complete and it was 
             * successful, clear the swap pending.
             */
            if ((job_queue_element_p->error_code == FBE_JOB_SERVICE_ERROR_NO_ERROR) &&
                (encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)           &&
                (swap_command_type == FBE_SPARE_COMPLETE_COPY_COMMAND)                 )
            {
                status = fbe_spare_lib_utils_database_service_clear_pvd_swap_pending(js_swap_request_p);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                           FBE_TRACE_LEVEL_ERROR,
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                           "%s: vd obj: 0x%x source obj: 0x%x clear pvd offline failed - status: 0x%x\n",
                                           __FUNCTION__, js_swap_request_p->vd_object_id, js_swap_request_p->orig_pvd_object_id, status);

                    /*! @note We don't want to fail the request so set the 
                     *        status to success.
                     */
                    status = FBE_STATUS_OK;
                }
            }
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND;
            /* Invalid command type.*/
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s :invalid swap command type %d", __FUNCTION__, swap_command_type);
            break;
    }

    /* If the persist failed we still complete the packet in success but we
     * set the payload status to failure.  The swap request will be rolled back.
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* Complete the packet with success
     */
    fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
    
    /* Return the execution status (this is ignored by the job service).
     */ 
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_drive_swap_request_persist_configuration_db_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_drive_swap_rollback_function()
 ******************************************************************************
 * @brief
 * This function is used to send the response to the caller(virtual drive
 * object) on error.
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param swap_request_p - Drive swap request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/13/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_drive_swap_request_rollback_function(fbe_packet_t *packet_p)
{   
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_drive_swap_request_t   *js_swap_request_p = NULL;
    fbe_spare_swap_command_t                swap_command_type;
    fbe_payload_control_status_t            payload_status;
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    status = fbe_spare_lib_utils_get_control_buffer(packet_p,
                                                    &control_operation_p,
                                                    (void **)&job_queue_element_p,
                                                    sizeof(*job_queue_element_p), __FUNCTION__);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Get the swap request status.
     */
    js_swap_request_p = (fbe_job_service_drive_swap_request_t *) job_queue_element_p->command_data;
    if (js_swap_request_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s swap request is NULL\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the virtual drive object-id from the job service swap-out request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id (js_swap_request_p, &vd_object_id);

    /* roll back the transaction if we get an error while persisting or updating configuration. */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)          ||
        (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)    )
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "spare_drive_swap_req_rollback: aborting transaction id %llu\n",
              (unsigned long long)js_swap_request_p->transaction_id);

        /* Tell the virtual drive to perform any cleanup before starting the rollback.
         */
        status = fbe_spare_lib_utils_send_swap_request_rollback(vd_object_id, js_swap_request_p);
        if (status != FBE_STATUS_OK) 
        {
            /* The virtual drive could not cleanup.  We cannot perform the rollback.
             */
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "spare_drive_swap_req_rollback:Virtual drive rollback failed id %llu \n", (unsigned long long)js_swap_request_p->transaction_id);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        }
        else
        {
            /* Else abort the transaction to rollback. */
            status = fbe_spare_lib_utils_abort_database_transaction(js_swap_request_p->transaction_id);
            if(status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "spare_drive_swap_req_rollback:Cannot abort transaction id %llu with cfg service\n", (unsigned long long)js_swap_request_p->transaction_id);
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            }
        }
    }
    else
    {
        /* abort is not required during validation or selection of spare failure. */
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "spare_drive_swap_req_rollback abort not required,pre_state %d,cur_state %d,vd_id:0x%x\n",
                          job_queue_element_p->previous_state, job_queue_element_p->current_state, vd_object_id);        
    }

    /* We expect a failure status.  If no error found set the status to
     * `internal error'
     */
    if ((js_swap_request_p->status_code == FBE_JOB_SERVICE_ERROR_NO_ERROR) ||
        (job_queue_element_p->status    == FBE_STATUS_OK)                     )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s unexpected error code: %d or status: 0x%x\n", 
                               __FUNCTION__, js_swap_request_p->status_code, job_queue_element_p->status);
        js_swap_request_p->status_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    }

    /* Copy the swap error code to the job element error code.
     */
    fbe_job_service_drive_swap_request_get_status(js_swap_request_p, &job_queue_element_p->error_code);

    /* Get the swap command type of the swap request. */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &swap_command_type);

    /* Swap request failed, return appropriate status to Virtual Drive object */
    switch (swap_command_type)
    {
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
        case FBE_SPARE_COMPLETE_COPY_COMMAND:
        case FBE_SPARE_ABORT_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* For all request types send the usurper to the virtual drive so
             * that it can perform any necessary cleanup.
             */
            status = fbe_spare_lib_utils_send_swap_completion_status(js_swap_request_p,
                                                                     js_swap_request_p->b_operation_confirmation);
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND;

            /* Invalid command type.*/
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "spare_drive_swap_req_rollback: invalid swap command type %d", swap_command_type);
            
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            break;
    }

    /* Generate the proper event based on the swap command type and status.
     */
    status = fbe_spare_lib_utils_rollback_write_event_log(js_swap_request_p);
    if (status != FBE_STATUS_OK)
    {
        /*  We generate an error trace but continue (so that we cleanup)
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s write event log failed. status: 0x%x\n", __FUNCTION__, status);
    }

    /*! @note For user initiated requests it is the notification that populates
     *        the response status.
     */
    status = fbe_spare_lib_utils_send_notification(js_swap_request_p,
                                                   job_queue_element_p->job_number,
                                                   job_queue_element_p->status,
                                                   FBE_JOB_ACTION_STATE_ROLLBACK);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s send notification failed. status: 0x%x\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* set the payload status to ok if it is not updated as failure. */
    fbe_payload_control_get_status(control_operation_p, &payload_status);
    if (payload_status != FBE_PAYLOAD_CONTROL_STATUS_FAILURE)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    /* Notify the virtual drive that the request is complete.
     * We purposefully ignore the status.
     */
    fbe_spare_lib_utils_send_job_complete(vd_object_id);

    /* Complete the packet with success
     */
    fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
    
    /* Return the execution status (this is ignored by the job service).
     */ 
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_drive_swap_request_rollback_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_drive_swap_request_commit_function()
 ******************************************************************************
 * @brief
 *  This function is used to send the notification for the job service
 *  completion to the caller and it is also used to send the usurpur command
 *  to virtual drive object to clear the pending flag for the job service
 *  operation.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  06/01/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_spare_lib_drive_swap_request_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_drive_swap_request_t   *js_swap_request_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_payload_control_status_t            payload_status;
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    
    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    status = fbe_spare_lib_utils_get_control_buffer(packet_p,
                                                    &control_operation_p,
                                                    (void **)&job_queue_element_p,
                                                    sizeof(*job_queue_element_p), __FUNCTION__);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    js_swap_request_p = (fbe_job_service_drive_swap_request_t *) job_queue_element_p->command_data;
    if (js_swap_request_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s job service queue element command data is null\n", __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the virtual drive object-id from the job service swap-out request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id (js_swap_request_p, &vd_object_id);

    /* We expect a success status.  If an error is found set the status to
     * `internal error'
     */
    if ((js_swap_request_p->status_code != FBE_JOB_SERVICE_ERROR_NO_ERROR) ||
        (job_queue_element_p->status    != FBE_STATUS_OK)                     )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s unexpected error code: %d or status: 0x%x\n", 
                               __FUNCTION__, js_swap_request_p->status_code, job_queue_element_p->status);
        js_swap_request_p->status_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    }

    /* log the event for the spare command 
     */
    fbe_spare_lib_utils_commit_complete_write_event_log(js_swap_request_p);

    /*! @note For user initiated requests it is the notification that populates
     *        the response status.
     */
    status = fbe_spare_lib_utils_send_notification(js_swap_request_p,
                                                   job_queue_element_p->job_number,
                                                   job_queue_element_p->status,
                                                   FBE_JOB_ACTION_STATE_COMMIT);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s:job status send notification failed, status: 0x%X\n",
                          __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* set the payload status to ok if it is not updated as failure. */
    fbe_payload_control_get_status(control_operation_p, &payload_status);
    if(payload_status != FBE_PAYLOAD_CONTROL_STATUS_FAILURE)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    /* Notify the virtual drive that the request is complete.
     * We purposefully ignore the status.
     */
    fbe_spare_lib_utils_send_job_complete(vd_object_id);

    /* Complete the packet with success
     */
    fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
    
    /* Return the execution status (this is ignored by the job service).
     */ 
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_drive_swap_request_commit_function()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_spare_main_get_operation_timeout_secs()
 ****************************************************************************** 
 * 
 * @brief   Return the operation timeout seconds.
 * 
 * @param   none
 *
 * @return   operation_timeout_secs - operation timeout seconds
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_u32_t fbe_spare_main_get_operation_timeout_secs(void)
{
    return fbe_spare_lib_operation_timeout_seconds;
}
/*************************************************
 * end fbe_spare_main_get_operation_timeout_secs()
 *************************************************/

/*!****************************************************************************
 *          fbe_spare_main_is_operation_confirmation_enabled()
 ****************************************************************************** 
 * 
 * @brief   Return if the job service to object confirmation is enabled or not.
 * 
 * @param   none
 *
 * @return  bool - FBE_TRUE - job service operation to object confirmation is
 *              enable.
 *
 * @author
 *  04/25/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_bool_t fbe_spare_main_is_operation_confirmation_enabled(void)
{
    return fbe_spare_lib_operation_confirmation_enable;
}
/********************************************************
 * end fbe_spare_main_is_operation_confirmation_enabled()
 ********************************************************/

/*!****************************************************************************
 *          fbe_spare_main_set_operation_timeout_secs()
 ****************************************************************************** 
 * 
 * @brief   Set the operation timeout seconds.
 * 
 * @param   operation_timeout_secs - Set operation timeout seconds
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_main_set_operation_timeout_secs(fbe_u32_t operation_timeout_secs)
{
    fbe_spare_lib_operation_timeout_seconds = operation_timeout_secs;
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_spare_main_set_operation_timeout_secs()
 *************************************************/

/*!****************************************************************************
 *          fbe_spare_main_set_operation_confirmation()
 ****************************************************************************** 
 * 
 * @brief   Set the operation confirmation to the value (FBE_TRUE or FBE_FALSE)
 * 
 * @param   b_set_operation_confirmation - FBE_TRUE - Enable operation confirmation
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  04/25/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_main_set_operation_confirmation(fbe_bool_t b_set_operation_confirmation)
{
    fbe_spare_lib_operation_confirmation_enable = b_set_operation_confirmation;
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_spare_main_set_operation_confirmation()
 *************************************************/

/*!****************************************************************************
 *          fbe_spare_main_set_default_operation_timeout_secs()
 ****************************************************************************** 
 * 
 * @brief   Set the operation timeout seconds.
 * 
 * @param   None
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_main_set_default_operation_timeout_secs(void)
{
    fbe_spare_lib_operation_timeout_seconds = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS;
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_spare_main_set_default_operation_timeout_secs()
 ********************************************************/

/*!****************************************************************************
 *          fbe_spare_main_set_default_operation_confirmation_enable()
 ****************************************************************************** 
 * 
 * @brief   Set the operation confirmation enable to the default value.
 * 
 * @param   None
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_spare_main_set_default_operation_confirmation_enable(void)
{
    fbe_spare_lib_operation_confirmation_enable = FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_CONFIRMATION_ENABLE;
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_spare_main_set_default_operation_confirmation_enable()
 ****************************************************************/

/*!****************************************************************************
 *          fbe_update_spare_library_config_validation_function()
 ****************************************************************************** 
 * 
 * @brief   This function is used to validate the `update spare config' job
 *          request.
 * 
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_update_spare_library_config_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_service_update_spare_library_config_request_t  *update_spare_service_config_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t             length = 0;
    fbe_job_queue_element_t                        *job_queue_element_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s NULL job queue element \n", 
                               __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s length: %d not expected: %d \n", 
                               __FUNCTION__, length, (fbe_u32_t)sizeof(fbe_job_queue_element_t));

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the update spare service configuration data.
     */
    update_spare_service_config_p = (fbe_job_service_update_spare_library_config_request_t *)job_queue_element_p->command_data;
    if (update_spare_service_config_p == NULL) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s command is NULL\n", 
                               __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NULL_COMMAND;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the values
     */
    if (update_spare_service_config_p->b_set_default)
    {
        /* Ignore any parameters and set the default values.
         */
    }
    else if (update_spare_service_config_p->b_disable_operation_confirmation == FBE_FALSE)
    {
        /* Else validate the requested values
         */
        if (((fbe_s32_t)update_spare_service_config_p->operation_timeout_secs < FBE_JOB_SERVICE_MINIMUM_SPARE_OPERATION_TIMEOUT_SECONDS) ||
            (update_spare_service_config_p->operation_timeout_secs > FBE_JOB_SERVICE_MAXIMUM_SPARE_OPERATION_TIMEOUT_SECONDS)               )
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s command is NULL\n", 
                               __FUNCTION__);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_VALUE;

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*********************************************************** 
 * end fbe_update_spare_library_config_validation_function()
 ***********************************************************/

/*!****************************************************************************
 * fbe_update_spare_library_config_update_configuration_in_memory_function()
 ******************************************************************************
 *
 * @brief   This function is used to update the `update spare config' job
 *          request.
 * 
 * @param   packet_p          - Job service packet.
 *
 * @return  status           - The status of the operation.
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created.
 ******************************************************************************/
static fbe_status_t fbe_update_spare_library_config_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_job_service_update_spare_library_config_request_t  *update_spare_service_config_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t             length = 0;
    fbe_job_queue_element_t                        *job_queue_element_p = NULL;
    fbe_u32_t                                       current_operation_timeout = fbe_spare_main_get_operation_timeout_secs();
    fbe_bool_t                                      b_set_operation_confirmation = FBE_TRUE;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s NULL job queue element \n", 
                               __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s length: %d not expected: %d \n", 
                               __FUNCTION__, length, (fbe_u32_t)sizeof(fbe_job_queue_element_t));

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the update spare service configuration data.
     */
    update_spare_service_config_p = (fbe_job_service_update_spare_library_config_request_t *)job_queue_element_p->command_data;
    if (update_spare_service_config_p == NULL) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s command is NULL\n", 
                               __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NULL_COMMAND;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Valus have already been validated.  Set them
     */
    b_set_operation_confirmation = (update_spare_service_config_p->b_disable_operation_confirmation) ? FBE_FALSE : FBE_TRUE;
    if (update_spare_service_config_p->b_set_default)
    {
        /* Ignore any parameters and set the default values.
         */
        fbe_spare_main_set_default_operation_timeout_secs();
        fbe_spare_main_set_default_operation_confirmation_enable();

        /* Trace this operation
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: restored default operation confirmation: %d timeout secs: %d\n",
                               fbe_spare_main_is_operation_confirmation_enabled(),
                               fbe_spare_main_get_operation_timeout_secs());
    }
    else if (update_spare_service_config_p->b_disable_operation_confirmation)
    {
        /* Disable the operation confirmation
         */
        fbe_spare_main_set_operation_confirmation(b_set_operation_confirmation);

        /* Trace this operation
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: disabled job service to object confirmation\n");
    }
    else
    {
        /* Else set the value (validation already done)
         */
        fbe_spare_main_set_operation_timeout_secs(update_spare_service_config_p->operation_timeout_secs);
        fbe_spare_main_set_operation_confirmation(b_set_operation_confirmation);

        /* Trace this operation
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare: changed operation timeout secs from: %d to: %d \n",
                               current_operation_timeout, update_spare_service_config_p->operation_timeout_secs);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_update_spare_library_config_update_configuration_in_memory_function()
 *******************************************************************************/

/*!****************************************************************************
 *          fbe_update_spare_library_config_rollback_function()
 ******************************************************************************
 * @brief
 *  This function is used to rollback the udpate spare service config
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  04/24/2013  Ron Proulx  - Created
 ******************************************************************************/
static fbe_status_t fbe_update_spare_library_config_rollback_function (fbe_packet_t *packet_p)
{
    fbe_job_service_update_spare_library_config_request_t  *update_spare_service_config_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t             length = 0;
    fbe_job_queue_element_t                        *job_queue_element_p = NULL;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if (job_queue_element_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s NULL job queue element \n", 
                               __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(fbe_job_queue_element_t))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s length: %d not expected: %d \n", 
                               __FUNCTION__, length, (fbe_u32_t)sizeof(fbe_job_queue_element_t));

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the update spare service configuration data.
     */
    update_spare_service_config_p = (fbe_job_service_update_spare_library_config_request_t *)job_queue_element_p->command_data;
    if (update_spare_service_config_p == NULL) 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s command is NULL\n", 
                               __FUNCTION__);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NULL_COMMAND;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the default values
     */
    fbe_spare_main_set_default_operation_timeout_secs();
    fbe_spare_main_set_default_operation_confirmation_enable();

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* update job_queue_element status */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/************************************************************ 
 * end fbe_update_spare_library_config_rollback_function()
 ************************************************************/


/********************************* 
 * end of file fbe_spare_main.c
 ********************************/
