/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cooling_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the COOLING
 *  Management object. This includes the create and
 *  destroy methods for the COOLING management object, as well
 *  as the event entry point, load and unload methods.
 * 
 * @ingroup cooling_mgmt_class_files
 * 
 * @version
 *   22-Jan-2011:  PHE - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_cooling_mgmt_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_cooling_mgmt_load(void);
fbe_status_t fbe_cooling_mgmt_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_cooling_mgmt_class_methods = { FBE_CLASS_ID_COOLING_MGMT,
                                                  fbe_cooling_mgmt_load,
                                                  fbe_cooling_mgmt_unload,
                                                  fbe_cooling_mgmt_create_object,
                                                  fbe_cooling_mgmt_destroy_object,
                                                  fbe_cooling_mgmt_control_entry,
                                                  fbe_cooling_mgmt_event_entry,
                                                  NULL,
                                                  fbe_cooling_mgmt_monitor_entry
                                                };
/*!***************************************************************
 * fbe_cooling_mgmt_load()
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
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_COOLING_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_cooling_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);
    return fbe_cooling_mgmt_monitor_load_verify();
}
/******************************************
 * end fbe_cooling_mgmt_load()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_unload()
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
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_COOLING_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cooling_mgmt_unload()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_create_object()
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
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_cooling_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_cooling_mgmt_t * cooling_mgmt;

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    cooling_mgmt = (fbe_cooling_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) cooling_mgmt, FBE_CLASS_ID_COOLING_MGMT);
    fbe_cooling_mgmt_init(cooling_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cooling_mgmt_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_destroy_object()
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
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_cooling_mgmt_destroy_object( fbe_object_handle_t object_handle)
{ 
    fbe_cooling_mgmt_t * cooling_mgmt;
    fbe_status_t         status;

    cooling_mgmt = (fbe_cooling_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)cooling_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* TODO - Deregister FBE API notifications, free up
     * any memory that was allocted here 
     */
    
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)cooling_mgmt, cooling_mgmt->pFanInfo);

    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_cooling_mgmt_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_event_entry()
 ****************************************************************
 * @brief
 *  This function is the event entry.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                      fbe_event_type_t event_type, 
                                      fbe_event_context_t event_context)
{
    fbe_cooling_mgmt_t * cooling_mgmt;

    cooling_mgmt = (fbe_cooling_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)cooling_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cooling_mgmt_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_init()
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
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_init(fbe_cooling_mgmt_t * pCoolingMgmt)
{
    fbe_u32_t                     fanIndex = 0;
        
    fbe_topology_class_trace(FBE_CLASS_ID_COOLING_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    
    pCoolingMgmt->pFanInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)pCoolingMgmt, sizeof(fbe_cooling_mgmt_fan_info_t) * FBE_COOLING_MGMT_MAX_FAN_COUNT);

    pCoolingMgmt->fanCount = 0;
    for(fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++) 
    {
        fbe_cooling_mgmt_init_fan_info(&pCoolingMgmt->pFanInfo[fanIndex]);
    }

    pCoolingMgmt->dpeFanSourceObjectType = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    pCoolingMgmt->dpeFanSourceOffset = 0;

    pCoolingMgmt->coolingCacheStatus = FBE_CACHE_STATUS_FAILED;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cooling_mgmt_init()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_init_fan_info()
 ****************************************************************
 * @brief
 *  This function is called to initialize the FAN info
 *  structure
 *
 * @param  pCoolingInfo - Pointer to fbe_cooling_mgmt_fan_info_t.
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_init_fan_info(fbe_cooling_mgmt_fan_info_t * pCoolingInfo)
{
    fbe_zero_memory(pCoolingInfo, sizeof(fbe_cooling_mgmt_fan_info_t));
    pCoolingInfo->objectId = FBE_OBJECT_ID_INVALID;
    pCoolingInfo->location.bus = FBE_INVALID_PORT_NUM;
    pCoolingInfo->location.enclosure = FBE_INVALID_ENCL_NUM;
    pCoolingInfo->location.slot = FBE_INVALID_SLOT_NUM;

    fbe_base_env_init_resume_prom_info(&pCoolingInfo->fanResumeInfo);

    return FBE_STATUS_OK;
}

