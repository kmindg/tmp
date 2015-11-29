/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_create.c
 ***************************************************************************n
 *
 * @brief
 *  This file contains routines for Raid Group create used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   01/21/2010:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "global_catmerge_rg_limits.h"

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_parity.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes
#include "fbe_database.h"
#include "fbe_spare.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe_create_private.h"
#include "fbe_notification.h"
#include "fbe_private_space_layout.h"


/*@todo: start. these should be defined by a limits file 
 */
#define FBE_RG_USER_COUNT 256
#define RAID_GROUP_COUNT 256 
#define MAX_RAID_GROUP_ID GLOBAL_CATMERGE_RAID_GROUP_ID_MAX
#define FBE_PLATFORM_FRU_COUNT 1024

#define WAIT_FOR_PVD_RDY_LIFECYCLE        30000 /*wait maximum of 30 seconds*/

/*@todo: end
 */

/* Raid Group create library request validation function.
 */
static fbe_status_t fbe_raid_group_create_validation_function (fbe_packet_t *packet_p);

/* Raid Group create library update configuration in memory function.
 */
static fbe_status_t fbe_raid_group_create_update_configuration_in_memory_function (fbe_packet_t *packet_p);

/* Raid Group create library persist configuration in db function.
 */
static fbe_status_t fbe_raid_group_create_persist_configuration_db_function (fbe_packet_t *packet_p);

/* Raid Group create library rollback function.
 */
static fbe_status_t fbe_raid_group_create_rollback_function (fbe_packet_t *packet_p);

/* Raid Group create library commit function.
 */
static fbe_status_t fbe_raid_group_create_commit_function (fbe_packet_t *packet_p);
/* Job service lun create registration.
*/
fbe_job_service_operation_t fbe_raid_group_create_job_service_operation = 
{
    FBE_JOB_TYPE_RAID_GROUP_CREATE,
    {
        /* validation function */
        fbe_raid_group_create_validation_function,

        /* selection function */
        NULL,

        /* update in memory function */
        fbe_raid_group_create_update_configuration_in_memory_function,

        /* persist function */
        fbe_raid_group_create_persist_configuration_db_function,

        /* response/rollback function */
        fbe_raid_group_create_rollback_function,

        /* commit function */
        fbe_raid_group_create_commit_function,
    }
};

/* functions that validate raid group creation request */
static fbe_status_t fbe_raid_group_create_validate_parameters(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_validate_raid_group_class_parameters(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p);

static fbe_status_t fbe_raid_group_create_validate_provision_drive_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_validate_physical_drive_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_get_configured_provision_drive_info(
        fbe_object_id_t in_object_id,
        fbe_u32_t       num_upstream_objects, 
        fbe_provision_drive_configured_physical_block_size_t *configured_physical_block_size,
        fbe_lba_t *provision_drive_imported_capacity,
        fbe_lba_t *provision_drive_exported_capacity,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_get_physical_drive_info(fbe_object_id_t in_object_id,
                                                                  fbe_drive_type_t *drive_type);

static fbe_status_t fbe_raid_group_create_validate_configured_provision_drive_block_size(
        fbe_object_id_t in_object_id,
        fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size);

static fbe_bool_t fbe_raid_group_create_is_valid_raid_group_number(
        fbe_raid_group_number_t raid_group_id);

static fbe_bool_t fbe_raid_group_create_is_valid_raid_group_type(
         fbe_raid_group_type_t raid_type);

static fbe_status_t fbe_raid_group_find_duplicate_provision_drive_ids(
        fbe_u32_t provision_drive_id_array[],
        fbe_u32_t number_provision_drives);

static fbe_status_t fbe_raid_group_create_are_provision_drives_compatible(
        fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size[],
        fbe_u32_t number_provision_drives);

static fbe_status_t fbe_raid_group_create_are_physical_drives_compatible(
        fbe_drive_type_t drive_type_array[],
        fbe_u32_t number_provision_drives);

/* functions to obtain object's exported size */
static fbe_status_t fbe_raid_group_create_get_provision_drive_exported_size(fbe_object_id_t              pvd_object_id,
        fbe_lba_t in_provision_drive_imported_capacity, fbe_lba_t *out_provision_drive_exported_capacity);

static fbe_status_t fbe_raid_group_create_get_virtual_drive_capacity(fbe_lba_t virtual_drive_imported_capacity,
                                                                     fbe_lba_t virtual_drive_block_offset, 
                                                                     fbe_lba_t *out_virtual_drive_exported_capacity,
                                                                     fbe_lba_t *out_virtual_drive_imported_capacity);

/* functions to create objects that are part of a raid group */
static fbe_status_t fbe_raid_group_create_virtual_drive_object(
        fbe_database_transaction_id_t transaction_id,
        fbe_lba_t virtual_drive_exported_capacity, 
        fbe_lba_t virtual_drive_default_offset,
        fbe_object_id_t *virtual_drive_object_id);

static fbe_lba_t fbe_raid_group_create_get_minimum_virtual_drive_size(
        fbe_lba_t virtual_drive_capacity_array[], fbe_u32_t number_of_items_in_array);

static fbe_lba_t fbe_raid_group_create_get_object_lifecycle_state(
        fbe_object_id_t object_id,  fbe_lifecycle_state_t *lifecycle_state);

static fbe_status_t fbe_create_raid_group_create_virtual_drives_and_edges(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t *error_code_p,
        fbe_lba_t virtual_drive_exported_capacity[]);

static fbe_status_t fbe_create_raid_group_create_raid_group(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_element_size_t element_size,
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t *sep_raid_group_id);

static fbe_status_t fbe_create_raid_group_create_raid10_group(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_create_raid_group_create_raid10_group_top_bottom_approach(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_create_raid_group_create_raid10_group_bottom_top_approach(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_create_raid_group_create_raid_group_edges_to_virtual_drive(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_object_id_t sep_raid_group_id, fbe_lba_t minimum_virtual_drive_capacity);

static fbe_status_t fbe_create_raid_group_create_mirror_under_striper_rgs(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_u32_t         num_mirrors,
        fbe_element_size_t element_size, 
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t   mirror_rg_id[]);

static fbe_status_t fbe_create_raid_group_create_edges_between_striper_and_mirror(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_u32_t         num_mirrors,
        fbe_lba_t         mirror_rg_capacity,
        fbe_object_id_t   mirror_rg_id[]);

static fbe_status_t fbe_create_raid_group_create_edges_between_mirror_and_vd(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_u32_t         width,
        fbe_lba_t         edge_capacity,
        fbe_object_id_t   mirror_rg_id[]);

/* raid group utility functions */
static void fbe_raid_group_create_init_object_arrays(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p);

static fbe_status_t fbe_raid_group_create_get_provision_drive_by_location(
        fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, fbe_object_id_t *object_id);

static void fbe_raid_group_create_set_packet_status(fbe_payload_control_operation_t *control_operation,
                                         fbe_payload_control_status_t payload_status,
                                         fbe_packet_t *packet, fbe_status_t packet_status);

static fbe_class_id_t fbe_raid_group_create_get_class_id(fbe_raid_group_type_t raid_group_type);

static fbe_status_t fbe_raid_group_create_raid_group_creation(
        job_service_raid_group_create_t * rg_create_ptr,
        fbe_element_size_t element_size,
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t *object_id);

static fbe_status_t fbe_raid_group_create_raid10_group_creation(
        job_service_raid_group_create_t * rg_create_ptr,
        fbe_element_size_t element_size, 
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t *object_id);

static fbe_status_t fbe_raid_group_create_calculate_raid_group_capacity(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p, 
        fbe_lba_t *minimum_virtual_drive_capacity,
        fbe_element_size_t *element_size,
        fbe_elements_per_parity_t *elements_per_parity,
        fbe_lba_t virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_calculate_raid10_group_capacity(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p, 
        fbe_element_size_t *element_size,
        fbe_elements_per_parity_t *elements_per_parity,
        fbe_lba_t *rg_mirror_capacity);

static fbe_status_t fbe_create_raid_group_create_determine_if_extent_size_fits(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_object_id_t object_array[],
        fbe_u32_t      object_array_count,
        fbe_lba_t      *minimum_object_capacity,
        fbe_bool_t  b_ignore_offset,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_get_pvd_object_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_get_physical_drive_object_id_by_location_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code);

static fbe_status_t fbe_raid_group_create_translate_raid_type(fbe_raid_group_type_t raid_type, 
                                                              const char ** pp_control_code_name);

static fbe_status_t fbe_raid_group_create_generic_completion_function(fbe_packet_t * packet, 
                                                                      fbe_packet_completion_context_t context);
static void fbe_raid_group_create_print_request_elements(job_service_raid_group_create_t *req_p);

static fbe_status_t fbe_create_raid_group_create_get_extent_size(job_service_raid_group_create_t *fbe_raid_group_create_req_p,
                                                                 fbe_lba_t virtual_drive_capacity_array[]);

static fbe_status_t fbe_raid_group_create_get_unused_provision_drive_extent_size(fbe_object_id_t pvd_object_id, 
                                                                                 fbe_lba_t  *provision_drive_unused_extent_capacity);

static fbe_status_t fbe_create_rg_validate_system_limits(void);

static fbe_status_t fbe_create_lib_check_system_rg(fbe_raid_group_number_t rg_num,
                                                fbe_object_id_t rg_id);

static fbe_status_t fbe_raid_group_create_system_raid_group_and_edges(
        job_service_raid_group_create_t * rg_create_ptr);


/*!**************************************************************
 * fbe_raid_group_create_validation_function()
 ****************************************************************
 * @brief
 * This function validates the Raid Group create request
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_validation_function(fbe_packet_t *packet_p)
{
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    job_service_raid_group_create_t *fbe_raid_group_create_req_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_bool_t                          double_degraded;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);
    if (job_queue_element_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, job_queue_element_p is NULL\n", __FUNCTION__);

        fbe_raid_group_create_set_packet_status(control_operation,
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t)) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, incorrect lenght of job_queue_element_p provided\n", __FUNCTION__);        
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    fbe_raid_group_create_req_p = (job_service_raid_group_create_t *)job_queue_element_p->command_data;

    if (fbe_raid_group_create_req_p == NULL)
    {
      job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, parameter list for raid group creation request is empty\n", 
                __FUNCTION__);
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_raid_group_create_print_request_elements(fbe_raid_group_create_req_p);

    /*If we are going to create a system RG ....*/
    if (fbe_raid_group_create_req_p->user_input.is_system_rg)
    {
        
        status = fbe_create_lib_check_system_rg(fbe_raid_group_create_req_p->user_input.raid_group_id,
                                        fbe_raid_group_create_req_p->user_input.rg_id);
        if (status != FBE_STATUS_OK)
        {
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            fbe_raid_group_create_set_packet_status(control_operation, 
                    FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
            return status;
        }
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK,
                                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

    /*before sweating too much, let's see if the system configuration limits will allow up to create a new RG*/
    status = fbe_create_rg_validate_system_limits();
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s can't create new RG, reached system limits\n", 
                __FUNCTION__);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SYSTEM_LIMITS_EXCEEDED;
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }

    status = fbe_create_lib_validate_system_triple_mirror_healthy(&double_degraded);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s check system triple mirror healthy failed.\n", 
                __FUNCTION__);
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
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
        fbe_raid_group_create_set_packet_status(control_operation,
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }

    /* now validate raid group parameters */ 
    status = fbe_raid_group_create_validate_parameters(fbe_raid_group_create_req_p,
        &job_queue_element_p->error_code);
    if (status != FBE_STATUS_OK) 
    {
        job_queue_element_p->status = status;
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }

    fbe_raid_group_create_init_object_arrays(fbe_raid_group_create_req_p);

    /* convert bus, enclosure, slot, input to pvd id */
    status = fbe_raid_group_create_get_pvd_object_list(fbe_raid_group_create_req_p, &job_queue_element_p->error_code);
    if (status != FBE_STATUS_OK) 
    {
        job_queue_element_p->status = status;
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }

    /* validate PVD info */
    status = fbe_raid_group_create_validate_provision_drive_list(fbe_raid_group_create_req_p,
            &job_queue_element_p->error_code);
    if (status != FBE_STATUS_OK) 
    {
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }

    /* convert bus, enclosure, slot, input to pdo id */
    status = fbe_raid_group_create_get_physical_drive_object_id_by_location_list(fbe_raid_group_create_req_p,
            &job_queue_element_p->error_code);
    if (status != FBE_STATUS_OK) 
    {
        job_queue_element_p->status = status;
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }

    /* validate LDO info */
    status = fbe_raid_group_create_validate_physical_drive_list(fbe_raid_group_create_req_p, 
            &job_queue_element_p->error_code);
    if (status != FBE_STATUS_OK) 
    {
        job_queue_element_p->status = status;
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return status;
    }



    fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK,
            packet_p, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_create_validation_function()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_update_configuration_in_memory_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in memory 
 * for the raid group create request 
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_update_configuration_in_memory_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    job_service_raid_group_create_t  *fbe_raid_group_create_req_p = NULL;
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_lba_t                            virtual_drive_capacity_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; 
    fbe_status_t                         status = FBE_STATUS_OK;  
    fbe_payload_control_buffer_length_t  length = 0;
    fbe_u32_t                            pvd_index = 0;
    fbe_lba_t                            minimum_virtual_drive_capacity;
    fbe_element_size_t                   element_size = 0;
    fbe_elements_per_parity_t            elements_per_parity = 0;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    for (pvd_index = 0; pvd_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH;  pvd_index++) {
     virtual_drive_capacity_array[pvd_index] = FBE_LBA_INVALID;
    }

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);
    if(job_queue_element_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,\
            "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   job_queue_element_p is NULL\n");

        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {   
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,\
            "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   incorrect lenght of job_queue_element_p provided\n");  

        fbe_raid_group_create_set_packet_status(control_operation,
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    fbe_raid_group_create_req_p = (job_service_raid_group_create_t *)job_queue_element_p->command_data;
    if (fbe_raid_group_create_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,\
            "%s entry\n",__FUNCTION__);
   
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   parameter list for raid group creation request is empty\n");
        fbe_raid_group_create_set_packet_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE, packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_start_database_transaction(&fbe_raid_group_create_req_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,\
                "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   call to create transaction id failed, status %d\n",status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return status;
    }

    /*If we are going to create a system RG ...., we won't create the vds*/
    if (fbe_raid_group_create_req_p->user_input.is_system_rg)
    {
        status = fbe_raid_group_create_system_raid_group_and_edges(fbe_raid_group_create_req_p);
        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "create system raid group failed\n");
        
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        
            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);
            return status;
        }

        
        /* complete the raid group create with good status. */
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_OK;

    }

    /* create virtual drives */
    status = fbe_create_raid_group_create_virtual_drives_and_edges(fbe_raid_group_create_req_p,
                                                                   &job_queue_element_p->error_code,
                                                                   virtual_drive_capacity_array);
    if (status != FBE_STATUS_OK) 
    {
        /*! @note This is an error level trace since we shouldn't have made it to the
         *        create code if there was insufficient capacity etc.
         */
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s VD create failed. ERR: %d status: 0x%x\n",
                          __FUNCTION__, job_queue_element_p->error_code, status);

        job_queue_element_p->status = status;

        /*! @note error_code is populated from call to create virtual drive and
         *        edges.
         */
        if (job_queue_element_p->error_code == FBE_JOB_SERVICE_ERROR_NO_ERROR)
        {
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        }

        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                                                packet_p, FBE_STATUS_OK);
        return status;
    }

    /* extent call will fill the vd capacity array */
    status = fbe_create_raid_group_create_get_extent_size(fbe_raid_group_create_req_p,
            virtual_drive_capacity_array);

    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,\
                "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "  could not obtain extent size\n");

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return status;
    }

    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "    current capacity of VD object %d is 0x%llx\n",
                fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index],
                (unsigned long long)virtual_drive_capacity_array[pvd_index]);
    }

    /* if any extent size is 0, we can not create another edge */
    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++)
    {
        if (virtual_drive_capacity_array[pvd_index] == 0)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n",__FUNCTION__);

            job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "VD object %d create failes without enough capacity.\n",
                              fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index]);

            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;

            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* check if user supplied Raid Group capacity */
    if (fbe_raid_group_create_req_p->user_input.capacity != FBE_LBA_INVALID) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "  Specific capacity for Raid Group create provided 0x%llx \n",
                (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);
    }

    /* Raid 10 is special case as it is built of 2 mirror under striper RG and a striper RG type */
    if (fbe_raid_group_create_req_p->user_input.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* create raid group object */
        status = fbe_create_raid_group_create_raid10_group(fbe_raid_group_create_req_p,
                virtual_drive_capacity_array,
                &job_queue_element_p->error_code);

        if (status != FBE_STATUS_OK) 
        {
            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);
            return status;
        }
    }
    else
    {
        /* determine the minimum virtual drive capacity, this is to be used to calculate the
         * raid group capacity, if raid group capacity is specified, the mininum virtual drive capacity will
         * be reset by the function fbe_raid_group_create_calculate_raid_group_capacity()
         */
        status = fbe_raid_group_create_calculate_raid_group_capacity(fbe_raid_group_create_req_p, 
                                                                     &minimum_virtual_drive_capacity,
                                                                     &element_size,
                                                                     &elements_per_parity,
                                                                     virtual_drive_capacity_array,
                                                                     &job_queue_element_p->error_code);

        if (status != FBE_STATUS_OK)
        {
            job_queue_element_p->status = status;
            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);
            return status;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   calculated_minimum_virtual_drive_capacity 0x%llx \n",
                (unsigned long long)minimum_virtual_drive_capacity);

        /* create raid group object */
        status = fbe_create_raid_group_create_raid_group(fbe_raid_group_create_req_p,
                virtual_drive_capacity_array, 
                element_size,
                elements_per_parity,
                &fbe_raid_group_create_req_p->raid_group_object_id);

        if (status != FBE_STATUS_OK) {
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);
            return status;
        }

        /* create edges between raid group and virtual drives */
        status = fbe_create_raid_group_create_raid_group_edges_to_virtual_drive(fbe_raid_group_create_req_p, 
                fbe_raid_group_create_req_p->raid_group_object_id,
                minimum_virtual_drive_capacity);

        if (status != FBE_STATUS_OK) {
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);
            return status;
        }
    }

    /* complete the raid group create with good status. */
    fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK,
            packet_p, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_raid_group_create_update_configuration_in_memory_function()
 ********************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_persist_configuration_db_function()
 ****************************************************************
 * @brief
 * This function is used to update the configuration in database 
 * for the Raid Group create request 
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_persist_configuration_db_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    fbe_status_t                         status = FBE_STATUS_OK;
    job_service_raid_group_create_t  *fbe_raid_group_create_req_p = NULL;
    fbe_payload_control_buffer_length_t  length = 0;
    fbe_job_queue_element_t              *job_queue_element_p = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    fbe_raid_group_create_req_p = (job_service_raid_group_create_t *)job_queue_element_p->command_data;

    status = fbe_create_lib_commit_database_transaction(fbe_raid_group_create_req_p->transaction_id);
    if(status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Complete the packet so that the job service thread can continue */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK,
            packet_p, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_create_persist_configuration_db_function()
 **********************************************************/


/*!**************************************************************
 * fbe_raid_group_create_rollback_function()
 ****************************************************************
 * @brief
 * This function is used to rollback for the Raid Group create request 
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_rollback_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_status_t         payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_operation_t      *control_operation = NULL;    
    job_service_raid_group_create_t  *fbe_raid_group_create_req_p = NULL;
    fbe_payload_control_buffer_length_t  length = 0;
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_notification_info_t              notification_info;
    fbe_status_t                         status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    fbe_raid_group_create_req_p = (job_service_raid_group_create_t *)job_queue_element_p->command_data;
    if (fbe_raid_group_create_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "rg_create_rollback_fun, param list for rg create request is empty,stat %d",
                status);
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_VALIDATE)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "rg_create_rollback_fun, rollback not required, pre state %d, cur state %d\n", 
                job_queue_element_p->previous_state,
                job_queue_element_p->current_state);
    }

    if ((job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_PERSIST)||
            (job_queue_element_p->previous_state == FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY))
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "rg_create_rollback_fun: aborting transaction id %llu\n",
                (unsigned long long)fbe_raid_group_create_req_p->transaction_id);

        status = fbe_create_lib_abort_database_transaction(fbe_raid_group_create_req_p->transaction_id);
        if(status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "rg_create_rollback_fun:cannot abort transaction %llu with cfg service\n", 
                    (unsigned long long)fbe_raid_group_create_req_p->transaction_id);

            fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                    packet_p, FBE_STATUS_OK);
            return status;
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
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "rg_create_rollback_fun: job status notification failed, status: 0x%X\n",
                status);
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_raid_group_create_set_packet_status(control_operation, payload_status,
            packet_p, FBE_STATUS_OK);
    return status;
}
/***********************************************
 * end fbe_raid_group_create_rollback_function()
 ***********************************************/


/*!**************************************************************
 * fbe_raid_group_create_commit_function()
 ****************************************************************
 * @brief
 * This function is used to committ the Raid Group create request 
 *
 * @param packet_p - Incoming packet pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 *  11/15/2012 - Modified by Vera Wang
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_commit_function (fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_status_t        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_operation_t     *control_operation = NULL;    
    job_service_raid_group_create_t *fbe_raid_group_create_req_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_job_queue_element_t             *job_queue_element_p = NULL;
    fbe_notification_info_t             notification_info;
    fbe_status_t                        status = FBE_STATUS_OK;
    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if(job_queue_element_p == NULL)
    {
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the parameters passed to the validation function */
    fbe_raid_group_create_req_p = (job_service_raid_group_create_t *)job_queue_element_p->command_data;
    if (fbe_raid_group_create_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, parameter list for raid group create request is empty, status %d", 
                __FUNCTION__, status);
        fbe_raid_group_create_set_packet_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE,
                packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry, job number 0x%llx, rg_id=0x%x, status 0x%x, error_code %d\n",
        __FUNCTION__,
        job_queue_element_p->job_number, 
        fbe_raid_group_create_req_p->raid_group_object_id,
        job_queue_element_p->status,
        job_queue_element_p->error_code);

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    job_queue_element_p->object_id = fbe_raid_group_create_req_p->raid_group_object_id;
    job_queue_element_p->need_to_wait = fbe_raid_group_create_req_p->user_input.wait_ready;
    job_queue_element_p->timeout_msec = fbe_raid_group_create_req_p->user_input.ready_timeout_msec;

    /* Send a notification for this commit*/
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.notification_data.job_service_error_info.object_id = fbe_raid_group_create_req_p->raid_group_object_id;
    notification_info.notification_data.job_service_error_info.status = FBE_STATUS_OK;
    notification_info.notification_data.job_service_error_info.error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    notification_info.notification_data.job_service_error_info.job_number = job_queue_element_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = job_queue_element_p->job_type;
    notification_info.class_id = FBE_CLASS_ID_INVALID;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

    /*! if upper_layer(user) choose to wait for RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that upper_layer(user) can update RG/LUN only depending on job notification without worrying about lifecycle state.
    */
    if(fbe_raid_group_create_req_p->user_input.wait_ready) {
        status = fbe_job_service_wait_for_expected_lifecycle_state(fbe_raid_group_create_req_p->raid_group_object_id, 
                                                                  FBE_LIFECYCLE_STATE_READY, 
                                                                  fbe_raid_group_create_req_p->user_input.ready_timeout_msec);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s timeout for waiting rg object id %d be ready. status %d\n", 
                              __FUNCTION__, fbe_raid_group_create_req_p->raid_group_object_id, status);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
            payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
            fbe_raid_group_create_set_packet_status(control_operation, payload_status,
                                                    packet_p, FBE_STATUS_OK);
            return status;
        }
    }

    status = fbe_notification_send(fbe_raid_group_create_req_p->raid_group_object_id, notification_info);
    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: job status notification failed, status: 0x%X\n",
                __FUNCTION__, status);
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_raid_group_create_set_packet_status(control_operation, payload_status,
            packet_p, FBE_STATUS_OK);
    return status;
}
/*********************************************
 * end fbe_raid_group_create_commit_function()
 *********************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_pvd_object_list()
 ****************************************************************
 * @brief
 * This function obtains all the object ids from the list of bus,
 * enclosure, slot provided for each PVD
 *
 * @param object_id IN -  fbe_raid_group_create_req_p
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/06/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_get_pvd_object_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code)
{
    fbe_u32_t    drive_index; 
    fbe_status_t status = FBE_STATUS_OK;
    fbe_topology_control_get_provision_drive_id_by_location_t provision_drive_id_by_location;
    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry\n", __FUNCTION__);

    /* get PVD ids */
    for (drive_index = 0;  drive_index < fbe_raid_group_create_req_p->user_input.drive_count; drive_index++)
    {
        provision_drive_id_by_location.object_id = FBE_OBJECT_ID_INVALID;

        /* set bus value */
        provision_drive_id_by_location.port_number = 
            fbe_raid_group_create_req_p->user_input.disk_array[drive_index].bus;

        /* set enclosure value */
        provision_drive_id_by_location.enclosure_number = 
            fbe_raid_group_create_req_p->user_input.disk_array[drive_index].enclosure;

        /* set slot value */
        provision_drive_id_by_location.slot_number =
            fbe_raid_group_create_req_p->user_input.disk_array[drive_index].slot;

        status = fbe_create_lib_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION,
                                                     &provision_drive_id_by_location,
                                                     sizeof(fbe_topology_control_get_provision_drive_id_by_location_t),
                                                     FBE_PACKAGE_ID_SEP_0,                                                     
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

        if (provision_drive_id_by_location.object_id == FBE_OBJECT_ID_INVALID)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   could not determine PVD object id for port %d, encl %d, slot %d\n", 
                provision_drive_id_by_location.port_number,
                provision_drive_id_by_location.enclosure_number,
                provision_drive_id_by_location.slot_number);

            *error_code = FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION;
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "   could not obtain PVD id by location, for for port %d, encl %d, slot %d, status %d\n", 
                    provision_drive_id_by_location.port_number,
                    provision_drive_id_by_location.enclosure_number,
                    provision_drive_id_by_location.slot_number,
                    status);
            *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            return status;
        }
        else
        {
            fbe_raid_group_create_req_p->provision_drive_id_array[drive_index] = 
                provision_drive_id_by_location.object_id;

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "   PVD object id %d, for port %d, encl %d, slot %d\n", 
                    fbe_raid_group_create_req_p->provision_drive_id_array[drive_index],
                    provision_drive_id_by_location.port_number,
                    provision_drive_id_by_location.enclosure_number,
                    provision_drive_id_by_location.slot_number);

        }
    }
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_create_get_pvd_object_list()
 ************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_physical_drive_object_id_by_location_list()
 ****************************************************************
 * @brief
 * This function obtains all the physical locations from the list of bus,
 * enclosure, slot provided for each PVD
 *
 * @param object id -  fbe_raid_group_create_req_p
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/16/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_get_physical_drive_object_id_by_location_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code)
{
    fbe_topology_control_get_physical_drive_by_location_t       topology_control_get_drive_by_location;
    fbe_u32_t                                                   drive_index; 
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry\n", __FUNCTION__);
    
    /* get PVD ids */
    for (drive_index = 0;  drive_index < fbe_raid_group_create_req_p->user_input.drive_count; drive_index++)
    {
        topology_control_get_drive_by_location.port_number = 
            fbe_raid_group_create_req_p->user_input.disk_array[drive_index].bus;

        topology_control_get_drive_by_location.enclosure_number = 
            fbe_raid_group_create_req_p->user_input.disk_array[drive_index].enclosure;

        topology_control_get_drive_by_location.slot_number = 
            fbe_raid_group_create_req_p->user_input.disk_array[drive_index].slot;

        topology_control_get_drive_by_location.physical_drive_object_id = FBE_OBJECT_ID_INVALID;
        
        status = fbe_create_lib_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                     &topology_control_get_drive_by_location,
                                                     sizeof(fbe_topology_control_get_physical_drive_by_location_t),
                                                     FBE_PACKAGE_ID_PHYSICAL,                                           
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry\n", __FUNCTION__);

            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   could not obtain physical object id, for for port %d, encl %d, slot %d, status %d\n", 
                topology_control_get_drive_by_location.port_number,
                topology_control_get_drive_by_location.enclosure_number,
                topology_control_get_drive_by_location.slot_number,
                status);
            *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            return status;
        }
        else
        {
            fbe_raid_group_create_req_p->physical_drive_id_array[drive_index] = 
               topology_control_get_drive_by_location.physical_drive_object_id;

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "   physical drive object id %d, for port %d, encl %d, slot %d\n", 
                    fbe_raid_group_create_req_p->physical_drive_id_array[drive_index],
                    topology_control_get_drive_by_location.port_number,
                    topology_control_get_drive_by_location.enclosure_number,
                    topology_control_get_drive_by_location.slot_number);
        }
    }
    return status;
}
/***************************************************************************
 * end fbe_raid_group_create_get_physical_drive_object_id_by_location_list()
 **************************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_determine_virtual_drive_capacity()
 ****************************************************************
 * @brief
 * This function use the raid group create request to determine
 * the capacity required for the virtual drive.
 *
 * @param fbe_raid_group_create_req_p - data structure which 
 * contains raid group creation parameters, it also contains the
 * array of provision drives
 * @param virtual_drive_exported_capacity_p - Pointer to virtual
 *         drive capacity.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/15/2012  Ron Proulx  - Created.
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_determine_virtual_drive_capacity(job_service_raid_group_create_t *fbe_raid_group_create_req_p,
                                                                           fbe_lba_t *virtual_drive_exported_capacity_p)
{
    fbe_u32_t     pvd_index;
    fbe_lba_t     current_virtual_drive_exported_capacity = FBE_LBA_INVALID;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__); 

    *virtual_drive_exported_capacity_p = current_virtual_drive_exported_capacity;

    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++)
    {
        /* Locate the lowest capacity provisioned drive.
         */
        if (fbe_raid_group_create_req_p->pvd_exported_capacity_array[pvd_index] < current_virtual_drive_exported_capacity)
        {
            current_virtual_drive_exported_capacity = fbe_raid_group_create_req_p->pvd_exported_capacity_array[pvd_index];
        }
    }

    /* If something went wrong report an error.
     */
    if (current_virtual_drive_exported_capacity == FBE_LBA_INVALID)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Failed for job number: %d raid group id: %d raid type: %d\n", 
                          __FUNCTION__, (int)fbe_raid_group_create_req_p->user_input.job_number,
                          fbe_raid_group_create_req_p->user_input.raid_group_id,
                          fbe_raid_group_create_req_p->user_input.raid_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Update the out and return success.
     */
    *virtual_drive_exported_capacity_p = current_virtual_drive_exported_capacity;

    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_create_raid_group_determine_virtual_drive_capacity()
 ***************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_virtual_drives_and_edges()
 ****************************************************************
 * @brief
 * This function creates all virtual drives for a raid group 
 * and attaches edges beyween the virtual drive objects passed
 * in virtual array and the corresponding pvd objects
 *
 * @param fbe_raid_group_create_req_p - data structure which 
 * contains raid group creation parameters, it also contains the
 * array of provision drives
 * @param error_code_p - If this method fails this gives more information
 * @param virtual_drive_exported_capacity - array of virtual drives capacities
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/06/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_virtual_drives_and_edges(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t *error_code_p,
        fbe_lba_t virtual_drive_exported_capacity_array[])
{
    fbe_u32_t     pvd_index;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_lba_t     virtual_drive_imported_capacity = FBE_LBA_INVALID;
    fbe_lba_t     virtual_drive_imported_capacity_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; 
    //fbe_system_encryption_mode_t		system_encryption_mode;
	//fbe_base_config_encryption_mode_t	base_config_encryption_mode;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__); 

    /* Initialize the error code to `no error'.
     */
    *error_code_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /*! @note Determine the virtual drive capacity.  This does not include
     *        the bind offset.  It only include the raid group capacity required
     *        for each virtual drive.
     */
    status = fbe_create_raid_group_determine_virtual_drive_capacity(fbe_raid_group_create_req_p, &virtual_drive_imported_capacity);
    if (status != FBE_STATUS_OK)
    {
        *error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Determine vd capacity for trans id: 0x%llx failed. status: 0x%x\n", 
                          __FUNCTION__, (unsigned long long)fbe_raid_group_create_req_p->transaction_id, status);
        return status;
    }
    if (virtual_drive_imported_capacity == 0)
    {
        *error_code_p = FBE_JOB_SERVICE_ERROR_INSUFFICIENT_DRIVE_CAPACITY;
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "create_raid_group: create vd and edges trans id: 0x%llx min importable cap: 0x%llx\n", 
                          (unsigned long long)fbe_raid_group_create_req_p->transaction_id, (unsigned long long)virtual_drive_imported_capacity);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Based on the drives requested, determine the bind offset.
     */
    status  = fbe_create_raid_group_create_determine_if_extent_size_fits(fbe_raid_group_create_req_p,
                                                                         fbe_raid_group_create_req_p->provision_drive_id_array,
                                                                         fbe_raid_group_create_req_p->user_input.drive_count,
                                                                         &virtual_drive_imported_capacity,
                                                                         FBE_TRUE, /* Ignore the offset when detemining if extent fits*/
                                                                         error_code_p);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "create_raid_group: create vd and edges trans id: 0x%llx extent fits cap: 0x%llx\n", 
                          (unsigned long long)fbe_raid_group_create_req_p->transaction_id, (unsigned long long)virtual_drive_imported_capacity);
        return status;
    }

    /* Create the virtual drives for each position.
     */
    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++)
    {
        if (fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index] != FBE_OBJECT_ID_INVALID)
        {
            /* If there is a Virtual drive created, this means we are using a PVD--to-VD shared
             * connection
             */
            *error_code_p = FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES;
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "create_raid_group: create vd and edges trans id: 0x%llx index: %d pvd obj: 0x%x has upstream vd obj: 0x%x\n", 
                              (unsigned long long)fbe_raid_group_create_req_p->transaction_id, pvd_index,
                              fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                              fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /*! @note Else use the the provisioned drive exported capacity and
             *        the capacity consumed by the raid group to validate and 
             *        determine the virtual drive capacity.
             *
             *  @note This implies that only (1) raid group can be created per
             *        virtual drive.
             */
            status = fbe_raid_group_create_get_virtual_drive_capacity(
                        virtual_drive_imported_capacity,
                        fbe_raid_group_create_req_p->block_offset[pvd_index],
                        &virtual_drive_exported_capacity_array[pvd_index],
                        &virtual_drive_imported_capacity_array[pvd_index]);
            if (status != FBE_STATUS_OK)
            {
                *error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "create_raid_group: create vd and edges trans id: 0x%llx index: %d pvd obj: 0x%x get vd cap failed.\n", 
                              (unsigned long long)fbe_raid_group_create_req_p->transaction_id, pvd_index,
                              fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index]);
                return status;
            }

            /* the VD object is created using the PVD's exported capacity */
            status = fbe_raid_group_create_virtual_drive_object(fbe_raid_group_create_req_p->transaction_id,
                                                                virtual_drive_exported_capacity_array[pvd_index], 
                                                                fbe_raid_group_create_req_p->block_offset[pvd_index],
                                                                &fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index]);
            if (status != FBE_STATUS_OK) 
            {
                *error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "create_raid_group: create vd and edges trans id: 0x%llx index: %d pvd obj: 0x%x create vd failed.\n", 
                              (unsigned long long)fbe_raid_group_create_req_p->transaction_id, pvd_index,
                              fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index]);
                return status;
            }

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "   NEW VD object %d created with capacity 0x%llx\n", 
                    fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index],
                    (unsigned long long)virtual_drive_exported_capacity_array[pvd_index]);
        }

    } /* end create virtual drives */

    /* Determine the virtual drive capacity.
     */
    status = fbe_create_raid_group_create_get_extent_size(fbe_raid_group_create_req_p,
                                                          virtual_drive_exported_capacity_array);
    if (status != FBE_STATUS_OK)
    {
        *error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                          "create_raid_group: create vd and edges trans id: 0x%llx vd exported cap: 0x%llx get extent failed.\n", 
                          (unsigned long long)fbe_raid_group_create_req_p->transaction_id,
                          (unsigned long long)virtual_drive_exported_capacity_array[0]);
        return status;
    }

    /* Update PVD configuration. */
    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++) {
        /* update the configuration type of the pvd object as raid. */
        status = fbe_create_lib_database_service_update_pvd_config_type(fbe_raid_group_create_req_p->transaction_id, 
                fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID);
        if (status != FBE_STATUS_OK) {
            *error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, could not update the PVD config type, PVD object:%d, status %d\n",
                    __FUNCTION__, fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index], status);
            return status; 
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "     updated config type to RAID for PVD[%d] object.\n",
                fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index]);
            
#if 0 /* We will implement this ogic in different place */

        status = fbe_database_get_system_encryption_mode(&system_encryption_mode);

		if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
			/* temporary hack */
			base_config_encryption_mode = system_encryption_mode;

			/* update the configuration type of the pvd object as raid. */
			status = fbe_create_lib_database_service_update_encryption_mode(fbe_raid_group_create_req_p->transaction_id, 
					fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
					base_config_encryption_mode);
			if (status != FBE_STATUS_OK) {
				*error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
				job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
						"%s entry, could not update the PVD encryption mode, PVD object:%d, status %d\n",
						__FUNCTION__, fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index], status);
				return status; 
			}

			job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"     updated encryption mode for PVD[%d] object.\n",
					fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index]);
			
		}/* if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) */		
#endif
	}



    /* Create the edge between  PVD and VD */
    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++) {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "     edges between PVD[%d] and VD[%d] will use capacity 0x%llx\n", 
                    fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                    fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index],
                    (unsigned long long)virtual_drive_imported_capacity_array[pvd_index]);

            /* create edges between the provision drive and the virtual drive */
            status = fbe_create_lib_database_service_create_edge(fbe_raid_group_create_req_p->transaction_id, 
                    fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                    fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index],
                    0, /* client index */
                    virtual_drive_imported_capacity_array[pvd_index],
                    fbe_raid_group_create_req_p->block_offset[pvd_index], /* used in case of all drives */
                    NULL,
                    FBE_EDGE_FLAG_IGNORE_OFFSET /* ignore the offset */);

            if (status != FBE_STATUS_OK) 
            {
                *error_code_p = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s entry, could not attach edges between PVD object %d and VD object %d, status %d\n", __FUNCTION__, 
                        fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                        fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index],
                        status);
                return status; 
            }
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "     attached edges between PVD[%d] and VD[%d] \n", 
                    fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                    fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index]);
            
    } /* end create the edge between virtual drive and provision drive */

    return status;
}
/***************************************************************
 * end fbe_create_raid_group_create_virtual_drives_and_edges()
 ***************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_raid_group()
 ****************************************************************
 * @brief
 * This function creates a raid group 
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_capacity_array IN - array of virtual drives capacities
 * @param sep_raid_group_id OUT - id of newly created raid group
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/06/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_raid_group(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_element_size_t element_size,
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t *sep_raid_group_id)
{
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_object_id_t   raid_group_id = FBE_OBJECT_ID_INVALID;

    /* create the raid group */
    status = fbe_raid_group_create_raid_group_creation(fbe_raid_group_create_req_p,
                                                       element_size,
                                                       elements_per_parity,
                                                       &raid_group_id);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, could not create raid group type %d, status %d\n", __FUNCTION__, 
                fbe_raid_group_create_req_p->user_input.raid_type,
                status);
        return status;
    }

    *sep_raid_group_id = raid_group_id;
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry, Succesfully created Raid Group %d, status %d\n", __FUNCTION__, 
            *sep_raid_group_id,  status);

    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_create_raid_group_create_raid_group()
 ************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_raid10_group()
 ****************************************************************
 * @brief
 * This function creates a raid group type 10
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_capacity_array IN - array of virtual drives capacities
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/06/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_raid10_group(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code)
{
    fbe_status_t      status = FBE_STATUS_OK;

    /* if capacity is not specified, this implies the user wants to
     * consume whatever amount of free space is found on the VDs
     */
    if (fbe_raid_group_create_req_p->user_input.capacity == FBE_LBA_INVALID)
    {
        status  = fbe_create_raid_group_create_raid10_group_bottom_top_approach(fbe_raid_group_create_req_p,
                                                                                virtual_drive_capacity_array,
                                                                                error_code);
    }
    else
    {
        status  = fbe_create_raid_group_create_raid10_group_top_bottom_approach(fbe_raid_group_create_req_p,
                                                                                virtual_drive_capacity_array,
                                                                                error_code);
    }
    return status;
}

/*!**************************************************************
 * fbe_create_raid_group_create_raid10_group_top_bottom_approach()
 ****************************************************************
 * @brief
 * This function creates a raid group type 10 from top to
 * bottom, meaning user specifies capacity of RG
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_capacity_array IN - array of virtual drives capacities
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_raid10_group_top_bottom_approach(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code)
{
    fbe_object_id_t   mirror_rg_id[8];
    fbe_u32_t         mirror_width = 2, width;
    fbe_u32_t         rg_width;
    fbe_lba_t         expected_capacity = FBE_LBA_INVALID;
    fbe_lba_t         mirror_rg_capacity = FBE_LBA_INVALID;
    fbe_lba_t         edge_capacity_between_raid_groups = FBE_LBA_INVALID;
    fbe_lba_t         vd_capacity = FBE_LBA_INVALID;
    fbe_u32_t         num_mirrors = 0;
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_lba_t         striper_capacity = FBE_LBA_INVALID;
    fbe_element_size_t element_size = 0;
    fbe_elements_per_parity_t elements_per_parity = 0;

    width = fbe_raid_group_create_req_p->user_input.drive_count;
    rg_width = width / mirror_width;
    num_mirrors = width / 2;

    /* top to bottom approach 
     * 
     * user specifies capacity
     *          
     *            create RGs from to top
     *            VDs --> RGs(mirror) --> RG(Striper) 
     *                            
     *                               creation 
     *                               order
     *   |
     *   |        ()          < ---- (3) striper RG 
     *   |       /  \
     *   |      /    \        < ---- (7) Mirror to striper edges
     *   |     /      \
     *   |    ()      ()      < ---- (2) Mirror under striper RGs
     *   |    /        \
     *   |   /          \     < ---- (5) VDs to Mirror under striper edges
     *   |  /            \
     *   | ()            ()   < ---- (1) VDs - created before hand, but not commited if size does not fit
     *   |                                 or there is an issue while creating the RG
     *   |
     *   |                    steps (4) and (5) we check if the extent fits the required size
     *  \|/
     */

    /* step 1: create VDs, these were created initially by the function 
     * fbe_raid_group_create_update_configuration_in_memory_function() 
     */

    /* Step 2:
     *
     *  Create mirror under striper RGs
     *
     */

    /* save capacity and user id given by user */
    striper_capacity = fbe_raid_group_create_req_p->user_input.capacity;

    /* calculate capacity of mirrors, we know the exported capacity, ask the RG class
     * for the imported capacity. Note that imported capacity would be of a single
     * mirror RG. 
     */
    fbe_raid_group_create_req_p->user_input.raid_type = FBE_RAID_GROUP_TYPE_RAID10;
    status = fbe_raid_group_create_calculate_raid10_group_capacity(fbe_raid_group_create_req_p, 
                                                                   &element_size,
                                                                   &elements_per_parity,
                                                                   &mirror_rg_capacity);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* set the fbe_raid_group_create_req_p field values for mirror RGs */
    fbe_raid_group_create_req_p->user_input.capacity = mirror_rg_capacity;
    fbe_raid_group_create_req_p->user_input.drive_count = mirror_width;
    fbe_raid_group_create_req_p->user_input.raid_type = FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER;

    /* Create the mirrors under striper RGs */
    status = fbe_create_raid_group_create_mirror_under_striper_rgs(fbe_raid_group_create_req_p,
                                                                   num_mirrors,
                                                                   element_size, 
                                                                   elements_per_parity,
                                                                   mirror_rg_id);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* Step 3:
     *
     *  Create striper RG
     *
     */
    fbe_raid_group_create_req_p->user_input.capacity = striper_capacity;
    fbe_raid_group_create_req_p->user_input.drive_count = num_mirrors;
    fbe_raid_group_create_req_p->user_input.raid_type = FBE_RAID_GROUP_TYPE_RAID10;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg_rg10_top_bottom_appr entry,Create striper RG user id %d, with cap 0x%llx\n", 
            fbe_raid_group_create_req_p->user_input.raid_group_id,
            (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);

    status = fbe_raid_group_create_raid_group_creation(fbe_raid_group_create_req_p,
                                                       element_size,
                                                       elements_per_parity,
                                                       &fbe_raid_group_create_req_p->raid_group_object_id);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "create_rg_rg10_top_bottom_appr entry,cannot create rg type %d, stat %d\n", 
                fbe_raid_group_create_req_p->user_input.raid_type,
                status);
        return status;
    }

   job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg_rg10_top_bottom_appr entry,created striper RG id %d, user id %d, with cap 0x%llx\n", 
            fbe_raid_group_create_req_p->raid_group_object_id,
            fbe_raid_group_create_req_p->user_input.raid_group_id,
            (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);

    /* save capacity of edges between RGs, this is required because the edges between striper RG
     * and mirror rg are created after the VDs and edges to PVDs are created
     */
    edge_capacity_between_raid_groups = mirror_rg_capacity;

    /* Step 4:
     *
     *  Determine if extent between VDs and mirror under striper RG fit
     *
     */
    fbe_raid_group_create_req_p->user_input.raid_type = FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER; 
    fbe_raid_group_create_req_p->user_input.capacity = mirror_rg_capacity; 
    vd_capacity = FBE_LBA_INVALID;

    /* determine the expected capacity of the VDs */
     status = fbe_raid_group_create_calculate_raid10_group_capacity(fbe_raid_group_create_req_p, 
                                                                   &element_size,
                                                                   &elements_per_parity,
                                                                   &vd_capacity);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg_rg10_top_bottom_appr entry,vd_cap for single VD 0x%llx\n",
            (unsigned long long)vd_capacity);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "checking if extent fits between mirror under striper RGs and VDs\n");

    expected_capacity = vd_capacity;

    /* determine if minimum size fits all available free contiguous spaces in the VDs */
    status = fbe_create_raid_group_create_determine_if_extent_size_fits(fbe_raid_group_create_req_p,
                                                                        fbe_raid_group_create_req_p->virtual_drive_id_array,
                                                                        width,
                                                                        &vd_capacity,
                                                                        FBE_FALSE, /* Don't ignore offset*/
                                                                        error_code);                                                             
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    /* if we can only fit in a smaller capacity, we need to failed the request */
    if (vd_capacity!=expected_capacity)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Step 5:
     *
     *  create edges between VDs and mirror under striper RG
     *
     */
    status = fbe_create_raid_group_create_edges_between_mirror_and_vd(fbe_raid_group_create_req_p,
                                                                      width,  
                                                                      vd_capacity, 
                                                                      mirror_rg_id);
    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* Step 6:
     *
     *  Determine if extent between mirror under striper and striper RG fits
     *
     */
    /* determine if minimum size fits all available free contiguous spaces in the Mirror under striper RGs */

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "checking if extent fits between mirror under striper RGs and striper RG\n");

    expected_capacity = mirror_rg_capacity;
    status = fbe_create_raid_group_create_determine_if_extent_size_fits(fbe_raid_group_create_req_p,
                                                                        mirror_rg_id,
                                                                        num_mirrors,
                                                                        &mirror_rg_capacity,
                                                                        FBE_FALSE, /* Don't ignore offset*/
                                                                        error_code);
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }
     /* if we can only fit in a smaller capacity, we need to failed the request */
    if (mirror_rg_capacity!=expected_capacity)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Step 7:
     *
     *  create edges between mirror under striper RG and striper RG
     *
     */
    status = fbe_create_raid_group_create_edges_between_striper_and_mirror(fbe_raid_group_create_req_p,
                                                                           num_mirrors,
                                                                           edge_capacity_between_raid_groups,
                                                                           mirror_rg_id);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg_rg10_top_bottom_appr entry, Succesfully created Raid Group %d, status %d\n", 
            fbe_raid_group_create_req_p->raid_group_object_id,  status);

    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_create_raid_group_create_raid10_group_top_bottom_approach()
 ********************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_raid10_group_bottom_top_approach()
 ****************************************************************
 * @brief
 * This function creates a raid group type 10 from bottom to
 * top, meaning user does not specify capacity of RG
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_capacity_array IN - array of virtual drives capacities
 * @param sep_raid_group_id OUT - id of newly created raid group

 * @return status - The status of the operation.
 *
 * @author
 *  06/03/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_raid10_group_bottom_top_approach(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code)
{
    fbe_object_id_t   mirror_rg_id[8];
    fbe_u32_t         mirror_width = 2, width;
    fbe_u32_t         rg_width;
    fbe_lba_t         mirror_rg_capacity = FBE_LBA_INVALID;
    fbe_u32_t         num_mirrors = 0;
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_lba_t         minimum_virtual_drive_capacity = FBE_LBA_INVALID;
    fbe_element_size_t element_size = 0;
    fbe_elements_per_parity_t elements_per_parity = 0;

    width = fbe_raid_group_create_req_p->user_input.drive_count;
    rg_width = width / mirror_width;
    num_mirrors = width / 2;

    /* bottom to top approach 
     * 
     * user does not specify capacity, which means we will consume all available
     * VD space
     *          
     *            create RGs from bottom to top
     *            VDs --> RGs(mirror) --> RG(Striper) 
     *                            
     *                               creation 
     *                               order
     *  /|\
     *   |        ()          < ---- (3) striper RG 
     *   |       /  \
     *   |      /    \        < ---- (7) Mirror to striper edges
     *   |     /      \
     *   |    ()      ()      < ---- (2) Mirror under striper RGs
     *   |    /        \
     *   |   /          \     < ---- (5) VDs to Mirror under striper edges
     *   |  /            \
     *   | ()            ()   < ---- (1) VDs - created before hand, but not commited if size does not fit
     *   |                                 or there is an issue while creating the RG
     *   |
     *   |                    steps (4) and (5) we check if the extent fits the required size
     *   |
     */

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg10_bottom_top_appr, create R10 with cap LBA_INVALID,consume whole PVDs for creation\n");

    /* step 1: create VDs, these were created initially by the function 
     * fbe_raid_group_create_update_configuration_in_memory_function() 
     */

    /* Step 2:
     *
     *  Create mirror under striper RGs
     *
     */

    /* set raid type to mirror under stripper since we are configuring RG from mirrors up */
    fbe_raid_group_create_req_p->user_input.raid_type = FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER;

    /* call below will fill fbe_raid_group_create_req_p->user_input.capacity based on smallest VD size available
     * this capacity is the capacity to be used to create the mirror under striper RGs
     * Note that the smallest VD will be used to determine the RG capacity, and the smallest
     * VD may be part of another mirror RG in the configuration.
     */
    status = fbe_raid_group_create_calculate_raid_group_capacity(fbe_raid_group_create_req_p, 
                                                                 &minimum_virtual_drive_capacity,
                                                                 &element_size,
                                                                 &elements_per_parity,
                                                                 virtual_drive_capacity_array,
                                                                 error_code);

    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg10_bottom_top_appr, capacity of each mirror RG 0x%llx\n",
            (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);
    
    /* remember mirror capacity, this is needed when we need to feed the capacity to the RG class
     * to calculate the striper RG size base on the mirror capacity
    */
    mirror_rg_capacity = fbe_raid_group_create_req_p->user_input.capacity;

    status = fbe_create_raid_group_create_mirror_under_striper_rgs(fbe_raid_group_create_req_p,
                                                                   num_mirrors,
                                                                   element_size, 
                                                                   elements_per_parity,
                                                                   mirror_rg_id);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* Step 3:
     *
     *  Create striper RG
     *
     */
    fbe_raid_group_create_req_p->user_input.capacity = FBE_LBA_INVALID;
    fbe_raid_group_create_req_p->user_input.raid_type = FBE_RAID_GROUP_TYPE_RAID10;
    status = fbe_raid_group_create_calculate_raid10_group_capacity(fbe_raid_group_create_req_p, 
                                                                   &element_size,
                                                                   &elements_per_parity,
                                                                   &mirror_rg_capacity);

    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    fbe_raid_group_create_req_p->user_input.drive_count = num_mirrors;
    status = fbe_raid_group_create_raid_group_creation(fbe_raid_group_create_req_p,
                                                       element_size,
                                                       elements_per_parity,
                                                       &fbe_raid_group_create_req_p->raid_group_object_id);


    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "create_rg10_bottom_top_appr entry, could not create raid group type %d, status %d\n", 
                fbe_raid_group_create_req_p->user_input.raid_type,
                status);
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "Created striper RG id %d,  id %d, with capacity 0x%llx\n",
            fbe_raid_group_create_req_p->raid_group_object_id,
            fbe_raid_group_create_req_p->user_input.raid_group_id,
            (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);


    /* Step 5:
     *
     *  create edges between VDs and mirror under striper RG
     *
     */
    status = fbe_create_raid_group_create_edges_between_mirror_and_vd(fbe_raid_group_create_req_p,
                                                                      width,
                                                                      minimum_virtual_drive_capacity,
                                                                      mirror_rg_id);
    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* Step 6:
     *
     *  Determine if extent between mirror under striper and striper RG fits
     *
     */
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "checking if extent fits between mirror under striper RGs and striper RG\n");

    status = fbe_create_raid_group_create_determine_if_extent_size_fits(fbe_raid_group_create_req_p,
                                                                        mirror_rg_id,
                                                                        num_mirrors,
                                                                        &mirror_rg_capacity,
                                                                        FBE_FALSE, /* Don't ignore offset*/
                                                                        error_code);
    if (status != FBE_STATUS_OK) 
    {
        return status;
    }
 
    /* Step 7:
     *
     *  create edges between mirror under striper RG and striper RG
     *
     */
    status = fbe_create_raid_group_create_edges_between_striper_and_mirror(fbe_raid_group_create_req_p,
                                                                           num_mirrors,
                                                                           mirror_rg_capacity,
                                                                           mirror_rg_id);
    if (status != FBE_STATUS_OK)
    {
        *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "create_rg10_bottom_top_appr entry, Success create RG %d, status %d\n", 
            fbe_raid_group_create_req_p->raid_group_object_id,  status);

    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_create_raid_group_create_raid10_group_bottom_top_approach()
 ********************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_mirror_under_striper_rgs()
 ****************************************************************
 * @brief
 * This function creates the mirror under striper RGs
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters

 * @return status - The status of the operation.
 *
 * @author
 *  07/07/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_mirror_under_striper_rgs(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_u32_t         num_mirrors,
        fbe_element_size_t element_size, 
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t   mirror_rg_id[])
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t         i;

    /* Create the mirrors under striper RGs */
    for (i = 0; i < num_mirrors; i++)
    {
        mirror_rg_id[i] = FBE_OBJECT_ID_INVALID;

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "create_rg_mirror_under_striper_rg entry, Create mirror in striper RG%d with cap 0x%llx\n",
                i, (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);

        /* create all mirror under stripper raid groups */
        status = fbe_raid_group_create_raid_group_creation(fbe_raid_group_create_req_p,
                                                           element_size,
                                                           elements_per_parity,
                                                           &mirror_rg_id[i]);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "create_rg_mirror_under_striper_rg entry, could not create rg type %d, stat %d\n",
                    FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER,
                    status);
            return status;
        }
    }
    return status;
}
/*!**************************************************************
 * fbe_create_raid_group_create_edges_between_striper_and_mirror()
 ****************************************************************
 * @brief
 * This function creates the edges between a striper Rg and
 * Mirror RGs
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_capacity_array IN - array of virtual drives capacities
 * @param sep_raid_group_id OUT - id of newly created raid group

 * @return status - The status of the operation.
 *
 * @author
 *  07/06/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_edges_between_striper_and_mirror(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_u32_t         num_mirrors,
        fbe_lba_t         mirror_rg_capacity,
        fbe_object_id_t   mirror_rg_id[])
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t         i;

    /*  connect the striper to the mirror RGs*/
    for (i = 0; i < num_mirrors; i++)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry\n",__FUNCTION__);
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "  Creating edge from mirror_rg[%d](s) to striper_rg[%d](c) with capacity 0x%llx, offset %d\n", 
                mirror_rg_id[i],
                fbe_raid_group_create_req_p->raid_group_object_id,
                (unsigned long long)mirror_rg_capacity,
                0);

        /* create edges between mirror and striper */
        status = fbe_create_lib_database_service_create_edge(fbe_raid_group_create_req_p->transaction_id, 
                                                             mirror_rg_id[i], 
                                                             fbe_raid_group_create_req_p->raid_group_object_id, 
                                                             i, /* index */ 
                                                             mirror_rg_capacity, 
                                                             0 /* not used */,
                                                             NULL,
                                                             0 /* not used */);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, could not attach edges between object %d and object %d, status %d\n", 
                    __FUNCTION__, mirror_rg_id[i], fbe_raid_group_create_req_p->raid_group_object_id, status);

            return status;
        }
    }
    return status;
}

/*!**************************************************************
 * fbe_create_raid_group_create_edges_between_mirror_and_vd()
 ****************************************************************
 * @brief
 * This function creates the edges between mirror RGs and
 * its corresponding VDs
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_capacity_array IN - array of virtual drives capacities
 * @param sep_raid_group_id OUT - id of newly created raid group

 * @return status - The status of the operation.
 *
 * @author
 *  07/06/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_edges_between_mirror_and_vd(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_u32_t         width,
        fbe_lba_t         edge_capacity,
        fbe_object_id_t   mirror_rg_id[])
{
    fbe_object_id_t   vd_object_id;
    fbe_u32_t         i, vd_index;
    fbe_status_t      status = FBE_STATUS_OK;
    
    vd_index = 0;
    vd_object_id = fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index];

    for (i = 0; i < width / 2; ++i)
    {
        status = fbe_create_lib_database_service_create_edge(fbe_raid_group_create_req_p->transaction_id, 
                                                             vd_object_id, 
                                                             mirror_rg_id[i], 
                                                             0, /* index */ 
                                                             edge_capacity, 
                                                             fbe_raid_group_create_req_p->block_offset[vd_index],
                                                             NULL,
                                                             0 /* not used*/);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, could not attach edges between object %d and object %d, status %d\n", 
                    __FUNCTION__, fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index], mirror_rg_id[i], status);

            return status;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "    Created edge from vd object[%d] (server) to mirror under striper rg [%d] (client)\n",
                vd_object_id,
                mirror_rg_id[i]);

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "    with capacity 0x%llx, offset 0x%llx\n",
                (unsigned long long)edge_capacity,
                (unsigned long long)fbe_raid_group_create_req_p->block_offset[vd_index]);

        vd_index++;
        vd_object_id = fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index];

        /* create edges between mirror and virtual drives */
        status = fbe_create_lib_database_service_create_edge(fbe_raid_group_create_req_p->transaction_id, 
                                                             vd_object_id, 
                                                             mirror_rg_id[i], 
                                                             1, /* index */ 
                                                             edge_capacity, 
                                                             fbe_raid_group_create_req_p->block_offset[vd_index],
                                                             NULL,
                                                             0 /* not used*/);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, could not attach edges between object %d and object %d, status %d\n", 
                    __FUNCTION__, fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index], mirror_rg_id[i], status);

            return status;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "    Created edge from vd object[%d] (server) to mirror under striper rg [%d] (client)\n",
                vd_object_id,
                mirror_rg_id[i]);

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "    with capacity 0x%llx, offset 0x%llx\n",
                (unsigned long long)edge_capacity,
                (unsigned long long)fbe_raid_group_create_req_p->block_offset[vd_index]);

        vd_index++;
        vd_object_id = fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index];
    }
    return status;
}


/*!**************************************************************
 * fbe_create_raid_group_create_get_extent_size()
 ****************************************************************
 * @brief
 * This function calls the base config to determine the
 * largest contiguos free capacity of an object
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param virtual_drive_exported_capacity OUT- array of virtual drives capacities
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/25/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_get_extent_size(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_lba_t       virtual_drive_capacity_array[])
{
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_block_transport_control_get_max_unused_extent_size_t  max_extent_size;
    fbe_u32_t        index; 

    /* create edges between Raid group object and virtual drives */
    for (index = 0; index < fbe_raid_group_create_req_p->user_input.drive_count; index++)
    {
        max_extent_size.extent_size = FBE_LBA_INVALID;

        status = fbe_create_lib_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE,
                                                     &max_extent_size,
                                                     sizeof(fbe_block_transport_control_get_max_unused_extent_size_t),
                                                     FBE_PACKAGE_ID_SEP_0,                                                     
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     fbe_raid_group_create_req_p->virtual_drive_id_array[index],
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, could not obtain extent size for object %d, status %d\n", __FUNCTION__, 
                    fbe_raid_group_create_req_p->virtual_drive_id_array[index],
                    status);
            return status;
        }

        virtual_drive_capacity_array[index] = max_extent_size.extent_size;
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "largest extent size for object %d, is 0x%llx\n", 
                fbe_raid_group_create_req_p->virtual_drive_id_array[index],
                (unsigned long long)virtual_drive_capacity_array[index]);
    }

    return status;
}

/*!**************************************************************
 * fbe_raid_group_create_get_unused_provision_drive_extent_size()
 ****************************************************************
 * @brief
 * This function calls the base config to determine the
 * largest contiguos free capacity of an object
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 *        contains raid group creation parameters
 * @param provision_drive_exported_capacity_p OUT- pointer to provision drive exported capacity
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/23/2012 - Created. Ashwin Tamilarasan
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_get_unused_provision_drive_extent_size(
                                        fbe_object_id_t       pvd_object_id,
                                        fbe_lba_t             *provision_drive_exported_capacity_p)
{
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_block_transport_control_get_max_unused_extent_size_t  max_extent_size;

    max_extent_size.extent_size = FBE_LBA_INVALID;

    status = fbe_create_lib_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE,
                                                 &max_extent_size,
                                                 sizeof(fbe_block_transport_control_get_max_unused_extent_size_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                     
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 pvd_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, could not obtain extent size for object %d, status %d\n", __FUNCTION__, 
                pvd_object_id, status);
        return status;
    }

    *provision_drive_exported_capacity_p = max_extent_size.extent_size;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "largest extent size for object %d, is 0x%llx\n", 
            pvd_object_id, (unsigned long long)*provision_drive_exported_capacity_p);
    

    return status;

}
/********************************************************************
 * end fbe_raid_group_create_get_unused_provision_drive_extent_size()
 ********************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_determine_if_extent_size_fits()
 ****************************************************************
 * @brief
 * This function calls the base config to determine if the
 * largest contiguos free capacity of an object fits the
 * expected size for given object
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param minimum_object_capacity IN/OUT - minimum capacity (available)
 * @param b_ignore_offset - Ignore offset when checking if capacity fits
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/07/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_determine_if_extent_size_fits(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_object_id_t object_array[],
        fbe_u32_t      object_array_count,
        fbe_lba_t       *minimum_object_capacity,
        fbe_bool_t  b_ignore_offset,
        fbe_job_service_error_type_t  *error_code)
{
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_u32_t         index; 
    fbe_lba_t         max_offset  = 0;
    fbe_lba_t         available_capacity  = 0;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    // find the biggest offset with the first fit placement 
    for (index = 0; index < object_array_count; index++)
    {
        fbe_raid_group_create_req_p->block_offset[index] = max_offset;
        /* is the space available? get the starting offset and client index */
        status = fbe_create_lib_base_config_validate_capacity (object_array[index],
                                                               *minimum_object_capacity,
                                                               FBE_BLOCK_TRANSPORT_FIRST_FIT,
                                                               b_ignore_offset,
                                                               &fbe_raid_group_create_req_p->block_offset[index], 
                                                               &fbe_raid_group_create_req_p->client_index[index],
                                                               &available_capacity);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "create_raid_group: client %d(%d) can not support capacity 0x%llx\n", 
                            fbe_raid_group_create_req_p->client_index[index], index,
                            (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity);
            *error_code = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;
            fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY, NULL, 0, 
                                " ");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (fbe_raid_group_create_req_p->block_offset[index]>max_offset)
        {
            /* if one client needs to start at a bigger offset,
             * everyone else has to agree with it.
             */
            max_offset = fbe_raid_group_create_req_p->block_offset[index];
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "create_raid_group: client %d(%d) select offset 0x%llx\n", 
                            fbe_raid_group_create_req_p->client_index[index], index,
                            (unsigned long long)max_offset);
        }
    }

    // now let's place it with the FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION
    // find the biggest offset with the first fit placement 
    for (index = 0; index < object_array_count; index++)
    {
        fbe_raid_group_create_req_p->block_offset[index] = max_offset;
        /* is the space available? get the starting offset and client index */
        status = fbe_create_lib_base_config_validate_capacity (object_array[index],
                                                               *minimum_object_capacity,
                                                               FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION,
                                                               b_ignore_offset,
                                                               &fbe_raid_group_create_req_p->block_offset[index], 
                                                               &fbe_raid_group_create_req_p->client_index[index],
                                                               minimum_object_capacity);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "create_raid_group: client %d(%d) can not support capacity 0x%llx at offset 0x%llx, available 0x%llx\n", 
                            fbe_raid_group_create_req_p->client_index[index], index,
                            (unsigned long long)fbe_raid_group_create_req_p->user_input.capacity,
                (unsigned long long)max_offset,
                (unsigned long long)(*minimum_object_capacity));
            if (*minimum_object_capacity==0)
            {
                // failed the request if nothing fits
                *error_code = FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY;
                fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY, NULL, 0, 
                                    " ");
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_create_raid_group_create_determine_if_extent_size_fits()
 *****************************************************************/

/*!**************************************************************
 * fbe_create_raid_group_create_raid_group_edges_to_virtual_drive()
 ****************************************************************
 * @brief
 * This function attaches edges from virtual drive objects
 * to a raid group 
 *
 * @param fbe_raid_group_create_req_p IN - data structure which 
 * contains raid group creation parameters
 * @param sep_raid_group_id IN - id of raid group
 * @param minimum_virtual_drive_capacity IN - edge size 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/08/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_create_raid_group_create_raid_group_edges_to_virtual_drive(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_object_id_t sep_raid_group_id,
        fbe_lba_t       minimum_virtual_drive_capacity)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t    vd_index;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry\n", __FUNCTION__);

    /* create edges between Raid group object and virtual drives */
    for (vd_index = 0; vd_index < fbe_raid_group_create_req_p->user_input.drive_count; vd_index++)
    {
        /* create edges between the provision drive and the virtual drive */
        status = fbe_create_lib_database_service_create_edge(fbe_raid_group_create_req_p->transaction_id, 
                                                             fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index], 
                                                             sep_raid_group_id, 
                                                             vd_index, 
                                                             minimum_virtual_drive_capacity, 
                                                             fbe_raid_group_create_req_p->block_offset[vd_index],
                                                             NULL,
                                                             0 /*not used */);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry\n",__FUNCTION__);

            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   could not attach edges between object %d and object %d, status %d\n", 
                fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index], sep_raid_group_id, status);

            return status;
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   attached edges from RG[client] %d to VD[server] %d with capacity 0x%llx\n", 
                sep_raid_group_id, 
                fbe_raid_group_create_req_p->virtual_drive_id_array[vd_index], 
                (unsigned long long)minimum_virtual_drive_capacity);
        }
    }

    if (status == FBE_STATUS_OK) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",__FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "Succesfully attached all edges to raid group %d, status %d\n", 
            sep_raid_group_id, status);

    }

    return FBE_STATUS_OK;

}
/*********************************************************************
 * end fbe_create_raid_group_create_raid_group_edges_to_virtual_drive()
 ********************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_raid_group_creation()
 ****************************************************************
 * @brief
 * This function creates a raid group
 *
 * @param rg_crete_ptr IN - structure with all data
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/28/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_raid_group_creation(
        job_service_raid_group_create_t * rg_create_ptr,
        fbe_element_size_t element_size,
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t *object_id)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_database_control_raid_t create_raid;

    /* for private RGs we must set the private_rg flag */
    if (rg_create_ptr->user_input.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        create_raid.private_raid                           = FBE_TRUE;
        create_raid.raid_id                                = FBE_RAID_ID_INVALID; /* value is ignored by config */
        create_raid.raid_configuration.width               = 2;
        create_raid.class_id                               = FBE_CLASS_ID_MIRROR;
    }
    else
    {
        create_raid.private_raid                           = FBE_FALSE;
        create_raid.raid_id                            = rg_create_ptr->user_input.raid_group_id;
        create_raid.raid_configuration.width           = rg_create_ptr->user_input.drive_count;
        create_raid.class_id                               = rg_create_ptr->class_id;

        status = fbe_raid_group_create_get_physical_drive_info(
                rg_create_ptr->physical_drive_id_array[0],
                &create_raid.raid_configuration.raid_group_drives_type);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s entry, could not obtain physical drive information, status %d\n", 
                              __FUNCTION__, status);

            return status;
        }
    }
    create_raid.object_id                              = FBE_OBJECT_ID_INVALID;
    create_raid.transaction_id                         = rg_create_ptr->transaction_id;
    create_raid.user_private                           = rg_create_ptr->user_input.user_private;
    create_raid.raid_configuration.capacity            = rg_create_ptr->user_input.capacity;

    create_raid.raid_configuration.element_size        = element_size; 
    create_raid.raid_configuration.elements_per_parity = elements_per_parity;
    create_raid.raid_configuration.chunk_size          = FBE_RAID_DEFAULT_CHUNK_SIZE; 
    create_raid.raid_configuration.debug_flags         = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
    create_raid.raid_configuration.raid_type           = rg_create_ptr->user_input.raid_type;

    create_raid.raid_configuration.power_saving_idle_time_in_seconds = 
        rg_create_ptr->user_input.power_saving_idle_time_in_seconds;

    create_raid.raid_configuration.max_raid_latency_time_in_sec = 
        rg_create_ptr->user_input.max_raid_latency_time_in_seconds;

    create_raid.raid_configuration.power_saving_enabled = FBE_FALSE; /*by default it's disabled until the user enables it*/
  
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_RAID,
                                                 &create_raid,
                                                 sizeof(create_raid),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, raid group id %d\n", 
                __FUNCTION__, 
                create_raid.object_id);

        *object_id = create_raid.object_id;
    }
     return status;
}
/*********************************************
 * end fbe_raid_group_create_raid_group_creation()
 *********************************************/

/*!**************************************************************
 * fbe_raid_group_create_raid10_group_creation()
 ****************************************************************
 * @brief
 * This function creates a raid group of type 10
 *
 * @param rg_crete_ptr IN - structure with all data
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/11/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_raid10_group_creation(
        job_service_raid_group_create_t * rg_create_ptr,
        fbe_element_size_t element_size, 
        fbe_elements_per_parity_t elements_per_parity,
        fbe_object_id_t *object_id)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_raid_t create_raid;

    create_raid.class_id                               = rg_create_ptr->class_id;
    create_raid.raid_id                                = rg_create_ptr->user_input.raid_group_id;
    create_raid.object_id                              = FBE_OBJECT_ID_INVALID;
    create_raid.transaction_id                         = rg_create_ptr->transaction_id;
    create_raid.raid_configuration.capacity            = rg_create_ptr->user_input.capacity;
    create_raid.raid_configuration.width               = rg_create_ptr->user_input.drive_count;
    create_raid.raid_configuration.element_size        = element_size; 
    create_raid.raid_configuration.elements_per_parity = elements_per_parity;
    create_raid.raid_configuration.chunk_size          = FBE_RAID_DEFAULT_CHUNK_SIZE; 
    create_raid.raid_configuration.debug_flags         = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
    create_raid.raid_configuration.raid_type           = rg_create_ptr->user_input.raid_type;

    create_raid.raid_configuration.power_saving_idle_time_in_seconds = 
        rg_create_ptr->user_input.power_saving_idle_time_in_seconds;

    create_raid.raid_configuration.max_raid_latency_time_in_sec = 
        rg_create_ptr->user_input.max_raid_latency_time_in_seconds;

    /*by default it's disabled until the user enables it*/
    create_raid.raid_configuration.power_saving_enabled = FBE_FALSE;
    
    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_RAID,
                                                 &create_raid,
                                                 sizeof(create_raid),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, raid group id %d\n", 
                __FUNCTION__, 
                create_raid.object_id);

        *object_id = create_raid.object_id;
    }
     return status;
}
/***************************************************
 * end fbe_raid_group_create_raid10_group_creation()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_object_lifecycle_state()
 ****************************************************************
 * @brief
 * This function obtains the object's lifecycle state
 *
 * @param object_id - IN object id
 * @param lifecycle state - OUT lifecycle object state
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/22/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_lba_t fbe_raid_group_create_get_object_lifecycle_state(
        fbe_object_id_t object_id,
        fbe_lifecycle_state_t *lifecycle_state)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_mgmt_get_lifecycle_state_t object_lifecycle_state;
    object_lifecycle_state.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    status = fbe_create_lib_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                &object_lifecycle_state,
                                                sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, object %d, lifecycle state %d\n", 
                __FUNCTION__, 
                object_id,
                object_lifecycle_state.lifecycle_state);

        *lifecycle_state = object_lifecycle_state.lifecycle_state;
    }

    return status;
}
/*************************************************************
 * end fbe_raid_group_create_get_object_lifecycle_state()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_provision_drive_by_location()
 ****************************************************************
 * @brief
 * This function obtains the SEP object id of a PVD
 * by X_Y_Z location
 *
 * @param provision_drive_id_by_location - structure containing X_Y_Z location
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/04/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_get_provision_drive_by_location(
        fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot, fbe_object_id_t *object_id)
{
    fbe_status_t                                               status = FBE_STATUS_OK;
    fbe_topology_control_get_provision_drive_id_by_location_t provision_drive_id_by_location;

    provision_drive_id_by_location.object_id = FBE_OBJECT_ID_INVALID;
    provision_drive_id_by_location.port_number = bus;
    provision_drive_id_by_location.enclosure_number = enclosure;
    provision_drive_id_by_location.slot_number = slot;

    status = fbe_create_lib_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_PROVISION_DRIVE_BY_LOCATION,
                                                 &provision_drive_id_by_location,
                                                 sizeof(fbe_topology_control_get_provision_drive_id_by_location_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if ((status == FBE_STATUS_OK) && (provision_drive_id_by_location.object_id != FBE_OBJECT_ID_INVALID))
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, provision drive by location id %d\n", 
                __FUNCTION__, 
                provision_drive_id_by_location.object_id);

        *object_id = provision_drive_id_by_location.object_id;
    }

    return status;
}
/*************************************************************
 * end fbe_raid_group_create_get_provision_drive_by_location()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_virtual_drive_capacity()
 ****************************************************************
 * @brief
 * This function obtains a virtual drive exported capacity
 *
 * @param virtual_drive_imported_capacity - The capacity (not including offset)
 *          that is imported to the virtual drive
 * @param virtual_drive_block_offset - The virtual drive block offset
 * @param out_virtual_drive_exported_capacity - OUT virtual drive
 * exported capacity (including offset)
 * @param out_virtual_drive_imported_capacity - OUT virtual drive
 * imported capacity (including offset)
 * @return status - of call to get virtual drive exported capacity.
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_get_virtual_drive_capacity(
        fbe_lba_t virtual_drive_imported_capacity,
        fbe_lba_t virtual_drive_block_offset,  
        fbe_lba_t *out_virtual_drive_exported_capacity,
        fbe_lba_t *out_virtual_drive_imported_capacity)
{
    fbe_status_t                                           status = FBE_STATUS_OK;
    fbe_virtual_drive_control_class_calculate_capacity_t   virtual_drive_capacity_info;
 
    /* create the VD objects */
    virtual_drive_capacity_info.imported_capacity = virtual_drive_imported_capacity + virtual_drive_block_offset;

    /* @todo: must implement way to get the logical block geometry from the pvd */
    //virtual_drive_capacity_info.block_edge_geometry = configured_physical_block_size;
    virtual_drive_capacity_info.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s entry\n", 
                __FUNCTION__);

    /* exported capacity is filled by calling the VD class */
    virtual_drive_capacity_info.exported_capacity = 0;

    status = fbe_create_lib_send_control_packet (FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,
                                                 &virtual_drive_capacity_info,
                                                 sizeof(fbe_virtual_drive_control_class_calculate_capacity_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s exported: 0x%llx imported: 0x%llx\n", 
                __FUNCTION__, 
                (unsigned long long)virtual_drive_capacity_info.exported_capacity,
                (unsigned long long)virtual_drive_capacity_info.imported_capacity);

        *out_virtual_drive_exported_capacity = virtual_drive_capacity_info.exported_capacity;
        *out_virtual_drive_imported_capacity = virtual_drive_capacity_info.imported_capacity;
    }
    else
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry, status 0x%x\n",__FUNCTION__, status);

        *out_virtual_drive_exported_capacity = FBE_LBA_INVALID;
        *out_virtual_drive_imported_capacity = FBE_LBA_INVALID;
    }


    return status;

}
/*************************************************************
 * end fbe_raid_group_create_get_virtual_drive_capacity()
 *************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_provision_drive_exported_size()
 ****************************************************************
 * @brief
 * This function obtains a provision drive exported capacity
 *
 * @param virtual_drive_imported_capacity - IN provision imported
 * capacity
 * @param virtual_drive_exported_capacity - OUT provision exported
 * capacity
 * @param configured_physical_block_size - IN PVD physical block size
 *
 * @return status - of call to exported get capacity.
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_get_provision_drive_exported_size(
                                    fbe_object_id_t              pvd_object_id,
                                    fbe_lba_t                    in_provision_drive_imported_capacity, 
                                    fbe_lba_t                    *out_provision_drive_exported_capacity)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_provision_drive_control_class_calculate_capacity_t  provision_drive_capacity_info;


    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", 
            __FUNCTION__);

    /* The imported capacity should be correct independant whether it is a system
     * drive or not.
     */
    provision_drive_capacity_info.imported_capacity = in_provision_drive_imported_capacity;

    /* @todo: must implement way to get the logical block geometry from the pvd */
    //provision_drive_capacity_info.block_edge_geometry = configured_physical_block_size;
    
    provision_drive_capacity_info.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    
    /* exported capacity is filled by calling the VD class */
    provision_drive_capacity_info.exported_capacity = 0;

    status = fbe_create_lib_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY,
                                                 &provision_drive_capacity_info,
                                                 sizeof(fbe_provision_drive_control_class_calculate_capacity_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_PROVISION_DRIVE,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    
    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "  exported provision drive capacity 0x%llx\n", 
                (unsigned long long)provision_drive_capacity_info.exported_capacity);
    
        *out_provision_drive_exported_capacity = provision_drive_capacity_info.exported_capacity;
    }

    return status;

}
/***************************************************************
 * end fbe_raid_group_create_get_provision_drive_exported_size()
 **************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_virtual_drive_object()
 ****************************************************************
 * @brief
 * This function creates a virtual drive object
 *
 * @param transaction_id - IN id used for the configuration service
 * to persist objects
 * @param IN provision_drive_exported_capacity - capacity for the
 * virtual drive object
 *
 * @return status - of call to create virtual drive object
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_virtual_drive_object(fbe_database_transaction_id_t transaction_id,
        fbe_lba_t virtual_drive_set_capacity,
        fbe_lba_t virtual_drive_default_offset,
        fbe_object_id_t *out_virtual_drive_object_id)
{
    fbe_database_control_vd_t         create_virtual_drive;
    fbe_status_t                           status = FBE_STATUS_OK;

    /* Validate that the capacity is as least as much as the default offset.
     */
    if (virtual_drive_set_capacity <= virtual_drive_default_offset)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s trans: 0x%llx capacity: 0x%llx doesn't support offset: 0x%llx\n", 
                          __FUNCTION__, (unsigned long long)transaction_id, (unsigned long long)virtual_drive_set_capacity, (unsigned long long)virtual_drive_default_offset);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the create request.
     */
    create_virtual_drive.object_id = FBE_OBJECT_ID_INVALID;
    create_virtual_drive.transaction_id                     = transaction_id;
    create_virtual_drive.configurations.capacity            = virtual_drive_set_capacity;
    create_virtual_drive.configurations.width               = FBE_VIRTUAL_DRIVE_WIDTH;
    create_virtual_drive.configurations.element_size        = 128;
    create_virtual_drive.configurations.elements_per_parity = 0;
    create_virtual_drive.configurations.chunk_size          = FBE_RAID_DEFAULT_CHUNK_SIZE;
    create_virtual_drive.configurations.raid_type           = FBE_RAID_GROUP_TYPE_SPARE; 
    create_virtual_drive.configurations.debug_flags         = 0;
    create_virtual_drive.configurations.configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
    create_virtual_drive.configurations.vd_default_offset  = virtual_drive_default_offset;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_VD,
                                                 &create_virtual_drive,
                                                 sizeof(create_virtual_drive),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s FBE_DATABASE_CONTROL_CODE_CREATE_VIRTUAL_DRIVE failed\n", __FUNCTION__ );
    }

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry, created virtual drive object 0x%x\n", 
            __FUNCTION__, 
            create_virtual_drive.object_id);

    *out_virtual_drive_object_id = create_virtual_drive.object_id;


    return status;
}
/*********************************************
 * end fbe_raid_group_create_virtual_drive()
 *********************************************/

/*!**************************************************************
 * fbe_raid_group_create_print_request_elements()
 ****************************************************************
 * @brief
 * This function prints the raid group request fields
 *
 * @param packet - Packet to use in the callback call
 * @param context - the semaphore
 *
 * @return none
 *
 * @author
 *  01/19/2010 - Created. Jesus Medina
 ****************************************************************/
static void fbe_raid_group_create_print_request_elements(job_service_raid_group_create_t *req_p)
{
    const char                                   *p_raid_type_str = NULL;
    fbe_u32_t                                    drive_index = 0;
    fbe_raid_group_create_translate_raid_type(req_p->user_input.raid_type, &p_raid_type_str);
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "Job Control Service, execution queue:::Received Raid group create request\n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "   Raid_type %s, user id %d\n", p_raid_type_str, req_p->user_input.raid_group_id);

    if (req_p->user_input.capacity == FBE_LBA_INVALID)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   capacity = FBE_LBA_INVALID (will consume minimum VD space available)\n");
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   drive_count %d, is_private %d, be_bandwidth: %d, user_private: %d\n", req_p->user_input.drive_count, \
                req_p->user_input.is_private,
                req_p->user_input.b_bandwidth,
                req_p->user_input.user_private);
    }
    else
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   capacity 0x%llx, drive_count %d, is_private %d, be_bandwidth: %d, user_private: %d\n",
                (unsigned long long)req_p->user_input.capacity, req_p->user_input.drive_count,
                req_p->user_input.is_private,
                req_p->user_input.b_bandwidth,
                req_p->user_input.user_private);
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "   Drives:\n");

    for(drive_index = 0; drive_index < req_p->user_input.drive_count; drive_index++)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "       drive %d: %d_%d_%d\n",
                drive_index,
                req_p->user_input.disk_array[drive_index].bus,
                req_p->user_input.disk_array[drive_index].enclosure,
                req_p->user_input.disk_array[drive_index].slot);

    }
    
    //job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            //"   power_saving_idle_time_in_seconds %d, explicit_removal %d\n",
            //req_p->user_input.power_saving_idle_time_in_seconds, req_p->user_input.explicit_removal);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    return;
}
/****************************************************
 * end fbe_raid_group_create_print_request_elements()
 * *************************************************/


/*!**************************************************************
 * fbe_raid_group_create_generic_completion_function()
 ****************************************************************
 * @brief
 * This function is used as the completion callback function
 * for cimmunicating between services
 *
 * @param packet - Packet to use in the callback call
 * @param context - the semaphore
 *
 * @return status - always status ok 
 *
 * @author
 *  01/15/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_create_generic_completion_function(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_raid_group_create_generic_completion_function()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_translate_raid_type()
 ****************************************************************
 * @brief
 * This function is used to translate a raid type to a string value
 *
 * @param raid_type.
 *
 * @return status - The raid type in string value.
 *
 * @author
 *  01/15/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_translate_raid_type (fbe_raid_group_type_t raid_type, 
        const char ** pp_raid_type)
{
    *pp_raid_type = NULL;
    switch (raid_type) {
        case FBE_RAID_GROUP_TYPE_RAID1:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_RAID1";
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_RAID10";
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_RAID3";
            break;
        case FBE_RAID_GROUP_TYPE_RAID0:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_RAID0";
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_RAID5";
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_RAID6";
            break;
        case FBE_RAID_GROUP_TYPE_SPARE:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_SPARE";
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER";
            break;
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_UNKNOWN";
            break;
        default:
            *pp_raid_type = "FBE_RAID_GROUP_TYPE_UNKNOWN";
            break;
    }
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_create_translate_raid_type()
 ************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_configured_provision_drive_info()
 ****************************************************************
 * @brief
 * This function obtains the configured provision drive
 * info
 * 
 * @param in_object_id - IN provision drive id
 * @param num_upstream_objects - Number of upstream objects
 * @param configured_physical_block_size - OUT provision drive's 
 * block size 
 * @param provision_drive_exported_capacity - OUT provision drive's 
 * imported capacity
 * @param error_code - Job service error code.
 *
 * @return FBE_STATUS_OK - if all parameters are ok
 * @return FBE_STATUS_GENERIC_FAILURE - if status to obtain provision
 *                                      info returns error
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 *  08/16/2012 - Modified Prahlad Purohit
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_get_configured_provision_drive_info(fbe_object_id_t in_object_id,
        fbe_u32_t       num_upstream_objects, 
        fbe_provision_drive_configured_physical_block_size_t *configured_physical_block_size,
        fbe_lba_t *provision_drive_imported_capacity,
        fbe_lba_t *provision_drive_exported_capacity,
        fbe_job_service_error_type_t  *error_code)

{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_provision_drive_info_t  provision_drive_info;

    // Initialize to Internal error we will set a more specific error if required.
    *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

    /* Always invoke the method to get the block size.
     */
    status = fbe_create_lib_send_control_packet (FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                 &provision_drive_info,
                                                 sizeof(fbe_provision_drive_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 in_object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s get provision drive info call failed for object id %d, status %d\n", 
                __FUNCTION__, in_object_id, status);

        return status;
    }

    /* verify configured physical block size */
    if (provision_drive_info.configured_physical_block_size 
            == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s Invalid configured block side for PVD %d\n", 
                __FUNCTION__, in_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify configuration type */
    if (provision_drive_info.config_type 
            == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s PVD %d, configured as spare type, invalid configuration\n", 
                __FUNCTION__, in_object_id);

        *error_code = FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE;

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify END OF LIFE STATE */
    /*If end of life state of pvd is set it means this drive is about to die.
     We can not use such drive for raid group creation.
    */
    if (provision_drive_info.end_of_life_state == FBE_TRUE) 
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s PVD %d, end of life state is set, invalid state\n", 
                __FUNCTION__, in_object_id);

        *error_code = FBE_JOB_SERVICE_ERROR_PVD_IS_IN_END_OF_LIFE_STATE;

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! @note If the provision drive already has an upstream object, the imported
     *        capacity is the amount of capacity remaining.
     */
    *provision_drive_exported_capacity = 0;
    if (num_upstream_objects > 0)
    {
        fbe_lba_t   provision_drive_remaining_exported_capacity = 0;

        /* Check if there are any upstream objects. If there are, calculate the 
         * exported capacity differently. 
         */
        status = fbe_raid_group_create_get_unused_provision_drive_extent_size(in_object_id, &provision_drive_remaining_exported_capacity);
        if (status == FBE_STATUS_OK)
        {
            /* Add the metadata capacity.
             */
            *provision_drive_imported_capacity = provision_drive_remaining_exported_capacity + provision_drive_info.paged_metadata_capacity;
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s PVD %d, Error: %d getting unused size.\n", 
                    __FUNCTION__, in_object_id, (int)status);

            return status;
        }
    }
    else
    {
        /* Else there are no upstream objects. Use the imported capacity.
         */
        *provision_drive_imported_capacity = provision_drive_info.imported_capacity;
        *provision_drive_exported_capacity = provision_drive_info.capacity;
    }

    /* Populate the block size information.
     */
    *configured_physical_block_size = provision_drive_info.configured_physical_block_size;

    *error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    return status;
} 
/*****************************************************************
 * end fbe_raid_group_create_get_configured_provision_drive_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_physical_drive_info()
 ****************************************************************
 * @brief
 * This function obtains the physical provision drive
 * info
 * 
 * @param in_object_id - IN provision drive id
 * @param configured_physical_block_size - OUT provision drive's 
 * block size 
 * @return FBE_STATUS_OK - if all parameters are ok
 * @return FBE_STATUS_GENERIC_FAILURE - if status to obtain physical
 *                                      info returns error
 *
 * @author
 *  08/16/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_get_physical_drive_info(fbe_object_id_t in_object_id,
        fbe_drive_type_t *drive_type)
{
    fbe_physical_drive_mgmt_get_drive_information_t drive_information;
    fbe_status_t status = FBE_STATUS_OK;
    status = fbe_create_lib_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                                 &drive_information,
                                                 sizeof(fbe_physical_drive_mgmt_get_drive_information_t),
                                                 FBE_PACKAGE_ID_PHYSICAL,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 in_object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, physical drive info error, in_object_id %d, status %d\n", 
                __FUNCTION__, in_object_id, status);

        return status;
    }

    *drive_type = drive_information.get_drive_info.drive_type;
    return status;
} 
/*****************************************************************
 * end fbe_raid_group_create_get_physical_drive_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_validate_configured_provision_drive_block_size()
 ****************************************************************
 * @brief
 * This function validates the provision drive info block size
 * 
 * @param in_object_id - IN provision drive id
 * @param configured_physical_block_size - IN provision drive's 
 * block size 
 *
 * @return FBE_STATUS_OK - if all parameters are ok
 * @return FBE_STATUS_GENERIC_FAILURE - if any of the provision 
 *                                      drive info parameters is
 *                                      invalid
 *
 * @author
 *  03/19/2010 - Created. Jesus Medina
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_validate_configured_provision_drive_block_size(
        fbe_object_id_t in_object_id,
        fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* verify configured physical block size */
    if (configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, Invalid configured block size for PVD object %d, status %d\n", 
                __FUNCTION__, in_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
} 
/*****************************************************************
 * end fbe_raid_group_create_validate_configured_provision_drive_block_size()
 ****************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_validate_raid_group_class_parameters
 ****************************************************************
 * @brief
 * This function determines the number of disks passed to create
 * the raid group is enough for the raid group creation
 *
 * @param raid_group_type - IN raid group type
 *
 * @return FBE_STATUS_OK - if all parameters are ok
 * @return FBE_STATUS_GENERIC_FAILURE - if raid group type can
 * not be determined
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_validate_raid_group_class_parameters(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_group_class_get_info_t get_info;
    fbe_class_id_t                  class_id = FBE_CLASS_ID_INVALID;

    class_id = fbe_raid_group_create_get_class_id(fbe_raid_group_create_req_p->user_input.raid_type);
    fbe_raid_group_create_req_p->class_id = class_id;
    if (class_id == FBE_CLASS_ID_INVALID) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    get_info.raid_type             = fbe_raid_group_create_req_p->user_input.raid_type;
    get_info.width                 = fbe_raid_group_create_req_p->user_input.drive_count;
    get_info.b_bandwidth           = fbe_raid_group_create_req_p->user_input.b_bandwidth;
    get_info.exported_capacity     = fbe_raid_group_create_req_p->user_input.capacity;
    /* The single drive capacity must be at least 2x the default chunk 
     * size. 
     */
    get_info.single_drive_capacity = FBE_RAID_DEFAULT_CHUNK_SIZE * 2;

    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO,
                                                 &get_info,
                                                 sizeof(fbe_raid_group_class_get_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 class_id,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, raid group Invalid class parameter, status %d\n", 
                __FUNCTION__, status);
    }
    return status;        
}
/******************************************************************
 * end fbe_raid_group_create_validate_raid_group_class_parameters()
 *****************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_is_valid_raid_group_number()
 ****************************************************************
 * @brief
 * This function is used to determine if the Raid Group id is valid
 *
 * @param raid_group_id.
 *
 * @return TRUE - The raid group id is valid
 * @return FALSE - The raid group id is invalid
 *
 * @author
 *  01/15/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_bool_t fbe_raid_group_create_is_valid_raid_group_number(fbe_raid_group_number_t raid_group_id)
{
    if (raid_group_id == FBE_RAID_ID_INVALID) {
        return FBE_TRUE;/*user wants us to populate it for him*/
    }

    if (((raid_group_id > (FBE_RG_USER_COUNT - 1)) && (raid_group_id < RAID_GROUP_COUNT )) ||
         (raid_group_id > MAX_RAID_GROUP_ID))
    {
        return (FBE_FALSE);
    }

    return (FBE_TRUE);

}
/********************************************************
 * end fbe_raid_group_create_is_valid_raid_group_number()
 *******************************************************/

/*!**************************************************************
 * fbe_raid_group_create_is_valid_raid_group_type()
 ****************************************************************
 * @brief
 * This function is used to determine if the Raid Group type is
 * valid
 *
 * @param raid_type 
 *
 * @return TRUE - The raid group type is valid
 * @return FALSE - The raid group type is invalid
 *
 * @author
 *  01/15/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_bool_t fbe_raid_group_create_is_valid_raid_group_type(fbe_raid_group_type_t raid_type)
{
    fbe_bool_t valid_type;

    switch (raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_RAID10:
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID5:
        case FBE_RAID_GROUP_TYPE_RAID6:
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            valid_type = FBE_TRUE;
            break;
        case FBE_RAID_GROUP_TYPE_SPARE:
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
        case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
        default:
            valid_type = FBE_FALSE;
            break;
    }

    return valid_type;
}
/******************************************************
 * end fbe_raid_group_create_is_valid_raid_group_type()
 *****************************************************/

/*!**************************************************************
 * fbe_raid_group_create_are_provision_drives_compatible()
 ****************************************************************
 * @brief
 * This function is used to determine if the provision drives
 * used for creating the raid group are compatible
 *
 * @param configured_physical_block_size - array of block sizes
 * @param number_provision_drives - size of array of block sizes
 *
 *
 * @return FBE_STATUS_OK - if all provision drives are compatible
 * @return FBE_STATUS_GENERIC_FAILURE - if any of the provision 
 * drives are not compatible
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_are_provision_drives_compatible(
        fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size[],
        fbe_u32_t number_provision_drives)
{
    /* With the introduction of 4k we will not limit the creation of 
     * raid groups with mixed block size configurations (4k and 520). 
     */
    return FBE_STATUS_OK;
#if 0
    fbe_u32_t current_pvd_index = 0;
    fbe_u32_t next_pvd_index = 0;
    fbe_provision_drive_configured_physical_block_size_t provision_drive_type1;
    fbe_provision_drive_configured_physical_block_size_t provision_drive_type2;
    fbe_status_t status = FBE_STATUS_OK;
    
    provision_drive_type1 = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID;
    provision_drive_type2 = FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID;

    while ((status != FBE_STATUS_GENERIC_FAILURE) &&
            (current_pvd_index < number_provision_drives))
    {
        /* Increment next_pvd_index to check for the next PVD id in the 
         * PVD array.
         */
        next_pvd_index = current_pvd_index + 1;

        while (next_pvd_index < number_provision_drives)
        {
            /*  All the PVD's configured block size should be either
             *
             *  FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_512 or
             *  FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520
             *
             *  But not combination of each other and not invalid type
             */
            provision_drive_type1 = configured_physical_block_size[current_pvd_index];
            provision_drive_type2 = configured_physical_block_size[next_pvd_index];

            if (provision_drive_type1 != provision_drive_type2)
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s entry, Mismatch PVD configured block size: PVD_1=%d, block_size_type = 0x%x\n", 
                        __FUNCTION__, 
                        configured_physical_block_size[current_pvd_index], provision_drive_type1);

                job_service_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry,  PVD_2=%d, block_size_type = 0x%x\n", 
                        __FUNCTION__, 
                        configured_physical_block_size[next_pvd_index], provision_drive_type2);

                status = FBE_STATUS_GENERIC_FAILURE;
                return status;
            }

            /* Increment the next_pvd_index to check for the next FRU in the 
             * list.
             */
            next_pvd_index++;
        }

        /* Reset the next_pvd_index */
        next_pvd_index = 0;

        /* Increment the current_pvd_index */
        current_pvd_index++;
    }
    return status;
#endif
}
/*************************************************************
 * end fbe_raid_group_create_are_provision_drives_compatible()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_are_physical_drives_compatible()
 ****************************************************************
 * @brief
 * This function is used to determine if the physical drives
 * used for creating the raid group are compatible in terms of
 * drive type
 *
 * @param drive_type_array - array of drive types
 * @param number_provision_drives - size of array of block sizes
 *
 *
 * @return FBE_STATUS_OK - if all physical drives are compatible
 * @return FBE_STATUS_GENERIC_FAILURE - if any of the provision 
 * drives are not comparable
 *
 * @author
 *  02/28/2013  Ron Proulx  - Updated to use same rules as sparing
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_are_physical_drives_compatible(
        fbe_drive_type_t drive_type_array[],
        fbe_u32_t number_provision_drives)

{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       first_drive_index = 0;
    fbe_performance_tier_number_t   first_drive_performance_tier_number = FBE_PERFORMANCE_TIER_INVALID;
    fbe_u32_t                       current_drive_index;
    fbe_drive_type_t                first_drive_type;
    fbe_bool_t                      b_is_drive_type_valid_for_performance_tier;
    
    /*! @note Use the same drive compatibility rules as sparing.  Get the
     *        performance tier information for the first physical drive
     *        and simply validate that the remaining drives in the drive
     *        array are in that performance tier.
     *
     *  @note We have already validated `number_provision_drives'.
     */

    /* First get the performance tier for the first drive.
     */
    first_drive_type = drive_type_array[first_drive_index];
    status = fbe_spare_lib_selection_get_drive_performance_tier_number(first_drive_type, &first_drive_performance_tier_number);
    if (status != FBE_STATUS_OK)
    {
        /* Report the failure
         */
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s failed to get performance tier for index: %d drive type: %d - status: 0x%x\n", 
                          __FUNCTION__, first_drive_index, first_drive_type, status);
        return status;
    }

    /* Now walk thru the remaining drive and check if they are in the same
     * performance tier.
     */
    for (current_drive_index = (first_drive_index + 1); current_drive_index < number_provision_drives; current_drive_index++)
    {
        /* Check if the current index drive type is compatible or not.
         */
        status = fbe_spare_lib_utils_is_drive_type_valid_for_performance_tier(first_drive_performance_tier_number,
                                                                              drive_type_array[current_drive_index],
                                                                              &b_is_drive_type_valid_for_performance_tier);
        if (status != FBE_STATUS_OK)
        {
            /* Report the failure
             */
            job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to check drive index: %d drive type: %d for tier: %d - status: 0x%x \n",
                              __FUNCTION__, current_drive_index, drive_type_array[current_drive_index], 
                              first_drive_performance_tier_number, status);
            return status;
        }

        /* Now check if the drive type was compatible with the performance tier
         * if the first drive.
         */
        if (b_is_drive_type_valid_for_performance_tier == FBE_FALSE)
        {
            /* Trace the failure.
             */
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_create: drive index: %d type: %d tier: %d not compatible with index: %d type: %d \n",
                              first_drive_index, first_drive_type, first_drive_performance_tier_number,
                              current_drive_index, drive_type_array[current_drive_index]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } /* end for all drives in the list */

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*************************************************************
 * end fbe_raid_group_create_are_physical_drives_compatible()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_minimum_virtual_drive_size()
 ****************************************************************
 * @brief
 * This function determines the smallest capacity of an array
 * of virtual drives
 *
 * @param virtual_drive_id_array - array of provision drive capacities
 *
 * @return smallest virtual drive capacity
 *
 * @author
 *  01/22/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_lba_t fbe_raid_group_create_get_minimum_virtual_drive_size(
        fbe_lba_t virtual_drive_capacity_array[], fbe_u32_t number_of_items_in_array)
{
    fbe_u32_t vd_index = 0;
    fbe_lba_t drive_capacity = virtual_drive_capacity_array[0];

    for (vd_index=0; vd_index < number_of_items_in_array; vd_index++)
    {
        if (virtual_drive_capacity_array[vd_index] < drive_capacity)
            drive_capacity = virtual_drive_capacity_array[vd_index];
    }
    return drive_capacity;
}
/*********************************************************
 * end fbe_raid_group_create_get_minimum_virtual_drive_size()
 *********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_calculate_raid_group_capacity()
 ****************************************************************
 * @brief
 * This function determines the capacity to be given to the
 * raid group set configuration call
 *
 * @param fbe_raid_group_create_req_p IN - data structure containing
 * raid type and number of disks for raid group
 *
 * @return status
 *
 * @author
 *  01/22/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_calculate_raid_group_capacity(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p, 
        fbe_lba_t *minimum_virtual_drive_capacity,
        fbe_element_size_t *element_size,
        fbe_elements_per_parity_t *elements_per_parity,
        fbe_lba_t virtual_drive_capacity_array[],
        fbe_job_service_error_type_t  *error_code)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_group_class_get_info_t get_info;
    fbe_class_id_t                  class_id = FBE_CLASS_ID_INVALID;

    class_id = fbe_raid_group_create_get_class_id(fbe_raid_group_create_req_p->user_input.raid_type);

    if (class_id == FBE_CLASS_ID_INVALID) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_info.raid_type             = fbe_raid_group_create_req_p->user_input.raid_type;
    get_info.width                 = fbe_raid_group_create_req_p->user_input.drive_count;
    get_info.b_bandwidth           = fbe_raid_group_create_req_p->user_input.b_bandwidth;
    get_info.exported_capacity     = fbe_raid_group_create_req_p->user_input.capacity;
    get_info.single_drive_capacity = FBE_LBA_INVALID;

    /* if fbe_raid_group_create_req_p->user_input.capacity = FBE_LBA_INVALID, reset single drive capacity,
     * this means call to the RG class will NOT calculate the individual VD size */
    if (fbe_raid_group_create_req_p->user_input.capacity == FBE_LBA_INVALID) {

        *minimum_virtual_drive_capacity = fbe_raid_group_create_get_minimum_virtual_drive_size(
                             virtual_drive_capacity_array,
                             fbe_raid_group_create_req_p->user_input.drive_count);

        // we need to adjust the minimum capacity here so that the raid capacity can be calculated correctly
        status = fbe_create_raid_group_create_determine_if_extent_size_fits(fbe_raid_group_create_req_p,
                            fbe_raid_group_create_req_p->virtual_drive_id_array,
                            fbe_raid_group_create_req_p->user_input.drive_count,
                            minimum_virtual_drive_capacity,
                            FBE_FALSE, /* Don't ignore offset*/
                            error_code);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s calc determine if extent fits failed - status: 0x%x\n",
                    __FUNCTION__, status);
            return status;
        }

        get_info.single_drive_capacity = *minimum_virtual_drive_capacity;
    }

    if (get_info.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        get_info.width = 2;
    }

    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO,
                                                 &get_info,
                                                 sizeof(fbe_raid_group_class_get_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 class_id,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, could not calculate raid group capacity, type %d, status %d\n",
                __FUNCTION__, fbe_raid_group_create_req_p->user_input.raid_type, status);
        return status;
    }

    *element_size = get_info.element_size;
    *elements_per_parity = get_info.elements_per_parity;
    if (fbe_raid_group_create_req_p->user_input.capacity != FBE_LBA_INVALID) 
    {
        *minimum_virtual_drive_capacity = get_info.single_drive_capacity;
        // we need to adjust the minimum capacity
        status = fbe_create_raid_group_create_determine_if_extent_size_fits(fbe_raid_group_create_req_p,
                            fbe_raid_group_create_req_p->virtual_drive_id_array,
                            fbe_raid_group_create_req_p->user_input.drive_count,
                            minimum_virtual_drive_capacity,
                            FBE_FALSE, /* Don't ignore offset*/
                            error_code);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s val determine if extent fits failed - status: 0x%x\n",
                    __FUNCTION__, status);
            return status;
        }
    }
    else
    {
        fbe_raid_group_create_req_p->user_input.capacity = get_info.exported_capacity;
    }
    return status;
}
/************************************************************
 * end fbe_raid_group_create_calculate_raid_group_capacity()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_calculate_raid10_group_capacity()
 ****************************************************************
 * @brief
 * This function determines the capacity to be given to the
 * raid group set configuration call -- Raid 10 only
 *
 * @param fbe_raid_group_create_req_p IN - data structure containing
 * raid type and number of disks for raid group
 *
 * @return status
 *rg_mirror_capacity
 * @author
 *  05/13/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_create_calculate_raid10_group_capacity(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p, 
        fbe_element_size_t *element_size,
        fbe_elements_per_parity_t *elements_per_parity,
        fbe_lba_t *rg_mirror_capacity)
{

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_group_class_get_info_t get_info;
    fbe_class_id_t                  class_id = FBE_CLASS_ID_INVALID;
    class_id = fbe_raid_group_create_get_class_id(fbe_raid_group_create_req_p->user_input.raid_type);

    if (class_id == FBE_CLASS_ID_INVALID) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_info.raid_type             = fbe_raid_group_create_req_p->user_input.raid_type;
    get_info.width                 = fbe_raid_group_create_req_p->user_input.drive_count;
    get_info.b_bandwidth           = fbe_raid_group_create_req_p->user_input.b_bandwidth;
    get_info.exported_capacity     = fbe_raid_group_create_req_p->user_input.capacity;
    get_info.single_drive_capacity = FBE_LBA_INVALID;

    if (fbe_raid_group_create_req_p->user_input.capacity == FBE_LBA_INVALID) 
    {
        get_info.single_drive_capacity = *rg_mirror_capacity;
    }

    if (get_info.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        get_info.width = 2;
    }

    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO,
                                                 &get_info,
                                                 sizeof(fbe_raid_group_class_get_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 class_id,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, could not calculate raid group capacity, type %d, status %d\n",
                __FUNCTION__, fbe_raid_group_create_req_p->user_input.raid_type, status);
        return status;
    }

    *element_size = get_info.element_size;
    *elements_per_parity = get_info.elements_per_parity;
    if (fbe_raid_group_create_req_p->user_input.capacity != FBE_LBA_INVALID) 
    {
        *rg_mirror_capacity = get_info.single_drive_capacity;
    }
    else
    {
        fbe_raid_group_create_req_p->user_input.capacity = get_info.exported_capacity;
    }
    return status;
}
/************************************************************
 * end fbe_raid_group_create_calculate_raid10_group_capacity()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_group_create_validate_provision_drive_list()
 ****************************************************************
 * @brief
 * This function validates the list of provision drives passed
 * to the raid group creation function
 *
 * @param fbe_raid_group_create_req_p - data structure containing
 * list of provisional drive ids
 * @param error_code - any error found while verifying pvd info
 *
 * @return status
 *
 * @author
 *  02/02/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_validate_provision_drive_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code)
{
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_u32_t         pvd_index = 0;
    fbe_bool_t        is_sys_drive = FBE_FALSE;
    fbe_u32_t         number_of_upstream_objects = 0;
    fbe_lba_t         provision_drive_imported_capacity_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_object_id_t   upstream_object_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_provision_drive_configured_physical_block_size_t
        configured_physical_block_size_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_lifecycle_state_t lifecycle_state;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",__FUNCTION__);

    for (pvd_index = 0; pvd_index < FBE_MAX_UPSTREAM_OBJECTS;  pvd_index++) {
        upstream_object_list[pvd_index] = FBE_OBJECT_ID_INVALID;
    }

    for (pvd_index = 0; pvd_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH;  pvd_index++) {
        provision_drive_imported_capacity_array[pvd_index] = FBE_LBA_INVALID;
        configured_physical_block_size_array[pvd_index] = 
            FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID;
    }

    /* determine if there are duplicate PVD ids */
    status = fbe_raid_group_find_duplicate_provision_drive_ids(
        fbe_raid_group_create_req_p->provision_drive_id_array,
        fbe_raid_group_create_req_p->user_input.drive_count);

    /* error if provided list of PVDs has a duplicate value */
    if (status != FBE_STATUS_OK) {
        *error_code = FBE_JOB_SERVICE_ERROR_DUPLICATE_PVD;
        return status;
    }

    /* get pvd objects info */
    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++)
    {
        /* if the PVD has an edge to a VD, this PVD has been previously configured as part of
         * a Raid Group and it is being provided so that we build another Raid group on 
         * top of the VD
         */
        number_of_upstream_objects = 0;
        status = fbe_create_lib_get_upstream_edge_list(fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index], 
                                                       &number_of_upstream_objects,
                                                       upstream_object_list);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry, \n", __FUNCTION__);

            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "   fbe_create_lib_get_upstream_edge_list call for PVD object failed, status %d\n", 
                    status);

            *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            return status;
        }

        /* obtain configured provision drive info, this will get the PVD imported capacity */
        status = fbe_raid_group_create_get_configured_provision_drive_info(
                fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                number_of_upstream_objects,
                &configured_physical_block_size_array[pvd_index],
                &provision_drive_imported_capacity_array[pvd_index],
                &fbe_raid_group_create_req_p->pvd_exported_capacity_array[pvd_index],
                error_code);

        if (status != FBE_STATUS_OK) 
        {
            return status;
        }

        /*is this PVD even alive ?*/
        status = fbe_create_lib_get_object_state(fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index], &lifecycle_state);
        if ((status != FBE_STATUS_OK) || (lifecycle_state != FBE_LIFECYCLE_STATE_READY && lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)) 
        {
            if (status != FBE_STATUS_OK) 
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "fbe_create_lib_get_object_state failed, status %d\n", 
                        status);
                *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

                return status;
            }

            /* If PVD is in the Fail or Destroy state, just return Bad PVD configuration error and generic failure */
            if ((lifecycle_state == FBE_LIFECYCLE_STATE_FAIL) || (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY)) 
            {
                job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD 0x%X current state:%d is not good; fail with BAD PVD configuration.\n", 
                                  fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                                  lifecycle_state);  
              
                *error_code = FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION;
 
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Any other lifecycle states, will wait for 30 seconds before timeout */
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD 0x%X current state:%d is not good; wait at least %d for RDY state.\n", 
                                  fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                                  lifecycle_state,
                                  WAIT_FOR_PVD_RDY_LIFECYCLE);

            /* wait for 30 seconds */
            status = fbe_job_service_wait_for_expected_lifecycle_state(fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index], 
                                                                      FBE_LIFECYCLE_STATE_READY, 
                                                                      WAIT_FOR_PVD_RDY_LIFECYCLE);
            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Time-out waiting for PVD 0x%X to be in RDY state; current state:%d. Status:0x%X\n", 
                                  fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index], lifecycle_state, status);

                *error_code = FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION;
 
                return status;
            }
        }

        /* Check if it is a sys pvd */
        fbe_raid_group_create_check_if_sys_drive(fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                                                 &is_sys_drive);

        /* if no upstream edges on the Provision drive object, we must then configure a VD for the PVD */
        if ( (number_of_upstream_objects == 0) || (is_sys_drive))
        {
            job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: VD will be configured for PVD: 0x%x", 
                              __FUNCTION__, fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index]);

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "create:  pvd id:0x%x index: %d has no upstream edges: %d\n", 
                              fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                              pvd_index, number_of_upstream_objects);

            /* validate the provision drive info block size, this is for PVDs that do not have
             * a VD connected 
             */
            status = fbe_raid_group_create_validate_configured_provision_drive_block_size(
                    fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                    configured_physical_block_size_array[pvd_index]);

            if (status != FBE_STATUS_OK) {
                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s failed with status: 0x%x\n", 
                                  __FUNCTION__, status);

                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                    "   validate configured provision drive block size call failed, status: 0x%x\n", 
                        status);

                *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
                return status;
            }

            /* Use the imported (i.e. available) capacity, calculate the available
             * export capacity.
             */
            /* If upstream edge is 0, we already get exported capacity from PVD.
             * Use that export capacity directly.
             */
            if (fbe_raid_group_create_req_p->pvd_exported_capacity_array[pvd_index] == 0)
            {
                status = fbe_raid_group_create_get_provision_drive_exported_size(
                        fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                        provision_drive_imported_capacity_array[pvd_index], 
                        &fbe_raid_group_create_req_p->pvd_exported_capacity_array[pvd_index]);
            }

            if (status != FBE_STATUS_OK) 
            {
                job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry, \n", __FUNCTION__);

               job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                   "   fbe raid group create get provision drive exported size call failed, status: 0x%x\n", 
                       status);

               *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR; 
               return status;
            }

            job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry, \n", __FUNCTION__);

            job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   provision drive id %d, Imported Cap 0x%llx, Exported capacity  0x%llx\n", 
                fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                (unsigned long long)provision_drive_imported_capacity_array[pvd_index],
                (unsigned long long)fbe_raid_group_create_req_p->pvd_exported_capacity_array[pvd_index]);

        }
        else
        {
            /* there is a VD connected to the PVD, just get the id of the VD for later use */
            fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index] = upstream_object_list[0];

            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "  PVD id %d upstream %d edges, PVD has VD object %d connected \n",
                fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                number_of_upstream_objects,
                fbe_raid_group_create_req_p->virtual_drive_id_array[pvd_index]);

            *error_code = FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES;
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* determine if PVD's block sizes are compatible */
    status = fbe_raid_group_create_are_provision_drives_compatible(
            configured_physical_block_size_array, 
            fbe_raid_group_create_req_p->user_input.drive_count);

    if (status != FBE_STATUS_OK) { 
        *error_code = FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_PVDS;
        return status;
    }
    return status;
}
/***********************************************************
 * end fbe_raid_group_create_validate_provision_drive_list()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_validate_physical_drive_list()
 ****************************************************************
 * @brief
 * This function validates the list of physical drives passed
 * to the raid group creation function
 *
 * @param fbe_raid_group_create_req_p - data structure containing
 * list of physical drive ids
 *
 * @return status
 *
 * @author
 *  08/18/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_validate_physical_drive_list(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code)
{
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_u32_t         pvd_index = 0;
    fbe_drive_type_t drive_type_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",__FUNCTION__);

    for (pvd_index = 0; pvd_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH;  pvd_index++) {
        drive_type_array[pvd_index] = FBE_DRIVE_TYPE_INVALID;        
    }

    /* get physical drive type */
    for (pvd_index = 0; pvd_index < fbe_raid_group_create_req_p->user_input.drive_count;  pvd_index++)
    {
        /* obtain physical drive info, this will get the drive type */
        status = fbe_raid_group_create_get_physical_drive_info(
                fbe_raid_group_create_req_p->physical_drive_id_array[pvd_index],
                &drive_type_array[pvd_index]);

        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s entry, could not obtain physical drive information, status %d\n", 
                    __FUNCTION__, status);

            *error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
            return status;
        }

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "  physical drive id %d, pvd id %d, drive_type = %d\n", 
                fbe_raid_group_create_req_p->physical_drive_id_array[pvd_index],
                fbe_raid_group_create_req_p->provision_drive_id_array[pvd_index],
                drive_type_array[pvd_index]);
    }

    /* validation of LDO type is supposed to be based on performance type not
     * LDO type...checking, for now disable call to validate LDO based on
     * LDO type 
     */

    /* determine if the physical drives are compatible */
    status = fbe_raid_group_create_are_physical_drives_compatible(drive_type_array,
            fbe_raid_group_create_req_p->user_input.drive_count);

    if (status != FBE_STATUS_OK) 
    { 
        *error_code = FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_DRIVE_TYPES;
        fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES, NULL, 0, 
                            "%d %d",
                            fbe_raid_group_create_req_p->user_input.raid_type,
                            fbe_raid_group_create_req_p->user_input.drive_count);
        return status;
    }

    return status;
}
/***********************************************************
 * end fbe_raid_group_create_validate_physical_drive_list()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_set_packet_status()
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
 *  01/22/2010 - Created. Jesus Medina
 ****************************************************************/
static void fbe_raid_group_create_set_packet_status(fbe_payload_control_operation_t *control_operation,
                                         fbe_payload_control_status_t payload_status,
                                         fbe_packet_t *packet, fbe_status_t packet_status)
{
    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet, packet_status, 0);
    fbe_transport_complete_packet(packet);
}
/***********************************************
 * end fbe_raid_group_create_set_packet_status()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_group_create_get_class_id()
 ****************************************************************
 * @brief
 * This function sets the class id of a raid group based on the
 * raid group type
 *
 * @param raid_group_type - type of raid group 
 *
 * @return class_id - class type according to raid group size
 *
 * @author
 *  01/22/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_class_id_t fbe_raid_group_create_get_class_id(fbe_raid_group_type_t raid_group_type)
{
    fbe_class_id_t class_id;

    switch (raid_group_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID10:
            class_id = FBE_CLASS_ID_STRIPER;
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
        case FBE_RAID_GROUP_TYPE_RAID6:
            class_id = FBE_CLASS_ID_PARITY;
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:    
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            class_id = FBE_CLASS_ID_MIRROR;
            break;
        case FBE_RAID_GROUP_TYPE_SPARE: /* this is actually a class FBE_RAID_GROUP_TYPE_SPARE, but 
                                           there is RG of this type, so mark this invalid */
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
        default:
            class_id = FBE_CLASS_ID_INVALID;
            break;
    }
    return class_id;
}
/***********************************************
 * end fbe_raid_group_create_get_class_id()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_group_find_duplicate_provision_drive_ids()
 ****************************************************************
 * @brief
 * This function is used to determine if any of the provision drives
 * used for creating the raid group is a duplicate 
 *
 * @param provision_drive_id_array - array of provision drive ids
 * @param number_provision_drives - size of array of provision drive ids
 *
 * @return FBE_STATUS_OK - if all provision drives are unique 
 * @return FBE_STATUS_GENERIC_FAILURE - if provision drive ids 
 * repeat in array
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 ****************************************************************/

static fbe_status_t fbe_raid_group_find_duplicate_provision_drive_ids(fbe_u32_t provision_drive_id_array[], 
        fbe_u32_t number_provision_drives)
{
    fbe_u32_t current_provision_drive_index = 0;
    fbe_u32_t next_provision_drive_index = 0;
    fbe_status_t status = FBE_STATUS_OK;

    while ((status == FBE_STATUS_OK) &&
            (current_provision_drive_index < number_provision_drives))
    {
        /* Increment next_provision_drive_index to check for the next PVD id in the 
         * PVD array.
         */
        next_provision_drive_index = current_provision_drive_index + 1;

        while (next_provision_drive_index < number_provision_drives)
        {
            /* If a duplicate PVD Id found, display the error and exit */
            if (provision_drive_id_array[current_provision_drive_index] == 
                    provision_drive_id_array[next_provision_drive_index])
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s entry,  PVD[%d]=%d is duplicate of PVD[%d]=%d\n", 
                        __FUNCTION__, 
                        next_provision_drive_index, provision_drive_id_array[next_provision_drive_index],
                        current_provision_drive_index, provision_drive_id_array[current_provision_drive_index]);
                status = FBE_STATUS_GENERIC_FAILURE;
                return status;
            }

            /* Increment the next_provision_drive_index to check for the next FRU in the 
             * list.
             */
            next_provision_drive_index++;
        }

        /* Reset the next_provision_drive_index */
        next_provision_drive_index = 0;

        /* Increment the current_provision_drive_index */
        current_provision_drive_index++;
    }

    return status;
}
/*********************************************************
 * end fbe_raid_group_find_duplicate_provision_drive_ids()
 *********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_validate_parameters()
 ****************************************************************
 * @brief
 * This function is used to validate the following Raid Group
 * parameters:
 *
 *  raid group id
 *  raid type
 *  raid expansion settings
 *
 * @param fbe_raid_group_create_req_p.
 *
 * @return FBE_STATUS_OK - if all parameters are ok
 * @return FBE_STATUS_GENERIC_FAILURE - if any of the raid group
 * parameters are invalid
 *
 * @author
 *  01/21/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t fbe_raid_group_create_validate_parameters(
        job_service_raid_group_create_t *fbe_raid_group_create_req_p,
        fbe_job_service_error_type_t  *error_code)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t is_raid_group_valid = FBE_FALSE;
    fbe_object_id_t  rg_object_id = FBE_OBJECT_ID_INVALID;

    is_raid_group_valid = fbe_raid_group_create_is_valid_raid_group_type(
            fbe_raid_group_create_req_p->user_input.raid_type);

    if (is_raid_group_valid != FBE_TRUE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "rg_create_validate_param, Invalid Raid Group type %d\n", 
                fbe_raid_group_create_req_p->user_input.raid_type);
        *error_code = FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_TYPE;
        status = FBE_STATUS_GENERIC_FAILURE;

        fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INVALID_RAID_TYPE, NULL, 0, 
                            "%d",
                            fbe_raid_group_create_req_p->user_input.raid_type);
        return status;
    }

    if (fbe_raid_group_create_req_p->user_input.raid_group_id != FBE_RAID_ID_INVALID)
    {
        /* see if the provided RG id is already used */
        status = fbe_create_lib_database_service_lookup_raid_object_id(
            fbe_raid_group_create_req_p->user_input.raid_group_id, 
            &rg_object_id);

        if ((status == FBE_STATUS_OK) || (rg_object_id != FBE_OBJECT_ID_INVALID))
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "rg_create_validate_param, Raid Group ID already in use %d\n",
                              fbe_raid_group_create_req_p->user_input.raid_group_id);

            *error_code = FBE_JOB_SERVICE_ERROR_RAID_GROUP_ID_IN_USE;
            return FBE_STATUS_GENERIC_FAILURE;
        }

        is_raid_group_valid = fbe_raid_group_create_is_valid_raid_group_number(
                fbe_raid_group_create_req_p->user_input.raid_group_id);

        if (is_raid_group_valid != FBE_TRUE)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "rg_create_validate_param, Invalid Raid Group number %d\n", 
                    fbe_raid_group_create_req_p->user_input.raid_group_id);
            *error_code = FBE_JOB_SERVICE_ERROR_INVALID_ID;
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (fbe_raid_group_create_req_p->user_input.raid_type == FBE_RAID_GROUP_TYPE_UNKNOWN)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "rg_create_validate_param, Invalid Raid Group type %d\n", 
                fbe_raid_group_create_req_p->user_input.raid_type);

        *error_code = FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_TYPE;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_raid_group_create_req_p->user_input.drive_count > FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "rg_create_validate_param, Invalid Raid Group drive count %d\n", 
                fbe_raid_group_create_req_p->user_input.drive_count);

        *error_code = FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT;
        fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT, NULL, 0, 
                            "%d %d",
                            fbe_raid_group_create_req_p->user_input.raid_type,
                            fbe_raid_group_create_req_p->user_input.drive_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_raid_group_create_req_p->user_input.drive_count < 1 )
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "rg_create_validate_param, Invalid Raid Group drive count %d\n", 
                fbe_raid_group_create_req_p->user_input.drive_count);

        *error_code = FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT;
        fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT, NULL, 0, 
                            "%d %d",
                            fbe_raid_group_create_req_p->user_input.raid_type,
                            fbe_raid_group_create_req_p->user_input.drive_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {   /* ask the raid group class to validate minimum raid group width */
        status = fbe_raid_group_create_validate_raid_group_class_parameters(fbe_raid_group_create_req_p);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "rg_create_validate_param, Invalid param for rg create req, stat %d\n", 
                    FBE_STATUS_GENERIC_FAILURE);
            *error_code = FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION;
            fbe_event_log_write(SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION, NULL, 0, 
                            "%d %d",
                            fbe_raid_group_create_req_p->user_input.raid_type,
                            fbe_raid_group_create_req_p->user_input.drive_count);
        }
    }

    /*
     * @todo: do the following parameters need to be validated?
     We need to determine if the disk count need to be checked for
     platform limits

     if (disk_count >= PLATFORM_FRU_COUNT) ?
     */ 
    return status;
} 
/*************************************************
 * end fbe_raid_group_create_validate_parameters()
 ************************************************/

static void fbe_raid_group_create_init_object_arrays(job_service_raid_group_create_t *fbe_raid_group_create_req_p)
{
    fbe_u32_t index = 0;

    for (index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH;  index++)
    {
        fbe_raid_group_create_req_p->provision_drive_id_array[index] = FBE_OBJECT_ID_INVALID;
        fbe_raid_group_create_req_p->physical_drive_id_array[index] = FBE_OBJECT_ID_INVALID;
        fbe_raid_group_create_req_p->virtual_drive_id_array[index] = FBE_OBJECT_ID_INVALID;
        fbe_raid_group_create_req_p->pvd_exported_capacity_array[index] = FBE_LBA_INVALID;
        //fbe_raid_group_create_req_p->configured_physical_block_size_array[index] = 
        //    FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID;
        fbe_raid_group_create_req_p->block_offset[index] = 0;
        fbe_raid_group_create_req_p->client_index[index] = 0;
    }
}


/*!**************************************************************
 * fbe_create_rg_validate_system_limits
 ****************************************************************
 * @brief
 * make sure we are allowed to create this rg on this platform.  
 *
 *
 * @return status - The status of the operation.
 ****************************************************************/
static fbe_status_t fbe_create_rg_validate_system_limits(void)
{

    fbe_database_control_get_stats_t                get_db_stats;
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;

    /*let's see how many user rg we have so far*/
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

    if (get_db_stats.num_user_raids == get_db_stats.max_allowed_user_rgs) {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: reached system limits of %d rgs\n", 
                          __FUNCTION__, get_db_stats.max_allowed_user_rgs);
                          
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*if we got here we are good to go*/
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_create_lib_check_system_rg(fbe_raid_group_number_t rg_num,
                                                fbe_object_id_t rg_id)
{
    fbe_status_t    status;
    fbe_private_space_layout_region_t region;
    fbe_object_id_t rg_object_id;
    fbe_lifecycle_state_t   state;
    fbe_u32_t               index;

    status = fbe_private_space_layout_get_region_by_raid_group_id(rg_num, &region);
    if (status != FBE_STATUS_OK) {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: get region by rg object id %d failed.\n", 
                          __FUNCTION__, rg_id);
         return status;
    }
    if (rg_id != region.raid_info.object_id) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: rg_id and rg number don't match the one defined in PSL\n", 
                          __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_create_lib_database_service_lookup_raid_object_id(rg_num, &rg_object_id);
    if (status != FBE_STATUS_NO_OBJECT) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: Rg has existed in system\n", 
                          __FUNCTION__);
        return status;
    }

    for (index = 0; index < region.number_of_frus; index++)
    {
        status = fbe_raid_group_create_get_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + index,
                                    &state);
        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: Fail to get PVD's lifecycle state. PVD object id = %d\n", 
                              __FUNCTION__, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + index);
            return status; 
        }
        if (state != FBE_LIFECYCLE_STATE_READY && state != FBE_LIFECYCLE_STATE_HIBERNATE)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: PVD is not in ready or hibernate state. PVD object id = %d\n", 
                              __FUNCTION__, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + index);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

    }

    return FBE_STATUS_OK;
}


static fbe_status_t fbe_raid_group_create_system_raid_group_and_edges(
        job_service_raid_group_create_t * rg_create_ptr)
{
    fbe_private_space_layout_region_t region;
    fbe_database_control_raid_t create_raid;
    fbe_u32_t   edge_index;
    fbe_status_t    status;

    
    status = fbe_private_space_layout_get_region_by_raid_group_id(rg_create_ptr->user_input.raid_group_id, &region);
    if (status != FBE_STATUS_OK) {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s: get region by rg object id %d failed.\n", 
                          __FUNCTION__, rg_create_ptr->user_input.rg_id);
         return status;
    }

    create_raid.object_id       = rg_create_ptr->user_input.rg_id;
    create_raid.raid_id         = rg_create_ptr->user_input.raid_group_id;
    create_raid.private_raid    = FBE_TRUE;
    create_raid.raid_configuration.width = region.number_of_frus;
    create_raid.class_id        = fbe_raid_group_create_get_class_id(region.raid_info.raid_type);

    create_raid.transaction_id  = rg_create_ptr->transaction_id;
    create_raid.user_private    = FBE_FALSE;

    create_raid.raid_configuration.capacity = region.raid_info.exported_capacity;
    create_raid.raid_configuration.raid_type = region.raid_info.raid_type;
    create_raid.raid_configuration.width = region.number_of_frus;
    create_raid.raid_configuration.chunk_size = FBE_RAID_DEFAULT_CHUNK_SIZE;
    create_raid.raid_configuration.debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
    create_raid.raid_configuration.element_size = FBE_RAID_SECTORS_PER_ELEMENT;

    if ((create_raid.raid_configuration.raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
        (create_raid.raid_configuration.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)  ||
        (create_raid.raid_configuration.raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR))
    {
        /* Mirrors do not have parity.
         */
        create_raid.raid_configuration.elements_per_parity = 0;
    }
    else
    {
        create_raid.raid_configuration.elements_per_parity = FBE_RAID_ELEMENTS_PER_PARITY;
    }
    create_raid.raid_configuration.power_saving_enabled = FBE_FALSE;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_CREATE_RAID,
                                                 &create_raid,
                                                 sizeof(create_raid),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    
    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, raid group id %d\n", 
                __FUNCTION__, 
                create_raid.object_id);
    }

    /* create edges between Raid group object and PVDs */
    for (edge_index = 0; edge_index < region.number_of_frus; edge_index++)
    {
        /* create edges between the provision drive and the virtual drive */
        status = fbe_create_lib_database_service_create_edge(rg_create_ptr->transaction_id, 
                                                             FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + edge_index, 
                                                             rg_create_ptr->user_input.rg_id, 
                                                             edge_index, 
                                                             region.size_in_blocks, 
                                                             region.starting_block_address,
                                                             NULL,
                                                             0 /*not used */);

        if (status != FBE_STATUS_OK)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry\n",__FUNCTION__);

            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "   could not attach edges between object %d and object %d, status %d\n", 
                FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + edge_index, rg_create_ptr->user_input.rg_id, status);

            return status;
        }

    }

    return status;
}

