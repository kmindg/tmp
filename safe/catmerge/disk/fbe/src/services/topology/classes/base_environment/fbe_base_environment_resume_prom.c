/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_resume_prom.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Resume prom functionality
 *  for base_environment.
 *
 * @version
 *   01-Nov-2010:  Arun S - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe_base_environment_private.h"
#include "EspMessages.h"
#include "fbe/fbe_event_log_api.h"
#include "generic_utils_lib.h"
#include "fbe_ps_mgmt.h"
#include "fbe_board_mgmt.h"
#include "fbe_sps_mgmt.h"
#include "fbe_cooling_mgmt.h"

static fbe_status_t 
fbe_base_env_resume_prom_read_async_completion_function(fbe_packet_completion_context_t pContext, 
                                                        fbe_resume_prom_get_resume_async_t * pGetResumeProm);
static fbe_status_t 
fbe_base_env_resume_prom_log_event(fbe_base_environment_t * pBaseEnv, 
                                   fbe_base_env_resume_prom_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_resume_prom_get_emc_serial_num(fbe_base_env_resume_prom_work_item_t * pWorkItem, 
                                            fbe_u8_t *bufferPtr,
                                            fbe_u32_t bufferSize);

static fbe_status_t 
fbe_base_env_save_resume_prom_info(fbe_base_environment_t * pBaseEnv,
                                   fbe_base_env_resume_prom_work_item_t * pWorkItem);

static fbe_status_t 
fbe_base_env_resume_prom_validata_resume_write_cmd(fbe_base_environment_t * pBaseEnv, 
                                     fbe_resume_prom_cmd_t * pWriteResumePromCmd,
                                     fbe_bool_t * pCmdValid);
static fbe_status_t
fbe_base_env_resume_prom_reset_work_item_for_retry(fbe_base_environment_t * pBaseEnv,
                                fbe_base_env_resume_prom_work_item_t * pWorkItem);
static fbe_status_t 
fbe_base_env_resume_prom_updateResPromFailed(fbe_base_environment_t * pBaseEnv,
                                             fbe_u64_t deviceType,
                                             fbe_device_physical_location_t *pLocation,
                                             fbe_bool_t newValue);


/*!**************************************************************
 * @fn fbe_base_env_read_resume_prom_cond_function()
 ***************************************************************
 * @brief
 *   This function issues the resume prom command with the work item
 *   recieved from the ESP object.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 * 
 * @version:
 *  15-Nov-2010 Arun S - Created. 
 *
 ****************************************************************/

fbe_lifecycle_status_t 
fbe_base_env_read_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t                    * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_queue_head_t                          * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                            * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t      * pWorkItem = NULL;
    fbe_base_env_resume_prom_work_item_t      * pNextWorkItem = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_bool_t                                  needToIssueRead = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry.\n",
                                __FUNCTION__);

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    if(fbe_queue_is_empty(pWorkItemQueueHead))
    {
        fbe_spinlock_unlock(pWorkItemQueueLock);

        /* Queue is empty. Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) pBaseEnv);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "RP: %s Failed to clear current condition, status 0x%X.\n",
                                __FUNCTION__, status); 
                 
        }
    }
    else
    {
        /* Loop the queue to find an item which is needed to send the resume prom read.
         * If the work item is the first time to send resume prom read, or it needs to
         * retry and its waiting time since last read exceeds 3 seconds, this item
         * needs to issue the resume prom read.
         * When the resume prom read completes successfully, this work item will
         * be removed and the memory allocated for it will be released.
         * We don't want to send the resume prom read for all the work items together
         * because the large memory consumption would be used and there may not be enough
         * memory available.
         */
        pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
        fbe_spinlock_unlock(pWorkItemQueueLock);
        while (pWorkItem != NULL)
        {
            fbe_spinlock_lock(pWorkItemQueueLock);
            pNextWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
            fbe_spinlock_unlock(pWorkItemQueueLock);

            if (pWorkItem->workState == FBE_BASE_ENV_RESUME_PROM_READ_QUEUED)
            {
                if (pWorkItem->readCmdTimeStamp == 0
                        || (fbe_get_elapsed_milliseconds(pWorkItem->readCmdTimeStamp) >= FBE_BASE_ENV_RESUME_PROM_RETRY_INTERVAL))
                {
                    needToIssueRead = FBE_TRUE;
                    break;
                }
            }
            pWorkItem = pNextWorkItem;
        }
    }

    if(needToIssueRead)
    {
        /* Issue the resume prom read */
        status = fbe_base_env_send_resume_prom_read_async_cmd(pBaseEnv, pWorkItem);
        pWorkItem->readCmdTimeStamp = fbe_get_time();

        if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
        {
            /* Retry required */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                                   "send_resume_prom_read_async_cmd failed, status 0x%x, will retry.\n",
                                                   status);

            /* Set the workState so that it can be retried
             * when the condition function gets to run again.
             */
            pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_QUEUED;
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_INFO,
                                                   fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                                   "send_resume_prom_read_async_cmd is sent successfully, retry count: %d.\n",
                                                   pWorkItem->retryCount);

            /* The command is sent successfully. 
             * Clear the current condition to avoid the multiple resume read requests are outstanding 
             * for the same object in ESP. 
             * We don't have a way to avoid the multiple resume read requests for different objects in ESP. 
             * If that happened, we should get the BUSY status and then we will retry.
             * When too many SCSI requests are outstanding, it could cause the issue in the 
             * the sas expander and the sas driver. 
             * The condition will be set again after the command is completed.
             */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) pBaseEnv);
        
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "RP: %s Failed to clear current condition, status 0x%X.\n",
                                    __FUNCTION__, status); 
                     
            }
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
} // End of function fbe_base_env_read_resume_prom_cond_function

/*!**************************************************************
 * @fn fbe_base_env_update_resume_prom_cond_function()
 ***************************************************************
 * @brief
 *   This function goes through the resume prom work item queue to
 *   see whether there is any resume prom update needed. If yes,
 *   update the resume prom.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 * 
 * @version:
 *  12-Apr-2011 PHE - Created. 
 *
 ****************************************************************/

fbe_lifecycle_status_t 
fbe_base_env_update_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t                    * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_queue_head_t                          * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                            * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t      * pWorkItem = NULL;
    fbe_base_env_resume_prom_work_item_t      * pNextWorkItem = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_bool_t                                  isRetryNeeded = FALSE;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry.\n",
                          __FUNCTION__);

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        fbe_spinlock_lock(pWorkItemQueueLock);
        pNextWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        if(pWorkItem->workState == FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED)
        {
            if(pWorkItem->resume_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS) 
            {
                status = fbe_base_env_resume_prom_verify_checksum(pBaseEnv, pWorkItem);
                if(status != FBE_STATUS_OK)
                {
                    pWorkItem->resume_status = FBE_RESUME_PROM_STATUS_CHECKSUM_ERROR;
                }
            }

            isRetryNeeded = fbe_base_env_resume_prom_is_retry_needed(pBaseEnv, 
                                                                     pWorkItem->resume_status, 
                                                                     pWorkItem->retryCount);

            /* Case 1: Read Completed or Error - Log an event, Update status and release the work item. 
             * Case 2: Read Pending or In Progress - Set the condition to retry. No logging, No update.
             */
            if(!isRetryNeeded)
            {
                /* Log an event based on the resume status */
                fbe_base_env_resume_prom_log_event(pBaseEnv, pWorkItem);

                if(pWorkItem->resume_status != FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT)
                {
                    /* Save the resume prom info */
                    fbe_base_env_save_resume_prom_info(pBaseEnv, pWorkItem);
                }

                /* Data has been copied to the data structure. Now its time to remove and release the work item */
                fbe_base_env_resume_prom_remove_and_release_work_item(pBaseEnv, pWorkItem);  
            }
            else
            {
                /* Need to retry resume prom read. 
                 * Increment the retryCount only if resume_state is NOT FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS
                 */
                 
                if(pWorkItem->resume_status != FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS) 
                {
                    pWorkItem->retryCount ++;
                }
   
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                                   FBE_TRACE_LEVEL_WARNING, 
                                                   fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                                   "Retry resume prom read, resumeStatus %s(%d), retryCount %d.\n",
                                                   fbe_base_env_decode_resume_prom_status(pWorkItem->resume_status),
                                                   pWorkItem->resume_status,
                                                   pWorkItem->retryCount);

                /* Do not remove the work item so that it will be retried 
                 * when the READ_RESUME_PROM condition gets hit.
                 */
                fbe_base_env_resume_prom_reset_work_item_for_retry(pBaseEnv, pWorkItem);
            }
           
        } // End of - if(pWorkItem->workState == FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED)

        pWorkItem = pNextWorkItem;

    } // End of - while(pWorkItem != NULL) 

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) pBaseEnv);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) pBaseEnv,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "RP: %s Failed to clear current condition, status 0x%X.\n",
                            __FUNCTION__, status);
    }
               
    return FBE_LIFECYCLE_STATUS_DONE;

} // End of function fbe_base_env_update_resume_prom_cond_function

/*!**************************************************************
 * @fn fbe_base_env_resume_prom_reset_work_item_for_retry(fbe_base_environment_t * pBaseEnv,
 *                                    fbe_base_env_resume_prom_work_item_t * pWorkItem)
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
 *  22-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t
fbe_base_env_resume_prom_reset_work_item_for_retry(fbe_base_environment_t * pBaseEnv,
                                fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    fbe_status_t            status = FBE_STATUS_OK;

    pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_QUEUED;

    /* If we run this function in the destroy state, we may not do an actual retry.
     * So just release the memory allocated before.
     * We can allocate the memory again when we can really retry.
     */
    if(pWorkItem->pGetResumeCmd != NULL)
    {
        /* Release the buffer */
        if(pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer != NULL)
        {
            fbe_transport_release_buffer(pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer);
            pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer = NULL;
        }

        /* Release the Resume Data */
        fbe_memory_ex_release(pWorkItem->pGetResumeCmd);
        pWorkItem->pGetResumeCmd = NULL;
    }
   
    return status;  
}

/*!**************************************************************
 * @fn fbe_base_env_check_resume_prom_outstanding_request_cond_function()
 ***************************************************************
 * @brief
 *   This function goes through the resume prom work item queue to
 *   see whether there is any outstanding resume prom request.
 *   If yes, do not clear the condition. Otherwise, clear the condition.
 *
 * @param base_object - The pointer to the fbe_base_object_t object.
 * @param packet - The pointer to the packet. 
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 * 
 * @version:
 *  22-Feb-2012 PHE - Created. 
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_env_check_resume_prom_outstanding_request_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_environment_t                * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_queue_head_t                      * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                        * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t  * pWorkItem = NULL;
    fbe_bool_t                              outstandingRequest = FBE_FALSE;
    fbe_status_t                            status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "RP: %s entry.\n",
                                    __FUNCTION__);

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        if(pWorkItem->pGetResumeCmd != NULL)
        {
            /* Outstanding request is present! */
            outstandingRequest = FBE_TRUE;
            break;
        }

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }

    if(!outstandingRequest)
    {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) pBaseEnv);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "RP: %s Failed to clear current condition, status 0x%X.\n",
                                __FUNCTION__, status); 
                 
        }
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * @fn fbe_base_env_save_resume_prom_info(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_base_env_resume_prom_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function saves the resume prom.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem -
 *
 * @return fbe_status_t
 *
 * @version
 *  19-Jul-2011:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_save_resume_prom_info(fbe_base_environment_t * pBaseEnv,
                                   fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    fbe_base_env_resume_prom_info_t           * pResumePromInfo = NULL;
    fbe_class_id_t                              classId;
    fbe_handle_t                                object_handle;
    fbe_status_t                                status = FBE_STATUS_OK;

    /* Call the appropriate device to get the pointer to the resume prom info. */
    status = pBaseEnv->resume_prom_element.pGetResumePromInfoPtrCallback(pBaseEnv, 
                                                       pWorkItem->device_type,
                                                       &pWorkItem->location,
                                                       &pResumePromInfo);

    if((status != FBE_STATUS_OK) || pResumePromInfo == NULL) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_WARNING, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "%s, getResumePromInfoPtr failed, status 0x%x.\n",
                                           __FUNCTION__, status);
        return status;
    }

    /* Copy the data from Work Item to the object. */
    fbe_copy_memory(&pResumePromInfo->resume_prom_data, 
                    (RESUME_PROM_STRUCTURE *)pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer, 
                    sizeof(RESUME_PROM_STRUCTURE));

    /* Save the op status */
    pResumePromInfo->op_status = pWorkItem->resume_status;

    /* Save the resume prom fault status */
    pResumePromInfo->resumeFault = fbe_base_env_get_resume_prom_fault(pWorkItem->resume_status);
    
    pResumePromInfo->device_type = pWorkItem->device_type;
    pResumePromInfo->location = pWorkItem->location;

    /* Update the enclosure fault led */
    if(pBaseEnv->resume_prom_element.pUpdateEnclFaultLedCallback != NULL)
    {
        status = pBaseEnv->resume_prom_element.pUpdateEnclFaultLedCallback(pBaseEnv,
                                                        pWorkItem->device_type,
                                                        &pWorkItem->location);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) pBaseEnv,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Failed to update enclFaultLed, status 0x%X.\n",
                                  __FUNCTION__, status);
        }
    }

    object_handle = fbe_base_pointer_to_handle((fbe_base_t *) pBaseEnv);
    classId = fbe_base_object_get_class_id(object_handle);

    /* Send notification for data change */
    fbe_base_environment_send_data_change_notification(pBaseEnv, 
                                           classId, 
                                           pWorkItem->device_type,
                                           FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO,
                                           &pWorkItem->location);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_base_env_get_resume_prom_fault(fbe_resume_prom_status_t resumeStatus)
 ****************************************************************
 * @brief
 *  This function checks if the resume prom op status is
 *  considered to be an actual resume prom read fault or not.
 * 
 * @param resumeStatus - 
 *
 * @return fbe_bool_t
 *
 * @author
 *  08-Dec-2011: PHE - Created. 
 *
 ****************************************************************/
fbe_bool_t fbe_base_env_get_resume_prom_fault(fbe_resume_prom_status_t resumeStatus)
{
    fbe_bool_t  resumePromFault = FBE_FALSE;
   
    if(resumeStatus > FBE_RESUME_PROM_STATUS_READ_SUCCESS)
    {
        resumePromFault = FBE_TRUE;
    }
    else
    {
        resumePromFault = FBE_FALSE;
    }

    return resumePromFault;
}


/*!**************************************************************
 * @fn fbe_base_env_resume_prom_handle_destroy_state(fbe_base_environment_t * pBaseEnv,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function loops through the work item queue to remove the work items which
 *  reside on the specified enclosure from the work item queue because the enclosure
 *  goes to destroy state.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param bus -
 * @param enclosure -
 *
 * @return fbe_status_t
 *
 * @version
 *  20-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_resume_prom_handle_destroy_state(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_u32_t bus, 
                                                           fbe_u32_t enclosure)
{
    fbe_queue_head_t                          * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                            * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t      * pWorkItem = NULL;
    fbe_base_env_resume_prom_work_item_t      * pNextWorkItem = NULL;
    fbe_device_physical_location_t            * pWorkItemLocation = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "RP: %s, Encl %d_%d, Removed.\n",
                           __FUNCTION__, 
                           bus, 
                           enclosure);

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_resume_prom_work_item_location_ptr(pWorkItem);

        fbe_spinlock_lock(pWorkItemQueueLock);
        pNextWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        if((pWorkItemLocation->bus == bus) &&
           (pWorkItemLocation->enclosure == enclosure)) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                                       FBE_TRACE_LEVEL_INFO, 
                                                       fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                                       "Removed.\n");

            /* If there is no outstanding async resume prom read command for this work item, 
             * set the work state to FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED.
             * Otherwise, we can set it after the async resume prom read command comes back.
             */
            if(pWorkItem->pGetResumeCmd == NULL)
            {
                fbe_base_env_set_resume_prom_work_item_resume_status(pWorkItem, 
                                                                         FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT);
        
                pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED;
        
                /* Set the condition to update the resume prom. */
                status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                                (fbe_base_object_t *)pBaseEnv, 
                                                FBE_BASE_ENV_LIFECYCLE_COND_UPDATE_RESUME_PROM);
            
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                                       FBE_TRACE_LEVEL_ERROR, 
                                                       fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                                       "RP: Can't set UPDATE_RESUME_PROM condition.\n");
                }
            } 
        }      

        pWorkItem = pNextWorkItem;
    }
  
    return FBE_STATUS_OK;
} // End of function fbe_base_env_resume_prom_handle_destroy_state

/*!*************************************************************************
 *  @fn fbe_base_env_resume_prom_find_work_item()
 **************************************************************************
 *  @brief
 *      This function is to find the matching work item in the queue with the 
 *      specified location and work state. 
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *  @param deviceType - 
 *  @param pLocation -
 *  @param requestType - 
 *  @param pReturnWorkItemPtr(OUTPUT) - The pointer to the pointer of the return work item.
 *            NULL - The matching item is not found.
 *            Otherwise - The matching item is found.
 * 
 *  @return  fbe_status_t - always return FBE_STATUS_OK.
 * 
 *  @version
 *  02-Nov-2010 Arun S - Created. Replicated from FUP code. Changed for Resume prom.
 *
 **************************************************************************/
fbe_status_t 
fbe_base_env_resume_prom_find_work_item(fbe_base_environment_t * pBaseEnv, 
                                        fbe_u64_t deviceType, 
                                        fbe_device_physical_location_t * pLocation, 
                                        fbe_base_env_resume_prom_work_item_t ** pReturnWorkItemPtr)
{
    fbe_queue_head_t                      * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                        * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t  * pWorkItem = NULL;
    fbe_device_physical_location_t        * pWorkItemLocation = NULL;

    *pReturnWorkItemPtr = NULL;

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        pWorkItemLocation = fbe_base_env_get_resume_prom_work_item_location_ptr(pWorkItem);

        if((pWorkItem->device_type == deviceType) &&
           (fbe_equal_memory(pWorkItemLocation, pLocation, sizeof(fbe_device_physical_location_t))))
        {
            *pReturnWorkItemPtr = pWorkItem;
            break;
        }      

        fbe_spinlock_lock(pWorkItemQueueLock);
        pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);
    }
  
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_resume_prom_create_and_add_work_item(fbe_base_environment_t * pBaseEnv, 
 *                                                 fbe_object_id_t object_id,                                                 
 *                                                 fbe_u64_t deviceType,
 *                                                 fbe_device_physical_location_t * pLocation,                                               
 *                                                 fbe_u8_t * pLogHeader)
 ****************************************************************
 * @brief
 *  This function allocates the memory for the new work item and adds it 
 *  to the tail of the work item queue.The read/write resume prom operation are sync.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param object_id - Target object id.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param deviceType - The Device for which the resume prom read request is issued.
 * @param pLogHeader - The pointe to the log header.
 * 
 * @return fbe_status_t
 *
 * @version
 *  01-Nov-2010:  Arun S - Created.
 *  02-Aug-2011:  PHE - Set the FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM when needed. 
 ****************************************************************/
fbe_status_t
fbe_base_env_resume_prom_create_and_add_work_item(fbe_base_environment_t * pBaseEnv, 
                                                  fbe_object_id_t object_id,
                                                  fbe_u64_t deviceType,
                                                  fbe_device_physical_location_t * pLocation, 
                                                  fbe_u8_t * pLogHeader)
{
    fbe_base_env_resume_prom_work_item_t         * pWorkItem = NULL;
    fbe_base_env_resume_prom_work_item_t         * pFirstWorkItemInQueue = NULL;
    fbe_queue_head_t                             * pWorkItemQueueHead = NULL;
    fbe_spinlock_t                               * pWorkItemQueueLock = NULL;
    fbe_lifecycle_state_t                          lifecycleState;
    fbe_status_t                                   status = FBE_STATUS_OK;

    /* Check whether there is already a work item for the device. */
    fbe_base_env_resume_prom_find_work_item(pBaseEnv, deviceType, pLocation, &pWorkItem);

    if(pWorkItem != NULL)
    {
        /* There is already a work item for it. No need to continue. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                        "%s, Resume Prom Read already in Progress.\n",
                                        __FUNCTION__);

        return FBE_STATUS_OK;
    }

    pWorkItem = fbe_base_env_memory_ex_allocate(pBaseEnv, sizeof(fbe_base_env_resume_prom_work_item_t));
    if(pWorkItem == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "RP: mem alloc for work item failed. Device: %s\n",
                                    fbe_base_env_decode_device_type(deviceType));
        
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    
    
    fbe_zero_memory(pWorkItem, sizeof(fbe_base_env_resume_prom_work_item_t));

    fbe_copy_memory(&pWorkItem->location, 
                     pLocation, 
                     sizeof(fbe_device_physical_location_t));

    pWorkItem->object_id = object_id;
    pWorkItem->device_type = deviceType;
    pWorkItem->pGetResumeCmd = NULL;
    pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_QUEUED;
    pWorkItem->resume_status = FBE_RESUME_PROM_STATUS_QUEUED;
    pWorkItem->retryCount = 0;
    pWorkItem->readCmdTimeStamp = 0;

    fbe_copy_memory(&pWorkItem->logHeader[0], 
                    pLogHeader, 
                    FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);
    
    fbe_spinlock_lock(pWorkItemQueueLock);
    fbe_queue_push(pWorkItemQueueHead, (fbe_queue_element_t *)pWorkItem);
    pFirstWorkItemInQueue = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                       FBE_TRACE_LEVEL_INFO, 
                                       fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                       "Resume Prom read Queued, workItem %p, targetObj: %d\n",
                                       pWorkItem,
                                       pWorkItem->object_id);

    if(pFirstWorkItemInQueue == pWorkItem)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                        "%s, It is the first item in queue!\n",   
                        __FUNCTION__);  

        /* Do not set the FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM if we are not in the ready state yet.
         * When we transition to the ready state, 
         * FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM is the preset condition.
         */
        status = fbe_lifecycle_get_state(&fbe_base_environment_lifecycle_const, 
                                             (fbe_base_object_t*)pBaseEnv, 
                                             &lifecycleState);
    
        if((status == FBE_STATUS_OK) &&
           (lifecycleState == FBE_LIFECYCLE_STATE_READY))
        {
            status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                            (fbe_base_object_t *)pBaseEnv, 
                                            FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "Can not set COND_READ_RESUME_PROM, dequeue workItem %p.\n",
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
 * @fn fbe_base_env_resume_prom_remove_and_release_work_item(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_base_env_resume_prom_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function removes the work item from the resume prom work item queue.
 *  and release the memory allocated for the work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pWorkItem - The pointer to the work item to be removed.
 *
 * @return fbe_status_t
 *
 * @version
 *  01-Nov-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t 
fbe_base_env_resume_prom_remove_and_release_work_item(fbe_base_environment_t * pBaseEnv, 
                                                      fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    fbe_spinlock_t             * pWorkItemQueueLock = NULL;
   
    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                       FBE_TRACE_LEVEL_INFO, 
                                       fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                       "Resume Prom read Completed, removing workItem %p.\n",
                                       pWorkItem); 

    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    fbe_queue_remove((fbe_queue_element_t *)pWorkItem);
    fbe_spinlock_unlock(pWorkItemQueueLock);
    
    if(pWorkItem->pGetResumeCmd != NULL)
    {
        /* Release the buffer */
        if(pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer != NULL)
        {
            fbe_transport_release_buffer(pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer);
        }

        /* Release the Resume Data */
        fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem->pGetResumeCmd);
    }

    /* Release the WorkItem */
    fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);
    

    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_base_env_resume_prom_destroy_queue()
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
 *  01-Nov-2010 Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_resume_prom_destroy_queue(fbe_base_environment_t * pBaseEnv)
{
    fbe_queue_head_t                      * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                        * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t  * pWorkItem = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "RP: %s entry.\n",
                                    __FUNCTION__);

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);

    while(!fbe_queue_is_empty(pWorkItemQueueHead))
    {
        pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_pop(pWorkItemQueueHead);

        if(pWorkItem != NULL)
        {
            if(pWorkItem->pGetResumeCmd != NULL)
            {
                /* Release the buffer */
                if(pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer != NULL)
                {
                    fbe_transport_release_buffer(pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer);
                    pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer = NULL;
                }
        
                /* Release the Resume Data */
                fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem->pGetResumeCmd);
                pWorkItem->pGetResumeCmd = NULL;
            }
    
            /* Release the WorkItem */
            fbe_base_env_memory_ex_release(pBaseEnv, pWorkItem);
        }
    }

    fbe_spinlock_unlock(pWorkItemQueueLock);

    fbe_queue_destroy(pWorkItemQueueHead);
    fbe_spinlock_destroy(pWorkItemQueueLock);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_env_resume_prom_handle_device_removal(fbe_base_environment_t * pBaseEnv, 
 *                                      fbe_u64_t deviceType,
 *                                      fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets call when the device gets removed.
 *  It checks whether there is any read in progress for this device.
 *  If yes, removes and releases the work item.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param deviceType - The Device for which the resume prom read request is issued. 
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @version
 *  16-Dec-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t 
fbe_base_env_resume_prom_handle_device_removal(fbe_base_environment_t * pBaseEnv, 
                                               fbe_u64_t deviceType, 
                                               fbe_device_physical_location_t * pLocation)
{
    fbe_queue_head_t                          * pWorkItemQueueHead = NULL;  
    fbe_spinlock_t                            * pWorkItemQueueLock = NULL;
    fbe_base_env_resume_prom_work_item_t      * pWorkItem = NULL;
    fbe_base_env_resume_prom_work_item_t      * pNextWorkItem = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_device_physical_location_t            * pWorkItemLocation = NULL;

    pWorkItemQueueHead = fbe_base_env_get_resume_prom_work_item_queue_head_ptr(pBaseEnv);
    pWorkItemQueueLock = fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(pBaseEnv);

    fbe_spinlock_lock(pWorkItemQueueLock);
    pWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_front(pWorkItemQueueHead);
    fbe_spinlock_unlock(pWorkItemQueueLock);

    while(pWorkItem != NULL) 
    {
        fbe_spinlock_lock(pWorkItemQueueLock);
        pNextWorkItem = (fbe_base_env_resume_prom_work_item_t *)fbe_queue_next(pWorkItemQueueHead, &pWorkItem->queueElement);
        fbe_spinlock_unlock(pWorkItemQueueLock);

        pWorkItemLocation = fbe_base_env_get_resume_prom_work_item_location_ptr(pWorkItem);

        if((pWorkItem->device_type == deviceType) &&
           (fbe_equal_memory(pWorkItemLocation, pLocation, sizeof(fbe_device_physical_location_t)))) 
        {
            /* There is Resume prom work item for this device. */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                                "Device Removed, will Remove and Release the work item %p.\n",
                                               pWorkItem);

            /* If there is no outstanding async resume prom read/write command for this work item, 
             * set the work state to FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED/FBE_BASE_ENV_RESUME_PROM_WRITE_COMPLETED.
             * Otherwise, we can set it after the async resume prom read/write command comes back.
             */
            if(pWorkItem->pGetResumeCmd == NULL)
            {
                fbe_base_env_set_resume_prom_work_item_resume_status(pWorkItem, 
                                                                     FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT);
               
                pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED;;
        
                /* Set the condition to update the resume prom. */
                status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                                (fbe_base_object_t *)pBaseEnv, 
                                                FBE_BASE_ENV_LIFECYCLE_COND_UPDATE_RESUME_PROM);
            
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                                       FBE_TRACE_LEVEL_ERROR, 
                                                       fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                                       "RP: Can't set UPDATE_RESUME_PROM condition.\n");
                }
            }//End of - if(pWorkItem->pGetResumeCmd == NULL)
        }// End of - if(pWorkItem != NULL)

        pWorkItem = pNextWorkItem;
    }
   
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_base_env_resume_prom_get_emc_serial_num()
 ****************************************************************
 * @brief
 *  This function gets the serial number for the Resume prom device.
 *
 * @param pWorkItem - The pointer to the work item.     
 * @param bufferPtr - Resume prom buffer pointer.
 * @param bufferSize - Resume prom buffer size.
 * 
 * @return fbe_status_t
 *
 * @author
 *  13-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_resume_prom_get_emc_serial_num(fbe_base_env_resume_prom_work_item_t * pWorkItem, 
                                                                fbe_u8_t *bufferPtr,
                                                                fbe_u32_t bufferSize)
{
    RESUME_PROM_STRUCTURE *resume_data = (RESUME_PROM_STRUCTURE *)pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer;

    /* Zero out the bufferPtr before filling in the data */
    fbe_zero_memory(bufferPtr, bufferSize);

    if ((bufferSize < RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE) ||
        (bufferSize < RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        if(pWorkItem->resume_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS)
        {
            if(pWorkItem->device_type == FBE_DEVICE_TYPE_SP)
            {
                fbe_copy_memory(bufferPtr, 
                                &resume_data->emc_sub_assy_serial_num,
                                RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE);
            }
            else
            {
                fbe_copy_memory(bufferPtr, 
                                &resume_data->emc_tla_serial_num,
                                RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);
            }
        }
    }

    return FBE_STATUS_OK;

} /* End of cm_pe_rp_get_emc_serial_num() */

/*!***************************************************************
 * fbe_base_env_resume_prom_is_retry_needed()
 ****************************************************************
 * @brief
 *  This function checks the if any retry is required for
 *  the Resume prom read operation. For success, do a retry if the
 *  checksum fails. 
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param resumeStatus -      
 * @param retryCount - 
 * 
 * @return fbe_bool_t
 *
 * @author
 *  12-Jan-2011:  Arun S - Created.
 *  04-Aug-2011: PHE - updated to add retryCount.
 *
 ****************************************************************/
fbe_bool_t fbe_base_env_resume_prom_is_retry_needed(fbe_base_environment_t * pBaseEnv, 
                                                    fbe_resume_prom_status_t resumeStatus,
                                                    fbe_u8_t retryCount)
{
    fbe_bool_t retry = FALSE;

    switch(resumeStatus)
    {
        case FBE_RESUME_PROM_STATUS_READ_SUCCESS:
        case FBE_RESUME_PROM_STATUS_DEVICE_DEAD:
        case FBE_RESUME_PROM_STATUS_DEVICE_NOT_VALID_FOR_PLATFORM:
        case FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT:
        case FBE_RESUME_PROM_STATUS_DEVICE_POWERED_OFF:
            retry = FALSE;
            break;
            

        case FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS:
            retry = TRUE;
            break;

        /* For the following status, if the resume prom device was read by SPECL, 
         * there is actually no need to do the retry because 
         * these status are only set when SPECL finished retrying in base board.
         * However, we may want to do the retry for the resume prom device read by CDES. 
         */
        case FBE_RESUME_PROM_STATUS_DEVICE_ERROR:
        case FBE_RESUME_PROM_STATUS_CHECKSUM_ERROR:
        default:
            if(retryCount < FBE_BASE_ENV_RESUME_PROM_MAX_RETRY_COUNT) 
            {
                retry = TRUE;
            }
            break;
    }

    return retry;
}

/*!**************************************************************
 * @fn fbe_base_env_resume_prom_log_event()
 ****************************************************************
 * @brief
 *  This function logs the Resume Prom event based on the
 *  completion status. If resume prom read failed, turn on the enclosure
 *  fault led.
 * 
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.     
 *
 * @return fbe_status_t - always return FBE_STATUS_OK
 *
 * @version
 *  13-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_resume_prom_log_event(fbe_base_environment_t * pBaseEnv, 
                                   fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    fbe_u8_t       deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_u8_t       pBufferPtr[RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE + 1] = {0};
    fbe_status_t   status = FBE_STATUS_OK;

    fbe_zero_memory(deviceStr, FBE_DEVICE_STRING_LENGTH);
    fbe_zero_memory(pBufferPtr,RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);
    
    status = fbe_base_env_create_device_string(pWorkItem->device_type, 
                                               &pWorkItem->location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_ERROR, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                           "%s, Failed to create device string, status 0x%x.\n", 
                                           __FUNCTION__, 
                                           status); 

        return status;
    }

    switch(pWorkItem->resume_status)
    {
        case FBE_RESUME_PROM_STATUS_READ_SUCCESS:
            fbe_base_env_resume_prom_updateResPromFailed(pBaseEnv, 
                                                         pWorkItem->device_type, 
                                                         &pWorkItem->location, 
                                                         FALSE);
    
            fbe_event_log_write(ESP_INFO_RESUME_PROM_READ_SUCCESS, 
                                    NULL, 0, 
                                    "%s", 
                                    &deviceStr[0]);

            status = fbe_base_env_resume_prom_get_emc_serial_num(pWorkItem, 
                                                                 pBufferPtr, 
                                                                 RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);

            if(status == FBE_STATUS_OK)
            {
                fbe_event_log_write(ESP_INFO_SERIAL_NUM, 
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0], 
                                pBufferPtr);
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                                   FBE_TRACE_LEVEL_WARNING, 
                                                   fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                                   "Failed to get the Serial No, status 0x%x.\n",
                                                   status); 
            }
                
            break;
    
                
        case FBE_RESUME_PROM_STATUS_DEVICE_NOT_VALID_FOR_PLATFORM:

            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                               FBE_TRACE_LEVEL_WARNING, 
                                               fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                               "%s, %s Not Valid For Platform.\n",
                                               __FUNCTION__,
                                               &deviceStr[0]);
            break;

        case FBE_RESUME_PROM_STATUS_DEVICE_DEAD:
        case FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT:
        default:
            fbe_base_env_resume_prom_updateResPromFailed(pBaseEnv, 
                                                         pWorkItem->device_type, 
                                                         &pWorkItem->location, 
                                                         TRUE);
// ESP_STAND_ALONE_ALERT - remove when CP support available
            fbe_event_log_write(ESP_ERROR_RESUME_PROM_READ_FAILED, 
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0], 
                                fbe_base_env_decode_resume_prom_status(pWorkItem->resume_status));
            

            break;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_base_env_resume_prom_verify_checksum()
 ****************************************************************
 * @brief
 *  This function verifies the Resume prom checksum.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.     
 * 
 * @return fbe_status_t FBE_STATUS_OK - checksum is good.
 *  otherwise, checksum is not good.
 *
 * @author
 *  13-Nov-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_resume_prom_verify_checksum(fbe_base_environment_t * pBaseEnv, 
                                                      fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    RESUME_PROM_STRUCTURE            * pResumeData = NULL;
    fbe_u32_t                          checksum, exp_checksum;                    

    pResumeData = (RESUME_PROM_STRUCTURE *)pWorkItem->pGetResumeCmd->resumeReadCmd.pBuffer;

    /* Calculate the checksum that we expect to see */
    exp_checksum = calculateResumeChecksum(pResumeData);

    /* Get the checksum from the resume prom */
    checksum = pResumeData->resume_prom_checksum;

    /* Compare the checksum value only if we dont have NULL or 0 */
    if(checksum)
    {
        if(exp_checksum != checksum)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "%s: CheckSum Error, Expected:0x%08x, Read:0x%08x\n", 
                                           __FUNCTION__,
                                           exp_checksum, 
                                           checksum); 
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}



/*!*************************************************************************
 * @fn fbe_base_env_send_resume_prom_read_async_cmd()
 **************************************************************************
 * @brief
 *      This function issues the Async Resume prom read command for a particular device.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWorkItem - The pointer to the work item.     
 *
 * @return  fbe_status_t
 *
 * @note 
 *
 * @version
 *  09-Nov-2010 Arun S - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_send_resume_prom_read_async_cmd(fbe_base_environment_t * pBaseEnv, 
                                                          fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_resume_prom_get_resume_async_t      *pGetResumeCmd = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "%s entry.\n",
                                           __FUNCTION__);   


    /* It is possible that the read is re-issued as a part of retry. In that case,
     * we end up allocating again without freeing the previously allocated memory. 
     * So, it is safe to first check if the pGetResume 
     */
    if(pWorkItem->pGetResumeCmd == NULL)
    {
        pGetResumeCmd = fbe_base_env_memory_ex_allocate(pBaseEnv, sizeof(fbe_resume_prom_get_resume_async_t));
        if(pGetResumeCmd == NULL)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "mem alloc for pGetResumeProm failed.\n");
    
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    
        /* Verify the size of resume prom structure vs memory chunk size before allocating
         * memory for Resume prom buffer.
         */
        FBE_ASSERT_AT_COMPILE_TIME(sizeof(RESUME_PROM_STRUCTURE) < FBE_MEMORY_CHUNK_SIZE);

        /* Set the buffer, buffer size and offset */
        pGetResumeCmd->resumeReadCmd.pBuffer = fbe_transport_allocate_buffer();
        if (pGetResumeCmd->resumeReadCmd.pBuffer == NULL)  
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "mem alloc for Resume Prom buffer failed.\n");
    
            fbe_base_env_memory_ex_release(pBaseEnv, pGetResumeCmd);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            fbe_zero_memory(pGetResumeCmd->resumeReadCmd.pBuffer, FBE_MEMORY_CHUNK_SIZE);
        }

        pGetResumeCmd->resumeReadCmd.bufferSize = sizeof(RESUME_PROM_STRUCTURE);
        pGetResumeCmd->resumeReadCmd.resumePromField = RESUME_PROM_ALL_FIELDS; 
        pGetResumeCmd->resumeReadCmd.offset = 0;

        /* Set the device type and location */
        pGetResumeCmd->resumeReadCmd.deviceLocation = pWorkItem->location;
        pGetResumeCmd->resumeReadCmd.deviceType = pWorkItem->device_type;
    
        /* Set the completion function and context */
        pGetResumeCmd->completion_function = (fbe_resume_completion_function_t)fbe_base_env_resume_prom_read_async_completion_function; 
        pGetResumeCmd->completion_context = pBaseEnv; 
    
        pWorkItem->pGetResumeCmd = pGetResumeCmd;
    }
    else
    {
        pGetResumeCmd = pWorkItem->pGetResumeCmd;
    }

    /* SET the workStatus to FBE_BASE_ENV_RESUME_PROM_READ_SENT 
     * so that it we don't retry it unless the READ itself fails. 
     * Please make sure the workState is set before sending the command.
     * The command to base board is actually sync so we need to make sure
     * Setting FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED after SENT
     */

    pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_SENT;

    /* Issue the resume read */
    status = fbe_api_resume_prom_read_async(pWorkItem->object_id, pGetResumeCmd);

    if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "fbe_api_resume_prom_read_async failed, status 0x%x.\n", 
                                           status);

        /* Release the buffer */
        fbe_transport_release_buffer(pGetResumeCmd->resumeReadCmd.pBuffer);
        fbe_base_env_memory_ex_release(pBaseEnv, pGetResumeCmd);
        pWorkItem->pGetResumeCmd = NULL;
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_INFO, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "fbe_api_resume_prom_read_async is sent successfully.\n");
    }

    return status;
}// End of function fbe_base_env_send_resume_prom_read_async_cmd

/*!***************************************************************
 * fbe_base_env_resume_prom_read_async_completion_function()
 ****************************************************************
 * @brief
 *  This function gets called when the async resume prom read command
 *  completed. It will set the work state for the work item and set the
 *  condition so that the update of resume prom can be done.
 *
 * @param pContext - The pointer to the fbe_base_environment_t object.
 * @param fbe_resume_prom_get_resume_async_t - 
 * 
 * @return fbe_status_t
 *
 * @author
 *  08-Nov-2010:  Arun S - Created.
 *  12-Apr-2011: PHE - Set the condition to do the update for the resume prom to avoid
 *                     the locking for the resume prom work item queue.                     
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_env_resume_prom_read_async_completion_function(fbe_packet_completion_context_t pContext, 
                                                        fbe_resume_prom_get_resume_async_t * pGetResumeProm)
{
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_u64_t                          device_type;
    fbe_device_physical_location_t           * pLocation = NULL;
    fbe_base_env_resume_prom_work_item_t     * pWorkItem = NULL;
    fbe_base_environment_t                   * pBaseEnv = (fbe_base_environment_t *)pContext;

    device_type = pGetResumeProm->resumeReadCmd.deviceType;
    pLocation = &pGetResumeProm->resumeReadCmd.deviceLocation;
    
    /* Find the work item for the device and location. */
    fbe_base_env_resume_prom_find_work_item(pBaseEnv, device_type, pLocation, &pWorkItem);

    if(pWorkItem == NULL)
    {
        /* Could not find the Work Item. */
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "RP: Work Item not found for Device: %s.\n", 
                                    fbe_base_env_decode_device_type(device_type));

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED;
    
    fbe_base_env_set_resume_prom_work_item_resume_status(pWorkItem, 
                                                             pGetResumeProm->resumeReadCmd.readOpStatus);

    fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                       FBE_TRACE_LEVEL_INFO, 
                                       fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                       "Read async completed, workItem %p, resumeStatus %s(%d).\n",
                                       pWorkItem,
                                       fbe_base_env_decode_resume_prom_status(pWorkItem->resume_status),
                                       pWorkItem->resume_status);

    /* Set the condition to update the resume prom. */
    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                    (fbe_base_object_t *)pBaseEnv, 
                                    FBE_BASE_ENV_LIFECYCLE_COND_UPDATE_RESUME_PROM);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_ERROR, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "RP: Can't set UPDATE_RESUME_PROM condition.\n");
    }

    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                    (fbe_base_object_t *)pBaseEnv, 
                                    FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv, 
                                           FBE_TRACE_LEVEL_ERROR, 
                                           fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem), 
                                           "RP: Can't set READ_RESUME_PROM condition.\n");
    }

    return status;
} /* End of function fbe_base_env_resume_prom_read_completion_function */


/*!*************************************************************************
 * @fn fbe_base_env_send_resume_prom_read_sync_cmd()
 **************************************************************************
 * @brief
 *      This function issues the Resume prom read sync command with retries
 *      to a particular device.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pReadResumePromCmd - The pointer to the resume prom cmd.     
 *
 * @return  fbe_status_t
 *
 * @note
 *  It is not called currently. But it will be needed for the SN support work. 
 * 
 * @version
 *  15-Jul-2011 PHE  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_send_resume_prom_read_sync_cmd(fbe_base_environment_t * pBaseEnv, 
                                     fbe_resume_prom_cmd_t * pReadResumeCmd)
{
    fbe_base_env_resume_prom_info_t          * pResumePromInfo = NULL;
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_u8_t                                 * pBufferSaved = NULL;
    fbe_bool_t                                 isRetryNeeded = FBE_FALSE;
    fbe_u8_t                                   retryCount = 0;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry.\n",
                          __FUNCTION__); 

    if(pReadResumeCmd->bufferSize > FBE_MEMORY_CHUNK_SIZE) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s, bufferSize %d too large.\n", 
                              __FUNCTION__, pReadResumeCmd->bufferSize);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    pBufferSaved = pReadResumeCmd->pBuffer;

    /* Allocate the non-paged physically contiguous memory. */
    pReadResumeCmd->pBuffer = fbe_transport_allocate_buffer();
    if(pReadResumeCmd->pBuffer == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, mem alloc failed.\n",
                                  __FUNCTION__);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(pReadResumeCmd->pBuffer, FBE_MEMORY_CHUNK_SIZE);

    /* Call the appropriate device to get the pointer to the resume prom info. */
    pBaseEnv->resume_prom_element.pGetResumePromInfoPtrCallback(pBaseEnv, 
                                                       pReadResumeCmd->deviceType,
                                                       &pReadResumeCmd->deviceLocation,
                                                       &pResumePromInfo);
     
    do{
        status = fbe_api_resume_prom_read_sync(pResumePromInfo->objectId, pReadResumeCmd);
    
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, fbe_api_resume_prom_read_sync failed, status 0x%x.\n", 
                                  __FUNCTION__, status);

            isRetryNeeded = FALSE;
        }
        else if(pReadResumeCmd->readOpStatus == FBE_RESUME_PROM_STATUS_READ_SUCCESS) 
        {
            /* Copy the resume prom data over. */
            fbe_copy_memory(pBufferSaved,
                            pReadResumeCmd->pBuffer,
                            pReadResumeCmd->bufferSize);
            
            isRetryNeeded = FALSE;
        }
        else
        {
            isRetryNeeded = fbe_base_env_resume_prom_is_retry_needed(pBaseEnv, 
                                                                     pReadResumeCmd->readOpStatus, 
                                                                     retryCount);
            if(isRetryNeeded) 
            {
                /* Need to retry resume prom read. 
                 * Increment the retryCount only if resume_state is NOT FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS
                 */
                if(pReadResumeCmd->readOpStatus != FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS)
                {
                    retryCount ++;
                }
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }while(isRetryNeeded);

    /* Release the memory allocated. */
    fbe_transport_release_buffer(pReadResumeCmd->pBuffer);

    /* Restore the old buffer pointer. */
    pReadResumeCmd->pBuffer = pBufferSaved;

    return status;
} // End of function fbe_base_env_send_resume_prom_read_sync_cmd

/*!*************************************************************************
 * @fn fbe_base_env_send_resume_prom_write_sync_cmd()
 **************************************************************************
 * @brief
 *      This function issues the Resume prom write sync command for a particular device.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWriteResumePromCmd - The pointer to the resume prom cmd.    
 * @param issue_read - Do we need to issue resume prom read after write finish?
 *
 * @return  fbe_status_t
 *
 * @note 
 *
 * @version
 *  15-Jul-2011 PHE  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_send_resume_prom_write_sync_cmd(fbe_base_environment_t * pBaseEnv, 
                                     fbe_resume_prom_cmd_t * pWriteResumeCmd, fbe_bool_t issue_read)
{
    fbe_base_env_resume_prom_info_t          * pResumePromInfo = NULL;
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_bool_t                                 cmdValid = FBE_TRUE;
    fbe_u8_t                                 * pBufferSaved = NULL;
    fbe_u32_t                                  checksum = 0;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry.\n",
                          __FUNCTION__); 

    fbe_base_env_resume_prom_validata_resume_write_cmd(pBaseEnv,pWriteResumeCmd,&cmdValid);

    if(!cmdValid) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pWriteResumeCmd->bufferSize > FBE_MEMORY_CHUNK_SIZE) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s, bufferSize %d too large.\n", 
                              __FUNCTION__, pWriteResumeCmd->bufferSize);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    pBufferSaved = pWriteResumeCmd->pBuffer;
   
    /* Allocate the non-paged physically contiguous memory so that we don't need the use
    * to allocate the non-paged physically contiguous memory.
    */
    pWriteResumeCmd->pBuffer = fbe_transport_allocate_buffer();
    if(pWriteResumeCmd->pBuffer == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, mem alloc failed.\n",
                                  __FUNCTION__);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(pWriteResumeCmd->pBuffer, FBE_MEMORY_CHUNK_SIZE);
    
    if(pWriteResumeCmd->bufferSize != 0) 
    {
        fbe_copy_memory(pWriteResumeCmd->pBuffer, 
                        pBufferSaved,
                        pWriteResumeCmd->bufferSize);
    }

    /* Call the appropriate device to get the pointer to the resume prom info. */
    status = pBaseEnv->resume_prom_element.pGetResumePromInfoPtrCallback(pBaseEnv, 
                                                       pWriteResumeCmd->deviceType,
                                                       &pWriteResumeCmd->deviceLocation,
                                                       &pResumePromInfo);
    if (status == FBE_STATUS_NO_DEVICE)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s Invalid device type: %s, status: 0x%X\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(pWriteResumeCmd->deviceType), 
                              status);

    }
    else if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s failed. status: 0x%X\n", 
                              __FUNCTION__, status);
    }
    else
    {
        if(pWriteResumeCmd->resumePromField == RESUME_PROM_CHECKSUM)
        {
            /* The enclosure object does not save a copy of the resume prom.
             * We can NOT calcuate the checksum in enclosure object like what 
             * we did in the board object. So we need to calcuate the checksum here.
             */
            checksum = calculateResumeChecksum(&pResumePromInfo->resume_prom_data);

            fbe_copy_memory(pWriteResumeCmd->pBuffer, 
                        &checksum,
                        sizeof(fbe_u32_t));

            pWriteResumeCmd->bufferSize = sizeof(fbe_u32_t);
        }

        status = fbe_api_resume_prom_write_sync(pResumePromInfo->objectId, pWriteResumeCmd);
        

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, fbe_api_resume_prom_write_sync failed.\n", 
                                  __FUNCTION__);
        }
        else
        {
            if(issue_read)
            {
                /* Initiate the resume prom read for the device again to update the resume prom copy saved. */
                status = pBaseEnv->resume_prom_element.pInitiateResumePromReadCallback(pBaseEnv,
                                                                                       pWriteResumeCmd->deviceType,
                                                                                       &pWriteResumeCmd->deviceLocation);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "RP: failed to initiate resume read after write, deviceType %s(.\n",
                                          fbe_base_env_decode_device_type(pWriteResumeCmd->deviceType));
                }
            }
        }
    }

    if(pWriteResumeCmd->pBuffer != NULL)
    {
        /* Release the memory allocated. */
        fbe_transport_release_buffer(pWriteResumeCmd->pBuffer);
    }

    /* Restore the old buffer pointer. */
    pWriteResumeCmd->pBuffer = pBufferSaved;

    return status;
} // End of function fbe_base_env_send_resume_prom_write_sync_cmd

/*!*************************************************************************
 * @fn fbe_base_env_send_resume_prom_write_async_cmd()
 **************************************************************************
 * @brief
 *      This function issues the Resume prom write async command for a particular device.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param rpWriteAsynchOp - The pointer to the resume prom async op structure.    
 *
 * @return  fbe_status_t
 *
 * @note 
 *
 * @version
 *  17-Dec-2012 Dipak Patel - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_send_resume_prom_write_async_cmd(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp)
{
    fbe_base_env_resume_prom_info_t          * pResumePromInfo = NULL;
    fbe_status_t                               status = FBE_STATUS_OK;
    void                                     * pBufferSaved = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry.\n",
                          __FUNCTION__); 

    if(rpWriteAsynchOp->rpWriteAsynchCmd.bufferSize > FBE_MEMORY_CHUNK_SIZE) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s, bufferSize %d too large.\n", 
                              __FUNCTION__, rpWriteAsynchOp->rpWriteAsynchCmd.bufferSize);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    pBufferSaved = rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer;

    /* Allocate the non-paged physically contiguous memory so that we don't need the use
    * to allocate the non-paged physically contiguous memory.
    */
    rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer = fbe_transport_allocate_buffer();
    if(rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, mem alloc failed.\n",
                                  __FUNCTION__);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_zero_memory(rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer, FBE_MEMORY_CHUNK_SIZE);

    fbe_copy_memory(rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer, 
                    pBufferSaved,
                    rpWriteAsynchOp->rpWriteAsynchCmd.bufferSize);


    /* Call the appropriate device to get the pointer to the resume prom info. */
    status = pBaseEnv->resume_prom_element.pGetResumePromInfoPtrCallback(pBaseEnv, 
                                                       rpWriteAsynchOp->rpWriteAsynchCmd.deviceType,
                                                       &rpWriteAsynchOp->rpWriteAsynchCmd.deviceLocation,
                                                       &pResumePromInfo);
    if (status == FBE_STATUS_NO_DEVICE)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s Invalid device type: %s, status: 0x%X\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(rpWriteAsynchOp->rpWriteAsynchCmd.deviceType), 
                              status);

        /* Free the allocated memory */
        fbe_transport_release_buffer(rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer);
        rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer = NULL;

    }
    else if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s failed. status: 0x%X\n", 
                              __FUNCTION__, status);

        /* Free the allocated memory */
        fbe_transport_release_buffer(rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer);
        rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer = NULL;
    }
    else
    {

        status = fbe_api_resume_prom_write_async(pResumePromInfo->objectId, rpWriteAsynchOp);

        if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
        {
            fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: %s, fbe_api_resume_prom_write_sync failed.\n", 
                                  __FUNCTION__);

            /* Free the allocated memory */
            fbe_transport_release_buffer(rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer);
            rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer = NULL;
        }
    }

    return status;
} // End of function fbe_base_env_send_resume_prom_write_sync_cmd

/*!*************************************************************************
 * @fn fbe_base_env_resume_prom_validata_resume_write_cmd()
 **************************************************************************
 * @brief
 *      This function validates the Resume prom write sync command.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 * @param pWriteResumePromCmd - The pointer to the resume prom cmd.
 * @param pCmdValid (output)
 *
 * @return  fbe_status_t
 *
 * @version
 *  15-Jul-2011 PHE  - Created.
 *
 **************************************************************************/
static fbe_status_t 
fbe_base_env_resume_prom_validata_resume_write_cmd(fbe_base_environment_t * pBaseEnv, 
                                     fbe_resume_prom_cmd_t * pWriteResumePromCmd,
                                     fbe_bool_t * pCmdValid)
{
    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry.\n",
                          __FUNCTION__); 

    if(pWriteResumePromCmd->resumePromField == RESUME_PROM_CHECKSUM)
    {
        *pCmdValid = FBE_TRUE;
    }
    else if((pWriteResumePromCmd->deviceType == FBE_DEVICE_TYPE_ENCLOSURE) && 
            ((pWriteResumePromCmd->resumePromField == RESUME_PROM_WWN_SEED) ||
             (pWriteResumePromCmd->resumePromField == RESUME_PROM_PRODUCT_SN) || 
             (pWriteResumePromCmd->resumePromField == RESUME_PROM_PRODUCT_PN) ||
             (pWriteResumePromCmd->resumePromField == RESUME_PROM_PRODUCT_REV) ||
             (pWriteResumePromCmd->resumePromField == RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM) || 
             (pWriteResumePromCmd->resumePromField == RESUME_PROM_EMC_SUB_ASSY_PART_NUM)))
    {
            /* Resume PROM on the SP has TLA fields, that are not present on any other Resume PROM.
             * i.e. EMC_SERIAL_NUM on Midplane Resume PROM has the same address as TLA Serial Num
             * on SP resume PROM. Since we have one structure for all the resume PROMs, convert the 
             * field to be written to EMC_TLA_XXX
             */ 
            if(pWriteResumePromCmd->resumePromField == RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM)
            {
                pWriteResumePromCmd->resumePromField = RESUME_PROM_EMC_TLA_SERIAL_NUM;
            }
            else if(pWriteResumePromCmd->resumePromField == RESUME_PROM_EMC_SUB_ASSY_PART_NUM)
            {
                pWriteResumePromCmd->resumePromField = RESUME_PROM_EMC_TLA_PART_NUM;
            }

            *pCmdValid = FBE_TRUE;
    }
    else
    {
        *pCmdValid = FBE_FALSE;
    }

    if(*pCmdValid == FBE_FALSE) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "Resume write cmd invalid, deviceType %lld, field %d.\n",
                          pWriteResumePromCmd->deviceType, pWriteResumePromCmd->resumePromField); 
    }
    else
    {
        //update the offset by the resume prom field.
        pWriteResumePromCmd->offset = getResumeFieldOffset(pWriteResumePromCmd->resumePromField);
    }

    return FBE_STATUS_OK;
}

fbe_u32_t fbe_base_env_resume_prom_getFieldOffset(RESUME_PROM_FIELD resume_field)
{
    return(getResumeFieldOffset(resume_field));
}


/*!*************************************************************************
 * @fn fbe_base_env_resume_prom_updateResPromFailed()
 **************************************************************************
 * @brief
 *      This function will update the appropriate component's
 *      Resume prom read failed status.
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
fbe_base_env_resume_prom_updateResPromFailed(fbe_base_environment_t * pBaseEnv,
                                             fbe_u64_t deviceType,
                                             fbe_device_physical_location_t *pLocation,
                                             fbe_bool_t newValue)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t    *mgmtPtr;

    // determine the device type
    switch (deviceType)
    {
    case FBE_DEVICE_TYPE_SP:
        mgmtPtr = (fbe_u8_t *)pBaseEnv;
        fbe_board_mgmt_updateSpFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_RP_READ_FAILURE, newValue);
        break;
    case FBE_DEVICE_TYPE_PS:
        mgmtPtr = (fbe_u8_t *)pBaseEnv;
        fbe_ps_mgmt_updatePsFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_RP_READ_FAILURE, newValue);
        break;
    case FBE_DEVICE_TYPE_BATTERY:
        mgmtPtr = (fbe_u8_t *)pBaseEnv;
        fbe_sps_mgmt_updateBbuFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_RP_READ_FAILURE, newValue);
        break;
    case FBE_DEVICE_TYPE_FAN:
        mgmtPtr = (fbe_u8_t *)pBaseEnv;
        fbe_cooling_mgmt_updateFanFailStatus(mgmtPtr, pLocation, FBE_BASE_ENV_RP_READ_FAILURE, newValue);
        break;
    default:
        break;
    }

    return status;

}   // end of fbe_base_env_resume_prom_updateResPromFailed

/*!*************************************************************************
 * @fn fbe_base_env_init_resume_prom_info()
 **************************************************************************
 * @brief
 *      This function initializes the resume prom info.
 *
 * @param pResumePromInfo - The pointer to the resume prom info.
 *
 * @return  fbe_status_t
 *
 * @version
 *  17-Dec-2013 PHE  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_init_resume_prom_info(fbe_base_env_resume_prom_info_t * pResumePromInfo)
{
    /* Init the op_state so we can initiate the first read prom read.
     */
    pResumePromInfo->op_status = FBE_RESUME_PROM_STATUS_UNINITIATED;

    /* Init the objectId and location in psResumeInfo structure.
     * If the device is not inserted at the beginning, we would not initiate the resume prom read. 
     * So the objectId and location in psResumeInfo should be initiated.
     */
    pResumePromInfo->objectId = FBE_OBJECT_ID_INVALID;
    pResumePromInfo->location.bus = FBE_INVALID_PORT_NUM;
    pResumePromInfo->location.enclosure = FBE_INVALID_ENCL_NUM;
    pResumePromInfo->location.slot = FBE_INVALID_SLOT_NUM;  

    return FBE_STATUS_OK;
}

//End of file
