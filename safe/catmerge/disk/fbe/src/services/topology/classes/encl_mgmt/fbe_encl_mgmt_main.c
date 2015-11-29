/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_encl_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the enclosure
 *  Management object. This includes the create and
 *  destroy methods for the enclosure management object, as well
 *  as the event entry point, load and unload methods.
 * 
 * @ingroup encl_mgmt_class_files
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
#include "fbe_topology.h"
#include "fbe_encl_mgmt_private.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_registry.h"

#define IgnoreUnsupportedEnclosuresKey      "IgnoreUnsupportedEnclosures"

/* Class methods forward declaration */
fbe_status_t fbe_encl_mgmt_load(void);
fbe_status_t fbe_encl_mgmt_unload(void);
static fbe_bool_t fbe_base_environment_ignoreUnsupportedEnclosures(fbe_base_object_t * base_object);

/* Export class methods  */
fbe_class_methods_t fbe_encl_mgmt_class_methods = { FBE_CLASS_ID_ENCL_MGMT,
                                                    fbe_encl_mgmt_load,
                                                    fbe_encl_mgmt_unload,
                                                    fbe_encl_mgmt_create_object,
                                                    fbe_encl_mgmt_destroy_object,
                                                    fbe_encl_mgmt_control_entry,
                                                    fbe_encl_mgmt_event_entry,
                                                    NULL,
                                                    fbe_encl_mgmt_monitor_entry
                                                    };

/*!***************************************************************
 * fbe_encl_mgmt_load()
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
fbe_status_t fbe_encl_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_ENCL_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_encl_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);
    return fbe_encl_mgmt_monitor_load_verify();
}
/******************************************
 * end fbe_encl_mgmt_load()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_unload()
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
fbe_status_t fbe_encl_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_ENCL_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_unload()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_create_object()
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
fbe_encl_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_encl_mgmt_t * encl_mgmt;

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    encl_mgmt = (fbe_encl_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) encl_mgmt, FBE_CLASS_ID_ENCL_MGMT);
    fbe_encl_mgmt_init(encl_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_destroy_object()
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
fbe_encl_mgmt_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_encl_mgmt_t * encl_mgmt;
    fbe_u32_t encl_count;

    encl_mgmt = (fbe_encl_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* TODO - Deregister FBE API notifications, free up
     * any memory that was allocted here 
     */
   
    for(encl_count= 0; encl_count < FBE_ESP_MAX_ENCL_COUNT; encl_count ++) {
        if (encl_mgmt->encl_info[encl_count]->eirSampleData != NULL)
        {
            fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, encl_mgmt->encl_info[encl_count]->eirSampleData);
            encl_mgmt->encl_info[encl_count]->eirSampleData = NULL;
        }
        fbe_spinlock_destroy(&encl_mgmt->encl_info[encl_count]->encl_info_lock); /* SAFEBUG - needs destroy */
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, encl_mgmt->encl_info[encl_count]);
    }

    fbe_spinlock_destroy(&encl_mgmt->encl_mgmt_lock); /* SAFEBUG - needs destroy */
    
    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_encl_mgmt_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_event_entry()
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
fbe_encl_mgmt_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_init()
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
fbe_encl_mgmt_init(fbe_encl_mgmt_t * encl_mgmt)
{
    fbe_u32_t encl_count;
    fbe_encl_info_t *encl_info;
    fbe_status_t status;
    fbe_u32_t persistent_storage_data_size = 0;

    fbe_topology_class_trace(FBE_CLASS_ID_ENCL_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    for(encl_count = 0; encl_count < FBE_ESP_MAX_ENCL_COUNT; encl_count++) {
        encl_mgmt->encl_info[encl_count] = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt, sizeof(fbe_encl_info_t));
        encl_info = encl_mgmt->encl_info[encl_count];
        fbe_spinlock_init(&encl_info->encl_info_lock);
        fbe_encl_mgmt_init_encl_info(encl_mgmt, encl_info);
    }
    

    fbe_spinlock_init(&encl_mgmt->encl_mgmt_lock);

    /*
     * Initialize encl_mgmt CacheStatus
     * (we only check ShutdownReason from DAE0, so set to OK for DPE-based arrays)
     */
    if ((encl_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) ||
        (encl_mgmt->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE))
    {
        encl_mgmt->enclCacheStatus = FBE_CACHE_STATUS_FAILED;
    }
    else
    {
        encl_mgmt->enclCacheStatus = FBE_CACHE_STATUS_OK;
    }

    encl_mgmt->total_encl_count = 0;
    encl_mgmt->total_drive_slot_count = 0;
    encl_mgmt->eirSampleCount = 0;
    encl_mgmt->eirSampleIndex = 0;
    encl_mgmt->system_id_info_initialized = FBE_FALSE;

    encl_mgmt->platformFruLimit = fbe_environment_limit_get_platform_fru_count();
    //currently (for beachcomber), single SP system support half the disk number as dual SP system
    if (encl_mgmt->base_environment.isSingleSpSystem)
    {
        encl_mgmt->platformFruLimit = encl_mgmt->platformFruLimit/2;
    }
    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s setting Fru Limit to %d\n", __FUNCTION__, encl_mgmt->platformFruLimit);

    encl_mgmt->ignoreUnsupportedEnclosures = 
        fbe_base_environment_ignoreUnsupportedEnclosures((fbe_base_object_t *)encl_mgmt);
    fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s, ignoreUnsupportedEnclosures %d\n", 
                             __FUNCTION__, 
                             encl_mgmt->ignoreUnsupportedEnclosures);


    persistent_storage_data_size = sizeof(fbe_encl_mgmt_persistent_info_t);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_encl_mgmt_persistent_info_t)
                                      < FBE_PERSIST_DATA_BYTES_PER_ENTRY);

    fbe_base_environment_set_persistent_storage_data_size((fbe_base_environment_t *) encl_mgmt, persistent_storage_data_size);

    /* Set the condition to initialize persistent storage and read the data from disk. */
    status = fbe_lifecycle_set_cond(&fbe_encl_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)encl_mgmt,
                                        FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_init()
 ******************************************/
/*!***************************************************************
 * fbe_encl_mgmt_init_encl_info()
 ****************************************************************
 * @brief
 *  This function is called to initialize the enclosure info
 *  structure
 *
 * @param  encl_info - Pointer to the enclosure info structure.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_init_encl_info(fbe_encl_mgmt_t *encl_mgmt, fbe_encl_info_t *encl_info)
{
    fbe_u32_t index = 0;

    encl_info->object_id = FBE_OBJECT_ID_INVALID;
    encl_info->enclState = FBE_ESP_ENCL_STATE_MISSING;
    encl_info->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
    encl_info->enclType = FBE_ESP_ENCL_TYPE_INVALID;
    encl_info->location.bus = FBE_INVALID_PORT_NUM;
    encl_info->location.enclosure = FBE_INVALID_ENCL_NUM;
    encl_info->location.slot = FBE_INVALID_SLOT_NUM;
    fbe_zero_memory(&encl_info->serial_number[0], RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE);
    encl_info->lccCount = 0;
    encl_info->psCount = 0;
    encl_info->fanCount = 0;
    encl_info->driveSlotCount = 0;
    encl_info->connectorCount = 0;
    encl_info->subEnclCount = 0;
    encl_info->currSubEnclCount = 0;

    encl_info->enclShutdownDelayInProgress = FBE_FALSE;
    encl_info->shutdownReasonDebounceComplete = FBE_FALSE;
    encl_info->shutdownReasonDebounceInProgress = FBE_FALSE;

    // if memory already allocated, then free it up (will be allocated when needed)
    if (encl_info->eirSampleData != NULL)
    {
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, encl_info->eirSampleData);
        encl_info->eirSampleData = NULL;
    }
    fbe_zero_memory(&encl_info->lccInfo[0], FBE_ESP_MAX_LCCS_PER_ENCL * sizeof(fbe_lcc_info_t));
    fbe_zero_memory(&encl_info->lccFupInfo[0], FBE_ESP_MAX_LCCS_PER_ENCL * sizeof(fbe_base_env_fup_persistent_info_t));
    fbe_zero_memory(&encl_info->lccResumeInfo[0], FBE_ESP_MAX_LCCS_PER_ENCL * sizeof(fbe_base_env_resume_prom_info_t));
    fbe_zero_memory(&encl_info->connectorInfo[0], FBE_ESP_MAX_CONNECTORS_PER_ENCL * sizeof(fbe_connector_info_t));
    for(index = 0; index <FBE_ESP_MAX_LCCS_PER_ENCL; index++ ) 
    {
        fbe_base_env_init_resume_prom_info(&encl_info->lccResumeInfo[index]);   
    }

    for(index = 0; index <FBE_ESP_MAX_SUBENCLS_PER_ENCL; index++ ) 
    {
        fbe_encl_mgmt_init_subencl_info(&encl_info->subEnclInfo[index]);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_init_encl_info()
 ******************************************/ 

/*!***************************************************************
 * fbe_encl_mgmt_init_subencl_info()
 ****************************************************************
 * @brief
 *  This function is called to initialize the subenclosure info
 *  structure
 *
 * @param  fbe_sub_encl_info_t - Pointer to the subenclosure info structure.
 *
 * @return fbe_status_t
 *
 * @author
 *  03-Feb-2012: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_init_subencl_info(fbe_sub_encl_info_t * pSubEnclInfo)
{
    pSubEnclInfo->objectId = FBE_OBJECT_ID_INVALID;
    pSubEnclInfo->location.bus = 0xFF;
    pSubEnclInfo->location.enclosure = 0xFF;
    pSubEnclInfo->location.componentId = 0xFF;
    pSubEnclInfo->driveSlotCount = 0;
    fbe_zero_memory(&pSubEnclInfo->lccInfo[0], FBE_ESP_MAX_LCCS_PER_SUBENCL * sizeof(fbe_lcc_info_t));
    fbe_zero_memory(&pSubEnclInfo->lccFupInfo[0], FBE_ESP_MAX_LCCS_PER_SUBENCL * sizeof(fbe_base_env_fup_persistent_info_t));
    
    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_encl_mgmt_init_subencl_info()
 ******************************************/ 

/*!***************************************************************
 * fbe_encl_mgmt_init_all_encl_info()
 ****************************************************************
 * @brief
 *  This function is called to initialize all encl info.
 *  structure
 *
 * @param  fbe_sub_encl_info_t - Pointer to the subenclosure info structure.
 *
 * @return fbe_status_t
 *
 * @author
 *  03-Feb-2012: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_init_all_encl_info(fbe_encl_mgmt_t * encl_mgmt)
{
    fbe_u32_t         encl = 0;
    fbe_encl_info_t * encl_info = NULL;

    for(encl = 0; encl < FBE_ESP_MAX_ENCL_COUNT; encl ++) 
    {
        encl_info = encl_mgmt->encl_info[encl];
        fbe_encl_mgmt_init_encl_info(encl_mgmt, encl_info);
    }

    encl_mgmt->total_encl_count = 0;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_init_all_encl_info()
 ******************************************/ 


/*!**************************************************************
 * fbe_base_environment_ignoreUnsupportedEnclosures()
 ****************************************************************
 * @brief
 *  This function will determine if the IgnoreUnsupportedPs
 *  registry parameter exisist & will return the value.
 *
 * @param base_object - This is the base object for the ESP object.
 *
 * @return fbe_bool_t - TRUE if SP in Bootflash
 *
 * @author
 *  22-Oct-2015:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_bool_t
fbe_base_environment_ignoreUnsupportedEnclosures(fbe_base_object_t * base_object)
{
    fbe_u32_t ignoreUnsupportedEnclosures = FALSE;   
    fbe_u8_t defaulIgnoreUnsupportedEnclosures =0;
    fbe_status_t status;

    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               IgnoreUnsupportedEnclosuresKey, 
                               &ignoreUnsupportedEnclosures,
                               sizeof(ignoreUnsupportedEnclosures),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaulIgnoreUnsupportedEnclosures,
                               0,
                               FALSE);
    if(status != FBE_STATUS_OK)
    {
        // if cannot read Registry, assume BootFlash
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, error reading Registry ignoreUnsupportedEnclosures, status %d\n",
                              __FUNCTION__, status);
        return FALSE;
    }

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, ignoreUnsupportedPs %d\n",
                          __FUNCTION__, ignoreUnsupportedEnclosures);

    if (ignoreUnsupportedEnclosures == 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}   // end of fbe_base_environment_ignoreUnsupportedEnclosures
