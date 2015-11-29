/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_spare_lib_selection.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the hot spare/proactive spare selection algorithm. It
 *  gathers all the required information from different object and stored
 *  configuration and based on this information it applies different 
 *  selection rules to select the best suitable spare for the swap-in 
 *  operation.
 *
 * @version
 *  12/09/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_limits.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_database.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_spare.h"
#include "fbe_spare_lib_private.h"
#include "fbe_job_service.h"
#include "fbe_private_space_layout.h"
#include "fbe_database.h"

// Finds the available spare pool with system using topology service.
static fbe_status_t fbe_spare_lib_selection_find_available_spare_pool(fbe_packet_t *packet_p,
                                                                      fbe_bool_t b_unused_as_spare,
                                                                      fbe_spare_drive_pool_t *spare_drive_pool_p);

// Finds the desired spare information to select the best suitable spare.
static fbe_status_t fbe_spare_lib_selection_get_desired_spare_drive_info(fbe_object_id_t vd_object_id,
                                                                         fbe_spare_drive_info_t *out_desired_spare_info_p);

// Finds the upstream RAID object list associated with virtual drive object.
static fbe_status_t fbe_spare_lib_selection_get_upstream_raid_object_list(fbe_packet_t *packet_p,
                                                                          fbe_object_id_t vd_object_id,
                                                                          fbe_base_config_upstream_object_list_t *base_config_upstream_object_list_p);

// Finds the downnstream VD object list associated with RAID object.
static fbe_status_t fbe_spare_lib_selection_get_downstream_vd_object_list(fbe_packet_t *in_packet_p,
                                                                          fbe_object_id_t in_raid_object_id,
                                                                          fbe_base_config_downstream_object_list_t *in_base_config_downstream_object_list_p);

// Finds the best suitable spare from the available spare pool using desired spare information as input.
static fbe_status_t fbe_spare_lib_selection_select_suitable_spare_from_spare_pool(fbe_packet_t *packet_p,
                                                                                  fbe_object_id_t * spare_object_id_p,
                                                                                  fbe_spare_drive_info_t *desired_spare_info_p,
                                                                                  fbe_spare_drive_pool_t *spare_drive_pool_p,
                                                                                  fbe_spare_drive_info_t *selected_spare_info_p,
                                                                                  fbe_object_id_t desired_spare_object_id);


/*!****************************************************************************
 * fbe_spare_initialize_spare_selection_drive_info()
 ******************************************************************************
 * @brief 
 *   It initializes the drive spare information with default values.
 *
 * @param spare_selection_drive_info_p   - spare drive information pointer.
 ******************************************************************************/
void fbe_spare_initialize_spare_selection_drive_info(fbe_spare_selection_info_t *spare_selection_drive_info_p)
{
    fbe_spare_lib_hot_spare_info_set_object_id(spare_selection_drive_info_p, FBE_OBJECT_ID_INVALID);
    fbe_spare_selection_init_spare_selection_priority(spare_selection_drive_info_p);

    fbe_spare_initialize_spare_drive_info(&spare_selection_drive_info_p->spare_drive_info);
    return;
}
/******************************************************************************
 * end fbe_spare_initialize_spare_selection_drive_info()
 ******************************************************************************/

/*!**************************************************************
 * fbe_spare_lib_selection_rg_get_num_copies()
 ****************************************************************
 * @brief
 *  Determine the number of copies that are in progress
 *  for a given raid group.
 *
 * @param rg_object_id
 * @param num_copies_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_spare_lib_selection_rg_get_num_copies(fbe_object_id_t rg_object_id,
                                                              fbe_u32_t *num_copies_p)
{
    fbe_status_t status;
    fbe_u32_t object_index;
    fbe_u32_t num_copies = 0;
    fbe_base_config_downstream_object_list_t ds_object_list;
    fbe_virtual_drive_configuration_t get_configuration;

    *num_copies_p = 0;
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                     &ds_object_list,
                                                     sizeof(fbe_base_config_downstream_object_list_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     rg_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s get ds objects for rg obj: 0x%x failed - status: 0x%x\n", 
                               __FUNCTION__, rg_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for ( object_index = 0; object_index < ds_object_list.number_of_downstream_objects; object_index++)
    {
        
        status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                         &get_configuration,
                                                         sizeof(fbe_virtual_drive_configuration_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         ds_object_list.downstream_object_list[object_index],
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare lib get num copies failed for rg: 0x%x index: %d ds_obj: 0x%x status: 0x%x\n",
                                   rg_object_id, object_index, ds_object_list.downstream_object_list[object_index],
                                   status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
            (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
        {
            num_copies++;
        }
    }
    *num_copies_p = num_copies;
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_spare_lib_selection_rg_get_num_copies()
 ************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_virtual_drive_upstream_object_info()
 ******************************************************************************
 * @brief
 * This function sends the get drive info control packet to the passed in 
 * virtual drive object and in return it gets required drive information for 
 * the selected upstream object.
 *
 * @param vd_object_id          - Virtual drive object id.
 * @param in_upstream_object_info_p - Virtual drive upstream object information
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/27/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_spare_lib_selection_get_virtual_drive_upstream_object_info(fbe_object_id_t in_vd_object_id,
                                                                            fbe_spare_get_upsteam_object_info_t *in_upstream_object_info_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_base_config_upstream_object_list_t  upstream_object_list;
    fbe_object_id_t                         upstream_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         parent_rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_get_info_t               raid_group_info;
    fbe_raid_group_get_degraded_info_t      raid_group_degraded_info;
    fbe_u32_t                               num_copies_inprogress = 0;
    fbe_base_object_mgmt_get_lifecycle_state_t base_object_mgmt_get_lifecycle_state;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL. */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (in_upstream_object_info_p);
    fbe_zero_memory(in_upstream_object_info_p, sizeof(*in_upstream_object_info_p));
    in_upstream_object_info_p->upstream_object_id = FBE_OBJECT_ID_INVALID;

    /* Verify the passed-in VD object-id. */
    if (in_vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid vd obj: 0x%x \n", 
                               __FUNCTION__, in_vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                     &upstream_object_list,
                                                     sizeof(fbe_base_config_upstream_object_list_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     in_vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s get upsteam objects for vd obj: 0x%x failed - status: 0x%x\n", 
                               __FUNCTION__, in_vd_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* If there is no upstream raid groups we should not be here.
     */
    if (upstream_object_list.number_of_upstream_objects == 0)
    {
        /* We expect at least (1) upstream raid group.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s No parent raid groups found for vd obj: 0x%x \n", 
                               __FUNCTION__, in_vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Currently we only support (1) parent raid group.
     */
    if (upstream_object_list.number_of_upstream_objects > 1)
    {
        /* We expect at least (1) upstream raid group.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Too many raid groups: %d found for vd obj: 0x%x \n", 
                               __FUNCTION__, upstream_object_list.number_of_upstream_objects, in_vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note Since we support (1) parent raid group, the edge to select is always 0.
     */
    in_upstream_object_info_p->upstream_edge_to_select = 0;

    /* Now get the raid group information
     */
    upstream_rg_object_id = upstream_object_list.upstream_object_list[0];
    parent_rg_object_id = upstream_rg_object_id;
    in_upstream_object_info_p->upstream_object_id = upstream_rg_object_id;
    status = fbe_spare_lib_utils_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                     &raid_group_info,
                                                     sizeof(fbe_raid_group_get_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     upstream_rg_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s get raid group info for rg obj: 0x%x vd obj: 0x%x failed. status: 0x%x\n", 
                               __FUNCTION__, upstream_rg_object_id, in_vd_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now determine whether the raid group is in progress.
     */
    if ((raid_group_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (raid_group_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS))
    {
        in_upstream_object_info_p->b_is_encryption_in_progress = FBE_TRUE;
    }
    else
    {
        /* If we complete the encryption process but the keys are not really pushed down then
         * reject the request 
         */
        if((raid_group_info.encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) &&
           (raid_group_info.encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s Upstream obj: 0x%x DIP Mode:%d State:%d\n", 
                                   __FUNCTION__, upstream_rg_object_id, 
                                   raid_group_info.encryption_mode,
                                   raid_group_info.encryption_state);
            in_upstream_object_info_p->b_is_encryption_in_progress = FBE_TRUE;
        }
        else 
        {
            in_upstream_object_info_p->b_is_encryption_in_progress = FBE_FALSE;
        }
    }

    /* Now determine if this is a `redundant' raid group type or not.
     */
    switch(raid_group_info.raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
            /* This doesn't support sparing.*/
            in_upstream_object_info_p->b_is_upstream_object_redundant = FBE_FALSE;
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
        case FBE_RAID_GROUP_TYPE_RAID6:
            /* These raid types are redundant.*/
            in_upstream_object_info_p->b_is_upstream_object_redundant = FBE_TRUE;
            break;

        default:
            /* Unsupported raid type.
             */ 
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s invalid raid type: %d for rg obj: 0x%x vd obj: 0x%x \n", 
                                   __FUNCTION__, raid_group_info.raid_type, upstream_rg_object_id, in_vd_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now determine if the raid group is `consumed' (i.e. are there any LUs bound).
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                     &upstream_object_list,
                                                     sizeof(fbe_base_config_upstream_object_list_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     upstream_rg_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s get upsteam objects for rg obj: 0x%x failed - status: 0x%x\n", 
                               __FUNCTION__, upstream_rg_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* If the raid type is RAID-10 do it again.
     */
    if (raid_group_info.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        /* Validate at (1) upstream object.
         */
        if (upstream_object_list.number_of_upstream_objects != 1)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s no upstream for raid type: %d rg obj: 0x%x vd obj: 0x%x\n", 
                                   __FUNCTION__, raid_group_info.raid_type, upstream_rg_object_id, in_vd_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*! @note The current policy looks at each mirrored pair of a RAID-10
         *        as the `parent' raid group.  This as long as the mirror
         *        associated with the request virtual drive is not:
         *          o Degraded
         *          o Already has a copy in-progress
         */
        parent_rg_object_id = upstream_object_list.upstream_object_list[0];
        status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                         &upstream_object_list,
                                                         sizeof(fbe_base_config_upstream_object_list_t),
                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                         FBE_CLASS_ID_INVALID,
                                                         parent_rg_object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s get upsteam objects for rg obj: 0x%x failed - status: 0x%x\n", 
                                   __FUNCTION__, parent_rg_object_id, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Get the raid group lifecycle state.  The only states allowed are:
     *  o Ready
     *  o Hibernate
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &base_object_mgmt_get_lifecycle_state,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     parent_rg_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
        
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s get life for rg obj: 0x%x failed - status:%d\n", 
                               __FUNCTION__, parent_rg_object_id, status);

        return status;
    }
    if ((base_object_mgmt_get_lifecycle_state.lifecycle_state == FBE_LIFECYCLE_STATE_READY)     ||
        (base_object_mgmt_get_lifecycle_state.lifecycle_state == FBE_LIFECYCLE_STATE_HIBERNATE)    )
    {
        in_upstream_object_info_p->b_is_upstream_rg_ready = FBE_TRUE;
    }
    else
    {
        /* Else report the parent raid group id that failed.
         */
        in_upstream_object_info_p->upstream_object_id = parent_rg_object_id;
        in_upstream_object_info_p->b_is_upstream_rg_ready = FBE_FALSE;
    }

    /*! @note If we want to treat RAID-10 as a single raid group, uncomment the line below.
     */
    //upstream_rg_object_id = (raid_group_info.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) ? parent_rg_object_id : upstream_rg_object_id;

    /* Now check if the parent raid group is degraded or not.
     */
    status = fbe_spare_lib_utils_send_control_packet(FBE_RAID_GROUP_CONTROL_CODE_GET_DEGRADED_INFO,
                                                     &raid_group_degraded_info,
                                                     sizeof(fbe_raid_group_get_degraded_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     upstream_rg_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s get degraded info for rg obj: 0x%x vd obj: 0x%x failed. status: 0x%x\n", 
                               __FUNCTION__, upstream_rg_object_id, in_vd_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_upstream_object_info_p->b_is_upstream_object_degraded = raid_group_degraded_info.b_is_raid_group_degraded;

    /* If there is already a copy in progress then we will fail the copy request. 
     * Our policy is to only allow (1) copy per raid group (even for RAID-10).
     */
    status = fbe_spare_lib_selection_rg_get_num_copies(upstream_rg_object_id, &num_copies_inprogress);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING, 
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s failed get of num copies for rg_obj: 0x%x status: 0x%x\n", 
                               __FUNCTION__, upstream_rg_object_id, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_upstream_object_info_p->num_copy_in_progress = num_copies_inprogress;

    /* If there is no upstream objects it means there are no LUs bound and
     * this is unconsumed.
     */
    if (upstream_object_list.number_of_upstream_objects == 0)
    {
        in_upstream_object_info_p->b_is_upstream_object_in_use = FBE_FALSE;
    }
    else
    {
        in_upstream_object_info_p->b_is_upstream_object_in_use = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_virtual_drive_upstream_object_info()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_spare_selection_validate_spare_request()
 ******************************************************************************
 *
 * @brief   This function validates the sparing request:
 *              o Validate that the `desired' spare information can be retrieved
 *                and is correct.
 *              o Validate that the parent raid group is `in-use'.
 *
 * @param   desired_spare_drive_info_p - Pointer to desired spare info to populate.
 * @param   packet_p - Pointer to original packet.
 * @param   job_queue_element_p - Pointer to job queue element (to populate status)
 * @param   js_swap_request_p - Drive swap request.
 * @param   performance_tier_number_p - Pointer to performance tier.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t fbe_spare_selection_validate_spare_request(fbe_spare_drive_info_t *desired_spare_drive_info_p,
                                                               fbe_packet_t *packet_p,
                                                               fbe_job_queue_element_t *job_queue_element_p,
                                                               fbe_job_service_drive_swap_request_t *js_swap_request_p,
                                                               fbe_performance_tier_number_t *performance_tier_number_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                    permanent_spare_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                     original_pvd_id;
    fbe_spare_get_upsteam_object_info_t upstream_object_info;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* get the virtual drive object-id from the job service swap-in request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id (js_swap_request_p, &vd_object_id);

    /* Get the edge-index where we need to swap-in the permanent spare. */
    fbe_spare_lib_utils_get_permanent_spare_swap_edge_index(vd_object_id, &permanent_spare_edge_index);
    if (FBE_EDGE_INDEX_INVALID == permanent_spare_edge_index)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Failed to get failed edge index for vd obj: 0x%x\n", 
                               __FUNCTION__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the swap edge index.
     */
    fbe_job_service_drive_swap_request_set_swap_edge_index(js_swap_request_p, permanent_spare_edge_index);

    /* get the server object id for the swap out edge index. */
    status = fbe_spare_lib_utils_get_server_object_id_for_edge(vd_object_id,permanent_spare_edge_index,&original_pvd_id);
    if (FBE_STATUS_OK != status)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Failed to get pvd id for vd obj: 0x%x edge index: %d\n",
                               __FUNCTION__, vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* Get the drive spare information from the drive which requires swap-in or
     * collect the information from all asssociated RAID drives and rationalize
     * this information and get the desired spare information for selection of
     * best suitable spare.
     */
    status = fbe_spare_lib_selection_get_desired_spare_drive_info(vd_object_id, desired_spare_drive_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Failed to get desired drive info for vd obj: 0x%x edge index: %d\n", 
                               __FUNCTION__, vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

	/* desired_spare_drive_info_p->drive_type may be wrong */
	if((desired_spare_drive_info_p->drive_type == FBE_DRIVE_TYPE_INVALID) || (desired_spare_drive_info_p->drive_type >= FBE_DRIVE_TYPE_LAST)){
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							   "%s Failed to get desired drive info drive_type for vd obj: 0x%x edge index: %d drive_type: %d\n", 
                               __FUNCTION__, vd_object_id, permanent_spare_edge_index, desired_spare_drive_info_p->drive_type);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;

	}


    /*Check whether the desired spare drive type has a valid performance tier*/
    status = fbe_spare_lib_selection_get_drive_performance_tier_number(desired_spare_drive_info_p->drive_type, performance_tier_number_p);
    if ((status != FBE_STATUS_OK) && (*performance_tier_number_p == FBE_PERFORMANCE_TIER_INVALID))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Failed to get performance tier for drive type: %d vd obj: 0x%x edge index: %d\n", 
                               __FUNCTION__, desired_spare_drive_info_p->drive_type,
                               vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE;
        return status;
    }

    /* Validate that the parent raid group is `in-use'
     */
    status = fbe_spare_lib_selection_get_virtual_drive_upstream_object_info(vd_object_id, &upstream_object_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Failed to get upstream info for vd obj: 0x%x edge index: %d\n", 
                               __FUNCTION__, vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = status;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        return status;
    }

    /* If the upsteam raid group is not redundant there is no point is swapping
     * in a spare drive.
     */
    if (upstream_object_info.b_is_upstream_object_redundant == FBE_FALSE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream obj: 0x%x vd obj: 0x%x edge index: %d in not redundant\n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id,
                               vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the upstream raid group is not in the Ready we will not honor the 
     * request.
     */
    if (upstream_object_info.b_is_upstream_rg_ready == FBE_FALSE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream raid group obj: 0x%x is not Ready. vd obj: 0x%x edge index: %d \n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id,
                               vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the upsteam raid group is not `in-use' we will reject the sparing request.
     */
    if (upstream_object_info.b_is_upstream_object_in_use == FBE_FALSE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Upstream obj: 0x%x vd obj: 0x%x edge index: %d in not `in-use'\n", 
                               __FUNCTION__, upstream_object_info.upstream_object_id,
                               vd_object_id, permanent_spare_edge_index);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If all the checks passed return success.
     */
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_spare_selection_validate_spare_request()
 **************************************************/

/*!****************************************************************************
 * fbe_spare_find_best_suitable_spare()
 ******************************************************************************
 * @brief
 * This function is used to process the drive swap-in request for swap-in the 
 * proactive spare or hot spare. Based on the request type it selects suitable
 * Proactive Spare or Hot Spare.
 *
 * @param packet_p - Packet pointer.
 * @param js_swap_request_p - Drive swap request.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t 
fbe_spare_lib_selection_find_best_suitable_spare(fbe_packet_t *packet_p,
                                                 fbe_job_service_drive_swap_request_t *js_swap_request_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_spare_drive_pool_t             *spare_drive_pool_p = NULL;
    fbe_spare_drive_info_t              desired_spare_drive_info;
    fbe_object_id_t                     vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                     selected_spare_object_id = FBE_OBJECT_ID_INVALID;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_performance_tier_number_t       performance_tier_number =  FBE_PERFORMANCE_TIER_INVALID;
    fbe_spare_drive_info_t              selected_spare_drive_info;
    fbe_bool_t                          unused_drive_as_spare = FBE_FALSE;
    fbe_job_queue_element_t            *job_queue_element_p = NULL;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /*! get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &job_queue_element_p);

    /* get the virtual drive object-id from the job service swap-in request. */
    fbe_job_service_drive_swap_request_get_virtual_drive_object_id (js_swap_request_p, &vd_object_id);

    /* Validate that spare request and if the request is required or not.
     */
    status = fbe_spare_selection_validate_spare_request(&desired_spare_drive_info,
                                                        packet_p,
                                                        job_queue_element_p,
                                                        js_swap_request_p,
                                                        &performance_tier_number);
    if (status != FBE_STATUS_OK)
    {
        /* The validation routine generated any trace information and 
         * populated the specific error code.  Simply mark the request as failed
         * and complete it.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: find spare - spare validation for vd obj: 0x%x failed. status: 0x%x\n", 
                               vd_object_id, status);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, job_queue_element_p->error_code);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return status;
    }

    /* Retrieve the value of flag from virtual drive object. */
    status = fbe_spare_lib_utils_get_unused_drive_as_spare_flag(vd_object_id, &unused_drive_as_spare);
    
    /* Allocate the memory for the drive spare pool. */
    spare_drive_pool_p = (fbe_spare_drive_pool_t *) fbe_memory_native_allocate(sizeof(fbe_spare_drive_pool_t));
    if (spare_drive_pool_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "spare_selection: find spare - failed-memory allocation error(line:%d), vd_id:0x%x\n",
                                __LINE__, vd_object_id);

        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the spare drive pool as invalid object ids. */
    fbe_spare_initialize_spare_drive_pool(spare_drive_pool_p);

    /* Send request to topology service to find out the list of suitable spare pool. 
     */
    status = fbe_spare_lib_selection_find_available_spare_pool(packet_p, unused_drive_as_spare, spare_drive_pool_p);

    /* Validate the following:
     *  o Get spare pool successful.
     *  o First spare drive object id is valid.
     */
    if ((status != FBE_STATUS_OK)                                           ||
        (spare_drive_pool_p->spare_object_list[0] == FBE_OBJECT_ID_INVALID)    )
    {
        /* Release the memory allocated to get the pool of spares.
         */
        fbe_memory_native_release(spare_drive_pool_p);

        /* If status is success populate:
         *  o swap status with `no spare'
         *  o job status with `generic failure'
         *  o job error code wtih `no spare'
         */
        if (status == FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare_selection: find spare for vd obj: 0x%x failed-no spare available\n", 
                                   vd_object_id);
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE);
            job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE;
        }
        else
        {
            /* Else an unexpected error occurred.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare_selection: find spare for vd obj: 0x%x failed-unexpected status: 0x%x\n", 
                                   vd_object_id, status);
            fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
            job_queue_element_p->status = status;
            job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR;
        }

        /* Update the drive swap request with 'invalid object id'. */
        fbe_job_service_drive_swap_request_set_spare_object_id(js_swap_request_p, FBE_OBJECT_ID_INVALID);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, status);
        return status;
    }

    /* Initialize the selected spare drive inforamtion with default values. */
    fbe_spare_initialize_spare_drive_info(&selected_spare_drive_info);

    /* Apply the spare selection algorithm using desired spare information
     * as input to select the best suitable spare from the available hot spare
     * pool information.
     */
    status = fbe_spare_lib_selection_select_suitable_spare_from_spare_pool(packet_p,
                                                                  &selected_spare_object_id,
                                                                  &desired_spare_drive_info,
                                                                  spare_drive_pool_p,
                                                                  &selected_spare_drive_info,
                                                                  vd_object_id);
    if ((status != FBE_STATUS_OK)||(selected_spare_object_id == FBE_OBJECT_ID_INVALID) )
    {
        /* if selected hot spare object-id is invalid then set the swap status. */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: find spare - fail-no suitable spare available,vd_id:0x%x\n", 
                               vd_object_id);
        fbe_memory_native_release(spare_drive_pool_p);

        fbe_job_service_drive_swap_request_set_status(js_swap_request_p, FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE);
        job_queue_element_p->status = FBE_STATUS_GENERIC_FAILURE;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE;

        /* Update the drive swap request with 'invalid object id'. */
        fbe_job_service_drive_swap_request_set_spare_object_id(js_swap_request_p, FBE_OBJECT_ID_INVALID);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_spare_lib_utils_complete_packet(packet_p, status);
        return status;
    }

    /* release the memory for the spare drive pool after applying spare selection. */
    fbe_memory_native_release(spare_drive_pool_p);

    /* Update the drive swap request with selected spare object-id. */
    fbe_job_service_drive_swap_request_set_spare_object_id(js_swap_request_p, selected_spare_object_id);

    /* Update  the desired spare drive info for permanent spare command*/
    if(js_swap_request_p->command_type == FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        fbe_spare_lib_utils_get_original_drive_info(packet_p,js_swap_request_p,
                                                    &desired_spare_drive_info);
    }

    job_queue_element_p->status = FBE_STATUS_OK;
    job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* Complete the job service packet with good status. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_spare_lib_utils_complete_packet(packet_p, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_selection_select_spare()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_drive_spare_info()
 ******************************************************************************
 * @brief
 * This function sends the get spare inforamtion usurpur command to virtual 
 * drive object which requires to swap-in spare and if it gets error in response
 * then it sends the same usurpur command to all the virtual drive object 
 * associated with RAID and get the spare inforamtion from it. 
 *
 * It derives the desired spare inforamtion from the collected information from
 * all the virtual drive object.
 *
 * @param vd_object_id - VD object-id which requires to swap-in.
 * @param out_desired_spare_info_p - Returns desired spare information
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/09/2009 - Created. Dhaval Patel
 *  03/15/2010   Modified Vishnu Sharma
 ******************************************************************************/
static fbe_status_t fbe_spare_lib_selection_get_desired_spare_drive_info(fbe_object_id_t vd_object_id,
                                                     fbe_spare_drive_info_t *out_desired_spare_info_p)
{

    fbe_status_t            status = FBE_STATUS_OK;
        
    /* Return with error if input parameters are NULL. */
    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (out_desired_spare_info_p);
   
    /* get desired spare info */
    status = fbe_spare_lib_selection_get_virtual_drive_spare_info(vd_object_id,out_desired_spare_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: Failed to get virtual drive: 0x%x spare info. status: 0x%x\n",
                               __FUNCTION__, vd_object_id, status);
    }

    /* If the `required capacity is 0 or invalid something is wrong.
     */
    if ((out_desired_spare_info_p->capacity_required == 0)               ||
        (out_desired_spare_info_p->capacity_required == FBE_LBA_INVALID)    )
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: Invalid required capacity: 0x%llx for virtual drive: 0x%x\n",
                               __FUNCTION__, (unsigned long long)out_desired_spare_info_p->capacity_required, vd_object_id);
        out_desired_spare_info_p->capacity_required = FBE_LBA_INVALID;
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_desired_spare_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_find_available_spare_pool()
 ******************************************************************************
 * @brief
 * This function sends the GET spare drive pool control packet to topology 
 * service and receives the list of PVD object ID which can be configured as 
 * spare.
 *
 * @param in_packet_p - The packet requesting this operation.
 * @param b_unused_as_spare - FBE_TRUE - (default) Any drive marked as `unconsumed'
 *          will be considered a suitable spare.
 *                            FBE_FALSE - (test case) Any drive marked as `test
 *          spare' will be considered a suitable spare.
 * @param in_spare_drive_pool_p - Spare drive pool pointer.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_spare_lib_selection_find_available_spare_pool(fbe_packet_t *in_packet_p,
                                                  fbe_bool_t b_unused_as_spare,
                                                  fbe_spare_drive_pool_t *in_spare_drive_pool_p)
{
    fbe_u32_t                                       index = 0;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t                                       number_of_selected_spare = 0;
    fbe_topology_control_get_spare_drive_pool_t     topology_spare_drive_pool;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* return with error if input parameters are NULL. */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL(in_spare_drive_pool_p);

    /* If `b_unused_as_spare' is True (default), then any `unconsumed' drive is
     * a valid spare.  If `b_unused_as_spare' is False (test case), then any
     * drive marked as `test spare' is a valid spare.
     */
    if (b_unused_as_spare == FBE_TRUE)
    {
        topology_spare_drive_pool.desired_spare_drive_type = FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_UNCONSUMED;
    }
    else
    {
        topology_spare_drive_pool.desired_spare_drive_type = FBE_TOPOLOGY_GET_SPARE_DRIVE_TYPE_TEST_SPARE;
    }
    /* initialize the topology spare drive pool request. */
    topology_spare_drive_pool.number_of_spare = 0;
    for (index = 0; index < FBE_MAX_SPARE_OBJECTS; index++)
    {
        topology_spare_drive_pool.spare_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    /* send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_TOPOLOGY_CONTROL_CODE_GET_SPARE_DRIVE_POOL,
                                                     &topology_spare_drive_pool,
                                                     sizeof(fbe_topology_control_get_spare_drive_pool_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:spare selection failed - get spare pool failed status:%d\n", 
                               __FUNCTION__, status);
        return status;
    }

    /* if no spare available then return good status but add the warning message. */
    if (topology_spare_drive_pool.number_of_spare == 0)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:spare selection failed - no spare available\n",
                               __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Determine the number of spare we get from the topology spare list,
     * we will copy the minimum based on the number of spare we have 
     * received from topology service and copy this information to the 
     * caller's spare pool pointer.
     */
    if (topology_spare_drive_pool.number_of_spare < FBE_MAX_SPARE_OBJECTS)
    {
        number_of_selected_spare = topology_spare_drive_pool.number_of_spare;
    }
    else
    {
        number_of_selected_spare = FBE_MAX_SPARE_OBJECTS;
    }

    /* Copy from topology drive spare pool list to the drive spare library 
     * context spare pool.
     */
    fbe_copy_memory (in_spare_drive_pool_p->spare_object_list, 
                     topology_spare_drive_pool.spare_object_list,
                     (sizeof(fbe_object_id_t) * number_of_selected_spare));

    return status;
}
/******************************************************************************
 * end fbe_spare_lib_selection_find_available_spare_pool()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_virtual_drive_spare_info()
 ******************************************************************************
 * @brief
 * This function sends the get drive info control packet to the passed in 
 * virtual drive object and in return it gets required drive information for 
 * the sparing.
 *
 * @param vd_object_id          - Virtual drive object id.
 * @param spare_drive_info_p    - Spare drive information.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_selection_get_virtual_drive_spare_info(fbe_object_id_t in_vd_object_id,
                                                     fbe_spare_drive_info_t *in_spare_drive_info_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL. */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (in_spare_drive_info_p);

    /* Verify the passed-in VD object-id. */
    if (in_vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s  Invalid vd: 0x%x \n", 
                               __FUNCTION__, in_vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the spare drive information with default values. */
    fbe_spare_initialize_spare_drive_info(in_spare_drive_info_p);

    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
                                                     in_spare_drive_info_p,
                                                     sizeof(fbe_spare_drive_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     in_vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:get drive spare info failed for VD: 0x%x.\n", 
                               __FUNCTION__, in_vd_object_id);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_virtual_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_hot_spare_info()
 ******************************************************************************
 * @brief
 * This function sends the get drive info control packet to the passed in hot
 * spare pvd.drive object and in return it gets required drive information for
 * the sparing.
 *
 * @param vd_object_id - Virtual drive object id.
 * @param in_spare_drive_info_p
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
fbe_status_t fbe_spare_lib_selection_get_hot_spare_info(fbe_object_id_t hs_pvd_object_id,
                                                        fbe_spare_drive_info_t *in_spare_drive_info_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL.
     */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (in_spare_drive_info_p);

    /* Verify the input hot spare pvd object id.
     */
    if (hs_pvd_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s  Invalid pvd: 0x%x \n", 
                               __FUNCTION__, hs_pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the spare drive information with default values. */
    fbe_spare_initialize_spare_drive_info(in_spare_drive_info_p);

    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
                                                     in_spare_drive_info_p,
                                                     sizeof(fbe_spare_drive_info_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     hs_pvd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:get spare drive info failed for pvd: 0x%x\n", 
                               __FUNCTION__, hs_pvd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_hot_spare_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_upstream_raid_object_list()
 ******************************************************************************
 * @brief
 * This function sends the get upstream object list usurper command to passed 
 * in virtual drive object and in response it receives the list of the attached
 * RAID object list details.
 *
 * @param in_packet_p - The packet requesting this operation.
 * @param in_vd_object_id - Virtual drive object id.
 * @param in_base_config_upstream_object_list_p - VD upstream object list.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/23/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_spare_lib_selection_get_upstream_raid_object_list(fbe_packet_t *in_packet_p,
                                                      fbe_object_id_t in_vd_object_id,
                                                      fbe_base_config_upstream_object_list_t *in_base_config_upstream_object_list_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       index = 0;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL. */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (in_base_config_upstream_object_list_p);

    /* Verify the inputs provided to find available spare pool request. */
    if (in_vd_object_id == FBE_OBJECT_ID_INVALID)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    /* Initialize the virtual drive upstream object list with default values. */
    in_base_config_upstream_object_list_p->number_of_upstream_objects  = 0;
    for (index = 0; index < FBE_MAX_UPSTREAM_OBJECTS; index++)
    {
        in_base_config_upstream_object_list_p->upstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }

    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                     in_base_config_upstream_object_list_p,
                                                     sizeof(fbe_base_config_upstream_object_list_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     in_vd_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:get upstream RAID object failed.\n", 
                               __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_upstream_raid_object_list()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_downstream_vd_object_list()
 ******************************************************************************
 * @brief
 * This function sends the get upstream object list usurper command
 * to passed in virtual drive object and in response it receives
 * the list of the attached RAID object list details.
 *
 * @param in_packet_p - The packet requesting this operation.
 * @param in_vd_object_id - Virtual drive object id.
 * @param in_base_config_upstream_object_list_p - VD upstream object list.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/23/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_spare_lib_selection_get_downstream_vd_object_list (fbe_packet_t *in_packet_p,
                                                   fbe_object_id_t in_raid_object_id,
                                                   fbe_base_config_downstream_object_list_t *in_raid_group_downstream_object_list_p)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_u32_t     index = 0;

    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL.
     */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (in_raid_group_downstream_object_list_p);

    /* Verify the inputs provided to find available spare pool request.
     */
    if (in_raid_object_id == FBE_OBJECT_ID_INVALID)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    /* Initialize the virtual drive upstream object list with default values.
     */
    in_raid_group_downstream_object_list_p->number_of_downstream_objects = 0;
    for (index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        in_raid_group_downstream_object_list_p->downstream_object_list [index] = FBE_OBJECT_ID_INVALID;
    }

    /* Send the control packet and wait for its completion. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                     in_raid_group_downstream_object_list_p,
                                                     sizeof(fbe_base_config_downstream_object_list_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     in_raid_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s:get downstream VD object failed.\n", 
                               __FUNCTION__);
    }
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_downstream_vd_object_list()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_hard_rule_drive_must_be_healthy()
 *****************************************************************************
 *
 * @brief   Hard Rule: Drive must be in a healthy state.  This rule validates:
 *              o Drive is accessible on both SPs.
 *              o EOL is not set on either SP downstream path
 *
 * @param   proposed_spare_info_p - Proposed spare drive information.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is healthy.
 *              Other - Either error occurred or drive is not suitable
 *              replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_hard_rule_drive_must_be_healthy(fbe_spare_selection_info_t *proposed_spare_info_p)
{
    fbe_status_t    status = FBE_STATUS_DEAD;
    fbe_block_transport_control_get_edge_info_t block_edge_info;
    fbe_base_object_mgmt_get_lifecycle_state_t  base_object_mgmt_get_lifecycle_state;

    /*! @todo Validate that the drive is accessible and healthy on peer SP.
     *              
     */
    //fbe_spare_lib_is_peer_healthy();

    /* If we find that hot spare has end of life bit set then skip this spare 
     * for the selection. 
     */
    status = fbe_spare_lib_utils_get_block_edge_info(proposed_spare_info_p->spare_object_id, 0, &block_edge_info);
    if (status != FBE_STATUS_OK) 
    {
        /* If we can't get the downstream edge trace an informational message.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: is healthly pvd obj: 0x%x failed get block edge. status: 0x%x\n", 
                               proposed_spare_info_p->spare_object_id, status);

        return status;
    }

    /* Get selected spare PVD lifecycle state. We will allow sparing only if it is in Ready or Hibernate state. */
    status = fbe_spare_lib_utils_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &base_object_mgmt_get_lifecycle_state,
                                                     sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     proposed_spare_info_p->spare_object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
        
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: is healthly pvd obj: 0x%x checking destination PVD state failed - status:%d\n", 
                               proposed_spare_info_p->spare_object_id, status);

        return status;
    }

    /* If the lifecycle state is not Ready/Hibernate or the edge state is not
     * Enabled/Slumber or EOL is set, the drive is not health.
     */
    if ((proposed_spare_info_p->spare_drive_info.b_is_end_of_life == FBE_TRUE)                       ||
        ((block_edge_info.base_edge_info.path_state != FBE_PATH_STATE_ENABLED) &&
         (block_edge_info.base_edge_info.path_state != FBE_PATH_STATE_SLUMBER)    )                  ||
        ((base_object_mgmt_get_lifecycle_state.lifecycle_state != FBE_LIFECYCLE_STATE_READY)     &&
         (base_object_mgmt_get_lifecycle_state.lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)    )   )
    {
        /* Trace a debug message.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: is healthly pvd obj: 0x%x lifecycle: %d EOL: %d path state: %d path attr: 0x%x \n", 
                               proposed_spare_info_p->spare_object_id,
                               base_object_mgmt_get_lifecycle_state.lifecycle_state,
                               proposed_spare_info_p->spare_drive_info.b_is_end_of_life,
                               block_edge_info.base_edge_info.path_state,
                               block_edge_info.base_edge_info.path_attr);
        return FBE_STATUS_DEAD;
    }

    /* Validate that the drive is not in SLF.
     */
    if (proposed_spare_info_p->spare_drive_info.b_is_slf == FBE_TRUE)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: is in slf pvd obj: 0x%x \n", 
                               proposed_spare_info_p->spare_object_id);
        return FBE_STATUS_DEAD;
    }

    /*! @note Validate that the drive is accessible and healthy on peer SP.
     *              
     */
    //fbe_spare_lib_is_peer_healthy();

    /* Otherwise the drive is healthy.
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_spare_lib_selection_hard_rule_drive_must_be_healthy()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_hard_rule_check_drive_offset()
 *****************************************************************************
 *
 * @brief   Hard Rule: Replacement drive must have an offset that is less than
 *              o Current requirement is that the raid group offset cannot
 *                change.  But if the replacement drive has an offset that is
 *                less than the offset required, it can be increased.
 *              o Since the offset of the replacement drive cannot be decreased
 *                only allow the drive replacement if he replacement drive
 *                offset is less than or equal to the raid group offset.
 *
 * @param   proposed_spare_info_p - Proposed spare drive information.
 * @param   exported_offset - The virtual drive exported extent starting offset.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive has the same
 *              exportable extent offset as the virtual drive requesting the
 *              spare.
 *              Other - Either error occurred or drive is not suitable
 *              replacement.
 *
 * @author
 *  05/23/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_hard_rule_check_drive_offset(fbe_spare_selection_info_t *proposed_spare_info_p,
                                                                         fbe_lba_t exported_offset)
{
    /* Now compare the offset with the proposed replacement.
     */
    if (proposed_spare_info_p->spare_drive_info.exported_offset > exported_offset)
    {
        /* Trace a debug message.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: check offset pvd obj: 0x%x offset: 0x%llx vd offset: 0x%llx\n", 
                               proposed_spare_info_p->spare_object_id,
                               proposed_spare_info_p->spare_drive_info.exported_offset,
                               exported_offset);
        return FBE_STATUS_DEAD;
    }

    /* Otherwise the drive is healthy.
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_spare_lib_selection_hard_rule_check_drive_offset()
 ************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_hard_rule_drive_cannot_be_reserved()
 *****************************************************************************
 *
 * @brief   Hard Rule: Replacement drive cannot be flagged as reserved:
 *              o The pool id is not `invalid'.
 *
 * @param   proposed_spare_info_p - Proposed spare drive information.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is not reserved.
 *              Other - Either error occurred or drive is not suitable
 *              replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_hard_rule_drive_cannot_be_reserved(fbe_spare_selection_info_t *proposed_spare_info_p)
{
    fbe_status_t    status = FBE_STATUS_INSUFFICIENT_RESOURCES;

    /* Verify that pool id is invalid.
     */
    if (proposed_spare_info_p->spare_drive_info.pool_id != FBE_POOL_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: is reserved pvd obj: 0x%x is reserved pool id: %d\n", 
                               proposed_spare_info_p->spare_object_id,
                               proposed_spare_info_p->spare_drive_info.pool_id);
        return status;
    }

    /* Otherwise return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_spare_lib_selection_hard_rule_drive_cannot_be_reserved()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_hard_rule_is_same_performance_tier()
 *****************************************************************************
 *
 * @brief   Hard Rule: Replacement drive must be in the same performance tier.
 *              o  Replacement drive must be in the same performance tier.
 *
 * @param   proposed_spare_info_p - Proposed spare drive information.
 * @param   original_drive_performance_tier - Origianl drive performance tier.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is in the same
 *              performance tier.
 *              Other - Either error occurred or drive is not suitable
 *              replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_hard_rule_is_same_performance_tier(fbe_spare_selection_info_t *proposed_spare_info_p,
                                                                               fbe_performance_tier_number_t original_drive_performance_tier)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                      b_is_same_tier = FBE_FALSE;
    fbe_performance_tier_number_t   proposed_performance_tier = FBE_PERFORMANCE_TIER_INVALID;

    /* Check if drive is in the same performance tier.
     */
    fbe_spare_lib_selection_get_drive_performance_tier_number(proposed_spare_info_p->spare_drive_info.drive_type, 
                                                              &proposed_performance_tier);
    status = fbe_spare_lib_utils_is_drive_type_valid_for_performance_tier(original_drive_performance_tier,
                                                                          proposed_spare_info_p->spare_drive_info.drive_type,
                                                                          &b_is_same_tier);
    if ((status != FBE_STATUS_OK)     ||
        (b_is_same_tier == FBE_FALSE)    )
    {
        /* Trace a debug message and return.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: same tier pvd obj: 0x%x drive type: %d tier: %d different than orig tier: %d\n", 
                               proposed_spare_info_p->spare_object_id,
                               proposed_spare_info_p->spare_drive_info.drive_type,
                               proposed_performance_tier,
                               original_drive_performance_tier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Otherwise return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_spare_lib_selection_hard_rule_is_same_performance_tier()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_hard_rule_is_same_block_size()
 *****************************************************************************
 *
 * @brief   Hard Rule: Replacement drive must be in the same performance tier.
 *              o  Replacement drive must be in the same performance tier.
 *
 * @param   proposed_spare_info_p - Proposed spare drive information.
 * @param   original_drive_physical_block_size - Original drive physical block size
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive has th same
 *              physical block size.
 *              Other - Either error occurred or drive is not suitable
 *              replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_hard_rule_is_same_block_size(fbe_spare_selection_info_t *proposed_spare_info_p,
                                                                         fbe_provision_drive_configured_physical_block_size_t original_drive_physical_block_size)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* Check if drive is in the same bloc size.
     */
    if (proposed_spare_info_p->spare_drive_info.configured_block_size != original_drive_physical_block_size)
    {
        /* Trace a debug message and return.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: same block size pvd obj: 0x%x block size: %d orig block size: %d\n", 
                               proposed_spare_info_p->spare_object_id,
                               proposed_spare_info_p->spare_drive_info.configured_block_size,
                               original_drive_physical_block_size);
        return status;
    }

    /* Otherwise return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_spare_lib_selection_hard_rule_is_same_block_size()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_hard_rule_is_sufficient_capacity()
 *****************************************************************************
 *
 * @brief   Hard Rule: Replacement drive must have sufficient capacity.
 *              o  Replacement drive must have sufficient capacity
 *
 * @param   proposed_spare_info_p - Proposed spare drive information.
 * @param   capacity_required - Exportable capacity that the replacement drive must have
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive has
 *              sufficient capacity.
 *              physical block size.
 *              Other - Either error occurred or drive is not suitable
 *              replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_hard_rule_is_sufficient_capacity(fbe_spare_selection_info_t *proposed_spare_info_p,
                                                                             fbe_lba_t capacity_required)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t      b_is_sufficient_capacity = FBE_FALSE;

    /* Check if drive has sufficient capacity.
     */
    status = fbe_spare_lib_utils_check_capacity(capacity_required,
                                                proposed_spare_info_p->spare_drive_info.configured_capacity,
                                                &b_is_sufficient_capacity);
    if ((status != FBE_STATUS_OK)               ||
        (b_is_sufficient_capacity == FBE_FALSE)    )
    {
        /* Trace a debug message and return.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: sufficient capacity pvd obj: 0x%x capacity: 0x%llx required capacity: 0x%llx\n", 
                               proposed_spare_info_p->spare_object_id,
                               proposed_spare_info_p->spare_drive_info.configured_capacity,
                               capacity_required);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Otherwise return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_spare_lib_selection_hard_rule_is_sufficient_capacity()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_apply_hard_rules()
 *****************************************************************************
 *
 * @brief   Apply the spare selection `hard' rules.  The term `hard' implies 
 *          that if the drive proposed to replace the original doesn't pass
 *          any of these rules is is not a validate candidate.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive passed all
 *              the `hard' sparing rules.
 *
 * @note    See `MCR Sparing Functional Specificaiton' for an up-to-date list
 *          of the current `hard' spare replacement rules.
 *
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_apply_hard_rules(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                             fbe_spare_selection_info_t *proposed_spare_info_p)
{
    fbe_status_t                    status = FBE_STATUS_FAILED;
    fbe_spare_drive_info_t         *original_drive_info_p = &original_spare_drive_info_p->spare_drive_info;
    fbe_performance_tier_number_t   original_drive_performance_tier = FBE_PERFORMANCE_TIER_INVALID;

    /*! @note 
     *  Spare Selection Hard Rule: 1: Replacement drive must be in a healthly state.
     *
     *  Only apply this rule when there is PVD connected to the sparing drive.
     *
     *  There maybe no PVD connecting to the sparing drive, which means we are determining
     *  through sparing policy whether the sparing drive should connect to the PVD having
     *  the same location. In this case, the PVD is definitely not healthy and before applying
     *  sparing policy, we have set the object id in proposed_spare_info the same with
     *  the object id in original_spare_drive_info.
     *              
     */
    if(original_spare_drive_info_p->spare_object_id != proposed_spare_info_p->spare_object_id)
    {
        status = fbe_spare_lib_selection_hard_rule_drive_must_be_healthy(proposed_spare_info_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    /*! @note 
     *  Spare Selection Hard Rule: 2: Replacement drive must have an acceptable offset.
     *              
     */
    status = fbe_spare_lib_selection_hard_rule_check_drive_offset(proposed_spare_info_p, original_drive_info_p->exported_offset);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Hard Rule: 3: Replacement drive must not be reserved.
     *              
     */
    status = fbe_spare_lib_selection_hard_rule_drive_cannot_be_reserved(proposed_spare_info_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Hard Rule: 4: Replacement drive must be in the same performance tier.
     *              
     */
    fbe_spare_lib_selection_get_drive_performance_tier_number(original_drive_info_p->drive_type, &original_drive_performance_tier);
    status = fbe_spare_lib_selection_hard_rule_is_same_performance_tier(proposed_spare_info_p, original_drive_performance_tier);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Hard Rule: 5: Replacement drive must have the same block size.
     *  
     *  With the introduction of 4k drives we must allow different block sizes to swap in.
     */
#if 0
    status = fbe_spare_lib_selection_hard_rule_is_same_block_size(proposed_spare_info_p, original_drive_info_p->configured_block_size);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
#endif
    /*! @note 
     *  Spare Selection Hard Rule: 6: Replacement drive must have sufficient capacity.
     *              
     */
    status = fbe_spare_lib_selection_hard_rule_is_sufficient_capacity(proposed_spare_info_p, original_drive_info_p->capacity_required);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Passed all the hard rules.  Return success.
     */
    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_spare_lib_selection_apply_hard_rules()
 ************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rules_update_selected_drive()
 *****************************************************************************
 *
 * @brief   Update the selected replacement drive with the proposed drive.
 *
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 *
 * @return  status - FBE_STATUS_OK 
 *
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rules_update_selected_drive(fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                             fbe_spare_selection_info_t *selected_hot_spare_info_p)
{
    fbe_object_id_t saved_proposed_object_id;

    /*! @note Only purpose of `get object id' is to track where we update it.
     */
    fbe_spare_lib_hot_spare_info_get_object_id(proposed_hot_spare_info_p, &saved_proposed_object_id);
    fbe_spare_lib_hot_spare_info_set_object_id(selected_hot_spare_info_p, saved_proposed_object_id);
    fbe_spare_selection_set_spare_selection_priority(selected_hot_spare_info_p, proposed_hot_spare_info_p->spare_selection_bitmask);

    /* Replace the current replacement drive information with the proposed.
     */
    fbe_copy_memory (&(selected_hot_spare_info_p->spare_drive_info),
                     &(proposed_hot_spare_info_p->spare_drive_info), 
                     sizeof(fbe_spare_drive_info_t));

    /* Always succeeds.
     */
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_spare_lib_selection_soft_rules_update_selected_drive()
 ****************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rule_choose_matching_block_size()
 *****************************************************************************
 *
 * @brief   Soft Rule: If a matching block size is found choose it.
 *              o  A drive that is of the same block size is a better replacement
 *                 than a non-matching block size.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   selection_priority - The priority of this rule
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is a better
 *              match and has been selected as the best replacement.
 *
 * @author
 *  06/06/2014  Ashok Tamilarasan  - Created based on 
 *                      fbe_spare_lib_selection_soft_rule_choose_matching_drive_type
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rule_choose_matching_block_size(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                                                 fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                                 fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                                                 fbe_spare_selection_priority_t selection_priority,
                                                                                 fbe_bool_t b_determine_selection_priority)

{
    /* Verify the block size of current hot spare, if block size of
     * selected hot spare is not the same as desired hot spare block size and
     * current hot spare block size is same as desired hot spare block size
     * then update the selected hot spare drive as current hot spare drive 
     * info.
     */
    if (proposed_hot_spare_info_p->spare_drive_info.configured_block_size == original_spare_drive_info_p->spare_drive_info.configured_block_size)
    {
        /* If we are determining the selection priority simply set it.
         */
        if (b_determine_selection_priority == FBE_TRUE)
        {
            /* Simply add the rule priority and return `continue'.
             */
            fbe_spare_selection_add_spare_selection_priority(proposed_hot_spare_info_p, selection_priority);
            return FBE_STATUS_CONTINUE;
        }

        /* If the proposed drive is a better match then replace the currently
         * selected drive with the proposed drive.
         */
        if (fbe_spare_selection_is_proposed_drive_higher_priority(proposed_hot_spare_info_p, selected_hot_spare_info_p))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: match bl size select pvd:0x%x BL size:%d over current:0x%x BL Size:%d\n", 
                               proposed_hot_spare_info_p->spare_object_id,
                               proposed_hot_spare_info_p->spare_drive_info.configured_block_size,
                               selected_hot_spare_info_p->spare_object_id,
                               selected_hot_spare_info_p->spare_drive_info.configured_block_size);
            fbe_spare_lib_selection_soft_rules_update_selected_drive(proposed_hot_spare_info_p, selected_hot_spare_info_p);
            return FBE_STATUS_OK;
        }
    }

    /* Otherwise the proposed drive is not a better replacement.
     */
    return FBE_STATUS_CONTINUE;

}
/********************************************************************
 * end fbe_spare_lib_selection_soft_rule_choose_matching_block_size()
 ********************************************************************/


/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rule_choose_matching_drive_type()
 *****************************************************************************
 *
 * @brief   Soft Rule: If a matching drive type is found choose it.
 *              o  A drive that is of the same drive type is a better replacement
 *                 than a non-matching drive type.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   selection_priority - The priority of this rule
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is a better
 *              match and has been selected as the best replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx  - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rule_choose_matching_drive_type(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                                                 fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                                 fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                                                 fbe_spare_selection_priority_t selection_priority,
                                                                                 fbe_bool_t b_determine_selection_priority)

{
    /* Verify the drive type of current hot spare, if drive type of
     * selected hot spare is not the same as desired hot spare drive type and
     * current hot spare drive type is same as desired hot spare drive type
     * then update the selected hot spare drive as current hot spare drive 
     * info.
     */
    if (proposed_hot_spare_info_p->spare_drive_info.drive_type == original_spare_drive_info_p->spare_drive_info.drive_type)
    {
        /* If we are determining the selection priority simply set it.
         */
        if (b_determine_selection_priority == FBE_TRUE)
        {
            /* Simply add the rule priority and return `continue'.
             */
            fbe_spare_selection_add_spare_selection_priority(proposed_hot_spare_info_p, selection_priority);
            return FBE_STATUS_CONTINUE;
        }

        /* If the proposed drive is a better match then replace the currently
         * selected drive with the proposed drive.
         */
        if (fbe_spare_selection_is_proposed_drive_higher_priority(proposed_hot_spare_info_p, selected_hot_spare_info_p))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: match drive types select pvd obj: 0x%x type: %d over current: 0x%x type: %d\n", 
                               proposed_hot_spare_info_p->spare_object_id,
                               proposed_hot_spare_info_p->spare_drive_info.drive_type,
                               selected_hot_spare_info_p->spare_object_id,
                               selected_hot_spare_info_p->spare_drive_info.drive_type);
            fbe_spare_lib_selection_soft_rules_update_selected_drive(proposed_hot_spare_info_p, selected_hot_spare_info_p);
            return FBE_STATUS_OK;
        }
    }

    /* Otherwise the proposed drive is not a better replacement.
     */
    return FBE_STATUS_CONTINUE;

}
/********************************************************************
 * end fbe_spare_lib_selection_soft_rule_choose_matching_drive_type()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rule_choose_matching_port()
 *****************************************************************************
 *
 * @brief   Soft Rule: If a drive that is on the same port; choose it.
 *              o  A drive that is on the same port as the original, choose it
 *                 over the currently selected replacement drive.
 *
 * @param   selection_priority - The priority of this rule
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is a better
 *              match and has been selected as the best replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx  - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rule_choose_matching_port(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                                           fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                           fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                                           fbe_spare_selection_priority_t selection_priority,
                                                                           fbe_bool_t b_determine_selection_priority)
{
    /* Verify the io port number of current hot spare, if io port number of
     * selected hot spare is not the same as desired hot spare io port number and
     * current hot spare io port number is same has desired hot spare io port number
     * then check for current spare drive type is same as selecteced spare drive type.
     * If both conditions are true then update the selected hot spare with current hot spare.
     */
    if (proposed_hot_spare_info_p->spare_drive_info.port_number == original_spare_drive_info_p->spare_drive_info.port_number)
    {
        /* If we are determining the selection priority simply set it.
         */
        if (b_determine_selection_priority == FBE_TRUE)
        {
            /* Simply add the rule priority and return `continue'.
             */
            fbe_spare_selection_add_spare_selection_priority(proposed_hot_spare_info_p, selection_priority);
            return FBE_STATUS_CONTINUE;
        }

        /* If the proposed drive is a better match then replace the currently
         * selected drive with the proposed drive.
         */
        if (fbe_spare_selection_is_proposed_drive_higher_priority(proposed_hot_spare_info_p, selected_hot_spare_info_p))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: match backend port select pvd obj: 0x%x port: %d over current: 0x%x port: %d\n", 
                               proposed_hot_spare_info_p->spare_object_id,
                               proposed_hot_spare_info_p->spare_drive_info.port_number,
                               selected_hot_spare_info_p->spare_object_id,
                               selected_hot_spare_info_p->spare_drive_info.port_number);
            fbe_spare_lib_selection_soft_rules_update_selected_drive(proposed_hot_spare_info_p, selected_hot_spare_info_p);
            return FBE_STATUS_OK;
        }
    }

    /* Otherwise the proposed drive is not a better replacement.
     */
    return FBE_STATUS_CONTINUE;

}
/********************************************************************
 * end fbe_spare_lib_selection_soft_rule_choose_matching_port()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rule_choose_best_offset()
 *****************************************************************************
 *
 * @brief   Soft Rule: If the failed drive was in one of the system drive slots:
 *              o  If the failed drive was in a system drive slot then give
 *                 preference to sapre drives that are in the same BBU enclosure.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   selection_priority - The priority of this rule
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is a better
 *              match and has been selected as the best replacement.
 *
 * @author
 *  06/05/2012  Ron Proulx  - Created.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rule_choose_best_offset(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                                      fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                      fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                                      fbe_spare_selection_priority_t selection_priority,
                                                                      fbe_bool_t b_determine_selection_priority)
{
    fbe_lba_t       selected_difference_in_offset;
    fbe_lba_t       proposed_difference_in_offset;

    /* Now compare the offset with the proposed replacement.  It is assumed that
     * the proposed offset is less than or 
     */
    if (selected_hot_spare_info_p->spare_drive_info.exported_offset > original_spare_drive_info_p->spare_drive_info.exported_offset)
    {
        selected_difference_in_offset = selected_hot_spare_info_p->spare_drive_info.exported_offset - 
                                            original_spare_drive_info_p->spare_drive_info.exported_offset;
    }
    else
    {
        selected_difference_in_offset = original_spare_drive_info_p->spare_drive_info.exported_offset -
                                            selected_hot_spare_info_p->spare_drive_info.exported_offset;
    }
    if (proposed_hot_spare_info_p->spare_drive_info.exported_offset > original_spare_drive_info_p->spare_drive_info.exported_offset)
    {
        proposed_difference_in_offset = proposed_hot_spare_info_p->spare_drive_info.exported_offset - 
                                            original_spare_drive_info_p->spare_drive_info.exported_offset;
    }
    else
    {
        proposed_difference_in_offset = original_spare_drive_info_p->spare_drive_info.exported_offset -
                                            proposed_hot_spare_info_p->spare_drive_info.exported_offset;
    }
    if (proposed_difference_in_offset <= selected_difference_in_offset)
    {
        /* If we are determining the selection priority simply set it.
         */
        if (b_determine_selection_priority == FBE_TRUE)
        {
            /* Add the `rule' priority
             */
            fbe_spare_selection_add_spare_selection_priority(proposed_hot_spare_info_p, selection_priority);

            /* If this is a better offset, then remove the priority from the 
             * currently selected spare.
             */
            if (proposed_difference_in_offset < selected_difference_in_offset)
            {
                fbe_spare_selection_remove_spare_selection_priority(selected_hot_spare_info_p, selection_priority);
            }
            return FBE_STATUS_CONTINUE;
        }

        /* If the proposed drive is a better match then replace the currently
         * selected drive with the proposed drive.
         */
        if (fbe_spare_selection_is_proposed_drive_higher_priority(proposed_hot_spare_info_p, selected_hot_spare_info_p))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare_selection: select better offset pvd obj: 0x%x offset: 0x%llx over current: 0x%x offset: 0x%llx\n", 
                                   proposed_hot_spare_info_p->spare_object_id,
                                   proposed_hot_spare_info_p->spare_drive_info.exported_offset,
                                   selected_hot_spare_info_p->spare_object_id,
                                   selected_hot_spare_info_p->spare_drive_info.exported_offset);
            fbe_spare_lib_selection_soft_rules_update_selected_drive(proposed_hot_spare_info_p, selected_hot_spare_info_p);
            return FBE_STATUS_OK;
        }
    }

    /* Otherwise the proposed drive is not a better replacement.
     */
    return FBE_STATUS_CONTINUE;

}
/***********************************************************************
 * end fbe_spare_lib_selection_soft_rule_choose_best_offset()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rule_choose_best_fit()
 *****************************************************************************
 *
 * @brief   Soft Rule: If a drive has a better fit (i.e. less wasted capacity)
 *          choose it.
 *              o  A drive that has less wasted capaciyt is a better choice since
 *                 a larger might be used for a large raid group etc.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   selection_priority - The priority of this rule
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is a better
 *              match and has been selected as the best replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx  - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rule_choose_best_fit(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                                      fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                      fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                                      fbe_spare_selection_priority_t selection_priority,
                                                                      fbe_bool_t b_determine_selection_priority)
{
    /* Verify whether current spare capacity is less than
     * selected hot spare capacity and current spare port number is
     * same as current spare port number current spare drive type is
     * same as selecteced spare drive type.
     * If all conditions are true then update the selected hot spare with current hot spare.
     */
    if (proposed_hot_spare_info_p->spare_drive_info.configured_capacity <= 
                        selected_hot_spare_info_p->spare_drive_info.configured_capacity)   
    {
        /* If we are determining the selection priority simply set it.
         */
        if (b_determine_selection_priority == FBE_TRUE)
        {
            /* Add the `rule' priority.
             */
            fbe_spare_selection_add_spare_selection_priority(proposed_hot_spare_info_p, selection_priority);

            /* If this is a better fit, then remove the priority from the 
             * currently selected spare.
             */
            if (proposed_hot_spare_info_p->spare_drive_info.configured_capacity <
                        selected_hot_spare_info_p->spare_drive_info.configured_capacity)        
            {
                fbe_spare_selection_remove_spare_selection_priority(selected_hot_spare_info_p, selection_priority);
            }
            return FBE_STATUS_CONTINUE;
        }

        /* If the proposed drive is a better match then replace the currently
         * selected drive with the proposed drive.
         */
        if (fbe_spare_selection_is_proposed_drive_higher_priority(proposed_hot_spare_info_p, selected_hot_spare_info_p))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "spare_selection: match best fit select pvd obj: 0x%x capacity: 0x%llx over current: 0x%x capacity: 0x%llx\n", 
                                   proposed_hot_spare_info_p->spare_object_id,
                                   proposed_hot_spare_info_p->spare_drive_info.configured_capacity,
                                   selected_hot_spare_info_p->spare_object_id,
                                   selected_hot_spare_info_p->spare_drive_info.configured_capacity);
            fbe_spare_lib_selection_soft_rules_update_selected_drive(proposed_hot_spare_info_p, selected_hot_spare_info_p);
            return FBE_STATUS_OK;
        }
    }

    /* Otherwise the proposed drive is not a better replacement.
     */
    return FBE_STATUS_CONTINUE;

}
/********************************************************************
 * end fbe_spare_lib_selection_soft_rule_choose_best_fit()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_soft_rule_choose_matching_enclosure()
 *****************************************************************************
 *
 * @brief   Soft Rule: If a drive that is on the same enclosure; choose it.
 *              o  A drive that is on the same enclosure as the original, choose it
 *                 over the currently selected replacement drive.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   selection_priority - The priority of this rule
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive is a better
 *              match and has been selected as the best replacement.
 *
 * @author
 *  05/16/2012  Ron Proulx  - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_soft_rule_choose_matching_enclosure(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                                                fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                                                fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                                                fbe_spare_selection_priority_t selection_priority,
                                                                                fbe_bool_t b_determine_selection_priority)
{
    /* Verify the encl number of current hot spare, if encl number of
     * selected hot spare is not the same as desired hot spare encl number and
     * current hot spare encl number is same as desired hot spare encl number
     * then check for conditions like current spare port number is same as selected hot
     * spare port number, current spare capacity is less than selected
     * hot spare capacity and current spare drive type is same as selecteced spare drive type.
     * If all conditions are true then update the selected hot spare with current hot spare.
     */
    if (proposed_hot_spare_info_p->spare_drive_info.enclosure_number == original_spare_drive_info_p->spare_drive_info.enclosure_number)
    {
        /* If we are determining the selection priority simply set it.
         */
        if (b_determine_selection_priority == FBE_TRUE)
        {
            /* Simply add the rule priority and return `continue'.
             */
            fbe_spare_selection_add_spare_selection_priority(proposed_hot_spare_info_p, selection_priority);
            return FBE_STATUS_CONTINUE;
        }

        /* If the proposed drive is a better match then replace the currently
         * selected drive with the proposed drive.
         */
        if (fbe_spare_selection_is_proposed_drive_higher_priority(proposed_hot_spare_info_p, selected_hot_spare_info_p))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: match enclosure select pvd obj: 0x%x encl: %d over current: 0x%x encl: %d\n", 
                               proposed_hot_spare_info_p->spare_object_id,
                               proposed_hot_spare_info_p->spare_drive_info.enclosure_number,
                               selected_hot_spare_info_p->spare_object_id,
                               selected_hot_spare_info_p->spare_drive_info.enclosure_number);
            fbe_spare_lib_selection_soft_rules_update_selected_drive(proposed_hot_spare_info_p, selected_hot_spare_info_p);
            return FBE_STATUS_OK;
        }
    }

    /* Otherwise the proposed drive is not a better replacement.
     */
    return FBE_STATUS_CONTINUE;

}
/********************************************************************
 * end fbe_spare_lib_selection_soft_rule_choose_matching_enclosure()
 ********************************************************************/

/*!****************************************************************************
 * fbe_spare_selection_decrement_selection_priority()
 ******************************************************************************
 * @brief 
 *   Decrement the spare selection priority.
 *
 * @param spare_selection_priority_p - Pointer to priority to decrement
 *
 * @return  status - If cannot decrment return FBE_STATUS_GENERIC_FAILURE
 *
 ******************************************************************************/
static fbe_status_t fbe_spare_selection_decrement_selection_priority(fbe_spare_selection_priority_t *spare_selection_priority_p)
{
    /* Validate value before decrementing.
     */
    if (*spare_selection_priority_p <= FBE_SPARE_SELECTION_PRIORITY_ZERO)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Attempt to decrement: %d below lowest value: %d\n", 
                               __FUNCTION__, *spare_selection_priority_p, FBE_SPARE_SELECTION_PRIORITY_ZERO);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *spare_selection_priority_p = *spare_selection_priority_p - 1;
    return FBE_STATUS_OK;
}
/********************************************************************
 * end fbe_spare_selection_decrement_selection_priority()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_spare_lib_selection_apply_soft_rules()
 *****************************************************************************
 *
 * @brief   Apply the spare selection `soft' rules.  The term `soft' implies 
 *          that even of the proposed replacement drive doesn't pass hte rules
 *          we have located a replacement drive.
 *
 * @param   original_spare_drive_info_p - Original drive information.
 * @param   proposed_spare_info_p - Proposed replacement drive information.
 * @param   selected_hot_spare_info_p - Currently selected replacement drive.
 * @param   b_determine_selection_priority - FBE_TRUE - Simply determine the
 *              selection priority of the proposed spare.
 *
 * @return  status - FBE_STATUS_OK - The proposed replacement drive passed all
 *              the `hard' sparing rules.
 *
 * @note    See `MCR Sparing Functional Specificaiton' for an up-to-date list
 *          of the current `hard' spare replacement rules.
 *
 *
 * @author
 *  05/16/2012  Ron Proulx  - Updated.
 *****************************************************************************/
static fbe_status_t fbe_spare_lib_selection_apply_soft_rules(fbe_spare_selection_info_t *original_spare_drive_info_p,
                                                             fbe_spare_selection_info_t *proposed_hot_spare_info_p,
                                                             fbe_spare_selection_info_t *selected_hot_spare_info_p,
                                                             fbe_bool_t b_determine_selection_priority)
{
    fbe_status_t                    status = FBE_STATUS_CONTINUE;
    /*! @note If the number of rules below changes change the value of `FBE_SPARE_SELECTION_PRIORITY_NUM_OF_RULES'.*/
    fbe_spare_selection_priority_t  selection_priority = FBE_SPARE_SELECTION_PRIORITY_NUM_OF_RULES;

    /* If we are determining the proposed spare selection priority.
     */
    if (b_determine_selection_priority == FBE_TRUE)
    {
        /* Validate that the currently selected priority is valid.
         */
        if ((selected_hot_spare_info_p->spare_selection_bitmask & ~FBE_SPARE_SELECTION_PRIORITY_VALID_MASK) != 0)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s Invalid selection priority mask: 0x%x valid mask: 0x%x\n", 
                                   __FUNCTION__, selected_hot_spare_info_p->spare_selection_bitmask, 
                                   FBE_SPARE_SELECTION_PRIORITY_VALID_MASK); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Initialize the selection priority of the proposed drive.
         */
        fbe_spare_selection_set_spare_selection_priority(proposed_hot_spare_info_p, 0);
    }

    /*! @note If the soft rule returns `OK' (i.e. success), it means that the
     *        the proposed replacement drive has now been selected as the 
     *        best replacement drive.
     */

     /*! @note 
     *  Spare Selection Soft Rule: 1: Choose the matching block size
     *              
     */
    status = fbe_spare_lib_selection_soft_rule_choose_matching_block_size(original_spare_drive_info_p,
                                                                          proposed_hot_spare_info_p,
                                                                          selected_hot_spare_info_p,
                                                                          selection_priority,
                                                                          b_determine_selection_priority);
    if (status == FBE_STATUS_OK)        
    {
        return status;
    }
    status = fbe_spare_selection_decrement_selection_priority(&selection_priority);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Soft Rule: 2: Choose the matching drive type
     *              
     */
    status = fbe_spare_lib_selection_soft_rule_choose_matching_drive_type(original_spare_drive_info_p,
                                                                          proposed_hot_spare_info_p,
                                                                          selected_hot_spare_info_p,
                                                                          selection_priority,
                                                                          b_determine_selection_priority);
    if (status == FBE_STATUS_OK)        
    {
        return status;
    }
    status = fbe_spare_selection_decrement_selection_priority(&selection_priority);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Soft Rule: 3: Choose the closest offset
     *              
     */
    status = fbe_spare_lib_selection_soft_rule_choose_best_offset(original_spare_drive_info_p,
                                                                  proposed_hot_spare_info_p,
                                                                  selected_hot_spare_info_p,
                                                                  selection_priority,
                                                                  b_determine_selection_priority);
    if (status == FBE_STATUS_OK)        
    {
        return status;
    }
    status = fbe_spare_selection_decrement_selection_priority(&selection_priority);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Soft Rule: 4: Choose the matching backend port
     *              
     */
    status = fbe_spare_lib_selection_soft_rule_choose_matching_port(original_spare_drive_info_p,    
                                                                    proposed_hot_spare_info_p,      
                                                                    selected_hot_spare_info_p,      
                                                                    selection_priority,             
                                                                    b_determine_selection_priority);
    if (status == FBE_STATUS_OK)        
    {
        return status;
    }
    status = fbe_spare_selection_decrement_selection_priority(&selection_priority);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }


    /*! @note 
     *  Spare Selection Soft Rule: 5: Choose the drive with the best fit
     *              
     */
    status = fbe_spare_lib_selection_soft_rule_choose_best_fit(original_spare_drive_info_p,
                                                               proposed_hot_spare_info_p,
                                                               selected_hot_spare_info_p,
                                                               selection_priority,
                                                               b_determine_selection_priority);
    if (status == FBE_STATUS_OK)        
    {
        return status;
    }
    status = fbe_spare_selection_decrement_selection_priority(&selection_priority);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note 
     *  Spare Selection Soft Rule: 6: Choose the matching enclosure
     *              
     */
    status = fbe_spare_lib_selection_soft_rule_choose_matching_enclosure(original_spare_drive_info_p,
                                                                         proposed_hot_spare_info_p,
                                                                         selected_hot_spare_info_p,
                                                                         selection_priority,
                                                                         b_determine_selection_priority);
    if (status == FBE_STATUS_OK)        
    {
        return status;
    }

    /* Failed all soft rules.
     */
    return FBE_STATUS_CONTINUE;
}
/************************************************
 * end fbe_spare_lib_selection_apply_soft_rules()
 ************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_apply_selection_algorithm()
 ******************************************************************************
 * @brief
 *
 * @param packet_p - The packet requesting this operation.
 * @param desired_spare_drive_info_p - Desired spare drive information..
 * @param curr_hot_spare_info_p - Current hot spare info.
 * @param selected_hot_spare_info_p - Previously selected hot spare information.
 *
 * @return status 
 * @author
 *  05/16/2012  Ron Proulx - Updated.
 ******************************************************************************/
fbe_status_t fbe_spare_lib_selection_apply_selection_algorithm(fbe_spare_selection_info_t *desired_spare_drive_info_p,
                                                                      fbe_spare_selection_info_t *curr_hot_spare_info_p,
                                                                      fbe_spare_selection_info_t *selected_hot_spare_info_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
   
    /*! @note
     *          Apply hard rules.  If the proposed replacement drive doesn't
     *          pass the hard rules it is not a suitable replacement drive.
     */
    status = fbe_spare_lib_selection_apply_hard_rules(desired_spare_drive_info_p, curr_hot_spare_info_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /*! @note
     *          Apply soft rules.  If the proposed replacement drive doesn't
     *          pass the soft rules it is not the best replacement drive.  We
     *          invoke the method (2) times:
     *              o First time to determine the selection priority of the
     *                proposed spare drive.
     *              o Second time to determine and choose it if it is a better
     *                match.
     *              
     */
    status = fbe_spare_lib_selection_apply_soft_rules(desired_spare_drive_info_p, curr_hot_spare_info_p, selected_hot_spare_info_p,
                                                      FBE_TRUE /* Simply determine selection priority of proposed spare.*/);
    if (status != FBE_STATUS_CONTINUE)
    {
        /* If we couldn't determine the selection priority of the propose drive
         * something is wrong.
         */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s determine priority for pvd obj: 0x%x b/e/s: %d_%d_%d failed. status: 0x%x\n",
                               __FUNCTION__, 
                               curr_hot_spare_info_p->spare_object_id,
                               curr_hot_spare_info_p->spare_drive_info.drive_type,
                               curr_hot_spare_info_p->spare_drive_info.port_number,
                               curr_hot_spare_info_p->spare_drive_info.enclosure_number,
                               curr_hot_spare_info_p->spare_drive_info.slot_number);
        return status;
    }

    /* Now determine if the proposed spare is a better fit or not.
     */
    status = fbe_spare_lib_selection_apply_soft_rules(desired_spare_drive_info_p, curr_hot_spare_info_p, selected_hot_spare_info_p,
                                                      FBE_FALSE /* Determine if the proposed spare is a better fit. */);
    if (status == FBE_STATUS_OK)
    {
        return status;
    }

    /* If above rules does not apply and we have not selected any hot spare so 
     * using above rules then select the current hot spare during this cycle.
     */
    if (selected_hot_spare_info_p->spare_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "spare_selection: select pvd obj: 0x%x drive type: %d b/e/s: %d_%d_%d capacity: 0x%llx\n", 
                               curr_hot_spare_info_p->spare_object_id,
                               curr_hot_spare_info_p->spare_drive_info.drive_type,
                               curr_hot_spare_info_p->spare_drive_info.port_number,
                               curr_hot_spare_info_p->spare_drive_info.enclosure_number,
                               curr_hot_spare_info_p->spare_drive_info.slot_number,
                               curr_hot_spare_info_p->spare_drive_info.configured_capacity);
        fbe_spare_lib_selection_soft_rules_update_selected_drive(curr_hot_spare_info_p, selected_hot_spare_info_p);
        return FBE_STATUS_OK;
    }

    /* Other return status.
     */
    return status;
}
/******************************************************************************
 * end fbe_spare_lib_selection_apply_selection_algorithm()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_select_suitable_spare_from_spare_pool()
 ******************************************************************************
 * @brief
 * It is used to select the suitable spare from the spare pool.
 *
 * @param packet_p                   - The packet requesting this operation.
 * @param desired_spare_drive_info_p       - Desired spare drive info.
 * @param spare_drive_pool_p         - Spare drive pool.
 * @param selected_spare_object_id_p - Returns selected spare object-id on success.
 * @param selected_spare_info_p       - Selected spare drive info.
 * @param vd_object_id - Virtual drive object id
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t 
fbe_spare_lib_selection_select_suitable_spare_from_spare_pool(fbe_packet_t *packet_p,
                                                              fbe_object_id_t *selected_spare_object_id_p,
                                                              fbe_spare_drive_info_t *desired_spare_drive_info_p,
                                                              fbe_spare_drive_pool_t *spare_drive_pool_p,
                                                              fbe_spare_drive_info_t *selected_spare_info_p,
                                                              fbe_object_id_t vd_object_id)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   curr_spare_index = 0;
    fbe_spare_selection_info_t  desired_spare_info;
    fbe_spare_selection_info_t  curr_hot_spare_info;
    fbe_spare_selection_info_t  selected_hot_spare_info;
    
    FBE_SPARE_LIB_TRACE_FUNC_ENTRY();

    /* Return with error if input parameters are NULL. */
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (desired_spare_drive_info_p);
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (spare_drive_pool_p);
    FBE_SPARE_LIB_TRACE_RETURN_ON_NULL (selected_spare_info_p);

    /* Initialize the selected hot spare drive information as default values. */
    fbe_spare_initialize_spare_selection_drive_info(&desired_spare_info);
    fbe_copy_memory(&desired_spare_info.spare_drive_info, desired_spare_drive_info_p, sizeof(*desired_spare_drive_info_p));
    fbe_spare_lib_hot_spare_info_set_object_id(&desired_spare_info, vd_object_id);

    /* Initialize the selected hot spare drive information as default values. */
    fbe_spare_initialize_spare_selection_drive_info(&selected_hot_spare_info);
    
    /* Loop through the spare object list to select the best suitable spare. */
    while (spare_drive_pool_p->spare_object_list[curr_spare_index] != FBE_OBJECT_ID_INVALID)
    {
        /* Update the hot spare drive inforamtion with current object-id. */
        fbe_spare_initialize_spare_selection_drive_info(&curr_hot_spare_info);
        fbe_spare_lib_hot_spare_info_set_object_id(&curr_hot_spare_info,
                                                   spare_drive_pool_p->spare_object_list[curr_spare_index]);

        /* Get hot spare information for this hot spare object. */
        status = fbe_spare_lib_selection_get_hot_spare_info(curr_hot_spare_info.spare_object_id,
                                                            &curr_hot_spare_info.spare_drive_info);
        if (status != FBE_STATUS_OK) 
        {
            /* Ignore this hot spare object on error and jump to next object. */
            curr_spare_index++;
            continue;
        }

        /*! @note We must allow system drives to be used as spare since the user
         *        can bind on the system drives and then need to `spare' back to
         *        them after the original spare fails.  Log an warning and
         *        continue.
         */
        if (fbe_database_is_object_system_pvd(curr_hot_spare_info.spare_object_id) == FBE_TRUE)
        {
            /* Log an warning and continue.
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Allow system pvd: 0x%x as a spare.\n", 
                                   __FUNCTION__, curr_hot_spare_info.spare_object_id);
        }

        /* Apply the rules to select the best suitable spare. */
        status = fbe_spare_lib_selection_apply_selection_algorithm(&desired_spare_info,
                                                                   &curr_hot_spare_info,
                                                                   &selected_hot_spare_info);

        /* Increment the index to go through the next available spare. */
        curr_spare_index++;
    }

    /* Copy the selected spare object id. */
    *selected_spare_object_id_p = selected_hot_spare_info.spare_object_id;

    /* copy the selected hot spare drive info. */
    fbe_copy_memory (selected_spare_info_p,
                     &(selected_hot_spare_info.spare_drive_info), 
                     sizeof(fbe_spare_drive_info_t));   

    /* Return the status. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_selection_select_suitable_spare_from_spare_pool()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_drive_performance_tier_number()
 ******************************************************************************
 * @brief
 * It is used to get the performance tier number for a given drive type.
 *
 * @param desired_spare_drive_type - Desired spare drive type.
 * @param performance_tier_number_p - Returns performance tier number on success.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/28/2013  Ron Proulx  - Updated.
 * 
 ******************************************************************************/
 fbe_status_t
 fbe_spare_lib_selection_get_drive_performance_tier_number(fbe_drive_type_t  desired_spare_drive_type, 
                                                           fbe_performance_tier_number_t *performance_tier_number_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_performance_tier_number_t   primary_tier_number = FBE_PERFORMANCE_TIER_INVALID;
    fbe_performance_tier_number_t   secondary_tier_number = FBE_PERFORMANCE_TIER_INVALID;
    fbe_u32_t                       tier_table_index;
    fbe_performance_tier_drive_information_t tier_drive_info;
    fbe_u32_t                       tier_drive_index;

    /* By default the performance teir is invalid
     */
    *performance_tier_number_p = FBE_PERFORMANCE_TIER_INVALID;

    /* Validate that the desired drive type is supported
     */
    if ((desired_spare_drive_type == FBE_DRIVE_TYPE_INVALID) ||
        (desired_spare_drive_type >= FBE_DRIVE_TYPE_LAST)       )
    {
        /* This is an error */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s invalid desired drive type: %d\n",
                               __FUNCTION__, desired_spare_drive_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Walk thru the performance tier table and locate either a primary
     * or a secondary tier number.
     */
    for (tier_table_index = FBE_PERFORMANCE_TIER_ZERO; 
         (tier_table_index < FBE_PERFORMANCE_TIER_LAST) && (primary_tier_number == FBE_PERFORMANCE_TIER_INVALID); 
         tier_table_index++)
    {
        /* Get the performance tier information for the tier indicated
         */
        status = fbe_spare_lib_get_performance_table_info(tier_table_index, &tier_drive_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s get drive tier info for tier: %d failed - status: 0x%x\n",
                               __FUNCTION__, tier_table_index, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Find the first compatible drive type in the tier specified.
         */
        for (tier_drive_index = 0; tier_drive_index < FBE_DRIVE_TYPE_LAST; tier_drive_index++)
        {
            /* Check if the proposed spare drive 
             */
            if (tier_drive_info.supported_drive_type_array[tier_drive_index] == desired_spare_drive_type)
            {
                /* If it is the first entry then populate the primary value.
                 * Else populate the secondary.
                 */
                if (tier_drive_index == 0)
                {
                    primary_tier_number = tier_table_index;
                }
                else
                {
                    secondary_tier_number = tier_table_index;
                }
                break;
            }

        } /* end for all drive types */

    } /* end for all tiers */

    /* If neither the primary or secondary is found return a failure.
     */
    if (primary_tier_number == FBE_PERFORMANCE_TIER_INVALID)
    {
        /* If the secondary is also invalid report the failure.
         */
        if (secondary_tier_number == FBE_PERFORMANCE_TIER_INVALID)
        {
            /* Report the error */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                   "%s get drive tier info for drive type: %d not found\n",
                                   __FUNCTION__, desired_spare_drive_type);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /* Else generate a warning (each drive type should have a primary)
             */
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "spare: get performance tier for drive type: %d only secondary tier found: %d\n",
                                   desired_spare_drive_type, secondary_tier_number);
            *performance_tier_number_p = secondary_tier_number;
            return FBE_STATUS_OK;
        }
    }

    /* Use the primary
     */
    *performance_tier_number_p = primary_tier_number;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_drive_performance_tier_number()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_spare_lib_selection_get_drive_performance_tier_group()
 ******************************************************************************
 * @brief
 * It is used to get the performance tier `group' number.  The performance tier
 * `group' is the lowest performance tier that supports this drive type.
 *
 * @param desired_spare_drive_type - Desired spare drive type.
 * @param performance_tier_group_p - Returns performance tier group on success.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/15/2015  Ron Proulx  - Created.
 * 
 ******************************************************************************/
 fbe_status_t
 fbe_spare_lib_selection_get_drive_performance_tier_group(fbe_drive_type_t  desired_spare_drive_type, 
                                                          fbe_performance_tier_number_t *performance_tier_group_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       tier_table_index;
    fbe_performance_tier_drive_information_t tier_drive_info;
    fbe_u32_t                       tier_drive_index;

    /* By default the performance teir is invalid
     */
    *performance_tier_group_p = FBE_PERFORMANCE_TIER_INVALID;

    /* Validate that the desired drive type is supported
     */
    if ((desired_spare_drive_type == FBE_DRIVE_TYPE_INVALID) ||
        (desired_spare_drive_type >= FBE_DRIVE_TYPE_LAST)       )
    {
        /* This is an error */
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s invalid desired drive type: %d\n",
                               __FUNCTION__, desired_spare_drive_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Walk thru the performance tier table and locate either a primary
     * or a secondary tier number.
     */
    for (tier_table_index = FBE_PERFORMANCE_TIER_ZERO; 
         tier_table_index < FBE_PERFORMANCE_TIER_LAST; 
         tier_table_index++)
    {
        /* Get the performance tier information for the tier indicated
         */
        status = fbe_spare_lib_get_performance_table_info(tier_table_index, &tier_drive_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "%s get drive tier info for tier: %d failed - status: 0x%x\n",
                               __FUNCTION__, tier_table_index, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Find the first compatible drive type in the tier specified.
         */
        for (tier_drive_index = 0; tier_drive_index < FBE_DRIVE_TYPE_LAST; tier_drive_index++)
        {
            /* Check if the proposed spare drive 
             */
            if (tier_drive_info.supported_drive_type_array[tier_drive_index] == desired_spare_drive_type)
            {
                /* Simply return the lowest tier that support this drive type.
                 */
                *performance_tier_group_p = tier_table_index;
                return FBE_STATUS_OK;
            }

        } /* end for all drive types */

    } /* end for all tiers */

    /* If neither the primary or secondary is found return a failure.
     */
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                           "%s get drive tier info for drive type: %d not found\n",
                           __FUNCTION__, desired_spare_drive_type);
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************************************************
 * end fbe_spare_lib_selection_get_drive_performance_tier_group()
 ******************************************************************************/

/********************************* 
 * end file fbe_spare_selection.c
 *********************************/
