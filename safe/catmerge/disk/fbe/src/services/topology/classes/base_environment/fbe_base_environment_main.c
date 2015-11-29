/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the base
 *  environment object. This includes the create and destroy
 *  methods for the enclosure management object, as well as the
 *  event entry point, load and unload methods.
 * 
 * @ingroup base_environment_class_files
 * 
 * @version
 *   25-Feb-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_file.h"
#include "fbe_topology.h"
#include "fbe_base_environment_private.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_notification.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_base_object_trace.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_registry.h"

/* Class methods forward declaration */
fbe_status_t fbe_base_environment_load(void);
fbe_status_t fbe_base_environment_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_base_environment_class_methods = { FBE_CLASS_ID_BASE_ENVIRONMENT,
                                                            fbe_base_environment_load,
                                                            fbe_base_environment_unload,
                                                            fbe_base_environment_create_object,
                                                            fbe_base_environment_destroy_object,
                                                            fbe_base_environment_control_entry,
                                                            fbe_base_environment_event_entry,
                                                            NULL,
                                                            fbe_base_environment_monitor_entry
                                                         };


/* Local Function Prototypes */
fbe_status_t fbe_base_environment_event_notification_call_back(update_object_msg_t * update_object_msg, void * context);
fbe_status_t fbe_base_environment_cmi_notification_call_back(fbe_cmi_event_t cmi_event, 
                                                          fbe_u32_t msg_length,
                                                          fbe_cmi_message_t msg, 
                                                          fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_base_env_init_basic_info(fbe_base_environment_t * pBaseEnv);
static fbe_status_t fbe_base_env_event_destroy_queue(fbe_base_environment_t * pBaseEnv);
static fbe_status_t fbe_base_env_get_persist_db_state(fbe_base_environment_t * pBaseEnv);
static void fbe_base_env_cmi_destroy_queue(fbe_base_environment_t *base_environment);
static void fbe_base_env_reboot_sp_sync_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context);
static void fbe_base_env_reboot_sp_async_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context);

/*!***************************************************************
 * fbe_base_environment_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return fbe_base_environment_monitor_load_verify();
}
/******************************************
 * end fbe_base_environment_load()
 ******************************************/

/*!***************************************************************
 * fbe_base_environment_unload()
 ****************************************************************
 * @brief
 *  This function is called to perform any operation during
 *  unload
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_environment_unload()
 ******************************************/

/*!***************************************************************
 * fbe_base_environment_create_object()
 ****************************************************************
 * @brief
 *  This is the main entry point for creating the base
 *  environment object which calls the parent constructor here
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_base_environment_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_base_environment_t * base_environment;

    /* Call parent constructor */
    status = fbe_base_object_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s failed to create base environment object. Status:%d\n", 
                                 __FUNCTION__, status);
        return status;
    }

    /* Set class id */
    base_environment = (fbe_base_environment_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) base_environment, FBE_CLASS_ID_BASE_ENVIRONMENT);

    /* Init the variable in the base environment data structure
     * here
     */
    fbe_base_environment_init(base_environment);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_environment_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_base_environment_destroy_object()
 ****************************************************************
 * @brief
 *  This function performs the action necessary to clean up due
 *  to the destruction of the object
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_base_environment_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_base_environment_t * base_environment;
    const fbe_u32_t memory_ex_allocate_check_retry_count_max = 10;
    fbe_u32_t memory_ex_allocate_check_retry_count;

    base_environment = (fbe_base_environment_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)base_environment,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Unregister with any service that we would registered during
     * the creation
     */
    status = fbe_api_notification_interface_unregister_notification((fbe_api_notification_callback_function_t)fbe_base_environment_event_notification_call_back, /* SAFEBUG - temporary adding cast for now to supress warning but this function should not return status */ 
                                                                    base_environment->event_element.notification_registration_id);
    status = fbe_cmi_unregister(base_environment->cmi_element.client_id);

    /* Destroy the firmware upgrade work item queue and release images.*/
    fbe_base_env_fup_destroy_queue(base_environment);
    fbe_base_env_fup_release_image(base_environment);
    /* Note: It needs to be called after fbe_base_env_fup_release_image. */
    
    fbe_base_env_memory_ex_release(base_environment, base_environment->fup_element_ptr);

    /* Destroy the Resume prom work item queue and release images.*/
    fbe_base_env_resume_prom_destroy_queue(base_environment);

    fbe_base_env_event_destroy_queue(base_environment);

    fbe_base_env_cmi_destroy_queue(base_environment);

    if(base_environment->my_persistent_data != NULL)
    {
        fbe_base_env_memory_ex_release(base_environment, base_environment->my_persistent_data);
    }

    //check if there are any unreleased memory
    for(memory_ex_allocate_check_retry_count = 0;
            memory_ex_allocate_check_retry_count < memory_ex_allocate_check_retry_count_max;
            memory_ex_allocate_check_retry_count++)
    {
        if( base_environment->memory_ex_allocate_count != 0 )
        {
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                    FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Waiting for outstanding request! ClassId: %s, memory ex allocate count %d.\n",
                    fbe_base_env_decode_class_id(base_environment->base_object.class_id),
                    base_environment->memory_ex_allocate_count);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)base_environment,
                    FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Allocate memory check finished, no memory leak. ClassId: %s.\n",
                    fbe_base_env_decode_class_id(base_environment->base_object.class_id));
            break;
        }
        fbe_thread_delay(500);
    }

    /* Destroy the spinlock*/
    fbe_spinlock_destroy(&base_environment->base_env_lock);

    /* Call parent destructor */
    status = fbe_base_object_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_base_environment_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_base_environment_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_base_environment_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_environment_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_base_environment_init()
 ****************************************************************
 * @brief
 *  This function init any object data structure
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_base_environment_init(fbe_base_environment_t * base_environment)
{
    fbe_u8_t                     * pImagePath = NULL;
    fbe_u32_t                      imagePathIndex = 0;
    fbe_u32_t                      subEnclIndex = 0;
    fbe_u32_t                      imageIndex = 0;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;
        
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    /* Init the spinlock. 
     * The spin lock base_env_lock is used while calling fbe_base_env_memory_ex_allocate.
     */
    fbe_spinlock_init(&base_environment->base_env_lock);

    fbe_queue_init(&base_environment->event_element.event_queue);
    fbe_spinlock_init(&base_environment->event_element.event_queue_lock);
    fbe_queue_init(&base_environment->cmi_element.cmi_callback_queue);
    fbe_spinlock_init(&base_environment->cmi_element.cmi_queue_lock);

    base_environment->fup_element_ptr = fbe_base_env_memory_ex_allocate(base_environment, sizeof(fbe_base_env_fup_element_t));

    if(base_environment->fup_element_ptr == NULL) 
    {
        fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
                             FBE_TRACE_LEVEL_ERROR, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s, size %d, failed to allocate memory for fup_element.\n", 
                             __FUNCTION__, (int)sizeof(fbe_base_env_fup_element_t));

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Init firmware upgrade related fields. */
    fbe_zero_memory(base_environment->fup_element_ptr, sizeof(fbe_base_env_fup_element_t));

    fbe_queue_init(&base_environment->fup_element_ptr->workItemQueueHead);
    fbe_spinlock_init(&base_environment->fup_element_ptr->workItemQueueLock);

    for(imagePathIndex = 0; imagePathIndex < FBE_BASE_ENV_FUP_MAX_NUM_IMAGE_PATH; imagePathIndex ++) 
    {
        pImagePath = fbe_base_env_get_fup_image_path_ptr(base_environment, imagePathIndex);
        fbe_copy_memory(pImagePath, "NULL", 4);
    }

    for(subEnclIndex = 0; subEnclIndex < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclIndex ++) 
    {
        pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(base_environment, subEnclIndex);
        fbe_set_memory(&pManifestInfo->subenclProductId[0], ' ', FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE);

        for(imageIndex = 0; imageIndex < FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL; imageIndex ++) 
        {
            pManifestInfo->firmwareCompType[imageIndex] = SES_COMP_TYPE_UNKNOWN;
            pManifestInfo->firmwareTarget[imageIndex] = FBE_FW_TARGET_INVALID;
        }
    }

    base_environment->timeStartToWaitBeforeUpgrade = 0;

    base_environment->waitBeforeUpgrade = FBE_TRUE;
    base_environment->abortUpgradeCmdReceived = FBE_FALSE;

    /* Init Resume Prom related fields. */
    fbe_zero_memory(&base_environment->resume_prom_element, sizeof(fbe_base_env_resume_prom_element_t));
    fbe_queue_init(&base_environment->resume_prom_element.workItemQueueHead);
    fbe_spinlock_init(&base_environment->resume_prom_element.workItemQueueLock);

    base_environment->read_persistent_data_retries = 0;

    fbe_base_env_init_basic_info(base_environment);

    /* fbe_base_env_init_fup_params should be called after fbe_base_env_init_basic_info
     * because it needs to know what SP it is running on.
     */
    fbe_base_env_init_fup_params(base_environment, 
                                 FBE_BASE_ENV_UPGRADE_DELAY_AT_BOOT_UP_ON_SPA,
                                 FBE_BASE_ENV_UPGRADE_DELAY_AT_BOOT_UP_ON_SPB);

    base_environment->requestPeerRevision = NO_PEER_REVISION_REQUEST;
    base_environment->peerRevision = 0;

    base_environment->isBootFlash = 
        fbe_base_environment_isSpInBootflash((fbe_base_object_t *)base_environment);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_environment_init()
 ******************************************/

/*!***************************************************************
 * fbe_base_environment_init_persistent_storage_data()
 ****************************************************************
 * @brief
 *  This function initializes the persistent storage data members.
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Dec-2010 bphilbin - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_init_persistent_storage_data(fbe_base_environment_t * base_environment, fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_OK;

    base_environment->my_persistent_data = NULL;
    base_environment->my_persistent_entry_id = 0;
    base_environment->wait_for_peer_data = PERSISTENT_DATA_WAIT_NONE;
    base_environment->read_outstanding = FALSE;

    base_environment->my_persistent_id = fbe_base_environment_convert_class_id_to_persistent_id(base_environment,((fbe_base_object_t *)base_environment)->class_id);

    if(base_environment->my_persistent_data_size != 0) 
    {
        base_environment->my_persistent_data = fbe_base_env_memory_ex_allocate(base_environment, base_environment->my_persistent_data_size);
        if(base_environment->my_persistent_data == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error failed to allocate Persistent Storage data.\n",
                              __FUNCTION__);
        }
        else
        {
            fbe_set_memory(base_environment->my_persistent_data, 0, base_environment->my_persistent_data_size);
        }
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error Persistent Storage data size not set.\n",
                              __FUNCTION__);
    }
    return status;
    
}

fbe_status_t fbe_base_environment_peer_gone(fbe_base_environment_t *base_environment)
{
    fbe_status_t status;
    fbe_lifecycle_cond_id_t condition  = 0;

    fbe_base_env_fup_processSpContactLost(base_environment);

    if(base_environment->wait_for_peer_data == PERSISTENT_DATA_WAIT_READ)
    {
        condition = FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA;
    }
    else if(base_environment->wait_for_peer_data == PERSISTENT_DATA_WAIT_WRITE)
    {
        condition = FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA;
    }
    /*
     * We were waiting for the peer's response but they went away.  Take care
     * of the outstanding operation ourselves.
     */
    base_environment->wait_for_peer_data = PERSISTENT_DATA_WAIT_NONE;

    if(condition != 0)
    {
         /* Set the condition to read the data from persistent storage service. */
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                         (fbe_base_object_t *) base_environment,
                                         condition);
        if(status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_environment,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s setting LIFECYCLE_COND:0x%x, status:0x%x.\n",
                                      __FUNCTION__, condition, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)base_environment,FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Failed setting LIFECYCLE_COND:0x%x, status:0x%x.\n",
                                      __FUNCTION__, condition, status);
        }
    }

    
    return FBE_STATUS_OK;
}



/*!***************************************************************
 * fbe_base_environment_set_persistent_storage_data_size()
 ****************************************************************
 * @brief
 *  This function sets the size of the persistent data stored for
 *  this object. This should be set by the leaf class before requesting
 *  to initialize the persistent storage data.
 *  
 * @param  - base_environment - object context pointer.
 * @param  - size - size in bytes of persistent data for this object.
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Dec-2010 bphilbin - Created. 
 *
 ****************************************************************/
void fbe_base_environment_set_persistent_storage_data_size(fbe_base_environment_t * base_environment, fbe_u32_t size)
{
    fbe_base_object_trace((fbe_base_object_t *)base_environment,  
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s to size:%d bytes.\n",
                              __FUNCTION__, size);
    base_environment->my_persistent_data_size = size;
}

/*!***************************************************************
 * fbe_base_env_init_basic_info()
 ****************************************************************
 * @brief
 *  This function gets the spid and the platform info and 
 *  saves them in the base_environment.
 *
 * @param pBaseEnv - 
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_base_env_init_basic_info(fbe_base_environment_t * pBaseEnv)
{
    fbe_object_id_t objectId;
    fbe_board_mgmt_misc_info_t miscInfo = {0};
    fbe_board_mgmt_platform_info_t platformInfo = {0};
    fbe_status_t status = FBE_STATUS_OK;
    
    status = fbe_api_get_board_object_id(&objectId);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);
        return status;
    }

    status = fbe_api_board_get_misc_info(objectId, &miscInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the base board misc info, status: 0x%X\n",
                              __FUNCTION__, status);
        return status;
    }

    pBaseEnv->spid = miscInfo.spid;

    status = fbe_api_board_get_platform_info(objectId, &platformInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the base board misc info, status: 0x%X\n",
                              __FUNCTION__, status);
        return status;
    }

    pBaseEnv->processorEnclType = platformInfo.hw_platform_info.enclosureType;
    pBaseEnv->isSingleSpSystem = platformInfo.is_single_sp_system;
    pBaseEnv->isSimulation = platformInfo.is_simulation;

    /* Read SEP configuration from registry.
     * SEP is disabled at Virtual platform and SEP persistend DB is not used.
     */
    fbe_base_env_get_persist_db_state(pBaseEnv);

    /* Assume the boot enclosure location is 0.
     * If we support the boot enclosure which is not 0_0, we need to update this code.
     */
    fbe_zero_memory(&pBaseEnv->bootEnclLocation, sizeof(fbe_device_physical_location_t));

    /* The enclosure location for the OS drives. */
    fbe_zero_memory(&pBaseEnv->enclLocationForOsDrives, sizeof(fbe_device_physical_location_t));

    /* init peerCacheStatus */
    pBaseEnv->peerCacheStatus = FBE_CACHE_STATUS_UNINITIALIZE;

    return FBE_STATUS_OK;
}

/**************************************************************************
 *          fbe_base_env__get_persist_db_state
 * ************************************************************************
 *  @brief
 *  Called to get persist DB usage status.
 *
 *  @param persist_db_state - whether or not persist DB is used installed at the system
 *    Virtual platform may run w/o SEP and therefore w/o persist DB
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  29-Oct-14: Yuri Stotski - Created
 * ***********************************************************************/
fbe_status_t fbe_base_env_get_persist_db_state(fbe_base_environment_t * pBaseEnv)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue;

    DefaultInputValue = 0;

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_MODULE_MGMT_PERSIST_DB_DISABLED_KEY,
                               &pBaseEnv->persist_db_disabled,
                               sizeof(fbe_bool_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &DefaultInputValue,
                               sizeof(fbe_bool_t),
                               TRUE);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting persist db state registry entry, status: 0x%X\n",
                              __FUNCTION__, status);

        pBaseEnv->persist_db_disabled = FBE_FALSE;
    }

    return (status);
}

/*!***************************************************************
 * fbe_base_env_get_and_adjust_encl_location()
 ****************************************************************
 * @brief
 *  This function gets the enclosure location based on the object id.
 *  If the object type is Board, it will adjust the enclosure location
 *  based on the processor enclosure type. 
 *
 * @param pBaseEnv - 
 * @param objectId - The could be either the encl object id or the subencl object id. 
 * @param pLocation - 
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_get_and_adjust_encl_location(fbe_base_environment_t * pBaseEnv,
                                                fbe_object_id_t objectId,
                                                fbe_device_physical_location_t * pLocation)
{
    fbe_topology_object_type_t   objectType = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    HW_ENCLOSURE_TYPE            processorEnclType = INVALID_ENCLOSURE_TYPE;
    fbe_port_number_t            portNum = 0;
    fbe_enclosure_number_t       enclNum = 0;
    fbe_status_t                 status = FBE_STATUS_OK;

    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,  
                           FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s Error in getting Object Type for objectId %d, status: 0x%x.\n",
                           __FUNCTION__, objectId, status);
        return status;
    }

    switch(objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        status = fbe_base_env_get_processor_encl_type(pBaseEnv, &processorEnclType);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,  
                           FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s Error in getting peType, status: 0x%x.\n",
                           __FUNCTION__, status);
            return status;
        }
    
        if(processorEnclType == XPE_ENCLOSURE_TYPE)
        {
            pLocation->bus = FBE_XPE_PSEUDO_BUS_NUM;
            pLocation->enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        else if (processorEnclType == VPE_ENCLOSURE_TYPE)
        {
            pLocation->bus = FBE_XPE_PSEUDO_BUS_NUM;
            pLocation->enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }

        else if(processorEnclType == DPE_ENCLOSURE_TYPE) 
        {
            pLocation->bus = pBaseEnv->bootEnclLocation.bus;
            pLocation->enclosure = pBaseEnv->bootEnclLocation.enclosure;
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,  
                           FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s Invalid peType %d.\n",
                           __FUNCTION__, processorEnclType);
            return status;
        }
        break;

    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
    case FBE_TOPOLOGY_OBJECT_TYPE_LCC:

        status = fbe_api_get_object_port_number (objectId, &portNum);
        if (status != FBE_STATUS_OK) 
        {
            return status;
        }

        status = fbe_api_get_object_enclosure_number (objectId, &enclNum);
        if (status != FBE_STATUS_OK) 
        {
            return status;
        }

        pLocation->bus = portNum;
        pLocation->enclosure = enclNum;
      
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,  
                           FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s unhandled objectType %d, objectId %d.\n",
                           __FUNCTION__, (int)objectType, objectId);

        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_base_env_is_processor_encl()
 ****************************************************************
 * @brief
 *  This function returns whether this is the processor enclosure. 
 *
 * @param pBaseEnv (INPUT) - 
 * @param pLocation (INPUT)- The pointer to the location of the enclosure.
 * @param pProcessorEncl (OUTPUT) - 
 *
 * @return fbe_status_t
 *
 * @author
 *  16-Mar-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_is_processor_encl(fbe_base_environment_t * pBaseEnv,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_bool_t * pProcessorEncl)
{
    HW_ENCLOSURE_TYPE        processorEnclType = INVALID_ENCLOSURE_TYPE;
    fbe_status_t             status = FBE_STATUS_OK;

    /* Init to FALSE. */
    *pProcessorEncl = FALSE;

    if((pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) &&
       (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
    {
        *pProcessorEncl = TRUE;
    }
    else
    {
        status = fbe_base_env_get_processor_encl_type(pBaseEnv, &processorEnclType);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,  
                           FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s Error in getting peType, status: 0x%x.\n",
                           __FUNCTION__, status);
            return status;
        }
    
        if((processorEnclType == DPE_ENCLOSURE_TYPE) &&
           (pLocation->bus == pBaseEnv->bootEnclLocation.bus) &&
           (pLocation->enclosure == pBaseEnv->bootEnclLocation.enclosure))
        {
            *pProcessorEncl = TRUE;
        }
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_base_environment_register_event_notification()
 ****************************************************************
 * @brief
 *  This function registers with FBE API for notification of
 *  events.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_register_event_notification(fbe_base_environment_t * base_environment,
                                                              fbe_api_notification_callback_function_t call_back, 
                                                              fbe_notification_type_t state_mask,
                                                              fbe_u64_t device_mask,
                                                              void *context,
                                                              fbe_topology_object_type_t object_type,
                                                              fbe_package_id_t package_id)
{
    fbe_status_t status;

    /* Save the callers registeration info and register with the
     * FBE API with our own call back handler. This way the base
     * environment can hide the details that are required to swtich
     * context on call back which will be called in FBE API context
     */
    base_environment->event_element.event_call_back = call_back;
    base_environment->event_element.context = context;
    base_environment->event_element.life_cycle_mask = state_mask;
    base_environment->event_element.device_mask = device_mask;

    /* Pass the other registeration information down to the FBE
     * API
     */
    status = fbe_api_notification_interface_register_notification(state_mask, package_id, object_type,
                                                                  (fbe_api_notification_callback_function_t)fbe_base_environment_event_notification_call_back, /* SAFEBUG - temporary adding cast for now to supress warning but this function should not return status */ 
                                                                  base_environment,
                                                                  &base_environment->event_element.notification_registration_id);
    return status;

}
/*********************************************************
 * end fbe_base_environment_register_event_notification()
 ********************************************************/
/*!***************************************************************
 * fbe_base_environment_event_notification_call_back()
 ****************************************************************
 * @brief
 *  This is the callback function that is called by the API to
 *  notify event changes. This runs in the context of the API
 *  and so we just queue the notification event and then set the
 *  condition rotary so that the handling will be in object's
 *  context
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_event_notification_call_back(update_object_msg_t * update_object_msg, void * context)
{
    fbe_base_environment_t *base_environment = (fbe_base_environment_t *) context;
    update_object_msg_t * event_msg;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_lifecycle_state_t               lifecycleState;

    /* We got notification, we need to validate to make sure if
     * this is an event the leaf class cares about before queuing
     * it
     */
    /* If the leaf class has registered for object data change and
     * this is a specific notification that object data has changed,
     * we need to make sure the device type matches. If not just
     * return from here because the leaf class does not care about
     * the notification for this particular device type
     */
    if(base_environment->event_element.life_cycle_mask & FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED) {
        if(notification_info->notification_type == FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED) {
            if((base_environment->event_element.device_mask & 
                notification_info->notification_data.data_change_info.device_mask) == 0) {
                /* This is a notification the leaf class does not care about
                 * and so just return from here
                 */
                return status;
            }
        }
    }

    event_msg = (update_object_msg_t*)fbe_base_env_memory_ex_allocate(base_environment, sizeof(update_object_msg_t));
    if(event_msg == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Queue the event and set the condition so that it is handled
     * in the object's context.
     */
    fbe_copy_memory(event_msg, update_object_msg, sizeof(update_object_msg_t));
    fbe_spinlock_lock(&base_environment->event_element.event_queue_lock);
    fbe_queue_push(&base_environment->event_element.event_queue, (fbe_queue_element_t *)event_msg);
    fbe_spinlock_unlock(&base_environment->event_element.event_queue_lock);

    /* Check the lifecycle state. It is OK that we don't set HANDLE_EVENT_NOTIFICATION
     * condition if the object is not ready not. When the object becomes ready,
     * because HANDLE_EVENT_NOTIFICATION is the preset condition, the event will be processed
     * at that time. 
     */
    status = fbe_lifecycle_get_state(&fbe_base_environment_lifecycle_const, 
                                         (fbe_base_object_t*)base_environment, 
                                         &lifecycleState);

    if((status == FBE_STATUS_OK) && (lifecycleState == FBE_LIFECYCLE_STATE_READY))
    {
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                        (fbe_base_object_t *) base_environment, 
                                        FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_EVENT_NOTIFICATION);
        if(status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Failed to set the event notification condition\n",
                                    __FUNCTION__);
        }
    }

    return status;
}
/*********************************************************
 * end fbe_base_environment_event_notification_call_back()
 ********************************************************/
/*!***************************************************************
 * fbe_base_environment_register_cmi_notification()
 ****************************************************************
 * @brief
 *  This function register with the CMI on behalf of the leaf
 *  class. We need to do this because the CMI callback is from
 *  CMI Service context and we need to break the context which
 *  will be done in the base environment cmi callback function
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_register_cmi_notification(fbe_base_environment_t * base_environment,
                                                            fbe_cmi_client_id_t cmi_client,
                                                            fbe_base_environment_cmi_callback_function_t call_back)
{
    fbe_status_t status;

    /* Save the callback so that we can send the notification back
     * the leaf class from the base environment context
     */
    base_environment->cmi_element.cmi_callback = call_back;
    base_environment->cmi_element.client_id = cmi_client;
    status = fbe_cmi_register(cmi_client, 
                              fbe_base_environment_cmi_notification_call_back,
                              base_environment);
    return status;

}
/*********************************************************
 * end fbe_base_environment_register_cmi_notification()
 ********************************************************/
/*!***************************************************************
 * fbe_base_environment_cmi_notification_call_back()
 ****************************************************************
 * @brief
 *  This is the callback function that is called by the CMI
 *  Service to notify CMI messages. This runs in the context of
 *  the CMI Service and so we just queue the message and then
 *  set the condition rotary so that the handling will be in
 *  object's context
 **
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @notes 
 *  msg_length is only valid for received msg notifications.  All other cases
 *  will have zero passed in.
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_cmi_notification_call_back(fbe_cmi_event_t cmi_event, 
                                                          fbe_u32_t msg_length,
                                                          fbe_cmi_message_t msg, 
                                                          fbe_cmi_event_callback_context_t context)
{
    fbe_base_environment_t *base_environment = (fbe_base_environment_t *) context;
    fbe_base_environment_cmi_message_info_t *cmi_message;
    fbe_status_t status;
    
    cmi_message = (fbe_base_environment_cmi_message_info_t*)fbe_base_env_memory_ex_allocate(base_environment, sizeof(fbe_base_environment_cmi_message_info_t));

    if(cmi_message == NULL) {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s cmiMsg memory allocation failed, userMsg %p.\n",
                              __FUNCTION__, msg);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    if(cmi_event == FBE_CMI_EVENT_MESSAGE_RECEIVED)
    {
        /*
         * This is a message from the peer, we need to allocate memory here
         * for the message as the current memory for this belongs to the cmi_service
         * We can not use fbe_transport_allocate_buffer because we can not wait
         * in this context. 
         * In addition, we don't need the physically contiguous memory for the received message. 
         */
        cmi_message->user_message = (fbe_cmi_message_t *) fbe_base_env_memory_ex_allocate(base_environment, msg_length);
        if(cmi_message->user_message == NULL) {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s userMsg memory allocation failed.\n",
                                    __FUNCTION__);
            fbe_base_env_memory_ex_release(base_environment, cmi_message);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (msg != NULL)
        {
                fbe_copy_memory(cmi_message->user_message, msg, msg_length);
             fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s copying user_message %p length %d\n",
                                    __FUNCTION__, msg, msg_length);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s just assigning the user_message %p, cmiEvent %d\n",
                                    __FUNCTION__, 
                                    msg, cmi_event);
//                                    fbe_sps_mgmt_getCmiEventTypeString(cmi_event));
        /* 
         * This message is just being returned to the local SP, just need
         * to copy the original message pointer back 
         */
        cmi_message->user_message = msg;
    }
    
    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s cmiMsg %p, userMsg %p.\n",
                                    __FUNCTION__, cmi_message, cmi_message->user_message);

    /*
     * Queue the CMI message and set the condition so that it is
     * handled in the object's context.
     */
    cmi_message->event = cmi_event;
    fbe_spinlock_lock(&base_environment->cmi_element.cmi_queue_lock);
    fbe_queue_push(&base_environment->cmi_element.cmi_callback_queue, (fbe_queue_element_t *)cmi_message);
    fbe_spinlock_unlock(&base_environment->cmi_element.cmi_queue_lock);
    
    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const, 
                                    (fbe_base_object_t *) base_environment, 
                                    FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_CMI_NOTIFICATION);
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to set notification condition.\n",
                                __FUNCTION__);
    }

    return status;
}
/*********************************************************
 * end fbe_base_environment_cmi_notification_call_back()
 ********************************************************/

/*!***********************************************************
 *  @fn  fbe_base_env_release_cmi_msg_memory(fbe_base_environment_t *base_environment,
 *                               fbe_base_environment_cmi_message_info_t *cmi_message)
 ************************************************************
 * @berief
 *   This function releases the memory allocated for fbe_base_environment_cmi_message_info_t
 *   and the memory for user message.
 *
 * @param   base_environment  - The pointer to fbe_base_environment_t
 * @param   cmi_message - The pointer to fbe_base_environment_cmi_message_info_t
 * 
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   16-Feb-2011: PHE - Created.
 *
 ************************************************************/
fbe_status_t fbe_base_env_release_cmi_msg_memory(fbe_base_environment_t *base_environment,
                                fbe_base_environment_cmi_message_info_t *cmi_message)
{
    fbe_base_object_trace((fbe_base_object_t *)base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry, cmiMsg %p, userMsg %p\n",
                                __FUNCTION__, cmi_message,
                                cmi_message->user_message);

    switch(cmi_message->event)
    {
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            /* In fbe_base_environment_cmi_notification_call_back, 
             * we allocate the memory to save the user message. 
             * Because the CMI service will release the memory for the user message
             * after the call fbe_base_environment_cmi_notification_call_back completes. 
             * Therefore, here we processed the CMI message, we need to release the memory.
             */
            if(cmi_message->user_message != NULL) 
            {
                fbe_base_object_trace((fbe_base_object_t *)base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s, MESSAGE_RECEIVED, release userMsg %p\n",
                                __FUNCTION__, cmi_message->user_message);

                fbe_base_env_memory_ex_release(base_environment, cmi_message->user_message);
            }
        
            break;

        default:
            // For all other cases we need to free the user message
            if(cmi_message->user_message != NULL) 
            {
                fbe_base_object_trace((fbe_base_object_t *)base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s, other msg, release userMsg %p\n",
                                __FUNCTION__, cmi_message->user_message);

                fbe_transport_release_buffer(cmi_message->user_message);
            }
            break;
    }

    fbe_base_env_memory_ex_release(base_environment, cmi_message);

    return FBE_STATUS_OK;
}

/*!****************************************
 * fbe_base_environment_send_cmi_message()
 ******************************************
 * @brief
 *  This function is used by the leaf class to send message to
 *  the peer.  This function allocates the contiguous memory necessary
 *  to send a cmi message, callers to this function do not have to worry
 *  about the contiguous memory allocations.  The memory will be freed
 *  by base environment when handling the cmi message queue in the
 *  condition function.
 *
 * @param  - None.
 *
 * @return fbe_status_t - FBE_STATUS_OK
 * 
 * @notes : CMI always return success here. When there is an
 *  error, the callback will have the event code that indicates
 *  the status and the caller can handle it appropriately
 * 
 *  @author 25-Feb-2010
 *  - Created. Ashok Tamilarasan
 *  07-Mar-2013: PHE - Do not send the CMI message when the object state is detroy or pending destroy.
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_send_cmi_message(fbe_base_environment_t *base_environment,
                                                   fbe_u32_t message_length,
                                                   fbe_cmi_message_t message)
{
    fbe_cmi_message_t          *user_message = NULL;
    fbe_lifecycle_state_t       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_status_t                status = FBE_STATUS_OK;

    /* We need to check the object lifecycle state. 
     * If it is in pending destroy or destroy state, do NOT send the cmi message. 
     * (1) If this object sent the cmi message in pending destroy or destroy lifecycle state, 
     * the object might not get the cmi callback before it got destroyed.
     * If it happened, the allocated memory for cmi message would not have the chance to be released.
     * Thus we would have the memory leak.
     * (2) If the CMI message was sent after the CMI queue got destroyed, we would also have the memory leak.
     */
    status = fbe_lifecycle_get_state(&fbe_base_environment_lifecycle_const, 
                                    (fbe_base_object_t *)base_environment, &lifecycle_state);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s fbe_lifecycle_get_state failed, status 0x%X.", 
                            __FUNCTION__, status);

        return status;
    }

    if ((lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY)||
        (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s DO NOT send CMI message, lifecyle state %d.", 
                            __FUNCTION__, lifecycle_state);

        /* Return FBE_STATUS_OK since we are going down anyway. If we returns the FAILURE here,
         * The caller or the caller's caller might print the error ktrace which would fail the fbe_test. 
         */
        return FBE_STATUS_OK;
    }

    /* The message_length must be less than or equal to FBE_MEMORY_CHUNK_SIZE.
     * Otherwise, please send by using multiple messages. 
     */
    if(message_length > FBE_MEMORY_CHUNK_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s msg size %d larger than memory chunk size %d, need to use multiple messages.\n",
                                    __FUNCTION__, message_length, FBE_MEMORY_CHUNK_SIZE);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    // allocate the contiguous memory
    user_message = fbe_transport_allocate_buffer();

    // if the allocation fails something is very wrong
    if(user_message == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s unable to allocate contiguous memory for userMsg.\n",
                                    __FUNCTION__);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_copy_memory(user_message, message, message_length);

    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s sending userMsg %p.\n",
                                    __FUNCTION__, user_message);

    // send the message down to the cmi service, we will free it when it is transmitted
    fbe_cmi_send_message(base_environment->cmi_element.client_id,
                         message_length,
                         user_message,
                         (fbe_cmi_event_callback_context_t) base_environment);
    return FBE_STATUS_OK;
}
/*********************************************
 * end fbe_base_environment_send_cmi_message()
 **********************************************/

/*!***********************************************************
 *  @fn  fbe_base_env_file_read(char * fileName, fbe_u8_t * pBuffer, fbe_u32_t readSize,
 *                    fbe_u32_t fileAccessOptions, fbe_u32_t * pBytesRead)
 ************************************************************
 * @berief
 *   This function does open, read and close the file specified
 *   by the fileName.
 *
 * @param   pBaseEnv  - The pointer to fbe_base_environment_t
 * @param   fileName  - file to be read
 * @param   pBuffer   - the pointer to the buffer tp store the chunks of data read
 * @param   bytesToRead  - size of data to be read, in bytes.
 * @param   FileAccessOptions  - Desired access of file to be opened
 * @param   pBytesRead - pointer to int for actual bytes read
 *
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   15-June-2010: PHE - Created.
 *
 ************************************************************/
fbe_status_t 
fbe_base_env_file_read(fbe_base_environment_t * pBaseEnv, 
                       char * fileName, 
                       fbe_u8_t * pBuffer, 
                       fbe_u32_t bytesToRead,
                       fbe_u32_t fileAccessOptions, 
                       fbe_u32_t * pBytesRead)
{
    fbe_file_handle_t      fileHandle = FBE_FILE_INVALID_HANDLE;
    fbe_u32_t   actualBytesRead = 0;
    fbe_u32_t   status = FBE_STATUS_OK;

    // Open the file 
    fileHandle = fbe_file_open(fileName, fileAccessOptions, 0/* protection mode, unused */, NULL);

    if(fileHandle != FBE_FILE_INVALID_HANDLE)
    {
        // Read the file
        actualBytesRead = fbe_file_read(fileHandle, pBuffer, bytesToRead, NULL);
        
        if(actualBytesRead == FBE_FILE_ERROR)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, read file %s failed.\n", __FUNCTION__, fileName); 
            *pBytesRead = 0;
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            *pBytesRead = actualBytesRead;
            status = FBE_STATUS_OK;
        }
        // Close the file 
        if(fbe_file_close(fileHandle)== FBE_FILE_ERROR)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, close file %s failed.\n", __FUNCTION__, fileName);

            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {        
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, open file %s failed.\n", __FUNCTION__, fileName); 
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
} //End of function fbe_base_env_file_read

/*!***********************************************************
 *  @fn  fbe_base_env_get_file_size(fbe_base_environment_t * pBaseEnv, 
                                   char *pFileName, 
                                   fbe_u32_t *pFileSize)
 ************************************************************
 * @berief
 *  Open the file, call fbe_file_get_size to return the size
 *  of the file. NOTE: THIS FUNCTION TRUNCATES THE SIZE TO
 *  32 bits!
 *
 * @param   pBaseEnv  - The pointer to fbe_base_environment_t
 * @param   pFileName  - file to be read
 * @param   pFileSize - the size of the file
 *
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   15-June-2010: PHE - Created.
 *
 ************************************************************/
fbe_status_t fbe_base_env_get_file_size(fbe_base_environment_t * pBaseEnv, 
                                        char *pFileName, 
                                        fbe_u32_t *pFileSize)
{
    fbe_file_handle_t   fileHandle = FBE_FILE_INVALID_HANDLE;
    fbe_u32_t           status = FBE_STATUS_OK;

    // Open the file 
    fileHandle = fbe_file_open(pFileName, FBE_FILE_RDONLY, 0/* protection mode, unused */, NULL);

    if(fileHandle == FBE_FILE_INVALID_HANDLE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, open file %s failed.\n", 
                              __FUNCTION__, 
                              pFileName); 
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *pFileSize = (fbe_u32_t)fbe_file_get_size(fileHandle);

        // Close the file 
        if(fbe_file_close(fileHandle)== FBE_FILE_ERROR)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, close file %s failed.\n", 
                                  __FUNCTION__, 
                                  pFileName);

            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
} // fbe_base_env_get_file_size

/*!***********************************************************
 *  @fn  fbe_base_env_read_persistent_data(fbe_base_environment_t *base_environment) 
 ************************************************************
 * @berief
 *   This function initiates the process of reading the persistent data
 *   for this object.
 *
 * @param   base_environment  - The pointer to fbe_base_environment_t
 *
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   31-Dec-2010: bphilbin - Created.
 *
 ************************************************************/
fbe_status_t 
fbe_base_env_read_persistent_data(fbe_base_environment_t *base_environment) 
{
    fbe_status_t status;
    fbe_base_environment_cmi_message_t message;

    
    if(fbe_base_env_is_active_sp(base_environment))
    {
    
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                        (fbe_base_object_t*)base_environment,
                                        FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed setting LIFECYCLE_COND_READ_PERSISTENT_DATA status:0x%x\n",
                                __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment,
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s setting LIFECYCLE_COND_READ_PERSISTENT_DATA status:0x%x.\n",
                                      __FUNCTION__, status);
        }
    }
    else
    {
        base_environment->wait_for_peer_data = PERSISTENT_DATA_WAIT_READ;
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                        (fbe_base_object_t*)base_environment,
                                        FBE_BASE_ENV_LIFECYCLE_COND_WAIT_PEER_PERSISTENT_DATA);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed setting LIFECYCLE_COND_WAIT_PEER_PERSISTENT_DATA status:0x%x\n",
                                __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment,
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s setting LIFECYCLE_COND_WAIT_PEER_PERSISTENT_DATA status:0x%x.\n",
                                      __FUNCTION__, status);
        }
        
        //request data from the peer SP
        message.opcode = FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST;
        status = fbe_base_environment_send_cmi_message(base_environment,
                                                       sizeof(fbe_base_environment_cmi_message_t), 
                                                       &message);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed sending cmi message with status 0x%x\n",
                                __FUNCTION__, status);
        }


    }
    return status;
}

/*!***********************************************************
 *  @fn  fbe_base_env_is_active_sp(void) 
 ************************************************************
 * @berief
 *   This function checks whether or not this SP is the active SP.
 *   If handshaking is still going on, it will delay and re-check.
 *
 * @param   base_environment  - The pointer to fbe_base_environment_t
 *
 * @return fbe_bool_t - Is the active SP.
 *
 * @version
 *   31-Dec-2010: bphilbin - Created.
 *
 ************************************************************/
fbe_bool_t fbe_base_env_is_active_sp(fbe_base_environment_t *base_environment)
{
    fbe_bool_t                  is_active = FBE_TRUE;
    fbe_cmi_service_get_info_t  get_cmi_info;
    fbe_status_t                status;
    fbe_lifecycle_state_t       lifecycle_state;

    get_cmi_info.sp_state = FBE_CMI_STATE_INVALID;

    if(base_environment->isBootFlash)
    {
        // no cmi in service mode, act as though this is the active SP
        return FBE_TRUE;
    }

    status = fbe_api_cmi_service_get_info(&get_cmi_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) base_environment,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Failed to get CMI Info.\n",
                                  __FUNCTION__);
    }
    while( (get_cmi_info.sp_state != FBE_CMI_STATE_ACTIVE && 
            get_cmi_info.sp_state != FBE_CMI_STATE_PASSIVE && 
            get_cmi_info.sp_state != FBE_CMI_STATE_SERVICE_MODE) ||
            status != FBE_STATUS_OK)
    {
        fbe_thread_delay(500);
        status = fbe_api_cmi_service_get_info(&get_cmi_info);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *) base_environment,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Failed to get CMI Info.\n",
                                  __FUNCTION__);
        }

        /* get the lifecycly state */
        status = fbe_lifecycle_get_state(&fbe_base_environment_lifecycle_const, 
                                        (fbe_base_object_t *)base_environment, &lifecycle_state);
        if(status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
            is_active = FBE_FALSE;
            break;
        }

        /* let's break out if the object needs to be destroyed */
        if ((lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY)||
            (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY))
        {
            is_active = FBE_FALSE;
            break;
        }
    }
    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s SP_STATE:%d\n",
                                __FUNCTION__, get_cmi_info.sp_state);
    if (get_cmi_info.sp_state != FBE_CMI_STATE_ACTIVE) {
        is_active = FBE_FALSE;
    }

    return is_active;
}


/*!***********************************************************
 *  @fn  fbe_base_env_is_active_sp_esp_cmi(void) 
 ************************************************************
 * @berief
 *   This function checks whether or not this SP is the active SP.
 *   If handshaking is still going on, it will delay and re-check.
 *
 * @param   base_environment  - The pointer to fbe_base_environment_t
 *
 * @return fbe_bool_t - Is the active SP.
 *
 * @version
 *   31-Dec-2010: bphilbin - Created.
 *
 ************************************************************/
fbe_bool_t fbe_base_env_is_active_sp_esp_cmi(fbe_base_environment_t *base_environment)
{
    fbe_cmi_sp_state_t sp_state;
    fbe_bool_t         is_active = FBE_TRUE;

    fbe_cmi_get_current_sp_state(&sp_state);
    while(sp_state != FBE_CMI_STATE_ACTIVE && sp_state != FBE_CMI_STATE_PASSIVE && sp_state != FBE_CMI_STATE_SERVICE_MODE)
    {
        fbe_thread_delay(100);
        fbe_cmi_get_current_sp_state(&sp_state);
    }
    if (sp_state != FBE_CMI_STATE_ACTIVE) {
        is_active = FBE_FALSE;
    }

    return is_active;
}



/*!***********************************************************
 *  @fn  fbe_base_env_write_persistent_data(fbe_base_environment_t *base_environment) 
 ************************************************************
 * @berief
 *   This function initiates the process of writing the persistent data
 *   for this object.
 *
 * @param   base_environment  - The pointer to fbe_base_environment_t
 *
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   31-Dec-2010: bphilbin - Created.
 *
 ************************************************************/
fbe_status_t 
fbe_base_env_write_persistent_data(fbe_base_environment_t *base_environment) 
{
    fbe_status_t status;
    fbe_base_environment_persistent_data_cmi_message_t *cmi_msg = NULL;

    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s function enter\n",
                                    __FUNCTION__);

    if(fbe_base_env_is_active_sp(base_environment))
    {
        // this is the active sp go ahead and set the condition to do the write
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                        (fbe_base_object_t*)base_environment,
                                        FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA);
    }
    else
    {
        /* Send a cmi request to the peer to issue the write request. */
        base_environment->wait_for_peer_data = PERSISTENT_DATA_WAIT_WRITE;
        cmi_msg = fbe_base_env_memory_ex_allocate(base_environment, FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment));
        if(cmi_msg == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to allocate memory for cmi message.\n",
                                    __FUNCTION__);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        fbe_zero_memory(cmi_msg, FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment));
        cmi_msg->msg.opcode = FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_REQUEST;

        /*
         * Use the persistent_entry_id we got when we read the data, or a default we set if there was
         * no data found previously.
         */
        cmi_msg->persistent_data.persist_header.entry_id = base_environment->my_persistent_entry_id;
        cmi_msg->persistent_data.header.persistent_id = base_environment->my_persistent_id;
    
        fbe_copy_memory(&cmi_msg->persistent_data.header.data, 
                         base_environment->my_persistent_data, 
                         base_environment->my_persistent_data_size);

        status = fbe_base_environment_send_cmi_message(base_environment,
                                                       FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment), 
                                                       cmi_msg);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed sending cmi message with status 0x%x\n",
                                __FUNCTION__, status);
        }
        fbe_base_env_memory_ex_release(base_environment, cmi_msg);
    }
    
    return status;
}

/*!*************************************************************************
 *  @fn fbe_base_env_event_destroy_queue()
 **************************************************************************
 *  @brief
 *      This function releases all the event elments in the event queue
 *      and then destroys the event queue and spinlock. 
 *
 *  @param pBaseEnv - The pointer to the fbe_base_environment_t object.
 *
 *  @return  fbe_status_t - always return FBE_STATUS_OK 
 * 
 *  @version
 *  29-Nov-2012 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_base_env_event_destroy_queue(fbe_base_environment_t * pBaseEnv)
{
    fbe_queue_head_t                  * pEventQueueHead = NULL;  
    fbe_spinlock_t                    * pEventQueueLock = NULL;
    fbe_queue_element_t               * pEventElement = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pBaseEnv, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s entry.\n",
                                    __FUNCTION__);

    pEventQueueHead = &pBaseEnv->event_element.event_queue;
    pEventQueueLock = &pBaseEnv->event_element.event_queue_lock;

    fbe_spinlock_lock(pEventQueueLock);
    while(!fbe_queue_is_empty(pEventQueueHead))
    {
        pEventElement = (fbe_queue_element_t *)fbe_queue_pop(pEventQueueHead);
        fbe_base_env_memory_ex_release(pBaseEnv, pEventElement);
    }

    fbe_spinlock_unlock(pEventQueueLock);

    fbe_queue_destroy(pEventQueueHead);
    fbe_spinlock_destroy(pEventQueueLock);

    return FBE_STATUS_OK;
}

/*!***********************************************************
 *  @fn  fbe_base_env_cmi_destroy_queue(fbe_base_environment_t *base_environment) 
 ************************************************************
 * @berief
 *   This function removes all entries from the CMI message queue
 *   and frees the associated memory allocated for each message.
 *
 * @param   base_environment  - The pointer to fbe_base_environment_t
 *
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   28-Jan-2011: bphilbin - Created.
 *
 ************************************************************/
void fbe_base_env_cmi_destroy_queue(fbe_base_environment_t *base_environment)
{
    fbe_base_environment_cmi_message_info_t *cmi_message = NULL;

    fbe_base_object_trace((fbe_base_object_t*)base_environment, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s entry.\n",
                                    __FUNCTION__);

    /*
     * We should also be freeing up the user buffers here but not everybody is
     * allocating them the same way so that will have to be done later.
     */

    fbe_spinlock_lock(&base_environment->cmi_element.cmi_queue_lock);
    while(!fbe_queue_is_empty(&base_environment->cmi_element.cmi_callback_queue)){
        cmi_message = (fbe_base_environment_cmi_message_info_t *) fbe_queue_pop(&base_environment->cmi_element.cmi_callback_queue);
        fbe_spinlock_unlock(&base_environment->cmi_element.cmi_queue_lock);

        switch(cmi_message->event)
        {
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            /* In fbe_base_environment_cmi_notification_call_back, 
             * we allocate the memory to save the user message. 
             * Because the CMI service will release the memory for the user message
             * after the call fbe_base_environment_cmi_notification_call_back completes. 
             * Therefore, here we processed the CMI message, we need to release the memory.
             */
            if(cmi_message->user_message != NULL) 
            {
                fbe_base_env_memory_ex_release(base_environment, cmi_message->user_message);
            }
        
            break;

        default:
            // For all other cases we need to free the user message
            if(cmi_message->user_message != NULL) 
            {
                fbe_transport_release_buffer(cmi_message->user_message);
            }
            break;
        }

        fbe_base_object_trace((fbe_base_object_t *)base_environment, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s freeing cmi_message:%p\n",
                              __FUNCTION__, cmi_message);

        fbe_base_env_memory_ex_release(base_environment, cmi_message);
        fbe_spinlock_lock(&base_environment->cmi_element.cmi_queue_lock);
    }

    fbe_spinlock_unlock(&base_environment->cmi_element.cmi_queue_lock);

    fbe_queue_destroy(&base_environment->cmi_element.cmi_callback_queue);
    fbe_spinlock_destroy(&base_environment->cmi_element.cmi_queue_lock);

    return;

}
/*********************************************************
 * end fbe_base_env_cmi_destroy_queue()
 ********************************************************/

/*!***********************************************************
 *  @fn  fbe_base_env_reboot_sp(fbe_base_environment_t *base_env, SP_ID sp, fbe_bool_t hold_in_post, fbe_bool_t hold_in_reset) 
 ************************************************************
 * @berief
 *   This function issues a request to reboot the specified SP.
 *   This function will return immediately for the peer SP and handle
 *   the retries asynchronously.  For the Local SP the request is
 *   synchronous and will return if all retries have exhausted, or else
 *   there is no need to return because the SP is already rebooting.
 *
 * @param   base_env  - The pointer to fbe_base_environment_t
 * @param   sp - sp to reboot
 * @param   hold_in_post - T/F hold SP in POST
 * @param   hold_in_reset - T/F hold SP in Reset
 * @param   is_sync_cmd   - Sync Command or not
 *
 * @return fbe_status_t
 *             FBE_STATUS_OK - no error.
 *             otherwise - error encountered. 
 *
 * @version
 *   23-Feb-2011: bphilbin - Created.
 *   20-Nov-2012: AT - Modified to use sync command parameter
 *
 ************************************************************/
fbe_status_t fbe_base_env_reboot_sp(fbe_base_environment_t *base_env, 
                                    SP_ID sp, 
                                    fbe_bool_t hold_in_post, 
                                    fbe_bool_t hold_in_reset,
                                    fbe_bool_t is_sync_cmd)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context = NULL;
    fbe_object_id_t board_object_id;

#if ESP_BUGCHECK_ON_REBOOT
    fbe_base_object_trace((fbe_base_object_t *)base_env, 
                                FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s stopping SP\n",
                                __FUNCTION__);
#endif

    status = fbe_api_get_board_object_id(&board_object_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)base_env, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error in getting the Board Object ID, status: 0x%X\n",
                                __FUNCTION__, status);
        return status;
    }

    command_context = fbe_base_env_memory_ex_allocate(base_env, sizeof(fbe_board_mgmt_set_PostAndOrReset_async_context_t));
    if(command_context == NULL)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        command_context->command.sp_id = sp;
        command_context->command.holdInPost = hold_in_post;
        command_context->command.holdInReset = hold_in_reset;
        command_context->command.rebootBlade = TRUE;
        command_context->command.retryStatusCount = POST_AND_OR_RESET_STATUS_RETRY_COUNT;
        command_context->status = FBE_STATUS_GENERIC_FAILURE;
        command_context->completion_function = is_sync_cmd ? fbe_base_env_reboot_sp_sync_completion : fbe_base_env_reboot_sp_async_completion;
        command_context->object_context = base_env;
        
    }

    if(is_sync_cmd)
    {
        fbe_semaphore_init(&base_env->sem, 0, 1);
        /*
         * If this is the local SP we are requesting to reboot, hold the object
         * here and wait synchronously for the completion of the reboot request.
         */

        do
        {
            status = fbe_api_board_set_PostAndOrReset_async(board_object_id, command_context);
            if(status == FBE_STATUS_PENDING)
            {
                fbe_semaphore_wait(&base_env->sem, NULL);
                if(command_context->status == FBE_STATUS_OK)
                {
                    status = FBE_STATUS_OK;
                    break;
                }
            }
            command_context->retry_count++;
        } while(command_context->retry_count < FBE_ESP_MAX_REBOOT_RETRIES);

        fbe_semaphore_destroy(&base_env->sem); /* SAFEBUG - needs destroy */
        fbe_base_env_memory_ex_release(base_env, command_context);
    }
    else
    {
        status = fbe_api_board_set_PostAndOrReset_async(board_object_id, command_context);
    }
    return status;
}

/*********************************************************
 * end fbe_base_env_reboot_sp()
 ********************************************************/


/*!***********************************************************
 *  @fn  fbe_base_env_reboot_sp_async_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context) 
 ************************************************************
 * @berief
 *   This function handles the completion of a reboot request.
 *   It releases a semaphore for local sp requests and allows
 *   the caller to handle retries.  For peer SP requests it
 *   handles the retries itself.
 *
 * @param   command_context - Structure containing the context for the reboot command
 *
 * @return none
 *
 * @version
 *   23-Feb-2011: bphilbin - Created.
 *
 ************************************************************/
void fbe_base_env_reboot_sp_async_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context)
{
    fbe_base_environment_t *base_env = (fbe_base_environment_t *)command_context->object_context;
    fbe_status_t status;
    
    if(command_context->retry_count < FBE_ESP_MAX_REBOOT_RETRIES)
    {
        // for peer SP we handle retries asynchronously
        fbe_object_id_t board_object_id;

        command_context->retry_count++;
        status = fbe_api_get_board_object_id(&board_object_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)base_env, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Error in getting the Board Object ID, status: 0x%X\n",
                                    __FUNCTION__, status);
            fbe_base_env_memory_ex_release(base_env, command_context);
            return;
        }

        status = fbe_api_board_set_PostAndOrReset_async(board_object_id, command_context);
        if(status != FBE_STATUS_PENDING)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_env, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Error issuing sp reboot status:0x%X\n",
                                    __FUNCTION__, status);
        }
    }
    else if(command_context->retry_count >= FBE_ESP_MAX_REBOOT_RETRIES)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_env, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Error retries exhausted.\n",
                                __FUNCTION__);
        fbe_base_env_memory_ex_release(base_env, command_context);

    }
    return;
}

/*!***********************************************************
 *  @fn  fbe_base_env_reboot_sp_sync_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context) 
 ************************************************************
 * @berief
 *   This function handles the completion of a reboot request.
 *   It releases a semaphore for local sp requests and allows
 *   the caller to handle retries.  For peer SP requests it
 *   handles the retries itself.
 *
 * @param   command_context - Structure containing the context for the reboot command
 *
 * @return none
 *
 * @version
 *   23-Feb-2011: bphilbin - Created.
 *
 ************************************************************/
void fbe_base_env_reboot_sp_sync_completion(fbe_board_mgmt_set_PostAndOrReset_async_context_t *command_context)
{
    fbe_base_environment_t *base_env = (fbe_base_environment_t *)command_context->object_context;
    
    // local sp is waiting synchronously to handle retries, kick that off
    fbe_semaphore_release(&base_env->sem, 0, 1 ,FALSE);
}

 /*!*************************************************************************
 *  fbe_base_environment_send_data_change_notification()
 ***************************************************************************
 * @brief
 *  This functions sends the ESP object data change notification for leaf class.
 *
 * @param  pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param  classId - Class ID of leaf class
 * @param  device_mask - Device mask for which sending notification 
 * @param  location - Physical location of device.
 *
 * @return - void
 * 
 * @author
 *  25-Feb-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void 
fbe_base_environment_send_data_change_notification(fbe_base_environment_t *pBaseEnv,
                                                   fbe_class_id_t classId,
                                                   fbe_u64_t device_type,
                                                   fbe_device_data_type_t data_type,
                                                   fbe_device_physical_location_t *location)
{
    fbe_object_id_t objectId;
    fbe_notification_info_t notification;

    fbe_base_object_get_object_id((fbe_base_object_t *)pBaseEnv, &objectId);

    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
    notification.class_id = classId;
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
    notification.notification_data.data_change_info.data_type = data_type;
    notification.notification_data.data_change_info.device_mask= device_type;
    notification.notification_data.data_change_info.phys_location = *location;

    fbe_notification_send(objectId, notification);
    return;
}
/*********************************************************
 * end fbe_base_environment_send_data_change_notification()
 ********************************************************/

/*!*************************************************************************
 * @fn fbe_base_env_decode_cmi_msg_opcode(fbe_base_environment_cmi_message_opcode_t opcode)
 **************************************************************************
 *  @brief
 *  This function decodes fbe_base_environment_cmi_message_opcode_t.
 *
 *  @param    fbe_base_environment_cmi_message_opcode_t
 *
 *  @return     char * - string
 *
 *  @author
 *    27-Apr-2011: PHE- created.
 **************************************************************************/
char * fbe_base_env_decode_cmi_msg_opcode(fbe_base_environment_cmi_message_opcode_t opcode)
{
    char * str;

    switch(opcode)
    {
    case FBE_BASE_ENV_CMI_MSG_INVALID :
        str = "CMI_MSG_INVALID";
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_REQUEST:
        str = "CMI_MSG_PERSISTENT_WRITE_REQUEST";
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_COMPLETE:
        str = "CMI_MSG_PERSISTENT_WRITE_COMPLETE";
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST:
        str = "CMI_MSG_PERSISTENT_READ_REQUEST";
        break;

    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_COMPLETE:
        str = "CMI_MSG_PERSISTENT_READ_COMPLETE";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST :
        str = "CMI_MSG_FUP_PEER_PERMISSION_REQUEST";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        str = "CMI_MSG_FUP_PEER_PERMISSION_GRANT";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
        str = "CMI_MSG_FUP_PEER_PERMISSION_DENY";
        break;

    case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_REQUEST:
        str = "CMI_MSG_ENCL_CABLING_REQUEST";
        break;

    case FBE_BASE_ENV_CMI_MSG_REQ_SPS_INFO:
        str = "CMI_MSG_REQ_SPS_INFO";
        break;

    case FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO:
        str = "CMI_MSG_SEND_SPS_INFO";
        break;

    case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
        str = "CMI_MSG_CACHE_STATUS_UPDATE" ;
        break;

    case FBE_BASE_ENV_CMI_MSG_REQUEST_PEER_REVISION:
        str = "CMI_MSG_REQUEST_PEER_REVISION" ;
        break;

    case FBE_BASE_ENV_CMI_MSG_PEER_REVISION_RESPONSE:
        str = "CMI_MSG_PEER_REVISION_RESPONSE ";
        break;

    case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
        str = "CMI_MSG_PEER_SP_ALIVE ";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_ACTIVATE_START:
        str = "CMI_MSG_FUP_PEER_ACTIVATE_START";
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_UPGRADE_END:
        str = "CMI_MSG_FUP_PEER_UPGRADE_END";
        break;

    default:
        // not supported
        str = "CMI_MSG_UNKNOWN";
        break;
    }
    return(str);
} // fbe_base_env_decode_cmi_msg_opcode

//esp allocate memory function wrapper
void *  _fbe_base_env_memory_ex_allocate(fbe_base_environment_t * pBaseEnv, fbe_u32_t allocation_size_in_bytes, const char* callerFunctionNameString, fbe_u32_t callerLineNumber)
{
    void * pmemory = NULL;

    fbe_spinlock_lock(&pBaseEnv->base_env_lock);
    pBaseEnv->memory_ex_allocate_count++;
    fbe_spinlock_unlock(&pBaseEnv->base_env_lock);

    /* memory allocated and zeroed */
    pmemory = fbe_memory_ex_allocate(allocation_size_in_bytes);

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "MEM:allocate %p,%s Ln%d\n",
                                    pmemory, callerFunctionNameString, callerLineNumber);

    return pmemory;
}

//esp release memory function wrapper
void  _fbe_base_env_memory_ex_release(fbe_base_environment_t * pBaseEnv, void * ptr, const char* callerFunctionNameString, fbe_u32_t callerLineNumber)
{
    fbe_spinlock_lock(&pBaseEnv->base_env_lock);
    pBaseEnv->memory_ex_allocate_count--;
    fbe_spinlock_unlock(&pBaseEnv->base_env_lock);

    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "MEM:release %p,%s Ln%d\n",
                                    ptr, callerFunctionNameString, callerLineNumber);

    fbe_memory_ex_release(ptr);
    return;
}

fbe_status_t fbe_base_environment_get_peerCacheStatus(fbe_base_environment_t *base_environment,
                                                     fbe_common_cache_status_t *peerCacheStatus)
{
    *peerCacheStatus = base_environment->peerCacheStatus;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_environment_set_peerCacheStatus(fbe_base_environment_t *base_environment,
                                                     fbe_common_cache_status_t peerCacheStatus)
{
    base_environment->peerCacheStatus = peerCacheStatus;
    return FBE_STATUS_OK;
}

fbe_persistent_id_t fbe_base_environment_convert_class_id_to_persistent_id(fbe_base_environment_t *base_environment, fbe_class_id_t class_id)
{
    fbe_persistent_id_t persistent_id = 0;
    switch (class_id) 
    {
    case FBE_CLASS_ID_MODULE_MGMT:
        persistent_id = FBE_PERSISTENT_ID_MODULE_MGMT;
        break;

    case FBE_CLASS_ID_ENCL_MGMT:
        persistent_id = FBE_PERSISTENT_ID_ENCL_MGMT;
        break;

    case FBE_CLASS_ID_PS_MGMT:
        persistent_id = FBE_PERSISTENT_ID_PS_MGMT;
        break;

    case FBE_CLASS_ID_SPS_MGMT:
        persistent_id = FBE_PERSISTENT_ID_SPS_MGMT;
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *) base_environment,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s class_id %d unhandled\n",
                            __FUNCTION__, class_id);
        break;
    }
    return persistent_id;
}
