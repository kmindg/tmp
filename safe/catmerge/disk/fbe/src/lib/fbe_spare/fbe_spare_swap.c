/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare_swap.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functionality to update the edge inforamtion and
 *  required configuration changes in to objects memory. It handles the 
 *  different configuration udpate request like (swap-in the permanent spare,
 *  swap-in proactive spare, swap-out the proactive spare etc.
 *
 * @version
 *  12/09/2009:  Created. Dhaval Patel
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
#include "fbe_transport_memory.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_database.h"
#include "fbe_spare.h"
#include "fbe_spare_lib_private.h"
#include "fbe_job_service.h"
#include "fbe/fbe_event_log_utils.h"                    //  for message codes

static fbe_status_t 
fbe_spare_lib_get_provision_drive_update_type(fbe_object_id_t pvd_object_id,
                                              fbe_object_id_t vd_object_id,
                                              fbe_database_control_update_pvd_t *update_provision_drive);

/*!****************************************************************************
 * fbe_spare_lib_swap_in_permanent_spare()
 ******************************************************************************
 * @brief
 * This function is used to swap-in permanent spare, it starts transaction to
 * detach an existing edge and attach an new edge at the same location with
 * another spare drive. it updates this configuration in memory on both the 
 * SPs.
 *
 * @param packet_p          - The packet requesting this operation.
 * @param js_swap_request_p - Drive sparing request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_swap_in_permanent_spare(fbe_packet_t *packet_p,
                                                   fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     selected_spare_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                    permanent_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_provision_drive_info_t          spare_pvd_info;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;
    fbe_block_transport_control_get_edge_info_t block_edge_info;
    fbe_database_control_update_pvd_t   update_provision_drive;
    fbe_u32_t                           pool_id;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /*! get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* Get the virtual drive object-id from the job service swap-in request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);

    /* Get the edge-index where we need to swap-in the permanent spare. */
    fbe_spare_lib_utils_get_permanent_spare_swap_edge_index(vd_object_id, &permanent_spare_edge_index);
    if ((permanent_spare_edge_index == FBE_EDGE_INDEX_INVALID)             ||
        (permanent_spare_edge_index != js_swap_request_p->swap_edge_index)    )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - invalid permanent spare edge index, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note At this point it is too late to get the original pvd object id.
     *        The spare validation code has already set it.
     */
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(js_swap_request_p, &original_pvd_object_id);
    if ((original_pvd_object_id == 0)                     ||
        (original_pvd_object_id == FBE_OBJECT_ID_INVALID)    )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x invalid original pvd obj: 0x%x\n",
                               __FUNCTION__, vd_object_id, original_pvd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the existing block edge information. */
    status = fbe_spare_lib_utils_get_block_edge_info(vd_object_id, permanent_spare_edge_index, &block_edge_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - get edge info failed, vd_id:0x%x, edge_idx:0x%x\n",
                               __FUNCTION__, vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* start the transaction to perform edge swap-out/swap-in operation. */
    status = fbe_spare_lib_utils_start_database_transaction(&js_swap_request_p->transaction_id, job_queue_element_p->job_number);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - start transaction failed, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, status);
        return status;
    }

    /* Get the object-id of the selected spare (spare is selected using spare
     * selection algorithm in previous step).
     */
    fbe_job_service_drive_swap_request_get_spare_object_id(js_swap_request_p, &selected_spare_object_id);
    if (selected_spare_object_id == FBE_OBJECT_ID_INVALID)

    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - invalid spare object, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the swapped-in hot spare configuration. */
    status = fbe_spare_lib_utils_get_provision_drive_info(selected_spare_object_id, &spare_pvd_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - get hot spare provision drive info failed, spare_pvd_id:0x%x\n",
                               __FUNCTION__, selected_spare_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 1: Destroy the existing edge.
     */
    status = fbe_spare_lib_utils_database_service_destroy_edge(js_swap_request_p->transaction_id,
                                                               vd_object_id,
                                                               permanent_spare_edge_index,
                                                               js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in destroy edge - vd_id:0x%x, index: 0x%x. Status: 0x%x\n",
                               __FUNCTION__, vd_object_id, permanent_spare_edge_index, status);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /*one of the things we'll need to do it to set the pool ID of newly replaced drive to match the one we are replacing*/
    status = fbe_database_get_pvd_pool_id(js_swap_request_p->orig_pvd_object_id, &pool_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:getting POOL Id of original drive failed, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* udpate the spare configuration type to `consumed'.*/
    update_provision_drive.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID;
    update_provision_drive.object_id = selected_spare_object_id;
    update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
    update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE; 
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.update_opaque_data = FBE_FALSE;

    status = fbe_spare_lib_get_provision_drive_update_type(selected_spare_object_id, vd_object_id, &update_provision_drive);
    if(status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - get object enc mode failed, vd_id:0x%x, hs_id:0x%x st: %d\n",
                               __FUNCTION__, vd_object_id, selected_spare_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* update the configuration type of the spare pvd object to raid. */
    status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - update config type failed, vd_id:0x%x, hs_id:0x%x\n",
                               __FUNCTION__, vd_object_id, selected_spare_object_id);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Now update the pool id of the swapped-in drive */
    update_provision_drive.pool_id = pool_id;
    update_provision_drive.object_id = selected_spare_object_id;
    update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
    update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.update_opaque_data = FBE_FALSE;
            /* update the configuration type of the spare pvd object to raid. */
    status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - update config type failed, vd_id:0x%x, hs_id:0x%x\n",
                               __FUNCTION__, vd_object_id, selected_spare_object_id);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 2: Update the VD non paged metadata to indicate that we swapped in a new edge for the failed drive */
    status = fbe_spare_lib_utils_set_vd_nonpaged_metadata(packet_p, js_swap_request_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:updating the NP failed, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 3:  Create a new edge.
     */
    status = fbe_spare_lib_utils_database_service_create_edge(js_swap_request_p->transaction_id,
                                                              selected_spare_object_id,
                                                              vd_object_id,
                                                              permanent_spare_edge_index,
                                                              block_edge_info.capacity,
                                                              block_edge_info.offset,
                                                              js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in create edge - vd:0x%x, pvd:0x%x, index: 0x%x. Status: 0x%x\n",
                               __FUNCTION__, vd_object_id, selected_spare_object_id, permanent_spare_edge_index, status);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CREATE_EDGE_FAILED);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* update the configuration type of the original pvd object if it is not invalid. */
    if (js_swap_request_p->orig_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        update_provision_drive.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
        update_provision_drive.object_id = js_swap_request_p->orig_pvd_object_id;
        update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
        update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE; 
        update_provision_drive.update_type_bitmask = 0;
        update_provision_drive.update_opaque_data = FBE_FALSE;

        status = fbe_spare_lib_get_provision_drive_update_type(js_swap_request_p->orig_pvd_object_id, FBE_OBJECT_ID_INVALID, &update_provision_drive);
        if(status != FBE_STATUS_OK) {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: get object enc mode failed, vd_id:0x%x, hs_id:0x%x st: %d\n",
                                   __FUNCTION__, vd_object_id, js_swap_request_p->orig_pvd_object_id, status);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }

        /* change the configuration type as unknown because this drive has been  removed from the raid group. */
        status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: - update config type failed, vd_id:0x%x, original drive pvd:0x%x\n",
                                   __FUNCTION__, vd_object_id, js_swap_request_p->orig_pvd_object_id);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }

        update_provision_drive.pool_id = FBE_POOL_ID_INVALID;
        update_provision_drive.object_id = js_swap_request_p->orig_pvd_object_id;
        update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
        update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID; 
        update_provision_drive.update_opaque_data = FBE_FALSE;
        /* change the configuration type as unknown because this drive has been  removed from the raid group. */
        status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: - update config type failed, vd_id:0x%x, original drive pvd:0x%x\n",
                                   __FUNCTION__, vd_object_id, js_swap_request_p->orig_pvd_object_id);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }
    }


    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_spare_lib_utils_complete_packet(packet_p, status);
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_swap_in_permanent_spare()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_get_spare_drive_encryption_mode()
 *****************************************************************************
 *
 * @brief
 * This function is used to get spare drive encryption mode from upstream objects.
 *
 * @param pvd_object_id - PVD object id.
 * @param vd_object_id - VD object id.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/24/2014  Lili Chen.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_get_provision_drive_update_type(fbe_object_id_t pvd_object_id,
                                                           fbe_object_id_t vd_object_id,
                                                           fbe_database_control_update_pvd_t *update_provision_drive)
{
    fbe_status_t                                status;
    fbe_base_config_control_encryption_mode_t   encryption_mode_info;
    fbe_base_config_upstream_object_list_t      upstream_object_list;
    fbe_u32_t                                   i;
    fbe_base_config_encryption_mode_t           encryption_mode;
    fbe_base_config_encryption_mode_t           vd_encryption_mode;
    fbe_system_encryption_mode_t                system_encryption_mode;

    status = fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if(status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: failed to get sys enc mode vd:0x%x, pvd:0x%x st: %d\n",
                               __FUNCTION__, vd_object_id, pvd_object_id, status);
        return status;
    }

    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
        return FBE_STATUS_OK;
    }

    encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
    /* For system drive, get system raid group encryption_mode first */
    if (fbe_database_is_object_system_pvd(pvd_object_id))
    {
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                         &upstream_object_list,
                                                         sizeof(fbe_base_config_upstream_object_list_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         pvd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                    FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "fbe_spare_lib_get_spare_drive_encryption_mode get upstream failed, status %d\n", status);
            return status;
        }

        for (i = 0; i < upstream_object_list.number_of_upstream_objects; i++)
        {
            if (fbe_database_is_object_system_rg(upstream_object_list.upstream_object_list[i]))
            {
                /* update the proision drive configuration. */
                status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                                 &encryption_mode_info,
                                                                 sizeof(fbe_base_config_control_encryption_mode_t),
                                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                                 FBE_CLASS_ID_INVALID,
                                                                 upstream_object_list.upstream_object_list[i],
                                                                 FBE_PACKET_FLAG_NO_ATTRIB);
                if (status != FBE_STATUS_OK) {
                    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                           FBE_TRACE_LEVEL_ERROR, 
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "GET_ENCRYPTION_MODE failed for obj 0x%x\n", 
                                           upstream_object_list.upstream_object_list[i]);
                    return status;
                }

                if (encryption_mode_info.encryption_mode > encryption_mode) {
                    encryption_mode = encryption_mode_info.encryption_mode;
                }
            }
        }
    }

    /* Then get vd encryption_mode */
    if (vd_object_id != FBE_OBJECT_ID_INVALID) {
        /* This is swap in case */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                         &encryption_mode_info,
                                                         sizeof(fbe_base_config_control_encryption_mode_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if(status != FBE_STATUS_OK) {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: get object enc mode failed, vd_id:0x%x, st: %d\n",
                                   __FUNCTION__, vd_object_id, status);
            return status;
        }
        vd_encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
        if (encryption_mode_info.encryption_mode > vd_encryption_mode) {
            vd_encryption_mode = encryption_mode_info.encryption_mode;
        }

        /* This is swap-in case. We should set up update_type based on VD and Vault encryption mode */
        update_provision_drive->update_type = FBE_UPDATE_PVD_TYPE;
        if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS || vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) {
            update_provision_drive->update_type = FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS;
        } else if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS || vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS) {
            update_provision_drive->update_type = FBE_UPDATE_PVD_REKEY_IN_PROGRESS;
        } else if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED && vd_encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) {
            update_provision_drive->update_type = FBE_UPDATE_PVD_UNENCRYPTED;
        } 
    } else {
        /* This is swap-out case */
        update_provision_drive->update_type = FBE_UPDATE_PVD_UNENCRYPTED;
        if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) {
            update_provision_drive->update_type = FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS;
        } else if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS) {
            update_provision_drive->update_type = FBE_UPDATE_PVD_REKEY_IN_PROGRESS;
        } else if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) {
            update_provision_drive->update_type = FBE_UPDATE_PVD_TYPE;
        } 
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_get_spare_drive_encryption_mode()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_initiate_copy_operation()
 *****************************************************************************
 *
 * @brief
 * This function is used to swap-in proactive spare, it starts transaction to
 * create an edge at the specified position and it also changes configuration
 * mode to mirror.
 *
 * @param packet_p          - The packet requesting this operation.
 * @param js_swap_request_p - Drive sparing request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/31/2012  Ron Proulx  - Updated to perform change to mirror mode also.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_initiate_copy_operation(fbe_packet_t *packet_p,
                                                   fbe_job_service_drive_swap_request_t * js_swap_request_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_object_id_t                             vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_base_object_mgmt_get_lifecycle_state_t  vd_lifecycle;
    fbe_base_object_mgmt_get_lifecycle_state_t  spare_lifecycle;
    fbe_u32_t                                   wait_for_ready_secs = fbe_spare_main_get_operation_timeout_secs();
    fbe_object_id_t                             original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                             selected_spare_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                            copy_swap_in_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_block_transport_control_get_edge_info_t block_edge_info;
    fbe_job_queue_element_t                    *job_queue_element_p = NULL;
    fbe_database_control_update_pvd_t           update_provision_drive;
    fbe_database_control_update_vd_t            update_virtual_drive;
    fbe_virtual_drive_configuration_mode_t      updated_vd_configuration_mode;
    fbe_base_config_upstream_object_list_t      upstream_object_list;
    fbe_raid_group_set_extended_media_error_handling_t parent_set_emeh_request;
    
    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the payload.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* Step 1:  Validate the request.
     */

    /* Get the virtual drive object-id from the job service swap-in request. 
     */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);

    /* Get the edge-index where we need to swap-in the proactive spare drive. 
     */
    fbe_spare_lib_utils_get_copy_swap_edge_index(vd_object_id, &copy_swap_in_edge_index);
    if (copy_swap_in_edge_index == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x could not locate broken edge\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_PROACTIVE_SPARE_EDGE_INDEX;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_PROACTIVE_SPARE_EDGE_INDEX);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! @note At this point it is too late to get the original pvd object id.
     *        The spare validation code has already set it.
     */
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(js_swap_request_p, &original_pvd_object_id);
    if ((original_pvd_object_id == 0)                     ||
        (original_pvd_object_id == FBE_OBJECT_ID_INVALID)    )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x invalid original pvd obj: 0x%x\n",
                               __FUNCTION__, vd_object_id, original_pvd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Create edge with selected spare drive and store the same edge information
     * as previously selected edge.
     */
    status = fbe_spare_lib_utils_get_block_edge_info(vd_object_id,
                                                     (copy_swap_in_edge_index == 0) ? 1 : 0, // get the mirror of spare edge index.
                                                     &block_edge_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - get edge info failed, vd_id:0x%x, edge_idx:0x%x\n",
                               __FUNCTION__, vd_object_id, ((copy_swap_in_edge_index == 0) ? 1 : 0));

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Get the object-id of the selected spare (spare is selected using spare
     * selection algorithm in previous step).
     */
    fbe_job_service_drive_swap_request_get_spare_object_id(js_swap_request_p, &selected_spare_object_id);
    if (selected_spare_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - invalid spare object, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The only supported states for the spare drive are Ready or Hibernate.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &spare_lifecycle,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     selected_spare_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if ((status != FBE_STATUS_OK)                                                        ||
        ((spare_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY)             &&
         (spare_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)         &&
         (spare_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_HIBERNATE)    )     )
    {
        /* Unexpected virtual drive lifecycle state.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: spare obj: 0x%x life: %d not supported - status: 0x%x\n",
                               __FUNCTION__, selected_spare_object_id, spare_lifecycle.lifecycle_state, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The only supported states for the virtual drive are Ready or Hibernate.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &vd_lifecycle,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if ((status != FBE_STATUS_OK)                                                    ||
        ((vd_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY)             &&
         (vd_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)         && 
         (vd_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_HIBERNATE)    )    )


    {
        /* Unexpected virtual drive lifecycle state.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x life: %d not supported - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, vd_lifecycle.lifecycle_state, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the spare drive is in hibernation exit hibernation and wait for
     * it to become Ready.
     */
    if (spare_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        /* Send the usurper to have the spare drive exit hibernation.
         */
        status = fbe_spare_lib_utils_wait_for_object_to_be_ready(selected_spare_object_id);
        if (status != FBE_STATUS_OK)
        {
            /* Unexpected virtual drive lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: spare obj: 0x%x waited: %d seconds to exit hibernation failed.\n",
                               __FUNCTION__, selected_spare_object_id, wait_for_ready_secs);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* If the virtual drive is in hibernation exit hibernation and wait for
     * it to become Ready.
     */
    if (vd_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        /* Wait for virtual drive to become Ready.
         */
        status = fbe_spare_lib_utils_wait_for_object_to_be_ready(vd_object_id);
        if (status != FBE_STATUS_OK)
        {
            /* Unexpected virtual drive lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x waited: %d seconds to exit hibernation failed.\n",
                               __FUNCTION__, vd_object_id, wait_for_ready_secs);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Get the updated virtual drive configuration mode.
     */
    status = fbe_spare_lib_utils_initiate_copy_get_updated_virtual_drive_configuration_mode(vd_object_id,
                                                                                   &updated_vd_configuration_mode);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed to get updated mode - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Step 2:  Start the transaction to initiate the copy operation.
     */
    status = fbe_spare_lib_utils_start_database_transaction(&js_swap_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - start transaction failed, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, status);
        return status;
    }

    /* Step 2a: If this is a user copy we must send a usurper to the the 
     *          virtual drive to `initiate' the copy request.
     */
    if ((js_swap_request_p->command_type == FBE_SPARE_INITIATE_USER_COPY_COMMAND)    ||
        (js_swap_request_p->command_type == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )
    {
        /* Send the usurper to start the copy
         */
        status = fbe_spare_lib_utils_start_user_copy_request(vd_object_id,
                                                             js_swap_request_p->command_type,
                                                             js_swap_request_p->swap_edge_index,
                                                             js_swap_request_p->b_operation_confirmation);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: initiate user copy failed - status:%d\n", 
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
    }

    /* Step 3: If this is a proactive copy request send the usurper to parent 
     *         raid group that increases the downstream media error thresholds.
     */
    if (js_swap_request_p->command_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
    {
        /* Get the upstream object id.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                         &upstream_object_list,
                                                         sizeof(fbe_base_config_upstream_object_list_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         vd_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if ((status != FBE_STATUS_OK)                              ||
            (upstream_object_list.number_of_upstream_objects != 1)    )
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: get upstream for emeh failed. vd obj:0x%x status: %d\n",
                                   __FUNCTION__, vd_object_id, status);
    
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, status);
            return status;
        }

        /* Now send usurper to have parent raid group enter EMEH `high availability mode'.
         */
        parent_set_emeh_request.set_control = FBE_RAID_EMEH_COMMAND_PACO_STARTED;
        parent_set_emeh_request.b_is_paco = FBE_TRUE;
        parent_set_emeh_request.paco_vd_object_id = vd_object_id;
        parent_set_emeh_request.b_change_threshold = FBE_FALSE;
        parent_set_emeh_request.percent_threshold_increase = 0;
        status = fbe_spare_lib_utils_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                         &parent_set_emeh_request,
                                                         sizeof(fbe_raid_group_set_extended_media_error_handling_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         upstream_object_list.upstream_object_list[0],
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            /* This is a warning and we continue (it is not neccessary to 
             * change the thresholds).
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: set emeh for rg obj:0x%x vd obj: 0x%x status: %d\n",
                                   __FUNCTION__, upstream_object_list.upstream_object_list[0], 
                                   vd_object_id, status);
        }
    }

    /* Step 4: Change the configuration type of the pvd consumed to 
     *         `in-use by RAID group'.
     */
    update_provision_drive.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.object_id = selected_spare_object_id;
    update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
    update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE;
    update_provision_drive.update_opaque_data = FBE_FALSE;

    status = fbe_spare_lib_get_provision_drive_update_type(selected_spare_object_id, vd_object_id, &update_provision_drive);
    if(status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - get object enc mode failed, vd_id:0x%x, hs_id:0x%x st: %d\n",
                               __FUNCTION__, vd_object_id, selected_spare_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* update the configuration type of the spare pvd object to raid. */
    status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - update config type failed, vd_id:0x%x, hs_id:0x%x\n",
                               __FUNCTION__, vd_object_id, selected_spare_object_id);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 5: Update the new drive with the pool ID of the original PVD.
        */
    status = fbe_spare_lib_utils_switch_pool_ids(js_swap_request_p, selected_spare_object_id);
    if (status != FBE_STATUS_OK)
    {
        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 6: Update the VD non paged metadata to indicate that we swapped in 
     *         a new edge.
     */
    status = fbe_spare_lib_utils_set_vd_nonpaged_metadata(packet_p, js_swap_request_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:updating the NP failed, vd_id:0x%x\n",
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 7: Configure a new edge with selected spare drive.  Normally this
     *         operation is fast but if encryption is enabled we must provide
     *         the new keys etc.  Therefore we must always wait for the operation
     *         to complete.
     */
    status = fbe_spare_lib_utils_database_service_create_edge(js_swap_request_p->transaction_id,
                                                              selected_spare_object_id,
                                                              vd_object_id,
                                                              copy_swap_in_edge_index,
                                                              block_edge_info.capacity,
                                                              block_edge_info.offset,
                                                              FBE_TRUE /* Always wait for the swap-in to complete */);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in spare failed - create edge failed, vd_id:0x%x, edge_idx:0x%x\n",
                               __FUNCTION__, vd_object_id, copy_swap_in_edge_index);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CREATE_EDGE_FAILED);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 8: Change the virtual drive configuration mode to mirror.
     */
    update_virtual_drive.transaction_id = js_swap_request_p->transaction_id;
    update_virtual_drive.object_id = vd_object_id;
    update_virtual_drive.update_type = FBE_UPDATE_VD_MODE;
    update_virtual_drive.configuration_mode = updated_vd_configuration_mode;

    /* update the virtual drive configuration mode to mirror */
    status = fbe_spare_lib_utils_update_virtual_drive_configuration_mode(&update_virtual_drive,
                                                                         js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed to update config mode - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    
        /* Determine if the virtual drive failed after the copy process was started.
         */
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &vd_lifecycle,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
        if ((status != FBE_STATUS_OK)                                   ||
            (vd_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY)    )
        {
            /* Unexpected virtual drive lifecycle state.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: update mode vd obj: 0x%x life: %d not supported - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, vd_lifecycle.lifecycle_state, status);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Else unexpected failure.
         */
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Populate the job element status.
     */
    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_spare_lib_utils_complete_packet(packet_p, status);
    return status;
}
/*********************************************
 * end fbe_spare_lib_initiate_copy_operation()
 *********************************************/

/*!***************************************************************************
 *          fbe_spare_lib_complete_copy_operation()
 ***************************************************************************** 
 * 
 * @brief   This function is used to detach an edge between VD object and proactive spare 
 * PVD object, it updates this configuration in memory on both the SPs.
 *
 * @param packet_p              - The packet requesting this operation.
 * @param job_service_js_swap_request_p  - Sparing request info.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/31/2012  Ron Proulx  - Updated to perform change to mirror mode also.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_complete_copy_operation(fbe_packet_t *packet_p,
                            fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                        swap_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        mirror_swap_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         destination_pvd_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_database_control_update_pvd_t       update_provision_drive;
    fbe_database_control_update_vd_t        update_virtual_drive;
    fbe_virtual_drive_configuration_mode_t  updated_vd_configuration_mode;
    fbe_system_encryption_mode_t            encryption_mode;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the payload.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* Step 1: Validate the copy complete request.
     */

    /* get the virtual drive object-id from the job service swap-out request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id (js_swap_request_p, &vd_object_id);

    /* get the edge-index from where we need to swap-out the proactive spare drive. */
    fbe_job_service_drive_swap_request_get_swap_edge_index(js_swap_request_p, &swap_out_edge_index);

    /* get the mirrored edge-index of the edge we need to swap-out. */
    fbe_job_service_drive_swap_request_get_mirror_swap_edge_index(js_swap_request_p, &mirror_swap_out_edge_index);

    /* fill information to be used by event logging */
    /* get the server object id for the swap out edge index. */
    fbe_spare_lib_utils_get_server_object_id_for_edge(vd_object_id, swap_out_edge_index, &source_pvd_object_id);

    fbe_job_service_drive_swap_request_set_original_pvd_object_id(js_swap_request_p, source_pvd_object_id);
   
    /* get the destenation pvd id */
    fbe_spare_lib_utils_get_server_object_id_for_edge(vd_object_id, mirror_swap_out_edge_index, &destination_pvd_obj_id);

    fbe_job_service_drive_swap_request_set_spare_object_id(js_swap_request_p, destination_pvd_obj_id);

    /* Determine if encryption mode is enabled.
     */
    fbe_database_get_system_encryption_mode(&encryption_mode);

    /* Get the updated virtual dirve configuration mode.
     */
    status = fbe_spare_lib_utils_complete_copy_get_updated_virtual_drive_configuration_mode(vd_object_id,
                                                                                   &updated_vd_configuration_mode);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed to get updated mode - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }
       
    /* Step 2: Start the transaction to perform the `complete copy' operation.
     */
    status = fbe_spare_lib_utils_start_database_transaction(&js_swap_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd_obj: 0x%x swap out index: %d start failed\n",
                               __FUNCTION__, vd_object_id, swap_out_edge_index);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, status);
        return status;
    }

    /* Step 3: Update the virtual drive configuration mode to pass-thru
     */
    update_virtual_drive.transaction_id = js_swap_request_p->transaction_id;
    update_virtual_drive.object_id = vd_object_id;
    update_virtual_drive.update_type = FBE_UPDATE_VD_MODE;
    update_virtual_drive.configuration_mode = updated_vd_configuration_mode;

    /* Update the virtual drive configuration mode to pass-thru.*/
    status = fbe_spare_lib_utils_update_virtual_drive_configuration_mode(&update_virtual_drive,
                                                                         js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed to update config mode - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 4: If encryption is enabled, temporarily mark `swap pending' in the
     *         source pvd non-paged so that if a rollback occurs the encryption
     *         zero process will not run and the data will be preserved.
     */
    if (encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) 
    {
        status = fbe_spare_lib_utils_database_service_mark_pvd_swap_pending(js_swap_request_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x source obj: 0x%x mark pvd swap pending failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, source_pvd_object_id, status);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }
    }

    /* Step 5: Destroy an specific edge between VD and spare PVD object.
     */
    status = fbe_spare_lib_utils_database_service_destroy_edge(js_swap_request_p->transaction_id,
                                                               vd_object_id,
                                                               swap_out_edge_index,
                                                               js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x swap-out index: %d failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, swap_out_edge_index, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 6: Update the configuration type of the original pvd to `unconsumed'.
     */
    if(js_swap_request_p->orig_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        update_provision_drive.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
        update_provision_drive.update_type_bitmask = 0;
        update_provision_drive.object_id = js_swap_request_p->orig_pvd_object_id;
        update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
        update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE; 
        update_provision_drive.update_opaque_data = FBE_FALSE;

        status = fbe_spare_lib_get_provision_drive_update_type(js_swap_request_p->orig_pvd_object_id, FBE_OBJECT_ID_INVALID, &update_provision_drive);
        if(status != FBE_STATUS_OK) {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: get object enc mode failed, vd_id:0x%x, hs_id:0x%x st: %d\n",
                                   __FUNCTION__, vd_object_id, js_swap_request_p->orig_pvd_object_id, status);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }

        /* change the configuration type as unknown because this drive has been  removed from the raid group. */
        status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: update config type failed, vd_id:0x%x, original drive pvd:0x%x\n",
                                   __FUNCTION__, vd_object_id, js_swap_request_p->orig_pvd_object_id);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }

        /*now update the pool ID of the PVD*/
        update_provision_drive.pool_id = FBE_POOL_ID_INVALID;
        update_provision_drive.object_id = js_swap_request_p->orig_pvd_object_id;
        update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
        update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;
        update_provision_drive.update_opaque_data = FBE_FALSE;
        /* update the configuration tpe of the spare pvd object to raid. */
        status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: update config type failed, vd_id:0x%x, dest pvd:0x%x\n",
                                   __FUNCTION__, vd_object_id, destination_pvd_obj_id);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }
    }

    /* Step 7: Update the virtual drive non-paged metadata (a.k.a set the
     *         rebuild checkpoint to the end marker).
     */
    status = fbe_spare_lib_utils_set_vd_checkpoint_to_end_marker(vd_object_id,
                                                                 js_swap_request_p->command_type,
                                                                 js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x set chkpt to end failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Success.
     */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_spare_lib_utils_complete_packet(packet_p, status);
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_complete_copy_operation()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_complete_aborted_copy_operation()
 ***************************************************************************** 
 * 
 * @brief   This function is used to detach an edge between VD object and proactive spare 
 * PVD object, it updates this configuration in memory on both the SPs.
 *
 * @param packet_p              - The packet requesting this operation.
 * @param job_service_js_swap_request_p  - Sparing request info.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/07/2012  Ron Proulx  - Updated to perform change to mirror mode also.
 *
 *****************************************************************************/
fbe_status_t fbe_spare_lib_complete_aborted_copy_operation(fbe_packet_t *packet_p,
                                                           fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_edge_index_t                        failed_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                         source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                        destination_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                         destination_pvd_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         failed_pvd_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_virtual_drive_configuration_mode_t  vd_configuration_mode;
    fbe_database_control_update_pvd_t       update_provision_drive;
    fbe_database_control_update_vd_t        update_virtual_drive;
    fbe_virtual_drive_configuration_mode_t  updated_vd_configuration_mode;


    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the payload.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* Step 1: Validate the copy complete request.
     */

    /* get the virtual drive object-id from the job service swap-out request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id (js_swap_request_p, &vd_object_id);

    /* get the edge-index from where we need to swap-out the proactive spare drive. */
    fbe_job_service_drive_swap_request_get_swap_edge_index(js_swap_request_p, &failed_edge_index);

    /* get the configuration mod eto determine which pvd object failed.*/
    fbe_spare_lib_utils_get_configuration_mode(vd_object_id, &vd_configuration_mode);
    switch(vd_configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* Set the source and destination edge indexes.
             */
            source_edge_index = FIRST_EDGE_INDEX;
            destination_edge_index = SECOND_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Set the source and destination edge indexes.
             */
            source_edge_index = SECOND_EDGE_INDEX;
            destination_edge_index = FIRST_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
        default:
            /* Unexpected configuration mode.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x unexpected configuration mode: %d\n",
                               __FUNCTION__, vd_object_id, vd_configuration_mode);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* fill information to be used by event logging */
    /* get the server object id for the swap out edge index. */

    fbe_spare_lib_utils_get_server_object_id_for_edge(vd_object_id, source_edge_index, &source_pvd_object_id);
    fbe_job_service_drive_swap_request_set_original_pvd_object_id(js_swap_request_p, source_pvd_object_id);
   
    /* get the destination pvd object id */
    fbe_spare_lib_utils_get_server_object_id_for_edge(vd_object_id, destination_edge_index, &destination_pvd_obj_id);
    fbe_job_service_drive_swap_request_set_spare_object_id(js_swap_request_p, destination_pvd_obj_id);

    /* Get the failed pvd object id */
    fbe_spare_lib_utils_get_server_object_id_for_edge(vd_object_id, failed_edge_index, &failed_pvd_obj_id);

    /* Get the updated virtual drive configuration mode.
     */
    status = fbe_spare_lib_utils_complete_aborted_copy_get_updated_virtual_drive_configuration_mode(vd_object_id,
                                                                                   failed_edge_index,
                                                                                   &updated_vd_configuration_mode);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed to get updated mode - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }
       
    /* Step 2: Start the transaction to perform the `complete copy' operation.
     */
    status = fbe_spare_lib_utils_start_database_transaction(&js_swap_request_p->transaction_id, job_queue_element_p->job_number);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd_obj: 0x%x swap out index: %d start failed\n",
                               __FUNCTION__, vd_object_id, failed_edge_index);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, status);
        return status;
    }

    /* Step 3: Update the virtual drive configuration mode to pass-thru
     */
    update_virtual_drive.transaction_id = js_swap_request_p->transaction_id;
    update_virtual_drive.object_id = vd_object_id;
    update_virtual_drive.update_type = FBE_UPDATE_VD_MODE;
    update_virtual_drive.configuration_mode = updated_vd_configuration_mode;

    /* Update the virtual drive configuration mode to pass-thru.*/
    status = fbe_spare_lib_utils_update_virtual_drive_configuration_mode(&update_virtual_drive,
                                                                         js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed to update config mode - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 4: Destroy an specific edge between VD and spare PVD object.
     */
    status = fbe_spare_lib_utils_database_service_destroy_edge(js_swap_request_p->transaction_id,
                                                               vd_object_id,
                                                               failed_edge_index,
                                                               js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed index: %d failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, failed_edge_index, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Step 5: Update the configuration type of the failed pvd object to `unconsumed'
     */
    if(js_swap_request_p->orig_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        update_provision_drive.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;
        update_provision_drive.update_type_bitmask = 0;
        update_provision_drive.object_id = failed_pvd_obj_id;
        update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
        update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE; 
        update_provision_drive.update_opaque_data = FBE_FALSE;

        status = fbe_spare_lib_get_provision_drive_update_type(failed_pvd_obj_id, FBE_OBJECT_ID_INVALID, &update_provision_drive);
        if(status != FBE_STATUS_OK) {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: get object enc mode failed, vd_id:0x%x, hs_id:0x%x st: %d\n",
                                   __FUNCTION__, vd_object_id, failed_pvd_obj_id, status);
    
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }

        /* change the configuration type as unknown because this drive has been  removed from the raid group. */
        status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: update config type failed - vd_id:0x%x, failed drive pvd:0x%x\n",
                                   __FUNCTION__, vd_object_id, failed_pvd_obj_id);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }

        /*now update the pool ID of the PVD*/
        update_provision_drive.pool_id = FBE_POOL_ID_INVALID;
        update_provision_drive.object_id = failed_pvd_obj_id;
        update_provision_drive.transaction_id = js_swap_request_p->transaction_id;
        update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;
        update_provision_drive.update_opaque_data = FBE_FALSE;
        /* update the configuration tpe of the spare pvd object to raid. */
        status = fbe_spare_lib_utils_database_service_update_pvd(&update_provision_drive);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: update config type failed - vd_id:0x%x, failed pvd:0x%x\n",
                                   __FUNCTION__, vd_object_id, failed_pvd_obj_id);

            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
            return status;
        }
    }

    /* Step 6: Update the virtual drive non-paged metadata (a.k.a set the
     *         rebuild checkpoint to the end marker).
     */
    status = fbe_spare_lib_utils_set_vd_checkpoint_to_end_marker(vd_object_id,
                                                                 js_swap_request_p->command_type,
                                                                 js_swap_request_p->b_operation_confirmation);
    if (status != FBE_STATUS_OK)    
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x set chkpt to end failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
    
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Success.
     */
    job_queue_element_p->status = status;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;  // status will only be OK when it gets here.

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_spare_lib_utils_complete_packet(packet_p, status);
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_complete_aborted_copy_operation()
 ******************************************************************************/


/******************************* 
 * end of file fbe_spare_swap.c
 *******************************/
