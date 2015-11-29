/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare_validation.c
 ***************************************************************************
 *
 * @brief
 *  This file contains drive swap request validation functionality.
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
#include "fbe/fbe_topology_interface.h"
#include "fbe_spare.h"
#include "fbe_spare_lib_private.h"
#include "fbe_job_service.h"
#include "fbe/fbe_object.h"

/*!****************************************************************************
 * fbe_spare_lib_validation_virtual_drive_error_to_swap_error()
 ******************************************************************************
 * @brief
 * Convert the error reported by the virtual drive the appropriate swap status.
 *
 * @param js_swap_request_p - Sparing request info.
 * @param virtual_drive_swap_status - Status returned from virtual drive
 * 
 * @return status
 *
 * @author
 *  04/06/2012  Ron Proulx  - Created.
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_validation_virtual_drive_error_to_swap_error(fbe_job_service_drive_swap_request_t *js_swap_request_p,
                                                                               fbe_virtual_drive_swap_status_t virtual_drive_swap_status)
{
    /* Based on virtual drive status
     */
    switch(virtual_drive_swap_status)
    {
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_NO_ERROR);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_PERMANENT_SPARE_NOT_REQUIRED:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_PERMANENT_SPARE_NOT_REQUIRED);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_PROACTIVE_SPARE_NOT_REQUIRED:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_BROKEN:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_VIRTUAL_DRIVE_DEGRADED:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_PERMANENT_SPARE_REQUEST:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_PROACTIVE_SPARE_REQUEST:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_UPSTREAM_DENIED_USER_COPY_REQUEST:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED);
            break;
        case FBE_VIRTUAL_DRIVE_SWAP_STATUS_UNSUPPORTED_COMMAND:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND);
            break;
        default:
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR);
            break;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_validation_virtual_drive_error_to_swap_error()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_validation_validate_user_request_swap_edge_index()
 ******************************************************************************
 * @brief   Using the virtual drive configuration mode determine the edge index
 *          for the destination drive of a user swap request.
 *
 * @param   js_swap_request_p                 - Sparing request info.
 * 
 * @return  status - The status of the operation.
 *
 * @note    This method does not modify the job queue element.
 *
 * @author
 *  12/11/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_validation_validate_user_request_swap_edge_index(fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     vd_object_id;
    fbe_virtual_drive_configuration_t   get_configuration;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* initialize configuration mode as unknown. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);
    get_configuration.configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* get the configuration of the virtual drive object. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                     &get_configuration,
                                                     sizeof(fbe_virtual_drive_configuration_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        /* no object could happen when object is destroyed. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               (status==FBE_STATUS_NO_OBJECT)?FBE_TRACE_LEVEL_INFO:FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in get config vd(0x%x) object validation failed\n",
                               __FUNCTION__, vd_object_id);

        /* Set the proper status in the job request.
         */
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return status;
    }

    /* The `swap edge index' is the edge that the destination drive will use.
     * The `mirror' edge is the original edge index.  Thus if we are in pass-thru
     * first edge, the swap-in index would be the second edge and visa versa.
     */
    switch(get_configuration.configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
        default:
            /* These are unsupported or unexpected.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: swap-in user get swap index for vd obj: 0x%x, invalid config mode: %d\n",
                                   __FUNCTION__, vd_object_id, get_configuration.configuration_mode);

            /* Set the proper status in the job request.
             */
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_validation_validate_user_request_swap_edge_index()
 ******************************************************************************/

/*!***************************************************************************
 * fbe_spare_lib_validation_validate_virtual_drive_object_id()
 *****************************************************************************
 *
 * @brief   Using the virtual drive object id from the swap request, validate
 *          that supplied object id is indeed for a virtual drive.
 *
 * @param   js_swap_request_p                 - Sparing request info.
 * 
 * @return  status - The status of the operation.
 *
 * @note    This method does not modify the job queue element.
 *
 * @author
 *  07/31/2012  - Ron Proulx    - Created.
 *****************************************************************************/
fbe_status_t fbe_spare_lib_validation_validate_virtual_drive_object_id(fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     vd_object_id;
    fbe_base_object_mgmt_get_class_id_t base_object_mgmt_get_class_id;  

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the virtual drive object id from the incoming job service drive swap request. 
     */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);
    if (vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in validation failed, invalid vd object:%u \n",
                               __FUNCTION__, vd_object_id);

        /* Set the proper status in the job request.
         */
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the virtual drive information.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                     &base_object_mgmt_get_class_id,
                                                     sizeof (fbe_base_object_mgmt_get_class_id_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: get vd class id - status:%d\n", 
                               __FUNCTION__, status);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Simply validate that this is a virtual drive
     */
    if (base_object_mgmt_get_class_id.class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: class id: %d of vd obj: 0x%x not expected: %d\n",
                               __FUNCTION__, base_object_mgmt_get_class_id.class_id, 
                               vd_object_id, FBE_CLASS_ID_VIRTUAL_DRIVE);

        /* Set the proper status in the job request.
         */
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_validation_validate_virtual_drive_object_id()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_validation_copy_to_validate_destination()
 ******************************************************************************
 * @brief
 * This function validates that the dest. drive from the user copy request fits
 *
 * @param js_swap_request_p - Sparing request info.
 *
 * @return status
 *
 * @author
 *  3/14/2011   - Created. Maya Dagon
 *  5/11/2012   - Modified - Prahlad Purohit
 *  07/27/2012  - Fixed - Ron Proulx
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_validation_copy_to_validate_destination(fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_block_transport_control_get_edge_info_t     block_edge_info;
    fbe_bool_t                                      dest_drive_fit = TRUE;
    fbe_object_id_t                                 vd_object_id;
    fbe_object_id_t                                 spare_pvd_object_id;
    fbe_spare_drive_info_t                          dest_drive_info;
    fbe_spare_drive_info_t                          desired_spare_drive_info;
    fbe_performance_tier_number_t                   performance_tier_number;
    fbe_base_object_mgmt_get_lifecycle_state_t      base_object_mgmt_get_lifecycle_state;
    fbe_base_config_upstream_object_list_t          upstream_object_list;
    fbe_u32_t                                       upstream_index = 0;
    fbe_spare_drive_info_t                          destination_spare_drive_info;

    /* Get the virtual drive object id from the incoming job service  request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);

    /* Get the provision drive object id from the incoming job service  request. */
    fbe_job_service_drive_swap_request_get_spare_object_id(js_swap_request_p, &spare_pvd_object_id);

    if (spare_pvd_object_id == FBE_EDGE_INDEX_INVALID)
    {
        /* invalid spare object-id */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x invalid dest object id\n",
                               __FUNCTION__, vd_object_id);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_INVALID_DESTINATION_DRIVE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check that PVD state is READY */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &base_object_mgmt_get_lifecycle_state,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     spare_pvd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x dest: 0x%x failed - status:%d\n", 
                               __FUNCTION__, vd_object_id, spare_pvd_object_id, status);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If destination drive lifecycle state is Ready or Hibernate then it is healthy.
     */
    if ((base_object_mgmt_get_lifecycle_state.lifecycle_state != FBE_LIFECYCLE_STATE_READY)     &&
        (base_object_mgmt_get_lifecycle_state.lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)    )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x dest: 0x%x not ready \n", 
                               __FUNCTION__, vd_object_id, spare_pvd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get spare drive information
     */
    fbe_spare_initialize_spare_drive_info(&destination_spare_drive_info);
    status = fbe_spare_lib_utils_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
                                                     &destination_spare_drive_info,
                                                     sizeof (fbe_spare_drive_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     spare_pvd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x dest: 0x%x get spare info failed - status: 0x%x\n", 
                               __FUNCTION__, vd_object_id, spare_pvd_object_id, status);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that End-Of-Life is not set.
     */
    if (destination_spare_drive_info.b_is_end_of_life == FBE_TRUE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x dest: 0x%x has EOL set\n",
                               __FUNCTION__, vd_object_id, spare_pvd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the drive is not in SLF.
     */
    /*! @note we should add a option if the user forced to copy to the drive. */
    if (destination_spare_drive_info.b_is_slf == FBE_TRUE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x dest: 0x%x has SLF set\n",
                               __FUNCTION__, vd_object_id, spare_pvd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get downstream edge */
    fbe_spare_lib_utils_get_block_edge_info(spare_pvd_object_id, 0, &block_edge_info);
    
    /* get path attributes of the block edge. Return FALSE if it has EOL bit set or if it was removed. */
    if ((block_edge_info.base_edge_info.path_state != FBE_PATH_STATE_ENABLED) &&
        (block_edge_info.base_edge_info.path_state != FBE_PATH_STATE_SLUMBER)    )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x dest: 0x%x removed\n",
                               __FUNCTION__, vd_object_id, spare_pvd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* make sure that destination drive doesn't have an upstream object */
    
    /* send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                     &upstream_object_list,
                                                     sizeof(fbe_base_config_upstream_object_list_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     spare_pvd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x get upstream failed - status: 0x%x\n", 
                               __FUNCTION__, vd_object_id, status);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /*! @note Since we allow system drives to be used for the destination of a
     *        copy request, we cannot simply check if the request drive as any
     *        upsteam objects or not.  Therefore we need to validate that if
     *        there any upstream objects, that they are all system objects.
     */
    if (upstream_object_list.number_of_upstream_objects > 0)
    {
        // Check upstream objects to determine if there are any objects other than system RGs.
        for (upstream_index = 0; upstream_index < upstream_object_list.number_of_upstream_objects; upstream_index++)
        {
            if(!fbe_database_is_object_system_rg(upstream_object_list.upstream_object_list[upstream_index]))
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: vd obj: 0x%x dest: 0x%x has upstream obj: 0x%x\n",
                                       __FUNCTION__, vd_object_id, 
                                       spare_pvd_object_id, upstream_object_list.upstream_object_list[upstream_index]);
                fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_HAS_UPSTREAM_RAID_GROUP);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    /* Get destination drive information */
    fbe_spare_lib_selection_get_hot_spare_info(spare_pvd_object_id, &dest_drive_info);
     
    /* get desired spare info */
    fbe_spare_lib_selection_get_virtual_drive_spare_info(vd_object_id, &desired_spare_drive_info);

    /* Check whether the destination drive type is in performance tier table with row index as desired spare drive type. */
    fbe_spare_lib_selection_get_drive_performance_tier_number(desired_spare_drive_info.drive_type, &performance_tier_number);
    fbe_spare_lib_utils_is_drive_type_valid_for_performance_tier(performance_tier_number, dest_drive_info.drive_type, &dest_drive_fit);
    if (dest_drive_fit == FALSE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x drive type: %d tier: %d doesnt match\n",
                               __FUNCTION__, vd_object_id, dest_drive_info.drive_type, performance_tier_number);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check offset */
    if (dest_drive_info.exported_offset > desired_spare_drive_info.exported_offset)
    {   
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd_obj: 0x%x desired offset 0x%llx dest offset 0x%llx\n",
                               __FUNCTION__, vd_object_id,desired_spare_drive_info.exported_offset, dest_drive_info.exported_offset);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_OFFSET_MISMATCH);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check capacity fit */
    fbe_spare_lib_utils_check_capacity(desired_spare_drive_info.capacity_required, dest_drive_info.configured_capacity,&dest_drive_fit);
    if (dest_drive_fit == FALSE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x desired cap: 0x%llx dest cap: 0x%llx\n",
                               __FUNCTION__, vd_object_id, desired_spare_drive_info.configured_capacity, dest_drive_info.configured_capacity);
        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_CAPACITY_MISMATCH);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_validation_copy_to_validate_destination()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_validation_swap_in_request()
 ******************************************************************************
 * @brief
 * This function validates the incoming drive swap-in request.
 *
 * @param js_swap_request_p                 - Sparing request info.
 * 
 * @return status - The status of the operation.
 *
 * @author
 *  12/11/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_validation_swap_in_request(fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         vd_object_id;
    fbe_object_id_t                         original_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_spare_swap_command_t                command_type;
    fbe_edge_index_t                        swap_in_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_swap_in_validation_t  virtual_drive_swap_in_validation;
    fbe_job_service_error_type_t            status_code = FBE_JOB_SERVICE_ERROR_INVALID;
    fbe_spare_get_upsteam_object_info_t     upstream_object_info;
    fbe_bool_t                              b_confirmation_enabled;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the virtual drive object id from the incoming job service drive swap request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);
    if (vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:swap-in validation failed, invalid vd object:%u\n",
                               __FUNCTION__, vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the swap-in command type.
     */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &command_type);

    /* swap-in edge request command must give a valid edge index which it wants to swap-in. */
    fbe_job_service_drive_swap_request_get_swap_edge_index(js_swap_request_p, &swap_in_edge_index);
    if (swap_in_edge_index == FBE_EDGE_INDEX_INVALID)
    {
        /* invalid edge index and so complete the packet with error. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: failed - invalid edge index, vd obj:0x%x, edge_idx:0x%x.\n",
                               __FUNCTION__, vd_object_id, swap_in_edge_index);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the original pvd object id for debug.
     */
    fbe_job_service_drive_swap_request_get_original_pvd_object_id(js_swap_request_p, &original_pvd_object_id);

    /* send the request to virtual drive object to validate before it starts swap-in request. */
    virtual_drive_swap_in_validation.swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;
    virtual_drive_swap_in_validation.command_type = command_type;
    virtual_drive_swap_in_validation.swap_edge_index = js_swap_request_p->swap_edge_index;
    fbe_job_service_drive_swap_request_get_confirmation_enable(js_swap_request_p, &b_confirmation_enabled);
    virtual_drive_swap_in_validation.b_confirmation_enabled = b_confirmation_enabled;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s send validation request to vd (0x%x), command_type:0x%x.\n", 
                           __FUNCTION__, vd_object_id, virtual_drive_swap_in_validation.command_type);

   /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_IN_VALIDATION,
                                                     &virtual_drive_swap_in_validation,
                                                     sizeof(fbe_virtual_drive_swap_in_validation_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* no object could happen when object is destroyed. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               (status==FBE_STATUS_NO_OBJECT)?FBE_TRACE_LEVEL_INFO:FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s failed - vd(0x%x) object validation failed\n",
                               __FUNCTION__, vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return status;
    }

    /* Translate the virtual drive swap request status to the job service status.
     */
    fbe_spare_lib_validation_virtual_drive_error_to_swap_error(js_swap_request_p,
                                                               virtual_drive_swap_in_validation.swap_status);
    fbe_job_service_drive_swap_request_get_status(js_swap_request_p, &status_code);

    /* The virtual drive should have populated the swap status with a valid code.
     */
    if (status_code == FBE_JOB_SERVICE_ERROR_INVALID)
    {
        /* This is unexpected.  Generate an error trace and change the status
         * to `internal error'.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x pvd obj: 0x%x vd status: %d unexpected status: %d\n", 
                               __FUNCTION__, vd_object_id, original_pvd_object_id,
                               virtual_drive_swap_in_validation.swap_status, status_code);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Even if the virtual drive rejected the request with `denied' we
     *        continue so that we can determine:
     *          o Rejected because parent raid type does support request (i.e. non-redundant)
     *                  OR
     *          o Rejected because no LU on parent raid group (i.e. unconsumed)
     *                  OR 
     *          o Rejected because parent raid group is degraded (copy operations
     *            are not allowed on degraded raid groups).
     */
    if ((status_code != FBE_JOB_SERVICE_ERROR_NO_ERROR)                    &&
        (status_code != FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED)    )
    {
        /* If the virtual drive didn't return success and didn't return `denied' 
         * then fail the request now.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x pvd obj: 0x%x vd status: %d swap status: %d\n", 
                               __FUNCTION__, vd_object_id, original_pvd_object_id,
                               virtual_drive_swap_in_validation.swap_status, status_code);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Only after we validate the source drive can we validate the 
     *        destination drive.
     */
    if (command_type == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)
    {
         status = fbe_spare_lib_validation_copy_to_validate_destination(js_swap_request_p);
         if (status != FBE_STATUS_OK)
         {
             fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s: vd obj: 0x%x destination validation failed - status: 0x%x\n",
                                    __FUNCTION__, vd_object_id, status);

             return FBE_STATUS_GENERIC_FAILURE;
         }
    }

    /* Now that we have validated the virtual drive, validate the parent raid group.
     */
    status = fbe_spare_lib_selection_get_virtual_drive_upstream_object_info(vd_object_id, &upstream_object_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Failed to get upstream info for vd obj: 0x%x - status: 0x%x\n", 
                               __FUNCTION__, vd_object_id, status);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return status;
    }

    /*! @note If the upstream raid group is not redundant we don't allow any
     *        type of sparing or copy operation.
     */
    if (upstream_object_info.b_is_upstream_object_redundant == FBE_FALSE)
    {
        /* We are already in the `swap-in' code.  None of the swap-in commands
         * support non-redundant raid groups.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream obj: 0x%x vd obj: 0x%x in not redundant.\n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id,
                               vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note If the upstream raid group is not in the Ready we will not 
     *        honor the request.
     */
    if (upstream_object_info.b_is_upstream_rg_ready == FBE_FALSE)
    {
        /* We don't allow any swap commands to a broekn raid group.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream obj: 0x%x vd obj: 0x%x in not Ready\n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id,
                               vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the upsteam raid group is not `in-use' we will reject the sparing request.
     */
    if (upstream_object_info.b_is_upstream_object_in_use == FBE_FALSE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream obj: 0x%x vd obj: 0x%x in not `in-use'\n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id,
                               vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the upstream raid group is degraded we don't allow copy operations.
     */
    if (upstream_object_info.b_is_upstream_object_degraded == FBE_TRUE)
    {
        /* Validate that the swap-in command is supported.
         */
        switch(command_type)
        {
            case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
                /* We must allow degraded raid groups to spare.*/
                break;

            case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
                /*! @note Due to the additional I/O and inherent risk we do not
                 *        allow any copy operation to degraded raid groups.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s Upstream obj: 0x%x vd obj: 0x%x is degraded.\n", 
                                       __FUNCTION__, upstream_object_info.upstream_object_id,
                                       vd_object_id);

                fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED);
                return FBE_STATUS_GENERIC_FAILURE;

            default:
                /* This means a new swap-in command was added. */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s Upstream obj: 0x%x vd obj: 0x%x is degraded bad command: %d\n", 
                                       __FUNCTION__, upstream_object_info.upstream_object_id,
                                       vd_object_id, command_type);

                fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
                return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* If there is already a copy in progress then we will fail the copy request. 
     * Our policy is to only allow (1) copy per raid group (even for RAID-10).
     */
    if (upstream_object_info.num_copy_in_progress > 0)
    {
        /* Validate that the swap-in command is supported.
         */
        switch(command_type)
        {
            case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
                /* Not applicable to permananent spare request.*/
                break;

            case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
                /*! @note Due to the additional I/O and inherent risk we do not
                 *        allow a second copy operation.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s Upstream obj: 0x%x vd obj: 0x%x has: %d copy operations in progress\n", 
                                       __FUNCTION__, upstream_object_info.upstream_object_id, vd_object_id,
                                       upstream_object_info.num_copy_in_progress);

                fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS);
                return FBE_STATUS_GENERIC_FAILURE;

            default:
                /* This means a new swap-in command was added. */
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s Upstream obj: 0x%x vd obj: 0x%x has copy in-progress bad command: %d\n", 
                                       __FUNCTION__, upstream_object_info.upstream_object_id,
                                       vd_object_id, command_type);

                fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
                return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (upstream_object_info.b_is_encryption_in_progress &&
        (command_type != FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream obj: 0x%x vd obj: 0x%x encryption in progress\n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id, vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_validation_swap_in_request()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_validation_swap_out_request()
 ******************************************************************************
 * @brief
 * This function validates the incoming drive swap-out request.
 *
 * @param js_swap_request_p - Sparing request info.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/11/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_validation_swap_out_request(fbe_job_service_drive_swap_request_t * js_swap_request_p)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_edge_index_t                swap_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                 vd_object_id;
    fbe_virtual_drive_swap_out_validation_t  virtual_drive_swap_out_validation;
    fbe_spare_swap_command_t        command_type;
    fbe_bool_t                      b_confirmation_enabled;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Get the virtual drive object id from the incoming job service drive swap request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id(js_swap_request_p, &vd_object_id);
    if (vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed - invalid object id\n",
                               __FUNCTION__, vd_object_id);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* swap-out edge request command must give a valid edge index which it wants to swap-out. */
    fbe_job_service_drive_swap_request_get_swap_edge_index(js_swap_request_p, &swap_out_edge_index);
    if (swap_out_edge_index == FBE_EDGE_INDEX_INVALID)
    {
        /* invalid edge index and so complete the packet with error. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x failed - invalid edge index: %d \n\n",
                               __FUNCTION__, vd_object_id, swap_out_edge_index);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* send the request to virtual drive object to validate before it starts swap-out request. */
    fbe_job_service_drive_swap_request_get_swap_command_type(js_swap_request_p, &command_type);
    virtual_drive_swap_out_validation.swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;
    virtual_drive_swap_out_validation.command_type = command_type;
    virtual_drive_swap_out_validation.swap_edge_index = swap_out_edge_index;
    fbe_job_service_drive_swap_request_get_confirmation_enable(js_swap_request_p, &b_confirmation_enabled);
    virtual_drive_swap_out_validation.b_confirmation_enabled = b_confirmation_enabled;

   /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_OUT_VALIDATION,
                                                     &virtual_drive_swap_out_validation,
                                                     sizeof(fbe_virtual_drive_swap_out_validation_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        /* no object could happen when object is destroyed. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: vd obj: 0x%x vd validation failed - status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        return status;
    }

    /* Translate the virtual drive swap request status to the job service status.
     */
    fbe_spare_lib_validation_virtual_drive_error_to_swap_error(js_swap_request_p,
                                                               virtual_drive_swap_out_validation.swap_status);

    /* if virtual drive object does not need swap-in any more then fail the request. */
    if (virtual_drive_swap_out_validation.swap_status != FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK)
    {
        /* no object could happen when object is destroyed. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: vd obj: 0x%x index: %d swap-out status: %d\n", 
                               __FUNCTION__, vd_object_id, swap_out_edge_index,
                               virtual_drive_swap_out_validation.swap_status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Trace some information */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: vd obj: 0x%x edge index: %d success\n",
                           __FUNCTION__, vd_object_id, swap_out_edge_index);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_validation_swap_out_request()
 ******************************************************************************/


/*************************************
 * end of file fbe_spare_validation.c
 *************************************/
