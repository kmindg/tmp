/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_board_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the board
 *  Management object. This includes the create and
 *  destroy methods for the Board management object, as well
 *  as the event entry point, load and unload methods.
 *
 * @ingroup board_mgmt_class_files
 * 
 * @version
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_board_mgmt_private.h"

/* Class methods forward declaration */
fbe_status_t fbe_board_mgmt_load(void);
fbe_status_t fbe_board_mgmt_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_board_mgmt_class_methods = {FBE_CLASS_ID_BOARD_MGMT,
                                                    fbe_board_mgmt_load,
                                                    fbe_board_mgmt_unload,
                                                    fbe_board_mgmt_create_object,
                                                    fbe_board_mgmt_destroy_object,
                                                    fbe_board_mgmt_control_entry,
                                                    fbe_board_mgmt_event_entry,
                                                    NULL,
                                                    fbe_board_mgmt_monitor_entry
                                                    };

/*!***************************************************************
 * fbe_board_mgmt_load()
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
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BOARD_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_board_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);
    return fbe_board_mgmt_monitor_load_verify();
}
/**********************************
*   end of fbe_board_mgmt_load()
***********************************/
/*!***************************************************************
 * fbe_board_mgmt_unload()
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
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BOARD_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/***********************************
*   end of fbe_board_mgmt_unload()
************************************/
/*!***************************************************************
 * fbe_board_mgmt_create_object()
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
 *  29-July-2010 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t 
fbe_board_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status; 
    fbe_board_mgmt_t *board_mgmt;

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK) {
        return status;
    }
    
    /* Set class id*/
    board_mgmt = (fbe_board_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) board_mgmt, FBE_CLASS_ID_BOARD_MGMT);
    fbe_board_mgmt_init(board_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
*   end of fbe_board_mgmt_create_object()
*******************************************/
/*!***************************************************************
 * fbe_board_mgmt_destroy_object()
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
 *  29-July-2010 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t 
fbe_board_mgmt_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_board_mgmt_t  *board_mgmt;

    board_mgmt = (fbe_board_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)board_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Release the alloted resume prom memory */
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)board_mgmt, board_mgmt->resume_info);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)board_mgmt, board_mgmt->cacheCardResumeInfo);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)board_mgmt, board_mgmt->dimmInfo);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)board_mgmt, board_mgmt->ssdInfo);

    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/*******************************************
*   end of fbe_board_mgmt_destroy_object()
********************************************/
/*!***************************************************************
 * fbe_board_mgmt_event_entry()
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
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t 
fbe_board_mgmt_event_entry(fbe_object_handle_t object_handle, 
                           fbe_event_type_t event_type, 
                           fbe_event_context_t event_context)
{
    fbe_board_mgmt_t * board_mgmt;

    board_mgmt = (fbe_board_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)board_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    return FBE_STATUS_OK;
}
/***************************************
*   end of fbe_board_mgmt_event_entry()
****************************************/
/*!***************************************************************
 * fbe_board_mgmt_init()
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
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_init(fbe_board_mgmt_t * board_mgmt)
{
    fbe_topology_class_trace(FBE_CLASS_ID_BOARD_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    board_mgmt->object_id = FBE_OBJECT_ID_INVALID;
    board_mgmt->peerInserted = FALSE;
    board_mgmt->peerPriorityDriveAccess = FBE_TRI_STATE_INVALID;
    board_mgmt->lowBattery = FBE_FALSE;
    board_mgmt->platform_info.familyType = UNKNOWN_FAMILY_TYPE;
    board_mgmt->platform_info.platformType = SPID_UNKNOWN_HW_TYPE;
    board_mgmt->platform_info.uniqueType = NO_MODULE;
    board_mgmt->platform_info.cpuModule = INVALID_CPU_MODULE;
    board_mgmt->platform_info.enclosureType = INVALID_ENCLOSURE_TYPE;
    board_mgmt->platform_info.midplaneType = INVALID_MIDPLANE_TYPE;

    board_mgmt->resume_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)board_mgmt, sizeof(fbe_base_env_resume_prom_info_t) * MAX_NUMBER_OF_SP);
    board_mgmt->cacheCardResumeInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)board_mgmt, sizeof(fbe_base_env_resume_prom_info_t) * MAX_CACHE_CARD_COUNT);
    fbe_zero_memory(board_mgmt->resume_info, (sizeof(fbe_base_env_resume_prom_info_t) * MAX_NUMBER_OF_SP));
    fbe_zero_memory(board_mgmt->cacheCardResumeInfo, (sizeof(fbe_base_env_resume_prom_info_t) * MAX_CACHE_CARD_COUNT));
    fbe_zero_memory(board_mgmt->cacheCardInfo, (sizeof(fbe_board_mgmt_cache_card_info_t) * MAX_CACHE_CARD_COUNT));
    fbe_zero_memory(board_mgmt->suitcase_info,
                    (sizeof(fbe_board_mgmt_suitcase_info_t) * SP_ID_MAX * MAX_SUITCASE_COUNT_PER_BLADE));
    fbe_zero_memory(board_mgmt->bmc_info,
                    (sizeof(fbe_board_mgmt_bmc_info_t) * SP_ID_MAX * TOTAL_BMC_PER_BLADE));

    board_mgmt->cacheCardCount = 0;
    board_mgmt->dimmCount = 0;
    board_mgmt->suitcaseCountPerBlade = 0;
    board_mgmt->boardCacheStatus = FBE_CACHE_STATUS_FAILED;
    board_mgmt->peerSpDown = TRUE;                          // default so peerCacheStatus will be sent when peer SP seen
   
    /* Initialized  peerBootInfo */
    fbe_board_mgmt_init_peerBootInfo(&(board_mgmt->peerBootInfo));
    fbe_zero_memory(board_mgmt->spPhysicalMemorySize, sizeof(fbe_u32_t) * SP_ID_MAX);
    
    board_mgmt->dimmInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)board_mgmt, sizeof(fbe_board_mgmt_dimm_info_t) * TOTAL_DIMM_PER_BLADE * SP_ID_MAX);
    fbe_zero_memory(board_mgmt->dimmInfo, (sizeof(fbe_board_mgmt_dimm_info_t) * TOTAL_DIMM_PER_BLADE * SP_ID_MAX));
   
    board_mgmt->ssdInfo = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)board_mgmt, sizeof(fbe_board_mgmt_ssd_info_t) * FBE_MAX_SSD_SLOTS_PER_SP * SP_ID_MAX);
    fbe_zero_memory(board_mgmt->ssdInfo, sizeof(fbe_board_mgmt_ssd_info_t) * FBE_MAX_SSD_SLOTS_PER_SP * SP_ID_MAX);

    board_mgmt->fbeInternalCablePort1 = FBE_CABLE_STATUS_UNKNOWN;
    return FBE_STATUS_OK;
}
/********************************
*   end of fbe_board_mgmt_init()
*********************************/
/*!*************************************************************************
 *  fbe_board_mgmt_init_flt_exp_info(fbe_flt_exp_info_t  *flt_exp_info)
 ***************************************************************************
 * @brief
 *  This functions initialized the fault expander info structure..
 *
 * @param  flt_exp_info - The pointer to the fbe_flt_exp_info_t.
 *
 * @return - 
 * 
 * @author
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void 
fbe_board_mgmt_init_peerBootInfo(fbe_board_mgmt_peer_boot_info_t *peerBootInfo)
{
    peerBootInfo->peerSp = SP_INVALID;
    peerBootInfo->faultHwType = FBE_DEVICE_TYPE_INVALID;
    peerBootInfo->peerBootCode = FBE_PEER_BOOT_CODE_LAST;
    peerBootInfo->peerBootState = FBE_PEER_INVALID_CODE;
    peerBootInfo->StateChangeStartTime = 0;
    peerBootInfo->stateChangeProcessRequired = FALSE;    

    fbe_zero_memory(&peerBootInfo->fltRegStatus, sizeof(SPECL_FAULT_REGISTER_STATUS));
    fbe_zero_memory(&peerBootInfo->lastFltRegClearTime, sizeof(fbe_system_time_t));
    fbe_zero_memory(&peerBootInfo->lastFltRegStatus, sizeof(SPECL_FAULT_REGISTER_STATUS));
    peerBootInfo->biosPostFail = FBE_FALSE;
    return;
}
/*******************************************************
* end of fbe_board_mgmt_init_flt_exp_info()
********************************************************/
