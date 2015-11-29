/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_BASE_ENVIORNMENT_RESUME_PROM_PRIVATE_H
#define FBE_BASE_ENVIORNMENT_RESUME_PROM_PRIVATE_H

/*!**************************************************************************
 * @file fbe_base_environment_resume_prom_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the base environment
 *  resume prom functionality.
 * 
 * @ingroup base_environment_class_files
 * 
 * @revision
 *   02-Nov-2010:  Arun S - Created.
 *
 ***************************************************************************/
#include "fbe/fbe_devices.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe_base_object.h"
#include "base_object_private.h"

#define FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE       30
#define FBE_BASE_ENV_RESUME_PROM_MAX_RETRY_COUNT        10
#define FBE_BASE_ENV_RESUME_PROM_RETRY_INTERVAL        3000     // 3 seconds
/*!****************************************************************************
 *    
 * @struct fbe_base_env_resume_prom_info_s
 *  
 * @brief 
 *   This is the definition of the Resume PROM information. 
 ******************************************************************************/
typedef struct fbe_base_env_resume_prom_info_s
{
    fbe_object_id_t                  objectId; // Target object id.
    fbe_u64_t                device_type;
    fbe_device_physical_location_t   location;
    fbe_resume_prom_status_t         op_status;
    fbe_bool_t                       resumeFault;
    RESUME_PROM_STRUCTURE            resume_prom_data;  
}fbe_base_env_resume_prom_info_t;

typedef struct fbe_base_env_resume_prom_work_item_s
{
    /*! This must be first. */
    fbe_queue_element_t			           queueElement; /* MUST be first */
    fbe_object_id_t                        object_id;
    fbe_u64_t                      device_type;
    fbe_device_physical_location_t         location;
    fbe_resume_prom_get_resume_async_t     *pGetResumeCmd;
    fbe_resume_prom_status_t               resume_status;
    fbe_base_env_resume_prom_work_state_t  workState;
    fbe_u8_t                               retryCount;
    fbe_time_t                             readCmdTimeStamp;
    fbe_u8_t                               logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1];
}fbe_base_env_resume_prom_work_item_t;

typedef fbe_status_t (* fbe_base_env_resume_prom_read_async_completion_callback_func_ptr_t)(void * pContext, 
                                                                                            fbe_u64_t deviceType, 
                                                                                            fbe_device_physical_location_t pLocation);

typedef fbe_status_t (* fbe_base_env_resume_prom_get_completion_status_callback_func_ptr_t)(void * pContext, 
                                                                                            fbe_u64_t deviceType, 
                                                                                            fbe_device_physical_location_t *pLocation,
                                                                                            fbe_base_env_resume_prom_info_t *resume_info);

typedef fbe_status_t (* fbe_base_env_initiate_resume_prom_read_callback_func_ptr_t)(void * pContext, 
                                                                                    fbe_u64_t deviceType, 
                                                                                    fbe_device_physical_location_t *pLocation);

typedef fbe_status_t (* fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t)(void * pContext, 
                                                                      fbe_u64_t deviceType,
                                                                      fbe_device_physical_location_t * pLocation,
                                                                      fbe_base_env_resume_prom_info_t **pResumePromInfoPtr);

typedef fbe_status_t (* fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t)(void * pCallbackContext, 
                                                                               fbe_u64_t deviceType,
                                                                               fbe_device_physical_location_t *pLocation);

typedef struct fbe_base_env_resume_prom_element_s 
{
    /*! Queue to hold all the work items */
    fbe_queue_head_t workItemQueueHead;

    /*! Lock for the work item queue */
    fbe_spinlock_t workItemQueueLock;

    fbe_base_env_initiate_resume_prom_read_callback_func_ptr_t         pInitiateResumePromReadCallback;
    fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t          pGetResumePromInfoPtrCallback;
    fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t pUpdateEnclFaultLedCallback;

    void * pCallbackContext;
}fbe_base_env_resume_prom_element_t;

#endif /* FBE_BASE_ENVIORNMENT_RESUME_PROM_PRIVATE_H */
