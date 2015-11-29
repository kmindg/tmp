/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_create.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for lun create used by job service.
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
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_notification.h"
#include "fbe_database.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes
#include "fbe_private_space_layout.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/
static fbe_status_t fbe_create_lun_get_imported_capacity (fbe_lba_t exported_capacity,
                                                          fbe_lba_t lun_align_size,
                                                          fbe_lba_t *imported_capacity);

static fbe_status_t fbe_create_lun_configuration_service_create_lun(fbe_database_transaction_id_t transaction_id, 
                                                                    fbe_lba_t exported_capacity,
                                                                    fbe_lun_number_t lun_number,
                                                                    fbe_bool_t ndb_b,
                                                                    fbe_bool_t noinitialverify_b,
                                                                    fbe_assigned_wwid_t *world_wide_name,
                                                                    fbe_user_defined_lun_name_t *user_defined_name,
                                                                    fbe_object_id_t *lun_object_id,
                                                                    fbe_time_t bind_time,
                                                                    fbe_bool_t export_lun_b,
                                                                    fbe_bool_t user_private);

static fbe_status_t fbe_create_lun_topology_get_bvd_interface(fbe_object_id_t *object_id_p);
static fbe_status_t fbe_create_lun_get_raid_group_info(fbe_object_id_t raid_group_object_id, 
                                                       fbe_raid_group_get_info_t * raid_group_info_p);

/* Lun create library request validation function. 
 */
static fbe_status_t fbe_lun_create_validation_function (fbe_packet_t *packet_p);
/* Lun create library update configuration in memory function.
 */
static fbe_status_t fbe_lun_create_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Lun create library persist configuration in db function.
 */
static fbe_status_t fbe_lun_create_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Lun create library rollback function.
 */
static fbe_status_t fbe_lun_create_rollback_function (fbe_packet_t *packet_p);

/* Lun create library commit function.
 */
static fbe_status_t fbe_lun_create_commit_function (fbe_packet_t *packet_p);

/*validate system limits*/
static fbe_status_t fbe_create_lun_validate_system_limits(fbe_job_service_lun_create_t *lun_create_request_p);

fbe_status_t
fbe_create_lun_configuration_service_create_system_lun_and_edges(fbe_job_service_lun_create_t *lun_create_req);

static fbe_status_t fbe_lun_create_print_request_elements(fbe_job_service_lun_create_t *req_p);

/*************************
 *   DEFINITIONS
 *************************/

/* Job service lun create registration.
*/
fbe_job_service_operation_t fbe_lun_create_job_service_operation = 
{
    FBE_JOB_TYPE_LUN_CREATE,
    {
        /* validation function */
        fbe_lun_create_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_lun_create_update_configuration_in_memory_function,

        /* persist function */
        fbe_lun_create_persist_configuration_db_function,

        /* response/rollback function */
        fbe_lun_create_rollback_function,

        /* commit function */
        fbe_lun_create_commit_function,
    }
};


/*************************
 *   FUNCTIONS
 *************************/

/*!**************************************************************
 * fbe_lun_create_validation_function()
 ****************************************************************
 * @brief
 * This function is used to validate the Lun create request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/

static fbe_status_t fbe_lun_create_validation_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_lun_create_t *      lun_create_request_p = NULL;
    fbe_payload_ex_t                    *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_raid_group_get_info_t           raid_group_get_info;
    fbe_lifecycle_state_t               lifecycle_state;
    fbe_lba_t                           available_capacity;
	fbe_block_count_t 					cache_zero_bit_map_size;
    fbe_bool_t                          double_degraded;

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "lun_create_valid_fun: job_queue_element is NULL.\n");
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
                          "lun_create_valid_fun: fbe_job_queue_element wrong size\n");

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_create_request_p = (fbe_job_service_lun_create_t *) job_queue_element_p->command_data;

	job_queue_element_p->status = FBE_STATUS_OK;
	job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_lun_create_print_request_elements(lun_create_request_p);

    /*If we create a system LUN, we assign the lun number and object id */
    if (!lun_create_request_p->is_system_lun && lun_create_request_p->lun_number != FBE_LUN_ID_INVALID) {
		status = fbe_create_lib_lookup_lun_object_id(lun_create_request_p->lun_number, &lun_create_request_p->lun_object_id);
		if((status == FBE_STATUS_OK)&&(lun_create_request_p->lun_object_id != FBE_OBJECT_ID_INVALID))
		{
			job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
			job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE;
			fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
			fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
			fbe_transport_complete_packet(packet_p);
			return FBE_STATUS_GENERIC_FAILURE;
		}
	
	}
    /* Get the RG object id from config service */
    status = fbe_create_lib_database_service_lookup_raid_object_id(lun_create_request_p->raid_group_id, 
                                                                   &lun_create_request_p->rg_object_id);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ID;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*validate we can create the lun as far as the system limits go*/
    status = fbe_create_lun_validate_system_limits(lun_create_request_p);
	if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SYSTEM_LIMITS_EXCEEDED;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_get_object_state(lun_create_request_p->rg_object_id, &lifecycle_state);
    if((status != FBE_STATUS_OK)||((lifecycle_state != FBE_LIFECYCLE_STATE_READY) && (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)))
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*If  recreate a new system LUN, we don't need more validation */
    if (lun_create_request_p->is_system_lun) 
    {
        /* Set the payload status */
        fbe_payload_control_set_status(control_operation_p, payload_status);

        /* Complete the packet so that the job service thread can continue */
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_OK;
    }

    /*Check the number of degraded db drives, if more than one drive degraded, we can't create lun*/
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

    /* RG info contains the stripe size and align size that will be used for calculate capacities */
    status = fbe_create_lun_get_raid_group_info(lun_create_request_p->rg_object_id, &raid_group_get_info);
    if(status != FBE_STATUS_OK)
    {		
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate the imported capacity by lun_class.  This is the capacity LUN asks from RG */
    status = fbe_create_lun_get_imported_capacity (lun_create_request_p->capacity,
                                                   raid_group_get_info.lun_align_size,
                                                   &lun_create_request_p->lun_imported_capacity);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*before we start to use the capacity requested by the user, we have to add to it a bit extra capacity
	This is done so cache can store at the end of the LUN the zero bitmaps. fbe_create_lun_get_imported_capacity()
    already applied this same math to give us the size we need to import.
    */
	fbe_lun_calculate_cache_zero_bit_map_size(lun_create_request_p->capacity, &cache_zero_bit_map_size);
	lun_create_request_p->capacity += cache_zero_bit_map_size;
    
    /* Is the space available in RG? get the starting offset and client index in rg */
    if(lun_create_request_p->ndb_b)
    {
        lun_create_request_p->placement = FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION;
        lun_create_request_p->rg_block_offset = lun_create_request_p->addroffset;
    }
    status = fbe_create_lib_base_config_validate_capacity (lun_create_request_p->rg_object_id,
                                              lun_create_request_p->lun_imported_capacity,
                                              lun_create_request_p->placement,
                                              FBE_FALSE, /* Don't ignore offset*/
                                              &lun_create_request_p->rg_block_offset, 
                                              &lun_create_request_p->rg_client_index,
                                              &available_capacity);
    if(status != FBE_STATUS_OK)
    {
        if (lun_create_request_p->ndb_b)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "lun_create_valid_fun:NDB param address offset: 0x%llx cannot be allocated\n",
                              (unsigned long long)lun_create_request_p->addroffset);
    
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else {
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        if ((lun_create_request_p->ndb_b) &&
            (available_capacity != lun_create_request_p->lun_imported_capacity))
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "lun_create_valid_fun:NDB param capacity:0x%llx cannot allocate at offset 0x%llx\n",
                              (unsigned long long)lun_create_request_p->lun_imported_capacity,
			      (unsigned long long)lun_create_request_p->addroffset);
    
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET;
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
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_lun_create_validation_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_create_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the Lun create request 
 *
 * @param packet_p - Incoming packet pointer (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_lun_create_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_lun_create_t        *lun_create_request_p = NULL;
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
    lun_create_request_p = (fbe_job_service_lun_create_t *) job_queue_element_p->command_data;
    lun_create_request_p->transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* open_transaction in configuration service.  should the type of transaction be given here? */
    status = fbe_create_lib_start_database_transaction(&lun_create_request_p->transaction_id, job_queue_element_p->job_number);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*If create system LUN, we generate the config form PSL*/
    if (lun_create_request_p->is_system_lun)
    {
        status = fbe_create_lun_configuration_service_create_system_lun_and_edges(lun_create_request_p);
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
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);

        return status;

    }
    
    /*! @todo: call configuration_service to persist on peer */
    /* call configuration_service to create_lun */
    status = fbe_create_lun_configuration_service_create_lun(lun_create_request_p->transaction_id, 
                                                             lun_create_request_p->capacity,
                                                             lun_create_request_p->lun_number,
                                                             lun_create_request_p->ndb_b,
                                                             lun_create_request_p->noinitialverify_b,
                                                             &lun_create_request_p->world_wide_name,
                                                             &lun_create_request_p->user_defined_name,
                                                             &lun_create_request_p->lun_object_id,
                                                             lun_create_request_p->bind_time,
                                                             lun_create_request_p->export_lun_b,
                                                             lun_create_request_p->user_private);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* ask database service to create edge between RG and LUN */
    status = fbe_create_lib_database_service_create_edge(lun_create_request_p->transaction_id, 
                                                         lun_create_request_p->rg_object_id, 
                                                         lun_create_request_p->lun_object_id, 
                                                         0, 
                                                         lun_create_request_p->lun_imported_capacity, 
                                                         lun_create_request_p->rg_block_offset,
                                                         NULL,
                                                         0/* not used */);
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
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_lun_create_update_configuration_in_memory_function()
 **********************************************************/

/*!**************************************************************
 * fbe_lun_create_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Lun create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - lun create request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_lun_create_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_lun_create_t        *lun_create_request_p = NULL;
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
    lun_create_request_p = (fbe_job_service_lun_create_t *) job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(lun_create_request_p->transaction_id);
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
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/**********************************************************
 * end fbe_lun_create_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_create_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Lun create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - lun create request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_lun_create_rollback_function (fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_job_service_lun_create_t        *lun_create_request_p = NULL;
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
    lun_create_request_p = (fbe_job_service_lun_create_t *) job_queue_element_p->command_data;

    /* see if transaction should be aborted */
    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state  == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY) &&
            (lun_create_request_p->transaction_id != FBE_DATABASE_TRANSACTION_ID_INVALID))
    {
        status = fbe_create_lib_abort_database_transaction(lun_create_request_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
    }

    /* Send a notification for this rollback*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = FBE_OBJECT_ID_INVALID;
    job_queue_element_p->object_id = FBE_OBJECT_ID_INVALID;
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
 * end fbe_lun_create_rollback_function()
 **********************************************************/


/*!**************************************************************
 * fbe_lun_create_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the Lun create request 
 *
 * @param packet_p - Incoming packet pointer.
 * @param control_code - Job control code.
 * @param job_command_p - lun create request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. guov
 *  11/15/2012 - Modified by Vera Wang
 ****************************************************************/
static fbe_status_t fbe_lun_create_commit_function (fbe_packet_t *packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_job_service_lun_create_t        *lun_create_request_p = NULL;
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
    lun_create_request_p = (fbe_job_service_lun_create_t *) job_queue_element_p->command_data;

	job_service_trace(FBE_TRACE_LEVEL_INFO, 
		FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
		"%s entry, job number 0x%llx, status 0x%x, error_code %d\n",
		__FUNCTION__,
		(unsigned long long)job_queue_element_p->job_number, 
		job_queue_element_p->status,
		job_queue_element_p->error_code);

	job_queue_element_p->status = FBE_STATUS_OK;
	//job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = lun_create_request_p->lun_object_id;
    job_queue_element_p->need_to_wait = lun_create_request_p->wait_ready;
    job_queue_element_p->timeout_msec = lun_create_request_p->ready_timeout_msec;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = lun_create_request_p->lun_object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = job_queue_element_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;

    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(lun_create_request_p->wait_ready) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(lun_create_request_p->lun_object_id, 
                                                                  FBE_LIFECYCLE_STATE_READY, 
                                                                  lun_create_request_p->ready_timeout_msec);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting lun object id %d be ready. status %d\n", 
                              __FUNCTION__, lun_create_request_p->lun_object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    status = fbe_notification_send(lun_create_request_p->lun_object_id, notification_info);
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
 * end fbe_lun_create_commit_function()
 **********************************************************/


/*
 * The following functions are utility functions used by the LUN create Job Service code
 */


/*!**************************************************************
 * fbe_create_lun_get_imported_capacity()
 ****************************************************************
 * @brief
 * This function asks the lun class for the impoerted capacity 
 * base on a given exported capacity.  
 *
 * @param exported_capacity - the exported capacity in blocks. (IN)
 * @param imported_capacity - calculated imported capacity in blocks. (OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_create_lun_get_imported_capacity (fbe_lba_t exported_capacity,
                                                          fbe_lba_t lun_align_size,
                                                          fbe_lba_t *imported_capacity)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_lun_control_class_calculate_capacity_t calculate_imported_capacity;

    calculate_imported_capacity.exported_capacity = exported_capacity;
    calculate_imported_capacity.lun_align_size = lun_align_size;

    status = fbe_create_lib_send_control_packet (FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY,
                                                 &calculate_imported_capacity,
                                                 sizeof(fbe_lun_control_class_calculate_capacity_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_LUN,
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

    *imported_capacity = calculate_imported_capacity.imported_capacity;

    return status;
}
/**********************************************************
 * end fbe_create_lun_get_imported_capacity()
 **********************************************************/


/*!**************************************************************
 * fbe_create_lun_configuration_service_create_lun()
 ****************************************************************
 * @brief
 * This function asks configuration service to create a lun object.  
 *
 * @param transaction_id - the exported capacity in blocks. (IN)
 * @param exported_capacity - calculated imported capacity in blocks. (IN)
 * @param lun_number - the user assigned lun number. (IN)
 * @param lun_object_id - Object id. (OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_create_lun_configuration_service_create_lun(fbe_database_transaction_id_t transaction_id, 
                                                                    fbe_lba_t exported_capacity,
                                                                    fbe_lun_number_t lun_number,
                                                                    fbe_bool_t ndb_b,
                                                                    fbe_bool_t noinitialverify_b,
                                                                    fbe_assigned_wwid_t *world_wide_name,
                                                                    fbe_user_defined_lun_name_t *user_defined_name,
                                                                    fbe_object_id_t *lun_object_id,
                                                                    fbe_time_t bind_time,
                                                                    fbe_bool_t export_lun_b,
                                                                    fbe_bool_t user_private)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_lun_t create_lun;

    fbe_zero_memory(&create_lun, sizeof(fbe_database_control_lun_t));

    create_lun.transaction_id = transaction_id;
    create_lun.object_id = FBE_OBJECT_ID_INVALID; /* topology will assign an object id, if the invalid is passed in*/
    create_lun.lun_set_configuration.capacity = exported_capacity;
    if (ndb_b == FBE_TRUE){
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_NO_USER_ZERO;

        /* if NDB flag is set, we will not perform initial verify on this lun */
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_NO_INITIAL_VERIFY;        
    }
    if(noinitialverify_b == FBE_TRUE){
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_NO_INITIAL_VERIFY;
    }
    if(export_lun_b == FBE_TRUE){
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_EXPROT_LUN;
    }
    create_lun.lun_number = lun_number;
    create_lun.bind_time = bind_time;
    create_lun.user_private = user_private;
    fbe_copy_memory(&create_lun.world_wide_name, world_wide_name, sizeof(create_lun.world_wide_name));
    fbe_copy_memory(&create_lun.user_defined_name, user_defined_name, sizeof(create_lun.user_defined_name));

    /*sharel:currently, power saving related information is set to default and will be updated by the RG,
      In the future, we will use the power saving policy FBE API to set these*/
    create_lun.lun_set_configuration.power_saving_enabled = FBE_FALSE;
    create_lun.lun_set_configuration.power_save_io_drain_delay_in_sec = FBE_LUN_POWER_SAVE_DELAY_DEFAULT;
    create_lun.lun_set_configuration.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    create_lun.lun_set_configuration.max_lun_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_LUN,
                                                 &create_lun,
												 sizeof(fbe_database_control_lun_t),
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

    *lun_object_id = create_lun.object_id;
    //*lun_number = create_lun.lun_number;
    
    return status;
}
/**********************************************************
 * end fbe_create_lun_configuration_service_create_lun()
 **********************************************************/


/*!**************************************************************
 * fbe_create_lun_topology_get_bvd_interface()
 ****************************************************************
 * @brief
 * This function gets the object id of the bvd interface.  
 *
 * @param lun_object_id - Object id. (OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_create_lun_topology_get_bvd_interface(fbe_object_id_t *object_id_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_topology_control_get_bvd_id_t			get_bvd_id;
  

    status = fbe_create_lib_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_BVD_OBJECT_ID,
												&get_bvd_id,
                                                sizeof(fbe_topology_control_get_bvd_id_t),
                                                FBE_PACKAGE_ID_SEP_0,                                                
                                                FBE_SERVICE_ID_TOPOLOGY,
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

    /* There is only one BVD interface in SEP */
    *object_id_p = get_bvd_id.bvd_object_id;

    return status;
}
/**********************************************************
 * end fbe_create_lun_topology_get_bvd_interface()
 **********************************************************/


/*!**************************************************************
 * fbe_create_lun_get_raid_group_info
 ****************************************************************
 * @brief
 * This function gets the rg info.  
 *
 * @param raid_group_object_id - raid group object id. (IN)
 * @param raid_group_info_p - pointer to the info buffer(OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/27/2010 - Created. guov
 ****************************************************************/
static fbe_status_t fbe_create_lun_get_raid_group_info(fbe_object_id_t raid_group_object_id, 
                                                       fbe_raid_group_get_info_t * raid_group_info_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                 raid_group_info_p,
                                                 sizeof(fbe_raid_group_get_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 raid_group_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: failed.  Error 0x%x\n", 
                          __FUNCTION__, 
                          status);
    }

    return status;
}
/**********************************************************
 * end fbe_create_lun_get_raid_group_info()
 **********************************************************/

/*!**************************************************************
 * fbe_create_lun_validate_system_limits
 ****************************************************************
 * @brief
 * make sure we are allowed to create this lun on this platform.  
 *
 * @param lun_create_request_p - pointer to creation (IN)
 *
 * @return status - The status of the operation.
 ****************************************************************/
static fbe_status_t fbe_create_lun_validate_system_limits(fbe_job_service_lun_create_t *lun_create_request_p)
{

	fbe_database_control_get_stats_t				get_db_stats;
	fbe_base_config_upstream_object_list_t			upstream_objects_list;
	fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;

	/*let's see how many user luns we have so far*/
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_GET_STATS,
                                                 &get_db_stats,
                                                 sizeof(fbe_database_control_get_stats_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: failed.  Error 0x%x\n", 
                          __FUNCTION__, 
                          status);

		return status;
    }

	if (get_db_stats.num_user_luns == get_db_stats.max_allowed_user_luns) {
		 job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: reached system limits of %d luns\n", 
                          __FUNCTION__, get_db_stats.max_allowed_user_luns);
                          
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*we are still not safe yet, let's see how may luns we already have on the requested RG*/
	status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                 &upstream_objects_list,
                                                 sizeof(fbe_base_config_upstream_object_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 lun_create_request_p->rg_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: get upstream object list failed.  Error 0x%x\n", 
                          __FUNCTION__, 
                          status);

		return status;
    }

    if (get_db_stats.num_user_luns == get_db_stats.max_allowed_user_luns) {
		 job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: failed to get the number of luns for rg id:%d\n", 
                          __FUNCTION__, lun_create_request_p->raid_group_id);
                          
		return status;
	}

	if (upstream_objects_list.number_of_upstream_objects == get_db_stats.max_allowed_luns_per_rg) {
		job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: reached lun per rg limit of %d\n", 
                          __FUNCTION__, get_db_stats.max_allowed_luns_per_rg);
                          
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*if we got here we are good to go*/
	return FBE_STATUS_OK;
}

fbe_status_t
fbe_create_lun_configuration_service_create_system_lun_and_edges(fbe_job_service_lun_create_t *lun_create_req)
{
    fbe_database_control_lun_t          create_lun;
    fbe_private_space_layout_region_t   region;
    fbe_private_space_layout_lun_info_t lun_info;
    fbe_status_t                        status;

    if (lun_create_req == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Input argument is NULL\n", 
                          __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_private_space_layout_get_lun_by_lun_number(lun_create_req->lun_number, &lun_info);
    if (status != FBE_STATUS_OK) {
		if(status == FBE_STATUS_NOT_INITIALIZED){
			job_service_trace(FBE_TRACE_LEVEL_WARNING,
							  FBE_TRACE_MESSAGE_ID_INFO,
							  "%s: PSL library not initialized\n", 
							  __FUNCTION__);

		} else {
			job_service_trace(FBE_TRACE_LEVEL_ERROR,
							  FBE_TRACE_MESSAGE_ID_INFO,
							  "%s: fail to get lun info from PSL\n", 
							  __FUNCTION__);
		}
        return status;
    }

    status = fbe_private_space_layout_get_region_by_raid_group_id(lun_info.raid_group_id, &region);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail to get region info from PSL\n", 
                          __FUNCTION__);
        return status;
    }
    
    create_lun.transaction_id = lun_create_req->transaction_id;
    create_lun.object_id = lun_create_req->lun_object_id; /*It is a system LUN, we passe in the object id*/
    if (lun_create_req->ndb_b == FBE_TRUE){
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_NO_USER_ZERO;

        /* if NDB flag is set, we will not perform initial verify on this lun */
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_NO_INITIAL_VERIFY;        
    }
    if(lun_create_req->export_lun_b == FBE_TRUE){
        create_lun.lun_set_configuration.config_flags |= FBE_LUN_CONFIG_EXPROT_LUN;
    }
    /*sharel:currently, power saving related information is set to default and will be updated by the RG,
      In the future, we will use the power saving policy FBE API to set these*/
    create_lun.lun_set_configuration.capacity = lun_info.external_capacity;
    create_lun.lun_set_configuration.power_saving_enabled = FBE_FALSE;
    create_lun.lun_set_configuration.power_save_io_drain_delay_in_sec = FBE_LUN_POWER_SAVE_DELAY_DEFAULT;
    create_lun.lun_set_configuration.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    create_lun.lun_set_configuration.max_lun_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    create_lun.lun_set_configuration.config_flags = FBE_LUN_CONFIG_NONE;
        
    create_lun.lun_number = lun_create_req->lun_number;
    create_lun.bind_time = lun_create_req->bind_time;
    create_lun.user_private = lun_create_req->user_private;

    /* copy the wwn */
    fbe_copy_memory(&create_lun.world_wide_name, &lun_create_req->world_wide_name, sizeof(create_lun.world_wide_name));
    fbe_copy_memory(&create_lun.user_defined_name, &lun_create_req->user_defined_name, sizeof(create_lun.user_defined_name));
    //fbe_copy_memory(&create_lun.user_defined_name, lun_info.exported_device_name, sizeof(create_lun.user_defined_name));

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_LUN,
                                                 &create_lun,
												 sizeof(fbe_database_control_lun_t),
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

    /* ask database service to create edge between RG and LUN */
    status = fbe_create_lib_database_service_create_edge(lun_create_req->transaction_id, 
                                                         region.raid_info.object_id, 
                                                         lun_info.object_id, 
                                                         0, 
                                                         lun_info.internal_capacity, 
                                                         lun_info.raid_group_address_offset,
                                                         NULL,
                                                         0/* not used */);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: fail to create edge. status = %d\n", 
                          __FUNCTION__, 
                          status);
        return status;
    }
    
    
    return status;

}


static fbe_status_t fbe_lun_create_print_request_elements(fbe_job_service_lun_create_t *req_p)
{
    fbe_u32_t   index = 0;
    fbe_u8_t wwn_bytes[3*FBE_WWN_BYTES + 16];
    fbe_u8_t *wwn_bytes_ptr = &wwn_bytes[0];

    if(req_p == NULL)
        return FBE_STATUS_GENERIC_FAILURE;

    fbe_zero_memory(wwn_bytes_ptr, sizeof(wwn_bytes));
    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "Job Control Service, execution queue:::Received LUN create request\n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    
    for(index = 0; index < FBE_WWN_BYTES; index++)
    {
        fbe_sprintf(wwn_bytes_ptr, 4, "%02x", (fbe_u32_t)(req_p->world_wide_name.bytes[index]));
        wwn_bytes_ptr++;
        wwn_bytes_ptr++;
        
        if(index+1 != FBE_WWN_BYTES)
        {
            *wwn_bytes_ptr = ':';
            wwn_bytes_ptr++;
        }
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    world_wide_name: %s\n", &wwn_bytes[0]);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    user_defined_name: %s\n", &req_p->user_defined_name.name[0]);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    lun_number: 0x%x, raid id: 0x%x\n", req_p->lun_number, req_p->raid_group_id);

    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    capacity 0x%x, ndb_b %d, user_private %d, bind_time 0x%llx\n",
            (unsigned int)req_p->capacity,
            req_p->ndb_b,
            req_p->user_private,
            (unsigned long long)req_p->bind_time);

    return FBE_STATUS_OK;
}
