/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_group_emeh.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group code that handles extended media error
 *  handling (EMEH).
 * 
 * @version
 *  03/02/2015:  Created. Ron Proulx
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_group_object.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_geometry.h"
#include "fbe_private_space_layout.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"

/*****************************************
 * LOCAL GLOBALS
 *****************************************/

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
static fbe_status_t fbe_raid_group_emeh_set_dieh_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_get_class_default_mode()
 *****************************************************************************
 *
 * @brief   Get the default EMEH mode for the raid group class (i.e. the mode
 *          for all raid groups).
 *
 * @param   emeh_mode_p - Address of mode value to update
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_get_class_default_mode(fbe_raid_emeh_mode_t *emeh_mode_p)
{
    *emeh_mode_p = FBE_RAID_EMEH_MODE_DEFAULT;
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_emeh_get_class_default_mode()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_get_class_current_mode()
 *****************************************************************************
 *
 * @brief   Get the current EMEH mode for the raid group class (i.e. the mode
 *          for all raid groups). This might have been modified from the default.
 *
 * @param   emeh_mode_p - Address of mode value to update
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_get_class_current_mode(fbe_raid_emeh_mode_t *emeh_mode_p)
{
    fbe_u32_t  *emeh_params_p = fbe_raid_group_class_get_extended_media_error_handling_params();

    *emeh_mode_p = (*emeh_params_p & FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_MODE_MASK);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_emeh_get_class_current_mode()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_set_class_current_mode()
 *****************************************************************************
 *
 * @brief   Set the current EMEH mode for the raid group class (i.e. the mode
 *          for all raid groups).
 *
 * @param   emeh_mode_p - Address of mode value to update
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_set_class_current_mode(fbe_raid_emeh_mode_t set_emeh_mode)
{
    fbe_u32_t  *emeh_params_p = fbe_raid_group_class_get_extended_media_error_handling_params();

    *emeh_params_p &= ~FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_MODE_MASK;
    *emeh_params_p |= (set_emeh_mode & FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_MODE_MASK);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_emeh_set_class_current_mode()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_get_class_default_increase()
 *****************************************************************************
 *
 * @brief   Get the default EMEH media error threshold increase for the raid 
 *          group class (i.e. the mode for all raid groups).
 *
 * @param   threshold_increase_p - Address of increase value to update
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_get_class_default_increase(fbe_u32_t *threshold_increase_p)
{
    *threshold_increase_p = FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT_INCREASE;
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_emeh_get_class_default_increase()
 ******************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_get_class_current_increase()
 *****************************************************************************
 *
 * @brief   Get the current EMEH media error threshold increase for the raid 
 *          group class (i.e. the mode for all raid groups).  This might have
 *          been modified from the default.
 *
 * @param   threshold_increase_p - Address of increase value to update
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_get_class_current_increase(fbe_u32_t *threshold_increase_p)
{
    fbe_u32_t  *emeh_params_p = fbe_raid_group_class_get_extended_media_error_handling_params();
    fbe_u32_t   threshold_increase;
                                      
    threshold_increase = (*emeh_params_p & FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_MASK) >> FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_SHIFT;
    if (threshold_increase == FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_USE_DEFAULT_INCREASE) {
        threshold_increase = FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT_INCREASE;
    }
    *threshold_increase_p = threshold_increase;
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_emeh_get_class_current_increase()
 ******************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_set_class_current_increase()
 *****************************************************************************
 *
 * @brief   Set the current EMEH increase for the raid group class (i.e. the mode
 *          for all raid groups).
 *
 * @param   set_threshold_increase - The percent incresae in the media error threshold
 * @param   set_emeh_control - Determine if we need to restore the default or not
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_set_class_current_increase(fbe_u32_t set_threshold_increase,
                                                            fbe_raid_emeh_command_t set_emeh_control)
{
    fbe_u32_t  *emeh_params_p = fbe_raid_group_class_get_extended_media_error_handling_params();

    *emeh_params_p &= ~FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_MASK;
    if (set_emeh_control == FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE) {
        *emeh_params_p |= ((FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT_INCREASE << FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_SHIFT) & 
                            FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_MASK);
    } else {    
        *emeh_params_p |= ((set_threshold_increase << FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_SHIFT) & 
                            FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_MASK);
    }
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_emeh_set_class_current_increase()
 ******************************************************/

/*!***************************************************************************
 *          fbe_raid_group_is_extended_media_error_handling_capable()
 *****************************************************************************
 *
 * @brief   This function determine is the raid group supports extended media
 *          error handling.
 *
 * @param   raid_group_p - raid group
 *
 * @return fbe_status_t
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_group_is_extended_media_error_handling_capable(fbe_raid_group_t *raid_group_p)
{
    fbe_object_id_t         object_id = ((fbe_base_object_t *)raid_group_p)->object_id;
    fbe_bool_t              b_is_emeh_supported = FBE_TRUE;
    fbe_raid_geometry_t    *raid_geometry_p = NULL;
    fbe_raid_group_type_t   raid_type;
    fbe_raid_emeh_mode_t    emeh_class_default_mode;
    fbe_raid_emeh_mode_t    emeh_class_current_mode;
    fbe_raid_emeh_mode_t    emeh_enabled_mode;
    fbe_raid_emeh_mode_t    emeh_current_mode;
    fbe_class_id_t          class_id;

    /* First determine if EMEH is support at all for this raid group.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    class_id = fbe_raid_group_get_class_id(raid_group_p);

    /*! @note No need to run on RAID-10 and only need to run on (1)
     *        system raid group that includes all drives.
     */
    if ((class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)                         ||
        (raid_type == FBE_RAID_GROUP_TYPE_RAID10)                        ||        
        ((object_id != FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG) &&
         (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST)         )    )
    {
        b_is_emeh_supported = FBE_FALSE;
    }

    /* Now determine if enabled.  The class value over-rides the
     * current value.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_emeh_get_class_default_mode(&emeh_class_default_mode);
    fbe_raid_group_emeh_get_class_current_mode(&emeh_class_current_mode);
    fbe_raid_group_get_emeh_enabled_mode(raid_group_p, &emeh_enabled_mode);
    fbe_raid_group_get_emeh_current_mode(raid_group_p, &emeh_current_mode);
    if ((b_is_emeh_supported == FBE_TRUE)                              &&
        (emeh_enabled_mode != emeh_class_current_mode)                 &&
        (emeh_class_current_mode != emeh_class_default_mode)           &&
        ((emeh_enabled_mode == FBE_RAID_EMEH_MODE_ENABLED_NORMAL) ||
         (emeh_enabled_mode == FBE_RAID_EMEH_MODE_DISABLED)          )     )
    {
        /* If the class enabled has been changed and it doesn't match
         * the raid group, override the raid group enable with the class
         * value.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "emeh is capable - change current: %d enabled mode: %d to class mode: %d class default: %d\n",
                              emeh_current_mode, emeh_enabled_mode, emeh_class_current_mode, emeh_class_default_mode);
        emeh_enabled_mode = emeh_class_current_mode;
        fbe_raid_group_set_emeh_enabled_mode(raid_group_p, emeh_class_current_mode);
        if ((emeh_current_mode == FBE_RAID_EMEH_MODE_ENABLED_NORMAL) ||
            (emeh_current_mode == FBE_RAID_EMEH_MODE_DISABLED)          )
        {
            /* Update the current.
             */
            fbe_raid_group_set_emeh_current_mode(raid_group_p, emeh_class_current_mode);
        }
    }
    fbe_raid_group_unlock(raid_group_p);

    /* Now check if it is enabled.
     */
    if ((b_is_emeh_supported == FBE_FALSE)                 ||
        (emeh_enabled_mode == FBE_RAID_EMEH_MODE_INVALID)  ||
        (emeh_enabled_mode == FBE_RAID_EMEH_MODE_DISABLED)    )
    {
        return FBE_FALSE;
    }

    /* EMEH is supported.
     */
    return FBE_TRUE;
}
/**************************************************************** 
 * end fbe_raid_group_is_extended_media_error_handling_capable()
 ****************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_is_extended_media_error_handling_enabled()
 *****************************************************************************
 *
 * @brief   This function determines if EMEH is enabled for this raid group
 *          or not.
 *
 * @param   raid_group_p - raid group
 *
 * @return fbe_status_t
 * 
 * @note    Assumes caller has taken raid group lock
 *
 * @author
 *  06/17/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_group_is_extended_media_error_handling_enabled(fbe_raid_group_t *raid_group_p)
{
    fbe_object_id_t object_id = ((fbe_base_object_t *)raid_group_p)->object_id;
    fbe_raid_geometry_t        *raid_geometry_p = NULL;
    fbe_raid_group_type_t       raid_type;
    fbe_raid_emeh_mode_t        emeh_enabled_mode;
    fbe_class_id_t              class_id;

    /* First determine if enabled.  The class value over-rides the
     * current value.
     */
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    fbe_raid_group_get_emeh_enabled_mode(raid_group_p, &emeh_enabled_mode);
    if ((emeh_enabled_mode == FBE_RAID_EMEH_MODE_INVALID)  ||
        (emeh_enabled_mode == FBE_RAID_EMEH_MODE_DISABLED) ||
        (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)              )
    {
        return FBE_FALSE;
    }

    /* Get geometry and raid type
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    
    /*! @note No need to run on RAID-10 and only need to run on (1)
     *        system raid group that includes all drives.
     */
    if ((raid_type != FBE_RAID_GROUP_TYPE_RAID10)                        &&
        ((object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_RG) ||
         (object_id > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST)         )    )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/**************************************************************** 
 * end fbe_raid_group_is_extended_media_error_handling_enabled()
 ****************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_get_reliability_from_dieh_info()
 *****************************************************************************
 *
 * @brief   From the current EMEH reliability, use the reliability from the
 *          drive (a.k.a. DIEH reliability) to update the reliability.
 *
 * @param   raid_group_t - Pointer to raid group object
 * @param   drive_reliability - The reliability for this drive
 * @param   emeh_reliability_p - Address of current EMEH reliability to update
 *
 * @return fbe_status_t
 *
 * @author
 *  05/28/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_emeh_get_reliability_from_dieh_info(fbe_raid_group_t *raid_group_p,
                                                                fbe_pdo_dieh_media_reliability_t drive_reliability,
                                                                fbe_raid_emeh_reliability_t *emeh_reliability_p)
{
    /* Validate current EMEH reliability.
     */
    switch(*emeh_reliability_p)
    {
        case FBE_RAID_EMEH_RELIABILITY_UNKNOWN:
        case FBE_RAID_EMEH_RELIABILITY_VERY_HIGH:
        case FBE_RAID_EMEH_RELIABILITY_LOW:
            break;

        default:
            /* Unsupported drive DIEH reliability !
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "emeh get emeh reliability - unsupported EMEH reliability: %d \n",
                                  *emeh_reliability_p);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Translate form DIEH to EMEH.
     */
    switch(drive_reliability)
    {
        case FBE_DRIVE_MEDIA_RELIABILITY_UNKNOWN:
                /* Only update of the drive reliability is known.
                 */
                break;
        case FBE_DRIVE_MEDIA_RELIABILITY_VERY_HIGH:
        case FBE_DRIVE_MEDIA_RELIABILITY_HIGH:
        case FBE_DRIVE_MEDIA_RELIABILITY_NORMAL:
            /* These drive reliability are `very high' EMEH reliability.
             */
            switch(*emeh_reliability_p)
            {
                case FBE_RAID_EMEH_RELIABILITY_UNKNOWN:
                case FBE_RAID_EMEH_RELIABILITY_VERY_HIGH:
                    /* Goto `very high'. 
                     */
                    *emeh_reliability_p = FBE_RAID_EMEH_RELIABILITY_VERY_HIGH;
                    break;
                case FBE_RAID_EMEH_RELIABILITY_LOW:
                default:
                    /* Do not change to `very high' if it is currently low.
                     */
                    break;
            }
            break;

        case FBE_DRIVE_MEDIA_RELIABILITY_LOW:
        case FBE_DRIVE_MEDIA_RELIABILITY_VERY_LOW:
            /* These drive reliability are `low' EMEH reliability.
             */
            *emeh_reliability_p = FBE_RAID_EMEH_RELIABILITY_LOW;
            break;

        default:
            /* Unsupported drive DIEH reliability !
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "emeh get emeh reliability - unsupported DIEH reliability: %d \n",
                                  drive_reliability);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**************************************************************** 
 * end fbe_raid_group_emeh_get_reliability_from_dieh_info()
 ****************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_emeh_get_dieh_completion()
 ******************************************************************************
 *
 * @brief   Handle the completion of a subpacket that was sent downstream to
 *          get the DIEH information for a raid group position.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_emeh_get_dieh_completion(fbe_packet_t *packet_p,
                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t                                        status;
    fbe_raid_group_t                                   *raid_group_p = NULL;
    fbe_u32_t                                           width;
    fbe_payload_ex_t                                   *payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_control_status_t                        control_status;
    fbe_raid_group_send_to_downstream_positions_t      *send_to_downstream_p = NULL;
    fbe_physical_drive_get_dieh_media_threshold_t      *drive_dieh_request_p = NULL;
    fbe_raid_group_get_extended_media_error_handling_t *get_emeh_info_p = NULL;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_group_get_width(raid_group_p, &width);

    /* Get the send to downstream request.
     */
    fbe_raid_group_get_send_to_downsteam_request(raid_group_p, packet_p, &send_to_downstream_p);
    if (send_to_downstream_p == NULL) {
        /* If we could not get the send to downstream buffer trace an error
         * and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed to get send to downstream \n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the Get EMEH request buffer
     */
    get_emeh_info_p = (fbe_raid_group_get_extended_media_error_handling_t *)send_to_downstream_p->request_context;
    if (get_emeh_info_p == NULL) {
        /* If we could not get the send to downstream buffer trace an error
         * and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed to get EMEH request buffer \n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the control operation and status.
     */
    status = fbe_transport_get_status_code(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* Get the emeh buffer.
     */
    fbe_payload_control_get_buffer(control_operation_p, &drive_dieh_request_p);

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p, 
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "emeh get dieh pos: %d sts: %d cntrl_sts: %d dieh mode: %d rel: %d weight: %d paco pos: %d\n", 
                         send_to_downstream_p->position_in_progress,
                         status, control_status,
                         drive_dieh_request_p->dieh_mode,
                         drive_dieh_request_p->dieh_reliability,
                         drive_dieh_request_p->media_weight_adjust,
                         drive_dieh_request_p->vd_paco_position);

    /*! @note Copy the data for this position into EMEH request.
     */
    if ((status == FBE_STATUS_OK)                            &&
        (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)    &&
        (send_to_downstream_p->position_in_progress < width)    )
    {
        /* We are done. Populate the usurper.
         */
        get_emeh_info_p->dieh_mode[send_to_downstream_p->position_in_progress] = drive_dieh_request_p->dieh_mode;
        get_emeh_info_p->dieh_reliability[send_to_downstream_p->position_in_progress] = drive_dieh_request_p->dieh_reliability;
        get_emeh_info_p->media_weight_adjust[send_to_downstream_p->position_in_progress] = drive_dieh_request_p->media_weight_adjust;
        get_emeh_info_p->vd_paco_position[send_to_downstream_p->position_in_progress] = drive_dieh_request_p->vd_paco_position;
    }

    /* Now complete the packet so that the send to downstream can perform the
     * bookkeeping. We leave the packet status alone.
     */
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_raid_group_emeh_get_dieh_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_emeh_set_dieh_completion()
 ******************************************************************************
 *
 * @brief   Handle the completion of a subpacket that was sent downstream to
 *          change the DIEH threshold settings.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  03/02/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_emeh_set_dieh_completion(fbe_packet_t *packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_status_t                                    status;
    fbe_raid_group_t                               *raid_group_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_status_t                    control_status;
    fbe_payload_control_status_qualifier_t          control_qualifier;
    fbe_raid_group_send_to_downstream_positions_t  *send_to_downstream_p = NULL;
    fbe_physical_drive_set_dieh_media_threshold_t  *drive_dieh_request_p = NULL;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *)context;

    /* Get the send to downstream request.
     */
    fbe_raid_group_get_send_to_downsteam_request(raid_group_p, packet_p, &send_to_downstream_p);
    if (send_to_downstream_p == NULL) {
        /* If we could not get the send to downstream buffer trace an error
         * and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Failed to get send to downstream \n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the control operaiton and status.
     */
    status = fbe_transport_get_status_code(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* Get the emeh buffer.
     */
    fbe_payload_control_get_buffer(control_operation_p, &drive_dieh_request_p);

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p, 
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "emeh set dieh cmd: %d option: 0x%x to pos: %d complete sts: %d cntrl_sts: %d\n", 
                         drive_dieh_request_p->cmd, drive_dieh_request_p->option,
                         send_to_downstream_p->position_in_progress, status, control_status);

    /*! @note For EMEH there is nothing to do on the completion.  But for other 
     *        users of send to downsteam you would add any processing here.
     */

    /* Now complete the packet so that the send to downstream can perform the
     * bookkeeping. We leave the packet status alone.
     */
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_raid_group_emeh_set_dieh_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_peer_memory_update()
 ***************************************************************************** 
 * 
 * @brief   This function looks at the raid group clustered bits
 *          and sees if we need to set one of the emeh conditions.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  05/11/2015  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_emeh_peer_memory_update(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_emeh_command_t emeh_request = FBE_RAID_EMEH_COMMAND_INVALID;

    /* If the peer requested an emeh command and the commmand hasn't been
     * started locally then start it now.
     */
    if ( fbe_raid_group_is_any_peer_clustered_flag_set(raid_group_p, (FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED |
                                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED     |
                                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED |
                                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED    )) )
    {
        fbe_bool_t  b_is_active;

        fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_get_emeh_request(raid_group_p, &emeh_request);

        /*! @note Since this routine could be invoke after we have set the
         *        but before it has run, there is the possibility emeh_request
         *        being valid (but for the specific request).
         */
        if ( fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED) &&
            !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE)    &&
            !fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_DEGRADED_STARTED)               )
        {
            if (emeh_request == FBE_RAID_EMEH_COMMAND_INVALID)
            {
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update start: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);

                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                       (fbe_base_object_t *) raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            }
            else if (emeh_request != FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update expected: %d but found: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED, emeh_request,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);
            }
        }
        else if ( fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED) &&
                 !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE)    &&
                 !fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_PACO_STARTED)               )
        {
            if (emeh_request == FBE_RAID_EMEH_COMMAND_INVALID)
            {
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_PACO_STARTED);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update start: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_PACO_STARTED,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);

                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                       (fbe_base_object_t *) raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            }
            else if (emeh_request != FBE_RAID_EMEH_COMMAND_PACO_STARTED)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update expected: %d but found: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_PACO_STARTED, emeh_request,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);
            }
        }
        else if ( fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED) &&
                 !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE)    &&
                 !fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_INCREASE_STARTED)               )
        {
            if (emeh_request == FBE_RAID_EMEH_COMMAND_INVALID)
            {
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update start: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);

                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                       (fbe_base_object_t *) raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            }
            else if (emeh_request != FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update expected: %d but found: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS, emeh_request,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);
            }
        }
        else if ( fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED) &&
                 !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE)    &&
                 !fbe_raid_group_is_local_state_set(raid_group_p, (fbe_u64_t)FBE_RAID_GROUP_LOCAL_STATE_EMEH_RESTORE_STARTED)    )
        {
            if (emeh_request == FBE_RAID_EMEH_COMMAND_INVALID)
            {
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update start: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);

                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                       (fbe_base_object_t *) raid_group_p,
                                       FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            }
            else if (emeh_request != FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "emeh peer memory update expected: %d but found: %d %s local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                                      FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE, emeh_request,
                                      b_is_active ? "Active" : "Passive",
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                      (unsigned long long)raid_group_p->local_state);
            }
        }
        else
        {
            /* Else the EMEH request is already active on the local.  Nothing to do.
             */
        }
        fbe_raid_group_unlock(raid_group_p);
    }
    
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_raid_group_emeh_peer_memory_update()
 **********************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_degraded_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function after starting a emeh request.
 *          This function simply trace an error and completes the packet back
 *          to the monitor.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_emeh_start_degraded_completion(fbe_packet_t *packet_p,
                                                                  fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_raid_emeh_command_t             emeh_command = FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED;

    /* The `Done' flag will be set when the peer is complete.  Update the current
     * mode to `degraded - High Availability'.
     */
    raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_emeh_current_mode(raid_group_p, FBE_RAID_EMEH_MODE_DEGRADED_HA);
    fbe_raid_group_unlock(raid_group_p);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If the request failed simply generate a warning.
     */
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: emeh request: %d failed - status: 0x%x control status: 0x%x\n",
                              emeh_command, status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_degraded_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_degraded()
 *****************************************************************************
 *
 * @brief   This function allocates the memory for modifying the extended
 *          media error handling (EMEH) for each the position that is still
 *          accessible.
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_start_degraded(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                    status;
    fbe_raid_position_bitmask_t                     rl_bitmask = 0;
    fbe_raid_position_bitmask_t                     positions_to_send_to = 0;
    fbe_physical_drive_set_dieh_media_threshold_t   local_dieh;
    fbe_raid_emeh_command_t                         emeh_command = FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED;
    fbe_object_id_t                                 rg_object_id;
    fbe_raid_group_number_t                         rg_id;

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_DEGRADED_STARTED);
    fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED);
    fbe_raid_group_unlock(raid_group_p);
    local_dieh.cmd = FBE_DRIVE_THRESHOLD_CMD_DISABLE;
    local_dieh.option = 0;    /*! @note Option is ignored for this request type.*/

    /* Set the completion function to return to monitor.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_emeh_start_degraded_completion, raid_group_p);

    /* Get the rebuild logging bitmask.  Send to all positions that are not rl.
     */
    status = fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get rl bitmask failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING; 
    }

    /* Get the valid bitmask and remove the degraded positions.
     */
    status = fbe_raid_group_get_valid_bitmask(raid_group_p, &positions_to_send_to);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get valid bitmask - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    positions_to_send_to &= ~rl_bitmask;

    /* Always trace the start of the request.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "emeh command: %d start - positions to send to mask: 0x%x\n",
                          emeh_command, positions_to_send_to);

    /* Generate the event log with the positions that have error thresholds disabled.
     */
    fbe_raid_group_logging_get_raid_group_number_and_object_id(raid_group_p, &rg_id, &rg_object_id);
    fbe_event_log_write(SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_DISABLED, NULL, 0, 
                        "%d 0x%x 0x%x", 
                        rg_id, rg_object_id, positions_to_send_to);

    /*!  Allocate and populate the `send usurper to position' control operation
     *   from the packet.
     *
     *   @note If you need a buffer to track each position etc. include the
     *         size required here. 
     */
    status = fbe_raid_group_build_send_to_downstream_positions(raid_group_p, packet_p, positions_to_send_to, 
                                                               sizeof(fbe_physical_drive_set_dieh_media_threshold_t));
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: build send downstream failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /*! Now send the request to each non-degraded position and call the emeh
     *  completion for each.
     *
     *   @note If you need a buffer to track each position etc. Need to add
     *         control operation here.
     */
    fbe_raid_group_send_to_downstream_positions(raid_group_p, packet_p,
                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD,
                                                (fbe_u8_t *)&local_dieh,
                                                sizeof(fbe_physical_drive_set_dieh_media_threshold_t),
                                                (fbe_packet_completion_function_t)fbe_raid_group_emeh_set_dieh_completion,
                                                (fbe_packet_completion_context_t)NULL /* No context*/);
    return FBE_LIFECYCLE_STATUS_PENDING; 
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_degraded()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_complete_degraded()
 ***************************************************************************** 
 * 
 * @brief   This function waits for both the local and peer to `start' the
 *          specified EMEH request to complete.  Only the Active will clear
 *          flags
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_complete_degraded(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_emeh_command_t     emeh_command = FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED;
    fbe_bool_t                  b_is_active;
    fbe_bool_t                  b_is_local_started;
    fbe_bool_t                  b_is_peer_started;
    fbe_bool_t                  b_is_local_done;
    fbe_bool_t                  b_is_peer_done;
    fbe_bool_t                  b_is_peer_complete;

    /* Check if we are active or passive (only the active will clear the flags).
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);

    /* If the done flag is set on both the local an peer then we are done.
         */
    b_is_local_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                              FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED);
    b_is_local_done = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                           FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_started, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_done, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE);
    b_is_peer_complete = b_is_peer_done;

    /* If this is the active AND the request is started AND peer has started
     * AND we haven't marked done do it now.
     */
    if ( b_is_active        &&
         b_is_local_started &&
         b_is_peer_started  &&
        !b_is_local_done       )
    {

        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE);
        b_is_local_done = FBE_TRUE;
    }
    else if (!b_is_active        &&
              b_is_local_started &&
              b_is_peer_started  &&
              b_is_peer_done        )
    {
        /* If this is the passive AND the request is started AND peer has started
         * AND the peer is done, mark done now.
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE);
        b_is_local_done = FBE_TRUE;
    }

    /* If the local is complete check for the hook.
     */
    if (b_is_local_done) 
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_DEGRADED,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_DEGRADED_DONE, 
                                   &hook_status);   
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* The `peer complete' flag is for the case where the peer has completed
     * because it has seen the done flag.  Now the local will not see the done
     * flag (even though the peer is done).  We detect this case by checking the
     * peer start flag.  If our done flag is set and the peer start flag is not
     * set then the peer is done (complete).
     */
    if ( b_is_local_done   &&
        !b_is_peer_started    )
    {
        b_is_peer_complete = FBE_TRUE;
    }

    /* If we are not done just wait for next cycle.
     */
    if (!b_is_local_done    ||
        !b_is_peer_complete    ) 
    {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If both the local and peer are done then clear all local flags and 
     * the peer started flag.  Then clear our local state. 
     */
    fbe_raid_group_clear_clustered_flag(raid_group_p,(FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED | 
                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE      ));
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_DEGRADED_STARTED);
    fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
    fbe_raid_group_unlock(raid_group_p);

    /* If enabled trace the completion.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "emeh command: %d complete - active: %d start: %d peer: %d done: %d peer: %d\n",
                         emeh_command, b_is_active, 
                         b_is_local_started, b_is_peer_started,
                         b_is_local_done, b_is_peer_done);

    /* Clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)    
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_raid_group_emeh_complete_degraded()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_emeh_degraded_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is set when a raid group becomes degraded.
 *          The `enter high availability' DIEH usurper is sent to all non-degraded
 *          positions.  We don't want to take another drive offline when the 
 *          raid group is degraded (data unavailable).
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p    - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_emeh_degraded_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_bool_t                  b_is_started = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* Use raid group EMEH info to determine
     */
    fbe_raid_group_lock(raid_group_p);
    b_is_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED);

    /* If we have not started (i.e. we haven't send the degraded usurper) set it now.
     */
    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_DEGRADED_STARTED))
    {
        /* Release lock and send request.
         */
        fbe_raid_group_unlock(raid_group_p);

        /* Send the usurpers. */
        return fbe_raid_group_emeh_start_degraded(raid_group_p, packet_p);
    }

    /* Else the degraded flag is set, wait for peer.
     */
    fbe_raid_group_unlock(raid_group_p);

    /* Check the start hook
     */
    if (b_is_started)
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_DEGRADED,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_DEGRADED_START, 
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* Wait for peer */
    return fbe_raid_group_emeh_complete_degraded(raid_group_p, packet_p);
}
/******************************************************************************
 * end fbe_raid_group_emeh_degraded_cond_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_paco_get_emeh_info_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function for getting the EMEH information
 *          for the raid group that just started a proactive copy.  We use this
 *          information to determine if all the drives are `high availability'
 *          or not.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_emeh_paco_get_emeh_info_completion(fbe_packet_t *packet_p,
                                                                      fbe_packet_completion_context_t context)
{
    fbe_status_t                                        status;
    fbe_raid_group_get_extended_media_error_handling_t *get_emeh_info_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_control_status_t                        control_status;
    fbe_raid_group_t                                   *raid_group_p = NULL;
    fbe_u32_t                                           width;
    fbe_raid_position_bitmask_t                         positions_to_send_to = 0;
    fbe_raid_position_bitmask_t                         paco_bitmask = 0;
    fbe_physical_drive_set_dieh_media_threshold_t       local_dieh;   
    fbe_raid_emeh_reliability_t                         emeh_reliability = FBE_RAID_EMEH_RELIABILITY_UNKNOWN;
    fbe_u32_t                                           paco_position = FBE_EDGE_INDEX_INVALID;
    fbe_raid_emeh_command_t                             emeh_command = FBE_RAID_EMEH_COMMAND_PACO_STARTED;
    fbe_object_id_t                                     rg_object_id;
    fbe_raid_group_number_t                             rg_id;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;
    fbe_raid_group_get_width(raid_group_p, &width);

    /* Get the raid group reliability.
     */
    fbe_raid_group_get_emeh_reliability(raid_group_p, &emeh_reliability);

    /* get the sep payload and control operation */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the status is not ok release the control operation
       and return ok so that packet gets completed
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_buffer(control_operation_p, &get_emeh_info_p);
    if ((status != FBE_STATUS_OK)                               || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)       ||
        (emeh_reliability == FBE_RAID_EMEH_RELIABILITY_UNKNOWN)    )
    {
        /* If the request failed trace a warning and continue.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "emeh paco get emeh info failed - sts: %d cntrl sts: %d use low reliability and continue\n",
                              status, control_status);
        emeh_reliability = FBE_RAID_EMEH_RELIABILITY_LOW;
    }

    /* Release the control buffer and control operation */
    fbe_transport_release_buffer(get_emeh_info_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* Get the valid bitmask and remove the degraded positions.
     */
    status = fbe_raid_group_get_valid_bitmask(raid_group_p, &positions_to_send_to);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get valid bitmask - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Need to determine proactive copy position.
     */
    fbe_raid_group_get_emeh_paco_position(raid_group_p, &paco_position);
    if (emeh_reliability == FBE_RAID_EMEH_RELIABILITY_VERY_HIGH)
    {
        /* If the paco position isn't valid trace a error and send
         * the increase thresholds to all positions.
         */
        if (paco_position >= width)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "emeh paco: high reliability but paco position: %d is invalid \n",
                                  paco_position);
        }
        else
        {
            /* Else exclude the proactive copy position when changing thresholds.
             */
            paco_bitmask = (1 << paco_position);
        }
    }

    /* Disable the error thresholds for all non-paco positions.
     * If any of the drives is not highly reliable, then also disable
     * the threshold for the paco position.
     */
    positions_to_send_to &= ~paco_bitmask;
    local_dieh.cmd = FBE_DRIVE_THRESHOLD_CMD_DISABLE;
    local_dieh.option = 0; 

    /* Always trace the start of the request.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "emeh command: %d start - paco pos: %d reliability: %d positions to send to mask: 0x%x\n",
                          emeh_command, paco_position, emeh_reliability, positions_to_send_to);

    /* Generate the event log with the positions that have error thresholds disabled.
     */
    fbe_raid_group_logging_get_raid_group_number_and_object_id(raid_group_p, &rg_id, &rg_object_id);
    fbe_event_log_write(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_DISABLED, NULL, 0, 
                        "%d 0x%x %d 0x%x", 
                        rg_id, rg_object_id, paco_position, positions_to_send_to);

    /*!  Allocate and populate the `send usurper to position' control operation
     *   from the packet.  No need to set a completion since we already have.
     *
     *   @note If you need a buffer to track each position etc. include the
     *         size required here. 
     */
    status = fbe_raid_group_build_send_to_downstream_positions(raid_group_p, packet_p, positions_to_send_to, 
                                                               sizeof(fbe_physical_drive_set_dieh_media_threshold_t));
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: build send downstream failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*! Now send the request to each non-degraded position and call the emeh
     *  completion for each.
     *
     *   @note If you need a buffer to track each position etc. Need to add
     *         control operation here.
     */
    fbe_raid_group_send_to_downstream_positions(raid_group_p, packet_p,
                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD,
                                                (fbe_u8_t *)&local_dieh,
                                                sizeof(fbe_physical_drive_set_dieh_media_threshold_t),
                                                (fbe_packet_completion_function_t)fbe_raid_group_emeh_set_dieh_completion,
                                                (fbe_packet_completion_context_t)NULL /* No context*/);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_raid_group_emeh_paco_get_emeh_info_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_paco_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function after starting a emeh request.
 *          This function simply trace an error and completes the packet back
 *          to the monitor.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_emeh_start_paco_completion(fbe_packet_t *packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_raid_emeh_command_t             emeh_command = FBE_RAID_EMEH_COMMAND_PACO_STARTED;

    /* Update the mode.  The `Done' clustered flag will be set when the peer
     * has also started the request.
     */
    raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_emeh_current_mode(raid_group_p, FBE_RAID_EMEH_MODE_PACO_HA);
    fbe_raid_group_unlock(raid_group_p);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If the request failed simply generate a warning.
     */
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: emeh request: %d failed - status: 0x%x control status: 0x%x\n",
                              emeh_command, status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_paco_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_paco()
 *****************************************************************************
 *
 * @brief   This function allocates and sends th `get EMEH info' request to
 *          this raid group.
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_start_paco(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                        status;
    fbe_object_id_t                                     rg_object_id;
    fbe_raid_group_get_extended_media_error_handling_t *get_emeh_info_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_edge_index_t                                    upstream_edge_index;
    fbe_object_id_t                                     upstream_object_id;
    fbe_block_edge_t                                   *upstream_edge_p = NULL;

    /* Allocate the buffer for the result of the get EMEH info for this raid group.
     */
    rg_object_id = fbe_raid_group_get_object_id(raid_group_p);
    get_emeh_info_p = (fbe_raid_group_get_extended_media_error_handling_t *)fbe_transport_allocate_buffer();
    if (get_emeh_info_p == NULL)
    {
        /* If the allocation failed, trace a message and clear this condition.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to allocate buffer for get EMEH info\n",
                              __FUNCTION__);

        /* Clear the condition.
        */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)    
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Get the upstream edge information.  There should be an upstream edge
     * since PACO wouldn't have started otherwise.
     */
    fbe_block_transport_find_first_upstream_edge_index_and_obj_id(&raid_group_p->base_config.block_transport_server,
                                                                  &upstream_edge_index, &upstream_object_id);
    if (upstream_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* No upstream edge is unexpected.
         */
        fbe_transport_release_buffer(get_emeh_info_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s No upstream edge found\n",
                              __FUNCTION__);

        /* Clear the condition.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
        if (status != FBE_STATUS_OK)    
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    upstream_edge_p = (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id(
                        (fbe_base_transport_server_t *)&raid_group_p->base_config.block_transport_server,
                        upstream_object_id);

    /* Allocate a new control operation.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING,
                                        get_emeh_info_p,
                                        sizeof(fbe_raid_group_get_extended_media_error_handling_t));
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    /* Set the local state and the clustered flag.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_PACO_STARTED);
    fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED);
    fbe_raid_group_unlock(raid_group_p);

    /* Set the completion function to return to monitor.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_emeh_start_paco_completion, raid_group_p);

    /* Set the completion function for the get EMEH info.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_emeh_paco_get_emeh_info_completion, raid_group_p);

    /* Set the destination address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              rg_object_id); 

    /* Send and call callback
     */
    fbe_block_transport_send_control_packet((fbe_block_edge_t *)upstream_edge_p, packet_p);  
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_paco()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_complete_paco()
 ***************************************************************************** 
 * 
 * @brief   This function waits for both the local and peer to `start' the
 *          specified EMEH request to complete.  Only the Active will clear
 *          flags
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_complete_paco(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_emeh_command_t     emeh_command = FBE_RAID_EMEH_COMMAND_PACO_STARTED;
    fbe_bool_t                  b_is_active;
    fbe_bool_t                  b_is_local_started;
    fbe_bool_t                  b_is_peer_started;
    fbe_bool_t                  b_is_local_done;
    fbe_bool_t                  b_is_peer_done;
    fbe_bool_t                  b_is_peer_complete;

    /* Check if we are active or passive (only the active will clear the flags).
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);

    /* Get the status of the local and peer clustered flags.
     */
    b_is_local_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                              FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED);
    b_is_local_done = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                           FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_started, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_done, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE);
    b_is_peer_complete = b_is_peer_done;

    /* If this is the active AND the request is started AND peer has started
     * AND we haven't marked done do it now.
     */
    if ( b_is_active        &&
         b_is_local_started &&
         b_is_peer_started  &&
        !b_is_local_done       )
    {

        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE);
        b_is_local_done = FBE_TRUE;
    }
    else if (!b_is_active        &&
              b_is_local_started &&
              b_is_peer_started  &&
              b_is_peer_done        )
    {
        /* If this is the passive AND the request is started AND peer has started
         * AND the peer is done, mark done now.
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE);
        b_is_local_done = FBE_TRUE;
    }

    /* If the local is complete check for the hook.
     */
    if (b_is_local_done) 
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_DONE, 
                                   &hook_status);   
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* The `peer complete' flag is for the case where the peer has completed
     * because it has seen the done flag.  Now the local will not see the done
     * flag (even though the peer is done).  We detect this case by checking the
     * peer start flag.  If our done flag is set and the peer start flag is not
     * set then the peer is done (complete).
     */
    if ( b_is_local_done   &&
        !b_is_peer_started    )
    {
        b_is_peer_complete = FBE_TRUE;
    }

    /* If we are not done just wait for next cycle.
     */
    if (!b_is_local_done    ||
        !b_is_peer_complete    ) 
    {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If both the local and peer are done then clear all local flags and 
     * the peer started flag.  Then clear our local state. 
     */
    fbe_raid_group_clear_clustered_flag(raid_group_p,(FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED | 
                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE      ));
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_PACO_STARTED);
    fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
    fbe_raid_group_unlock(raid_group_p);

    /* If enabled trace the completion.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "emeh command: %d complete - active: %d start: %d peer: %d done: %d peer: %d\n",
                         emeh_command, b_is_active, 
                         b_is_local_started, b_is_peer_started,
                         b_is_local_done, b_is_peer_done);

    /* Clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)    
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_raid_group_emeh_complete_paco()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_emeh_paco_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is set when a proactive copy starts on one raid group
 *          position.  The following sequence is performed:
 *              1. A `get DIEH reliability information' usurper is sent to all
 *                 positions in the raid group using the `get raid group EMEH info'
 *                 usurper.
 *              2. Using the results of the get EMEH info usurper, determine if
 *                 all the drives hare `highly reliable' or not.
 *              3. If all the drives are highly reliable, DON'T change/disable
 *                 the media error thresholds on the proactive copy position.
 *              4. If one or more drives is NOT `highly reliable' the change/disable
 *                 media error thresholds will be sent to all positions.
 *              5. Send the change/disable media error thresholds to all selected
 *                 positions.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p    - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_emeh_paco_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_bool_t                  b_is_started = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* Use raid group EMEH info to determine
     */
    fbe_raid_group_lock(raid_group_p);
    b_is_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED);

    /* If we have not started (i.e. we haven't send the degraded usurper) set it now.
     */
    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_PACO_STARTED))
    {
        /* Release lock and send request.
         */
        fbe_raid_group_unlock(raid_group_p);

        /* Send the usurpers. */
        return fbe_raid_group_emeh_start_paco(raid_group_p, packet_p);
    }

    /* Else the paco flag is set, wait for peer.
     */
    fbe_raid_group_unlock(raid_group_p);

    /* Check the start hook
     */
    if (b_is_started)
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_START, 
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* Wait for peer */
    return fbe_raid_group_emeh_complete_paco(raid_group_p, packet_p);
}
/******************************************************************************
 * end fbe_raid_group_emeh_paco_cond_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_increase_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function after starting a emeh request.
 *          This function simply trace an error and completes the packet back
 *          to the monitor.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_emeh_start_increase_completion(fbe_packet_t *packet_p,
                                                                  fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_raid_emeh_command_t             emeh_command = FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS;

    /* The `Done' flag will be set when the peer completes.  Set the current
     * mode to `thresholds increased'.
     */
    raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_emeh_current_mode(raid_group_p, FBE_RAID_EMEH_MODE_THRESHOLDS_INCREASED);
    fbe_raid_group_unlock(raid_group_p);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If the request failed simply generate a warning.
     */
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: emeh request: %d failed - status: 0x%x control status: 0x%x\n",
                              emeh_command, status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_increase_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_increase()
 *****************************************************************************
 *
 * @brief   This function allocates the memory for modifying the extended
 *          media error handling (EMEH) for each the position that is still
 *          accessible.
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_start_increase(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                    status;
    fbe_raid_position_bitmask_t                     rl_bitmask = 0;
    fbe_raid_position_bitmask_t                     positions_to_send_to = 0;
    fbe_physical_drive_set_dieh_media_threshold_t   local_dieh;
    fbe_u32_t                                       percent_increase = 100;   
    fbe_raid_emeh_command_t                         emeh_command = FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS; 

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_INCREASE_STARTED);
    fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED);
    fbe_raid_group_unlock(raid_group_p);
    fbe_raid_group_emeh_get_class_current_increase(&percent_increase);
    local_dieh.cmd = FBE_DRIVE_THRESHOLD_CMD_INCREASE;
    local_dieh.option = percent_increase;

    /* Set the completion function to return to monitor.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_emeh_start_increase_completion, raid_group_p);

    /* Get the rebuild logging bitmask.  Send to all positions that are not rl.
     */
    status = fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get rl bitmask failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING; 
    }

    /* Get the valid bitmask and remove the degraded positions.
     */
    status = fbe_raid_group_get_valid_bitmask(raid_group_p, &positions_to_send_to);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get valid bitmask - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    positions_to_send_to &= ~rl_bitmask;

    /* Always trace the start of the request.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "emeh command: %d start - positions to send to mask: 0x%x\n",
                          emeh_command, positions_to_send_to);

    /*!  Allocate and populate the `send usurper to position' control operation
     *   from the packet.
     *
     *   @note If you need a buffer to track each position etc. include the
     *         size required here. 
     */
    status = fbe_raid_group_build_send_to_downstream_positions(raid_group_p, packet_p, positions_to_send_to, 
                                                               sizeof(fbe_physical_drive_set_dieh_media_threshold_t));
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: build send downstream failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /*! Now send the request to each non-degraded position and call the emeh
     *  completion for each.
     *
     *   @note If you need a buffer to track each position etc. Need to add
     *         control operation here.
     */
    fbe_raid_group_send_to_downstream_positions(raid_group_p, packet_p,
                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD,
                                                (fbe_u8_t *)&local_dieh,
                                                sizeof(fbe_physical_drive_set_dieh_media_threshold_t),
                                                (fbe_packet_completion_function_t)fbe_raid_group_emeh_set_dieh_completion,
                                                (fbe_packet_completion_context_t)NULL /* No context*/);
    return FBE_LIFECYCLE_STATUS_PENDING; 
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_increase()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_complete_increase()
 ***************************************************************************** 
 * 
 * @brief   This function waits for both the local and peer to `start' the
 *          specified EMEH request to complete.  Only the Active will clear
 *          flags
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_complete_increase(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_emeh_command_t     emeh_command = FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS;
    fbe_bool_t                  b_is_active;
    fbe_bool_t                  b_is_local_started;
    fbe_bool_t                  b_is_peer_started;
    fbe_bool_t                  b_is_local_done;
    fbe_bool_t                  b_is_peer_done;
    fbe_bool_t                  b_is_peer_complete;

    /* Check if we are active or passive (only the active will clear the flags).
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);

    /* If the done flag is set on both the local an peer then we are done.
         */
    b_is_local_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                              FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED);
    b_is_local_done = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                           FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_started, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_done, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE);
    b_is_peer_complete = b_is_peer_done;

    /* If this is the active AND the request is started AND peer has started
     * AND we haven't marked done do it now.
     */
    if ( b_is_active        &&
         b_is_local_started &&
         b_is_peer_started  &&
        !b_is_local_done       )
    {

        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE);
        b_is_local_done = FBE_TRUE;
    }
    else if (!b_is_active        &&
              b_is_local_started &&
              b_is_peer_started  &&
              b_is_peer_done        )
    {
        /* If this is the passive AND the request is started AND peer has started
         * AND the peer is done, mark done now.
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE);
        b_is_local_done = FBE_TRUE;
    }

    /* If the local is complete check for the hook.
     */
    if (b_is_local_done) 
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_INCREASE,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_INCREASE_DONE, 
                                   &hook_status);   
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* The `peer complete' flag is for the case where the peer has completed
     * because it has seen the done flag.  Now the local will not see the done
     * flag (even though the peer is done).  We detect this case by checking the
     * peer start flag.  If our done flag is set and the peer start flag is not
     * set then the peer is done (complete).
     */
    if ( b_is_local_done   &&
        !b_is_peer_started    )
    {
        b_is_peer_complete = FBE_TRUE;
    }

    /* If we are not done just wait for next cycle.
     */
    if (!b_is_local_done    ||
        !b_is_peer_complete    ) 
    {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If both the local and peer are done then clear all local flags and 
     * the peer started flag.  Then clear our local state. 
     */
    fbe_raid_group_clear_clustered_flag(raid_group_p,(FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED | 
                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE      ));
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_INCREASE_STARTED);
    fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
    fbe_raid_group_unlock(raid_group_p);

    /* If enabled trace the completion.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "emeh command: %d complete - active: %d start: %d peer: %d done: %d peer: %d\n",
                         emeh_command, b_is_active, 
                         b_is_local_started, b_is_peer_started,
                         b_is_local_done, b_is_peer_done);

    /* Clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)    
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_raid_group_emeh_complete_increase()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_emeh_increase_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is set it is requested to increase (NOT disable) the
 *          media error thresholds for all non-degraded positions.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p    - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  05/21/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_emeh_increase_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_bool_t                  b_is_started = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* Use raid group EMEH info to determine
     */
    fbe_raid_group_lock(raid_group_p);
    b_is_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED);

    /* If we have not started (i.e. we haven't send the degraded usurper) set it now.
     */
    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EMEH_INCREASE_STARTED))
    {
        /* Release lock and send request.
         */
        fbe_raid_group_unlock(raid_group_p);

        /* Send the usurpers. */
        return fbe_raid_group_emeh_start_increase(raid_group_p, packet_p);
    }

    /* Else the increase flag is set, wait for peer.
     */
    fbe_raid_group_unlock(raid_group_p);

    /* Check the start hook
     */
    if (b_is_started)
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_INCREASE,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_INCREASE_START, 
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* Wait for peer */
    return fbe_raid_group_emeh_complete_increase(raid_group_p, packet_p);
}
/******************************************************************************
 * end fbe_raid_group_emeh_increase_cond_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_start_restore_default_completion()
 ***************************************************************************** 
 * 
 * @brief   This is the completion function after starting a emeh request.
 *          This function simply trace an error and completes the packet back
 *          to the monitor.
 *
 * @param   packet_p - pointer to a control packet from the scheduler.
 * @param   context  - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  05/20/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_emeh_start_restore_default_completion(fbe_packet_t *packet_p,
                                                                         fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_raid_emeh_command_t             emeh_command = FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE;

    /* The `Done' flag will be set when the peer completes. Clear the paco
     * position and restore the current mode to `enabled - normal'.
     */
    raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_emeh_paco_position(raid_group_p, FBE_EDGE_INDEX_INVALID);
    fbe_raid_group_set_emeh_current_mode(raid_group_p, FBE_RAID_EMEH_MODE_ENABLED_NORMAL);
    fbe_raid_group_unlock(raid_group_p);

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If the request failed simply generate a warning.
     */
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: emeh request: %d failed - status: 0x%x control status: 0x%x\n",
                              emeh_command, status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_restore_default_completion()
 ******************************************************************************/

/*!**************************************************************
 *          fbe_raid_group_emeh_start_restore_default()
 ****************************************************************
 * @brief
 *  This function allocates the memory for modifying the extended
 *  media error handling (EMEH) for each the position that is still
 *  accessible.
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/20/2015  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_start_restore_default(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                    status;
    fbe_u32_t                                       width;
    fbe_u32_t                                       paco_position = FBE_EDGE_INDEX_INVALID;
    fbe_raid_position_bitmask_t                     positions_to_send_to = 0;
    fbe_physical_drive_set_dieh_media_threshold_t   local_dieh;    
    fbe_raid_emeh_command_t                         emeh_command = FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE;
    fbe_object_id_t                                 rg_object_id;
    fbe_raid_group_number_t                         rg_id; 

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    fbe_raid_group_lock(raid_group_p);

    /* Send the restore default threshold usurper to each position.
     */
    fbe_raid_group_set_local_state(raid_group_p, (fbe_u64_t)FBE_RAID_GROUP_LOCAL_STATE_EMEH_RESTORE_STARTED);
    fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED);
    local_dieh.cmd = FBE_DRIVE_THRESHOLD_CMD_RESTORE_DEFAULTS;
    local_dieh.option = 0;    /*! @note Option is ignored for this request type.*/
    fbe_raid_group_get_emeh_paco_position(raid_group_p, &paco_position);
    fbe_raid_group_unlock(raid_group_p);

    /* Set the completion function to return to monitor.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_emeh_start_restore_default_completion, raid_group_p);

    /* Get the valid bitmask and remove the degraded positions.
     */
    status = fbe_raid_group_get_valid_bitmask(raid_group_p, &positions_to_send_to);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get valid bitmask - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /* Always trace the start of the request.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "emeh command: %d start - positions to send to mask: 0x%x\n",
                          emeh_command, positions_to_send_to);

    /* Generate the event log with the positions that have error thresholds have been restored.
     */
    fbe_raid_group_logging_get_raid_group_number_and_object_id(raid_group_p, &rg_id, &rg_object_id);
    if (paco_position < width)
    {
        /* This request was initiated by the completion of a proactive copy.
         */
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_RESTORED, NULL, 0, 
                            "%d 0x%x %d 0x%x", 
                            rg_id, rg_object_id, paco_position, positions_to_send_to);
    }
    else
    {
        /* Else the raid group is no longer degraded (rebuild has completed).
         */
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_RESTORED, NULL, 0, 
                            "%d 0x%x 0x%x", 
                            rg_id, rg_object_id, positions_to_send_to);
    }

    /*!  Allocate and populate the `send usurper to position' control operation
     *   from the packet.
     *
     *   @note If you need a buffer to track each position etc. include the
     *         size required here. 
     */
    status = fbe_raid_group_build_send_to_downstream_positions(raid_group_p, packet_p, positions_to_send_to, 
                                                               sizeof(fbe_physical_drive_set_dieh_media_threshold_t));
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: build send downstream failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    /*! Now send the request to each non-degraded position and call the emeh
     *  completion for each.
     *
     *   @note If you need a buffer to track each position etc. Need to add
     *         control operation here.
     */
    fbe_raid_group_send_to_downstream_positions(raid_group_p, packet_p,
                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD,
                                                (fbe_u8_t *)&local_dieh,
                                                sizeof(fbe_physical_drive_set_dieh_media_threshold_t),
                                                (fbe_packet_completion_function_t)fbe_raid_group_emeh_set_dieh_completion,
                                                (fbe_packet_completion_context_t)NULL /* No context*/);
    return FBE_LIFECYCLE_STATUS_PENDING; 
}
/******************************************************************************
 * end fbe_raid_group_emeh_start_restore_default()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_emeh_complete_restore_default()
 ***************************************************************************** 
 * 
 * @brief   This function waits for both the local and peer to `start' the
 *          specified EMEH request to complete.  Only the Active will clear
 *          flags
 *
 * @param   raid_group_p - raid group
 * @param   packet_p - Packet for this usurper.
 *
 * @return  lifecycle_status
 *
 * @author
 *  05/20/2015  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_lifecycle_status_t fbe_raid_group_emeh_complete_restore_default(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_raid_emeh_command_t     emeh_command = FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE;
    fbe_bool_t                  b_is_active;
    fbe_bool_t                  b_is_local_started;
    fbe_bool_t                  b_is_peer_started;
    fbe_bool_t                  b_is_local_done;
    fbe_bool_t                  b_is_peer_done;
    fbe_bool_t                  b_is_peer_complete;

    /* Check if we are active or passive (only the active will clear the flags).
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    /* Set the local state and the clustered flag.  Then populate the DIEH usurper.
     * This done under the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);

    /* If the done flag is set on both the local an peer then we are done.
         */
    b_is_local_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                              FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED);
    b_is_local_done = fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                                           FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_started, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_is_peer_done, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE);
    b_is_peer_complete = b_is_peer_done;

    /* If this is the active AND the request is started AND peer has started
     * AND we haven't marked done do it now.
     */
    if ( b_is_active        &&
         b_is_local_started &&
         b_is_peer_started  &&
        !b_is_local_done       )
    {

        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE);
        b_is_local_done = FBE_TRUE;
    }
    else if (!b_is_active        &&
              b_is_local_started &&
              b_is_peer_started  &&
              b_is_peer_done        )
    {
        /* If this is the passive AND the request is started AND peer has started
         * AND the peer is done, mark done now.
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE);
        b_is_local_done = FBE_TRUE;
    }

    /* If the local is complete check for the hook.
     */
    if (b_is_local_done) 
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_DONE, 
                                   &hook_status);   
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* The `peer complete' flag is for the case where the peer has completed
     * because it has seen the done flag.  Now the local will not see the done
     * flag (even though the peer is done).  We detect this case by checking the
     * peer start flag.  If our done flag is set and the peer start flag is not
     * set then the peer is done (complete).
     */
    if ( b_is_local_done   &&
        !b_is_peer_started    )
    {
        b_is_peer_complete = FBE_TRUE;
    }

    /* If we are not done just wait for next cycle.
     */
    if (!b_is_local_done    ||
        !b_is_peer_complete    ) 
    {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If both the local and peer are done then clear all local flags and 
     * the peer started flag.  Then clear our local state. 
     */
    fbe_raid_group_clear_clustered_flag(raid_group_p,(FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED | 
                                                      FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE      ));
    fbe_raid_group_clear_local_state(raid_group_p, (fbe_u64_t)FBE_RAID_GROUP_LOCAL_STATE_EMEH_RESTORE_STARTED);
    fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
    fbe_raid_group_unlock(raid_group_p);

    /* If enabled trace the completion.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "emeh command: %d complete - active: %d start: %d peer: %d done: %d peer: %d\n",
                         emeh_command, b_is_active, 
                         b_is_local_started, b_is_peer_started,
                         b_is_local_done, b_is_peer_done);

    /* Clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK)    
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_raid_group_emeh_complete_restore_default()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_emeh_restore_default_cond_function()
 ******************************************************************************
 *
 * @brief   This condition is set when we need to restore the default media
 *          error thresholds to all positions.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p    - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   FBE_LIFECYCLE_STATUS_DONE
 *
 * @author
 *  05/20/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_emeh_restore_default_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_bool_t                  b_is_started = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* Use raid group EMEH info to determine
     */
    fbe_raid_group_lock(raid_group_p);
    b_is_started = fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED);

    /* If we have not started (i.e. we haven't send the restore usurper) set it now.
     */
    if (!fbe_raid_group_is_local_state_set(raid_group_p, (fbe_u64_t)FBE_RAID_GROUP_LOCAL_STATE_EMEH_RESTORE_STARTED))
    {
        /* Release lock and send request.
         */
        fbe_raid_group_unlock(raid_group_p);

        /* Send the usurpers. */
        return fbe_raid_group_emeh_start_restore_default(raid_group_p, packet_p);
    }

    /* Else the restore default flags is set, wait for peer.
     */
    fbe_raid_group_unlock(raid_group_p);

    /* Check the start hook
     */
    if (b_is_started)
    {
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE,
                                   FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_START, 
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* Wait for peer */
    return fbe_raid_group_emeh_complete_restore_default(raid_group_p, packet_p);
}
/******************************************************************************
 * end fbe_raid_group_emeh_restore_default_cond_function()
 ******************************************************************************/


/*********************************** 
 * end file fbe_raid_group_emeh.c
 ***********************************/
