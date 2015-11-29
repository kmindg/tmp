/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_create.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for provision_drive create used by job service.
 * 
 * @ingroup job_lib_files
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
#include "fbe_database.h"
#include "fbe_raid_library.h"
#include "fbe_private_space_layout.h"

/*************************
 *   FORWARD DECLARATIONS
 *************************/


static fbe_status_t fbe_create_pvd_configuration_service_create_pvd(fbe_database_transaction_id_t transaction_id, 
																	fbe_object_id_t *pvd_object_id,
																	fbe_provision_drive_config_type_t config_type,
																	fbe_lba_t configured_capacity,
																	fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size,
																	fbe_bool_t sniff_verify_state,
																	fbe_config_generation_t generation_number,
																	fbe_u8_t *serial_num);
/* provision_drive create library request validation function.
 */
static fbe_status_t fbe_provision_drive_create_validation_function (fbe_packet_t *packet_p);
/* provision_drive create library update configuration in memory function.
 */
static fbe_status_t fbe_provision_drive_create_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* provision_drive create library persist configuration in db function.
 */
static fbe_status_t fbe_provision_drive_create_persist_configuration_db_function (fbe_packet_t *packet_p);

/* provision_drive create library rollback function.
 */
static fbe_status_t fbe_provision_drive_create_rollback_function (fbe_packet_t *packet_p);

/* provision_drive create library commit function.
 */
static fbe_status_t fbe_provision_drive_create_commit_function (fbe_packet_t *packet_p);


/*************************
 *   DEFINITIONS
 *************************/

/* Job service provision_drive create registration.
*/
fbe_job_service_operation_t fbe_create_provision_drive_job_service_operation = 
{
    FBE_JOB_TYPE_CREATE_PROVISION_DRIVE,
    {
        /* validation function */
        fbe_provision_drive_create_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_provision_drive_create_update_configuration_in_memory_function,

        /* persist function */
        fbe_provision_drive_create_persist_configuration_db_function,

        /* response/rollback function */
        fbe_provision_drive_create_rollback_function,

        /* commit function */
        fbe_provision_drive_create_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_provision_drive_create_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the provision drive create request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 ****************************************************************/

static fbe_status_t fbe_provision_drive_create_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_provision_drive_t *      provision_drive_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_u32_t                           i;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: job_queue_element_p is NULL.\n", 
                          __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: fbe_job_queue_element wrong size.\n", 
                          __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_create_request_p = (fbe_job_service_create_provision_drive_t *) job_queue_element_p->command_data;
    /* we need to make sure no duplicated creation */
    for (i=0; i< provision_drive_create_request_p->request_size; i++)
    {
        /*check database service against serial number */
        if (fbe_database_is_pvd_exists(provision_drive_create_request_p->PVD_list[i].serial_num))
        {
            /* skip */
            provision_drive_create_request_p->PVD_list[i].config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;
        }

    }

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_provision_drive_create_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_provision_drive_create_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the provision drive create request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_create_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_provision_drive_t        *pvd_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_u32_t                           i;


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

    /* get the user data from job command*/
    pvd_create_request_p = (fbe_job_service_create_provision_drive_t *) job_queue_element_p->command_data;

    /* open_transaction in configuration service.  should the type of transaction be given here? */
    status = fbe_create_lib_start_database_transaction(&pvd_create_request_p->transaction_id, job_queue_element_p->job_number);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (i=0; i< pvd_create_request_p->request_size; i++)
    {
        if (pvd_create_request_p->PVD_list[i].config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID)
        {
            status = fbe_create_pvd_configuration_service_create_pvd(pvd_create_request_p->transaction_id,
                                                                    &pvd_create_request_p->PVD_list[i].object_id,                                                              
                                                                    pvd_create_request_p->PVD_list[i].config_type,
                                                                    pvd_create_request_p->PVD_list[i].configured_capacity,
                                                                    pvd_create_request_p->PVD_list[i].configured_physical_block_size,
                                                                    pvd_create_request_p->PVD_list[i].sniff_verify_state,
                                                                    pvd_create_request_p->PVD_list[i].generation_number,
                                                                    &pvd_create_request_p->PVD_list[i].serial_num[0]);
            if(status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                    "%s: transaction %llu, failed to create PVD (SN%s).\n", 
                    __FUNCTION__,
		    (unsigned long long)pvd_create_request_p->transaction_id,
		    &pvd_create_request_p->PVD_list[i].serial_num[0]);
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
   
    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR; 
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_provision_drive_create_update_configuration_in_memory_function()
 **********************************************************/

/*!**************************************************************
 * fbe_provision_drive_create_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the provision_drive create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - provision_drive create request.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_create_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_provision_drive_t        *pvd_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
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

    /* get the user data from job command*/
    pvd_create_request_p = (fbe_job_service_create_provision_drive_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(pvd_create_request_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR; 

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_provision_drive_create_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_provision_drive_create_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the provision_drive create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - provision_drive create request.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_create_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_create_provision_drive_t        *pvd_create_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
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

    /* get the user data from job command*/
    pvd_create_request_p = (fbe_job_service_create_provision_drive_t *) job_queue_element_p->command_data;

    /* see if transaction should be aborted */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
        (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY))
    {
        status = fbe_create_lib_abort_database_transaction(pvd_create_request_p->transaction_id);
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
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_CREATE_PROVISION_DRIVE;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, 
                status);
    }

    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_provision_drive_create_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_provision_drive_create_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the provision_drive create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - provision_drive create request.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_create_commit_function (fbe_packet_t *packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_job_service_create_provision_drive_t        *pvd_create_request_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;
    fbe_u32_t                           i;
    fbe_u32_t                           pvd_index;
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

    /* get the user data from job command*/
    pvd_create_request_p = (fbe_job_service_create_provision_drive_t *) job_queue_element_p->command_data;

    for (i=0; i< pvd_create_request_p->request_size; i++)
    {
        if (pvd_create_request_p->PVD_list[i].config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID)
        {
			/* Send a notification for this commit*/
            notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
            notification_info.notification_data.job_service_error_info.object_id = pvd_create_request_p->PVD_list[i].object_id;
            notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
            notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
            notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
            notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_CREATE_PROVISION_DRIVE;

            notification_info.class_id = FBE_CLASS_ID_INVALID;
            notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

            status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
            if (status != FBE_STATUS_OK) {
                job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: job status notification failed, status: 0x%X\n",
                                __FUNCTION__, 
                                status);
            }
			/*if it is a system pvd, we should take care of the raw mirror*/
			if(fbe_private_space_layout_object_id_is_system_pvd(pvd_create_request_p->PVD_list[i].object_id)
			   && FBE_JOB_SERVICE_ERROR_NO_ERROR == notification_info.notification_data.job_service_error_info.error_code)
			{
			   pvd_index = pvd_create_request_p->PVD_list[i].object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;			   
			   /*set the quiesce flag in raw mirror*/
			   fbe_raw_mirror_set_quiesce_flags_in_all();
			   /*wait for the semphore*/
			   fbe_raw_mirror_wait_all_io_quiesced();
			   /*if we are here, there is no outgoing IO in raw mirror*/
			  fbe_raw_mirror_init_edges_in_all();
			  /*un set the quiese flag in raw mirror*/
			  fbe_raw_mirror_unset_quiesce_flags_in_all();
			   /*unmask the down disk in raw mirror*/
			   fbe_raw_mirror_unmask_down_disk_in_all(pvd_index);			   
			   
			}
        }
    }
    /* Set the payload status */
    fbe_payload_control_set_status(control_operation_p, payload_status);
    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_provision_drive_create_commit_function()
 **********************************************************/



/*!**************************************************************
 * fbe_create_pvd_configuration_service_create_pvd()
 ****************************************************************
 * @brief
 * This function asks configuration service to create a pvd object.  
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_create_pvd_configuration_service_create_pvd(fbe_database_transaction_id_t transaction_id, 
																	fbe_object_id_t *pvd_object_id,
																	fbe_provision_drive_config_type_t config_type,
																	fbe_lba_t configured_capacity,
																	fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size,
																	fbe_bool_t sniff_verify_state,
																	fbe_config_generation_t generation_number,
																	fbe_u8_t *serial_num)                                                                    
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_pvd_t create_pvd;
	fbe_system_encryption_mode_t	system_encryption_mode;

    fbe_zero_memory(&create_pvd, sizeof(fbe_database_control_pvd_t));
    create_pvd.transaction_id = transaction_id;
    create_pvd.object_id = *pvd_object_id;
    create_pvd.pvd_configurations.config_type = config_type;
    create_pvd.pvd_configurations.configured_capacity = configured_capacity;
    create_pvd.pvd_configurations.configured_physical_block_size = configured_physical_block_size;
    create_pvd.pvd_configurations.sniff_verify_state = sniff_verify_state;
    create_pvd.pvd_configurations.generation_number= generation_number;
    fbe_copy_memory(&create_pvd.pvd_configurations.serial_num, serial_num, sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE); 
    
    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        create_pvd.pvd_configurations.update_type_bitmask = FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED;
    }

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_PVD,
                                                &create_pvd,
                                                sizeof(fbe_database_control_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,                                             
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: failed.  Error 0x%x\n", 
                          __FUNCTION__, 
                          status);
        return status;
    }

    *pvd_object_id = create_pvd.object_id;
   
    
    return status;
}
/**********************************************************
 * end fbe_create_pvd_configuration_service_create_pvd()
 **********************************************************/


