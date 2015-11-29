/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_fup.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for firmware upgrade functionality
 *  for base_environment.
 *
 * @version
 *   15-June-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_environment_fup_private.h"
#include "fbe_base_environment_private.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "EspMessages.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_jsmn.h"
#include "fbe_ps_mgmt.h"
#include "fbe_encl_mgmt.h"
#include "fbe_sps_mgmt.h"
#include "fbe_board_mgmt.h"
#include "fbe_cooling_mgmt.h"
#include <ctype.h>


static fbe_status_t 
fbe_base_env_fup_cond_handler(fbe_base_environment_t * pBaseEnv,
                              fbe_base_env_fup_transition_func_ptr_t pTransitionFunc,
                              fbe_base_env_fup_work_state_t nextWorkState);

static fbe_status_t 
fbe_base_env_fup_abort_upgrade(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_wait_for_inter_device_delay(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_check_priority(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_read_image_header(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_check_rev(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_read_entire_image(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_download_image(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_get_download_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_get_peer_permission(fbe_base_environment_t * pBaseEnv, 
                                fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_check_env_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_activate_image(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_get_activate_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t
fbe_base_env_fup_check_result(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_check_rev_change(fbe_base_environment_t * pBaseEnv,
                                  fbe_base_env_fup_work_item_t * pWorkItem,
                                  fbe_bool_t * pRevChanged);

static fbe_status_t fbe_base_env_fup_refresh_device_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_end_upgrade(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_post_completion(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t
fbe_base_env_fup_retry_needed(fbe_base_environment_t * pBaseEnv,
                              fbe_base_env_fup_work_item_t * pWorkItem,
                              fbe_bool_t * pRetry);

static fbe_status_t
fbe_base_env_fup_reset_work_item_for_retry(fbe_base_environment_t * pBaseEnv,
                                fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_ready_to_transition(fbe_base_env_fup_work_item_t * pWorkItem,
                             fbe_base_env_fup_work_state_t nextWorkState,
                             fbe_bool_t * pReadyToTransition);

static fbe_status_t
fbe_base_env_fup_get_work_item_image_header_info_in_mem(fbe_base_environment_t * pBaseEnv,
                                 fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t
fbe_base_env_fup_get_work_item_image_header_info(fbe_base_environment_t * pBaseEnv,
                                 fbe_base_env_fup_work_item_t * pWorkItem,
                                 fbe_base_env_fup_image_info_t * pImageInfo);

static fbe_status_t 
fbe_base_env_fup_read_image_from_file(fbe_base_environment_t * pBaseEnv, 
                                             fbe_base_env_fup_work_item_t * pWorkItem,
                                             fbe_u8_t * pImage,
                                             fbe_u32_t bytesToRead);
static fbe_status_t 
fbe_base_env_fup_send_parse_image_header_cmd(fbe_base_environment_t * pBaseEnv,
                                                  fbe_base_env_fup_work_item_t * pWorkItem,
                                                  fbe_u8_t * pImageHeader,
                                                  fbe_enclosure_mgmt_parse_image_header_t *  pParseImageHeader);

static fbe_status_t 
fbe_base_env_find_next_image_info_ptr(fbe_base_environment_t * pBaseEnv,
                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                      fbe_base_env_fup_image_info_t ** pImageInfoPtr);

static fbe_status_t 
fbe_base_env_fup_hw_at_lower_rev(fbe_base_environment_t * pBaseEnv,
                                 fbe_base_env_fup_work_item_t * pWorkItem,
                                 fbe_bool_t * pLowerRev);

static fbe_status_t 
fbe_base_env_fup_device_type_activate_in_progress(fbe_base_environment_t * pBaseEnv, 
                                               fbe_u64_t deviceType,
                                               fbe_bool_t * pActivateInProgress);

static fbe_bool_t 
fbe_base_env_fup_rev_change(fbe_base_environment_t * pBaseEnv,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pNewFirmwareRev);

static fbe_status_t 
fbe_base_env_fup_handle_download_status_change(fbe_base_environment_t * pBaseEnv,
                                               fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_handle_activate_status_change(fbe_base_environment_t * pBaseEnv,
                                               fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_fup_log_event(fbe_base_environment_t * pBaseEnv,
                           fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t fbe_base_env_fup_send_cmd(fbe_base_environment_t * pBaseEnv,
                                   fbe_base_env_fup_work_item_t * pWorkItem,
                                   fbe_enclosure_firmware_opcode_t firmwareOpCode);

static fbe_status_t fbe_base_env_fup_succeeded(fbe_base_environment_t * pBaseEnv,
                                               fbe_base_env_fup_work_item_t * pWorkItem,
                                               fbe_bool_t * pUpgradeSucceeded);

static fbe_status_t fbe_base_env_fup_check_peer_pending(fbe_base_environment_t * pBaseEnv,
                                                fbe_base_env_fup_work_item_t *pWorkItem);

static fbe_status_t fbe_base_env_fup_send_peer_perm(fbe_base_environment_t * pBaseEnv,
                                            fbe_base_env_fup_work_item_t *pWorkItem);

static fbe_status_t fbe_base_env_fup_processReceivedCmiMessage(fbe_base_environment_t * pBaseEnv, 
                                                               fbe_base_env_fup_cmi_packet_t *pFupCmiPacket);

static fbe_status_t fbe_base_env_save_fup_info(fbe_base_environment_t * pBaseEnv, 
                                        fbe_base_env_fup_work_item_t * pWorkItem);

static fbe_status_t fbe_base_env_fup_encl_icm_priority_check(fbe_base_environment_t *pBaseEnv, 
                                                             fbe_base_env_fup_work_item_t *pWorkItem,
                                                             fbe_bool_t             *pWaitForEE);

static fbe_status_t fbe_base_env_fup_update_wait_before_upgrade_flag(fbe_base_environment_t * pBaseEnv);

static fbe_status_t fbe_base_env_fup_fwUpdatedFailed(fbe_base_environment_t * pBaseEnv,
                                                     fbe_u64_t deviceType,
                                                     fbe_device_physical_location_t *pLocation,
                                                     fbe_bool_t newValue);

static fbe_status_t fbe_base_env_fup_parse_manifest_file(fbe_base_environment_t * pBaseEnv, 
                                                         fbe_char_t * pJon, 
                                                         fbe_u32_t bytesRead);

static fbe_status_t fbe_base_env_fup_handle_related_work_item(fbe_base_environment_t * pBaseEnv,
                                       fbe_u64_t deviceType,
                                       fbe_device_physical_location_t * pLocation,
                                       fbe_base_env_fup_completion_status_t completionStatus);

/*!**************************************************************
 * @fn fbe_base_env_fup_cond_handler(fbe_base_environment_t * pBaseEnv,
 *                                    fbe_base_env_fup_transition_func_ptr_t pTransitionFunc,
 *                                    fbe_base_env_fup_work_state_t nextWorkState)
 ***************************************************************
 * @brief
 *   This function is the handler for all the firmware upgrade condition functions.
 *   It gets the first work item in teh queue to call the transition function
 *   to transition to the next work state if the work item is ready to transition.
 *   It clears the current condition if no retries are needed.
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pTransitionFunc - The transition function pointer. 
 * @param nextWorkState - The work state of the work item after the transition.
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 * 
 * @version:
 *  20-Jul-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_fup_cond_handler(fbe_base_environment_t * pBaseEnv,
                               fbe_base_env_fup_transition_func_ptr_t pTransitionFunc,
                               fbe_base_env_fup_work_state_t nextWorkState)                                  
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          readyToTransition = FALSE;
    fbe_bool_t                          retry = FALSE;
    fbe_base_env_fup_image_info_t     * pImageInfo = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    if(pWorkItem != NULL) 
    {
        pImageInfo = fbe_base_env_get_fup_work_item_image_info_ptr(pWorkItem);

        if((pImageInfo != NULL) && 
           (pImageInfo->badImage) &&
           (pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_NO_BAD_IMAGE_CHECK) == 0)
        {
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE);

            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem); 
        }
        else
        {
            fbe_base_env_fup_ready_to_transition(pWorkItem, nextWorkState, &readyToTransition);
            if(readyToTransition)
            {
                status = pTransitionFunc(pBaseEnv, pWorkItem);
    
                if(status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
                {
                    fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
                }
                else if(status != FBE_STATUS_OK)
                {
                    /* Retry is needed. */
                    retry = TRUE;
                }
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                "workState %s, nextWorkState %s, notReadyToTransition, SHOULD NEVER HAPPEN!\n",   
                                fbe_base_env_decode_fup_work_state(pWorkItem->workState),
                                fbe_base_env_decode_fup_work_state(nextWorkState));

                fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_TRANSITION_STATE);

                /* Remove the workItem so that it would not block other workItems in the queue. */
                fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
            }
        }
    }

    if(!retry)
    {
        /* No retry needed. Clear the current condition. */
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_DEBUG_LOW,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, CLR COND\n",   
                    __FUNCTION__);
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) pBaseEnv);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s Failed to clear current condition, status 0x%X.\n",
                                __FUNCTION__, status);        
        }
    }

    return FBE_STATUS_OK;
} // End of function fbe_base_env_fup_cond_handler

/*!**************************************************************
 * @fn fbe_base_env_fup_ready_to_transition
 ***************************************************************
 * @brief
 *  This function checks whether the work item is ready to transition to the
 *  next work state.
 *
 * @param pWorkItem - The pointer to the work item.
 * @param nextWorkState - The next work state that the work item needs to run to.
 * @param pReadyToTransition(OUTPUT) - ready to transition or not.
 *
 * @return fbe_status_t - always return FBE_STATUS_OK
 * 
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/                   
static fbe_status_t 
fbe_base_env_fup_ready_to_transition(fbe_base_env_fup_work_item_t * pWorkItem,
                             fbe_base_env_fup_work_state_t nextWorkState,
                             fbe_bool_t * pReadyToTransition)
{
    if((nextWorkState == FBE_BASE_ENV_FUP_WORK_STATE_NONE) ||
       (nextWorkState == FBE_BASE_ENV_FUP_WORK_STATE_REFRESH_DEVICE_STATUS_DONE) ||
       (nextWorkState == FBE_BASE_ENV_FUP_WORK_STATE_END_UPGRADE_DONE))
    {
        *pReadyToTransition = TRUE;
    }
    else if(pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_ABORT_CMD_SENT) 
    {
        *pReadyToTransition = TRUE;
    }
    else if(pWorkItem->workState == (nextWorkState - 1))
    {
        /* This work item is not in the previous state. */
        *pReadyToTransition = TRUE;
    }
    else
    {
        *pReadyToTransition = FALSE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_wait_before_upgrade_cond_function()
 ****************************************************************
 * @brief
 *  This function would delay the upgrade.
 *  We don't want to do the upgrade during the ICA process.
 *  So adding the delay before the intial upgrade.
 * 
 * @param base_object - This is the object handle, or in our
 * case the base_environment.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  04-Dec-2012:  PHE - Created.
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_wait_before_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t            * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_base_env_fup_work_item_t      * pNextWorkItem = NULL;
    fbe_status_t                        status = FBE_STATUS_OK; 

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "FUP: wait_before_upgrade, entry.\n");

    fbe_base_env_fup_update_wait_before_upgrade_flag(pBaseEnv);

    if(pBaseEnv->waitBeforeUpgrade) 
    {
        /* Clear the current condition to restart the timer. */
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "FUP: wait_before_upgrade can't clear current condition, status: 0x%X\n",
                                    status);
        }
    }
    else
    {
        pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
        pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        while(pWorkItem != NULL) 
        {
            fbe_spinlock_lock(pWorkItemQueueLock);
            pNextWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
            fbe_spinlock_unlock(pWorkItemQueueLock);

            if(fbe_base_env_get_fup_work_item_state(pWorkItem) == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE) 
            {
                fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_QUEUED);
            }

            pWorkItem = pNextWorkItem;
        }

        status = fbe_lifecycle_stop_timer(&fbe_base_environment_lifecycle_const, 
                                          (fbe_base_object_t *)pBaseEnv,
                                          FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "FUP: wait_before_upgrade can't stop timer, status: 0x%X\n",
                                  status);

        }

        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY);
        if(status != FBE_STATUS_OK) 
        {
             fbe_base_object_trace(base_object, 
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO, 
                                   "FUP: wait_before_upgrade, Can't set COND_FUP_READ_IMAGE_HEADER.\n"); 

        }
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}
   
/*!**************************************************************
 * @fn fbe_base_env_fup_update_wait_before_upgrade_flag()
 ****************************************************************
 * @brief
 *  The upgrade needs to wait at boot up.
 *  This function checks whether we still needs to wait for the upgrade or
 *  the wait is done and update the waitBeforeUpgrade flag.
 * 
 * @param pBaseEnv - 
 * @param pStartUpgrade (OUTPUT) - 
 *
 * @return fbe_status_t 
 *
 * @author
 *  04-Dec-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_update_wait_before_upgrade_flag(fbe_base_environment_t * pBaseEnv)
{
    fbe_base_object_t                 * base_object = (fbe_base_object_t *)pBaseEnv;
    fbe_u32_t                           elapsedTimeInSec = 0;

    if(pBaseEnv->waitBeforeUpgrade) 
    {
        if(pBaseEnv->upgradeDelayInSec == 0) 
        {
            /* In simulation, there is no upgrade delay. */
            pBaseEnv->waitBeforeUpgrade = FBE_FALSE;
    
            fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "FUP: wait_before_upgrade, no need to wait.\n");
        }
        else if(pBaseEnv->timeStartToWaitBeforeUpgrade == 0)
        {
            fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "FUP: wait_before_upgrade, Will wait %d seconds before upgrade.\n",
                                pBaseEnv->upgradeDelayInSec);
    
            pBaseEnv->timeStartToWaitBeforeUpgrade = fbe_get_time();
        }
        else
        {
            elapsedTimeInSec = fbe_get_elapsed_seconds(pBaseEnv->timeStartToWaitBeforeUpgrade);

            if(elapsedTimeInSec >= pBaseEnv->upgradeDelayInSec) 
            {
                pBaseEnv->waitBeforeUpgrade = FBE_FALSE;
            
                fbe_base_object_trace(base_object, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "FUP: wait_before_upgrade, Waited %d seconds. Will start upgrade.\n",
                                    elapsedTimeInSec);
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_terminate_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ***************************************************************
 * @brief
 *   This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_TERMINATE_UPGRADE.
 *   It calls the condition function handler to drain all the work items in the queue.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE.
 * 
 * @note 
 *
 * @version:
 *  04-Jan-2013 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_terminate_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)                                  
{
    fbe_base_environment_t            * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_bool_t                          checkQueue = TRUE;
    fbe_status_t                        status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "FUP: %s entry.\n",
                                    __FUNCTION__);

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    while(checkQueue)
    {
        fbe_spinlock_lock(pWorkItemQueueLock);
        if(fbe_queue_is_empty(pWorkItemQueueHead))
        {
            fbe_spinlock_unlock(pWorkItemQueueLock);
            checkQueue = FALSE;
            break;
        }

        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_pop(pWorkItemQueueHead);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        fbe_base_env_set_fup_work_item_completion_status(pWorkItem, FBE_BASE_ENV_FUP_COMPLETION_STATUS_TERMINATED);
        fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_NONE);

        // if peer is waiting for permission, send permission grant now
        status = fbe_base_env_fup_check_peer_pending(pBaseEnv, pWorkItem);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "%s, Failed to check_peer_pending.\n",   
                            __FUNCTION__);
        }
    
        status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "%s, Failed to return local firmware upgrade permission, status 0x%x.\n",   
                            __FUNCTION__, status);
        }
    
        /* Save firmware upgrade info */
        status = fbe_base_env_save_fup_info(pBaseEnv, pWorkItem);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "%s, Failed to save fup info ptr.\n",   
                            __FUNCTION__);
        }
    
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Clearing+Removing workItem %p\n",   
                        __FUNCTION__, pWorkItem);

        fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);

    }

    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE);
    if (status != FBE_STATUS_OK) 
    {
        /* Use of pWorkItem after doing the ex_release will lead to access voilation problems.
         * So changing the trace from customizable_trace to simple object_trace.
         */
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s Can't set FUP_RELEASE_IMAGE condition, status: 0x%x.\n", 
                              __FUNCTION__, status);
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) pBaseEnv);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "FUP: %s Failed to clear current condition, status 0x%X.\n",
                            __FUNCTION__, status);        
    }

    return FBE_LIFECYCLE_STATUS_DONE;

} // End of function fbe_base_env_fup_terminate_upgrade_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_abort_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ***************************************************************
 * @brief
 *   This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE.
 *   It calls the condition function handler to abort the upgrade.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE.
 * 
 * @note 
 *
 * @version:
 *  01-Sept-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_abort_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)                                  
{
    fbe_base_environment_t            * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    /* Originally, all the work items in the queue will be aborted unless 
     * the work item is waiting for the response from the physical package. 
     * However, when the first work item is waiting for the response from the physical package and 
     * the second work item is not waiting for the response from the physical package,
     * the next condition REFRESH_DEVICE_STATUS is set due to the second work item.
     * When REFRESH_DEVICE_STATUS condition gets to run, it processes the first work item
     * and end the upgrade for the first work item. This is not what we want.
     * Therefore, we changed to abort the upgrade for the work items one by one.
     * The second work item will be aborted after the first one gets aborted.
     * The abort upgrade condition can NOT be cleared until the resume command is received.
     */
    
    if(pWorkItem != NULL)
    {
        status = fbe_base_env_fup_abort_upgrade(pBaseEnv, pWorkItem);
        if(status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
        }
    }

    /* Do not need to the clear the abort upgrade condition. 
     * Because if there is a new enclosure added after the 
     * the abort command is recieved, we want to block the upgrade as well.
     * The condition will be cleared when the resume abort command is received. 
     */
    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_abort_cond_function
 
/*!**************************************************************
 * @fn fbe_base_env_fup_wait_for_inter_device_delay_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ***************************************************************
 * @brief
 *   This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY.
 *   It calls the condition function handler to wait for the inter device delay.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE.
 * 
 * @note 
 *
 * @version:
 *  04-Mar-2013 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_wait_for_inter_device_delay_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)                                  
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;


    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_wait_for_inter_device_delay, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_WAIT_FOR_INTER_DEVICE_DELAY_DONE);
    
    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_wait_for_inter_device_delay_cond_function


/*!**************************************************************
 * @fn fbe_base_env_fup_read_image_header_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ***************************************************************
 * @brief
 *   This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER.
 *   It calls the condition function handler to read the image header.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE.
 * 
 * @note 
 *
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_read_image_header_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)                                  
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;


    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_read_image_header, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_READ_IMAGE_HEADER_DONE);
    
    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_read_image_header_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_check_rev_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ***************************************************************
 * @brief
 *   This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV.
 *   It calls the condition function handler to check the image rev.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE.
 * 
 * @note 
 *
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_check_rev_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)                                  
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_check_rev, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_CHECK_REV_DONE);
    
    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_check_rev_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_read_entire_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ***************************************************************
 * @brief
 *   This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE.
 *   It calls the condition function handler to read the entire image.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE.
 * 
 * @note 
 *     The imagePath has to be ready when it is needed to read the image header or the entire image.
 *
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_read_entire_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)                                  
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_read_entire_image, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_READ_ENTIRE_IMAGE_DONE);
        
    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_read_entire_image_cond_function


/*!**************************************************************
 * @fn fbe_base_env_fup_download_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE.
 *  It calls the condition function handler to download image.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_download_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{

    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object; 

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_download_image, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_download_image_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_get_download_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS.
 *  It calls the condition function handler to get download status.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  02-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_get_download_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{

    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object; 

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_get_download_status, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_DONE);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_get_download_status_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_get_peer_permission_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION.
 *  It calls the condition function handler to send the request to get the peer permission.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_get_peer_permission_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object; 

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_get_peer_permission, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function 


/*!**************************************************************
 * @fn fbe_base_env_fup_check_env_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE.
 *  It calls the condition function handler to check the environmental status.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_check_env_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_check_env_status, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_CHECK_ENV_STATUS_DONE);
 
    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_check_env_status_cond_function


/*!**************************************************************
 * @fn fbe_base_env_fup_activate_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief 
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE.
 *  It calls the condition function handler to activate the image.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_activate_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_activate_image, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_activate_image_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_get_activate_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS.
 *  It calls the condition function handler to get download status.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  02-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_get_activate_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{

    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object; 

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                   &fbe_base_env_fup_get_activate_status, 
                                   FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_DONE);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_get_activate_status_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_check_result_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT.
 *  It calls the condition function handler to check the upgrade result. 
 *
  * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  08-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_check_result_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                  &fbe_base_env_fup_check_result, 
                                  FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_check_result_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_refresh_device_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS.
 *  It calls the condition function handler to refresh the device status. 
 *
  * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  02-May-2013:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_refresh_device_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                  &fbe_base_env_fup_refresh_device_status, 
                                  FBE_BASE_ENV_FUP_WORK_STATE_REFRESH_DEVICE_STATUS_DONE);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_refresh_device_status_cond_function

/*!**************************************************************
 * @fn fbe_base_env_fup_end_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets called for the FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE.
 *  It calls the condition function handler to end the upgrade. 
 *  
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet.
 *
 * @return fbe_lifecycle_status_t - always return FBE_LIFECYCLE_STATUS_DONE
 *
 * @version
 *  02-May-2013:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_fup_end_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_env_fup_cond_handler(pBaseEnv, 
                                  &fbe_base_env_fup_end_upgrade, 
                                  FBE_BASE_ENV_FUP_WORK_STATE_END_UPGRADE_DONE);

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_fup_end_upgrade_cond_function


/*!**************************************************************
 * fbe_base_env_fup_release_image_cond_function()
 ****************************************************************
 * @brief
 *  This function releases all the firmware upgrade images.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the PS management's constant
 *                        lifecycle data.
 *
 * @author
 *  12-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t
fbe_base_env_fup_release_image_cond_function(fbe_base_object_t * base_object, 
                                               fbe_packet_t * packet)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n",
                          __FUNCTION__);

    status = fbe_base_env_fup_release_image(pBaseEnv);

    if(status == FBE_STATUS_OK)
    {
        /* No retry needed. Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond(base_object);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s Failed to clear current condition, status 0x%X.\n",
                                __FUNCTION__, status);        
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
 *  @fn fbe_base_env_fup_release_image()
 **************************************************************************
 *  @brief
 *      This function should only be called when there is no work item in the queue.
 *      It releases the memory allocated for the entire image but still keeps
 *      the image header info.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *
 *  @return  fbe_status_t - always return FBE_STATUS_OK 
 * 
 *  @version
 *  16-June-2010 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_fup_release_image(fbe_base_environment_t * pBaseEnv)
{
    fbe_base_env_fup_image_info_t     * pImageInfo = NULL;
    fbe_u32_t                           imageIndex = 0;

    /* Release the memory for the entire image.
     * Do not clear the header related image info. 
     * If this is a bad image, then we can try to avoid the upgrade. 
     */
    for(imageIndex = 0; imageIndex < FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES; imageIndex ++)
    {
        pImageInfo = fbe_base_env_get_fup_image_info_ptr(pBaseEnv, imageIndex);
 
        if(pImageInfo->pImage != NULL)
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "FUP: Releasing Image %p, index %d.\n",
                                  pImageInfo->pImage,
                                  imageIndex);

            fbe_base_env_memory_ex_release(pBaseEnv, pImageInfo->pImage);

            pImageInfo->pImage = NULL;

            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "FUP: Image released, imageRev %s, imageSize 0x%x.\n",
                                  pImageInfo->imageRev,
                                  pImageInfo->imageSize);
        }
    } 
    return FBE_STATUS_OK;
}
  
/*!*************************************************************************
 *  @fn fbe_base_env_fup_get_first_work_item()
 **************************************************************************
 *  @brief
 *  Get the first item on the work queue.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param ppNextWorkItem(OUTPUT) - this is a double pointer that returns the next work item
 *            NULL - queue is empty
 *            Otherwise - the first work item
 * 
 *  @return  fbe_status_t - always return FBE_STATUS_OK.
 * 
 *  @version
 *  14-Feb-2011 GB - Created.
 **************************************************************************/
fbe_status_t fbe_base_env_fup_get_first_work_item(fbe_base_environment_t * pBaseEnv,
                                                  fbe_base_env_fup_work_item_t ** ppReturnWorkItem)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);
    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    *ppReturnWorkItem = pWorkItem;
    return FBE_STATUS_OK;
} //fbe_base_env_fup_get_first_work_item


/*!*************************************************************************
 *  @fn fbe_base_env_fup_get_next_work_item()
 **************************************************************************
 *  @brief
 *  This function will get get the next work item from the work item queue.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param pWorkItem - The starting point or reference work item
 *  @param ppNextWorkItem(OUTPUT) - this is a double pointer that returns the next work item
 *            NULL - there's no next item
 *            Otherwise - the next work item
 * 
 *  @return  fbe_status_t - always return FBE_STATUS_OK.
 * 
 *  @version
 *  14-Feb-2011 GB - Created.
 **************************************************************************/
fbe_status_t fbe_base_env_fup_get_next_work_item(fbe_base_environment_t * pBaseEnv,
                                                 fbe_base_env_fup_work_item_t *pWorkItem,
                                                 fbe_base_env_fup_work_item_t ** ppNextWorkItem)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);
    fbe_spinlock_lock(pWorkItemQueueLock);
    *ppNextWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    return FBE_STATUS_OK;
} //fbe_base_env_fup_get_next_work_item
 
/*!*************************************************************************
 *  @fn fbe_base_env_fup_find_work_item()
 **************************************************************************
 *  @brief
 *      This function is to find the first matching work item in the queue with the 
 *      specified device type and location. It could be possible that there are more than
 *      one matching work item in the queue. But this function is to get the first one.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param deviceType -
 *  @param pLocation -
 *  @param pReturnWorkItemPtr(OUTPUT) - The pointer to the pointer of the return work item.
 *            NULL - The matching item is not found.
 *            Otherwise - The matching item is found.
 * 
 *  @return  fbe_status_t - always return FBE_STATUS_OK.
 * 
 *  @version
 *  16-June-2010 PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_env_fup_find_work_item(fbe_base_environment_t * pBaseEnv,
                                fbe_u64_t deviceType,
                                fbe_device_physical_location_t * pLocation,
                                fbe_base_env_fup_work_item_t ** pReturnWorkItemPtr)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_device_physical_location_t    * pWorkItemLocation = NULL;

    *pReturnWorkItemPtr = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        if((pWorkItem->deviceType == deviceType) &&
           (pWorkItemLocation->bus == pLocation->bus) &&
           (pWorkItemLocation->enclosure == pLocation->enclosure) &&
           (pWorkItemLocation->componentId == pLocation->componentId) &&
           (pWorkItemLocation->slot == pLocation->slot))
        {
            *pReturnWorkItemPtr = pWorkItem;
            break;
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }
  
    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_base_env_fup_find_work_item_with_specific_fw_target()
 **************************************************************************
 *  @brief
 *      This function is to find the first matching work item in the queue with the 
 *      specified device type, location, firmware target. 
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param deviceType -
 *  @param pLocation -
 *  @param firmwareTarget -
 *  @param pReturnWorkItemPtr(OUTPUT) - The pointer to the pointer of the return work item.
 *            NULL - The matching item is not found.
 *            Otherwise - The matching item is found.
 * 
 *  @return  fbe_status_t - always return FBE_STATUS_OK.
 * 
 *  @version
 *  21-Aug-2014 PHE - Created.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_env_fup_find_work_item_with_specific_fw_target(fbe_base_environment_t * pBaseEnv,
                                fbe_u64_t deviceType,
                                fbe_device_physical_location_t * pLocation,
                                fbe_enclosure_fw_target_t firmwareTarget,
                                fbe_base_env_fup_work_item_t ** pReturnWorkItemPtr)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_device_physical_location_t    * pWorkItemLocation = NULL;

    *pReturnWorkItemPtr = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        if((pWorkItem->deviceType == deviceType) &&
           (pWorkItem->firmwareTarget == firmwareTarget) &&
           (pWorkItemLocation->bus == pLocation->bus) &&
           (pWorkItemLocation->enclosure == pLocation->enclosure) &&
           (pWorkItemLocation->componentId == pLocation->componentId) &&
           (pWorkItemLocation->slot == pLocation->slot))
        {
            *pReturnWorkItemPtr = pWorkItem;
            break;
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }
  
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_env_fup_work_item_pointer_is_valid()
 **************************************************************************
 *  @brief
 *      This function is to find whether a work item pointer is valid 
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param pWorkItem - work item pointer
 * 
 *  @return  fbe_bool_t - FBE_TRUE: valid
 * 
 *  @version
 *  10-Aug-2013 Rui Chang - Created.
 *
 **************************************************************************/
fbe_bool_t 
fbe_base_env_fup_work_item_pointer_is_valid(fbe_base_environment_t * pBaseEnv,
                                fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItemTemp = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItemTemp = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItemTemp != NULL) 
    {
        if (pWorkItemTemp == pWorkItem)
        {
            return FBE_TRUE;
        }

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItemTemp = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItemTemp->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }
  
    return FBE_FALSE;
}

/*!*************************************************************************
 *  @fn fbe_base_env_fup_isOkToGrantFupPerm()
 **************************************************************************
 *  @brief
 *  Use the work state to decide if it's ok to grant permission to the peer
 *  to fup.
 * 
 *  Use spid to stagger the decision according to which side is asking. This
 *  should minimize potential for deadlock.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param pWorkItem - the work item that will have the work state checked
 * 
 *  @return  true/false to indicate ok to grant permission
 * 
 *  @version
 *  16-June-2010 PHE - Created.
 *
 **************************************************************************/
fbe_bool_t fbe_base_env_fup_isOkToGrantFupPerm(fbe_base_environment_t *pBaseEnv,
                                             fbe_base_env_fup_work_item_t *pWorkItem)
{
    fbe_bool_t  grant_permission = FALSE;

    if ((pBaseEnv->spid == SP_A) &&
        (pWorkItem->workState < FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED))
    {
        // grant permission if downloading image has not started
        grant_permission = TRUE;
    }
    else if ((pBaseEnv->spid == SP_B) &&
             (pWorkItem->workState < FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_RECEIVED))
    {
        // grant permission if downloading image has not started
        grant_permission = TRUE;
    }
    return(grant_permission);
} //fbe_base_env_fup_isOkToGrantFupPerm

/*!*************************************************************************
 *  @fn fbe_base_env_fup_processRequestForPeerPerm()
 **************************************************************************
 *  @brief
 *  This function is to find the matching work item in the queue with the 
 *  specified location and work state. This function does not compare the
 *  slot field, it only compares the type, bus, and encl.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param deviceType - 
 *  @param pLocation - 
 *  @param pReturnWorkItemPtr(OUTPUT) - The pointer to the pointer of the return work item.
 *            NULL - The matching item is not found.
 *            Otherwise - The matching item is found.
 * 
 *  @return  fbe_status_t - always return FBE_STATUS_OK.
 * 
 *  @version
 *  03-Mar-2011 GB - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_fup_processRequestForPeerPerm(fbe_base_environment_t * pBaseEnv,
                                                        fbe_base_env_fup_cmi_packet_t * pFupCmiPacket)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_device_physical_location_t    * pWorkItemLocation = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          grant_permission;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    // if there is no work item for the requested location, we will
    // grant permission, therefore, set the default to TRUE
    grant_permission = TRUE;
    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        if((pWorkItem->deviceType == pFupCmiPacket->deviceType) &&
           (pWorkItemLocation->bus == pFupCmiPacket->location.bus) &&
           (pWorkItemLocation->enclosure == pFupCmiPacket->location.enclosure))
        {
            // test to grant peer perm
            // check for all work items at this enclosure location, if any
            // can't grant, none can grant
            grant_permission = fbe_base_env_fup_isOkToGrantFupPerm(pBaseEnv,pWorkItem);
            if (!grant_permission)
            {
                break;
            }
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }

    // For now, we only grant or pend peer permission. If case is added to deny
    // peer permission this test needs to be modified.
    if (grant_permission)
    {
        // fyi - for this case there may be no work item, so any reference must be tested first!
        status = fbe_base_env_fup_fill_and_send_cmi(pBaseEnv,
                                                    FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT, 
                                                    pFupCmiPacket->deviceType,
                                                    &pFupCmiPacket->location,
                                                    pFupCmiPacket->pRequestorWorkItem);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),Failed to send peer permission grant msg.\n",
                              fbe_base_env_decode_device_type(pFupCmiPacket->deviceType),
                              pFupCmiPacket->location.bus,
                              pFupCmiPacket->location.enclosure,
                              pFupCmiPacket->location.slot);
        }
    }
    else
    {
        // this case will always have a work item pointer
        pWorkItem->pendingPeerRequest = *pFupCmiPacket;

        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),Pending peer permission for this item on peer.\n",
                              fbe_base_env_decode_device_type(pFupCmiPacket->deviceType),
                              pFupCmiPacket->location.bus,
                              pFupCmiPacket->location.enclosure,
                              pFupCmiPacket->location.slot);


        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                            "Pending reason, Item on local SP:%d workstate:%d.\n",
                                            pBaseEnv->spid,
                                            pWorkItem->workState);
    }

    return status;
} //fbe_base_env_fup_processRequestForPeerPerm

/*!***************************************************************
 * @fn fbe_base_env_fup_get_image_path_from_registry()
 ****************************************************************
 * @brief
 *  This function gets the image path from the registry.
 *  This does not include the image file name.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pImagePathKey - The pointer to the image path key.
 * @param pImagePath (OUTPUT) - The pointer to the returned image path.
 * @param imagePathLength -
 * 
 * @return fbe_status_t  
 *          If the operation failed, return "NULL" as the image path.
 *
 * @version
 *  15-June-2010 PHE - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_base_env_fup_get_image_path_from_registry(fbe_base_environment_t * pBaseEnv, 
                                              fbe_u8_t * pImagePathKey, 
                                              fbe_u8_t * pImagePath,
                                              fbe_u32_t imagePathLength)
{  
    fbe_u8_t defaultImagePath[] = "NULL";
    fbe_status_t status = FBE_STATUS_OK;
    
    fbe_zero_memory(pImagePath, imagePathLength);

    status = fbe_registry_read(NULL,
                      fbe_get_fup_registry_path(),
                      pImagePathKey, 
                      pImagePath,
                      FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH,
                      FBE_REGISTRY_VALUE_SZ,
                      &defaultImagePath,
                      0,
                      FALSE);

    if(status != FBE_STATUS_OK)
    {
        /* 
         * In the fbe_registry_sim.c, the defaultImagePath is not filled in
         * in case of the failure of reading the registry.
         * So, we need to reset it to "NULL".
         */
        fbe_copy_memory(pImagePath, "NULL", 4);
    }

    return status;
}// End of function fbe_base_env_fup_get_image_path_from_registry

/*!**************************************************************
 * @fn fbe_base_env_fup_create_and_add_work_item(fbe_base_environment_t * pBaseEnv, 
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_enclosure_fw_target_t firmwareTarget,
 *                                         fbe_u8_t * pFirmwareRev,
 *                                         fbe_u8_t * pSubenclProductId,
 *                                         fbe_u8_t * pLogHeader,
 *                                         fbe_u32_t interDeviceDelayInSec,
 *                                         fbe_u32_t upgradeRetryCount)
 ****************************************************************
 * @brief
 *  This function allocates the memory for the new work item for the firmware upgrade
 *  and adds it to the tail of the work item queue. If this is the first work item
 *  in the queue and the object state is ready, the first firmware upgrade condition
 *  read image header will be set. 
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param deviceType -
 * @param firmwareTarget -
 * @param pFirmwareRev -
 * @param pSubenclProductId -
 * @param forceFlags -
 * @param pLogHeader -
 * @param interDeviceDelayInSec -
 * @param upgradeRetryCount - The count that the upgrade attempt has been done for this device.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t
fbe_base_env_fup_create_and_add_work_item(fbe_base_environment_t * pBaseEnv,                                           
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_u64_t deviceType,
                                          fbe_enclosure_fw_target_t firmwareTarget,
                                          fbe_char_t * pFirmwareRev,
                                          fbe_char_t * pSubenclProductId,
                                          HW_MODULE_TYPE uniqueId,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount,
                                          fbe_u16_t esesVersion)
{
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_base_env_fup_work_item_t      * pFirstWorkItemInQueue = NULL;
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_lifecycle_state_t               lifecycleState;

    /* Check whether there is already a work item for the device. */
    fbe_base_env_fup_find_work_item_with_specific_fw_target(pBaseEnv, deviceType, pLocation, firmwareTarget, &pWorkItem);

    if(pWorkItem != NULL)
    {
        /* There is already a work item for it. No need to continue. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "Upgrade already started, no need to start again.\n");
        
        return FBE_STATUS_OK;
    }

    pWorkItem = fbe_base_env_memory_ex_allocate(pBaseEnv, sizeof(fbe_base_env_fup_work_item_t));
    if(pWorkItem == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "FUP:deviceType 0x%llX, Loc(%d_%d_%d), fwTarget %d, mem alloc for work item failed.\n",
                                    deviceType,
                                    pLocation->bus,
                                    pLocation->enclosure,
                                    pLocation->slot,
                                    firmwareTarget);
        
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fbe_zero_memory(pWorkItem, sizeof(fbe_base_env_fup_work_item_t));

    pWorkItem->upgradeRetryCount = upgradeRetryCount;
    pWorkItem->deviceType = deviceType;
    pWorkItem->location = *pLocation;
    pWorkItem->firmwareTarget = firmwareTarget;
    pWorkItem->forceFlags = forceFlags;
    pWorkItem->uniqueId = uniqueId;
    pWorkItem->esesVersion = esesVersion;

    if(pBaseEnv->waitBeforeUpgrade) 
    {
        pWorkItem->workState = FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE;
    }
    else
    {
        pWorkItem->workState = FBE_BASE_ENV_FUP_WORK_STATE_QUEUED;
    }
    

    pWorkItem->completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_QUEUED;
    pWorkItem->firmwareStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
    pWorkItem->firmwareExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
    pWorkItem->cmdStartTimeStamp = 0;
    pWorkItem->interDeviceDelayStartTime = 0;
    pWorkItem->interDeviceDelayInSec = interDeviceDelayInSec; // in seconds.

    fbe_copy_memory(&pWorkItem->firmwareRev[0], 
                     pFirmwareRev, 
                     FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    fbe_copy_memory(&pWorkItem->subenclProductId[0], 
                    pSubenclProductId, 
                    FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE);

    fbe_copy_memory(&pWorkItem->imagePath[0], 
                    "NULL", 
                    4);

    fbe_copy_memory(&pWorkItem->logHeader[0], 
                    pLogHeader, 
                    FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);
    
    
    fbe_spinlock_lock(pWorkItemQueueLock);
    fbe_queue_push(pWorkItemQueueHead, (fbe_queue_element_t *)pWorkItem);
    pFirstWorkItemInQueue = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    /* Trace and log the workItem is queued. */
    fbe_base_env_fup_log_event(pBaseEnv, pWorkItem);

    /* Save the persistent FUP info. */
    status = fbe_base_env_save_fup_info(pBaseEnv, pWorkItem);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s Failed to save persistent fup info status 0x%X.\n",   
                        __FUNCTION__, status);  
        
        fbe_spinlock_lock(pWorkItemQueueLock);
        fbe_queue_remove((fbe_queue_element_t *)pWorkItem);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);
        return status;
    } 
    
    if(pFirstWorkItemInQueue == pWorkItem)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, It is the first item in queue!\n",   
                        __FUNCTION__);  

        /* Do not set FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE if we are not in the ready state yet.
         * When we transition to the ready state, 
         * FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE is the preset condition.
         */
        status = fbe_lifecycle_get_state(&fbe_base_environment_lifecycle_const, 
                                             (fbe_base_object_t*)pBaseEnv, 
                                             &lifecycleState);
    
        if((status == FBE_STATUS_OK) &&
           (lifecycleState == FBE_LIFECYCLE_STATE_READY))
        {
            status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_ERROR, 
                                           fbe_base_env_get_fup_work_item_logheader(pWorkItem), 
                                           "Can't set COND_FUP_WAIT_BEFORE_UPGRADE, dequeue workItem %p.\n",
                                           pWorkItem); 
    
                fbe_spinlock_lock(pWorkItemQueueLock);
                fbe_queue_remove((fbe_queue_element_t *)pWorkItem);
                fbe_spinlock_unlock(pWorkItemQueueLock);
    
                fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);
                return status;
            }
        }
    }
    return status;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_post_completion(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function is called after the upgrade has the completion status set.
 *  It sets the condition to refresh the device status.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_fup_post_completion(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pFirstWorkItemInQueue = NULL;

    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);
    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pFirstWorkItemInQueue = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    if(pFirstWorkItemInQueue == pWorkItem)
    {
        /* This is the first work item in the queue. 
         * It could be possible that the status was ignored when doing the activation.
         * So setting the FUP_REFRESH_DEVICE_STATUS condition to fresh the device status.
         */

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "First work item in queue,refresh device status,then end upgrade.\n");

        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "%s, Can't set FUP_REFRESH_DEVICE_STATUS condition, status: 0x%x.\n",
                                                __FUNCTION__, status);
        }
    }
    else
    {
        /* This is NOT the first work item in the queue.
         * Call the function fbe_base_env_fup_end_upgrade directly instead of setting the 
         * condition to removal the work item in the queue. 
         * If we set the END_UPGRADE condition to remove the work, 
         * it would impact the state of the work item because our conditions would process the first work item
         * in the queue.
         */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "NOT the first work item in queue, end upgrade directly.\n");

        status = fbe_base_env_fup_end_upgrade(pBaseEnv, pWorkItem);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "%s, Failed to send end of upgrade msg to peer SP.\n",   
                __FUNCTION__);
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_end_upgrade(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function is called after the upgrade ends for the work item.
 *  It logs the event. If retry is needed, it resets the work item for retry.
 *  Otherwise, it removes the work item from the fup work item queue.
 *  and releases the memory allocated for the work item.
 *  The new condition will be set to process the next work item while
 *  removing the work item or reset the workitem if needed.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item to be removed.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_fup_end_upgrade(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_bool_t                           retry = FALSE;
    fbe_bool_t                           upgradeSucceeded = FALSE;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_u64_t                            deviceType;
    fbe_device_physical_location_t       location = {0};
    fbe_base_env_fup_completion_status_t completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_NONE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    /* Send the CMI message to peer SP to let peer SP the end of upgrade. 
     * The peer SP needs this info so that it does not need to suppress the device fault.
     * If the upgrade was aborted, we would still come here.
     */
    status = fbe_base_env_fup_fill_and_send_cmi(pBaseEnv,
                                                FBE_BASE_ENV_CMI_MSG_FUP_PEER_UPGRADE_END, 
                                                pWorkItem->deviceType,
                                                &pWorkItem->location,
                                                pWorkItem);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "%s, Failed to send end of upgrade msg to peer SP.\n",   
                __FUNCTION__);
    }

    /* Upgrade completed. Trace and log event. */
    fbe_base_env_fup_log_event(pBaseEnv, pWorkItem);

    fbe_base_env_fup_retry_needed(pBaseEnv, pWorkItem, &retry);

    if(retry)
    {
        // if peer is waiting for permission, send permission grant now
        status = fbe_base_env_fup_check_peer_pending(pBaseEnv, pWorkItem);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "%s, Failed to check_peer_pending.\n",   
                            __FUNCTION__);
        }
        else 
        {
            fbe_zero_memory(&(pWorkItem->pendingPeerRequest), sizeof(fbe_base_env_fup_cmi_packet_t));
        }

        status = fbe_base_env_fup_reset_work_item_for_retry(pBaseEnv, pWorkItem);
        if(status == FBE_STATUS_OK)
        {
            return status;
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "reset_work_item_for_retry failed, upgradeRetryCount %d, status 0x%X.\n",   
                            pWorkItem->upgradeRetryCount, status);  
        }
    }

    fbe_base_env_fup_succeeded(pBaseEnv, pWorkItem, &upgradeSucceeded);
    if(upgradeSucceeded)
    {
        status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                "Failed to send completion notification.\n");
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "NOTIFY COMPLETION cmd was sent successfully.\n");
        }

        fbe_base_env_fup_remove_and_release_work_item(pBaseEnv, pWorkItem);
    }
    else if((pWorkItem->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_ACTIVATION_DEFERRED) ||
            (pWorkItem->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED) ||
            (pWorkItem->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED))
    {
        fbe_base_env_fup_remove_and_release_work_item(pBaseEnv, pWorkItem);
    }
    else 
    {
        /* Save some info about this work item before releasing it so that it can be used in 
         * fbe_base_env_fup_handle_related_work_item
         */
        deviceType = pWorkItem->deviceType;
        location = pWorkItem->location;
        completionStatus = pWorkItem->completionStatus; 

        fbe_base_env_fup_remove_and_release_work_item(pBaseEnv, pWorkItem);

        fbe_base_env_fup_handle_related_work_item(pBaseEnv, deviceType, &location, completionStatus);
    }
       
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_retry_needed(fbe_base_environment_t * pBaseEnv,
 *                                    fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks the work item completion status to determine
 *  if retry is desired.This will retry the entire sequence from
 *  work state FBE_BASE_ENV_FUP_WORK_STATE_QUEUED.
 * 
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 * @param pRetry(OUT) - The pointer to the returned value.
 * 
 * @return fbe_status_t
 *
 * @version
 *  13-Oct-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t
fbe_base_env_fup_retry_needed(fbe_base_environment_t * pBaseEnv,
                              fbe_base_env_fup_work_item_t * pWorkItem,
                              fbe_bool_t * pRetry)
{
    fbe_base_env_fup_image_info_t * pImageInfo = NULL;

    pImageInfo = fbe_base_env_get_fup_work_item_image_info_ptr(pWorkItem);

    /* Init to FALSE. */
    *pRetry = FALSE;
    switch(pWorkItem->completionStatus)
    {
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_DOWNLOAD_IMAGE:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE:
            /* Do not mark the bad image for the download image failure. 
             * It could be possible that the failure was caused by the 
             * individual enclosure, not the image itself.
             */
            /* Do not mark the bad image for the activate image failure.
             * We can get the FAIL_TO_ACTIVATE_IMAGE status when SPS firmware upgrade was interrupt.
             * It does not necessarily mean the image is bad.
             * It is too agressive to mark the bad image when 
             * we get the FAIL_TO_ACTIVATE_IMAGE status
             * Do the retry for FAIL_TO_ACTIVATE_IMAGE. 
             * Due to OPT453757, the activation could fail with the download status 0xf0(activation failed) and 
             * the additional status 0x7(micro update failed). The upgrade might be successful after retries. 
             * So we do the retry here.
             */
            if(pWorkItem->upgradeRetryCount < FBE_BASE_ENV_FUP_MAX_RETRY_COUNT)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "Will retry upgrade, upgradeRetryCount %d, maxRetryCount %d.\n",   
                            pWorkItem->upgradeRetryCount, FBE_BASE_ENV_FUP_MAX_RETRY_COUNT); 

                *pRetry = TRUE;
            }
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE:
            if(pImageInfo != NULL) 
            {
                pImageInfo->badImage = TRUE;
            }
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED:      
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_NO_REV_CHANGE:
            if(pImageInfo != NULL) 
            {
                pImageInfo->badImage = FALSE;
            }
            break;
        
        default:
            break;
    }
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_reset_work_item_for_retry(fbe_base_environment_t * pBaseEnv,
 *                                    fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function resets the work item for retry.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  13-Oct-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t
fbe_base_env_fup_reset_work_item_for_retry(fbe_base_environment_t * pBaseEnv,
                                fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_lifecycle_state_t   lifecycleState;

    pWorkItem->upgradeRetryCount ++;
    pWorkItem->pImageInfo = NULL;
    pWorkItem->workState = FBE_BASE_ENV_FUP_WORK_STATE_QUEUED;
    pWorkItem->completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_QUEUED;
    pWorkItem->firmwareStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
    pWorkItem->firmwareExtStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
    pWorkItem->cmdStartTimeStamp = 0;
    pWorkItem->interDeviceDelayStartTime = 0;

    fbe_zero_memory(&pWorkItem->newFirmwareRev[0], 
                     FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    /* Trace and log the event. */
    fbe_base_env_fup_log_event(pBaseEnv, pWorkItem);

    /* Do not initiate the upgrade right away if we are not in the ready state yet.
     * When we transition to the ready state.
     */
    status = fbe_lifecycle_get_state(&fbe_base_environment_lifecycle_const, 
                                         (fbe_base_object_t*)pBaseEnv, 
                                         &lifecycleState);

    if((status == FBE_STATUS_OK) &&
       (lifecycleState == FBE_LIFECYCLE_STATE_READY))
    {
        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                "FUP: %s Can't set FUP_READ_IMAGE_HEADER condition, status: 0x%x.\n",   
                            __FUNCTION__, status); 
        }
    } 
    
    return status;
} //fbe_base_env_fup_reset_work_item_for_retry

/*!**************************************************************
 * @fn fbe_base_env_fup_succeeded(fbe_base_environment_t * pBaseEnv,
 *                                    fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks whether the upgrade is successful.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 * @param pUpgradeSucceeded(OUT) - The pointer to the returned value.
 * 
 * @return fbe_status_t
 *
 * @version
 *  13-Oct-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_succeeded(fbe_base_environment_t * pBaseEnv,
                                               fbe_base_env_fup_work_item_t * pWorkItem,
                                               fbe_bool_t * pUpgradeSucceeded)
{
    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n",
                          __FUNCTION__);

    switch(pWorkItem->completionStatus)
    {
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED:      
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_NO_REV_CHANGE:   
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_EXIT_UP_TO_REV:
            *pUpgradeSucceeded = TRUE;
            break;

        default:
            *pUpgradeSucceeded = FALSE;
            break;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_save_fup_info(fbe_base_environment_t * pBaseEnv, 
 *                                fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The work item
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_save_fup_info(fbe_base_environment_t * pBaseEnv, 
                                        fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_base_env_fup_persistent_info_t  * pFupInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_class_id_t                        classId;
    fbe_handle_t                          object_handle;

    if(pBaseEnv->fup_element_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s fup element ptr NULL\n", 
                              __FUNCTION__);
        return status;
    }
    if(pBaseEnv->fup_element_ptr->pGetFupInfoPtrCallback == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s GetFupInfoPtrCallback NULL\n", 
                              __FUNCTION__);
        return status;
    }

    status = pBaseEnv->fup_element_ptr->pGetFupInfoPtrCallback(pBaseEnv, 
                                                               pWorkItem->deviceType,
                                                               &pWorkItem->location,
                                                               &pFupInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to get fup info ptr.\n",   
                        __FUNCTION__);
        return status;
    }
    
    /* Save the persistent firmware upgrade info. */
    
    /* Save the completion status. */
    pFupInfo->completionStatus = pWorkItem->completionStatus;
    
    /* Save the forceFlags so that the upgrade can be resumed with
     * the original forceFlags if the upgrade was aborted 
     */
    pFupInfo->forceFlags = pWorkItem->forceFlags;
    
    if(pWorkItem->completionStatus <= FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS) 
    {
        /* Save the pointer to the work item. */
        pFupInfo->pWorkItem = pWorkItem;
    }
    else
    {
        /* The upgrade completed. The workitem will be freed. So we set it to NULL. */
        pFupInfo->pWorkItem = NULL;
    }

    if((pWorkItem->firmwareTarget != FBE_FW_TARGET_LCC_FPGA) &&
       (pWorkItem->firmwareTarget != FBE_FW_TARGET_LCC_INIT_STRING))
    {
    
        if (pWorkItem->pImageInfo != NULL) 
        {
            /* Save the image file Rev */
            fbe_copy_memory(&pFupInfo->imageRev[0], 
                            &pWorkItem->pImageInfo->imageRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        }
        else
        {
            fbe_zero_memory(&pFupInfo->imageRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
        }

        /* Save the current HW image Rev */
        if(pWorkItem->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED)
        {
            fbe_copy_memory(&pFupInfo->currentFirmwareRev[0], 
                            &pWorkItem->newFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
        }
        else
        {
            fbe_copy_memory(&pFupInfo->currentFirmwareRev[0], 
                            &pWorkItem->firmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
        }

        /* Save the previous HW image Rev */    
        fbe_copy_memory(&pFupInfo->preFirmwareRev[0], 
                        &pWorkItem->firmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
    }

    object_handle = fbe_base_pointer_to_handle((fbe_base_t *) pBaseEnv);
    classId = fbe_base_object_get_class_id(object_handle);

    /* Send notification for data change */
    fbe_base_environment_send_data_change_notification(pBaseEnv, 
                                           classId, 
                                           pWorkItem->deviceType,
                                           FBE_DEVICE_DATA_TYPE_FUP_INFO,
                                           &pWorkItem->location);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_remove_and_release_work_item(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function removes the work item from the fup work item queue.
 *  and release the memory allocated for the work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item to be removed.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t 
fbe_base_env_fup_remove_and_release_work_item(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_queue_head_t                    * pWorkItemQueueHead = NULL;
    fbe_spinlock_t                      * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t        * pFirstWorkItemInQueue = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
  
    // if peer is waiting for permission, send permission grant now
    status = fbe_base_env_fup_check_peer_pending(pBaseEnv, pWorkItem);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to check_peer_pending.\n",   
                        __FUNCTION__);
    }

    status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to return local firmware upgrade permission, status 0x%x.\n",   
                        __FUNCTION__, status);
    }

    /* Save firmware upgrade info */
    status = fbe_base_env_save_fup_info(pBaseEnv, pWorkItem);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to save fup info ptr.\n",   
                        __FUNCTION__);
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Clearing+Removing workItem %p\n",   
                    pWorkItem);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, waitBeforeUpgrade %d.\n",   
                    __FUNCTION__, pBaseEnv->waitBeforeUpgrade);

    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);
    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pFirstWorkItemInQueue = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_queue_remove((fbe_queue_element_t *)pWorkItem);
   
    /* Check whether there are any work items in the queue.
     * If the queue is empty, it is time to release the images.
     */
    if(fbe_queue_is_empty(pWorkItemQueueHead))
    {
        fbe_spinlock_unlock(pWorkItemQueueLock);

        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE);
        if (status != FBE_STATUS_OK) 
        {
            /* Use of pWorkItem after doing the ex_release will lead to access voilation problems.
             * So changing the trace from customizable_trace to simple object_trace.
             */
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "FUP: %s Can't set FUP_RELEASE_IMAGE condition, status: 0x%x.\n", 
                                  __FUNCTION__, status);
        }
    }
    else 
    {
        fbe_spinlock_unlock(pWorkItemQueueLock);

        if((pFirstWorkItemInQueue == pWorkItem) && (!pBaseEnv->waitBeforeUpgrade))
        {
            /* Only sets FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY 
             * when the work item was the first work item in the queue. 
             * If the work item was removed due to the removal of the devices or 
             * the removal of the containing devices when it is not the first work item
             * in the queue, there is no need to set the condition. 
             * Because only the first item work in the queue is upgrade in progress. 
             * We don't need to set FUP_WAIT_FOR_INTER_DEVICE_DELAY
             * condition here if pBaseEnv->waitBeforeUpgrade is TRUE.
             * If pBaseEnv->waitBeforeUpgrade is TRUE, FUP_WAIT_FOR_INTER_DEVICE_DELAY condition
             * will be set when the wait finishes. 
             */
        
            status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY);
    
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "FUP: %s Can't set FUP_WAIT_FOR_INTER_DEVICE_DELAY condition, status: 0x%x.\n",
                                        __FUNCTION__, status);
            }
        }
    }
   
    fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);
        
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_log_event(fbe_base_environment_t * pBaseEnv,
 *                                fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function logs the firmware upgrade event based on the
 *  completion status.
 * 
 * @param pBaseEnv -
 * @param pWorkItem - The pointer to the work item to be removed.
 *
 * @return fbe_status_t - always return FBE_STATUS_OK
 *
 * @version
 *  08-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_fup_log_event(fbe_base_environment_t * pBaseEnv,
                           fbe_base_env_fup_work_item_t * pWorkItem)
{
    char deviceStr[FBE_DEVICE_STRING_LENGTH + 1] = {0};
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t   retry = FALSE;

    if(pWorkItem->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_QUEUED)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Upgrade Queued, FwRev %s.\n",   
                        pWorkItem->firmwareRev);

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "workItem %p,Pid %s,Flags0x%x,Retry %d.\n",   
                        pWorkItem,
                        pWorkItem->subenclProductId,
                        pWorkItem->forceFlags,
                        pWorkItem->upgradeRetryCount);

    }
    else if(pWorkItem->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Upgrade Started, FwRev %s.\n",  
                        pWorkItem->firmwareRev);

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "workItem %p, Pid %s,Flags0x%x,Retry %d.\n",  
                        pWorkItem,
                        pWorkItem->subenclProductId,
                        pWorkItem->forceFlags,
                        pWorkItem->upgradeRetryCount);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Upgrade Completed,workItem %p,compStatus %s,Retry %d.\n", 
                        pWorkItem,
                        fbe_base_env_decode_fup_completion_status(pWorkItem->completionStatus),
                        pWorkItem->upgradeRetryCount);

    }

    status = fbe_base_env_create_device_string(pWorkItem->deviceType, 
                                               &pWorkItem->location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to create device string, status 0x%x.\n",   
                        __FUNCTION__,
                        status); 
        return status;
    }

    if((pWorkItem->deviceType == FBE_DEVICE_TYPE_LCC) || 
       (pWorkItem->deviceType == FBE_DEVICE_TYPE_SP) ||
       (pWorkItem->deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE))
    {
        fbe_sprintf(&deviceStr[0], 
                (FBE_DEVICE_STRING_LENGTH), 
                "%s %s", 
                &deviceStr[0],
                fbe_base_env_decode_firmware_target(pWorkItem->firmwareTarget));
    }
    
    switch(pWorkItem->completionStatus)
    {
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_QUEUED:
            fbe_event_log_write(ESP_INFO_FUP_QUEUED,
                            NULL, 0,
                            "%s %s %s", 
                            &deviceStr[0],
                            pWorkItem->firmwareRev,
                            pWorkItem->subenclProductId);
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS:
            fbe_event_log_write(ESP_INFO_FUP_STARTED,
                            NULL, 0,
                            "%s %s %s", 
                            &deviceStr[0],
                            pWorkItem->firmwareRev,
                            pWorkItem->subenclProductId);
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED:      
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_NO_REV_CHANGE:
            fbe_base_env_fup_fwUpdatedFailed(pBaseEnv,
                                             pWorkItem->deviceType, 
                                             &pWorkItem->location, 
                                             FALSE);
            fbe_event_log_write(ESP_INFO_FUP_SUCCEEDED,
                            NULL, 0,
                            "%s %s %s", 
                            &deviceStr[0],
                            pWorkItem->newFirmwareRev,
                            pWorkItem->firmwareRev);
            break;
               
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_EXIT_UP_TO_REV:
            fbe_event_log_write(ESP_INFO_FUP_UP_TO_REV,
                            NULL, 0,
                            "%s %s", 
                            &deviceStr[0],
                            pWorkItem->firmwareRev);
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED:
            fbe_event_log_write(ESP_INFO_FUP_ABORTED,
                                NULL, 0,
                                "%s %s %s", 
                                &deviceStr[0],
                                pWorkItem->newFirmwareRev,
                                pWorkItem->firmwareRev);
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_TERMINATED:
            fbe_event_log_write(ESP_INFO_FUP_TERMINATED,
                                NULL, 0,
                                "%s %s %s", 
                                &deviceStr[0],
                                pWorkItem->newFirmwareRev,
                                pWorkItem->firmwareRev);
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_ACTIVATION_DEFERRED:
            fbe_event_log_write(ESP_INFO_FUP_ACTIVATION_DEFERRED,
                                NULL, 0,
                                "%s %s %s", 
                                &deviceStr[0],
                                pWorkItem->firmwareRev,
                                pWorkItem->subenclProductId);
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NULL_IMAGE_PATH:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_REG_IMAGE_PATH:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_REV_CHANGE:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_IMAGE_HEADER:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_PARSE_IMAGE_HEADER:  
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_ENTIRE_IMAGE:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE: 
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_MISMATCHED_IMAGE: 
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_IMAGE_TYPES_TOO_SMALL:           
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DOWNLOAD_CMD_DENIED:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_DOWNLOAD_IMAGE:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_DOWNLOAD_STATUS:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_ACTIVATE_STATUS:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED:
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED:
            // only report FUP Failure if retries exhausted
            fbe_base_env_fup_retry_needed(pBaseEnv, pWorkItem, &retry);
            if (!retry)
            {
                if((pWorkItem->completionStatus != FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED) &&
                    (pWorkItem->completionStatus != FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED))
                {
                    fbe_base_env_fup_fwUpdatedFailed(pBaseEnv,
                                                     pWorkItem->deviceType, 
                                                     &pWorkItem->location, 
                                                     TRUE);
                }
                // ESP_STAND_ALONE_ALERT - remove when CP support available
                fbe_event_log_write(ESP_ERROR_FUP_FAILED,
                                NULL, 0,
                                "%s %s %s", 
                                &deviceStr[0],
                                pWorkItem->firmwareRev,
                                fbe_base_env_decode_fup_completion_status(pWorkItem->completionStatus));
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_INFO,
                                                   fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                   "%s, %d_%d %s failed, retry.\n",   
                                                   __FUNCTION__,
                                                   pWorkItem->location.bus,
                                                   pWorkItem->location.enclosure,
                                                   &deviceStr[0]);
            }
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION:
            fbe_event_log_write(ESP_INFO_FUP_WAIT_PEER_PERMISSION,
                            NULL, 0,
                            "%s %s", 
                            &deviceStr[0],
                            pWorkItem->firmwareRev);
            break;

        default:
            break;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_handle_device_removal(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_u64_t deviceType,
 *                                      fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets call when the device gets removed.
 *  It checks whether there is any upgrade in progress for this device.
 *  If yes, removes and releases the work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param deviceType - 
 * @param pLocation -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_fup_handle_device_removal(fbe_base_environment_t * pBaseEnv, 
                                       fbe_u64_t deviceType,
                                       fbe_device_physical_location_t * pLocation)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_device_physical_location_t    * pWorkItemLocation = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        if((pWorkItem->deviceType == deviceType) &&
           (pWorkItemLocation->bus == pLocation->bus) &&
           (pWorkItemLocation->enclosure == pLocation->enclosure) &&
           (pWorkItemLocation->componentId == pLocation->componentId) &&
           (pWorkItemLocation->slot == pLocation->slot))
        {
            /* There is fup work item for this device. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "Device Removed, will remove and release the work item.\n");
    
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED);
    
            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);

            break;
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }
  
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_env_fup_destroy_queue()
 **************************************************************************
 *  @brief
 *      This function releases all the work items in the fup work item queue
 *      and then destroys the work item queue and spinlock. 
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *
 *  @return  fbe_status_t - always return FBE_STATUS_OK 
 * 
 *  @version
 *  12-Jul-2010 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_fup_destroy_queue(fbe_base_environment_t * pBaseEnv)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "FUP: %s entry.\n",
                                    __FUNCTION__);

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    while(!fbe_queue_is_empty(pWorkItemQueueHead))
    {
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_pop(pWorkItemQueueHead);
        fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);
    }

    fbe_spinlock_unlock(pWorkItemQueueLock);

    fbe_queue_destroy(pWorkItemQueueHead);
    fbe_spinlock_destroy(pWorkItemQueueLock);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_abort_upgrade
 ***************************************************************
 * @brief
 *  This function checks whether it is ok to abort the upgrade for 
 *  the specified work item. If the work item is in the middle of
 *  downloading image or activating the image, the upgrade for the
 *  work item will be aborted after the download or the activation
 *  is done.
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 * 
 * @version:
 *  01-Sept-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_abort_upgrade(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem)
{ 
    fbe_status_t status = FBE_STATUS_OK;

    if((pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT) ||
       (pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_DONE))
    { 
        /* The work item is in the middle of downloading image
         * OR the image was downloaded successfully and it is waiting for the activate command. 
         * Send the abort command.
         * The reason that we have to send the abort for the case that the image finished downloading
         * is that we want to the fw status to be set correctly to abort so that the next download
         * could be done after the abort is resumed and the new upgrade is initiated.
         *
         * If the abort command was sent successfully, update the work state 
         * so that it won't retry sending the abort command.
         * The upgrade for the work item will be aborted after the download is aborted. 
         *
         * If the abort command was NOT sent successfully, update the work state 
         * so that it won't retry sending the abort command.
         * The upgrade for the work item will be aborted after the download completes. 
         */
        status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_ABORT);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Failed to send ABORT cmd, will abort after dnld completes, status 0x%x.\n",   
                        status); 
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "ABORT cmd was sent successfully.\n");
        }

        fbe_base_env_set_fup_work_item_state(pWorkItem,
                                             FBE_BASE_ENV_FUP_WORK_STATE_ABORT_CMD_SENT);
        status = FBE_STATUS_OK;
    }
    else if((pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT) ||
            (pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_ABORT_CMD_SENT))
    {
        /* The work item is in the middle of activating image. 
         * There is nothing we can do.
         * Just return FBE_STATUS_OK.
         * The upgrade for the work item will be aborted after the activation is done. 
         */
        /* The work item is in the middle of aborting in the lower level. 
         * There is nothing we can do.
         * Just return FBE_STATUS_OK.
         * The upgrade for the work item will be aborted in ESP after the abort in the lower level completes. 
         */
        status = FBE_STATUS_OK;
    }
    else
    {
        /* It is safe to set the completion status to ARBORTED here. */
        fbe_base_env_set_fup_work_item_completion_status(pWorkItem,
                                                 FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED);

        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status; 
} //End of function fbe_base_env_fup_abort
 
 
/*!**************************************************************
 * @fn fbe_base_env_fup_wait_for_inter_device_delay
 ***************************************************************
 * @brief
 *  This function checks whether the delay is done. If not, continue to wait
 *  by returning the FBE_STATUS_GENERIC_FAILURE so that the condition can be retried.
 *  If yet, go ahead to set the next condition. 
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 * 
 * @note 
 *     The imagePath has to be ready when it is needed to read the image header or the entire image.
 *
 * @version:
 *  04-Mar-2013 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_wait_for_inter_device_delay(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem)
{ 
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              setNextCondition = FBE_FALSE;
    fbe_u32_t                               elapsedTimeInSec = 0;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "wait_for_inter_device_delay, entry.\n");

    /* We need to check what is the workItem's priority for some device type. 
     * Does it need to wait for some other workItem to go first?
     * If yes, remove the workItem and push it to the tail of the queue to 
     * let other workItem to go first.
     */
    status = fbe_base_env_fup_check_priority(pBaseEnv, pWorkItem);

    if(status != FBE_STATUS_OK) 
    {
        /* Return FBE_STATUS_GENERIC_FAILURE so that the condition would not be cleared
         * and the next workItem will come to this function again.
         */ 
         return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(pWorkItem->interDeviceDelayInSec == 0)
    {
        setNextCondition = FBE_TRUE;

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "wait_for_inter_device_delay, no need to wait.\n");
    }
    else if(pWorkItem->interDeviceDelayStartTime == 0) 
    {
        pWorkItem->interDeviceDelayStartTime = fbe_get_time();
        setNextCondition = FBE_FALSE;

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "wait_for_inter_device_delay, START, will wait %d seconds.\n",   
                        pWorkItem->interDeviceDelayInSec);
    }
    else 
    {
        elapsedTimeInSec = fbe_get_elapsed_seconds(pWorkItem->interDeviceDelayStartTime);
        if(elapsedTimeInSec >= pWorkItem->interDeviceDelayInSec) 
        {
            setNextCondition = FBE_TRUE;
            pWorkItem->interDeviceDelayStartTime = 0;

            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "wait_for_inter_device_delay, END, waited %d seconds.\n",   
                        elapsedTimeInSec);

        }
        else
        {
            setNextCondition = FBE_FALSE;
        }
    }
    
    if(setNextCondition) 
    {
        fbe_base_env_set_fup_work_item_state(pWorkItem, 
                            FBE_BASE_ENV_FUP_WORK_STATE_WAIT_FOR_INTER_DEVICE_DELAY_DONE);

        /* Set the condition to read the image header. */
        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "FUP: %s Can't set FUP_READ_IMAGE_HEADER condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
        }
    }
    else
    {
        /* Set the status so that the condition can be retried. */
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
    
} //End of function fbe_base_env_fup_read_image_header
  
/*!**************************************************************
 * @fn fbe_base_env_fup_check_priority
 ***************************************************************
 * @brief
 *  This function checks the priority of the work item.
 *  If this work item needs to wait for other item to go first,
 *  push it to the tail of the queue. 
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 * 
 * @note     
 *
 * @version:
 *  05-Mar-2013 PHE - Created. 
 *
 ****************************************************************/ 
static fbe_status_t fbe_base_env_fup_check_priority(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_queue_head_t                      * pWorkItemQueueHead = NULL;
    fbe_spinlock_t                        * pWorkItemQueueLock = NULL;
    fbe_bool_t                              needToWait = FALSE;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_base_env_fup_work_item_t          * pTempWorkItem = NULL;
           
    if(pBaseEnv->fup_element_ptr->pCheckPriorityCallback != NULL) 
    {
        status = pBaseEnv->fup_element_ptr->pCheckPriorityCallback(pBaseEnv, pWorkItem, &needToWait);
        
        if(needToWait)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                               "%s, wait for other device to upgrade first!\n",
                                               __FUNCTION__);
            /* Remove the workItem from the queue and push it to the tail of the queue.*/
            pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
            pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

            fbe_spinlock_lock(pWorkItemQueueLock);
            pTempWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
            fbe_spinlock_unlock(pWorkItemQueueLock);

            while(pTempWorkItem != NULL) 
            {
                if((pTempWorkItem->deviceType == pWorkItem->deviceType) &&
                   (pTempWorkItem->location.bus == pWorkItem->location.bus) &&
                   (pTempWorkItem->location.enclosure == pWorkItem->location.enclosure) &&
                   (pTempWorkItem->location.componentId == pWorkItem->location.componentId) &&
                   (pTempWorkItem->location.slot == pWorkItem->location.slot))
                {
                    /* There is fup work item for this device. */
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                        FBE_TRACE_LEVEL_INFO,
                                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                        "move work item to the end of queue.\n");
            
                    fbe_spinlock_lock(pWorkItemQueueLock);
                    fbe_queue_remove((fbe_queue_element_t *)pTempWorkItem);
                    fbe_queue_push(pWorkItemQueueHead, (fbe_queue_element_t *)pTempWorkItem);
                    fbe_spinlock_unlock(pWorkItemQueueLock);
                }      

                fbe_spinlock_lock(pWorkItemQueueLock);
                pTempWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
                fbe_spinlock_unlock(pWorkItemQueueLock);
            }
            

            /* Return FBE_STATUS_GENERIC_FAILURE so that the condition would not be cleared
             * and the next workItem will start the upgrade by reading the image header.
             */ 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_read_image_header
 ***************************************************************
 * @brief
 *  This function checks whether the image header is already in memory
 *  If not, read the image header and parse the image header to see 
 *  whether the image applies to the hardware.
 *  If not, set the appropriate return remove the work item from the queue.
 *  If yes, set the condition to check rev.
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 * 
 * @note 
 *     The imagePath has to be ready when it is needed to read the image header or the entire image.
 *
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_read_image_header(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem)
{ 
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u8_t                              * pImageHeader = NULL;
    fbe_base_env_fup_image_info_t         * pImageInfo = NULL;
    fbe_enclosure_mgmt_parse_image_header_t parseImageHeader = {0};
    fbe_u32_t                               index = 0;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    /* The first step to start the upgrade. 
     * Update the completionStatus to FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS.
     */
    fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                               FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS);

    /* Trace and log the event to indicate the upgrade has started for this workItem. */
    fbe_base_env_fup_log_event(pBaseEnv, pWorkItem);


    status = pBaseEnv->fup_element_ptr->pGetFullImagePathCallback(pBaseEnv, pWorkItem);

    if(status == FBE_STATUS_NOT_INITIALIZED)
    {
        /* The image path in the registry is NULL. 
         * Can not proceed. 
         */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Image path in registry is NULL.\n",   
                        __FUNCTION__);

        fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NULL_IMAGE_PATH); 

        /* The work item needs to be removed from the queue. */                                            
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else if(status != FBE_STATUS_OK)
    {
        /* Reading the registry for the image path failed. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Read registry image path failed, status 0x%x.\n",   
                        __FUNCTION__, status);

        
        fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_REG_IMAGE_PATH); 

        /* The work item needs to be removed from the queue. */                                            
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Check whether the image header info is in memory. 
     * If it is in memory, get the pointer to the image info 
     * for the work item (pWorkItem->pImageInfo). 
     */
    status = fbe_base_env_fup_get_work_item_image_header_info_in_mem(pBaseEnv, pWorkItem);

    if((status != FBE_STATUS_OK) ||
       (pWorkItem->pImageInfo == NULL) || 
       (pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_READ_IMAGE))
    {
        /* The image header is NOT in memory or the force to read image flag is set.
         * Allocate the memory to read the image header. 
         * The memory for the image header will be released after 
         * parsing the image header. 
         */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Reading Image header, force read image %s.\n",
                        (pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_READ_IMAGE) ? "YES" : "NO");

        pImageHeader = fbe_base_env_memory_ex_allocate(pBaseEnv, FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE);
        if(pImageHeader == NULL)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to alloc mem for image header.\n",   
                        __FUNCTION__);

            /* Return the bad status other than FBE_STATUS_OK so that it will retry. */
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Read the image header only.*/
        status = fbe_base_env_fup_read_image_from_file(pBaseEnv, 
                                               pWorkItem, 
                                               pImageHeader,
                                               FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE); // Read the image header only.                                               

        if(status != FBE_STATUS_OK)
        { 
            /* Release the memory for the image header. */
            fbe_base_env_memory_ex_release(pBaseEnv, pImageHeader);

            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_IMAGE_HEADER); 

            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /* Send down to the physical package to parse the image header.*/
        status = fbe_base_env_fup_send_parse_image_header_cmd(pBaseEnv, pWorkItem, pImageHeader, &parseImageHeader);

        /* Release the memory for the image header. */
        fbe_base_env_memory_ex_release(pBaseEnv, pImageHeader);

        if(status != FBE_STATUS_OK)
        {
            /* Parsing the image header failed. Can not proceed. */
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_PARSE_IMAGE_HEADER);       

            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
       
        /* Find the memory to save the new image header info. */
        status = fbe_base_env_find_next_image_info_ptr(pBaseEnv, pWorkItem, &pImageInfo);

        if(pImageInfo == NULL)
        {
            /* No image header space available for the work item. Can not proceed. 
             * This should never happen.
             */
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_IMAGE_TYPES_TOO_SMALL); 

            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        // image info needs to contain the image file name to ensure the correct file is referenced
        fbe_copy_memory(&pImageInfo->imagePath[0],
                        &pWorkItem->imagePath[0],
                        FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH);

        pImageInfo->imageSize = parseImageHeader.imageSize;
        pImageInfo->headerSize = parseImageHeader.headerSize;
        pImageInfo->firmwareTarget = parseImageHeader.firmwareTarget;
        pImageInfo->esesVersion = pWorkItem->esesVersion;
     
        fbe_copy_memory(&pImageInfo->imageRev[0],
                    &parseImageHeader.imageRev[0],
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE); 

        for(index = 0; 
            index < FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE;
            index ++) 
        {
            fbe_copy_memory(&pImageInfo->subenclProductId[index][0],
                        &parseImageHeader.subenclProductId[index][0],
                        FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE); 
        }

        /* Check whether the image header info just received is  
         * what the work item requires. 
         * If yes, get the pointer to the image info for the work item. 
         */
        status = fbe_base_env_fup_get_work_item_image_header_info(pBaseEnv, pWorkItem, pImageInfo);

        if(status != FBE_STATUS_OK) 
        {
            /* We still can not get the image header info for the work item 
             * after reading the image from the file.
             * The image from the file must be mismatched.
             */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                               "Reading Image header, mismatched image ProdId:%s, workitem ProdId:%s.\n",
                                               pImageInfo->subenclProductId[0],
                                               pWorkItem->subenclProductId);

            /* Bad image header for the work item. Can not proceed. */
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_MISMATCHED_IMAGE); 

            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    /* Save the persistent FUP info. */
    status = fbe_base_env_save_fup_info(pBaseEnv, pWorkItem);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s SHOULD NEVER HAPPEN! Failed to save persistent fup info status 0x%X.\n",   
                        __FUNCTION__, status);  
        
        /* Because of the bad status, it will retry again. */                                            
        return status;
    }     

    fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_READ_IMAGE_HEADER_DONE);

    /* Set the condition to check rev. */
    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s Can't set FUP_CHECK_REV condition, status: 0x%x.\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK; 
} //End of function fbe_base_env_fup_read_image_header


/*!*******************************************************************************
 * @fn fbe_base_env_fup_get_work_item_image_header_info_in_mem
 *********************************************************************************
 * @brief
 *  Check whether the image header info required is alreay in the memory or not.
 *  If yes, get the pointer to the image info.
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param pWorkItem - The pointer to the fup work item.
 * @param pImageInfoPtr(output) - The pointer to the image info pointer.
 *               If the return is NULL, the image header is not in mem.
 *               Otherwise, the image header is in mem.
 *
 * @return fbe_status_t -
 *         FBE_STATUS_OK - Got the image header info from the memory.
 *         Otherwise - could not get the image header info from the memory.
 *
 * @version
 *   15-June-2010 PHE - Created.
 *******************************************************************************/
static fbe_status_t
fbe_base_env_fup_get_work_item_image_header_info_in_mem(fbe_base_environment_t * pBaseEnv,
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_u32_t  imageIndex = FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES;
    fbe_base_env_fup_image_info_t * pImageInfo = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Check if there's already image header info in memory for this work item's subencl product id. */
    for (imageIndex = 0; imageIndex < FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES; imageIndex ++)
    { 
        pImageInfo = fbe_base_env_get_fup_image_info_ptr(pBaseEnv, imageIndex);

        status = fbe_base_env_fup_get_work_item_image_header_info(pBaseEnv,
                                                 pWorkItem,
                                                 pImageInfo);

        if((status == FBE_STATUS_OK) && (pWorkItem->pImageInfo != NULL))
        {
            if(pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_READ_IMAGE)
            {
                // it's a force read so remove the header entry because we
                // don't want to allow multiple headers containing different 
                // revisions for the same device
                // let's trace that we cleared the header
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_INFO,
                                                   fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                   "Force Rd clear old header:index:%d, Rev:%s.\n",
                                                   imageIndex,
                                                   (char *)pImageInfo->imageRev);
                if (pImageInfo->pImage != NULL)
                {
                    fbe_base_env_memory_ex_release(pBaseEnv, pImageInfo->pImage);
                }
                fbe_zero_memory(pImageInfo, sizeof(fbe_base_env_fup_image_info_t));
                pWorkItem->pImageInfo = NULL;
            }
            else
            {
                /* Got the image header for the work item. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                    "imageIndex %d, max imageIndex %d.\n", 
                                                    imageIndex, FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES);
            }

            break;
        }
    }// End of for loop
  
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "Image header in mem %s.\n",
                (pWorkItem->pImageInfo == NULL)? "NO" : "YES");

    return status;
} //fbe_base_env_fup_get_work_item_image_header_info_in_mem

/*!*******************************************************************************
 * @fn fbe_base_env_fup_get_work_item_image_header
 *********************************************************************************
 * @brief
 *  Check whether the image header info passed in matches with
 *  what the work item requires.
 *  If yes, assigned the pointer to the image info to the work item.
 *
 * @param pBaseEnv(input)- The pointer to the base env.
 * @param pWorkItem(input)- The pointer to the fup work item.
 * @param pImageInfo(input) - The pointer to the image info pointer.
 *
 * @return fbe_status_t -
 *      FBE_STATUS_OK - got the image header info for the work item.
 *      Otherwise - can not get the image header info for the work item.
 *
 * @version
 *   03-June-2011 PHE - Created.
 *******************************************************************************/
static fbe_status_t
fbe_base_env_fup_get_work_item_image_header_info(fbe_base_environment_t * pBaseEnv,
                                 fbe_base_env_fup_work_item_t * pWorkItem,
                                 fbe_base_env_fup_image_info_t * pImageInfo)
{
    char temp[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1] = {0};
    fbe_u32_t  index = 0;
    fbe_u32_t  i = 0;
    char hwProdIdChar;
    char imageProductIdChar;
    fbe_bool_t match = FBE_FALSE;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
              
    for(index = 0; 
        index < FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE;
        index ++) 
    {
        // the image path and file name must match else next item
        if(!fbe_equal_memory(&pImageInfo->imagePath[0], 
                             &pWorkItem->imagePath[0],
                             FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH))
        {
            match = FBE_FALSE;
            continue;
        }
        /* Would be all 0s if the image subencl product id is empty. */
        if(!fbe_equal_memory(&pImageInfo->subenclProductId[index][0], 
                             &temp[0], 
                             FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE))
        {
            /* The image subencl product id is not 0. Compare the hw pid with the image pid. */
            match = FBE_TRUE;

            for(i = 0; i < FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE; i++) 
            {
                // check for termination
                if((pWorkItem->subenclProductId[i] != 0) && 
                   (pImageInfo->subenclProductId[index][i] != 0))
                {
                    // don't be case sensitive in the comparison because the config 
                    // data and image header data may be different case (as seen with voyager)
                    hwProdIdChar = tolower(pWorkItem->subenclProductId[i]);
                    imageProductIdChar = tolower(pImageInfo->subenclProductId[index][i]);
                    
                    if(hwProdIdChar != imageProductIdChar)
                    {
                        // ignore mismatched ' ' and '\0'
                        if (((hwProdIdChar == ' ') || (hwProdIdChar == '\0')) &&
                            ((imageProductIdChar == ' ') || (imageProductIdChar ==  '\0')))
                        {
                            continue;
                        }
                        else
                        {
                            match = FBE_FALSE;
                            break;
                        }
                    }
                }
            }

            if(match) 
            {
                /* The matching subencl product id is found. */
                if((pWorkItem->firmwareTarget == pImageInfo->firmwareTarget) &&
                    (pWorkItem->esesVersion == pImageInfo->esesVersion))
                {
                    /* The matching firmwareTarget is found. */
                    fbe_base_env_set_fup_work_item_image_info_ptr(pWorkItem, pImageInfo);
                    status = FBE_STATUS_OK;
                }
                break;
               
            }
        }
    }// for all product id

    return status;
} //fbe_base_env_fup_get_work_item_image_header_info


/*!*******************************************************************************
 * @fn fbe_base_env_fup_read_image_from_file
 *********************************************************************************
 * @brief
 *  Reads the image from the file for the specified work item.
 *  The bytes to be read is specified by the parameter bytesToRead.
 *  It can be used to read the image header or the entire image.
 *
 * @param pBaseEnv  - pointer to fbe_base_environment_t.
 * @param pWorkItem  - pointer to the work item.
 * @param pImage (OUTPUT) - the pointer to the image.
 * @param bytesToRead  - bytes to read.
 *
 * @return fbe_status_t 
 *        FBE_STATUS_OK - successful
 *        otherwise - not successful
 *
 * @note
 *
 * @version
 *  16-June-2010 PHE - Created.
 *******************************************************************************/
static fbe_status_t 
fbe_base_env_fup_read_image_from_file(fbe_base_environment_t * pBaseEnv, 
                                             fbe_base_env_fup_work_item_t * pWorkItem,
                                             fbe_u8_t * pImage,
                                             fbe_u32_t bytesToRead)
{
    fbe_u32_t       bytesRead = 0;
    fbe_status_t       status = FBE_STATUS_OK;    

    if(fbe_equal_memory(&pWorkItem->imagePath[0], "NULL", 4))
    {
        /* Invalid image path. This is the coding error. It should never happen. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "base_env_fup_rd_img_from_file, NULL image path.\n"); 

        return FBE_STATUS_GENERIC_FAILURE;      
    }

    /* Read the file. */
    status  = fbe_base_env_file_read(pBaseEnv, &pWorkItem->imagePath[0], pImage, bytesToRead,
                                    FBE_FILE_RDONLY, &bytesRead);

    if(status != FBE_STATUS_OK)    
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_WARNING,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "base_env_fup_rd_img_from_file failed,imgPath%s\n", 
                    &pWorkItem->imagePath[0]); 

        return status;        
    }

    if(bytesRead < bytesToRead)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_WARNING,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "base_env_fup_rd_img_from_file,bytesRead(%d) < bytesToRead(%d)\n", 
                    bytesRead,
                    bytesToRead); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
} //End of function fbe_base_env_fup_read_image_from_file


/*!*******************************************************************************
 * @fn fbe_base_env_fup_send_parse_image_header_cmd
 *********************************************************************************
 * @brief
 *  This function sends down the parse image header command to the physical package 
 *  to parse the image header.
 *              
 * @param pBaseEnv -
 * @param work_item - contains the product id to match.
 * @param pImageHeader - The pointer to the image header.
 * @param pParseImageHeader - 
 *
 * @return fbe_status_t 
 *
 * @version
 *  16-June-2010 PHE - Created.
 *******************************************************************************/
static fbe_status_t 
fbe_base_env_fup_send_parse_image_header_cmd(fbe_base_environment_t * pBaseEnv,
                                                  fbe_base_env_fup_work_item_t * pWorkItem,
                                                  fbe_u8_t * pImageHeader,
                                                  fbe_enclosure_mgmt_parse_image_header_t *  pParseImageHeader)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t object_id;
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_topology_control_get_enclosure_by_location_t enclByLocation = {0};
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Sending cmd to parse image header.\n");    
        
    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    // get the object id to pass to phys package
    if((pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) && (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
    {
        // test location for SPS connected to proc encl
        // use the esp obj id directly for board object (SPS attached to pe)
        status = fbe_api_get_board_object_id(&object_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                               "%s, Failed to get board object Id, status 0x%x.\n",   
                                               __FUNCTION__,
                                               status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        // get the obj ids within the the enclosure (needed for voyager ee)
        status = fbe_api_get_enclosure_object_ids_array_by_location(pLocation->bus, 
                                                                    pLocation->enclosure, 
                                                                    &enclByLocation);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to get encl object id array, status 0x%x.\n",   
                        __FUNCTION__,
                        status);   
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else if((pLocation->componentId == 0) || (pWorkItem->deviceType == FBE_DEVICE_TYPE_SPS))
        {
            object_id = enclByLocation.enclosure_object_id;
        }
        else
        {
            object_id = enclByLocation.comp_object_id[pLocation->componentId];
        }
    }
        
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "%s, Encl %d_%d, get object id 0x%X.\n",   
                __FUNCTION__,
                pLocation->bus,
                pLocation->enclosure,
                object_id); 

    if(FBE_OBJECT_ID_INVALID == object_id)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, Invalid object Id.\n",   
                    __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(pParseImageHeader, sizeof(fbe_enclosure_mgmt_parse_image_header_t));
    fbe_copy_memory(&pParseImageHeader->imageHeader[0], 
                     pImageHeader, 
                     FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE);

    fbe_copy_memory(&pParseImageHeader->subenclProductId[0], 
                    &pWorkItem->subenclProductId[0], 
                    FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE);

    // send to phys package to parse the file header
    status = fbe_api_enclosure_parse_image_header(object_id, pParseImageHeader);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, parse_image_header failed, status 0x%x.\n",   
                    __FUNCTION__,
                    status);

    }
   
    return status;
} //fbe_base_env_fup_send_parse_image_header_cmd


/*!*******************************************************************************
 * @fn fbe_base_env_find_next_image_info_ptr
 *********************************************************************************
 * @brief
 *   This function is to find the next image info pointer.
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param pImageInfoPtr -
 * 
 * @return fbe_status_t
 *
 * @version
 *   15-June-2010 PHE - Created.
 *******************************************************************************/
static fbe_status_t 
fbe_base_env_find_next_image_info_ptr(fbe_base_environment_t * pBaseEnv, 
                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                      fbe_base_env_fup_image_info_t ** pImageInfoPtr)
{
    fbe_u32_t currentImageIndex = 0;
    fbe_u32_t nextImageIndex = 0;
    fbe_base_env_fup_image_info_t * pImageInfo = NULL;

    *pImageInfoPtr = NULL;

    currentImageIndex = fbe_base_env_get_fup_next_image_index(pBaseEnv);
    
    pImageInfo = fbe_base_env_get_fup_image_info_ptr(pBaseEnv, currentImageIndex);
   
    /* Since the upgrade for the devices is serial. So this is the only device whose upgrade is in progress.
     * It means the image here is not currently being used. That is why it can released.
     */
    if(pImageInfo->pImage != NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                              "Releasing Image %p, index %d.\n",
                              pImageInfo->pImage,
                              currentImageIndex);

        fbe_base_env_memory_ex_release(pBaseEnv, pImageInfo->pImage);

        pImageInfo->pImage = NULL;

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                              "Image released, imageRev %s, imageSize 0x%x.\n",
                              (char*)pImageInfo->imageRev,
                              pImageInfo->imageSize);
    }
    
    *pImageInfoPtr = pImageInfo;

    /* Update the next image index saved in base env. */
    nextImageIndex = (currentImageIndex + 1)% FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES;
    fbe_base_env_set_fup_next_image_index(pBaseEnv, nextImageIndex);
 
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
            FBE_TRACE_LEVEL_INFO,
            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
            "imageIndex %d, max imageIndex %d.\n", 
            currentImageIndex, FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES);
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_check_rev
 ***************************************************************
 * @brief
 *  This function compares the rev of the hardware with the rev of the image.
 *  If the rev of the hardware  is at lower rev than the rev of the image,
 *  set the condition to read the entire memory if the entire image is not in memory.
 *  Otherwise, set the condition to download the image.
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 * 
 * @note 
 *     The imagePath has to be ready when it is needed to read the image header or the entire image.
 *
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_check_rev(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_bool_t   lowerRev = FALSE;
    fbe_status_t status = FBE_STATUS_OK;

    if(pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, NO_REV_CHECK.\n",   
                        __FUNCTION__);                          
    }
    else 
    {
        fbe_base_env_fup_hw_at_lower_rev(pBaseEnv, pWorkItem, &lowerRev);

        if(!lowerRev)
        {
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_EXIT_UP_TO_REV); 

            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_CHECK_REV_DONE);

    /* Set the condition to read the entire image. */
    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s Can't set FUP_READ_ENTIRE_IMAGE condition, status: 0x%x.\n",
                                __FUNCTION__, status);
    }
   
    return FBE_STATUS_OK; 
} //End of function fbe_base_env_fup_check_rev



/*!*******************************************************************************
 * @fn fbe_base_env_fup_format_fw_rev
 *********************************************************************************
 * @brief
 *  Format the rev by removing the chars that we don't want. 
 *
 * @param pRevAfterFormatting
 * @param pRevToCheck
 * @param size
 *  
 * @return fbe_status_t - Always return FBE_STATUS_OK
 *
 * @version
 *       05-Oct-2010 Greg B. -- Created.
 *       09-Oct-2010 PHE - updated to only copy the chars that we want.
 *       07-Aug-2014 PHE - updated to format to major, minor and patch format.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_format_fw_rev(fbe_base_env_fup_format_fw_rev_t * pFwRevAfterFormatting,
                                            fbe_u8_t * pFwRev,
                                            fbe_u8_t fwRevSize)
{
    fbe_u32_t i = 0;  // loop through all the charactors in the string of firmware revision.
    fbe_u32_t dots = 0;
    fbe_u32_t returnCount = 0;
    fbe_u32_t major = 0;
    fbe_u32_t minor = 0;
    fbe_u32_t patch = 0;
    fbe_u8_t fwRevTemp[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1] = {0};

    fbe_zero_memory(pFwRevAfterFormatting, sizeof(fbe_base_env_fup_format_fw_rev_t));

    fbe_zero_memory(&fwRevTemp[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&fwRevTemp[0], pFwRev, FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    for(i = 0; i < fwRevSize; i++)
    {
        if(fwRevTemp[i] == '.')
        {
            dots++;
        }
        else if((fwRevTemp[i] == ' ') && (i < FBE_ESES_FW_REVISION_1_0_SIZE - 1))
        {
            /* The CDES-1 firmware rev size is 5.We don't care about the byte 4.
             * It is possible that firmware rev of CDES-1 or 
             * the CDES-2 PS firmware rev (still uses CDES-1 firmware rev format)
             * has the space in the first 5 bytes. It would cause the problem while
             * calling sscanf because sscanf would skip the leading spaces.
             * So fill out 0s for the spaces the first 4 bytes if needed. 
             */ 
            fwRevTemp[i] = '0';
        }
    }

    if(dots == 0) 
    {
        returnCount = sscanf(&fwRevTemp[0], "%2d%2d", &major, &minor);

        if(returnCount == EOF)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwRevAfterFormatting->majorNumber = major;
        pFwRevAfterFormatting->minorNumber = minor;
        pFwRevAfterFormatting->patchNumber = 0;
    }
    else if(dots == 1) 
    {
        returnCount = sscanf(&fwRevTemp[0], "%2d.%2d", &major, &minor);

        if(returnCount == EOF)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwRevAfterFormatting->majorNumber = major;
        pFwRevAfterFormatting->minorNumber = minor;
        pFwRevAfterFormatting->patchNumber = 0;
    }
    else if(dots == 2) 
    {
        returnCount = sscanf(&fwRevTemp[0], "%2d.%2d.%2d", &major, &minor, &patch);

        if(returnCount == EOF)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pFwRevAfterFormatting->majorNumber = major;
        pFwRevAfterFormatting->minorNumber = minor;
        pFwRevAfterFormatting->patchNumber = patch;
    }
    else
    {
        pFwRevAfterFormatting->majorNumber = 0;
        pFwRevAfterFormatting->minorNumber = 0;
        pFwRevAfterFormatting->patchNumber = 0;

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_hw_at_lower_rev
 *********************************************************************************
 * @brief
 *  Check whether the hareware rev is lower than the image rev.
 *  Note the rev has to be ASCII characters.
 *
 * @param pBaseEnv
 * @param pWorkItem
 * @param pLowerRev(OUTPUT) - The pointer to lowerRev.
 *         If lowerRev is TRUE, the hardware rev is lower than the image rev.
 *         Otherwise, the hardware rev is equal to or higer than the image rev.
 *  
 * @return fbe_status_t - Always return FBE_STATUS_OK
 *
 * @version
 *       16-June-2010 PHE -- Created.
 *******************************************************************************/
static fbe_status_t fbe_base_env_fup_hw_at_lower_rev(fbe_base_environment_t * pBaseEnv,
                                                     fbe_base_env_fup_work_item_t * pWorkItem,
                                                     fbe_bool_t * pLowerRev)
{
    fbe_base_env_fup_image_info_t * pImageInfo = NULL;
    fbe_base_env_fup_format_fw_rev_t firmwareRevAfterFormatting = {0};
    fbe_base_env_fup_format_fw_rev_t imageRevAfterFormatting = {0};


    /* Init to FALSE. */
    *pLowerRev = FALSE;
    pImageInfo = fbe_base_env_get_fup_work_item_image_info_ptr(pWorkItem);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Compare rev, HwRev %s, imageRev %s.\n", 
                    &pWorkItem->firmwareRev[0],
                    &pImageInfo->imageRev[0]);  

    // format the rev from the work item (read from hardware)
    fbe_base_env_fup_format_fw_rev(&firmwareRevAfterFormatting, 
                                   &pWorkItem->firmwareRev[0], 
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    // format the image rev from the manifest
    fbe_base_env_fup_format_fw_rev(&imageRevAfterFormatting, 
                                   &pImageInfo->imageRev[0], 
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Compare rev after format, HwRev:major %d,minor %d,patch %d.\n", 
                    firmwareRevAfterFormatting.majorNumber,
                    firmwareRevAfterFormatting.minorNumber,
                    firmwareRevAfterFormatting.patchNumber);  

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Compare rev after format, imageRev:major %d,minor %d,patch %d.\n", 
                    imageRevAfterFormatting.majorNumber,
                    imageRevAfterFormatting.minorNumber,
                    imageRevAfterFormatting.patchNumber);                  

    if(firmwareRevAfterFormatting.majorNumber < imageRevAfterFormatting.majorNumber) 
    {
        /* The firmware rev is lower than the image rev. */
        *pLowerRev = TRUE;
    }
    else if((firmwareRevAfterFormatting.majorNumber == imageRevAfterFormatting.majorNumber) && 
            (firmwareRevAfterFormatting.minorNumber < imageRevAfterFormatting.minorNumber))
    {
        /* The firmware rev is lower than the image rev. */
        *pLowerRev = TRUE;
    }
    else if((firmwareRevAfterFormatting.majorNumber == imageRevAfterFormatting.majorNumber) && 
            (firmwareRevAfterFormatting.minorNumber == imageRevAfterFormatting.minorNumber) &&
            (firmwareRevAfterFormatting.patchNumber < imageRevAfterFormatting.patchNumber))
    {
        /* The firmware rev is lower than the image rev. */
        *pLowerRev = TRUE;
    }
        
    return FBE_STATUS_OK;

} //fbe_base_env_fup_hw_at_lower_rev


/*!**************************************************************
 * @fn fbe_base_env_fup_read_entire_image
 ***************************************************************
 * @brief
 *  This function reads the entire image and sets the condition to
 *  download the image.
 *
 * @param pBaseEnv - The pointer to fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 * 
 * @note 
 *     The imagePath has to be ready when it is needed to read the image header or the entire image.
 *
 * @version:
 *  15-June-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_read_entire_image(fbe_base_environment_t * pBaseEnv, 
                                     fbe_base_env_fup_work_item_t * pWorkItem)
{ 
    fbe_u8_t   * pEntireImage = NULL;
    fbe_base_env_fup_image_info_t * pImageInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;
 
    pImageInfo = fbe_base_env_get_fup_work_item_image_info_ptr(pWorkItem);

    /* Check whether the entire image is in memory. */
    if(pImageInfo->pImage != NULL)
    {
        /* The entire image is already in memory. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "The entire image is in memory, NO need to read entire image.\n"); 
    }
    else
    {
        /* The entire image is NOT in memory. */         
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "The entire image is NOT in memory, reading entire image, size 0x%x.\n", 
                        pImageInfo->imageSize);
 
        /* Allocate the memory for the entire image. 
         * The memory for the image will be released when 
         * all the work items in the queue are removed.
         */
        pEntireImage = fbe_base_env_memory_ex_allocate(pBaseEnv, pImageInfo->imageSize);
        if(pEntireImage == NULL)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_WARNING,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "read_entire_image, failed to alloc mem for entire image.\n");
    
            /* Return the bad status other than FBE_STATUS_OK so that it will retry. */
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "read_entire_image, allocated pEntireImage %p, size 0x%x.\n",   
                        pEntireImage, pImageInfo->imageSize);

        /* Read the entire image. */
        status = fbe_base_env_fup_read_image_from_file(pBaseEnv, 
                                               pWorkItem,
                                               pEntireImage,
                                               pImageInfo->imageSize); // Read the entire image.
    
        if(status != FBE_STATUS_OK)
        {
            /* Release the memory for the entire image. */
            fbe_base_env_memory_ex_release(pBaseEnv, pEntireImage);
    
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_ENTIRE_IMAGE); 
    
            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        pImageInfo->pImage = pEntireImage;
    }
   
    fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_READ_ENTIRE_IMAGE_DONE);

    /* Set the next condition. */
    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: read_entire_image, Can't set FUP_GET_PEER_PERMISSION condition, status: 0x%x.\n",
                                status);
    }

    return FBE_STATUS_OK; 
} //End of function fbe_base_env_fup_read_entire_image

/*!**************************************************************
 * @fn fbe_base_env_fup_download_image(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function sends the download command for the particular work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_download_image(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_u32_t       elapsedTimeInSec = 0;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_status_t    status1 = FBE_STATUS_OK;
    fbe_status_t    status2 = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    // get fup permission from board object to avoid simultanious upgrade between encl 0_0 and other enclosures
    if((pWorkItem->location.bus != FBE_XPE_PSEUDO_BUS_NUM) || (pWorkItem->location.enclosure != FBE_XPE_PSEUDO_ENCL_NUM))
    {
        status1 = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION);
    }

    if (status1 == FBE_STATUS_OK)
    {
        status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD);
    }

    // download command failed. give permission back, will re-apply it later
    if (status != FBE_STATUS_OK)
    {
        status2 = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION);
        if (status2 != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, Failed to return local firmware upgrade permission, status 0x%x.\n",   
                        __FUNCTION__, status2);
        }
    }


    if(status != FBE_STATUS_OK || status1 != FBE_STATUS_OK)
    {
        if(pWorkItem->cmdStartTimeStamp == 0)
        {
            pWorkItem->cmdStartTimeStamp = fbe_get_time();
        }
        else
        {
            elapsedTimeInSec = fbe_get_elapsed_seconds(pWorkItem->cmdStartTimeStamp);
            if(elapsedTimeInSec >= FBE_BASE_ENV_FUP_MAX_DOWNLOAD_CMD_RETRY_TIMEOUT) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, Failed to send DOWNLOAD cmd in %d seconds, End of Upgrade, status 0x%x.\n",   
                    __FUNCTION__, 
                    FBE_BASE_ENV_FUP_MAX_DOWNLOAD_CMD_RETRY_TIMEOUT,
                    status);

                fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DOWNLOAD_CMD_DENIED); 

                /* The work item needs to be removed from the queue. */                                            
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "DOWNLOAD cmd was sent successfully.\n");

        pWorkItem->cmdStartTimeStamp = 0;

        fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT);
    }

    if (status1 != FBE_STATUS_OK)
    {
        return status1;
    }
    else
    {
        return status;
    }
} // End of function fbe_base_env_fup_download_image

/*!**************************************************************
 * @fn fbe_base_env_fup_get_download_status(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function sends the command to get the download status for the particular work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  02-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_get_download_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_u32_t       elapsedTimeInSec = 0;
    fbe_status_t    status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS);

    if(status != FBE_STATUS_OK)
    {
        if(pWorkItem->cmdStartTimeStamp == 0)
        {
            pWorkItem->cmdStartTimeStamp = fbe_get_time();
        }
        else
        {
            elapsedTimeInSec = fbe_get_elapsed_seconds(pWorkItem->cmdStartTimeStamp);
            if(elapsedTimeInSec >= FBE_BASE_ENV_FUP_MAX_GET_STATUS_CMD_RETRY_TIMEOUT) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Failed to send GET STATUS cmd for DOWNLOAD in %d seconds, End of Upgrade, status 0x%x.\n",   
                    FBE_BASE_ENV_FUP_MAX_GET_STATUS_CMD_RETRY_TIMEOUT,
                    status);

                fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_DOWNLOAD_STATUS); 

                /* The work item needs to be removed from the queue. */                                            
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "GET STATUS cmd was sent successfully.\n");

        pWorkItem->cmdStartTimeStamp = 0;

        status = fbe_base_env_fup_handle_download_status_change(pBaseEnv, pWorkItem);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "fup_handle_download_status_change failed, status 0x%X.\n",
                                                status);
        }
    }
    return status;
} // End of function fbe_base_env_fup_get_download_status

    /*!**************************************************************
 * @fn fbe_base_env_fup_get_activate_status(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function sends the command to get the activate status for the particular work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  02-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_get_activate_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_u32_t       elapsedTimeInSec = 0;
    fbe_status_t    status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS);

    if(status != FBE_STATUS_OK)
    {
        if(pWorkItem->cmdStartTimeStamp == 0)
        {
            pWorkItem->cmdStartTimeStamp = fbe_get_time();
        }
        else
        {
            elapsedTimeInSec = fbe_get_elapsed_seconds(pWorkItem->cmdStartTimeStamp);
            if(elapsedTimeInSec >= FBE_BASE_ENV_FUP_MAX_GET_STATUS_CMD_RETRY_TIMEOUT) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Failed to send GET STATUS cmd for ACTIVATE in %d seconds, End of Upgrade, status 0x%x.\n",   
                    FBE_BASE_ENV_FUP_MAX_GET_STATUS_CMD_RETRY_TIMEOUT,
                    status);
                    
                fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_ACTIVATE_STATUS); 

                /* The work item needs to be removed from the queue. */                                            
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "GET STATUS cmd was sent successfully.\n");

        pWorkItem->cmdStartTimeStamp = 0;
               
        status = fbe_base_env_fup_handle_activate_status_change(pBaseEnv, pWorkItem);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "fup_handle_activate_status_change failed, status 0x%X.\n",
                                                status);
        }
    }
    return status;
} // End of function fbe_base_env_fup_get_activate_status

/*!*******************************************************************************
 * @fn fbe_base_env_fup_encl_icm_priority_check
 *********************************************************************************
 * @brief
 *  We want to prioritize LCC FUP such that the ICM is upgraded before any EE. This
 *  function will check the status of ICM FUP and return TRUE/FALSE to indicate if
 *  it is ok to proceed.
 * 
 *  if device is lcc:
 *      if this work item is for ICM, ok
 *      if this work item is ee, check icm work item status
 * 
 * @param  pBaseEnv - The pointer to fbe_base_environment_t.
 * @param  pWorkItem - work item for the requesting component
 * @param  pWaitForICM (OUTPUT) -
 *        TRUE  - ICM FUP is not done
 *        FALSE - ICM FUP is finished
 * 
 * @return fbe_status_t 
 *
 * @version
 *  16-June-2010 PHE - Created.
 *******************************************************************************/
static fbe_status_t fbe_base_env_fup_encl_icm_priority_check(fbe_base_environment_t *pBaseEnv,
                                                             fbe_base_env_fup_work_item_t *pWorkItem,
                                                             fbe_bool_t             *pWaitForEE)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_device_physical_location_t      location;
    fbe_base_env_fup_persistent_info_t  *pFupInfo = NULL;
    
    *pWaitForEE = FALSE;
    if (pWorkItem->deviceType == FBE_DEVICE_TYPE_LCC)
    {
            // specify the cid in order to referenc the icm fup info
            location = pWorkItem->location;
            location.componentId = 0;
            //status = fbe_encl_mgmt_get_fup_info_ptr(pEnclMgmt, deviceType, &location, &pFupInfo);
            status = pBaseEnv->fup_element_ptr->pGetFupInfoPtrCallback(pBaseEnv, 
                                                                       pWorkItem->deviceType,
                                                                       &location,
                                                                       &pFupInfo);
            //gbb debug trace
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                               "%s status:%d ICM status:%s\n",   
                                               __FUNCTION__,
                                               status,
                                               fbe_base_env_decode_fup_completion_status(pFupInfo->completionStatus));
            
            if((status != FBE_STATUS_OK) || 
               (pFupInfo->completionStatus <= FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS))
            {
                *pWaitForEE = TRUE;
                if(status == FBE_STATUS_OK)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                    "%s, ICM FUP incomplete, delay EE FUP\n",   
                                    __FUNCTION__);
                }
            }
    }

    return status;
} //fbe_base_env_fup_encl_icm_priority_check

/*!**************************************************************
 * @fn fbe_base_env_fup_get_peer_permission(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 * This function 
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_get_peer_permission(fbe_base_environment_t * pBaseEnv, 
                                fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_u32_t           elapsedTimeInSec;
    fbe_bool_t          send_request = TRUE;
    fbe_status_t        status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    if((pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE)||
       (pBaseEnv->isSingleSpSystem == TRUE))
    {
        /* Single SP upgrade. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, SINGLE_SP_MODE.\n",   
                        __FUNCTION__);

        fbe_base_env_set_fup_work_item_state(pWorkItem, 
                                             FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_RECEIVED);
        
        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS);
        
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "FUP: %s Can't set FUP_CHECK_ENV_STATUS condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
        }
    }
    else
    {
        /* Dual SP upgrade */
        if(pWorkItem->cmdStartTimeStamp != 0)
        {
            /* This is not the first time to send the get peer permission request.
             * It is the retry. We want to delay before sending peer perm request.
             * This is part of delay retry of the request that has previously failed. 
             */
            elapsedTimeInSec = 
            fbe_get_elapsed_seconds(pWorkItem->cmdStartTimeStamp); 
        
            if(elapsedTimeInSec < FBE_BASE_ENV_FUP_CMI_RETRY_TIME) 
            { 
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "%s, Check to Resend PeerPermRqst elapsed:%d\n",   
                                                __FUNCTION__,
                                                elapsedTimeInSec);

                // not enough time has gone by to send another peer perm request
                send_request = FALSE;
                status = FBE_STATUS_BUSY;
            }
        }

        
        // Send request to peer for permission to upgrade

        // !!!For lcc we will need to adjust location information to reference the peer slot. The peer
        // uses the location information to find the work item for that lcc. If the slot is incorrect it won't 
        // find the work item or will use the wrong work item!!!!!

        if (send_request)
        {
            pWorkItem->cmdStartTimeStamp = fbe_get_time();

            status = fbe_base_env_fup_fill_and_send_cmi(pBaseEnv,
                                                        FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST, 
                                                        pWorkItem->deviceType,
                                                        &pWorkItem->location,
                                                        pWorkItem);

            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, fbe_base_env_fup_fill_and_send_cmi failed.\n",   
                    __FUNCTION__);
            }

            fbe_base_env_set_fup_work_item_state(pWorkItem, 
                                             FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED);
        }

        /* FBE_BASE_ENV_LIFECYCLE_FUP_CHECK_ENV_STATUS condition will be set 
         * when the peer permission is received. 
         */
    }

    return status;
}


/*!**************************************************************
 * @fn fbe_base_env_fup_check_env_status(fbe_base_environment_t * pBaseEnv, 
 *                                fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks the environmental status for the specified work item.
 *  If the environment does not allow the upgrade, remove the work item from the queue.
 *  Otherwise, set the condition to activate the image.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the packet.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_check_env_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);
   
    //changr2 
    //CDES 14.11 image have compatibility issue with later revision. When local side is 14.11 and 
    //the other side is newer than 14.11, then peer LCC is marked as faulted by CDES. 
    //This will block upgreade for local side. So skip this check to let upgrade go. 
    if((pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_NO_ENV_CHECK) ||
       (strncmp("1411 ", &pWorkItem->firmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) == 0) ||
       (strncmp("1410 ", &pWorkItem->firmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) == 0) ||
       (strncmp("1409 ", &pWorkItem->firmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) == 0) ||
       (strncmp("1408 ", &pWorkItem->firmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) == 0))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, NO_ENV_CHECK.\n",   
                        __FUNCTION__);
    }
    else
    {
        status = pBaseEnv->fup_element_ptr->pCheckEnvStatusCallback(pBaseEnv, pWorkItem);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "Bad Env status, status 0x%x.\n", 
                            status); 
    
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS); 
    
            /* The work item needs to be removed from the queue. */                                            
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_CHECK_ENV_STATUS_DONE);

    /* Set the next condition. */
    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s Can't set FUP_DOWNLOAD_IMAGE condition, status: 0x%x.\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;

} // End of function fbe_base_env_fup_check_env_status


/*!**************************************************************
 * @fn fbe_base_env_fup_activate_image(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 * This function checks whether there is any power supply image activation in progress
 * on the same bus as the specified work item. 
 * If not, send the command to activate the image.
 * We allow only one power supply firmware activation in progress.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_activate_image(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_bool_t   deviceTypeActivateInProgress = FALSE;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    /* We don't want to activate the same type of device at the same type
     * in case that the image is bad.
     */
    fbe_base_env_fup_device_type_activate_in_progress(pBaseEnv, pWorkItem->deviceType, &deviceTypeActivateInProgress);

    if(deviceTypeActivateInProgress)
    {
        /* Return the error status so that the current condition can be retried. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Other device with same device type already in activate, wait to activate the image.\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Before sending the activate command, send the CMI message to peer SP to 
     * let peer SP know we are about to activate image. 
     * The peer SP needs to know this info so that it can suppress the device fault 
     * because it could be caused by firmware activation.
     */
    status = fbe_base_env_fup_fill_and_send_cmi(pBaseEnv,
                                                    FBE_BASE_ENV_CMI_MSG_FUP_PEER_ACTIVATE_START, 
                                                    pWorkItem->deviceType,
                                                    &pWorkItem->location,
                                                    pWorkItem);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "%s, Failed to send CMI_MSG_FUP_PEER_ACTIVATE_START msg to peer SP.\n",   
                __FUNCTION__);
    }

    /* There is no activatation in progress on the same encl as the work item. 
    * Send the activate command.
    */
    status = fbe_base_env_fup_send_cmd(pBaseEnv, pWorkItem, FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE);


    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, Failed to send ACTIVATE cmd, status 0x%x.\n",   
                    __FUNCTION__,
                    status);    
    }
    else
    {
         fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "ACTIVATE cmd was sent successfully.\n");   

         fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT);
    }
   
    return status;
} // End of function fbe_base_env_fup_activate_image


/*!*******************************************************************************
 * @fn fbe_base_env_fup_device_type_activate_in_progress
 *********************************************************************************
 * @brief
 *  The function checks whether there is any work item with the specified device type
 *  is activation in progress.
 *
 * @param  pBaseEnv - The pointer to fbe_base_environment_t.
 * @param  deviceType - The specified device type.
 * @param  pActivateInProgress (OUTPUT) -
 *        TRUE  - There is a work item with the specified drive type which is activation in progress.
 *        FALSE - Otherwise.
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK
 *
 * @version
 *  16-June-2010 PHE - Created.
 *******************************************************************************/
static fbe_status_t fbe_base_env_fup_device_type_activate_in_progress(fbe_base_environment_t * pBaseEnv, 
                                               fbe_u64_t deviceType,
                                               fbe_bool_t * pActivateInProgress)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;

    /* Init to FALSE. */
    *pActivateInProgress = FALSE;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        if((pWorkItem->deviceType == deviceType) &&
           (pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT))                         
        {
            *pActivateInProgress = TRUE;
            break;
        }

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }

    return FBE_STATUS_OK;
} //fbe_base_env_fup_device_type_activate_in_progress


/*!**************************************************************
 * @fn fbe_base_env_fup_check_result(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 * This function checks the hardware firmware rev after the activation completes.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_check_result(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_bool_t revChanged = FALSE;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_fup_completion_status_t completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_NONE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s, entry.\n",   
                        __FUNCTION__);

    /* Get the latest firmwareRev on hardware. */
    status = pBaseEnv->fup_element_ptr->pGetFirmwareRevCallback(pBaseEnv, 
                                                           pWorkItem,
                                                           &pWorkItem->newFirmwareRev[0]);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Check upgrade result, failed to get fw rev, will retry, status 0x%x.\n", 
                        status); 

        /* Will retry. */
        return status;
    }

    status = fbe_base_env_fup_check_rev_change(pBaseEnv, pWorkItem, &revChanged);

    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    if(revChanged)
    {
        completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED;
    }        
    else if(pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK)
    {
        completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_NO_REV_CHANGE;
    }
    else
    {
        completionStatus = FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_REV_CHANGE;
    }
    
    fbe_base_env_set_fup_work_item_completion_status(pWorkItem, completionStatus); 

    fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE);
    
    /* The work item needs to be removed from the queue. */                                           
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}// End of funtion fbe_base_env_fup_check_result

         
/*!**************************************************************
 * @fn fbe_base_env_fup_check_rev_change(fbe_ps_mgmt_t * pPsMgmt,
 *                         fbe_base_env_fup_work_item_t * pWorkItem,
 *                         fbe_bool_t * pRevChanged)
 ****************************************************************
 * @brief
 *  This function checks whether the rev has changed.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item.
 * @param pRevChanged (OUTPUT) - The pointer to fbe_bool_t
 *                TRUE - rev has changed.
 *                FALSE - no rev change.
 *
 * @return fbe_status_t - always return FBE_STATUS_OK
 *
 * @version
 *  25-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_check_rev_change(fbe_base_environment_t * pBaseEnv,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                                                    fbe_bool_t * pRevChanged)
{    
    *pRevChanged = TRUE;
    
    if(pWorkItem->newFirmwareRev[0] == 0)
    {
        /* The new rev is not ready. We need to retry.
         * Return FBE_STATUS_GENERIC_FAILURE so that the condition can be retried.
         */
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Compare the new firmwareRev with the old one saved in the work item. */
    if(strncmp(&pWorkItem->newFirmwareRev[0], &pWorkItem->firmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) == 0)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Check upgrade result, No rev change, new rev %s, old rev %s.\n",  
                    &pWorkItem->newFirmwareRev[0],
                    &pWorkItem->firmwareRev[0]);

        *pRevChanged = FALSE;

    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Check upgrade result, upgrade successful!!!.\n");

        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "new rev %s, old rev %s.\n", 
                    &pWorkItem->newFirmwareRev[0],
                    &pWorkItem->firmwareRev[0]);

        *pRevChanged = TRUE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_refresh_device_status(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 * This function refresh the device status after the activation completes.
 * It is possible that we have filtered the status change during the upgrade.
 * So we want to refresh the device status here.
 * The work items will be removed and released after refreshing device status is done.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_refresh_device_status(fbe_base_environment_t * pBaseEnv, 
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Refresh the device status. */
    if(pBaseEnv->fup_element_ptr->pRefreshDeviceStatusCallback != NULL) 
    {
        status = pBaseEnv->fup_element_ptr->pRefreshDeviceStatusCallback(pBaseEnv, pWorkItem);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "Refresh Device Status, failed to refresh status, status 0x%X.\n", 
                            status); 

            
            /* Will retry. */
            return status;
        }
    }

    fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_REFRESH_DEVICE_STATUS_DONE);
    
    /* Set the next condition. */
    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s Can't set FUP_END_UPGRADE condition, status: 0x%x.\n",
                                __FUNCTION__, status);
    }
                                      
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_handle_firmware_status_change(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_u64_t deviceType,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets called when the notification was recieved that
 *  the download image status or the activate image status has changed. 
 *  It finds the work item with the specified device type and the location, 
 *  sends the command to get the download status and 
 *  handles the status based on the current work state.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param deviceType - The device type.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_fup_handle_firmware_status_change(fbe_base_environment_t * pBaseEnv,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation)
{
    fbe_base_env_fup_work_item_t * pWorkItem = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "FUP:%s(%d_%d_%d) %s entry.\n",
                           fbe_base_env_decode_device_type(deviceType),
                           pLocation->bus,
                           pLocation->enclosure,
                           pLocation->slot,
                           __FUNCTION__);

    fbe_base_env_fup_find_work_item(pBaseEnv, deviceType, pLocation, &pWorkItem);

    if(pWorkItem == NULL)
    {
        /* It is not an error if we could not find the work item.
         * The work item could have been removed from the queue 
         * due to the removal of the Power Supply or
         * because we got the newer status and then removed the Power Supply work item
         * for the previous status change notification
         * before receiving this notification.
         */
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),compId %d, %s, Can't find work item.\n",
                              fbe_base_env_decode_device_type(deviceType),
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              pLocation->componentId,
                              __FUNCTION__);
        return status;
    }

    switch(pWorkItem->workState)
    {
    case FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT:   
    case FBE_BASE_ENV_FUP_WORK_STATE_ABORT_CMD_SENT:
        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "FUP: %s Can't set FUP_GET_DOWNLOAD_STATUS condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
        }
        break;

    case FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT:
        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "FUP: %s Can't set FUP_GET_ACTIVATE_STATUS condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
        }
        
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "FUP: %s unexpected workState %s.\n",
                                __FUNCTION__, 
                                fbe_base_env_decode_fup_work_state(pWorkItem->workState));

        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
   
    return status;
} // End of function fbe_base_env_fup_handle_firmware_status_change

/*!**************************************************************
 * @fn fbe_base_env_fup_handle_download_status_change(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function handles the firmware status change after sending the download command.
 *  If the download completed successfully, update the work state and then set the
 *  FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION condition.
 *  If the download failed, remove the work item to complete the upgrade.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - 
 *
 * @return fbe_status_t
 *
 * @version
 *  15-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_handle_download_status_change(fbe_base_environment_t * pBaseEnv,
                                                       fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Handle DOWNLOAD status change, status/ExtStatus 0x%x/0x%x.\n",
                    pWorkItem->firmwareStatus,
                    pWorkItem->firmwareExtStatus);

    switch(pWorkItem->firmwareStatus)
    {
        case FBE_ENCLOSURE_FW_STATUS_NONE:
            if(pWorkItem->firmwareExtStatus == FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED) 
            {
                /* The image download completed successfully. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                    "DOWNLOAD Successful.\n");
     
                fbe_base_env_set_fup_work_item_state(pWorkItem,    
                                                     FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_DONE);
    
                if(pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_ACTIVATION_DEFERRED)
                {
                    fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_ACTIVATION_DEFERRED);

                    fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem); 

                    break;
                }
                else
                {
                    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE);
                
    
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                "FUP: %s Can't set FUP_ACTIVATE_IMAGE condition, status: 0x%x.\n",
                                                __FUNCTION__, status);
                    }
                }
            }
            else if(pWorkItem->firmwareExtStatus == FBE_ENCLOSURE_FW_EXT_STATUS_NONE) 
            {
                /* The image activation completed successfully. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                    "ACTIVATE Successful.\n");
            
        
                fbe_base_env_set_fup_work_item_state(pWorkItem, 
                                                     FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_DONE);
        
                status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT);
        
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                            FBE_TRACE_LEVEL_ERROR,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "FUP: %s Can't set FUP_CHECK_RESULT condition, status: 0x%x.\n",
                                            __FUNCTION__, status);
                }
            }
            break;

        case FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED:
            /* The image download failed.
             * Need to check the ext status before declaring error.
             * The enclosure object could be in the process of 
             * requesting failure info from the enclosure.
             */
            if(pWorkItem->firmwareExtStatus != FBE_ENCLOSURE_FW_EXT_STATUS_REQ)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                    "DOWNLOAD Failed.\n");
        
                if (pWorkItem->firmwareExtStatus == FBE_ENCLOSURE_FW_EXT_STATUS_ERR_CHECKSUM)
                { 
                    fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE);
                }
                else
                {
                    fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_DOWNLOAD_IMAGE);
                }
    
                fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);  
            }
            break;

        case FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED:
            /* The image activation failed.
             * Need to check the ext status before declaring error.
             * The enclosure object could be in the process of 
             * requesting failure info from the enclosure.
             */
            if(pWorkItem->firmwareExtStatus != FBE_ENCLOSURE_FW_EXT_STATUS_REQ)
            {
                /* The image activation failed. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                    "ACTIVATE Failed.\n");
    
                fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE);

                fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);  
            }
            break;

        case FBE_ENCLOSURE_FW_STATUS_ABORT:
            /* The image download aborted. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "DOWNLOAD Aborted.\n");
 
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED);

            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);   

            break;
        
        default:
            break;
    }

    return FBE_STATUS_OK;
} // End of function fbe_base_env_fup_handle_download_status_change

/*!**************************************************************
 * @fn fbe_base_env_fup_handle_activate_status_change(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_u64_t deviceType,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function This function handles the firmware status change after sending the activate command.
 *  If the activation completed successfully, update the work state and then set the
 *  FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT condition.  
 *  If the activation failed, remove the work item to complete the upgrade.
 * 
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - 
 *
 * @return fbe_status_t
 *
 * @version
 *  15-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_handle_activate_status_change(fbe_base_environment_t * pBaseEnv,
                                                       fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Handle ACTIVATE status change, status/ExtStatus 0x%d/0x%d.\n",
                    pWorkItem->firmwareStatus,
                    pWorkItem->firmwareExtStatus);

    switch(pWorkItem->firmwareStatus)
    {
        case FBE_ENCLOSURE_FW_STATUS_NONE: 
            /* The image activation completed successfully. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "ACTIVATE Successful.\n");
    
            fbe_base_env_set_fup_work_item_state(pWorkItem, 
                                                 FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_DONE);
    
            status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT);
    
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "FUP: %s Can't set FUP_CHECK_RESULT condition, status: 0x%x.\n",
                                        __FUNCTION__, status);
            }

            break;

        case FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED:
            /* The image activation failed.
             * Need to check the ext status before declaring error.
             * The enclosure object could be in the process of 
             * requesting failure info from the enclosure.
             */
            if(pWorkItem->firmwareExtStatus != FBE_ENCLOSURE_FW_EXT_STATUS_REQ)
            {
                /* The image activation failed. */
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                    "ACTIVATE Failed.\n");
    
                fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE);

                fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);  
            }
            break;
        
        default:
            break;
    }

    return FBE_STATUS_OK;
} // End of function fbe_base_env_fup_handle_activate_status_change


/*!**************************************************************
 * @fn fbe_base_env_fup_handle_destroy_state(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function loops through the work item queue to remove the work items which
 *  reside on the specified enclosure from the work item queue because the enclosure
 *  goes to destroy state.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param fbe_device_physical_location_t - The enclosure or the subenclosure
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_fup_handle_destroy_state(fbe_base_environment_t * pBaseEnv,
                                      fbe_device_physical_location_t * pLocation)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_base_env_fup_work_item_t      * pNextWorkItem = NULL;
    fbe_device_physical_location_t    * pWorkItemLocation = NULL;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "FUP: %s, Encl %d_%d.%d, Removed.\n",
                           __FUNCTION__, 
                           pLocation->bus, 
                           pLocation->enclosure,
                           pLocation->componentId);

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        fbe_spinlock_lock(pWorkItemQueueLock);
        pNextWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        if((pWorkItemLocation->bus == pLocation->bus) &&
           (pWorkItemLocation->enclosure == pLocation->enclosure) &&
           (pWorkItemLocation->componentId == pLocation->componentId)) 
        {
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED);

            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
        }      

        pWorkItem = pNextWorkItem;
    }
  
    return FBE_STATUS_OK;
} // End of function fbe_base_env_fup_handle_destroy_state.c


/*!**************************************************************
 * @fn fbe_base_env_fup_handle_fail_state(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function loops through the work item queue to remove the work items for LCC which
 *  reside on the specified enclosure from the work item queue because the enclosure
 *  goes to fail state.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param bus -
 * @param enclosure -
 *
 * @return fbe_status_t
 *
 * @version
 *  13-Oct-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_fup_handle_fail_state(fbe_base_environment_t * pBaseEnv,
                                      fbe_u32_t bus,
                                      fbe_u32_t enclosure)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_base_env_fup_work_item_t      * pNextWorkItem = NULL;
    fbe_device_physical_location_t    * pLocation = NULL;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: %s, bus %d encl %d removed.\n",
                              __FUNCTION__, bus, enclosure);

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        fbe_spinlock_lock(pWorkItemQueueLock);
        pNextWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        if((pLocation->bus == bus) &&
           (pLocation->enclosure == enclosure) &&
           (pWorkItem->deviceType == FBE_DEVICE_TYPE_LCC) &&
           (pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)) 
        {
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                             FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE);

            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
        }      

        pWorkItem = pNextWorkItem;
    }
  
    return FBE_STATUS_OK;
} // End of function fbe_base_env_fup_handle_fail_state.c


/*!*******************************************************************************
 * @fn fbe_base_env_fup_get_full_image_path
 *********************************************************************************
 * @brief
 *  This function checks whether the work item image path is populated.
 *  If not, gets  the image path from the base env image path and then
 *  appends the image file name to get the full image path. 
 *
 * @param  pBaseEnv -
 * @param  pWorkItem -
 * @param  imagePathIndex - 
 * @param  pImagePathKey - 
 * @param  pImageFileName - 
 * @param  imageFileNameSize -
 * 
 * @return fbe_status_t - 
 *
 * @version
 *  10-Aug-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t 
fbe_base_env_fup_get_full_image_path(fbe_base_environment_t * pBaseEnv,
                                     fbe_base_env_fup_work_item_t * pWorkItem,
                                     fbe_u32_t imagePathIndex,
                                     fbe_char_t * pImagePathKey,
                                     fbe_char_t * pImageFileName,
                                     fbe_u32_t imageFileNameSize)
{
    fbe_u8_t     * pWorkItemImagePath = NULL;
    fbe_u8_t     * pImagePath = NULL;
    fbe_status_t   status = FBE_STATUS_OK;

    /* Get the work item image path. */
    pWorkItemImagePath = fbe_base_env_get_fup_work_item_image_path_ptr(pWorkItem);

    if(!fbe_equal_memory(pWorkItemImagePath, "NULL", 4))
    {
        /* The image path is already there, no need to get it again. */
        return FBE_STATUS_OK;
    }
   
    /* Get the image path in base_environment. */
    pImagePath = fbe_base_env_get_fup_image_path_ptr(pBaseEnv, imagePathIndex);

    if(fbe_equal_memory(pImagePath, "NULL", 4))
    {
        /* The base_environment image path was not initialized or the previous op failed.
         * Read the base environment image path from the registry.
         */
        status = fbe_base_env_fup_get_image_path_from_registry(pBaseEnv, 
                                            pImagePathKey, 
                                            pImagePath,
                                            FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH);
    
        if(status == FBE_STATUS_OK)
        {       
            if(fbe_equal_memory(pImagePath, "NULL", 4))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "NULL image path in the registry.\n");

                return FBE_STATUS_NOT_INITIALIZED;
            }
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "Get image path from registry failed.\n");

            /* Failed to read image path from registry. */
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Copy the image path in base_environment to the work item image path. */
    fbe_copy_memory(pWorkItemImagePath, pImagePath, FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH);

    /* If there is no need to append the file name, the imageFileNameSize should be 0. */
    if(imageFileNameSize > 0) 
    {
#ifndef ALAMOSA_WINDOWS_ENV
        /* Before appending the file name to the folder path, first the "/" */
        strncat(pWorkItemImagePath, "/", 2);
#else
        /* Before appending the file name to the folder path, first the "\" */
        strncat(pWorkItemImagePath, "\\", 2);
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
        /* Append the image file name. */
        strncat(pWorkItemImagePath, pImageFileName, imageFileNameSize);
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "Full image path %s.\n",
                                        pWorkItemImagePath);

    return(FBE_STATUS_OK);

} //End of function fbe_base_env_fup_get_full_image_path

/*!*******************************************************************************
 * @fn fbe_base_env_fup_get_manifest_file_full_path
 *********************************************************************************
 * @brief
 *  This function gets the manifest file path from the registery and
 *  appends the manifest file name.  
 *
 * @param  pBaseEnv (INPUT) -
 * @param  pManifestFilePathKey (INPUT) -
 * @param  pManifestFileName (INPUT) -
 * @param  manifestFileNameSize (INPUT) -
 * @param  pManifestFileFullPath (OUTPUT) -
 * 
 * @return fbe_status_t - 
 *
 * @version
 *  12-Sep-2014 PHE -- Created.
 *******************************************************************************/
fbe_status_t 
fbe_base_env_fup_get_manifest_file_full_path(fbe_base_environment_t * pBaseEnv,
                                             fbe_char_t * pManifestFilePathKey,
                                             fbe_char_t * pManifestFileName,
                                             fbe_u32_t manifestFileNameSize,
                                             fbe_char_t * pManifestFileFullPath)
{
    fbe_status_t   status = FBE_STATUS_OK;
    
    status = fbe_base_env_fup_get_image_path_from_registry(pBaseEnv, 
                                        pManifestFilePathKey, 
                                        pManifestFileFullPath,
                                        FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH);

    if(status == FBE_STATUS_OK)
    {       
        if(fbe_equal_memory(pManifestFileFullPath, "NULL", 4))
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: manifest file path in regisitry is NULL.\n");

            return FBE_STATUS_NOT_INITIALIZED;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: failed to get manifest file path from registry.\n");

        /* Failed to read image path from registry. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If there is no need to append the file name, the imageFileNameSize should be 0. */
    if(manifestFileNameSize > 0) 
    {
#ifndef ALAMOSA_WINDOWS_ENV
        /* Before appending the file name to the folder path, first the "/" */
        strncat(pManifestFileFullPath, "/", 2);
#else
        /* Before appending the file name to the folder path, first the "\" */
        strncat(pManifestFileFullPath, "\\", 2);
#endif /* ALAMOSA_WINDOWS_ENV - PATHS */
        /* Append the image file name. */
        strncat(pManifestFileFullPath, pManifestFileName, manifestFileNameSize);
    }

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: Manifest file full path %s.\n",
                              pManifestFileFullPath);

    return(FBE_STATUS_OK);

} //End of function fbe_base_env_fup_get_manifest_file_full_path

/*!**************************************************************
 * fbe_base_env_fup_processGotPeerPerm()
 ****************************************************************
 * @brief
 *  Permission granted by the peer. Continue with the firmware
 *  update.
 *
 * @param pBaspBaseEnveObj - poiner to fbe_base_environment_t
 * @param pCmiMsg - the cmi packet received
 *
 * @return fbe_status_t - fbe_base_environment_send_cmi_message
 * always returns FBE_STATUS_OK
 *
 * @author
 *  21-Jan-2011 Created GB
 ****************************************************************/
fbe_status_t fbe_base_env_fup_processGotPeerPerm(fbe_base_environment_t * pBaseEnv, 
                                                 fbe_base_env_fup_work_item_t *pWorkItem)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (fbe_base_env_fup_work_item_pointer_is_valid(pBaseEnv, pWorkItem))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                            "Got peer permission.\n");
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                            "FUP:%s Invalid work item pointer, skip.\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    if(pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED)
    {
        pWorkItem->cmdStartTimeStamp = 0;
        fbe_base_env_set_fup_work_item_state(pWorkItem, FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_RECEIVED);
    
        // set the condition to check env
        status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS);
        
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "FUP:%s Can't set FUP_CHECK_ENV_STATUS condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
        }
    }
    return(status);
} // fbe_base_env_fup_processGotPeerPerm

/*!**************************************************************
 * fbe_base_env_fup_processDeniedPeerPerm()
 ****************************************************************
 * @brief
 *  Permission denied received by the peer. Remove the item from
 *  the work queue and set the work item completion status and
 *  return FBE_STATUS_OK which indicates no leaf class
 *  processing required.
 *
 * @param pBaseEnv - 
 * @param pWorkItem - 
 *
 * @return fbe_status_t - fbe_base_environment_send_cmi_message
 * always returns FBE_STATUS_OK
 *
 * @author
 *  21-Jan-2011 Created GB
 ****************************************************************/
fbe_status_t fbe_base_env_fup_processDeniedPeerPerm(fbe_base_environment_t * pBaseEnv, 
                                                    fbe_base_env_fup_work_item_t *pWorkItem)
{
    if (fbe_base_env_fup_work_item_pointer_is_valid(pBaseEnv, pWorkItem))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                            "Peer permission denied or no peer permission.\n");
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                            "FUP:%s Invalid work item pointer, skip.\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    if(pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED)
    {
        // denied means the peer detected a problem and this side should not upgrade 
        fbe_base_env_set_fup_work_item_completion_status(pWorkItem, 
                                                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION); 
    
        // done complete processing for the work item
        fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
    }

    return FBE_STATUS_OK;
} //fbe_base_env_fup_processDeniedPeerPerm

/*!**************************************************************
 * fbe_base_env_fup_processSpContactLost()
 ****************************************************************
 * @brief
 *  This function gets called when when Peer SP is dead and this local received the event with SP_CONTACT_LOST.
 *  Get the first work item in the queue. If it is still waiting for the peer permission, complete this work item.
 *
 * @param pBaseEnv - 
 * @param pWorkItem - 
 *
 * @return fbe_status_t - always returns FBE_STATUS_OK
 *
 * @author
 *  11-Dec-2014 PHE - Created
 ****************************************************************/
fbe_status_t fbe_base_env_fup_processSpContactLost(fbe_base_environment_t * pBaseEnv)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    /* Get the first work item in the queue.It is only possible for the first work item to be in  
     * the work state of FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED because we do the upgrade in serial.
     */
    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    if((pWorkItem != NULL) && 
       (pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED)) 
    {
        /* This work item was waiting for the peer permission. Let's complete ths work item.
         * The upgrade will be re-initiated when peer SP comes alive.
         */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                            "waiting for peer permission but peer SP contact lost, will complete this work item.\n");

        fbe_base_env_set_fup_work_item_completion_status(pWorkItem, FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION);

        fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
    }
  
    return FBE_STATUS_OK;
} //fbe_base_env_fup_processSpContactLost

/*!***************************************************************
 * fbe_base_env_fup_fill_and_send_cmi()
 ****************************************************************
 * @brief
 *  Fill the "user msg" struct and send to peer via cmi. This function
 *  is specific to sending fup related messages that use the
 *  fbe_base_env_fup_cmi_packet_t template.
 * 
 *  This is a common utility function that all leaf class fup objects
 *  can call this function to fill the message and send the message.
 *
 * @param pBaseEnv - base env 
 * @param msgType - the message type code
 * @param pWorkItem - work item pointer of the requestor
 * 
 * @return
 *      FBE_STATUS_INSUFFICIENT_RESOURCES - no memory available
 *      FBE_STATUS_OK
 *
 * @author
 *  17-Feb-2011: GB - Created. 
 ****************************************************************/
fbe_status_t fbe_base_env_fup_fill_and_send_cmi(fbe_base_environment_t *pBaseEnv, 
                                                fbe_base_environment_cmi_message_opcode_t msgType, 
                                                fbe_u64_t deviceType,
                                                fbe_device_physical_location_t * pLocation,
                                                fbe_base_env_fup_work_item_t *pWorkItem)
{
    fbe_status_t                    status;
    fbe_base_env_fup_cmi_packet_t   fupCmiMsg = {0};

    fupCmiMsg.baseCmiMsg.opcode = msgType;
    fupCmiMsg.deviceType = deviceType;
    fupCmiMsg.location = *pLocation;
    fupCmiMsg.pRequestorWorkItem = pWorkItem;

    /*Split trace into two lines*/
    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP:%s(%d_%d_%d) FupFillSendCmi>\n",
                          fbe_base_env_decode_device_type(deviceType),
                          fupCmiMsg.location.bus,
                          fupCmiMsg.location.enclosure,
                          fupCmiMsg.location.slot);
    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "msg:%s(0x%x) workitem:%p<\n",
                          fbe_base_env_decode_cmi_msg_opcode(msgType),
                          msgType,
                          pWorkItem);

    // send the message
    status = fbe_base_environment_send_cmi_message(pBaseEnv,
                                                   sizeof(fbe_base_env_fup_cmi_packet_t),
                                                   &fupCmiMsg);
    return(status);
    
} //fbe_base_env_fup_fill_and_send_cmi

/*!*************************************************************************
 *  @fn fbe_base_env_fup_send_cmd()
 **************************************************************************
 *  @brief
 *      This function sends the firmware upgrade related command to a particular
 *      device.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param pWorkItem - The pointer to the work item. 
 *  @param firmwareOpCode - The firmware opcode.
 * 
 *  @return  fbe_status_t
 *
 *  @note The EMA supports only one download microcode process at a time.
 *        It is the physical package's responsibility to prevent multiple microcode process or
 *        or notify ESP the failure when it does happen.
 *
 *  @version
 *  16-June-2010 PHE - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_base_env_fup_send_cmd(fbe_base_environment_t * pBaseEnv,
                                   fbe_base_env_fup_work_item_t * pWorkItem,
                                   fbe_enclosure_firmware_opcode_t firmwareOpCode)
{
    fbe_enclosure_mgmt_download_op_t download_op;
    fbe_object_id_t object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_fup_image_info_t * pImageInfo = NULL;
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_topology_control_get_enclosure_by_location_t enclByLocation = {0};

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "Sending %s cmd.\n", 
                    fbe_base_env_decode_firmware_op_code(firmwareOpCode));
                    
    fbe_zero_memory(&download_op, sizeof(fbe_enclosure_mgmt_download_op_t));

    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);
    pImageInfo = fbe_base_env_get_fup_work_item_image_info_ptr(pWorkItem);

    if((pWorkItem->location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (pWorkItem->location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
    {
        // test location for SPS connected to proc encl
        // use the esp obj id directly for board object (SPS attached to pe)
        status = fbe_api_get_board_object_id(&object_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                               "%s, Failed to get board object Id, status 0x%x.\n",   
                                               __FUNCTION__,
                                               status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        if (firmwareOpCode == FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION ||
             firmwareOpCode ==FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION)
        {
            status = fbe_api_get_board_object_id(&object_id);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                               FBE_TRACE_LEVEL_INFO,
                                               fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                               "%s, Failed to get board object Id, status 0x%x.\n",   
                                               __FUNCTION__,
                                               status);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            download_op.location = pWorkItem->location;
        }
        else
        {
            status = fbe_api_get_enclosure_object_ids_array_by_location(pLocation->bus, 
                                                                        pLocation->enclosure, 
                                                                        &enclByLocation);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "%s, Failed to get object Id, status 0x%x.\n",   
                            __FUNCTION__,
                            status);   
        
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else if((pLocation->componentId == 0) || (pWorkItem->deviceType == FBE_DEVICE_TYPE_SPS))
            {
                object_id = enclByLocation.enclosure_object_id;
            }
            else
            {
                object_id = enclByLocation.comp_object_id[pLocation->componentId];
            }
        }
    }
    
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                "%s, Encl %d_%d, get object id 0x%X.\n",   
                __FUNCTION__,
                pLocation->bus,
                pLocation->enclosure,
                object_id);

    if(FBE_OBJECT_ID_INVALID == object_id)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                    FBE_TRACE_LEVEL_INFO,
                    fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                    "%s, Invalid object Id.\n",   
                    __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    download_op.op_code = firmwareOpCode;
    download_op.target = pWorkItem->firmwareTarget;
    if (pWorkItem->deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE ||
        pWorkItem->deviceType == FBE_DEVICE_TYPE_SP)
    {
        download_op.side_id = pLocation->sp; 
    }
    else 
    {
        download_op.side_id = pLocation->slot; 
    }

    switch(firmwareOpCode) 
    {
        case FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD:
            download_op.image_p = pImageInfo->pImage;
            download_op.size = pImageInfo->imageSize;
            download_op.header_size = pImageInfo->headerSize;
        
            if(pWorkItem->forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK) 
            {
                download_op.checkPriority = FALSE;
            }
            else
            {
                download_op.checkPriority = TRUE;
            }
            break;
    
        case FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE:
        case FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS:
        case FBE_ENCLOSURE_FIRMWARE_OP_ABORT:
        case FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION:
        case FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION:
        case FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION:
            download_op.image_p = NULL;
            download_op.size = 0;
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                            "base_env_fup_send_cmd, unknown firmwareOpCode %d.\n",firmwareOpCode);

            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    status = fbe_api_enclosure_firmware_download(object_id, &download_op);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "base_env_fup_send_cmd, %s cmd, encl_firmware_download failed, status 0x%x.\n",   
                        fbe_base_env_decode_firmware_op_code(firmwareOpCode), 
                        status);
        return status;
    }

    if(firmwareOpCode == FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS)
    {
        pWorkItem->firmwareStatus = download_op.status;
        pWorkItem->firmwareExtStatus = download_op.extended_status;
    }
    else if (firmwareOpCode == FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION)
    {
        if (download_op.permissionGranted != TRUE)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "base_env_fup_send_cmd, fail to get local permission to download firmware.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_base_env_fup_check_peer_pending()
 ****************************************************************
 * @brief
 *  If the work item is waiting for peer permission, send the
 *  permission granted message.
 * 
 *  This is a leaf class function because the peer permission message
 *  is not common to all, it is specific to the leaf class object.
 *
 * @param pContext -
 *  pWorkItem - the workitem to check
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Feb-2011: GB - Created. 
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_check_peer_pending(fbe_base_environment_t * pBaseEnv,
                                                fbe_base_env_fup_work_item_t *pWorkItem)
{
    fbe_base_env_fup_cmi_packet_t    *  pFupCmiPacket = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;

    pFupCmiPacket = &pWorkItem->pendingPeerRequest;

    // fill the msg details
    if (pFupCmiPacket->baseCmiMsg.opcode == FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST)
    {
        status = fbe_base_env_fup_fill_and_send_cmi(pBaseEnv,
                                    FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT, 
                                    pFupCmiPacket->deviceType,
                                    &pFupCmiPacket->location,
                                    pFupCmiPacket->pRequestorWorkItem);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                FBE_TRACE_LEVEL_INFO,
                fbe_base_env_get_fup_work_item_logheader(pFupCmiPacket->pRequestorWorkItem),
                "%s, fbe_base_env_fup_fill_and_send_cmi failed.\n",   
                __FUNCTION__);
        }
    }
    return(status);
} //fbe_base_env_fup_check_peer_pending


/*!***************************************************************
 * fbe_base_env_fup_peerPermRetry()
 ****************************************************************
 * @brief
 *  Set the work state and lifecycle condition that will result
 *  in sendig a peer permission request. The request will get
 *  processed when the condition is triggered.
 * 
 * 
 * @param pBaseEnv -
 * @param pWorkItem - the work item to change the work state
 * 
 * @return fbe_status_t
 *
 * @author
 *  17-Mar-2011: GB - Created. 
 ****************************************************************/
fbe_status_t fbe_base_env_fup_peerPermRetry(fbe_base_environment_t *pBaseEnv,
                                            fbe_base_env_fup_work_item_t *pWorkItem)
{
    fbe_status_t status = FBE_STATUS_OK;
  
    // set work state to image done and set the condition to get_peer_perm
    // in order to re send peer perm req
    fbe_base_env_set_fup_work_item_state(pWorkItem,
                                         FBE_BASE_ENV_FUP_WORK_STATE_READ_ENTIRE_IMAGE_DONE);

    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t *)pBaseEnv,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                           "FUP: %s Can't set FUP_GET_PER_PERM condition, status: 0x%x.\n",   
                                           __FUNCTION__, 
                                           status);
    }

    return(status);
} //fbe_base_env_fup_peerPermRetry

/*!***************************************************************
 * fbe_base_env_fup_get_image_file_name_uid_string()
 ****************************************************************
 * @brief
 *  Get the part of the image file name which is related to the uniqueId.
 * 
 * @param pImageFileNameString -
 * @param bufferSize
 * @param uuid - the uniqueId.
 * 
 * @return fbe_status_t
 *
 * @author
 *  17-Mar-2011: GB - Created. 
 ****************************************************************/
fbe_status_t fbe_base_env_fup_get_image_file_name_uid_string(char *pIdString, fbe_u32_t bufferSize, fbe_u32_t uuid)
{
    unsigned char *v = NULL;
    char chi;
    char clo;
    fbe_u32_t   valueSize;
    fbe_u32_t   swappedValue;
    
    
    // swapping should happen in pp
    //swappedValue = (((uuid & 0xffff0000) >> 16) | ((uuid & 0x0000ffff) << 16));
    swappedValue = swap32(uuid);
    v = (unsigned char *)&swappedValue;
    
    valueSize = sizeof(fbe_u32_t);
    if((2*valueSize) > bufferSize)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    while (valueSize > 0) {
        chi = (char) (((*v) & 0xf0) >> 4);
        clo = (char) (((*v) & 0x0f) >> 0);
        chi += '0';
        if (chi > '9') {
            chi += 7;
        }
        clo += '0';
        if (clo > '9') {
            clo += 7;
        }
        *pIdString++ = chi;
        *pIdString++ = clo;
        v++;
        valueSize--;
    }
    return FBE_STATUS_OK;
}
//End of file fbe_base_environment_fup.c


/*!*************************************************************************
 *  @fn fbe_base_env_fup_set_condition()
 **************************************************************************
 *  @brief
 *      This function sets the condition based on the current work state of the first work item in the queue.
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param condId - fbe_lifecycle_cond_id_t
 * 
 *  @return  fbe_status_t
 *
 *  @version
 *  28-Jan-2013 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_fup_set_condition(fbe_base_environment_t *pBaseEnv,
                                            fbe_lifecycle_cond_id_t condId)
{
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_bool_t                          setCondition = FBE_FALSE;
    fbe_status_t                        status = FBE_STATUS_OK;

    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);
  
    if((condId == FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS) ||
       (condId == FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS) ||
       (condId == FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS) ||
       (condId == FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE) ||
       (condId == FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE) || 
       (condId == FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE))
    {
        /* These conditions have the priority higher than or equal to the abort upgrade condition.
         *  
         * Need to set COND_FUP_GET_DOWNLOAD_STATUS even if the abort upgrade condition is set.
         * Because when the abort_upgrade_cond is set and the abort command is sent, 
         * the get_download_status_status_cond still needs to be excuted to get the download status
         * as ABORTED and then transition the fup completion status to ABORTED 
         *  
         * Need to set COND_FUP_GET_DOWNLOAD_STATUS even if the abort upgrade condition is set.
         * In this way, we can check the activate status when it is done and then 
         * transition the fup completion status to ABORTED.
         *
         * Need to set COND_FUP_RELEASE_IMAGE even if the abort upgrade condition is set.
         * Otherwise, it could be possible that the image would not be released if all the work items
         * are done while the abort upgrade condition gets set.
         */
        setCondition = FBE_TRUE;
    }
    else
    {
        fbe_spinlock_lock(pWorkItemQueueLock);
        if(pBaseEnv->abortUpgradeCmdReceived)
        {
            fbe_spinlock_unlock(pWorkItemQueueLock);
            /* Do not set the condition. If we set it here, when the abort upgrade condition gets cleared,
             * this condition might get to run which makes the upgrade out of sequence.
             * abortUpgradeCmdReceived will be cleared when the resume upgrade command is recieved.
             */
            setCondition = FBE_FALSE;
        }
        else
        {
            fbe_spinlock_unlock(pWorkItemQueueLock);
            setCondition = FBE_TRUE;
        }
    }
  
    if(setCondition) 
    {
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                    (fbe_base_object_t *)pBaseEnv,
                                    condId);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP:%s, Can't set cond(0x%X), status: 0x%X.\n",
                          __FUNCTION__, condId, status);
            
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP:%s, set cond(0x%X) successfully.\n",
                          __FUNCTION__, condId);

        }
#ifndef ALAMOSA_WINDOWS_ENV
        csx_p_thr_superyield();
#endif
    }
    
    return status;
}

/*!**************************************************************
 * fbe_base_env_fup_process_peer_activate_start()
 ****************************************************************
 * @brief
 *  This function gets called when receiving the CMI message from
 *  the peer SP to know that the device firmware upgrade initiated
 *  by the peer SP starts activating the image. So this SP can set the flag
 *  to ignore the device fault caused by the firmware upgrade
 *  initiated by the peer SP. Because the fault could be caused due to the firmware activation.
 *
 * @param pBaseEnv - 
 * @param deviceType -
 * @param pLocation -  
 *
 * @return fbe_status_t - always returns FBE_STATUS_OK
 *
 * @author
 *  24-Jun-2014 PHE - Created.
 ****************************************************************/
fbe_status_t fbe_base_env_fup_process_peer_activate_start(fbe_base_environment_t * pBaseEnv, 
                                                          fbe_u64_t deviceType,
                                                          fbe_device_physical_location_t *pLocation)
{
    fbe_base_env_fup_persistent_info_t  *pFupInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),PEER_FUP_SUPPRESS_FLT start, due to peer FUP activation start.\n",
                              fbe_base_env_decode_device_type(deviceType),
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot);

    status = pBaseEnv->fup_element_ptr->pGetFupInfoPtrCallback(pBaseEnv, 
                                                               deviceType,
                                                               pLocation,
                                                               &pFupInfo);
    if (status != FBE_STATUS_OK)
    {
        // encl info not found. The enclosure may not present on this side. Just return FBE_STATUS_OK.
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),PEER_FUP_SUPPRESS_FLT start, pGetFupInfoPtrCallback failed.\n",
                              fbe_base_env_decode_device_type(deviceType),
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot);

        return FBE_STATUS_OK;
    }

    pFupInfo->suppressFltDueToPeerFup = FBE_TRUE;
    
    return FBE_STATUS_OK;

} //fbe_base_env_fup_process_peer_activate_start


/*!**************************************************************
 * fbe_base_env_fup_process_peer_upgrade_end()
 ****************************************************************
 * @brief
 *  This function gets called when receiving the CMI message from
 *  the peer SP to know that the device firmware upgrade initiated by the peer SP ends.
 *  This SP can clear the flag so that the device fault detected in the future will NOT be ignored.
 *  
 * @param pBaseEnv - 
 * @param deviceType -
 * @param pLocation - 
 *
 * @return fbe_status_t - always returns FBE_STATUS_OK
 *
 * @author
 *  24-Jun-2014 PHE - Created.
 ****************************************************************/
fbe_status_t fbe_base_env_fup_process_peer_upgrade_end(fbe_base_environment_t * pBaseEnv, 
                                                       fbe_u64_t deviceType,
                                                       fbe_device_physical_location_t *pLocation)
{
    fbe_base_env_fup_persistent_info_t  *pFupInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),PEER_FUP_SUPPRESS_FLT end, due to peer FUP end.\n",
                              fbe_base_env_decode_device_type(deviceType),
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot);

    status = pBaseEnv->fup_element_ptr->pGetFupInfoPtrCallback(pBaseEnv, 
                                                               deviceType,
                                                               pLocation,
                                                               &pFupInfo);
    if (status != FBE_STATUS_OK)
    {
        // encl info not found. The enclosure may not present on this side. Just return FBE_STATUS_OK.
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d),PEER_FUP_SUPPRESS_FLT end, pGetFupInfoPtrCallback failed.\n",
                              fbe_base_env_decode_device_type(deviceType),
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot);
        
        return FBE_STATUS_OK;
    }

    pFupInfo->suppressFltDueToPeerFup = FBE_FALSE;
    
    return FBE_STATUS_OK;

} //fbe_base_env_fup_process_peer_upgrade_end

/*!*************************************************************************
 * @fn fbe_base_env_fup_fwUpdatedFailed()
 **************************************************************************
 * @brief
 *      This function will update the appropriate component's
 *      FUP failed status.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWriteResumePromCmd - The pointer to the resume prom cmd.
 * @param pCmdValid (output)
 *
 * @return  fbe_status_t
 *
 * @version
 *  3-Mar-2014  Joe Perry - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_env_fup_fwUpdatedFailed(fbe_base_environment_t * pBaseEnv,
                                 fbe_u64_t deviceType,
                                 fbe_device_physical_location_t *pLocation,
                                 fbe_bool_t newValue)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t    *mgmtPtr;

    mgmtPtr = (fbe_u8_t *)pBaseEnv;

    // determine the device type
    switch (deviceType)
    {
    case FBE_DEVICE_TYPE_PS:
        fbe_ps_mgmt_updatePsFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_FUP_FAILURE, newValue);
        break;
    case FBE_DEVICE_TYPE_BATTERY:
        fbe_sps_mgmt_updateBbuFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_FUP_FAILURE, newValue);
        break;
    case FBE_DEVICE_TYPE_FAN:
        fbe_cooling_mgmt_updateFanFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_FUP_FAILURE, newValue);
        break;
    case FBE_DEVICE_TYPE_LCC:
        fbe_encl_mgmt_updateLccFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_FUP_FAILURE, newValue);
        break;
    default:
        break;
    }

    return status;

}   // end of fbe_base_env_fup_fwUpdatedFailed

/*!*******************************************************************************
 * @fn fbe_base_env_fup_find_file_name_from_manifest_info
 *********************************************************************************
 * @brief
 *   This function is to find the image file name for the work item from manifest info.
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param pWorkItem -
 * @param pImageFileNamePtr -
 * 
 * @return fbe_status_t
 *
 * @version
 *   01-Aug-2014 PHE - Created.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_find_file_name_from_manifest_info(fbe_base_environment_t * pBaseEnv, 
                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                      fbe_char_t ** pImageFileNamePtr)
{
    fbe_u32_t subEnclIndex = 0;
    fbe_u32_t imageIndex = 0;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;

    for(subEnclIndex = 0; subEnclIndex < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclIndex ++)
    {
        pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, subEnclIndex);

        if(strncmp(&pManifestInfo->subenclProductId[0], 
                   &pWorkItem->subenclProductId[0],
                   FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE) == 0)
        {
            for(imageIndex = 0; imageIndex < pManifestInfo->imageFileCount; imageIndex ++)
            {
                if(pManifestInfo->firmwareTarget[imageIndex] == pWorkItem->firmwareTarget)
                {
                    *pImageFileNamePtr = &pManifestInfo->imageFileName[imageIndex][0]; 
                    return FBE_STATUS_OK;
                }
            }
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_fw_comp_type_to_fw_target
 *********************************************************************************
 * @brief
 *   This function is to convert the firmware component type to firmware target.
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param firmwareCompType -
 * @param pFirmwareTarget (OUTPUT)-
 * 
 * @return fbe_status_t
 *
 * @version
 *   01-Aug-2014 PHE - Created.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_fw_comp_type_to_fw_target(fbe_base_environment_t * pBaseEnv, 
                                      ses_comp_type_enum firmwareCompType,
                                      fbe_enclosure_fw_target_t *pFirmwareTarget)
{
    switch(firmwareCompType)
    {
        case SES_COMP_TYPE_EXPANDER_FW_EMA:
            *pFirmwareTarget = FBE_FW_TARGET_LCC_EXPANDER;
            break;
        case SES_COMP_TYPE_INIT_STR:
            *pFirmwareTarget = FBE_FW_TARGET_LCC_INIT_STRING;
            break;
        case SES_COMP_TYPE_FPGA_IMAGE:
            *pFirmwareTarget = FBE_FW_TARGET_LCC_FPGA;
            break;
        case SES_COMP_TYPE_PS_FW:
            *pFirmwareTarget = FBE_FW_TARGET_PS;
            break;
        default:
            *pFirmwareTarget = FBE_FW_TARGET_INVALID;
            break;
    }
    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_convert_fw_rev_for_rp
 *********************************************************************************
 * @brief
 *   This function is to convert the firmware reve to the resume prom programmable rev format.
 *   The resume prom programmable rev has only 4 bytes. So we just save 2 bytes for major number
 *   and 2 bytes for the minor number. 
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param pFwRevForResume (OUTPUT)- The pointer to the firmware rev used in the resume prom.
 * @param pFwRev (INPUT) - The pointer to the firmware rev to be converted.
 * 
 * @return fbe_status_t
 *
 * @version
 *   14-Aug-2014 PHE - Created.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_convert_fw_rev_for_rp(fbe_base_environment_t * pBaseEnv, 
                                                    fbe_u8_t * pFwRevForResume,
                                                    fbe_u8_t * pFwRev)
{
    fbe_u32_t i = 0; // for loop.
    fbe_base_env_fup_format_fw_rev_t firmwareRevAfterFormatting = {0};

    fbe_zero_memory(&firmwareRevAfterFormatting, sizeof(fbe_base_env_fup_format_fw_rev_t));
    fbe_zero_memory(pFwRevForResume, RESUME_PROM_PROG_REV_SIZE);
       
    fbe_base_env_fup_format_fw_rev(&firmwareRevAfterFormatting, 
                                   pFwRev, 
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    fbe_sprintf(pFwRevForResume, 
                (RESUME_PROM_PROG_REV_SIZE + 1), 
                "%2d%2d", 
                firmwareRevAfterFormatting.majorNumber,
                firmwareRevAfterFormatting.minorNumber);

    for(i = 0; i < RESUME_PROM_PROG_REV_SIZE; i ++) 
    {
        if(pFwRevForResume[i] == ' ')
        {
            pFwRevForResume[i] = '0';
        }
    }

    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_read_and_parse_manifest_file
 *********************************************************************************
 * @brief
 *   This function is to read the manifest file and parse it to populate the data structure.
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param pFwRevForResume (OUTPUT)- The pointer to the firmware rev used in the resume prom.
 * @param pFwRev (INPUT) - The pointer to the firmware rev to be converted.
 * 
 * @return fbe_status_t
 *
 * @version
 *   29-Aug-2014 PHE - Created.
 *******************************************************************************/
fbe_status_t fbe_base_env_fup_read_and_parse_manifest_file(fbe_base_environment_t *pBaseEnv)
{
    fbe_char_t                  * pJson = NULL;
    fbe_u32_t                   bytesRead = 0;
    fbe_u32_t                   fileSize = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_char_t                  manifestFileFullPath[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH] = {0};  /* Includes the file name. */

    fbe_base_env_fup_set_manifest_info_avail(pBaseEnv, FBE_FALSE);

    if(pBaseEnv->fup_element_ptr->pGetManifestFileFullPathCallback == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "FUP: read_and_parse_manifest_file, NULL pointer for the callback.\n");
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = pBaseEnv->fup_element_ptr->pGetManifestFileFullPathCallback(pBaseEnv, &manifestFileFullPath[0]);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "FUP: read_and_parse_manifest_file, failed to get manifest file full path.\n");
        
       return status;
    }

    status = fbe_base_env_get_file_size(pBaseEnv, 
                                        &manifestFileFullPath[0], 
                                        &fileSize);
    if(status != FBE_STATUS_OK)    
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "FUP: read_and_parse_manifest_file, failed to get file size, status 0x%X.\n",
                               status);
        
        return status;        
    }

    pJson = (fbe_char_t *)fbe_base_env_memory_ex_allocate(pBaseEnv, fileSize);
    if(pJson == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "FUP: read_and_parse_manifest_file, failed to alloc mem, size:%d .\n", fileSize);
        
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(pJson, fileSize);
    
    /* Read the file. */
    status  = fbe_base_env_file_read(pBaseEnv, &manifestFileFullPath[0], pJson, fileSize, FBE_FILE_RDONLY, &bytesRead);
  
    if(status != FBE_STATUS_OK)    
    {
       fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: read_and_parse_manifest_file, failed to read file, status 0x%X.\n",
                              status);
        
        fbe_base_env_memory_ex_release(pBaseEnv, pJson);

        return status;        
    }

    status = fbe_base_env_fup_parse_manifest_file(pBaseEnv, pJson, bytesRead);

    if(status != FBE_STATUS_OK)    
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "FUP: read_and_parse_manifest_file, failed to parse manifest file, status 0x%X.\n",
                               status);

        fbe_base_env_memory_ex_release(pBaseEnv, pJson);
    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_env_fup_set_manifest_info_avail(pBaseEnv, FBE_TRUE);

    fbe_base_env_memory_ex_release(pBaseEnv, pJson);
    
    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_base_env_fup_parse_manifest_file
 *********************************************************************************
 * @brief
 *   This function is to parse the manifest file and populate the data structure.
 *
 * @param pBaseEnv - The pointer to the base env.
 * @param pJson - The Manifest Json data.
 * @param bytesRead - The size of the manifest jon file.
 * 
 * @return fbe_status_t
 *
 * @version
 *   29-Aug-2014 PHE - Created.
 *******************************************************************************/
static fbe_status_t fbe_base_env_fup_parse_manifest_file(fbe_base_environment_t * pBaseEnv, 
                                                         fbe_char_t * pJson, 
                                                         fbe_u32_t bytesRead)
{
    jsmn_parser_t                 parser;
    jsmn_tok_t                  * pTokens = NULL;
    jsmn_tok_t                  * pKey = NULL;
    jsmn_err_t                    err = JSMN_ERROR_NONE;
    fbe_u32_t                     i = 0; // for looping through all the tokens.
    fbe_u32_t                     j = 0; // for looping through the subenclosures.
    fbe_u32_t                     index = 0;   // Index to the manifestInfo array.
    fbe_u32_t                     subEnclStart = 0;
    fbe_u32_t                     subEnclEnd = 0;
    fbe_u32_t                     tokenCount = 0;
    fbe_u32_t                     len = 0;
    fbe_char_t                    fwCompTypeStr[FBE_BASE_ENV_FUP_COMP_TYPE_STR_SIZE + 1] = {0};
    ses_comp_type_enum            firmwareCompType = SES_COMP_TYPE_UNKNOWN;
    fbe_enclosure_fw_target_t     firmwareTarget = FBE_FW_TARGET_INVALID;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;

    pTokens = (jsmn_tok_t *)fbe_base_env_memory_ex_allocate(pBaseEnv, FBE_BASE_ENV_FUP_JSON_MAX_NUM_TOKENS * sizeof(jsmn_tok_t));
    if(pTokens == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "FUP: parse_manifest_file, failed to alloc mem for tokens.\n");
        
        /* Return the bad status other than FBE_STATUS_OK so that it will retry. */
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    fbe_zero_memory(pTokens, FBE_BASE_ENV_FUP_JSON_MAX_NUM_TOKENS * sizeof(jsmn_tok_t));

    jsmn_init(&parser);

    err = jsmn_parse(&parser, pJson, bytesRead, pTokens, FBE_BASE_ENV_FUP_JSON_MAX_NUM_TOKENS, &tokenCount);
    if(err != JSMN_ERROR_NONE)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "FUP: parse_manifest_file, Error parsing JSON, err %d.\n",
                          err);

        fbe_base_env_memory_ex_release(pBaseEnv, pTokens);

        return FBE_STATUS_GENERIC_FAILURE;    
    }

    for(i = 0; i < tokenCount; ++i)
    {
        pKey = &pTokens[i];
        if (((pKey->end - pKey->start) < 0) ||
            (strncmp("", &pJson[pKey->start], (size_t) (pKey->end - pKey->start)) == 0))
        {
            fbe_base_env_memory_ex_release(pBaseEnv, pTokens);
           
            return FBE_STATUS_OK;
        }

        if((pKey->type == JSMN_OBJECT) || 
           (pKey->type == JSMN_ARRAY))
        {
            continue;
        }

        if (strncmp("subenclosures", &pJson[pKey->start], (size_t) (pKey->end - pKey->start)) == 0)
        {
            i = i + 2;  // Skip the next JSMN_ARRAY type
            pKey = &pTokens[i];

            subEnclStart = index;
            while(strncmp("files", &pJson[pKey->start], (size_t) (pKey->end - pKey->start)) != 0) 
            {
                if(index >= FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST)
                {
                    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "FUP: parse_manifest_file, subEncl count exceeded, max %d.\n",
                               FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL);

                    fbe_base_env_memory_ex_release(pBaseEnv, pTokens);

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, index);
                len = ((pKey->end - pKey->start) < FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE ) ? 
                            (size_t) (pKey->end - pKey->start): FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE; 
                fbe_copy_memory(&pManifestInfo->subenclProductId[0],
                                &pJson[pKey->start], 
                                len);
                pKey = &pTokens[++i];
                index ++;
            }

            subEnclEnd = index;
            continue;
        }

        if (strncmp("image", &pJson[pKey->start], (size_t) (pKey->end - pKey->start)) == 0)
        {
            pKey = &pTokens[++i];

            for(j = subEnclStart; j < subEnclEnd; j++) 
            {
                pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, j);

                pManifestInfo->imageFileCount++;

                if(pManifestInfo->imageFileCount > FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL)
                {
                    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "FUP: parse_manifest_file, imageFileCount exceeded, max %d.\n",
                               FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL);

                    fbe_base_env_memory_ex_release(pBaseEnv, pTokens);

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                len = ((pKey->end - pKey->start) < FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN ) ? 
                            (size_t) (pKey->end - pKey->start): FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN; 
                fbe_copy_memory(&pManifestInfo->imageFileName[pManifestInfo->imageFileCount-1][0],
                                &pJson[pKey->start], 
                                len);
            }
            
            continue;
        }

        if (strncmp("version", &pJson[pKey->start], (size_t) (pKey->end - pKey->start)) == 0)
        {
            pKey = &pTokens[++i];

            for(j = subEnclStart; j < subEnclEnd; j++) 
            {
                pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, j);
                len = ((pKey->end - pKey->start) < FBE_ESES_CDES2_MCODE_IMAGE_REV_SIZE_BYTES ) ? 
                            (size_t) (pKey->end - pKey->start): FBE_ESES_CDES2_MCODE_IMAGE_REV_SIZE_BYTES; 
                fbe_copy_memory(&pManifestInfo->imageRev[pManifestInfo->imageFileCount-1][0],
                                &pJson[pKey->start], 
                                len);
            }

            continue;
        }

        if (strncmp("component_type", &pJson[pKey->start], (size_t) (pKey->end - pKey->start)) == 0)
        {
            pKey = &pTokens[++i];

            for(j = subEnclStart; j < subEnclEnd; j++) 
            {
                pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, j);
                len = ((pKey->end - pKey->start) < FBE_BASE_ENV_FUP_COMP_TYPE_STR_SIZE ) ? 
                            (size_t) (pKey->end - pKey->start): FBE_BASE_ENV_FUP_COMP_TYPE_STR_SIZE; 
                fbe_copy_memory(&fwCompTypeStr[0],
                                &pJson[pKey->start], 
                                len);
                firmwareCompType = (ses_comp_type_enum)csx_p_atoi_s32(&fwCompTypeStr[0]);
                fbe_base_env_fup_fw_comp_type_to_fw_target(pBaseEnv, firmwareCompType, &firmwareTarget);

                pManifestInfo->firmwareCompType[pManifestInfo->imageFileCount-1] = firmwareCompType;
                pManifestInfo->firmwareTarget[pManifestInfo->imageFileCount-1]= firmwareTarget;
            }

            continue;
        }
    }

    fbe_base_env_memory_ex_release(pBaseEnv, pTokens);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_base_env_fup_init_manifest_info(fbe_base_environment_t * pBaseEnv)
 ****************************************************************
 * @brief
 *  This function initializes the firmware upgrade manifest info.
 * 
 * @param pBaseEnv.
 *
 * @return fbe_status_t
 *
 * @author
 *  06-Oct-2014 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_fup_init_manifest_info(fbe_base_environment_t * pBaseEnv)
{
    fbe_u32_t                      subEnclIndex = 0;
    fbe_u32_t                      imageIndex = 0;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;

    for(subEnclIndex = 0; subEnclIndex < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclIndex ++) 
    {
        pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, subEnclIndex);
        fbe_zero_memory(pManifestInfo, (fbe_u32_t)sizeof(fbe_base_env_fup_manifest_info_t));
        fbe_set_memory(&pManifestInfo->subenclProductId[0], ' ', FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE);

        for(imageIndex = 0; imageIndex < FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL; imageIndex ++) 
        {
            pManifestInfo->firmwareCompType[imageIndex] = SES_COMP_TYPE_UNKNOWN;
            pManifestInfo->firmwareTarget[imageIndex] = FBE_FW_TARGET_INVALID;
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_fup_handle_related_work_item(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_u64_t deviceType,
 *                                      fbe_device_physical_location_t * pLocation,
 *                                      fbe_base_env_fup_completion_status_t completionStatus)
 ****************************************************************
 * @brief
 *  This function gets call when the device gets removed.
 *  It checks whether there is any upgrade in progress for this device.
 *  If yes, removes and releases the work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param deviceType - 
 * @param pLocation -
 * @param completionStatus -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @version
 *  07-Oct-2014:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_fup_handle_related_work_item(fbe_base_environment_t * pBaseEnv,
                                       fbe_u64_t deviceType,
                                       fbe_device_physical_location_t * pLocation,
                                       fbe_base_env_fup_completion_status_t completionStatus)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_device_physical_location_t    * pWorkItemLocation = NULL;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

        if((pWorkItem->deviceType == deviceType) &&
           (pWorkItemLocation->bus == pLocation->bus) &&
           (pWorkItemLocation->enclosure == pLocation->enclosure) &&
           (pWorkItemLocation->componentId == pLocation->componentId) &&
           (pWorkItemLocation->slot == pLocation->slot))
        {
            /* There is fup work item for this device. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "related work item failed, will remove and release this work item.\n");
    
            fbe_base_env_set_fup_work_item_completion_status(pWorkItem, completionStatus);
    
            fbe_base_env_fup_post_completion(pBaseEnv, pWorkItem);
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }
  
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_get_wait_time_before_upgrade(fbe_base_environment_t * pBaseEnv)
 ****************************************************************
 * @brief
 *  This function gets the wait time left in seconds before the upgrade.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * 
 * @return fbe_u32_t - the wait time left before upgrade (in seconds)
 *
 * @version
 *  07-Nov-2014:  PHE - Created. 
 *
 ****************************************************************/
fbe_u32_t fbe_base_env_get_wait_time_before_upgrade(fbe_base_environment_t * pBaseEnv)
{
    fbe_u32_t elapsedTimeInSec = 0;
    fbe_u32_t waitTimeBeforeUpgrade = 0;

    elapsedTimeInSec = fbe_get_elapsed_seconds(pBaseEnv->timeStartToWaitBeforeUpgrade);

    waitTimeBeforeUpgrade = (pBaseEnv->upgradeDelayInSec > elapsedTimeInSec) ? (pBaseEnv->upgradeDelayInSec - elapsedTimeInSec) : 0;

    return waitTimeBeforeUpgrade;
}
