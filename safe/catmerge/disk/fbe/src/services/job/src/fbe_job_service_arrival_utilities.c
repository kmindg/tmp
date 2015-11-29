/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_arrival_utilities.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service arrival utility functions that are required
 *  by the job service file to enqueue requests and by the job service file
 *  that processed job queue elements. 
 *  
 * @version
 *  08/24/2010 - Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_job_service_arrival_private.h"
#include "fbe_job_service_private.h"
#include "fbe_job_service.h"
#include "fbe_spare.h"
#include "fbe_job_service_operations.h"
#include "../test/fbe_job_service_test_private.h"

/*************************
 *    INLINE FUNCTIONS 
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* Job service lock accessors.
*/
void fbe_job_service_arrival_lock(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_spinlock_lock(&job_service_p->arrival.arrival_lock);
    return;
}

void fbe_job_service_arrival_unlock(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_spinlock_unlock(&job_service_p->arrival.arrival_lock);
    return;
}

/* Accessors for the number of arrival recovery objects.
*/
void fbe_job_service_increment_number_arrival_recovery_q_elements(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.number_arrival_recovery_q_elements++;
    return;
}

void fbe_job_service_decrement_number_arrival_recovery_q_elements(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.number_arrival_recovery_q_elements--;
    return;
}

fbe_u32_t fbe_job_service_get_number_arrival_recovery_q_elements(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->arrival.number_arrival_recovery_q_elements;
}

/* Accessors for the number of arrival creation objects.
*/
void fbe_job_service_increment_number_arrival_creation_q_elements(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.number_arrival_creation_q_elements++;
    return;
}

void fbe_job_service_decrement_number_arrival_creation_q_elements(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.number_arrival_creation_q_elements--;
    return;
}

fbe_u32_t fbe_job_service_get_number_arrival_creation_q_elements(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->arrival.number_arrival_creation_q_elements;
}

/* Accessors to member fields of job service
*/
void fbe_job_service_init_number_arrival_recovery_q_elements()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.number_arrival_recovery_q_elements = 0;
    return;
}

void fbe_job_service_init_number_arrival_creation_q_elements()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.number_arrival_creation_q_elements = 0;
    return;
}

void fbe_job_service_set_arrival_recovery_queue_access(fbe_bool_t value)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.arrival_recovery_queue_access = value;
    return;
}

void fbe_job_service_set_arrival_creation_queue_access(fbe_bool_t value)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.arrival_creation_queue_access = value;
    return;
}

fbe_bool_t fbe_job_service_get_arrival_creation_queue_access()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->arrival.arrival_creation_queue_access;
}

fbe_bool_t fbe_job_service_get_arrival_recovery_queue_access()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->arrival.arrival_recovery_queue_access;
}

/* job service arrival queue access */
void fbe_job_service_init_arrival_semaphore()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_semaphore_init(&job_service_p->arrival.job_service_arrival_queue_semaphore, 
            0, 
            MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS + 1);
    return;
}

/* job control arrival accessors */
void fbe_job_service_enqueue_arrival_recovery_q(fbe_job_queue_packet_element_t *entry_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_queue_push(&job_service_p->arrival.arrival_recovery_q_head, &entry_p->queue_element);
    return;
}

void fbe_job_service_dequeue_arrival_recovery_q(fbe_job_queue_packet_element_t *entry_p)
{
    fbe_queue_remove(&entry_p->queue_element);
    return;
}

void fbe_job_service_enqueue_arrival_creation_q(fbe_job_queue_packet_element_t *entry_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_queue_push(&job_service_p->arrival.arrival_creation_q_head, &entry_p->queue_element);
    return;
}

void fbe_job_service_dequeue_arrival_creation_q(fbe_job_queue_packet_element_t *entry_p)
{
    fbe_queue_remove(&entry_p->queue_element);
    return;
}

void fbe_job_service_get_arrival_recovery_queue_head(fbe_queue_head_t **queue_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    *queue_p = &job_service_p->arrival.arrival_recovery_q_head;
    return;
}

void fbe_job_service_get_arrival_creation_queue_head(fbe_queue_head_t **queue_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    *queue_p = &job_service_p->arrival.arrival_creation_q_head;
    return;
}


/*!**************************************************************
 * job_service_arrival_trace()
 ****************************************************************
 * @brief
 * Trace function for use by the job service.
 * Takes into account the current global trace level and
 * the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id  - generic identifier.
 * @param fmt...      - variable argument list starting with format.
 *
 * @return None.  
 *
 * @author
 * 08/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void job_service_arrival_trace(fbe_trace_level_t trace_level,
                               fbe_trace_message_id_t message_id,
                               const char * fmt, ...)
                               __attribute__((format(__printf_func__,3,4)));
void job_service_arrival_trace(fbe_trace_level_t trace_level,
        fbe_trace_message_id_t message_id,
        const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    /* Assume we are using the default trace level.
    */
    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    if (fbe_base_service_is_initialized(&fbe_job_service.base_service)) 
    {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_job_service.base_service);
        if (default_trace_level > service_trace_level) 
        {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level)
    {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
            FBE_SERVICE_ID_JOB_SERVICE,
            trace_level,
            message_id,
            fmt, 
            args);
    va_end(args);
}
/********************************************
 * end job_service_arrival_trace()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_get_packet_from_arrival_creation_queue()
 ****************************************************************
 * @brief
 * obtains a single packet from the arrival creation queue
 *
 * @param - packet_p             
 *
 * @return - none
 * 
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_get_packet_from_arrival_creation_queue(fbe_packet_t ** packet_p)
{
    fbe_job_queue_packet_element_t *packet_element_p;

    /* Prevent things from changing while we are processing the arrival creation queue.
    */
    fbe_job_service_arrival_lock();

    packet_element_p = 
        (fbe_job_queue_packet_element_t *)fbe_queue_front(&fbe_job_service.arrival.arrival_creation_q_head);

    fbe_queue_pop(&fbe_job_service.arrival.arrival_creation_q_head);
    fbe_job_service_decrement_number_arrival_creation_q_elements();

    /* Unlock while we process this job request.
    */    
    fbe_job_service_arrival_unlock();

    *packet_p = packet_element_p->packet_p;

    /*! when reaching this point we are done with the processing
     * of a job control packet, release the packet element
     */
    fbe_transport_release_buffer(packet_element_p);
    return;
}
/*************************************************************
 * end fbe_job_service_get_packet_from_arrival_creation_queue()
 *************************************************************/

/*!**************************************************************
 * fbe_job_service_allocate_and_fill_creation_queue_element()
 ****************************************************************
 * @brief
 * @brief
 * allocates a job service element for the creation queue
 *
 * @param - none             
 *
 * @return - status of operation
 * 
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_allocate_and_fill_creation_queue_element(fbe_packet_t * packet_p, 
        fbe_job_queue_element_t ** job_queue_element_p,
        fbe_job_type_t job_type,
        fbe_job_control_queue_type_t queue_type)
{
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t                           status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",
            __FUNCTION__);

    *job_queue_element_p = (fbe_job_queue_element_t *)fbe_transport_allocate_buffer();

    if(*job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Could not allocate memory for job request\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    (*job_queue_element_p)->job_type = job_type;
    (*job_queue_element_p)->queue_type = queue_type;
    (*job_queue_element_p)->object_id = FBE_OBJECT_ID_INVALID;
    (*job_queue_element_p)->class_id = FBE_CLASS_ID_INVALID;
    (*job_queue_element_p)->local_job_data_valid = FBE_FALSE;
    (*job_queue_element_p)->additional_job_data = NULL;

    control_code = fbe_transport_get_control_code(packet_p);

    switch(control_code) 
    {
        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
            /*get the command data out of the packet*/
            status = fbe_job_service_fill_lun_create_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;

        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
            /*get the command data out of the packet*/
            status = fbe_job_service_fill_lun_destroy_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_LUN_UPDATE:
            /*get the command data out of the packet*/
            status = fbe_job_service_fill_lun_update_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
            /*get the command data out of the packet*/
            status = fbe_job_service_fill_raid_group_create_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
            /*get the command data out of the packet*/
            status = fbe_job_service_fill_raid_group_destroy_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO:
            status = fbe_job_service_fill_system_power_save_change_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE:
            status =  fbe_job_service_fill_provision_drive_update_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
            status = fbe_job_service_fill_update_provision_drive_block_size_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
             status =  fbe_job_service_fill_provision_drive_create_request(packet_p, *job_queue_element_p);
             break;
        case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
            status =  fbe_job_service_fill_update_db_on_peer_create_request(packet_p, *job_queue_element_p);
            break;		
        case FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE:
            status =  fbe_job_service_fill_provision_drive_destroy_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
            status = fbe_job_service_fill_system_bg_service_control_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
            status = fbe_job_service_fill_ndu_commit_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO:
            status = fbe_job_service_fill_system_time_threshold_set_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CONNECT_DRIVE:
            status = fbe_job_service_fill_connect_drive_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP:
            status = fbe_job_service_fill_raid_group_update_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG:
            status =  fbe_job_service_fill_spare_config_update_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID:
            status = fbe_job_service_fill_update_multi_pvds_pool_id_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO:
            status = fbe_job_service_fill_system_encryption_change_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE:
            status = fbe_job_service_fill_update_encryption_mode_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS:
            status = fbe_job_service_fill_process_encryption_keys_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA:
            status = fbe_job_service_fill_scrub_old_user_data_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED:
            status = fbe_job_service_fill_pause_encryption_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE:
            status = fbe_job_service_fill_validate_database_request(packet_p, *job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL:
            status = fbe_job_service_fill_create_extent_pool_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN:
            status = fbe_job_service_fill_create_ext_pool_lun_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL:
            status = fbe_job_service_fill_destroy_extent_pool_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN:
            status = fbe_job_service_fill_destroy_ext_pool_lun_request(packet_p, *job_queue_element_p);
            /*add the request to the creation queue*/
            break;
        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&fbe_job_service, packet_p);
            break;
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_transport_release_buffer(*job_queue_element_p);
    }

    return status;
}
/***************************************************************
 * end fbe_job_service_allocate_and_fill_creation_queue_element()
 ***************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_packet()
 ****************************************************************
 * @brief
 * sets a packet's job number
 *
 * @param - none             
 *
 * @return - status of operation
 * 
 * @author
 * 09/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_packet(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p)

{
    fbe_payload_control_operation_opcode_t control_code;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry, job number to set %llu\n",
            __FUNCTION__, (unsigned long long)job_queue_element_p->job_number);

    control_code = fbe_transport_get_control_code(packet_p);

    switch(control_code) 
    {
        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
            fbe_job_service_set_job_number_on_lun_create_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
            fbe_job_service_set_job_number_on_lun_destroy_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_LUN_UPDATE:
            /*get the command data out of the packet*/
            fbe_job_service_set_job_number_on_lun_update_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
            fbe_job_service_set_job_number_on_raid_group_create_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
            fbe_job_service_set_job_number_on_raid_group_destroy_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO:
            fbe_job_service_set_job_number_on_system_power_save_change_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP:
            fbe_job_service_set_job_number_on_raid_group_update_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
            fbe_job_service_set_job_number_on_drive_swap_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
            fbe_job_service_set_job_number_on_virtual_drive_update_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
            fbe_job_service_set_job_number_on_provision_drive_create_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE:
            fbe_job_service_set_job_number_on_provision_drive_update_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
            fbe_job_service_set_job_number_on_update_provision_drive_block_size_request(packet_p, job_queue_element_p);;
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG:
            fbe_job_service_set_job_number_on_spare_config_update_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
            fbe_job_service_set_job_number_on_db_update_on_peer_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE:
            fbe_job_service_set_job_number_on_provision_drive_destroy_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
            fbe_job_service_set_job_number_on_system_bg_service_control_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
            fbe_job_service_set_job_number_on_ndu_commit_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO:
            fbe_job_service_set_job_number_on_system_time_threshold_set_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CONNECT_DRIVE:
            fbe_job_service_set_job_number_on_drive_connect_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID:
            fbe_job_service_set_job_number_on_update_multi_pvds_pool_id_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG:
            fbe_job_service_set_job_number_on_update_spare_library_config_request(packet_p, job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO:
            fbe_job_service_set_job_number_on_system_encryption_change_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE:
            fbe_job_service_set_job_number_on_update_encryption_mode_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS:
            fbe_job_service_set_job_number_on_process_encryption_keys_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA:
            fbe_job_service_set_job_number_on_scrub_old_user_data_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED:
            fbe_job_service_set_job_number_on_pause_encryption_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE:
            fbe_job_service_set_job_number_on_validate_database_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL:
            fbe_job_service_set_job_number_on_create_extent_pool_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN:
            fbe_job_service_set_job_number_on_create_ext_pool_lun_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL:
            fbe_job_service_set_job_number_on_destroy_extent_pool_request(packet_p, job_queue_element_p);
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN:
            fbe_job_service_set_job_number_on_destroy_ext_pool_lun_request(packet_p, job_queue_element_p);
            break;
        default:
                job_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s illegal  code %d\n",
                                  __FUNCTION__, control_code);
    }
    return;
}
/************************************************
 * end fbe_job_service_set_job_number_on_packet()
 ************************************************/

/*!**************************************************************
 * fbe_job_service_get_packet_from_arrival_recovery_queue()
 ****************************************************************
 * @brief
 * obtains a single packet from the arrival recovery queue
 *
 * @param - packet_p             
 *
 * @return - none
 * 
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_get_packet_from_arrival_recovery_queue(fbe_packet_t ** packet_p)
{
    fbe_job_queue_packet_element_t *packet_element_p;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n",
            __FUNCTION__);

    /* Prevent things from changing while we are processing the arrival recovery queue.
    */
    fbe_job_service_arrival_lock();

    packet_element_p =
        (fbe_job_queue_packet_element_t *)fbe_queue_front(&fbe_job_service.arrival.arrival_recovery_q_head);

    fbe_queue_pop(&fbe_job_service.arrival.arrival_recovery_q_head);
    fbe_job_service_decrement_number_arrival_recovery_q_elements();

    /* Unlock while we process this job request.
    */    
    fbe_job_service_arrival_unlock();
    *packet_p = packet_element_p->packet_p;

    /*! when reaching this point we are done with the processing
     * of a job control packet element, release the element
     */
    fbe_transport_release_buffer(packet_element_p);

    return;
}
/*************************************************************
 * end fbe_job_service_get_packet_from_arrival_recovery_queue()
 *************************************************************/

/*!**************************************************************
 * fbe_job_service_allocate_and_fill_recovery_queue_element()
 ****************************************************************
 * @brief
 * allocates a job service element for the recovery queue
 *
 * @param - packet_p            
 * @param - job_queue_element_p            
 *
 * @return - status of operation
 * 
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_allocate_and_fill_recovery_queue_element(fbe_packet_t *packet_p, 
        fbe_job_queue_element_t ** job_queue_element_p, 
        fbe_job_type_t job_type, 
        fbe_job_control_queue_type_t queue_type)
{
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t             status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n",
            __FUNCTION__);

    *job_queue_element_p = (fbe_job_queue_element_t *)fbe_transport_allocate_buffer();

    if(*job_queue_element_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    (*job_queue_element_p)->job_type = job_type;
    (*job_queue_element_p)->queue_type = queue_type;
    (*job_queue_element_p)->object_id = FBE_OBJECT_ID_INVALID;
    (*job_queue_element_p)->class_id = FBE_CLASS_ID_INVALID;

    control_code = fbe_transport_get_control_code(packet_p);

    switch(control_code) 
    {
        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
            /* get the drive swap command data out of the packet. */
            status = fbe_job_service_fill_drive_swap_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
            status = fbe_job_service_fill_virtual_drive_update_request(packet_p, *job_queue_element_p);
            break;

        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG:
            status = fbe_job_service_fill_update_spare_library_config_request(packet_p, *job_queue_element_p);
            break;

        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&fbe_job_service, packet_p);
            break;
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_transport_release_buffer(*job_queue_element_p);
    }

    return status;
}
/*****************************************************
 * end fbe_job_service_allocate_and_fill_recovery_queue_element()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_add_element_to_arrival_recovery_queue()
 ****************************************************************
 * @brief
 * This function adds an element to the arrival recovery queue
 *
 * @param packet_p - packet container for the 
 *                   recovery queue
 *
 * @return - fbe_status_t.
 *
 * @author
 * 08/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_add_element_to_arrival_recovery_queue(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_job_queue_packet_element_t         *job_packet_element_p = NULL;
    fbe_atomic_t			               js_gate = 0;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    if(fbe_job_service.arrival.number_arrival_recovery_q_elements >= MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS)
    {
        job_service_arrival_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Invalid number of arrival packets 0x%x\n", __FUNCTION__, 
                fbe_job_service.arrival.number_arrival_recovery_q_elements);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Check if service is shutting down */
    js_gate = fbe_atomic_increment(&fbe_job_service.arrival.outstanding_requests);
    if(js_gate & FBE_JOB_SERVICE_GATE_BIT){ 
        /* We have a gate, so fail the request */
        job_service_arrival_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job Service is shutting down\n", __FUNCTION__);

        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! get the job service packet request */
    job_packet_element_p = (fbe_job_queue_packet_element_t *)fbe_transport_allocate_buffer();
    if(job_packet_element_p == NULL)
    {
        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Could not allocate memory for job packet\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_packet_element_p->packet_p = packet_p;

    /*! Lock arrival recovery queue 
    */
    fbe_job_service_arrival_lock();

    /*! Queue the request */
    fbe_job_service_increment_number_arrival_recovery_q_elements();
    fbe_job_service_enqueue_arrival_recovery_q(job_packet_element_p);

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s, number arrival packets 0x%x\n",
            __FUNCTION__, fbe_job_service.arrival.number_arrival_recovery_q_elements);

    /* Unlock since we are done. */
    fbe_job_service_arrival_unlock();

    /* Release the semaphore only if the thread is in RUN mode.. It may be in SYNC mode.
     * In that case, the SYNC operation itself releases the semaphore once done... 
     */
    if(fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN))
    {
        fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);        
    }
    fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

    return FBE_STATUS_OK;
}
/*************************************************************
 * end fbe_job_service_add_element_to_arrival_recovery_queue()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_add_element_to_arrival_creation_queue()
 ****************************************************************
 * @brief
 * This function adds an element to the arrival creation queue
 *
 * @param packet_p - packet container for the 
 *                   recovery queue
 *
 * @return - fbe_status_t.
 *
 * @author
 * 08/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_add_element_to_arrival_creation_queue(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_job_queue_packet_element_t         *job_packet_element_p = NULL;
    fbe_atomic_t                           js_gate = 0;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    if (fbe_job_service.arrival.number_arrival_creation_q_elements >= MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS)
    {
        job_service_arrival_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Invalid number of arrival objects 0x%x\n", __FUNCTION__, 
                fbe_job_service.arrival.number_arrival_creation_q_elements);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Check if service is shutting down */
    js_gate = fbe_atomic_increment(&fbe_job_service.arrival.outstanding_requests);
    if(js_gate & FBE_JOB_SERVICE_GATE_BIT){ 
        /* We have a gate, so fail the request */
        job_service_arrival_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job Service is shutting down\n", __FUNCTION__);

        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /*! get the job service packet request */
    job_packet_element_p = (fbe_job_queue_packet_element_t *)fbe_transport_allocate_buffer();
    if(job_packet_element_p == NULL)
    {
        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
        "%s : ERROR: job request is NULL \n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_packet_element_p->packet_p = packet_p;

    /*! Lock arrival creation queue */
    fbe_job_service_arrival_lock();

    /*! Queue the packet */
    fbe_job_service_increment_number_arrival_creation_q_elements();
    fbe_job_service_enqueue_arrival_creation_q(job_packet_element_p);

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s, number arrival creation objects 0x%x\n",
            __FUNCTION__, fbe_job_service.arrival.number_arrival_creation_q_elements);

    /* Unlock since we are done. */
    fbe_job_service_arrival_unlock();

    /* Release the semaphore only if the thread is in RUN mode.. It may be in SYNC mode.
     * In that case, the SYNC operation itself releases the semaphore once done... 
     */
    if(fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN))
    {
        fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);        
    }
    fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_job_service_add_element_to_arrival_creation_queue()
 ***********************************************************/

/*!**************************************************************
 * fbe_job_service_print_objects_from_arrival_recovery_queue()
 ****************************************************************
 * @brief
 * Prints all objects from arrival recovery queue
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  08/23/2010 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_print_objects_from_arrival_recovery_queue(fbe_u32_t caller)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;

    /*! Lock queue */
    fbe_job_service_arrival_lock();

    fbe_job_service_get_arrival_recovery_queue_head(&head_p);
    request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        fbe_job_queue_element_t *next_request_p = NULL;
        job_service_arrival_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "arrival recovery queue--Lock(%d)::object id %d \n", 
                caller,
                request_p->object_id);
        next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
        request_p = next_request_p;
    }

    fbe_job_service_arrival_unlock();
}
/*****************************************************************
 * end fbe_job_service_print_objects_from_arrival_recovery_queue()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_print_objects_from_arrival_creation_queue()
 ****************************************************************
 * @brief
 * Prints all objects from arrival creation queue
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  08/23/2010 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_print_objects_from_arrival_creation_queue(fbe_u32_t caller)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;

    /*! Lock queue */
    fbe_job_service_arrival_lock();

    fbe_job_service_get_arrival_creation_queue_head(&head_p);
    request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        fbe_job_queue_element_t *next_request_p = NULL;
        job_service_arrival_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "arrival recovery queue--Lock(%d)::object id %d \n", 
                caller,
                request_p->object_id);
        next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
        request_p = next_request_p;
    }

    fbe_job_service_arrival_unlock();
}
/*****************************************************************
 * end fbe_job_service_print_objects_from_arrival_creation_queue()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_process_arrival_recovery_queue_access()
 ****************************************************************
 * @brief
 * process a job control request to disable/enable processing
 * of job elements from the arrival recovery queue
 *
 * @param packet_p - packet containing enable/disable request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        to either disable or enable the access
 *                        to the recovery queue
 * @author
 * 08/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_process_arrival_recovery_queue_access(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    job_service_arrival_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Lock queue */
    fbe_job_service_arrival_lock();
    if (fbe_job_service_get_arrival_recovery_queue_access() != job_queue_element_p->queue_access)
    {
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
        fbe_job_service_set_arrival_recovery_queue_access(job_queue_element_p->queue_access);
    }
    /*! Unlock queue */
    fbe_job_service_arrival_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*************************************************************
 * end fbe_job_service_process_arrival_recovery_queue_access()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_process_arrival_creation_queue_access()
 ****************************************************************
 * @brief
 * process a job control request to disable/enable processing
 * of job elements from the arrival creation queue
 *
 * @param packet_p - packet containing enable/disable request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        to either disable or enable the access
 *                        to the recovery queue
 * @author
 * 08/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_process_arrival_creation_queue_access(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    job_service_arrival_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Lock queue */
    fbe_job_service_arrival_lock();
    if (fbe_job_service_get_arrival_creation_queue_access() != job_queue_element_p->queue_access)
    {
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
        fbe_job_service_set_arrival_creation_queue_access(job_queue_element_p->queue_access);
    }
    /*! Unlock queue */
    fbe_job_service_arrival_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*************************************************************
 * end fbe_job_service_process_arrival_creation_queue_access()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_build_arrival_queue_element
 ****************************************************************
 * @brief
 * This function inits a job service arrival queue element
 * data structure fields
 * 
 * @param job_control_element - data structure to init
 * @param job_control_code    - external code 
 * @param object_id           - the ID of the target object
 * @param command_data_p      - pointer to source data
 * @param command_size        - size of source data
 *
 * @return status - status of init call
 *
 * @author
 * 08/23/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t fbe_job_service_build_arrival_queue_element(fbe_job_queue_element_t *in_job_queue_element_p,
        fbe_payload_control_operation_opcode_t in_control_code,
        fbe_object_id_t in_object_id,
        fbe_u8_t *in_command_data_p,
        fbe_u32_t in_command_size)
{
    fbe_job_type_t job_type = FBE_JOB_TYPE_INVALID;

    /* Context size cannot be greater than job element context size.
    */
    if (in_command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    in_job_queue_element_p->object_id = in_object_id;

    job_type = fbe_job_service_get_job_type_by_control_type(in_control_code);

    if (job_type == FBE_JOB_TYPE_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_job_queue_element_p->job_type = job_type;
    in_job_queue_element_p->timestamp = 0;
    in_job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_INVALID;
    in_job_queue_element_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    in_job_queue_element_p->queue_access = FBE_FALSE;
    in_job_queue_element_p->num_objects = 0;
    in_job_queue_element_p->job_number = 0;

    if (in_command_data_p != NULL)
    {
        fbe_copy_memory(in_job_queue_element_p->command_data, in_command_data_p, in_command_size);
    }
    return FBE_STATUS_OK;
}
/***************************************************
 * end fbe_job_service_build_arrival_queue_element()
 **************************************************/


/*!**************************************************************
 * fbe_job_service_remove_all_objects_from_arrival_recovery_q()
 ****************************************************************
 * @brief
 *  Removes all elements from Arrival recovery queue. This is
 *  required before actually destroying the queue.
 *
 * @param none  
 *
 * @return none
 *
 * @author
 *  07/11/2011 - Created. Arun S 
 *
 ****************************************************************/

void fbe_job_service_remove_all_objects_from_arrival_recovery_q()
{
    fbe_packet_t                     *packet_p = NULL;
    fbe_job_queue_element_t          *request_p = NULL;
    fbe_job_queue_packet_element_t   *job_packet_element_p = NULL;
    fbe_payload_ex_t                    *payload_p = NULL;
    fbe_payload_control_operation_t  *control_operation = NULL;
    fbe_u32_t number_recovery_objects = 0;

    number_recovery_objects = fbe_job_service_get_number_arrival_recovery_q_elements();

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "Arrival Recovery Q - Outstanding object count: %d\n", number_recovery_objects);

    while (fbe_queue_is_empty(&fbe_job_service.arrival.arrival_recovery_q_head) == FBE_FALSE)
    {
        job_packet_element_p = 
            (fbe_job_queue_packet_element_t *)fbe_queue_pop(&fbe_job_service.arrival.arrival_recovery_q_head);

        fbe_job_service_decrement_number_arrival_recovery_q_elements();

        request_p = (fbe_job_queue_element_t *) &job_packet_element_p->queue_element;
        packet_p = job_packet_element_p->packet_p;

        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);

        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "removing object with job #%llu from arrival recovery queue\n",
        (unsigned long long)request_p->job_number);

        number_recovery_objects = number_recovery_objects - 1;

        /* Complete the packet */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_COMPLETE_JOB_DESTROY, 0);
        fbe_transport_complete_packet(packet_p);

        /*! when reaching this point we are done with the processing
         * of a job control packet, release the packet element
         */
        fbe_transport_release_buffer(job_packet_element_p);
    }

    return;
}
/*******************************************************************
 * end fbe_job_service_remove_all_objects_from_arrival_recovery_q()
 *******************************************************************/

/*!**************************************************************
 * fbe_job_service_remove_all_objects_from_arrival_creation_q()
 ****************************************************************
 * @brief
 *  Removes all elements from Arrival creation queue. This is
 *  required before actually destroying the queue.
 *
 * @param none  
 *
 * @return none
 *
 * @author
 *  07/11/2011 - Created. Arun S 
 *
 ****************************************************************/

void fbe_job_service_remove_all_objects_from_arrival_creation_q()
{
    fbe_packet_t                     *packet_p = NULL;
    fbe_job_queue_element_t          *request_p = NULL;
    fbe_job_queue_packet_element_t   *job_packet_element_p = NULL;
    fbe_payload_ex_t                    *payload_p = NULL;
    fbe_payload_control_operation_t  *control_operation = NULL;
    fbe_u32_t number_recovery_objects = 0;

    number_recovery_objects = fbe_job_service_get_number_arrival_creation_q_elements();

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "Arrival creation Q - Outstanding object count: %d\n", number_recovery_objects);

    while (fbe_queue_is_empty(&fbe_job_service.arrival.arrival_creation_q_head) == FBE_FALSE)
    {
        job_packet_element_p = 
            (fbe_job_queue_packet_element_t *)fbe_queue_pop(&fbe_job_service.arrival.arrival_creation_q_head);

        fbe_job_service_decrement_number_arrival_creation_q_elements();

        request_p = (fbe_job_queue_element_t *) &job_packet_element_p->queue_element;
        packet_p = job_packet_element_p->packet_p;

        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);

        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "removing object with job #%llu from arrival creation queue\n",
        (unsigned long long)request_p->job_number);

        number_recovery_objects = number_recovery_objects - 1;

        /* Complete the packet */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_COMPLETE_JOB_DESTROY, 0);
        fbe_transport_complete_packet(packet_p);

        /*! when reaching this point we are done with the processing
         * of a job control packet, release the packet element
         */
        fbe_transport_release_buffer(job_packet_element_p);
    }

    return;
}
/*******************************************************************
 * end fbe_job_service_remove_all_objects_from_arrival_creation_q()
 *******************************************************************/


/*!****************************************************
 * job_service_drive_swap_set_default_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for drive swap job command
 *
 * @param[in] command_data - the command data buffer
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_drive_swap_set_default_command_values(fbe_u8_t * command_data)
{
    fbe_job_service_drive_swap_request_t   * request = (fbe_job_service_drive_swap_request_t*)command_data;

    /*currently we just zero the memory of this command
     *in future, the owner of this job should set default values for the command*/
    fbe_zero_memory(request, sizeof(fbe_job_service_drive_swap_request_t));
}

/*!****************************************************
 * job_service_update_raid_group_set_default_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for update raid group job command
 *
 * @param[in] command_data - the command data buffer
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_update_raid_group_set_default_command_values(fbe_u8_t * command_data)
{
    fbe_job_service_update_raid_group_t* request = (fbe_job_service_update_raid_group_t*)command_data;

    /*currently we just zero the memory of this command
     *in future, the owner of this job should set default values for the command*/
    fbe_zero_memory(request, sizeof(fbe_job_service_update_raid_group_t));
}

/*!****************************************************
 * job_service_update_virtual_drive_set_default_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for update virtual drive job command
 *
 * @param[in] command_data - the command data buffer
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_update_virtual_drive_set_default_command_values(fbe_u8_t * command_data)
{
    fbe_job_service_update_virtual_drive_t* request = (fbe_job_service_update_virtual_drive_t*)command_data;

    /*currently we just zero the memory of this command
     *in future, the owner of this job should set default values for the command*/
    fbe_zero_memory(request, sizeof(fbe_job_service_update_virtual_drive_t));
}

/*!****************************************************
 * job_service_update_spare_config_set_default_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for update spare config job command
 *
 * @param[in] command_data - the command data buffer
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_update_spare_config_set_default_command_values(fbe_u8_t * command_data)
{
    fbe_job_service_update_spare_config_t* request = (fbe_job_service_update_spare_config_t*)command_data;

    /*currently we just zero the memory of this command
     *in future, the owner of this job should set default values for the command*/
    fbe_zero_memory(request, sizeof(fbe_job_service_update_spare_config_t));
}


/*!****************************************************
 * job_service_update_db_on_peer_set_default_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for update db on peer command
 *
 * @param[in] command_data - the command data buffer
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_update_db_on_peer_set_default_command_values(fbe_u8_t * command_data)
{
    fbe_job_service_update_db_on_peer_t* request = (fbe_job_service_update_db_on_peer_t*)command_data;

    /*currently we just zero the memory of this command
     *in future, the owner of this job should set default values for the command*/
    fbe_zero_memory(request, sizeof(fbe_job_service_update_db_on_peer_t));
}

/*!****************************************************
 * job_service_sep_ndu_commit_set_default_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for sep ndu commit job command
 *
 * @param[in] command_data - the command data buffer
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_sep_ndu_commit_set_default_command_values(fbe_u8_t * command_data)
{
    fbe_job_service_sep_ndu_commit_t* request = (fbe_job_service_sep_ndu_commit_t*)command_data;

    /*currently we just zero the memory of this command
     *in future, the owner of this job should set default values for the command*/
    fbe_zero_memory(request, sizeof(fbe_job_service_sep_ndu_commit_t));
}

/*!****************************************************
 * job_service_set_default_job_command_values()
 ******************************************************
 * @brief
 * This is function to set default values for command data when receiving
 * a job element from peer.
 *
 * @param[in] command_data - the command data buffer
 * @param[in] job_type - the job type of this job element
 *
 * @return - none
 *
 * @author
 * 5/07/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/
void job_service_set_default_job_command_values(fbe_u8_t * command_data, fbe_job_type_t job_type)
{
    if(NULL == command_data)
        return;

    switch(job_type)
    {
            case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
                job_service_drive_swap_set_default_command_values(command_data);
                break;
        
            case FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP:
                job_service_update_raid_group_set_default_command_values(command_data);
                break;
        
            case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
                job_service_update_virtual_drive_set_default_command_values(command_data);
                break;
        
            case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG:
                job_service_update_spare_config_set_default_command_values(command_data);
                break;
        
            case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
                job_service_update_db_on_peer_set_default_command_values(command_data);
                break;

            case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
                job_service_sep_ndu_commit_set_default_command_values(command_data);
                break;

            default:
                /* all creation jobs (except UPDATE_DB_ON_PEER and SEP_NDU_COMMIT) do not 
                * need set default values when receiving peer's job element because the version of 
                * job commands on both SPs are always consistent (in ndu stage, no creation job is
                * allowed. Outof ndu stage, the versions between two sides are always consistent).
                * Recovery jobs and the two creation jobs (UPDATE_DB_ON_PEER and SEP_NDU_COMMIT)
                * need set default values because only these jobs are allowed in ndu commit stage. */
                return;
    }
    
}

