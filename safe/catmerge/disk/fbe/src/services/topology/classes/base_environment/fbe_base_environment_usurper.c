/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the PS Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_ps_mgmt_monitor_entry "ps_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup ps_mgmt_class_files
 * 
 * @version
 *   15-Apr-2010:  Created. Joe Perry
 *
 ***************************************************************************/
 /*!*************************************************************************
 * @file fbe_base_environment_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for the base environment class.
 *
 * @version
 *   22-Jul-2010:  PHE - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_event_log_api.h"

static fbe_status_t 
fbe_base_env_ctrl_get_any_upgrade_in_progress(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_initiate_upgrade(fbe_base_environment_t * pBaseEnv,  fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_terminate_upgrade(fbe_base_environment_t * pBaseEnv,  fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_abort_upgrade(fbe_base_environment_t * pBaseEnv,  fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_resume_upgrade(fbe_base_environment_t * pBaseEnv,  fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_get_fup_work_state(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_get_fup_work_state(fbe_base_environment_t * pBaseEnv, 
                                 fbe_u64_t deviceType,
                                 fbe_device_physical_location_t * pLocation,
                                 fbe_base_env_fup_work_state_t * pWorkState);

static fbe_status_t 
fbe_base_env_ctrl_get_fup_info(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_get_sp_id(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_get_resume_prom_info(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_initiate_resume_prom_read(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_write_resume_prom_sync(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t
fbe_base_env_ctrl_get_any_resume_prom_read_in_progress(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t
fbe_base_env_get_any_resume_prom_read_in_progress(fbe_base_environment_t * pBaseEnv, fbe_bool_t * pAnyReadInProgress);

static fbe_status_t 
fbe_base_env_ctrl_get_service_mode (fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_get_fup_manifest_info(fbe_base_environment_t * pBaseEnv, 
                                            fbe_packet_t * packet);

static fbe_status_t 
fbe_base_env_ctrl_modify_fup_delay(fbe_base_environment_t * pBaseEnv, 
                                   fbe_packet_t * packet);

/*!***************************************************************
 * fbe_base_environment_control_entry()
 ****************************************************************
 * @brief
 *  This function is entry point for control operation for this
 *  class
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * @author
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_base_environment_t                * pBaseEnv = NULL;
    
    pBaseEnv = (fbe_base_environment_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, control_code 0x%x.\n", 
                          __FUNCTION__, control_code);
    switch(control_code) 
    {
        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_UPGRADE_IN_PROGRESS:
            status = fbe_base_env_ctrl_get_any_upgrade_in_progress(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_UPGRADE:
            status = fbe_base_env_ctrl_initiate_upgrade(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_TERMINATE_UPGRADE:
            status = fbe_base_env_ctrl_terminate_upgrade(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_ABORT_UPGRADE:
            status = fbe_base_env_ctrl_abort_upgrade(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_RESUME_UPGRADE:
            status = fbe_base_env_ctrl_resume_upgrade(pBaseEnv, packet);
            break;
    
        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_WORK_STATE:
            status = fbe_base_env_ctrl_get_fup_work_state(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_INFO:
            status = fbe_base_env_ctrl_get_fup_info(pBaseEnv, packet);
            break;  
    
        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SP_ID:
            status = fbe_base_env_ctrl_get_sp_id(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_RESUME_PROM_INFO:
            status = fbe_base_env_ctrl_get_resume_prom_info(pBaseEnv, packet);
            break;  

        case FBE_ESP_BASE_ENV_CONTROL_CODE_WRITE_RESUME_PROM:
            status = fbe_base_env_ctrl_write_resume_prom_sync(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_RESUME_PROM_READ:
            status = fbe_base_env_ctrl_initiate_resume_prom_read(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_RESUME_PROM_READ_IN_PROGRESS:
            status = fbe_base_env_ctrl_get_any_resume_prom_read_in_progress(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SERVICE_MODE:
            status = fbe_base_env_ctrl_get_service_mode(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_MANIFEST_INFO:
            status = fbe_base_env_ctrl_get_fup_manifest_info(pBaseEnv, packet);
            break;

        case FBE_ESP_BASE_ENV_CONTROL_CODE_MODIFY_FUP_DELAY:
            status = fbe_base_env_ctrl_modify_fup_delay(pBaseEnv, packet);
            break;

        default:
            status = fbe_base_object_control_entry(object_handle, packet);
            break;
    }

    return status;
}



/*!***************************************************************
 * fbe_base_env_ctrl_get_any_upgrade_in_progress()
 ****************************************************************
 * @brief
 *  This function validates the control packet, get the any upgrade in progress status and
 *  completes the packet.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Jul-2010: PH - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_ctrl_get_any_upgrade_in_progress(fbe_base_environment_t * pBaseEnv, 
                                     fbe_packet_t * packet)
{
    fbe_esp_base_env_get_any_upgrade_in_progress_t  * pGetAnyUpgradeInProgress = NULL;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t    * control_operation = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetAnyUpgradeInProgress);
    if(pGetAnyUpgradeInProgress == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_get_any_upgrade_in_progress_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "FUP:%s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_get_any_upgrade_in_progress_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check whether there is any upgrade in progress for the specified device type. */
    status = fbe_base_env_get_any_upgrade_in_progress(pBaseEnv, 
                                                      pGetAnyUpgradeInProgress->deviceType,
                                                      &pGetAnyUpgradeInProgress->anyUpgradeInProgress);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_base_env_get_any_upgrade_in_progress failed.\n", 
                              __FUNCTION__);
    }
   
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
} /* End of function */

/*!***************************************************************
 * fbe_base_env_get_any_upgrade_in_progress()
 ****************************************************************
 * @brief
 *  This function check whether there is any upgrade in progress.
 *
 * @param pBaseEnv -
 * @param deviceType - The specified device type.
 * @param pAnyUpgradeInProgress (OUTPUT) - 
 *
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @author
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_base_env_get_any_upgrade_in_progress(fbe_base_environment_t * pBaseEnv,
                                         fbe_u64_t deviceType,
                                         fbe_bool_t * pAnyUpgradeInProgress)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
   
    *pAnyUpgradeInProgress = FALSE;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        if(pWorkItem->deviceType == deviceType)
        {
            *pAnyUpgradeInProgress = TRUE;
            break;
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }

    return FBE_STATUS_OK;
} /* End of function fbe_base_env_get_any_upgrade_in_progress */

/*!***************************************************************
 * fbe_base_env_get_specific_upgrade_in_progress()
 ****************************************************************
 * @brief
 *  This function check whether there is a specific upgrade in progress.
 *
 * @param pBaseEnv -
 * @param deviceType - The specified device type.
 * @param pAnyUpgradeInProgress (OUTPUT) - 
 *
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @author
 *  03-Jan-2012: Joe Perry - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_base_env_get_specific_upgrade_in_progress(fbe_base_environment_t * pBaseEnv,
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t *locationPtr,
                                              fbe_base_env_fup_work_state_t workStateToCheck,
                                              fbe_bool_t * pAnyUpgradeInProgress)
{
    fbe_queue_head_t                  * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                    * pWorkItemQueueLock = NULL;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
   
    *pAnyUpgradeInProgress = FALSE;

    pWorkItemQueueHead = fbe_base_env_get_fup_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        if((pWorkItem->deviceType == deviceType) &&
           (pWorkItem->location.bus == locationPtr->bus) &&
           (pWorkItem->location.enclosure == locationPtr->enclosure) &&
           (pWorkItem->location.slot == locationPtr->slot) &&
           (pWorkItem->workState >= workStateToCheck &&
            pWorkItem->workState <= FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE))
        {
            *pAnyUpgradeInProgress = TRUE;
            break;
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_fup_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }

    return FBE_STATUS_OK;
} /* End of function fbe_base_env_get_specific_upgrade_in_progress */

/*!***************************************************************
 * fbe_base_env_ctrl_initiate_upgrade()
 ****************************************************************
 * @brief
 *  This function validates the control packet, initiates the upgrade for the
 *  specified device and completes the packet.
 *
 * @param pPsMgmt -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Jul-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_initiate_upgrade(fbe_base_environment_t * pBaseEnv, 
                                                fbe_packet_t * packet)
{
    fbe_esp_base_env_initiate_upgrade_t     * pInitiateUpgrade = NULL;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: User initiated cmd received to Initiate Upgrade.\n");

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pInitiateUpgrade);
    if(pInitiateUpgrade == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_initiate_upgrade_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "FUP:%s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_initiate_upgrade_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:%s(%d_%d_%d) cid(%d): User initiated cmd received to Initiate Upgrade, forceFlag 0x%X.\n", 
                              fbe_base_env_decode_device_type(pInitiateUpgrade->deviceType), 
                              pInitiateUpgrade->location.bus,
                              pInitiateUpgrade->location.enclosure,
                              pInitiateUpgrade->location.slot,
                              pInitiateUpgrade->location.componentId,
                              pInitiateUpgrade->forceFlags);

    status = pBaseEnv->fup_element_ptr->pInitiateUpgradeCallback(pBaseEnv, 
                                 pInitiateUpgrade->deviceType, 
                                 &(pInitiateUpgrade->location), 
                                 pInitiateUpgrade->forceFlags,
                                 0); // upgradeRetryCount

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s(%d_%d_%d): Failed to Initiate Upgrade, status 0x%X.\n", 
                              fbe_base_env_decode_device_type(pInitiateUpgrade->deviceType),
                              pInitiateUpgrade->location.bus,
                              pInitiateUpgrade->location.enclosure,
                              pInitiateUpgrade->location.slot,
                              status);
    }
   
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_initiate_upgrade*/

/*!***************************************************************
 * fbe_base_env_ctrl_terminate_upgrade()
 ****************************************************************
 * @brief
 *  This function sets the condition to terminate the upgrade.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  04-Jan-2013: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_terminate_upgrade(fbe_base_environment_t * pBaseEnv, 
                                                fbe_packet_t * packet)
{
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "FUP: User initiated cmd received to terminate Upgrade for %s.\n",
                           fbe_base_env_decode_class_id(pBaseEnv->base_object.class_id));

    fbe_event_log_write(ESP_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED,
                                        NULL,
                                        0,
                                        "%s",
                                        fbe_base_env_decode_class_id(pBaseEnv->base_object.class_id));

    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                    (fbe_base_object_t *)pBaseEnv, 
                                    FBE_BASE_ENV_LIFECYCLE_COND_FUP_TERMINATE_UPGRADE);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: %s  Can't set FUP_TERMINATE_UPGRADE condition.\n",   
                              __FUNCTION__);
    }
   
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_terminate_upgrade*/


/*!***************************************************************
 * fbe_base_env_ctrl_abort_upgrade()
 ****************************************************************
 * @brief
 *  This function validates the control packet, resume the upgrade for all devices
 *  with the specified device type.
 *
 * @param pPsMgmt -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  01-Sept-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_abort_upgrade(fbe_base_environment_t * pBaseEnv, 
                                                fbe_packet_t * packet)
{
    fbe_spinlock_t                          * pWorkItemQueueLock = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "FUP: User initiated cmd received to Abort Upgrade for %s.\n",
                           fbe_base_env_decode_class_id(pBaseEnv->base_object.class_id));

    fbe_event_log_write(ESP_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED,
                                        NULL,
                                        0,
                                        "%s",
                                        fbe_base_env_decode_class_id(pBaseEnv->base_object.class_id));

    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    /* Set the flag so that the new upgrade related condition won't be set. */
    fbe_spinlock_lock(pWorkItemQueueLock);
    pBaseEnv->abortUpgradeCmdReceived = FBE_TRUE; 
    fbe_spinlock_unlock(pWorkItemQueueLock);

    /* Set the condition FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE*/
    status = fbe_base_env_fup_set_condition(pBaseEnv, FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: %s  Can't set FUP_ABORT_UPGRADE condition.\n",   
                              __FUNCTION__);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_abort_upgrade*/


/*!***************************************************************
 * fbe_base_env_ctrl_resume_upgrade()
 ****************************************************************
 * @brief
 *  This function validates the control packet, resume the upgrade for all devices
 *  with the specified device type.
 *
 * @param pPsMgmt -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  01-Sept-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_resume_upgrade(fbe_base_environment_t * pBaseEnv, 
                                                fbe_packet_t * packet)
{
    fbe_spinlock_t                          * pWorkItemQueueLock = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "FUP: User initiated cmd received to Resume Upgrade for %s.\n",
                           fbe_base_env_decode_class_id(pBaseEnv->base_object.class_id));

    fbe_event_log_write(ESP_INFO_ENV_RESUME_UPGRADE_COMMAND_RECEIVED,
                                        NULL,
                                        0,
                                        "%s",
                                        fbe_base_env_decode_class_id(pBaseEnv->base_object.class_id));

    pWorkItemQueueLock = fbe_base_env_get_fup_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pBaseEnv->abortUpgradeCmdReceived = FBE_FALSE; 
    fbe_spinlock_unlock(pWorkItemQueueLock);

    status = fbe_lifecycle_force_clear_cond(&fbe_base_environment_lifecycle_const, 
                                   (fbe_base_object_t *)pBaseEnv, 
                                   FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, force_clear_cond COND_FUP_ABORT_UPGRADE failed, status 0x%X.\n", 
                              __FUNCTION__, 
                              status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    status = pBaseEnv->fup_element_ptr->pResumeUpgradeCallback(pBaseEnv);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Resume upgrade failed, status 0x%X.\n", 
                              __FUNCTION__, 
                              status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_resume_upgrade*/


/*!***************************************************************
 * fbe_base_env_ctrl_get_fup_work_state()
 ****************************************************************
 * @brief
 *  This function validates the control packet, get the work state for the
 *  specified device and completes the packet.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Jul-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_get_fup_work_state(fbe_base_environment_t * pBaseEnv, 
                                             fbe_packet_t * packet)
{
    fbe_esp_base_env_get_fup_work_state_t   * pGetWorkState = NULL;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetWorkState);
    if(pGetWorkState == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_get_fup_work_state_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "FUP:%s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_get_fup_work_state_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check whether there is any upgrade in progress. */
    status = fbe_base_env_get_fup_work_state(pBaseEnv, 
                                             pGetWorkState->deviceType,
                                             &pGetWorkState->location,
                                             &pGetWorkState->workState);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_base_env_get_fup_work_state failed.\n", 
                              __FUNCTION__);
    }
   
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
}/* End of function fbe_base_env_ctrl_get_fup_work_state */

/*!***************************************************************
 * fbe_base_env_get_fup_work_state()
 ****************************************************************
 * @brief
 *  This function gets the firmware upgrade work state of the specified device.
 *
 * @param pBaseEnv -
 * @param deviceType -
 * @param pLocation -
 * @param pWorkState (OUTPUT) -
 * 
 * @return fbe_status_t
 *
 * @author
 *  22-Jul-2010: PH - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_get_fup_work_state(fbe_base_environment_t * pBaseEnv, 
                                             fbe_u64_t deviceType,
                                             fbe_device_physical_location_t * pLocation,
                                             fbe_base_env_fup_work_state_t * pWorkState)
{
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;

    *pWorkState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
    status = fbe_base_env_fup_find_work_item(pBaseEnv, deviceType, pLocation, &pWorkItem);

    if(pWorkItem != NULL)
    {
        *pWorkState = pWorkItem->workState;
    }
        return FBE_STATUS_OK;
} /* End of function fbe_base_env_get_fup_work_state*/

/*!***************************************************************
 * fbe_base_env_ctrl_get_fup_info()
 ****************************************************************
 * @brief
 *  This function validates the control packet, gets the
 *  firmware upgrade related information and completes the packet.
 *
 * @param pPsMgmt -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Jul-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_ctrl_get_fup_info(fbe_base_environment_t * pBaseEnv, 
                                            fbe_packet_t * packet)
{
    fbe_esp_base_env_get_fup_info_t         * pGetFupInfo = NULL;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_u8_t                                  indx;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetFupInfo);
    if(pGetFupInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_get_fup_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "FUP:%s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_get_fup_info_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = pBaseEnv->fup_element_ptr->pGetFupInfoCallback(pBaseEnv, 
                                     pGetFupInfo->deviceType,
                                     &pGetFupInfo->location,
                                     pGetFupInfo);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s(%d_%d_%d), %s, Failed to get fup info, status 0x%X.\n",
                              fbe_base_env_decode_device_type(pGetFupInfo->deviceType),
                              pGetFupInfo->location.bus,
                              pGetFupInfo->location.enclosure,
                              pGetFupInfo->location.slot,
                              __FUNCTION__,
                              status);

        for (indx=0;indx<FBE_ESP_MAX_PROGRAMMABLE_COUNT;indx++)
        pGetFupInfo->fupInfo[indx].completionStatus = 
            FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED;
    }
     
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_get_fup_info*/

fbe_status_t 
fbe_base_env_ctrl_get_sp_id(fbe_base_environment_t * pBaseEnv, fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_esp_base_env_get_sp_id_t                            *sp_id_ptr = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &sp_id_ptr);
    if (sp_id_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_get_sp_id_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (pBaseEnv->spid)
    {
        case SP_A:
            sp_id_ptr->sp_id = FBE_BASE_ENV_SPA;
            break;

        case SP_B:
            sp_id_ptr->sp_id = FBE_BASE_ENV_SPB;
            break;

        default:
            sp_id_ptr->sp_id = FBE_BASE_ENV_INVALID;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_base_env_ctrl_get_resume_prom_info()
 ****************************************************************
 * @brief
 *  This function validates the control packet, get the resume prom info
 *  for the specified device and completes the packet.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  28-Oct-2010: Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_get_resume_prom_info(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_packet_t * packet)
{
    fbe_esp_base_env_get_resume_prom_info_cmd_t * pGetResumeInfoCmd;
    fbe_base_env_resume_prom_info_t         * pResumePromInfo = NULL;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetResumeInfoCmd);
    if(pGetResumeInfoCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_get_resume_prom_info_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "RP: %s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_get_resume_prom_info_cmd_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the pointer to the resume prom info. */
    status = pBaseEnv->resume_prom_element.pGetResumePromInfoPtrCallback(pBaseEnv, 
                                                                      pGetResumeInfoCmd->deviceType, 
                                                                      &pGetResumeInfoCmd->deviceLocation, 
                                                                      &pResumePromInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, failed.\n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
 

    if(pBaseEnv->fup_element_ptr->pCopyFwRevToResumePromCallback != NULL)
    {
        status = pBaseEnv->fup_element_ptr->pCopyFwRevToResumePromCallback(pBaseEnv, 
                                                                          pGetResumeInfoCmd->deviceType, 
                                                                          &pGetResumeInfoCmd->deviceLocation);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, failed to update FW Rev.\n", 
                                  __FUNCTION__);
            /* Do not just return the failure here. We want to return the resume prom as much as we can.
             * Even if we could not get the firmware rev to copy to the resume prom, other resume prom info
             * might be available. 
             */
        }
    }
 
    fbe_copy_memory(&pGetResumeInfoCmd->resume_prom_data, 
                    &pResumePromInfo->resume_prom_data, 
                    sizeof(RESUME_PROM_STRUCTURE));

    pGetResumeInfoCmd->op_status = pResumePromInfo->op_status;
    pGetResumeInfoCmd->resumeFault = pResumePromInfo->resumeFault;
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}/* End of function fbe_base_env_ctrl_get_resume_prom_info */



/*!***************************************************************
 * fbe_base_env_ctrl_read_resume_prom_sync()
 ****************************************************************
 * @brief
 *  This function validates the control packet, reads the resume prom synchronously
 *  from the hardware for the specified device and completes the packet.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 * 
 * @note
 *  not sure sure whether this is needed. It depends on whether ESP or SEP support the SN work. 
 *  Comment it out for now.
 * 
 * @author
 *  28-Aug-2011: PHE - Created. 
 *
 ****************************************************************/
#if 0
static fbe_status_t fbe_base_env_ctrl_read_resume_prom_sync(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_packet_t * packet)
{
    fbe_resume_prom_cmd_t                   * pReadResumePromCmd;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_sg_element_t                        * sg_element = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: User cmd received to write resume prom.\n");

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pReadResumePromCmd);
    if(pReadResumePromCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_resume_prom_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "RP: %s, buf len %d invalid, expected %d.\n", 
                              __FUNCTION__, len, sizeof(fbe_resume_prom_cmd_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the sg element and the sg count
    fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

    if (sg_element != NULL)
    {
        pReadResumePromCmd->pBuffer = sg_element[0].address;
        pReadResumePromCmd->bufferSize = sg_element[0].count;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, SG_LIST is NULL.\n", 
                              __FUNCTION__);
       
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_env_send_resume_prom_read_sync_cmd(pBaseEnv, pReadResumePromCmd);
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}/* End of function fbe_base_env_ctrl_read_resume_prom_sync */
#endif
/*!***************************************************************
 * fbe_base_env_ctrl_write_resume_prom_sync()
 ****************************************************************
 * @brief
 *  This function validates the control packet, get the resume prom info
 *  for the specified device and completes the packet.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  28-Oct-2010: Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_write_resume_prom_sync(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_packet_t * packet)
{
    fbe_resume_prom_cmd_t                   * pWriteResumePromCmd;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_sg_element_t                        * sg_element = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: User initiated cmd received to write resume prom.\n");

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pWriteResumePromCmd);
    if(pWriteResumePromCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_resume_prom_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "RP: %s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len,
                              (unsigned long long)sizeof(fbe_resume_prom_cmd_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the sg element and the sg count
    fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

    if (sg_element != NULL)
    {
        pWriteResumePromCmd->pBuffer = sg_element[0].address;
        pWriteResumePromCmd->bufferSize = sg_element[0].count;
    }
    else if(pWriteResumePromCmd->resumePromField == RESUME_PROM_CHECKSUM) 
    {
        pWriteResumePromCmd->pBuffer = NULL;
        pWriteResumePromCmd->bufferSize = 0;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, SG_LIST is NULL.\n", 
                              __FUNCTION__);
       
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_env_send_resume_prom_write_sync_cmd(pBaseEnv, pWriteResumePromCmd, TRUE);
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}/* End of function fbe_base_env_ctrl_write_resume_prom_sync */

/*!***************************************************************
 * fbe_base_env_ctrl_initiate_resume_prom_read()
 ****************************************************************
 * @brief
 *  This function initiate the resume prom read from the hardware
 *  for the specified device.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  19-Jul-2011: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_initiate_resume_prom_read(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_packet_t * packet)
{
    fbe_esp_base_env_initiate_resume_prom_read_cmd_t      * pInitiateResumeReadCmd;
    fbe_payload_control_buffer_length_t       len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: User initiated cmd received to read resume prom.\n");

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pInitiateResumeReadCmd);
    if(pInitiateResumeReadCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_initiate_resume_prom_read_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "RP: %s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_initiate_resume_prom_read_cmd_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Use a function pointer to call the appropriate resume read function. Initialization 
    // of the function pointer is done using fbe_base_env_initiate_resume_prom_read_callback() and
    // is typically of the format fbe_xxxx_initiate_resume_prom_read()
    status = pBaseEnv->resume_prom_element.pInitiateResumePromReadCallback(pBaseEnv,
                                                                           pInitiateResumeReadCmd->deviceType, 
                                                                           &pInitiateResumeReadCmd->deviceLocation);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s failed. Invalid device type: %s, status: 0x%X\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(pInitiateResumeReadCmd->deviceType), 
                              status);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);

        return status;
    }

    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}/* End of function fbe_base_env_ctrl_initiate_resume_prom_read */

/*!***************************************************************
 * fbe_base_env_ctrl_get_any_resume_prom_read_in_progress()
 ****************************************************************
 * @brief
 *  This function validates the control packet, get the any resume prom read in progress status
 *  and completes the packet.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  15-Nov-2011: loul - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_get_any_resume_prom_read_in_progress (fbe_base_environment_t * pBaseEnv,
                                                                            fbe_packet_t * packet)
{
        fbe_esp_base_env_get_any_resume_prom_read_in_progress_t  * pGetAnyReadInProgress = NULL;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_ex_t                     * payload = NULL;
    fbe_payload_control_operation_t      * control_operation = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pGetAnyReadInProgress);
    if(pGetAnyReadInProgress == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_base_env_get_any_resume_prom_read_in_progress_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "RP:%s, buf len %d invalid, expected %llu.\n",
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_get_any_resume_prom_read_in_progress_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check whether there is any prom read in progress. */
    status = fbe_base_env_get_any_resume_prom_read_in_progress(pBaseEnv, &pGetAnyReadInProgress->anyReadInProgress);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:%s, fbe_base_env_get_any_resume_prom_read_in_progress failed.\n",
                              __FUNCTION__);
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_get_any_resume_prom_read_in_progress */

/*!***************************************************************
 * fbe_base_env_get_any_resume_prom_read_in_progress()
 ****************************************************************
 * @brief
 *  This function check whether there is any resume prom read in progress.
 *
 * @param pBaseEnv -
 * @param pAnyReadInProgress (OUTPUT) -
 *
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @author
 *  15-Nov-2011: loul - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_base_env_get_any_resume_prom_read_in_progress(fbe_base_environment_t * pBaseEnv, fbe_bool_t * pAnyReadInProgress)
{
    fbe_queue_head_t                          * pWorkItemQueueHead = NULL;
    fbe_spinlock_t                            * pWorkItemQueueLock = NULL;

    *pAnyReadInProgress = FALSE;

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    if(!fbe_queue_is_empty(pWorkItemQueueHead))
    {
        *pAnyReadInProgress = TRUE;
    }
    fbe_spinlock_unlock(pWorkItemQueueLock);

    return FBE_STATUS_OK;
} /* End of function fbe_base_env_get_any_resume_prom_read_in_progress */




/*!***************************************************************
 * fbe_base_env_ctrl_get_service_mode()
 ****************************************************************
 * @brief
 *  This function validates the control packet, returns whether or not
 *  we are currently in service mode
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  14-May-2014: bphilbin - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_ctrl_get_service_mode (fbe_base_environment_t * pBaseEnv,
                                                                            fbe_packet_t * packet)
{
    fbe_esp_base_env_get_service_mode_t *pGetServiceMode = NULL;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_ex_t                     * payload = NULL;
    fbe_payload_control_operation_t      * control_operation = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pGetServiceMode);
    if(pGetServiceMode == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_base_env_get_service_mode_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, buf len %d invalid, expected %d.\n",
                              __FUNCTION__, len, (fbe_u32_t)sizeof(fbe_esp_base_env_get_service_mode_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check whether there is any prom read in progress. */
    pGetServiceMode->isServiceMode = pBaseEnv->isBootFlash;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_get_any_resume_prom_read_in_progress */


/*!***************************************************************
 * fbe_base_env_ctrl_get_fup_manifest_info()
 ****************************************************************
 * @brief
 *  This function gets the manifest info for the specified device type.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  18-Sep-2014: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_ctrl_get_fup_manifest_info(fbe_base_environment_t * pBaseEnv, 
                                            fbe_packet_t * packet)
{
    fbe_esp_base_env_get_fup_manifest_info_t   * pGetFupManifestInfo = NULL;
    fbe_payload_control_buffer_length_t          len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t            * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetFupManifestInfo);
    if(pGetFupManifestInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_get_fup_manifest_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "FUP:%s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_get_fup_manifest_info_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&pGetFupManifestInfo->manifestInfo[0],
                    &pBaseEnv->fup_element_ptr->manifestInfo[0],
                    FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST * sizeof(fbe_base_env_fup_manifest_info_t));

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_get_fup_manifest_info*/


/*!***************************************************************
 * fbe_base_env_ctrl_set_fup_delay_in_sec()
 ****************************************************************
 * @brief
 *  This function modifies the firmware upgrade delay time after the SP reboot
 *  for the specified device type.
 *
 * @param pBaseEnv -
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  27-Oct-2014: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_ctrl_modify_fup_delay(fbe_base_environment_t * pBaseEnv, 
                                   fbe_packet_t * packet)
{
    fbe_esp_base_env_modify_fup_delay_t           * pModifyFupDelay = NULL;
    fbe_payload_control_buffer_length_t          len = 0;
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t            * control_operation = NULL;
    fbe_u32_t                                    elapsedTimeInSec = 0;
    fbe_u32_t                                    totalDelayInSec = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pModifyFupDelay);
    if(pModifyFupDelay == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_base_env_modify_fup_delay_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "FUP:%s, buf len %d invalid, expected %llu.\n", 
                              __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_base_env_modify_fup_delay_t));

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    elapsedTimeInSec = fbe_get_elapsed_seconds(pBaseEnv->timeStartToWaitBeforeUpgrade);

    if(elapsedTimeInSec > pBaseEnv->upgradeDelayInSec)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: can't modify fup delay time. elapsed %ds, delay %ds is already done.\n", 
                              elapsedTimeInSec, pBaseEnv->upgradeDelayInSec);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    totalDelayInSec = elapsedTimeInSec + pModifyFupDelay->delayInSec;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:user initiated cmd to change fup total delay time to %ds. elapsed %ds, delayLeft %ds.\n", 
                              totalDelayInSec, elapsedTimeInSec, pModifyFupDelay->delayInSec);

    fbe_base_env_init_fup_params(pBaseEnv, totalDelayInSec, totalDelayInSec);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} /* End of function fbe_base_env_ctrl_get_fup_manifest_info*/

/* End of file fbe_base_environment_usurper.c*/

