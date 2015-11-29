/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_job_service_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe job service interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_job_service_interface
 *
 * @version
 *   11/24/2009:  Created. Bo Gao
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_private_space_layout.h"

/**************************************
                Local variables
**************************************/
#define FBE_JOB_SERVICE_WAIT_TIMEOUT       10000  /*ms*/
#define FBE_JOB_SERVICE_WAIT_TO_DESTROY_SYS_PVD_TIMEOUT 600000  /*ms*/
#define FBE_JOB_SERVICE_WAIT_ENCRYPTION_TIMEOUT	30000 /*ms*/
/*******************************************
                Local functions
********************************************/
static fbe_status_t fbe_api_job_service_process_encryption_keys(fbe_api_job_service_process_encryption_keys_t *in_process_encryption_keys_p);

/*!***************************************************************
 * @fn fbe_api_job_service_process_command(
 *       fbe_job_queue_element_t *in_fbe_job_queue_element_p,
 *       fbe_job_service_control_t    in_control_code,
 *       fbe_packet_attr_t            in_attribute)
 ****************************************************************
 * @brief
 *  This function sends a generic request to the job service which
 *  requires no enqueing by the job service
 *
 * @param in_fbe_job_queue_element_p  - data structure to be queued by the job service
 * @param in_control_code             - generic control code
 * @param in_attribute                - attribute to packet 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  11/24/09 - Created. Bo Gao
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_process_command(
        fbe_job_queue_element_t *in_fbe_job_queue_element_p,
        fbe_job_service_control_t    in_control_code,
        fbe_packet_attr_t            in_attribute)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_job_queue_element_t                 job_queue_element;
    fbe_status_t                            status = FBE_STATUS_OK;

    if (in_fbe_job_queue_element_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_queue_element.job_type = in_fbe_job_queue_element_p->job_type;
    job_queue_element.queue_access = in_fbe_job_queue_element_p->queue_access;
    job_queue_element.num_objects = in_fbe_job_queue_element_p->num_objects;
    job_queue_element.object_id = in_fbe_job_queue_element_p->object_id;

    status = fbe_api_common_send_control_packet_to_service (in_control_code,
                                                            &job_queue_element,
                                                            sizeof(fbe_job_queue_element_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            in_attribute,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, 
                       status_info.packet_qualifier, 
                       status_info.control_operation_status, 
                       status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} // end fbe_api_job_service_process_command()

/*!***************************************************************
 * @fn fbe_api_job_service_add_element_to_queue_test(fbe_job_queue_element_t *in_fbe_job_queue_element_p,
                                    fbe_packet_attr_t            in_attribute)
 ****************************************************************
 * @brief
 *  This function sends a generic request to the job service to be queued.
 *
 * @param in_fbe_job_queue_element_p  - data structure to be queued by the job service
 * @param in_attribute                - attribute to packet 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  11/24/09 - Created. Bo Gao
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_add_element_to_queue_test(
        fbe_job_queue_element_t *in_fbe_job_queue_element_p,
        fbe_packet_attr_t in_attribute)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_job_queue_element_t                 job_queue_element;
    fbe_status_t                            status = FBE_STATUS_OK;

    if (in_fbe_job_queue_element_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (in_fbe_job_queue_element_p->command_data != NULL){
        memcpy(job_queue_element.command_data, in_fbe_job_queue_element_p->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);
    }
    job_queue_element.job_type      = in_fbe_job_queue_element_p->job_type;
    job_queue_element.job_action    = in_fbe_job_queue_element_p->job_action;
    job_queue_element.object_id     = in_fbe_job_queue_element_p->object_id;
    job_queue_element.queue_element = in_fbe_job_queue_element_p->queue_element;
    job_queue_element.current_state = in_fbe_job_queue_element_p->current_state;
    job_queue_element.timestamp     = in_fbe_job_queue_element_p->timestamp;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_ADD_ELEMENT_TO_QUEUE_TEST,
                                                            &job_queue_element,
                                                            sizeof(fbe_job_queue_element_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            in_attribute,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} // end fbe_api_job_service_add_element_to_queue_test()

/*!***************************************************************
 * @fn fbe_api_job_service_lun_create(fbe_api_job_service_lun_create_t *in_fbe_lun_create_req_p)
 ****************************************************************
 * @brief
 *  This function submits a lun create request to job service.
 *
 * @param in_fbe_lun_create_req_p - data structure containing lun 
 *                                  creation request
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  01/06/10 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_lun_create(
        fbe_api_job_service_lun_create_t *in_fbe_lun_create_req_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_lun_create_t            lun_create;

    if (in_fbe_lun_create_req_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&lun_create, sizeof(lun_create));
    lun_create.raid_group_id = in_fbe_lun_create_req_p->raid_group_id;
    lun_create.raid_type = in_fbe_lun_create_req_p->raid_type;
    lun_create.lun_number = in_fbe_lun_create_req_p->lun_number;
    lun_create.capacity = in_fbe_lun_create_req_p->capacity; 
    lun_create.placement = in_fbe_lun_create_req_p->placement;
    lun_create.addroffset = in_fbe_lun_create_req_p->addroffset;
    lun_create.ndb_b = in_fbe_lun_create_req_p->ndb_b;
    lun_create.noinitialverify_b = in_fbe_lun_create_req_p->noinitialverify_b;
    lun_create.bind_time = in_fbe_lun_create_req_p->bind_time;
    lun_create.user_private = in_fbe_lun_create_req_p->user_private;
    lun_create.wait_ready = in_fbe_lun_create_req_p->wait_ready;
    lun_create.ready_timeout_msec = in_fbe_lun_create_req_p->ready_timeout_msec;
    lun_create.export_lun_b = in_fbe_lun_create_req_p->export_lun_b;
    

    if (in_fbe_lun_create_req_p->is_system_lun)
    {
        lun_create.is_system_lun = in_fbe_lun_create_req_p->is_system_lun;
        lun_create.lun_object_id = in_fbe_lun_create_req_p->lun_id;
    }
    fbe_copy_memory (&lun_create.world_wide_name, &in_fbe_lun_create_req_p->world_wide_name, sizeof(lun_create.world_wide_name));
    fbe_copy_memory (&lun_create.user_defined_name, &in_fbe_lun_create_req_p->user_defined_name, sizeof(lun_create.user_defined_name));

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_LUN_CREATE,
                                                            &lun_create,
                                                            sizeof(fbe_job_service_lun_create_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        in_fbe_lun_create_req_p->job_number = lun_create.job_number;
    return status;

} // end fbe_api_job_service_lun_create()

/*!***************************************************************
 * @fn fbe_api_job_service_lun_destroy(fbe_api_job_service_lun_destroy_t *in_fbe_lun_destroy_req_p)
 ****************************************************************
 * @brief
 *  This function submit a lun destroy request to job service.
 *
 * @param in_fbe_lun_destroy_req_p - data structure containing lun 
 *                                   destroy request
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  01/06/10 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_lun_destroy(
        fbe_api_job_service_lun_destroy_t *in_fbe_lun_destroy_req_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_lun_destroy_t           lun_destroy;

    fbe_zero_memory(&lun_destroy, sizeof(fbe_job_service_lun_destroy_t));
    if (in_fbe_lun_destroy_req_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_destroy.lun_number = in_fbe_lun_destroy_req_p->lun_number;
    lun_destroy.allow_destroy_broken_lun = in_fbe_lun_destroy_req_p->allow_destroy_broken_lun;
    lun_destroy.wait_destroy = in_fbe_lun_destroy_req_p->wait_destroy;
    lun_destroy.destroy_timeout_msec = in_fbe_lun_destroy_req_p->destroy_timeout_msec;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_LUN_DESTROY,
                                                            &lun_destroy,
                                                            sizeof(fbe_job_service_lun_destroy_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_fbe_lun_destroy_req_p->job_number = lun_destroy.job_number;
    return status;

} // end end fbe_api_job_service_lun_destroy()

/*!***************************************************************
 * @fn fbe_api_job_service_lun_update(fbe_api_job_service_lun_update_t *in_fbe_lun_update_req_p)
 ****************************************************************
 * @brief
 *  This function submits a lun update request to job service.
 *
 * @param in_fbe_lun_update_req_p - data structure containing lun 
 *                                  update request
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  01/06/10 - Created. guov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_lun_update(
             fbe_api_job_service_lun_update_t *in_fbe_lun_update_req_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_lun_update_t            lun_update;

    if (in_fbe_lun_update_req_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_update.update_lun.object_id = in_fbe_lun_update_req_p->object_id; 
    lun_update.update_lun.update_type = in_fbe_lun_update_req_p->update_type;
    lun_update.update_lun.attributes = in_fbe_lun_update_req_p->attributes;
    fbe_copy_memory (&lun_update.update_lun.user_defined_name, &in_fbe_lun_update_req_p->user_defined_name, sizeof(lun_update.update_lun.user_defined_name));
    fbe_copy_memory (&lun_update.update_lun.world_wide_name, &in_fbe_lun_update_req_p->world_wide_name, sizeof(lun_update.update_lun.world_wide_name));

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_LUN_UPDATE,
                                                            &lun_update,
                                                            sizeof(fbe_job_service_lun_update_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_fbe_lun_update_req_p->job_number = lun_update.job_number;
    return status;

} // end fbe_api_job_service_lun_update()


/*!***************************************************************
 * @fn fbe_api_job_service_raid_group_create(
 *   fbe_api_job_service_raid_group_create_t *in_fbe_raid_group_create_req_p)
 ****************************************************************
 * @brief
 *  This function submit a raid group create request to job service
 *
 * @param in_fbe_raid_group_create_req_p - data structure containing 
 *                                         raid group creation request
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  01/13/10 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_raid_group_create(
        fbe_api_job_service_raid_group_create_t *in_fbe_raid_group_create_req_p)
{
    fbe_api_control_operation_status_info_t status_info = {0};
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_raid_group_create_t     raid_group_create = {0};
    fbe_u32_t                               drive_index = 0;
    fbe_u32_t                               valid_width = 0;  

    if (in_fbe_raid_group_create_req_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note The API should not be executing any sanity except to prevent
     *        data corruption.  Therefore we allow invalid width but won't
     *        populate those disk positions that are beyond FBE_RAID_MAX_DISK_ARRAY_WIDTH.
     */
    if (in_fbe_raid_group_create_req_p->drive_count > FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                      "%s: Drive count: %d is beyond max: %d. Only: %d positions will be populated\n", 
                      __FUNCTION__, in_fbe_raid_group_create_req_p->drive_count,
                      FBE_RAID_MAX_DISK_ARRAY_WIDTH, FBE_RAID_MAX_DISK_ARRAY_WIDTH);
    }
    valid_width = FBE_MIN(FBE_RAID_MAX_DISK_ARRAY_WIDTH, in_fbe_raid_group_create_req_p->drive_count);

    if (in_fbe_raid_group_create_req_p->is_system_rg)
    {
        raid_group_create.is_system_rg = FBE_TRUE;
        raid_group_create.rg_id = in_fbe_raid_group_create_req_p->rg_id;
        raid_group_create.raid_group_id = in_fbe_raid_group_create_req_p->raid_group_id;
    }

    raid_group_create.drive_count = in_fbe_raid_group_create_req_p->drive_count;

    /* copy list of drives */
    for (drive_index = 0; drive_index < valid_width; drive_index++) {
        /* set bus value */
        raid_group_create.disk_array[drive_index].bus = 
            in_fbe_raid_group_create_req_p->disk_array[drive_index].bus;

        /* set enclusre value */
        raid_group_create.disk_array[drive_index].enclosure = 
            in_fbe_raid_group_create_req_p->disk_array[drive_index].enclosure;

        /* set slot value */
        raid_group_create.disk_array[drive_index].slot = 
            in_fbe_raid_group_create_req_p->disk_array[drive_index].slot;
    }

    raid_group_create.b_bandwidth           = in_fbe_raid_group_create_req_p->b_bandwidth;
    raid_group_create.raid_group_id         = in_fbe_raid_group_create_req_p->raid_group_id;
    raid_group_create.drive_count           = in_fbe_raid_group_create_req_p->drive_count;
    raid_group_create.capacity              = in_fbe_raid_group_create_req_p->capacity;
    raid_group_create.power_saving_idle_time_in_seconds = in_fbe_raid_group_create_req_p->power_saving_idle_time_in_seconds; 
    raid_group_create.max_raid_latency_time_in_seconds = in_fbe_raid_group_create_req_p->max_raid_latency_time_is_sec;
    raid_group_create.raid_type             = in_fbe_raid_group_create_req_p->raid_type;
    //raid_group_create.explicit_removal      = in_fbe_raid_group_create_req_p->explicit_removal;
    raid_group_create.is_private            = in_fbe_raid_group_create_req_p->is_private;
    raid_group_create.user_private          = in_fbe_raid_group_create_req_p->user_private;
    raid_group_create.wait_ready            = in_fbe_raid_group_create_req_p->wait_ready;
    raid_group_create.ready_timeout_msec    = in_fbe_raid_group_create_req_p->ready_timeout_msec;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE,
                                                            &raid_group_create,
                                                            sizeof(fbe_job_service_raid_group_create_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        in_fbe_raid_group_create_req_p->job_number = raid_group_create.job_number;
        return status;
}
/*********************************************
 * end fbe_api_job_service_raid_group_create()
 ********************************************/

/*!***************************************************************
 * @fn fbe_api_job_service_raid_group_destroy(
 *    fbe_api_job_service_raid_group_destroy_t *in_fbe_raid_group_destroy_req_p)
 ****************************************************************
 * @brief
 *  This function submit a raid group destroy request to job service
 *
 * @param in_fbe_raid_group_destroy_req_p - data structure containing 
 *                                          raid group destroy request
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  01/13/10 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_raid_group_destroy(
        fbe_api_job_service_raid_group_destroy_t *in_fbe_raid_group_destroy_req_p)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_raid_group_destroy_t    raid_group_destroy;

    fbe_zero_memory(&raid_group_destroy, sizeof(fbe_job_service_raid_group_destroy_t));
    if (in_fbe_raid_group_destroy_req_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_group_destroy.raid_group_id = in_fbe_raid_group_destroy_req_p->raid_group_id;
    raid_group_destroy.allow_destroy_broken_rg = in_fbe_raid_group_destroy_req_p->allow_destroy_broken_rg;
    raid_group_destroy.wait_destroy = in_fbe_raid_group_destroy_req_p->wait_destroy;
    raid_group_destroy.destroy_timeout_msec = in_fbe_raid_group_destroy_req_p->destroy_timeout_msec;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY,
                                                            &raid_group_destroy,
                                                            sizeof(fbe_job_service_raid_group_destroy_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
            status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        in_fbe_raid_group_destroy_req_p->job_number = raid_group_destroy.job_number;
    return status;
} // end fbe_api_job_service_raid_group_destroy()

/*!***************************************************************
 * @fn fbe_api_job_service_set_system_power_save_info(fbe_api_job_service_change_system_power_save_info_t *in_change_power_save_info)
 ****************************************************************
 * @brief
 *  This function sets the system power saving information
 *
 * @param in_change_power_save_info - data structure containing iformation on how to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  04/28/10 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_system_power_save_info(fbe_api_job_service_change_system_power_save_info_t *in_change_power_save_info)
{
        fbe_job_service_change_system_power_saving_info_t       js_system_ps_info;
        fbe_api_control_operation_status_info_t                 status_info;
        fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;

        if (in_change_power_save_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

        js_system_ps_info.system_power_save_info.enabled = in_change_power_save_info->system_power_save_info.enabled;
        js_system_ps_info.system_power_save_info.hibernation_wake_up_time_in_minutes = in_change_power_save_info->system_power_save_info.hibernation_wake_up_time_in_minutes;
        js_system_ps_info.system_power_save_info.stats_enabled = in_change_power_save_info->system_power_save_info.stats_enabled;

        status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO,
                                                            &js_system_ps_info,
                                                            sizeof(fbe_job_service_change_system_power_saving_info_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
            status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_change_power_save_info->job_number = js_system_ps_info.job_number;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_change_rg_info(fbe_api_job_service_change_rg_info_t *in_change_rg_info)
 ****************************************************************
 * @brief
 *  This function changes the raid configuration
 *
 * @param in_change_rg_info - data structure containing iformation on how to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  05/06/10 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_change_rg_info(fbe_api_job_service_change_rg_info_t *in_change_rg_info, fbe_u64_t *job_number)
{
    fbe_job_service_update_raid_group_t         js_rg_update_info;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    if (in_change_rg_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the information correctly*/
    js_rg_update_info.update_type = in_change_rg_info->update_type;
    js_rg_update_info.object_id = in_change_rg_info->object_id;

    switch (in_change_rg_info->update_type)
    {
        case FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE:
        case FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE:
            break;
        case FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME:
            js_rg_update_info.power_save_idle_time_in_sec = in_change_rg_info->power_save_idle_time_in_sec;
            break;
        case FBE_UPDATE_RAID_TYPE_EXPAND_RG:
            js_rg_update_info.new_rg_size = in_change_rg_info->new_size;
            js_rg_update_info.new_edge_size = in_change_rg_info->new_edge_size;
            break;
        default:
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid update type:%d\n", __FUNCTION__, in_change_rg_info->update_type);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP,
                                                            &js_rg_update_info,
                                                            sizeof(fbe_job_service_update_raid_group_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
        status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *job_number = js_rg_update_info.job_number;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_drive_swap_in_request(fbe_api_job_service_drive_swap_in_request_t *in_change_rg_info)
 ******************************************************************************
 * @brief
 *  This function is used to swap-in proactive spare.
 *
 * @param in_drive_swap_request_p - drive swap request.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  05/06/10 - Created. Dhaval Patel
 *
 *******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_drive_swap_in_request(fbe_api_job_service_drive_swap_in_request_t * in_drive_swap_request_p)
{
    fbe_job_service_drive_swap_request_t          job_service_drive_swap_request;
    fbe_api_control_operation_status_info_t       status_info;
    fbe_status_t                                  status = FBE_STATUS_GENERIC_FAILURE;

    if(in_drive_swap_request_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set up the information correctly. */
    fbe_job_service_drive_swap_request_init(&job_service_drive_swap_request);
    job_service_drive_swap_request.vd_object_id = in_drive_swap_request_p->vd_object_id;
    job_service_drive_swap_request.command_type = in_drive_swap_request_p->command_type;
    job_service_drive_swap_request.orig_pvd_object_id = in_drive_swap_request_p->original_object_id;
    job_service_drive_swap_request.spare_object_id = in_drive_swap_request_p->spare_object_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_DRIVE_SWAP,
                                                           &job_service_drive_swap_request,
                                                           sizeof(fbe_job_service_drive_swap_request_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
        status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_drive_swap_request_p->job_number = job_service_drive_swap_request.job_number;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_update_provision_drive_config_type(
 *     fbe_api_job_service_update_provision_drive_config_type_t * update_pvd_config_p)
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive config type.
 *
 * @param update_pvd_config_p - provision drive config type update request.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  05/06/10 - Created. Dhaval Patel
 *
 *******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_provision_drive_config_type(fbe_api_job_service_update_provision_drive_config_type_t * in_update_pvd_config_p)
{
    fbe_job_service_update_provision_drive_t    job_service_update_provision_drive;
    fbe_api_control_operation_status_info_t      status_info;
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;

    if (in_update_pvd_config_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the information correctly*/
    job_service_update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE;
    job_service_update_provision_drive.object_id = in_update_pvd_config_p->pvd_object_id;
    job_service_update_provision_drive.config_type = in_update_pvd_config_p->config_type;
    fbe_copy_memory(&job_service_update_provision_drive.opaque_data, 
                    in_update_pvd_config_p->opaque_data,
                    sizeof (job_service_update_provision_drive.opaque_data));

    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE,
                                                           &job_service_update_provision_drive,
                                                           sizeof(fbe_job_service_update_provision_drive_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
        status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_update_pvd_config_p->job_number = job_service_update_provision_drive.job_number;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_update_pvd_sniff_verify(
 *     fbe_api_job_service_update_pvd_sniff_verify_status_t * in_update_pvd_sniff_verify_stat_p)
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive sniff verify status.
 *
 * @param in_update_pvd_sniff_verify_stat_p - provision drive sniff verify status update request.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  10/05/10 - Created. Manjit Singh
 *
 *******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_pvd_sniff_verify(fbe_api_job_service_update_pvd_sniff_verify_status_t  
                                                    * in_update_pvd_sniff_verify_stat_p)
{
    fbe_job_service_update_provision_drive_t    job_service_update_provision_drive;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    if (in_update_pvd_sniff_verify_stat_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the information correctly*/
    job_service_update_provision_drive.update_type = FBE_UPDATE_PVD_SNIFF_VERIFY_STATE;
    job_service_update_provision_drive.object_id = in_update_pvd_sniff_verify_stat_p->pvd_object_id;
    job_service_update_provision_drive.sniff_verify_state = in_update_pvd_sniff_verify_stat_p->sniff_verify_state;

    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE,
                                                           &job_service_update_provision_drive,
                                                           sizeof(fbe_job_service_update_provision_drive_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
        status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_update_pvd_sniff_verify_stat_p->job_number = job_service_update_provision_drive.job_number;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_update_pvd_pool_id(
 *     fbe_api_job_service_update_pvd_pool_id_t *in_update_pvd_pool_id_p)
 ******************************************************************************
 * @brief
 *  This function is used to update the provision drive pool_id.
 *
 * @param in_update_pvd_pool_id_p - provision drive pool-id update request.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  06/16/11 - Created. Arun S
 *
 *******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_pvd_pool_id(fbe_api_job_service_update_pvd_pool_id_t *in_update_pvd_pool_id_p)
{
    fbe_job_service_update_provision_drive_t    job_service_update_provision_drive;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    if (in_update_pvd_pool_id_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set up the information correctly */
    job_service_update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;
    job_service_update_provision_drive.object_id = in_update_pvd_pool_id_p->pvd_object_id;
    job_service_update_provision_drive.pool_id = in_update_pvd_pool_id_p->pvd_pool_id;

    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE,
                                                           &job_service_update_provision_drive,
                                                           sizeof(fbe_job_service_update_provision_drive_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
        status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_update_pvd_pool_id_p->job_number = job_service_update_provision_drive.job_number;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_update_provision_drive_block_size()
 ****************************************************************
 * @brief
 *  This function changes pvd's block size via the job service.
 *
 * @param in_block_size_p - data structure containing information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  06/10/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_provision_drive_block_size(fbe_object_id_t in_pvd_object_id,  fbe_provision_drive_configured_physical_block_size_t in_block_size)
{
    fbe_job_service_update_pvd_block_size_t     update_pvd_block_size;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;


    update_pvd_block_size.object_id = in_pvd_object_id;
    update_pvd_block_size.block_size = in_block_size;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE,
                                                            &update_pvd_block_size,
                                                            sizeof(fbe_job_service_update_pvd_block_size_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_set_permanent_spare_trigger_timer(
 *     fbe_api_job_service_update_permanent_spare_timer_t * in_update_permanent_spare_timer_p)
 ******************************************************************************
 * @brief
 *  This function sets permanent spare trigger timer
 *
 * @param in_update_spare_timer_p - data structure to update spare timer.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  04/28/10 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_permanent_spare_trigger_timer(fbe_api_job_service_update_permanent_spare_timer_t *in_update_spare_timer_p)
{
    fbe_job_service_update_spare_config_t       job_service_update_spare_config;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    if (in_update_spare_timer_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     /* update the spare configuration to update the permanent spare timer. */
    job_service_update_spare_config.update_type = FBE_DATABASE_UPDATE_PERMANENT_SPARE_TIMER;
    job_service_update_spare_config.system_spare_info.permanent_spare_trigger_time = in_update_spare_timer_p->permanent_spare_trigger_timer;

    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG,
                                                           &job_service_update_spare_config,
                                                           sizeof(fbe_job_service_update_spare_config_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK ||
       status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_update_spare_timer_p->job_number = job_service_update_spare_config.job_number;
    return status;
}


/*!****************************************************************************
 * @fn fbe_api_job_service_enable_pvd_sniff_verify(fbe_object_id_t in_pvd_object_id,
 * fbe_status_t* job_status)
 ******************************************************************************
 * @brief
 *  This function is used to enable the provision drive sniff verify.
 *
 * @param in_pvd_object_id - provision drive object ID.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  1-Feb- 2012 - Created. Shubhada Savdekar
 *
 *******************************************************************************/

fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_pvd_sniff_verify( fbe_object_id_t in_pvd_object_id , 
                                             fbe_status_t* job_status,
                                             fbe_job_service_error_type_t *js_error_type,
                                             fbe_bool_t sniff_verify_state)
{

    fbe_api_job_service_update_pvd_sniff_verify_status_t        update_pvd_sniff_verify_state;
    fbe_status_t                                                status = FBE_STATUS_GENERIC_FAILURE;
    

    update_pvd_sniff_verify_state.pvd_object_id = in_pvd_object_id;
    update_pvd_sniff_verify_state.sniff_verify_state = sniff_verify_state;

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_pvd_sniff_verify(&update_pvd_sniff_verify_state);

    if (status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_sniff_verify_state.job_number,
                                         FBE_JOB_SERVICE_WAIT_TIMEOUT,
                                         js_error_type,
                                         job_status,
                                         NULL);

    if(status == FBE_STATUS_TIMEOUT){
        return status;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_control_system_bg_service(
 * fbe_api_job_service_control_system_bg_service_t *in_control_system_bg_service)
 ****************************************************************
 * @brief
 *  This function control the system background service
 *
 * @param in_control_system_bg_service - data structure containing iformation on how to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  02/29/12 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_control_system_bg_service(fbe_api_job_service_control_system_bg_service_t *in_control_system_bg_service)
{
    fbe_job_service_control_system_bg_service_t       js_system_bg_service;
    fbe_api_control_operation_status_info_t           status_info;
    fbe_status_t                                      status = FBE_STATUS_GENERIC_FAILURE;

    if (in_control_system_bg_service == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_system_bg_service.system_bg_service = in_control_system_bg_service->system_bg_service;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
                                                            &js_system_bg_service,
                                                            sizeof(fbe_job_service_control_system_bg_service_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
            status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_control_system_bg_service->job_number = js_system_bg_service.job_number;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_destroy_system_pvd(fbe_object_id_t in_pvd_object_id)
 ******************************************************************************
 * @brief
 *  This function is used to destroy a system pvd.
 *  It is only for  raw mirror testing.
 *
 * @param in_pvd_object_id - provision drive object ID.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  3/7/2012 - Created. 
 *
 *******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_destroy_system_pvd( fbe_object_id_t in_pvd_object_id,fbe_status_t* job_status)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_destroy_provision_drive_t           pvd_destroy;
    fbe_job_service_error_type_t                                js_error_type = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    if(! fbe_private_space_layout_object_id_is_system_pvd(in_pvd_object_id))
    {
        
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: We can only destroy a system pvd for testing raw mirror\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
        
    }
    pvd_destroy.object_id = in_pvd_object_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE,
                                                            &pvd_destroy,
                                                            sizeof(fbe_job_service_destroy_provision_drive_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(pvd_destroy.job_number,
                                         FBE_JOB_SERVICE_WAIT_TO_DESTROY_SYS_PVD_TIMEOUT,
                                         &js_error_type,
                                         job_status,
                                         NULL);

    if(status == FBE_STATUS_TIMEOUT){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Time Out to destroy a system pvd \n", __FUNCTION__);
        return status;
    }

    return status;
}
/*!***************************************************************
 * @fn fbe_api_job_service_set_system_time_threshold_info()
 ****************************************************************
 * @brief
 *  This function sets the system time threshold information
 *
 * @param in_set_time_threshold_info - data structure containing setting value
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  05/31/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_system_time_threshold_info(fbe_api_job_service_set_system_time_threshold_info_t *in_set_time_threshold_info)
{
    fbe_job_service_set_system_time_threshold_info_t       js_system_time_threshold_info;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;

    if (in_set_time_threshold_info == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_system_time_threshold_info.system_time_threshold_info.time_threshold_in_minutes = in_set_time_threshold_info->system_time_threshold_info.time_threshold_in_minutes;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO,
                                                            &js_system_time_threshold_info,
                                                            sizeof(fbe_job_service_set_system_time_threshold_info_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
            status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_set_time_threshold_info->job_number = js_system_time_threshold_info.job_number;
    return status;
}

/*!****************************************************************************
 * @fn fbe_api_job_service_update_multi_pvds_pool_id(
        fbe_api_job_service_update_multi_pvds_pool_id_t *in_update_multi_pvds_pool_id_p);
 ******************************************************************************
 * @brief
 *  This function is used to update a list of the provision drive pool_id.
 *
 * @param in_update_multi_pvds_pool_id_p - provision drive pool-id list update request.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  04/07/13 - Created. Hongpo Gao
 *
 *******************************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_multi_pvds_pool_id(fbe_api_job_service_update_multi_pvds_pool_id_t *in_update_multi_pvds_pool_id_p)
{
    fbe_job_service_update_multi_pvds_pool_id_t pvd_pool_id_list;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    if (in_update_multi_pvds_pool_id_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set up the information correctly */
    fbe_zero_memory(&pvd_pool_id_list, sizeof(fbe_job_service_update_multi_pvds_pool_id_t));
    fbe_copy_memory(&pvd_pool_id_list.pvd_pool_data_list, &in_update_multi_pvds_pool_id_p->pvd_pool_data_list, sizeof(update_pvd_pool_id_list_t));

    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID,
                                                           &pvd_pool_id_list,
                                                           sizeof(fbe_job_service_update_multi_pvds_pool_id_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK ||
        status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
                       __FUNCTION__, status, status_info.packet_qualifier, 
                       status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_update_multi_pvds_pool_id_p->job_number = pvd_pool_id_list.job_number;
    return status;
}

/*!***************************************************************************
 * @fn fbe_api_job_service_set_spare_library_config(
 *                                  fbe_bool_t b_disable_operation_confirmation, 
 *                                  fbe_bool_t b_set_default_values,             
 *                                  fbe_u32_t spare_operation_timeout_in_seconds)
 ***************************************************************************** 
 * 
 * @brief   This function is used to `configure' the spare library.  The following
 *          configuration values can be modified:
 *              o b_disable_operation_confirmation - FBE_TRUE - Disable the job
 *                  service to object `confirmation' process.  This value should
 *                  only be modified for testing when the test is setting one or
 *                  more debug hooks in more than one virtual drive objects.  This
 *                  will prevent the job service from waiting for the virtual drive
 *                  to complete a particular operation (since it will not complete
 *                  due to the debug hook).  This allows the job to complete and
 *                  thus frees the recovery queue to process the job for the next
 *                  virtual drive.
 *              o b_set_default_values - FBE_TRUE - Restore the spare library
 *                  configuration to the default settings (all other parameters are
 *                  ignored).
 *              o spare_operation_timeout_in_seconds - The amount of time (in
 *                  seconds) that the spare job will wait for the virtual drive to
 *                  complete an operation (there could be multiple operations for
 *                  a single job).
 *
 * @param   spare_operation_timeout_in_seconds - Operation timeout in seconds
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  04/24/2013  - Ron Proulx    - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_set_spare_library_config(fbe_bool_t b_disable_operation_confirmation,
                                                                       fbe_bool_t b_set_default_values,
                                                                       fbe_u32_t spare_operation_timeout_in_seconds)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_job_service_update_spare_library_config_request_t   update_spare_config;
    fbe_api_control_operation_status_info_t                 status_info;

    /* Build the change config.
     */
    fbe_zero_memory(&update_spare_config, sizeof(fbe_job_service_update_spare_library_config_request_t));
    update_spare_config.b_disable_operation_confirmation = b_disable_operation_confirmation;
    update_spare_config.b_set_default = b_set_default_values;
    update_spare_config.operation_timeout_secs = spare_operation_timeout_in_seconds;

     /* update the spare library configuration*/
    status = fbe_api_common_send_control_packet_to_service(FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG,
                                                           &update_spare_config,
                                                           sizeof(fbe_job_service_update_spare_library_config_request_t),
                                                           FBE_SERVICE_ID_JOB_SERVICE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK ||
       status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/****************************************************************
 * end fbe_api_job_service_set_spare_library_config()
 ****************************************************************/

/*!***************************************************************
 * @fn fbe_api_job_service_set_system_encryption_info(fbe_api_job_service_change_system_encryption_info_t *in_change_encryption_info_p)
 ****************************************************************
 * @brief
 *  This function sets the system encryption information
 *
 * @param in_change_encryption_info_p - data structure containing iformation on how to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  04-Oct-2013 - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_system_encryption_info(fbe_api_job_service_change_system_encryption_info_t *in_change_encryption_info_p)
{
    fbe_job_service_change_system_encryption_info_t         js_system_encryption_info;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;

    if (in_change_encryption_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_system_encryption_info.system_encryption_info = in_change_encryption_info_p->system_encryption_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO,
                                                            &js_system_encryption_info,
                                                            sizeof(fbe_job_service_change_system_encryption_info_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_change_encryption_info_p->job_number = js_system_encryption_info.job_number;

    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_set_encryption_paused(fbe_bool_t encryption_paused)
 ****************************************************************
 * @brief
 *  This function sets the system encryption information
 *
 * @param in_change_encryption_info_p - data structure containing iformation on how to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  04-Oct-2013 - created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_set_encryption_paused(fbe_bool_t encryption_paused)
{
    fbe_job_service_pause_encryption_t          js_paused;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_status_t                                status = FBE_STATUS_INVALID;
    fbe_job_service_error_type_t                error_type;
    fbe_status_t                                job_status = FBE_STATUS_INVALID;

    fbe_zero_memory(&js_paused, sizeof(js_paused));
    fbe_zero_memory(&status_info, sizeof(status_info));

    js_paused.encryption_paused = encryption_paused;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED,
                                                            &js_paused,
                                                            sizeof(js_paused),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_wait_for_job(js_paused.job_number, FBE_JOB_SERVICE_WAIT_ENCRYPTION_TIMEOUT, 
                                         &error_type, &job_status, NULL);

    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return status;
    }
    

    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_update_encryption_mode()
 ****************************************************************
 * @brief
 *  This function sets the system encryption information
 *
 * @param in_update_mode_p - data structure containing iformation on how to change
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  10/31/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_update_encryption_mode(fbe_api_job_service_update_encryption_mode_t *in_update_mode_p)
{
    fbe_job_service_update_encryption_mode_t js_system_encryption_info;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;

    if (in_update_mode_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_system_encryption_info.encryption_mode = in_update_mode_p->encryption_mode;
    js_system_encryption_info.object_id = in_update_mode_p->object_id;
	js_system_encryption_info.job_callback = NULL; /* No callback for fbe_api */
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE,
                                                            &js_system_encryption_info,
                                                            sizeof(fbe_job_service_update_encryption_mode_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_update_mode_p->job_number = js_system_encryption_info.job_number;

    return status;
}


/*!***************************************************************
 * @fn fbe_api_job_service_setup_encryption_keys()
 ****************************************************************
 * @brief
 *  This function starts the job to setup encryption keys
 *
 * @param 
 *    keys_p - data structure containing keys
 *    job_status - job status
 *    job_error_type - job error type
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  19-Nov-13 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_setup_encryption_keys(fbe_encryption_key_table_t *keys_p,
                                          fbe_status_t* job_status,
                                          fbe_job_service_error_type_t *job_error_type)
{
    fbe_api_job_service_process_encryption_keys_t process_encryption_keys = {0};
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sg_element_t *                       sg_list = NULL;

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * FBE_API_SG_LIST_SIZE);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)keys_p;
    sg_list[0].count = sizeof(fbe_encryption_key_table_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;
    
    process_encryption_keys.operation    = FBE_JOB_SERVICE_SETUP_ENCRYPTION_KEYS;
    process_encryption_keys.keys_sg_list = sg_list;  // save the sg_list pointer

    status = fbe_api_job_service_process_encryption_keys(&process_encryption_keys);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to start job, status:0x%x\n", __FUNCTION__, status);

        return status;
    }

    // wait for the notification from the job service.
    status = fbe_api_common_wait_for_job(process_encryption_keys.job_number, FBE_JOB_SERVICE_WAIT_ENCRYPTION_TIMEOUT, 
                                         job_error_type, job_status, NULL);

    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) 
    {
        *job_status = FBE_STATUS_GENERIC_FAILURE;
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return status;
    }

    fbe_api_free_memory (sg_list);

    return status;
}


/*!***************************************************************
 * @fn fbe_api_job_service_process_encryption_keys()
 ****************************************************************
 * @brief
 *  This function send the process encryption keys to job services.
 *
 * @param in_process_encryption_keys_p - data structure containing keys
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  11/11/2013 - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_api_job_service_process_encryption_keys(fbe_api_job_service_process_encryption_keys_t *in_process_encryption_keys_p)
{
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_process_encryption_keys_t js_process_encryption_keys = {0};

    js_process_encryption_keys.operation = in_process_encryption_keys_p->operation;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS,
                                                            &js_process_encryption_keys,
                                                            sizeof(fbe_job_service_process_encryption_keys_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                            in_process_encryption_keys_p->keys_sg_list,
                                                            FBE_API_SG_LIST_SIZE,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_process_encryption_keys_p->job_number = js_process_encryption_keys.job_number;

    return status;

} // end of fbe_api_job_service_process_encryption_keys

/*!***************************************************************
 * @fn fbe_api_job_service_scrub_old_user_data()
 ****************************************************************
 * @brief
 *  This function send the scrub request to job services 
 * and wait for job completion.
 *
 * @param void
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  11/11/2013 - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_scrub_old_user_data(void)
{
    fbe_job_service_scrub_old_user_data_t                   js_scrub_old_data;
    fbe_api_control_operation_status_info_t                 status_info;
    fbe_status_t                                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_error_type_t							error_type;
    fbe_status_t											job_status;

    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA,
                                                            &js_scrub_old_data,
                                                            sizeof(js_scrub_old_data),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(js_scrub_old_data.job_number, FBE_JOB_SERVICE_WAIT_TO_DESTROY_SYS_PVD_TIMEOUT, &error_type, &job_status, NULL);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error status: %d, error type %d\n", __FUNCTION__, job_status, error_type);
    }
    
    return job_status;

} // end of fbe_api_job_service_scrub_old_user_data

/*!***************************************************************************
 * @fn      fbe_api_job_service_add_debug_hook()
 ***************************************************************************** 
 * 
 * @brief   This function adds the requested debug hook to the job service
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.
 *
 * @return  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  03/13/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_add_debug_hook(fbe_object_id_t object_id,
                                                       fbe_job_type_t hook_type,
                                                       fbe_job_action_state_t hook_state,
                                                       fbe_job_debug_hook_state_phase_t hook_phase,
                                                       fbe_job_debug_hook_action_t hook_action)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_job_service_add_debug_hook_t        debug_hook;

    /* Validate the input parameters.
     */
    if (((object_id == FBE_OBJECT_ID_INVALID) && (hook_type == FBE_JOB_TYPE_INVALID)) ||
        ((hook_type <= FBE_JOB_TYPE_INVALID) || 
         (hook_type >= FBE_JOB_TYPE_LAST)       )     ) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: object_id: 0x%x or hook type: 0x%x invalid\n", 
                      __FUNCTION__, object_id, hook_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the request
     */
    fbe_zero_memory(&debug_hook, sizeof(fbe_job_service_add_debug_hook_t));
    debug_hook.debug_hook.object_id = object_id;
    debug_hook.debug_hook.hook_type = hook_type;
    debug_hook.debug_hook.hook_state = hook_state;
    debug_hook.debug_hook.hook_phase = hook_phase;
    debug_hook.debug_hook.hook_action = hook_action;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK,
                                                            &debug_hook,
                                                            sizeof(fbe_job_service_add_debug_hook_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} // end of fbe_api_job_service_add_debug_hook

/*!***************************************************************************
 * @fn      fbe_api_job_service_get_debug_hook()
 ***************************************************************************** 
 * 
 * @brief   This function gets the requested debug hook to the job service
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.
 * @param   debug_hook_p - If FBE_STATUS_OK this hook is populated
 *
 * @return  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  03/13/2014  Ron Proulx  - fbe_job_service_add_debug_hook_t
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_get_debug_hook(fbe_object_id_t object_id,
                                                             fbe_job_type_t hook_type,
                                                             fbe_job_action_state_t hook_state,
                                                             fbe_job_debug_hook_state_phase_t hook_phase,
                                                             fbe_job_debug_hook_action_t hook_action,
                                                             fbe_job_service_debug_hook_t *debug_hook_p)

{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_job_service_get_debug_hook_t        debug_hook;

    /* Validate the input parameters.
     */
    if ((debug_hook_p == NULL)                    ||
        ((object_id == FBE_OBJECT_ID_INVALID) && (hook_type == FBE_JOB_TYPE_INVALID)) ||
        ((hook_type <= FBE_JOB_TYPE_INVALID) || 
         (hook_type >= FBE_JOB_TYPE_LAST)       )     ) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: object_id: 0x%x or hook type: 0x%x invalid\n", 
                      __FUNCTION__, object_id, hook_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the request
     */
    fbe_zero_memory(debug_hook_p, sizeof(*debug_hook_p));
    fbe_zero_memory(&debug_hook, sizeof(fbe_job_service_get_debug_hook_t));
    debug_hook.debug_hook.object_id = object_id;
    debug_hook.debug_hook.hook_type = hook_type;
    debug_hook.debug_hook.hook_state = hook_state;
    debug_hook.debug_hook.hook_phase = hook_phase;
    debug_hook.debug_hook.hook_action = hook_action;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK,
                                                            &debug_hook,
                                                            sizeof(fbe_job_service_get_debug_hook_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if the hook was found
     */
    if (debug_hook.b_hook_found == FBE_FALSE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s: request hook obj: 0x%x type: 0x%x state: %d phase: %d action: %d not found\n", 
                       __FUNCTION__, object_id, hook_type, hook_state, hook_phase, hook_action);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the input hook
     */
    *debug_hook_p = debug_hook.debug_hook;
    return status;

} // end of fbe_api_job_service_get_debug_hook

/*!***************************************************************************
 * @fn      fbe_api_job_service_delete_debug_hook()
 ***************************************************************************** 
 * 
 * @brief   This function delete the requested debug hook to the job service
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.
 *
 * @return  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  03/13/2014  Ron Proulx  - fbe_job_service_add_debug_hook_t
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_delete_debug_hook(fbe_object_id_t object_id,
                                                       fbe_job_type_t hook_type,
                                                       fbe_job_action_state_t hook_state,
                                                       fbe_job_debug_hook_state_phase_t hook_phase,
                                                       fbe_job_debug_hook_action_t hook_action)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_job_service_remove_debug_hook_t     debug_hook;

    /* Validate the input parameters.
     */
    if (((object_id == FBE_OBJECT_ID_INVALID) && (hook_type == FBE_JOB_TYPE_INVALID)) ||
        ((hook_type <= FBE_JOB_TYPE_INVALID) || 
         (hook_type >= FBE_JOB_TYPE_LAST)       )     ) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: object_id: 0x%x or hook type: 0x%x invalid\n", 
                      __FUNCTION__, object_id, hook_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the request
     */
    fbe_zero_memory(&debug_hook, sizeof(fbe_job_service_remove_debug_hook_t));
    debug_hook.debug_hook.object_id = object_id;
    debug_hook.debug_hook.hook_type = hook_type;
    debug_hook.debug_hook.hook_state = hook_state;
    debug_hook.debug_hook.hook_phase = hook_phase;
    debug_hook.debug_hook.hook_action = hook_action;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK,
                                                            &debug_hook,
                                                            sizeof(fbe_job_service_remove_debug_hook_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if the hook was found
     */
    if (debug_hook.b_hook_found == FBE_FALSE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s: request hook obj: 0x%x type: 0x%x state: %d phase: %d action: %d not found\n", 
                       __FUNCTION__, object_id, hook_type, hook_state, hook_phase, hook_action);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

} // end of fbe_api_job_service_delete_debug_hook

/*!***************************************************************************
 * @fn fbe_api_job_service_validate_database()
 *****************************************************************************
 * @brief
 *  This starts a `validate database' job 
 *
 * @param   validate_database_p - Pointer to validate database request.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  07/01/2014  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_job_service_validate_database(fbe_api_job_service_validate_database_t *validate_database_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_validate_database_t     validate_database_job;
    fbe_api_control_operation_status_info_t status_info;

    /* Populate the request.
     */
    fbe_zero_memory(&validate_database_job, sizeof(fbe_job_service_validate_database_t));
    validate_database_job.validate_caller = validate_database_p->validate_caller;
    validate_database_job.failure_action = validate_database_p->failure_action;

    /* Send the request.
     */
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE,
                                                            &validate_database_job,
                                                            sizeof(fbe_job_service_validate_database_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if ((status != FBE_STATUS_OK)                                               ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)    ) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the job nunber.
     */
    validate_database_p->job_number = validate_database_job.job_number;
    return status;
} // end fbe_api_job_service_validate_database


/*!***************************************************************
 * @fn fbe_api_job_service_create_extent_pool()
 ****************************************************************
 * @brief
 *  This function creates an extent pool to job service.
 *
 * @param in_create_pool_p - data structure containing information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  06/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_create_extent_pool(fbe_api_job_service_create_extent_pool_t *in_create_pool_p)
{
    fbe_job_service_create_extent_pool_t js_create_ext_pool;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_error_type_t             error_type;
    fbe_status_t                             job_status;

    if (in_create_pool_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_create_ext_pool.drive_count = in_create_pool_p->drive_count;
    js_create_ext_pool.drive_type = in_create_pool_p->drive_type;
    js_create_ext_pool.pool_id = in_create_pool_p->pool_id;
    js_create_ext_pool.job_callback = NULL; /* No callback for fbe_api */
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL,
                                                            &js_create_ext_pool,
                                                            sizeof(fbe_job_service_create_extent_pool_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_create_pool_p->job_number = js_create_ext_pool.job_number;

    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(in_create_pool_p->job_number, 30000, &error_type, &job_status, NULL);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_create_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function creates an extent pool lun to job service.
 *
 * @param in_create_lun_p - data structure containing information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  06/11/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_create_ext_pool_lun(fbe_api_job_service_create_ext_pool_lun_t *in_create_lun_p)
{
    fbe_job_service_create_ext_pool_lun_t    js_create_ext_pool_lun;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_error_type_t             error_type;
    fbe_status_t                             job_status;

    if (in_create_lun_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_create_ext_pool_lun.lun_id = in_create_lun_p->lun_id;
    js_create_ext_pool_lun.pool_id = in_create_lun_p->pool_id;
    js_create_ext_pool_lun.capacity = in_create_lun_p->capacity;
    js_create_ext_pool_lun.world_wide_name = in_create_lun_p->world_wide_name;
    js_create_ext_pool_lun.user_defined_name = in_create_lun_p->user_defined_name;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN,
                                                            &js_create_ext_pool_lun,
                                                            sizeof(fbe_job_service_create_ext_pool_lun_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_create_lun_p->job_number = js_create_ext_pool_lun.job_number;

    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(in_create_lun_p->job_number, 30000, &error_type, &job_status, &in_create_lun_p->lun_object_id);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_destroy_extent_pool()
 ****************************************************************
 * @brief
 *  This function creates an extent pool to job service.
 *
 * @param in_destroy_pool_p - data structure containing information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  06/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_destroy_extent_pool(fbe_api_job_service_destroy_extent_pool_t *in_destroy_pool_p)
{
    fbe_job_service_destroy_extent_pool_t    js_destroy_ext_pool;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_error_type_t             error_type;
    fbe_status_t                             job_status;

    if (in_destroy_pool_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_destroy_ext_pool.pool_id = in_destroy_pool_p->pool_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL,
                                                            &js_destroy_ext_pool,
                                                            sizeof(fbe_job_service_destroy_extent_pool_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_destroy_pool_p->job_number = js_destroy_ext_pool.job_number;

    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(in_destroy_pool_p->job_number, 30000, &error_type, &job_status, NULL);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_job_service_destroy_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function creates an extent pool lun to job service.
 *
 * @param in_destroy_lun_p - data structure containing information
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK    - if no error.
 *
 * @version
 *  06/11/2014 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_job_service_destroy_ext_pool_lun(fbe_api_job_service_destroy_ext_pool_lun_t *in_destroy_lun_p)
{
    fbe_job_service_destroy_ext_pool_lun_t    js_destroy_ext_pool_lun;
    fbe_api_control_operation_status_info_t  status_info;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_job_service_error_type_t             error_type;
    fbe_status_t                             job_status;

    if (in_destroy_lun_p == NULL){
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    js_destroy_ext_pool_lun.lun_id = in_destroy_lun_p->lun_id;
    js_destroy_ext_pool_lun.pool_id = in_destroy_lun_p->pool_id;
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN,
                                                            &js_destroy_ext_pool_lun,
                                                            sizeof(fbe_job_service_destroy_ext_pool_lun_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if ((status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    in_destroy_lun_p->job_number = js_destroy_ext_pool_lun.job_number;

    /*Wait for the job to complete*/
    status = fbe_api_common_wait_for_job(in_destroy_lun_p->job_number, 30000, &error_type, &job_status, NULL);
    if (status == FBE_STATUS_GENERIC_FAILURE || status == FBE_STATUS_TIMEOUT) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed waiting for job\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (job_status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:job failed with error code:%d\n", __FUNCTION__, error_type);
    }

    return status;
}


/******************************************
 * end file fbe_api_job_service_interface.c
 *****************************************/
