/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_reinitialize_provision_drive.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines to reinitialize the provision drive configuration 
 *  using job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   05/08/2012:  Created. Ashwin Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_transport.h"
#include "fbe/fbe_platform.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_base_config.h"
#include "fbe_spare.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_database.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"                      //  for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe_private_space_layout.h"
#include "fbe_cmi.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_reinitialize_provision_drive_validation_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_reinitialize_provision_drive_update_configuration_in_memory_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_reinitialize_provision_drive_rollback_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_reinitialize_provision_drive_persist_function(fbe_packet_t *packet_p);
static fbe_status_t fbe_reinitialize_provision_drive_commit_function(fbe_packet_t *packet_p);

static fbe_status_t fbe_update_provision_drive_send_notification(fbe_job_service_update_provision_drive_t * update_pvd_p,
                                                                 fbe_u64_t job_number,
                                                                 fbe_job_service_error_type_t job_service_error_type,
                                                                 fbe_job_action_state_t job_action_state);

static fbe_status_t fbe_pvd_check_with_upstream_objects(fbe_object_id_t *upstream_object_list, fbe_u32_t object_count,
                                                        fbe_bool_t  *can_proceed);

static fbe_status_t fbe_reinit_pvd_mark_NR_for_upstream_objects(fbe_object_id_t   *upstream_object_list, 
                                                                fbe_object_id_t   *upstream_vd_list, 
                                                               fbe_u32_t         object_count,
                                                               fbe_object_id_t   pvd_object_id,
                                                               fbe_bool_t        *is_NR_successful);

fbe_status_t fbe_reinit_pvd_update_provision_drive(fbe_database_control_update_pvd_t  *update_provision_drive);

static fbe_status_t fbe_reinit_pvd_get_drive_info(fbe_object_id_t pvd_object_id,
                                                  fbe_spare_drive_info_t *in_spare_drive_info_p);

static fbe_status_t fbe_reinit_pvd_get_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor);
static fbe_status_t fbe_reinit_pvd_set_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor);
static fbe_status_t fbe_reinit_pvd_set_fru_signature(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot);
static fbe_status_t fbe_reinit_wait_for_pvd_downstream_connection(fbe_object_id_t object_id, fbe_u32_t timeout_in_secs);
static void fbe_reinit_pvd_print_drive_info(fbe_spare_selection_info_t* drive_info);
static fbe_status_t fbe_reinit_pvd_correct_system_drive_info_if_needed(fbe_spare_selection_info_t* drive_info, fbe_bool_t *is_corrected);
static fbe_status_t fbe_reinit_pvd_generate_spare_info_from_physical_drive(fbe_job_queue_element_t* job_queue_element_p,
                                                                fbe_u32_t  port_number,
                                                                fbe_u32_t  enclosure_number,
                                                                fbe_u32_t  slot_number,
                                                                fbe_spare_selection_info_t* spare_drive_info);
static fbe_status_t fbe_reinit_pvd_apply_drive_replacement_algorithm(fbe_job_queue_element_t* job_queue_element_p, fbe_bool_t* can_be_reinit);
static fbe_status_t fbe_reinit_pvd_light_on_amber_led(fbe_u32_t  port_number,
                                                      fbe_u32_t  enclosure_number,
                                                      fbe_u32_t  slot_number);
static fbe_status_t fbe_configuration_service_destroy_pvd(fbe_database_transaction_id_t transaction_id, 
                                                                      fbe_object_id_t pvd_object_id);    
static fbe_status_t fbe_reinit_get_downstream_position(fbe_object_id_t rg_id, fbe_object_id_t server_id, fbe_u32_t *position);
static fbe_status_t fbe_reinit_send_kms_notification(fbe_object_id_t rg_id, fbe_u8_t position,
                                                     fbe_base_config_encryption_state_t encryption_state,
                                                     fbe_spare_swap_command_t swap_command_type);
static fbe_status_t fbe_reinit_pvd_send_notification_to_kms(fbe_object_id_t pvd_object_id, fbe_base_config_encryption_state_t state);
static fbe_status_t fbe_reinit_pvd_wait_pvd_ready_for_keys(fbe_object_id_t object_id, fbe_u32_t timeout_ms);

/*************************
 *   DEFINITIONS
 *************************/

fbe_job_service_operation_t fbe_reinitialize_pvd_job_service_operation = 
{
    FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE,
    {
        /* validation function */
        fbe_reinitialize_provision_drive_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_reinitialize_provision_drive_update_configuration_in_memory_function,

        /* persist function */
        fbe_reinitialize_provision_drive_persist_function,

        /* response/rollback function */
        fbe_reinitialize_provision_drive_rollback_function,

        /* commit function */
        fbe_reinitialize_provision_drive_commit_function,
    }
};

/*************************
 *   FUNCTIONS
 *************************/

/*!****************************************************************************
 * fbe_reinitialize_provision_drive_validation_function()
 ******************************************************************************
 * @brief
 *  This function is used to validate the reinitialize provision drive configuration 
 *  job service command. It checks the size of the new drive and checks with
 *  the upstream objects to make sure whether we can proceed with the reinit or not
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  05/08/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinitialize_provision_drive_validation_function(fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_object_id_t                             pvd_object_id;
    fbe_u32_t                                   number_of_upstream_objects = 0;
    fbe_object_id_t                             upstream_rg_object_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_object_id_t                             upstream_vd_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_bool_t                                  can_proceed = FBE_FALSE;
    
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

    /* 1) check whether the new drive fit the drive 'spare' rule
       2) Check with upstream objects to see whether we can proceed or not
    */

    pvd_object_id = update_pvd_p->object_id;

    /*apply the drive spare algorithm to determine whether the new system drive fits
     *the sparing rules*/
    status = fbe_reinit_pvd_apply_drive_replacement_algorithm(job_queue_element_p, &can_proceed);
    if(FBE_STATUS_OK != status)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if(FBE_FALSE == can_proceed)
    {
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL;        
        
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "PVD reinit validation: new disk NOT fit sparing rules\n");

        /* send an event for this error drive */
        fbe_event_log_write(SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE, NULL, 0, 
                    "%d %d %d",
                    0,
                    0,
                    pvd_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST);
        
        /* light on the amber LED */
        fbe_reinit_pvd_light_on_amber_led(0, 0, pvd_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST);
                           
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    can_proceed = FBE_FALSE;

    /* Get the upstream object list. Check with the upstream objects whether the
       reinitialization can proceed or not
     */
    fbe_create_lib_get_pvd_upstream_edge_list(pvd_object_id, &number_of_upstream_objects,
                                                  upstream_rg_object_list, upstream_vd_list);
    if(0 == number_of_upstream_objects)
    {
        can_proceed = FBE_TRUE;
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                            "reinit_pvd_validation_func, No upper stream RG object on this PVD:0x%x. It is OK to reinitialize it.\n", pvd_object_id);
    }
    else 
    {
        fbe_pvd_check_with_upstream_objects(upstream_rg_object_list, number_of_upstream_objects, &can_proceed);
        if(can_proceed == FBE_FALSE)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "PVD reinit validation: cannot proceed. upstream said NO.\n");

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
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
 * end fbe_reinitialize_provision_drive_validation_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_reinitialize_provision_drive_update_configuration_in_memory_function()
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive config type in memory.
 *
 * @param packet_p          - Job service packet.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  05/08/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinitialize_provision_drive_update_configuration_in_memory_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_object_id_t                             pvd_object_id;
    fbe_u32_t                                   number_of_upstream_objects = 0;
    fbe_object_id_t                             upstream_rg_object_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_object_id_t                             upstream_vd_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_bool_t                                  mark_nr_successful = FBE_FALSE;
    fbe_database_control_update_pvd_t           update_provision_drive;
    fbe_bool_t                                  found_a_user_pvd_has_same_SN = FBE_FALSE;
    fbe_object_id_t      user_pvd_object_id;

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

    /* The SN in PVD record which physical/logic drive relates to this PVD.
      * Before we start to reinit this system PVD, we should check whether 
      * there is another PVD has the same SN as the system PVD which to be
      * reinitialized. If there is, that means, the drive was original user drive,
      * we should destroy its original user PVD firstly, then reinitialize the system
      * PVD.
      */
    
    /* 0) Check wheter there is user PVD has the same SN. 
      * 1) Send a usurper to all the upstream ojects to mark NR
      * 2) Send a update PVD config to the database service
      * 3) Send an usurper to PVD to clear the NP and the generation number
      * 4) Create an edge between PVD and LDO
      * 5) Modify and persist new FRU descriptors if this drive is a system drive
      * 6) Stamp FRU signature of this drive
      */

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

    /* Check wheter there is user PVD has the same SN. */
    found_a_user_pvd_has_same_SN = fbe_database_is_user_pvd_exists(update_pvd_p->serial_num, &user_pvd_object_id);
    if (!found_a_user_pvd_has_same_SN)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "Do not find a user PVD has the same SN as the system PVD to be reinitialized.\n");
        /* nothing */;
    }
    else /* found a user pvd has the same SN as the system PVD to be reinitialized. */
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "Find a user PVD: 0x%x has the same SN as the system PVD to be reinitialized.\n", user_pvd_object_id);
        /* destroy that user pvd in transaction */
        if (user_pvd_object_id != FBE_OBJECT_ID_INVALID)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "Destroy that original user PVD.\n");
            status = fbe_configuration_service_destroy_pvd(update_pvd_p->transaction_id,
                                                                      user_pvd_object_id);
            if(status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s: transaction %llu, failed to destroy PVD (ID%d).\n", 
                                  __FUNCTION__,
                                  (unsigned long long)update_pvd_p->transaction_id,
                                  user_pvd_object_id);
                
                job_queue_element_p->status = status;
                job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }


    /* Mark NR on all the upstream objects */
    pvd_object_id = update_pvd_p->object_id;
    fbe_create_lib_get_pvd_upstream_edge_list(pvd_object_id, &number_of_upstream_objects,
                                                  upstream_rg_object_list, upstream_vd_list);



    if(0 == number_of_upstream_objects)
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "reinit_pvd_update_config_in_mem, No upper stream RG object on this PVD:0x%x. It is OK to reinitialize it.\n", pvd_object_id);
    else 
    {
        status = fbe_reinit_pvd_mark_NR_for_upstream_objects(upstream_rg_object_list, upstream_vd_list,
                                                    number_of_upstream_objects, pvd_object_id, &mark_nr_successful);
        if(mark_nr_successful == FBE_FALSE)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s failed to mark NR %d\n", __FUNCTION__, status);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* Send an update PVD config to the database service */
    update_provision_drive.object_id = update_pvd_p->object_id;
    update_provision_drive.transaction_id = update_pvd_p->transaction_id;
    update_provision_drive.configured_capacity = update_pvd_p->configured_capacity;
    update_provision_drive.configured_physical_block_size = update_pvd_p->configured_physical_block_size;
    fbe_copy_memory(update_provision_drive.serial_num, update_pvd_p->serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    update_provision_drive.update_type = FBE_UPDATE_PVD_USE_BITMASK;
    update_provision_drive.update_type_bitmask = FBE_UPDATE_PVD_CONFIG;

    status = fbe_reinit_pvd_update_provision_drive(&update_provision_drive);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%sfailed to update the pvd config %d. objid:0x%x\n", __FUNCTION__, status, update_pvd_p->object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_reinit_pvd_send_notification_to_kms(pvd_object_id, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to remove old keys. objid:0x%x\n", __FUNCTION__, update_pvd_p->object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s update PVD 0x%x successful %d\n", __FUNCTION__, update_pvd_p->object_id, status);
    

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
 * end fbe_reinitialize_provision_drive_update_configuration_in_memory_function()
 ******************************************************************************/

/*!**************************************************************
 * fbe_configuration_service_destroy_pvd()
 ****************************************************************
 * @brief
 * This function asks configuration service to destroy a pvd object.  
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t fbe_configuration_service_destroy_pvd(fbe_database_transaction_id_t transaction_id, 
                                                                      fbe_object_id_t pvd_object_id)                                                                    
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_destroy_object_t destroy_pvd;
    fbe_base_config_upstream_object_list_t  upstream_list;
    fbe_database_control_destroy_edge_t     edge_destroyed;
    fbe_u32_t                               index =0;

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
    return status;
}

/*!****************************************************************************
 * fbe_reinitialize_provision_drive_rollback_function()
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
 *  05/11/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinitialize_provision_drive_rollback_function(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_status_t                payload_status;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "reinit_pvd_rollback_fun entry\n");

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
    update_pvd_p = (fbe_job_service_update_provision_drive_t *)job_queue_element_p->command_data;
    if(update_pvd_p == NULL)
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
                "reinit_pvd_rollback_fun, rollback not required, prev state %d, cur state %d\n", 
                job_queue_element_p->previous_state,
                job_queue_element_p->current_state);
    }

    if((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
      (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY))
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "reinit_pvd_rollback_fun: abort transaction id %d\n", (int)update_pvd_p->transaction_id);

        /* rollback the configuration transaction. */
        status = fbe_create_lib_abort_database_transaction(update_pvd_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "reinit_pvd_rollback_fun: cannot abort transaction %d with cfg service\n", 
                              (int)update_pvd_p->transaction_id);
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
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "reinit_pvd_rollback_fun: status change notification fail, status:%d\n",
                status);
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
 * end fbe_reinitialize_provision_drive_rollback_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_reinitialize_provision_drive_persist_function()
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
 *  05/11/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinitialize_provision_drive_persist_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *  update_pvd_p = NULL;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_control_buffer_length_t         length = 0;
    fbe_job_queue_element_t *                   job_queue_element_p = NULL;
    fbe_database_control_drive_connection_t database_drive_connection;
    fbe_create_lib_physical_drive_info_t drive_info;
    
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
                          "PVD_reinit_persist: failed to commit PVD config type, status:%d, config_type:0x%x\n",
                          status, update_pvd_p->config_type);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*only after database transaction commit can we connect pvd to downstream LDO
    *this is because the PVD is connected to LDO by the updated database PVD entry*/
    database_drive_connection.request_size = 1;
    database_drive_connection.PVD_list[0] = update_pvd_p->object_id;
    status = fbe_create_lib_send_control_packet (
                              FBE_DATABASE_CONTROL_CODE_CONNECT_DRIVES,
                              &database_drive_connection,
                              sizeof(database_drive_connection),
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID,
                              FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to try to connect PVD 0x%x to LDO, status:%d\n",
                          __FUNCTION__, update_pvd_p->object_id, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_get_drive_info_by_serial_number(update_pvd_p->serial_num, &drive_info);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s get downstream drive info failed for PVD 0x%x\n", __FUNCTION__, update_pvd_p->object_id);
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*now wait for the connection success so that we can do FRU descriptor and signature persist*/
    status = fbe_reinit_wait_for_pvd_downstream_connection(update_pvd_p->object_id, 60/*seconds*/);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s failed to wait for pvd 0x%x downstream connection success. status:0x%x\n",
                          __FUNCTION__, update_pvd_p->object_id, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*now update the fru descriptor if this drive is a system drive*/
    if(fbe_private_space_layout_object_id_is_system_pvd(update_pvd_p->object_id))
    {
        fbe_u32_t system_drive_slot = update_pvd_p->object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;
        fbe_homewrecker_fru_descriptor_t  fru_descriptor;

        status = fbe_reinit_pvd_get_fru_descriptor(&fru_descriptor);
        if(FBE_STATUS_OK != status)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s get fru descriptor failed\n", __FUNCTION__);
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }

        if(system_drive_slot >= FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s this system drive's index >= FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER\n", __FUNCTION__);
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }

        fbe_copy_memory(fru_descriptor.system_drive_serial_number[system_drive_slot],
                                                update_pvd_p->serial_num,
                                                sizeof(update_pvd_p->serial_num));

        status = fbe_reinit_pvd_set_fru_descriptor(&fru_descriptor);
        if(FBE_STATUS_OK != status)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s set fru descriptor failed in processing pvd 0x%x\n", __FUNCTION__, update_pvd_p->object_id);
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }       
    }


    /*At last update the fru signature on the disk*/
    status = fbe_reinit_pvd_set_fru_signature(drive_info.bus, drive_info.enclosure, drive_info.slot);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s fail set fru signature in processing pvd 0x%x\n", __FUNCTION__, update_pvd_p->object_id);
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    status = fbe_reinit_pvd_send_notification_to_kms(update_pvd_p->object_id, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s failed to get new keys. objid:0x%x\n", __FUNCTION__, update_pvd_p->object_id);

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
 * end fbe_reinitialize_provision_drive_persist_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_reinitialize_provision_drive_commit_function()
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
 *  05/11/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinitialize_provision_drive_commit_function(fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_job_service_update_provision_drive_t *      update_pvd_p = NULL;
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
    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "reinitialized pvd committed: PVD(0x%x) \n",
                      update_pvd_p->object_id);

    /* Send a notification for this commit*/
    status = fbe_update_provision_drive_send_notification(update_pvd_p,
                                                          job_queue_element_p->job_number,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR,
                                                          FBE_JOB_ACTION_STATE_COMMIT);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "reinit_pvd_commit: stat change notification fail, stat %d\n", status);
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

    /* Complete the packet so that the job service thread can continue */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_reinitialize_provision_drive_commit_function()
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
    notification_info.notification_data.job_service_error_info.job_type = FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE;
    notification_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    //notification_info.notification_data.job_action_state = job_action_state;

    /* send the notification to registered callers. */
    status = fbe_notification_send(update_pvd_p->object_id, notification_info);
    return status;
}
/******************************************************************************
 * end fbe_update_provision_drive_send_notification()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_reinit_pvd_get_drive_info()
 ******************************************************************************
 * @brief
 * This function sends the get drive info control packet to the pvd to get the
 * drive info
 *
 * @param packet_p - The packet requesting this operation.
 * @param vd_object_id - Virtual drive object id.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/10/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinit_pvd_get_drive_info(fbe_object_id_t pvd_object_id,
                              fbe_spare_drive_info_t *in_spare_drive_info_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Verify the input hot spare pvd object id.
     */
    if (pvd_object_id == FBE_OBJECT_ID_INVALID)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    /* Initialize the spare drive information with default values. */
    fbe_spare_initialize_spare_drive_info(in_spare_drive_info_p);

    /* Send the control packet and wait for its completion. */
    status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
                                                     in_spare_drive_info_p,
                                                     sizeof(fbe_spare_drive_info_t),
                                                     FBE_PACKAGE_ID_SEP_0,
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     pvd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s:get virtual drive spare information failed.\n", 
                           __FUNCTION__);
                               
    }
    return status;
}
/******************************************************************************
 * end fbe_reinit_pvd_get_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_pvd_check_with_upstream_objects()
 ******************************************************************************
 * @brief
 * This function is used to check whether reinitialization can proceed or not
 * with upstream objects. Even if one of the upstream object says it cannot
 * proceed, then we will not proceed with the reinit
 *
 * @param upstream_object_list
 * @param can_proceed 
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  05/08/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_pvd_check_with_upstream_objects(fbe_object_id_t *upstream_object_list, 
                                    fbe_u32_t object_count,
                                    fbe_bool_t  *can_proceed)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   index = 0;
    fbe_bool_t                  ok_to_proceed = FBE_FALSE;

    for(index=0; index < object_count; index++)
    {
        status = fbe_create_lib_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_CAN_PVD_GET_REINTIALIZED,
                                                 &ok_to_proceed,
                                                 sizeof(fbe_bool_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 upstream_object_list[index],
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
        if(status == FBE_STATUS_OK)
        {
            *can_proceed = ok_to_proceed;
            if(ok_to_proceed == FBE_FALSE)
            {
                return status;
            }

        }
        else if(status != FBE_STATUS_OK)
        {
            *can_proceed = FBE_FALSE;
            return status;
        }
    }

    return status;
}
/******************************************************************************
 * end fbe_pvd_check_with_upstream_objects()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_reinit_pvd_mark_NR_for_upstream_objects()
 ******************************************************************************
 * @brief
 * This function is used to check whether reinitialization can proceed or not
 * with upstream objects. Even if one of the upstream object says it cannot
 * proceed, then we will not proceed with the reinit
 *
 * @param upstream_object_list
 * @param can_proceed 
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  05/08/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinit_pvd_mark_NR_for_upstream_objects(fbe_object_id_t   *upstream_object_list, 
                                            fbe_object_id_t   *upstream_vd_list, 
                                            fbe_u32_t         object_count,
                                            fbe_object_id_t   pvd_object_id,
                                            fbe_bool_t        *is_NR_successful)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   index = 0;
    fbe_base_config_drive_to_mark_nr_t      drive_to_mark_nr;
    fbe_u32_t                   retry_count = 0;
    fbe_u32_t                   max_retry_count = 30;

    drive_to_mark_nr.pvd_object_id = pvd_object_id;
    drive_to_mark_nr.nr_status = FBE_FALSE;
    drive_to_mark_nr.force_nr = FBE_FALSE;

    for(index=0; index < object_count; )
    {
        if(!fbe_private_space_layout_object_id_is_system_raid_group(upstream_object_list[index]))
        {
            drive_to_mark_nr.vd_object_id = upstream_vd_list[index];
        }
        status = fbe_create_lib_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_MARK_NR,
                                                 &drive_to_mark_nr,
                                                 sizeof(fbe_base_config_drive_to_mark_nr_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 upstream_object_list[index],
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
        if(status == FBE_STATUS_OK)
        {
            *is_NR_successful = drive_to_mark_nr.nr_status;
            if(drive_to_mark_nr.nr_status == FBE_FALSE)
            {
                job_service_trace(FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "reinit_pvd_mark_NR: RG 0x%x refuses MARK NR. Need retry...\n",
                                   upstream_object_list[index]);

                /*Need to retry because RG may still not finish eval/clear RB. 
                 *This is typical when a system drive is pulled offline and
                 *then system is booted*/
                if(retry_count == max_retry_count)
                {
                    job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "reinit_pvd_mark_NR: RG 0x%x still refuses MARK NR after retry...\n",
                                       upstream_object_list[index]);
                    return status;
                }

                retry_count++;
                fbe_thread_delay(1000);
            
                continue;
            }
        }
        else if(status == FBE_STATUS_IO_FAILED_RETRYABLE)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "reinit_pvd_mark_NR: retryable error on RG 0x%x. Retry...\n",
                               upstream_object_list[index]);
            continue;            
        }
        else
        {
            *is_NR_successful = FBE_FALSE;
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "reinit_pvd_mark_NR: fail to mark NR on RG 0x%x. status: 0x%x\n",
                               upstream_object_list[index],
                               status);
            return status;
        }

        retry_count = 0;

        index++;
    }

    return status;
}
/******************************************************************************
 * end fbe_reinit_pvd_mark_NR_for_upstream_objects()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_reinit_pvd_update_provision_drive()
 ******************************************************************************
 * @brief
 * This function is used to update the config of the provision drive
 * object.
 *
 * @param  update_provision_drive 
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  05/09/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
fbe_status_t
fbe_reinit_pvd_update_provision_drive(fbe_database_control_update_pvd_t  *update_provision_drive)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "reinit pvd config:update pvd-obj:0x%x,trans_id:0x%x,update_type:0x%x\n",
                       update_provision_drive->object_id, 
                       (unsigned int)update_provision_drive->transaction_id, 
                       update_provision_drive->update_type);

    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "reinit pvd config DB_CTRL_CODE_UPDATE_PVD failed\n");
                               
    }
    return status;
}

/***************************************************
 * end fbe_reinit_pvd_update_provision_drive()
 ***************************************************/

/*!****************************************************************************
 * fbe_reinit_pvd_get_fru_descriptor()
 ******************************************************************************
 * @brief
 * This function is used to get system drive fru descriptor from database service
 *
 * @param  fru_descriptor - the read fru descriptor is copied to this buffer
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  08/29/2012 - Created. Zhipeng Hu
 ******************************************************************************/
static fbe_status_t fbe_reinit_pvd_get_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor)
{
    fbe_status_t    status;
    fbe_database_control_get_fru_descriptor_t get_fru_descriptor;

    if(NULL == fru_descriptor)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_GET_FRU_DESCRIPTOR,
                                                &get_fru_descriptor,
                                                sizeof(get_fru_descriptor),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: fail get fru descriptor\n", __FUNCTION__);
        return status;                               
    }

    fbe_copy_memory(fru_descriptor, &get_fru_descriptor.descriptor, sizeof(*fru_descriptor));

    return status;
}


static fbe_status_t fbe_reinit_pvd_set_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor)
{
    fbe_status_t    status;
    fbe_database_control_set_fru_descriptor_t set_fru_descriptor;

    if(NULL == fru_descriptor)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    set_fru_descriptor.structure_version = fru_descriptor->structure_version;
    set_fru_descriptor.chassis_replacement_movement = fru_descriptor->chassis_replacement_movement;
    set_fru_descriptor.wwn_seed = fru_descriptor->wwn_seed;
    fbe_copy_memory(set_fru_descriptor.system_drive_serial_number, fru_descriptor->system_drive_serial_number, 
                                            FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER*sizeof(serial_num_t));

    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_SET_FRU_DESCRIPTOR,
                                                &set_fru_descriptor,
                                                sizeof(set_fru_descriptor),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: fail set fru descriptor\n", __FUNCTION__);
    }

    return status;
}

static fbe_status_t fbe_reinit_pvd_set_fru_signature(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_status_t    status;
    fbe_database_control_signature_t set_fru_signature;

    set_fru_signature.bus = bus;
    set_fru_signature.enclosure = encl;
    set_fru_signature.slot = slot;

    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_SET_DISK_SIGNATURE,
                                                &set_fru_signature,
                                                sizeof(set_fru_signature),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: fail set fru signature for drive %d_%d_%d\n", 
                               __FUNCTION__,
                               bus, encl, slot);
    }

    return status;
}

static fbe_status_t fbe_reinit_wait_for_pvd_downstream_connection(fbe_object_id_t object_id, fbe_u32_t timeout_in_secs)
{
    fbe_status_t    status;
    fbe_base_config_downstream_edge_list_t          down_edges;
    fbe_bool_t  do_count = timeout_in_secs> 0?FBE_TRUE:FBE_FALSE;

    while(1)
    {
        status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST,
                                                    &down_edges,
                                                    sizeof(down_edges),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    object_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if(FBE_STATUS_OK != status)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: fail get downstream edge for PVD 0x%x\n", 
                                   __FUNCTION__,
                                   object_id);
            return status;
        }

        if(down_edges.number_of_downstream_edges > 0)
        {
            return FBE_STATUS_OK;
        }

        if(FBE_TRUE == do_count)
        {
            timeout_in_secs--;
            if(0 == timeout_in_secs)
                break;
        }

        fbe_thread_delay(1000);
        
    }

    return FBE_STATUS_TIMEOUT;
    
}

/*!****************************************************************************
 * fbe_reinit_pvd_apply_drive_replacement_algorithm()
 ******************************************************************************
 * @brief
 * This function applies drive "spare" algorithm to the newly inserted system
 * drive and then decide whether we can reinitialize and connect the system PVD 
 *
 * @param[in]   job_queue_element_p - job element passed in by job service
 * @param[out]  can_be_reinit - whether we can reinitialize and connect the system PVD 
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  02/26/2013 - Created. Zhipeng Hu
 ******************************************************************************/
static fbe_status_t fbe_reinit_pvd_apply_drive_replacement_algorithm(fbe_job_queue_element_t* job_queue_element_p, fbe_bool_t* can_be_reinit)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_spare_selection_info_t desired_drive_info = {0};
    fbe_spare_selection_info_t replace_drive_info = {0};
    fbe_spare_selection_info_t algorithm_result_drive_info = {0};
    fbe_job_service_update_provision_drive_t* update_pvd_p = NULL;
    fbe_bool_t      info_corrected = FBE_FALSE;
    fbe_object_id_t pvd_object_id;

    if(NULL == job_queue_element_p || NULL == can_be_reinit)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: NULL parameter\n", __FUNCTION__);
        return status;
    }

    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;
    if(NULL == update_pvd_p)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: NULL command buffer\n", __FUNCTION__);
        return status;
    }

    pvd_object_id = update_pvd_p->object_id;
    *can_be_reinit = FBE_FALSE;

    /*1. first get the original system drive's information*/
    fbe_spare_initialize_spare_selection_drive_info(&desired_drive_info);
    desired_drive_info.spare_object_id = pvd_object_id;
    
    status = fbe_reinit_pvd_get_drive_info(pvd_object_id, &desired_drive_info.spare_drive_info);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail to get system PVD info. status: 0x%x\n",
                          __FUNCTION__,
                          status);
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: desired system drive info before correction:\n",
                      __FUNCTION__);

    fbe_reinit_pvd_print_drive_info(&desired_drive_info);

    status = fbe_reinit_pvd_correct_system_drive_info_if_needed(&desired_drive_info, &info_corrected);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail to correct desired system drive info. status: 0x%x\n",
                          __FUNCTION__,
                          status);
        return status;
    }
    else if(FBE_TRUE == info_corrected)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: corrected desired system drive info:\n",
                          __FUNCTION__);
        fbe_reinit_pvd_print_drive_info(&desired_drive_info);
    }
    else
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: not correct desired system drive info.\n",
                          __FUNCTION__);
    }

    /*2. get the new system drive's information*/
    status = fbe_reinit_pvd_generate_spare_info_from_physical_drive(job_queue_element_p,
                    0, /*port*/
                    0, /*enclosure*/
                    pvd_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST, /*slot*/
                    &replace_drive_info /*generated drive info*/);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail to generate drive info for sys slot %d. status: 0x%x\n",
                          __FUNCTION__,
                          pvd_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST,
                          status);
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s: replacement system drive info:\n",
                      __FUNCTION__);

    fbe_reinit_pvd_print_drive_info(&replace_drive_info);

    /*3. apply the drive "spare" algorithm*/
    fbe_spare_initialize_spare_selection_drive_info(&algorithm_result_drive_info);
    
    status = fbe_spare_lib_selection_apply_selection_algorithm(&desired_drive_info, /*original drive's info*/
                    &replace_drive_info, /*new system drive's info*/
                    &algorithm_result_drive_info /*algorithm apply result*/);
    if(FBE_OBJECT_ID_INVALID == algorithm_result_drive_info.spare_object_id)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: the new drive in sys slot %d is not suitable for spare\n",
                          __FUNCTION__,
                          pvd_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST);
        return FBE_STATUS_OK;
    }

    *can_be_reinit = FBE_TRUE;
    return FBE_STATUS_OK;

}
/*!****************************************************************************
 * end fbe_reinit_pvd_apply_drive_replacement_algorithm()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_reinit_pvd_generate_spare_info_from_physical_drive()
 ******************************************************************************
 * @brief
 * This function generates spare drive info from the physical drive
 * Usally used in the case where the corresponding PVD is not created or fully
 * initialized.
 *
 * @param[in]   job_queue_element_p - job element passed in by job service
 * @param[in]   port_number - drive location: port
 * @param[in]   enclosure_number - drive location: enclosure
 * @param[in]   slot_number - drive location: slot
 * @param[out]  spare_drive_info - generated spare drive information
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  02/27/2013 - Created. Zhipeng Hu
 ******************************************************************************/
static fbe_status_t fbe_reinit_pvd_generate_spare_info_from_physical_drive(
                                                                fbe_job_queue_element_t* job_queue_element_p,
                                                                fbe_u32_t  port_number,
                                                                fbe_u32_t enclosure_number,
                                                                fbe_u32_t slot_number,
                                                                fbe_spare_selection_info_t* spare_drive_info)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_update_provision_drive_t* update_pvd_p = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t get_physical_drive_info;
    fbe_topology_control_get_physical_drive_by_location_t    topology_location;

    if(NULL == spare_drive_info || NULL == job_queue_element_p)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: NULL parameter\n", __FUNCTION__);
        return status;
    }

    update_pvd_p = (fbe_job_service_update_provision_drive_t *) job_queue_element_p->command_data;
    if(NULL == update_pvd_p)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: NULL command buffer\n", __FUNCTION__);
        return status;
    }

    fbe_spare_initialize_spare_selection_drive_info(spare_drive_info);

    /******************************************************************************
     *              1. get downstream PDO's object id
     ******************************************************************************/
    topology_location.port_number      = port_number;
    topology_location.enclosure_number = enclosure_number;
    topology_location.slot_number      = slot_number;
    topology_location.physical_drive_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_create_lib_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                &topology_location,
                                                sizeof(topology_location),
                                                FBE_PACKAGE_ID_PHYSICAL,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_EXTERNAL);    
    if(FBE_STATUS_OK != status || FBE_OBJECT_ID_INVALID == topology_location.physical_drive_object_id)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail get pdo by location.0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /******************************************************************************
     *              2. Get the physical drive information
     ******************************************************************************/
    get_physical_drive_info.get_drive_info.port_number = FBE_PORT_NUMBER_INVALID;
    get_physical_drive_info.get_drive_info.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    get_physical_drive_info.get_drive_info.slot_number = FBE_SLOT_NUMBER_INVALID;
    get_physical_drive_info.get_drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
    get_physical_drive_info.get_drive_info.spin_down_qualified = FBE_FALSE;

    status = fbe_create_lib_send_control_packet(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                                &get_physical_drive_info,
                                                sizeof(get_physical_drive_info),
                                                FBE_PACKAGE_ID_PHYSICAL,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                topology_location.physical_drive_object_id,
                                                FBE_PACKET_FLAG_EXTERNAL);
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail get physical drive info. 0x%x\n", __FUNCTION__, status);
        return status;
    }
    
    /******************************************************************************
     *              3. We can generate the spare drive info now
     ******************************************************************************/
    spare_drive_info->spare_drive_info.port_number = get_physical_drive_info.get_drive_info.port_number;
    spare_drive_info->spare_drive_info.enclosure_number = get_physical_drive_info.get_drive_info.enclosure_number;
    spare_drive_info->spare_drive_info.slot_number = get_physical_drive_info.get_drive_info.slot_number;
    spare_drive_info->spare_drive_info.drive_type = get_physical_drive_info.get_drive_info.drive_type;
    spare_drive_info->spare_drive_info.configured_block_size = update_pvd_p->configured_physical_block_size;
    spare_drive_info->spare_drive_info.configured_capacity = update_pvd_p->configured_capacity;
    spare_drive_info->spare_object_id = update_pvd_p->object_id;
    
    /*has no PVD created/reinit for this new drive, so here just set it as default offset*/
    fbe_database_get_default_offset(FBE_CLASS_ID_PROVISION_DRIVE, update_pvd_p->object_id, 
                                    &spare_drive_info->spare_drive_info.exported_offset);
    spare_drive_info->spare_drive_info.capacity_required = spare_drive_info->spare_drive_info.exported_offset;
    
    spare_drive_info->spare_drive_info.b_is_system_drive = FBE_FALSE;
    if(FBE_TRUE == fbe_private_space_layout_object_id_is_system_pvd(update_pvd_p->object_id))
    {
        spare_drive_info->spare_drive_info.b_is_system_drive = FBE_TRUE;
    }
    spare_drive_info->spare_drive_info.b_is_end_of_life = FBE_FALSE; /*has no PVD created/reinit for this drive, so here just set it to FALSE*/
    spare_drive_info->spare_drive_info.b_is_slf = FBE_FALSE; /*has no PVD created/reinit for this drive, so here just set it to FALSE*/
    
    return FBE_STATUS_OK;    

}
/*!****************************************************************************
 * end fbe_reinit_pvd_generate_spare_info_from_physical_drive()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_reinit_pvd_light_on_amber_led()
 ******************************************************************************
 * @brief
 * This function light on the LED on the specified drive
 *
 * @param[in]   port_number - drive location: port
 * @param[in]   enclosure_number - drive location: enclosure
 * @param[in]   slot_number - drive location: slot
 *
 * @return status     - The status of the operation.
 *
 * @author
 *  02/27/2013 - Created. Zhipeng Hu
 ******************************************************************************/
static fbe_status_t fbe_reinit_pvd_light_on_amber_led(fbe_u32_t  port_number,
                                                       fbe_u32_t  enclosure_number,
                                                       fbe_u32_t  slot_number)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_physical_drive_by_location_t    topology_location;
    fbe_block_transport_logical_state_t drive_state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT;

    /*get downstream PDO's object id*/
    topology_location.port_number      = port_number;
    topology_location.enclosure_number = enclosure_number;
    topology_location.slot_number      = slot_number;
    topology_location.physical_drive_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_create_lib_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                &topology_location,
                                                sizeof(topology_location),
                                                FBE_PACKAGE_ID_PHYSICAL,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_EXTERNAL);    
    if(FBE_STATUS_OK != status || FBE_OBJECT_ID_INVALID == topology_location.physical_drive_object_id)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail get pdo by location.0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*send drive fault to PDO so that it will light on the LED*/
    status = fbe_create_lib_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED,
                                                &drive_state,
                                                sizeof(drive_state),
                                                FBE_PACKAGE_ID_PHYSICAL,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                topology_location.physical_drive_object_id,
                                                FBE_PACKET_FLAG_EXTERNAL);    
    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: fail to light on LED. 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
    
}

static void fbe_reinit_pvd_print_drive_info(fbe_spare_selection_info_t* drive_info)
{
    if(NULL == drive_info)
    {
        return;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "   object id: 0x%x\n", drive_info->spare_object_id);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "   location: %d_%d_%d drive type: %d\n",
            drive_info->spare_drive_info.port_number,
            drive_info->spare_drive_info.enclosure_number,
            drive_info->spare_drive_info.slot_number,
            drive_info->spare_drive_info.drive_type);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "   exp_offset: 0x%llx cfg_capacity: 0x%llx cap_required: 0x%llx blk_sz: 0x%x\n",
            drive_info->spare_drive_info.exported_offset,
            drive_info->spare_drive_info.configured_capacity,
            drive_info->spare_drive_info.capacity_required,
            drive_info->spare_drive_info.configured_block_size);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "   pool_id: 0x%x is_system_drive: %d is_end_of_life: %d is_slf: %d\n",
            drive_info->spare_drive_info.pool_id,
            drive_info->spare_drive_info.b_is_system_drive,
            drive_info->spare_drive_info.b_is_end_of_life,
            drive_info->spare_drive_info.b_is_slf);
    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    return;

}

static fbe_status_t fbe_reinit_pvd_correct_system_drive_info_if_needed(fbe_spare_selection_info_t* drive_info, fbe_bool_t *is_corrected)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       system_drive_count = fbe_private_space_layout_get_number_of_system_drives();
    fbe_provision_drive_info_t  cur_provision_drive_info;
    fbe_u32_t       drive_index = 0;
    fbe_object_id_t system_pvd_id;

    if(NULL == drive_info || NULL == is_corrected)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*currently we only correct the drive type and location*/
    if(drive_info->spare_drive_info.drive_type != FBE_DRIVE_TYPE_INVALID
       && drive_info->spare_drive_info.port_number == 0
       && drive_info->spare_drive_info.enclosure_number == 0
       && drive_info->spare_drive_info.slot_number == (drive_info->spare_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST))
    {
        *is_corrected = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    *is_corrected = FBE_TRUE;

    drive_info->spare_drive_info.port_number = 0;
    drive_info->spare_drive_info.enclosure_number = 0;
    drive_info->spare_drive_info.slot_number = drive_info->spare_object_id - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST;

    for(drive_index = 0; drive_index < system_drive_count; drive_index++)
    {
        system_pvd_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + drive_index;

        if(system_pvd_id == drive_info->spare_object_id)
        {
            continue;
        }

        status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                    &cur_provision_drive_info,
                                                    sizeof(cur_provision_drive_info),
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    FBE_SERVICE_ID_TOPOLOGY,
                                                    FBE_CLASS_ID_INVALID,
                                                    system_pvd_id,
                                                    FBE_PACKET_FLAG_NO_ATTRIB);
        if(FBE_STATUS_OK != status)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: fail to get PVD 0x%x info. 0x%x\n",
                              __FUNCTION__,
                              system_pvd_id, status);
            continue;
        }

        /*only correct drive type here because this is the only information wiped out in previouse PVD
         *reinitialized job*/        
        if(cur_provision_drive_info.drive_type != FBE_DRIVE_TYPE_INVALID
           && drive_info->spare_drive_info.drive_type == FBE_DRIVE_TYPE_INVALID)
        {
            drive_info->spare_drive_info.drive_type = cur_provision_drive_info.drive_type;
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: utilize PVD 0x%x to correct drive info\n",
                              __FUNCTION__,
                              system_pvd_id);
            break;
        }
        
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_reinit_pvd_()
 ******************************************************************************
 * @brief
 * This function is used to check whether reinitialization can proceed or not
 * with upstream objects. Even if one of the upstream object says it cannot
 * proceed, then we will not proceed with the reinit
 *
 * @param upstream_object_list
 * @param can_proceed 
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  05/08/2012 - Created. Ashwin Tamilarasan
 ******************************************************************************/
static fbe_status_t
fbe_reinit_get_downstream_position(fbe_object_id_t rg_id, 
                                   fbe_object_id_t server_id, 
                                   fbe_u32_t *position)
{
    fbe_status_t status;
    fbe_raid_group_get_disk_pos_by_server_id_t server_info;

    server_info.server_id = server_id;
    server_info.position = FBE_EDGE_INDEX_INVALID;

    status = fbe_create_lib_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_DRIVE_POS_BY_SERVER_ID,
                                                &server_info,
                                                sizeof(fbe_raid_group_get_disk_pos_by_server_id_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                rg_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);
    *position = server_info.position;
    return status;
}

/*!***************************************************************************
 *          fbe_reinit_send_kms_notification()
 ***************************************************************************** 
 * 
 * @brief   This function sends a notification to KMS for keys
 *
 * @param   rg_id - Raid Group ID for which the keys needs to be updated
 * @param   position - Position of the drive in RG
 * @param   encryption_state - encryption state.
 * @param   swap_command_type - Type of key removal
 * 
 * @return fbe_status_t
 *
 * @author
 *  05/05/2014  Ashok Tamilarasan  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_reinit_send_kms_notification(fbe_object_id_t rg_id, 
                                 fbe_u8_t position,
                                 fbe_base_config_encryption_state_t encryption_state,
                                 fbe_spare_swap_command_t swap_command_type)
{
    fbe_notification_info_t notification_info;
    fbe_status_t status;

    fbe_zero_memory(&notification_info, sizeof(fbe_notification_info_t));
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_ENCRYPTION_STATE_CHANGED;    
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    notification_info.class_id = FBE_CLASS_ID_RAID_GROUP;

    notification_info.notification_data.encryption_info.encryption_state = encryption_state;
    notification_info.notification_data.encryption_info.object_id = rg_id;
    notification_info.notification_data.encryption_info.ignore_state = FBE_TRUE;
    notification_info.notification_data.encryption_info.width = position; /* We cache VD position in RG here */
    notification_info.notification_data.encryption_info.u.vd.swap_command_type = swap_command_type;
    notification_info.notification_data.encryption_info.u.vd.edge_index = position;
    notification_info.notification_data.encryption_info.u.vd.upstream_object_id = rg_id;

    fbe_database_lookup_raid_by_object_id(rg_id, &notification_info.notification_data.encryption_info.control_number);
    if (notification_info.notification_data.encryption_info.control_number == FBE_RAID_ID_INVALID)
    {
        fbe_database_lookup_raid_for_internal_rg(rg_id, &notification_info.notification_data.encryption_info.control_number);
    }

    status = fbe_notification_send(rg_id, notification_info);
    if (status != FBE_STATUS_OK) {
	     job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: fail to send notification\n",
                              __FUNCTION__);

    }

    return status;
}

/*!***************************************************************************
 *          fbe_reinit_pvd_send_notification_to_kms()
 ***************************************************************************** 
 * 
 * @brief   This function sends notifications to KMS for all RGs above a pvd
 *
 * @param   pvd_object_id - pvd object id
 * @param   state - encryption state
 * 
 * @return fbe_status_t
 *
 * @author
 *  05/21/2014  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_reinit_pvd_send_notification_to_kms(fbe_object_id_t pvd_object_id, fbe_base_config_encryption_state_t state)
{
    fbe_system_encryption_mode_t				system_encryption_mode;
    fbe_u32_t                                   number_of_upstream_objects = 0;
    fbe_object_id_t                             upstream_rg_object_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_object_id_t                             upstream_vd_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_u8_t index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t server_object_id;
    fbe_u32_t position;
	fbe_status_t status;

    /* if the System is in encrypted mode, the old keys needs to be cleaned up before the new drive and new keys
     * are provided 
     */
    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if(system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        return FBE_STATUS_OK;
    }

    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED) 
    {
        status = fbe_reinit_pvd_wait_pvd_ready_for_keys(pvd_object_id, 30000);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting PVD object id %d ready. status %d\n", 
                              __FUNCTION__,  pvd_object_id, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_create_lib_get_pvd_upstream_edge_list(pvd_object_id, &number_of_upstream_objects,
                                              upstream_rg_object_list, upstream_vd_list);

    for(index=0; index < number_of_upstream_objects; index++)
    {
        rg_object_id = upstream_rg_object_list[index];
        if(fbe_private_space_layout_object_id_is_system_raid_group(rg_object_id)){
            if (rg_object_id != FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG){
                continue;
            }
            server_object_id = pvd_object_id;
        } else {
            server_object_id = upstream_vd_list[index];
        }

        /* First get the downstream position of this drive in RG. Need this to feed it to KMS */
        status = fbe_reinit_get_downstream_position(rg_object_id, server_object_id, &position);
        if(status != FBE_STATUS_OK || position == FBE_EDGE_INDEX_INVALID)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "failed to get position. Status:%d, position: %d, objid:0x%x server:0x%08x\n", 
                              status, position, rg_object_id, server_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s Notify KMS RG:0x%08x,Pos:%d State 0x%x\n", 
                          __FUNCTION__, rg_object_id, position, state);

        /* We need to notify KMS */
        fbe_reinit_send_kms_notification(rg_object_id, 
                                         position, 
                                         state,
                                         FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_reinit_pvd_send_notification_to_kms()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_reinit_pvd_wait_pvd_ready_for_keys()
 ***************************************************************************** 
 * 
 * @brief   This function waits pvd to be ready/activate for the new keys.
 *
 * @param   obj_id - pvd object id
 * @param   timeout_ms - timeout ms
 * 
 * @return fbe_status_t
 *
 * @author
 *  05/30/2014  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_reinit_pvd_wait_pvd_ready_for_keys(fbe_object_id_t obj_id, fbe_u32_t timeout_ms)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_lifecycle_state_t current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_u32_t           current_time = 0;
    fbe_bool_t			got_expected= FBE_FALSE;
    fbe_database_state_t   db_peer_state;

    while (current_time < timeout_ms) {
        status = fbe_job_service_get_object_lifecycle_state(obj_id, &current_state);
        if (status == FBE_STATUS_OK){
            if (current_state == FBE_LIFECYCLE_STATE_ACTIVATE || current_state == FBE_LIFECYCLE_STATE_READY){
                /*we are happy here, let's check the peer*/
                got_expected = FBE_TRUE;
                break;
            }
        }
        current_time = current_time + 100; 
        fbe_thread_delay(100);
    }

    if (!got_expected) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s: object 0x%x timeout this SP current state %d in %d ms!\n", 
                          __FUNCTION__, obj_id, current_state, timeout_ms);
    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_job_service_get_database_peer_state(&db_peer_state);
    if (!fbe_cmi_is_peer_alive() || db_peer_state != FBE_DATABASE_STATE_READY) {
        return FBE_STATUS_OK;
    }

    /*now let's check on the peer*/
    while (current_time < timeout_ms) {
        if (fbe_cmi_is_peer_alive()) {
            status = fbe_job_service_get_peer_object_state(obj_id, &current_state);
            if (status == FBE_STATUS_OK) {
                if (current_state == FBE_LIFECYCLE_STATE_ACTIVATE || current_state == FBE_LIFECYCLE_STATE_READY){
                    return FBE_STATUS_OK;
                }
            }
            current_time = current_time + 100; 
            fbe_thread_delay(100);
        }else{
            return FBE_STATUS_OK;/*peer is dead, who cares*/
        }
    }
    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s: object 0x%x timeout peer SP current state %d in %d ms!\n", 
                      __FUNCTION__, obj_id, current_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************************************************
 * end fbe_reinit_pvd_send_notification_to_kms()
 ******************************************************************************/
