/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_destroy_provision_drive.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for PVD destroy used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   12/30/2011:  Created. zhangy
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
static fbe_status_t fbe_destroy_pvd_configuration_service_destroy_pvd(fbe_database_transaction_id_t transaction_id,
                                                                      fbe_object_id_t pvd_object_id);                 
/* PVD destroy library request validation function.
 */
static fbe_status_t fbe_provision_drive_destroy_validation_function (fbe_packet_t *packet_p);
/* PVD destroy library update configuration in memory function.
 */
static fbe_status_t fbe_provision_drive_destroy_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* PVD destroy library persist configuration in db function.
 */
static fbe_status_t fbe_provision_drive_destroy_persist_configuration_db_function (fbe_packet_t *packet_p);

/* PVD destroy library rollback function.
 */
static fbe_status_t fbe_provision_drive_destroy_rollback_function (fbe_packet_t *packet_p);

/* PVD destroy library commit function.
 */
static fbe_status_t fbe_provision_drive_destroy_commit_function (fbe_packet_t *packet_p);


/*************************
 *   DEFINITIONS
 *************************/

/* Job service PVD destroy registration.
*/
fbe_job_service_operation_t fbe_destroy_provision_drive_job_service_operation = 
{
    FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE,
    {
        /* validation function */
        fbe_provision_drive_destroy_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_provision_drive_destroy_update_configuration_in_memory_function,

        /* persist function */
        fbe_provision_drive_destroy_persist_configuration_db_function,

        /* response/rollback function */
        fbe_provision_drive_destroy_rollback_function,

        /* commit function */
        fbe_provision_drive_destroy_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_provision_drive_destroy_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the PVD destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/30/2011 - Created. zhangy
 ****************************************************************/

static fbe_status_t fbe_provision_drive_destroy_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_destroy_provision_drive_t *      provision_drive_destroy_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
	fbe_u32_t                           sys_slot;
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

    provision_drive_destroy_request_p = (fbe_job_service_destroy_provision_drive_t *) job_queue_element_p->command_data;

    /* If object_id is set to UNCONSUMED_PVDS_ID,  then this will destroy first N number of PVDs that are unconsummed and failed. */
    if(provision_drive_destroy_request_p->object_id != FBE_DATABASE_TRANSACTION_DESTROY_UNCONSUMED_PVDS_ID)
    {    
        sys_slot = provision_drive_destroy_request_p->object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;
    	/* we need to make sure no duplicated destroy */
        /*check database service against pvd id */
        if (!fbe_database_is_pvd_exists_by_id(provision_drive_destroy_request_p->object_id))
         {
            /* skip when there is no pvd*/
            provision_drive_destroy_request_p->object_id = FBE_OBJECT_ID_INVALID;
         }else
         {
    		/*if it is a system pvd, we should take care of the raw mirror*/
    		 if(fbe_private_space_layout_object_id_is_system_pvd(provision_drive_destroy_request_p->object_id))
    		 	{
             
    			  /*mask the down disk in raw mirror*/
    				fbe_raw_mirror_mask_down_disk_in_all(sys_slot);
    			  /*set the quiesce_flag in raw mirror*/
    			  fbe_raw_mirror_set_quiesce_flags_in_all();
    			  /*wait the semaphore*/
    			  fbe_raw_mirror_wait_all_io_quiesced();
    			  /*if we are here, there is no more outgoing IO in raw mirror.
    			  unset the quiesce_flag in raw mirror*/ 
    			  fbe_raw_mirror_unset_quiesce_flags_in_all();
    			  /*we can destroy the system pvd*/
    		 	}
             
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
 * end fbe_provision_drive_destroy_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_provision_drive_destroy_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the provision drive destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/30/2011 - Created. zhangy
 ****************************************************************/
static fbe_status_t fbe_provision_drive_destroy_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_destroy_provision_drive_t        *pvd_destroy_request_p = NULL;
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
    pvd_destroy_request_p = (fbe_job_service_destroy_provision_drive_t *) job_queue_element_p->command_data;

    /* open_transaction in configuration service.  should the type of transaction be given here? */
    status = fbe_create_lib_start_database_transaction(&pvd_destroy_request_p->transaction_id, job_queue_element_p->job_number);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pvd_destroy_request_p->object_id != FBE_OBJECT_ID_INVALID)
    {        
        status = fbe_destroy_pvd_configuration_service_destroy_pvd(pvd_destroy_request_p->transaction_id,
                                                                       pvd_destroy_request_p->object_id);
    
        if(status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: transaction %llu, failed to destroy PVD (ID%d).\n", 
                              __FUNCTION__,
                          (unsigned long long)pvd_destroy_request_p->transaction_id,
                          pvd_destroy_request_p->object_id);
                        job_queue_element_p->status = status;
                        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                        fbe_transport_complete_packet(packet_p);
                        return FBE_STATUS_GENERIC_FAILURE;
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
 * end fbe_provision_drive_destroy_update_configuration_in_memory_function()
 **********************************************************/

/*!**************************************************************
 * fbe_provision_drive_destroy_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the provision_drive destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_destroy_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_destroy_provision_drive_t        *pvd_destroy_request_p = NULL;
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
    pvd_destroy_request_p = (fbe_job_service_destroy_provision_drive_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(pvd_destroy_request_p->transaction_id);
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
 * end fbe_provision_drive_destroy_persist_configuration_db_function()
 **********************************************************/

/*!**************************************************************
 * fbe_provision_drive_destroy_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the pvd destroy request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_destroy_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_destroy_provision_drive_t        *pvd_destroy_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_notification_info_t             notification_info;
	fbe_u32_t                           sys_slot;

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
    pvd_destroy_request_p = (fbe_job_service_destroy_provision_drive_t *) job_queue_element_p->command_data;
    sys_slot = pvd_destroy_request_p->object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;

    /* TODO: how will rollback know what to rollback? */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY)
            && (pvd_destroy_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID))
    {
        status = fbe_create_lib_abort_database_transaction(pvd_destroy_request_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
    }
    /*if it is a system pvd, we should take care of the raw mirror*/
	if(fbe_private_space_layout_object_id_is_system_pvd(pvd_destroy_request_p->object_id))
	{
              
		/*unmask the down disk in raw mirror*/
		fbe_raw_mirror_unmask_down_disk_in_all(sys_slot);
	}
    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.job_service_error_info.status = job_queue_element_p->status;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = pvd_destroy_request_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE;
    //notification_info.notification_data.job_action_state = FBE_JOB_ACTION_STATE_ROLLBACK;
    notification_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;

    status = fbe_notification_send(pvd_destroy_request_p->object_id, notification_info);
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
 * end fbe_provision_drive_destroy_rollback_function()
 **********************************************************/

/*!**************************************************************
 * fbe_provision_drive_destroy_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the provision_drive destroy request 
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_destroy_commit_function (fbe_packet_t *packet_p)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_job_service_destroy_provision_drive_t        *pvd_destroy_request_p = NULL;
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

    /* get the user data from job command*/
    pvd_destroy_request_p = (fbe_job_service_destroy_provision_drive_t *) job_queue_element_p->command_data;

    if (pvd_destroy_request_p->object_id != FBE_OBJECT_ID_INVALID)
    {
            /* Send a notification for this commit*/
            notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
            notification_info.notification_data.job_service_error_info.object_id = pvd_destroy_request_p->object_id;
            notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
            notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
            notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
            notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE;
            //notification_info.notification_data.job_action_state = FBE_JOB_ACTION_STATE_COMMIT;
            notification_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
            notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
            status = fbe_notification_send(pvd_destroy_request_p->object_id, notification_info);
            if (status != FBE_STATUS_OK) {
                job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: job status notification failed, status: 0x%X\n",
                                __FUNCTION__, 
                                status);
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
 * end fbe_provision_drive_destroy_commit_function()
 **********************************************************/


/*
 * The following functions are utility functions used by the PVD destroy Job Service code
 */

/*!**************************************************************
 * fbe_destroy_pvd_configuration_service_destroy_pvd()
 ****************************************************************
 * @brief
 * This function asks configuration service to destroy a pvd object.  
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_destroy_pvd_configuration_service_destroy_pvd(fbe_database_transaction_id_t transaction_id, 
                                                                      fbe_object_id_t pvd_object_id)                                                                    
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_destroy_object_t destroy_pvd;
    fbe_base_config_upstream_object_list_t  upstream_list;
    fbe_database_control_destroy_edge_t     edge_destroyed;
    fbe_u32_t                               index =0;

    if(pvd_object_id == FBE_DATABASE_TRANSACTION_DESTROY_UNCONSUMED_PVDS_ID)
    {
        /* Force garbage collection on some number of PVDs.    No need to detach upstream edges
           since the following control operation will only destroy PVDs that are unconsummed and in failed state.
           When an object is in this state there are no upstream or downstream edges. */

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Garbage Collection for Unconsumed/Failed PVDs\n", __FUNCTION__);

        fbe_zero_memory(&destroy_pvd, sizeof(fbe_database_control_destroy_object_t));
        destroy_pvd.transaction_id = transaction_id;
        destroy_pvd.object_id = pvd_object_id; 
        status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_PVD,
                                                    &destroy_pvd,
                                                    sizeof(fbe_database_control_destroy_object_t),
                                                    FBE_PACKAGE_ID_SEP_0,                                             
                                                    FBE_SERVICE_ID_DATABASE,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_OBJECT_ID_INVALID,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: failed.  Error 0x%x\n", 
                              __FUNCTION__, 
                              status);
            return status;
        } 
    }
    else  
    {
        /*first judge whether it has upstream object*/
        fbe_zero_memory(&upstream_list, sizeof(upstream_list));
        status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                                                                    &upstream_list,
                                                                                                    sizeof(upstream_list),
                                                                                                    FBE_PACKAGE_ID_SEP_0,                                             
                                                                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                                                                    FBE_CLASS_ID_INVALID,
                                                                                                    pvd_object_id,
                                                                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: cannot get upstream objects of pvd 0x%x\n", 
                              __FUNCTION__, 
                              pvd_object_id);
            return status;
        }
    
        /*if attached to upstream object (VD), the edge should be dettached first!*/
        if(upstream_list.number_of_upstream_objects > 0)
        {
            for(index = 0; index <upstream_list.number_of_upstream_objects; index++)
            {
                /* Right now the get upstream object list api assumes the client index is the the same for upstream objects */
                edge_destroyed.block_transport_destroy_edge.client_index = upstream_list.current_upstream_index;
                edge_destroyed.object_id = upstream_list.upstream_object_list[index];
                edge_destroyed.transaction_id = transaction_id;
                status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_EDGE,
                                                            &edge_destroyed,
                                                            sizeof(edge_destroyed),
                                                            FBE_PACKAGE_ID_SEP_0,                                             
                                                            FBE_SERVICE_ID_DATABASE,
                                                            FBE_CLASS_ID_INVALID,
                                                            FBE_OBJECT_ID_INVALID,
                                                            FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK)
                    {
                        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                          "%s: fail to detach edge for pvd 0x%x\n", 
                                          __FUNCTION__, 
                                          pvd_object_id);
                        return status;
                    }    
    
            }
            
        }
    
        fbe_zero_memory(&destroy_pvd, sizeof(fbe_database_control_destroy_object_t));
        destroy_pvd.transaction_id = transaction_id;
        destroy_pvd.object_id = pvd_object_id; 
        status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_DESTROY_PVD,
                                                    &destroy_pvd,
                                                    sizeof(fbe_database_control_destroy_object_t),
                                                    FBE_PACKAGE_ID_SEP_0,                                             
                                                    FBE_SERVICE_ID_DATABASE,
                                                    FBE_CLASS_ID_INVALID,
                                                    FBE_OBJECT_ID_INVALID,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: failed.  Error 0x%x\n", 
                              __FUNCTION__, 
                              status);
            return status;
        } 
    }
    return status;
}
/**********************************************************
 * end fbe_destroy_pvd_configuration_service_destroy_pvd()
 **********************************************************/
