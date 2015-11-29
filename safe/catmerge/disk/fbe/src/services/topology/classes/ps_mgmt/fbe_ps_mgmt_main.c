/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the PS
 *  Management object. This includes the create and
 *  destroy methods for the PS management object, as well
 *  as the event entry point, load and unload methods.
 * 
 * @ingroup ps_mgmt_class_files
 * 
 * @version
 *   15-Apr-2010:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_ps_mgmt_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_ps_mgmt_load(void);
fbe_status_t fbe_ps_mgmt_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_ps_mgmt_class_methods = { FBE_CLASS_ID_PS_MGMT,
                                                  fbe_ps_mgmt_load,
                                                  fbe_ps_mgmt_unload,
                                                  fbe_ps_mgmt_create_object,
                                                  fbe_ps_mgmt_destroy_object,
                                                  fbe_ps_mgmt_control_entry,
                                                  fbe_ps_mgmt_event_entry,
                                                  NULL,
                                                  fbe_ps_mgmt_monitor_entry
                                                };
/*!***************************************************************
 * fbe_ps_mgmt_load()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_PS_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_ps_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);
    return fbe_ps_mgmt_monitor_load_verify();
}
/******************************************
 * end fbe_ps_mgmt_load()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_unload()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_PS_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ps_mgmt_unload()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_create_object()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_ps_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_ps_mgmt_t * ps_mgmt;

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    ps_mgmt = (fbe_ps_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) ps_mgmt, FBE_CLASS_ID_PS_MGMT);
    fbe_ps_mgmt_init(ps_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ps_mgmt_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_destroy_object()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_ps_mgmt_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_ps_mgmt_t * ps_mgmt;

    ps_mgmt = (fbe_ps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)ps_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* TODO - Deregister FBE API notifications, free up
     * any memory that was allocted here 
     */

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)ps_mgmt, ps_mgmt->psInfo);

    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_ps_mgmt_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_event_entry()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                      fbe_event_type_t event_type, 
                                      fbe_event_context_t event_context)
{
    fbe_ps_mgmt_t * ps_mgmt;

    ps_mgmt = (fbe_ps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)ps_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ps_mgmt_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_init()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_init(fbe_ps_mgmt_t * ps_mgmt)
{
    fbe_u16_t       i;
    fbe_encl_ps_info_t * pEnclPsInfo = NULL;
    fbe_u32_t persistent_storage_data_size = 0;
    fbe_status_t status;
        
    fbe_topology_class_trace(FBE_CLASS_ID_PS_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    ps_mgmt->psInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)ps_mgmt, sizeof(fbe_encl_ps_info_array_t));
    fbe_zero_memory(ps_mgmt->psInfo, sizeof(fbe_encl_ps_info_array_t));

    for (i = 0; i < FBE_ESP_MAX_ENCL_COUNT; i++)
    {
        pEnclPsInfo = &ps_mgmt->psInfo->enclPsInfo[i];
        fbe_ps_mgmt_init_encl_ps_info(pEnclPsInfo);
    }
  
    ps_mgmt->enclCount = 1;     // always have PE

    ps_mgmt->psCacheStatus = FBE_CACHE_STATUS_FAILED;

    persistent_storage_data_size = (FBE_PS_MGMT_PERSISTENT_DATA_STORE_SIZE);

    FBE_ASSERT_AT_COMPILE_TIME(FBE_PS_MGMT_PERSISTENT_DATA_STORE_SIZE < FBE_PERSIST_DATA_BYTES_PER_ENTRY);
    fbe_base_environment_set_persistent_storage_data_size((fbe_base_environment_t *) ps_mgmt, persistent_storage_data_size);

    /* Set the condition to initialize persistent storage and read the data from disk. */
    status = fbe_lifecycle_set_cond(&fbe_ps_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)ps_mgmt,
                                        FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) ps_mgmt,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in setting INIT_PERSIST_STOR_COND, status: 0x%x.\n",
                              __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ps_mgmt_init()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_init_encl_ps_info()
 ****************************************************************
 * @brief
 *  This function is called to initialize the enclosure ps info
 *  structure
 *
 * @param  pEnclPsInfo - Pointer to the enclosure ps info structure.
 *
 * @return fbe_status_t
 *
 * @author
 *  18-Aug-2010 PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_init_encl_ps_info(fbe_encl_ps_info_t * pEnclPsInfo)
{
    fbe_u32_t                 index = 0;
    fbe_power_supply_info_t * pPsInfo = NULL;

    pEnclPsInfo->inUse = FALSE;
    pEnclPsInfo->objectId = FBE_OBJECT_ID_INVALID;
    pEnclPsInfo->location.bus = FBE_INVALID_PORT_NUM;
    pEnclPsInfo->location.enclosure = FBE_INVALID_ENCL_NUM;
    pEnclPsInfo->location.slot = FBE_INVALID_SLOT_NUM;
    pEnclPsInfo->psCount = 0;
    fbe_zero_memory(&pEnclPsInfo->psInfo[0], FBE_ESP_PS_MAX_COUNT_PER_ENCL * sizeof(fbe_power_supply_info_t));
    fbe_zero_memory(&pEnclPsInfo->psFupInfo[0], FBE_ESP_PS_MAX_COUNT_PER_ENCL * sizeof(fbe_base_env_fup_persistent_info_t));
    fbe_zero_memory(&pEnclPsInfo->psResumeInfo[0], FBE_ESP_PS_MAX_COUNT_PER_ENCL * sizeof(fbe_base_env_resume_prom_info_t));

    for(index = 0; index <FBE_ESP_PS_MAX_COUNT_PER_ENCL; index++)
    {
        pPsInfo = &pEnclPsInfo->psInfo[index];
        /* 
         * Initialize to TRUE so that we can log the event
         * if the power supply is not inserted at boot up.
         */
        pPsInfo->bInserted = TRUE;
        
        /* Init resume prom info */
        fbe_base_env_init_resume_prom_info(&pEnclPsInfo->psResumeInfo[index]);    

        // init PS debounce info
        pEnclPsInfo->psDebounceInfo[index].debounceTimerActive = FBE_FALSE;
        pEnclPsInfo->psDebounceInfo[index].debouncePeriodExpired = FBE_FALSE;
        pEnclPsInfo->psDebounceInfo[index].faultMasked = FBE_FALSE;
        //pEnclPsInfo->psDebounceInfo[index].generalFault = FBE_MGMT_STATUS_NA;
    }
    return FBE_STATUS_OK;
}

