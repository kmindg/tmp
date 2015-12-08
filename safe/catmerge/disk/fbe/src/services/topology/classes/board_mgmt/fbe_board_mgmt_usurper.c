/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/
 
/*!**************************************************************************
 * @file fbe_board_mgmt_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Board Management object usurper
 *  control code handling function.
 * 
 * @ingroup board_mgmt_class_files
 * 
 * @version
 *   29-July-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_cmi.h"
#include "fbe_board_mgmt_private.h"
#include "fbe/fbe_esp_board_mgmt.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "generic_utils_lib.h"
#include "fbe_base_environment_debug.h"


static fbe_status_t 
fbe_board_mgmt_get_board_info(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t
fbe_board_mgmt_reboot_sp(fbe_board_mgmt_t *board_mgmt, fbe_packet_t *packet);
static fbe_status_t 
fbe_board_mgmt_getCacheStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t *packet);
static fbe_status_t 
fbe_board_mgmt_getPeerBootInfo(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_getPeerCpuStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_getPeerDimmStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_getSuitcaseInfo(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_getBmcInfo(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_getCacheCardCount(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_getCacheCardStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t 
fbe_board_mgmt_flushSystem(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_board_mgmt_getDimmCount(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_board_mgmt_getDimmStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);

static fbe_status_t fbe_board_mgmt_getSSDCount(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_board_mgmt_getSSDStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet);


/*************************
 *   FUNCTION DEFINITIONS 
 *************************/
/*!***************************************************************
 * fbe_board_mgmt_control_entry()
 ****************************************************************
 * @brief
 *  This function is entry point for control operation for this
 *  class
 *
 * @param object_handle - Object handle.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t 
fbe_board_mgmt_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_board_mgmt_t *board_mgmt;
    
    board_mgmt = (fbe_board_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    
    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BOARD_INFO:
            status = fbe_board_mgmt_get_board_info(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_REBOOT_SP:
            status = fbe_board_mgmt_reboot_sp(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_STATUS:
            status = fbe_board_mgmt_getCacheStatus(board_mgmt, packet);
			break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_BOOT_INFO:
            status = fbe_board_mgmt_getPeerBootInfo(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_CPU_STATUS:
            status = fbe_board_mgmt_getPeerCpuStatus(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_DIMM_STATUS:
            status = fbe_board_mgmt_getPeerDimmStatus(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SUITCASE_INFO:
            status = fbe_board_mgmt_getSuitcaseInfo(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BMC_INFO:
            status = fbe_board_mgmt_getBmcInfo(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_CARD_COUNT:
            status = fbe_board_mgmt_getCacheCardCount(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_CARD_STATUS:
            status = fbe_board_mgmt_getCacheCardStatus(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_FLUSH_SYSTEM:
            status = fbe_board_mgmt_flushSystem(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_COUNT:
            status = fbe_board_mgmt_getDimmCount(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_INFO:
            status = fbe_board_mgmt_getDimmStatus(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_COUNT:
            status = fbe_board_mgmt_getSSDCount(board_mgmt, packet);
            break;
        case FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_INFO:
            status = fbe_board_mgmt_getSSDStatus(board_mgmt, packet);
            break;
        default:
            status =  fbe_base_environment_control_entry (object_handle, packet);
            break;
    }
    return status;
}
/******************************************
 * end fbe_board_mgmt_control_entry()
 ******************************************/
/*!***************************************************************
 * fbe_board_mgmt_get_board_info(fbe_board_mgmt_t *board_mgmt, 
 *                               fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function used to get board info
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_get_board_info(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_esp_board_mgmt_board_info_t *board_info = NULL;
    fbe_u32_t  index=0;
    fbe_u32_t  lccNum= SP_ID_MAX;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &board_info);
    if (board_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_board_mgmt_board_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__,len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Set Platform info
    board_info->platform_info = board_mgmt->platform_info;
    board_info->isSingleSpSystem = board_mgmt->base_environment.isSingleSpSystem;
    board_info->isSimulation = board_mgmt->base_environment.isSimulation;
    board_info->sp_id = board_mgmt->base_environment.spid;
    board_info->peerInserted = board_mgmt->peerInserted; // Peer SP inserted or not
    board_info->peerPresent = board_mgmt->peerInserted && 
        (fbe_pbl_BootPeerSuccessful(board_mgmt->peerBootInfo.peerBootState) ||
         fbe_pbl_BootPeerUnknown(board_mgmt->peerBootInfo.peerBootState) ) && 
        fbe_cmi_is_peer_alive();                                 // Peer SP inserted or not
    board_info->lowBattery = board_mgmt->lowBattery;
    board_info->engineIdFault = board_mgmt->engineIdFault;
    board_info->UnsafeToRemoveLED = board_mgmt->UnsafeToRemoveLED;
    board_info->internalCableStatus = board_mgmt->fbeInternalCablePort1;
    memcpy(board_info->spPhysicalMemorySize,
        board_mgmt->spPhysicalMemorySize, sizeof(board_info->spPhysicalMemorySize));
    for (index=0;index<lccNum;index++)
    {
        memcpy(&board_info->lccInfo[index], &board_mgmt->lccInfo[index], sizeof(board_mgmt->lccInfo[0]));    
    }

    if(fbe_base_env_is_active_sp_esp_cmi((fbe_base_environment_t *)board_mgmt))
    {
        board_info->is_active_sp = FBE_TRUE;
    }
    else
    {   
        board_info->is_active_sp = FBE_FALSE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_board_mgmt_get_platform_info()
 ******************************************/

/*!***************************************************************
 * fbe_board_mgmt_reboot_sp(fbe_board_mgmt_t *board_mgmt,
 *                               fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function used to reboot the sp
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  04-March-2011: Created  bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_reboot_sp(fbe_board_mgmt_t *board_mgmt, fbe_packet_t *packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_board_mgmt_set_PostAndOrReset_t *reboot_options = NULL;
    fbe_status_t status;
    fbe_base_environment_t *base_env = (fbe_base_environment_t *)board_mgmt;
    fbe_board_mgmt_set_FlushFilesAndReg_t           flushOptions;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &reboot_options);
    if (reboot_options == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_PostAndOrReset_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // if local reboot, set SP ID accordingly
    if (reboot_options->rebootLocalSp)
    {
        reboot_options->sp_id = board_mgmt->base_environment.spid;
    }

    /*
     * Flush info out if specified
     */
    if (reboot_options->flushBeforeReboot)
    {
        fbe_zero_memory(&flushOptions, sizeof(fbe_board_mgmt_set_FlushFilesAndReg_t));
        flushOptions.sp_id = reboot_options->sp_id;
        status = fbe_api_board_set_FlushFilesAndReg(board_mgmt->object_id, &flushOptions);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to Flush Files & Reg, status %d\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Flush Files & Reg successful\n",
                                  __FUNCTION__);
        }
    }

    status = fbe_base_env_reboot_sp(base_env, 
                                    reboot_options->sp_id,
                                    reboot_options->holdInPost,
                                    reboot_options->holdInReset,
                                    FBE_TRUE);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_board_mgmt_reboot_sp()
 ******************************************/

/*!***************************************************************
 * fbe_board_mgmt_flushSystem(fbe_board_mgmt_t *board_mgmt,
 *                               fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sends a Flush System (file system, registry, ...)
 *  to board object.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  19-March-2013: Created  Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_flushSystem(fbe_board_mgmt_t *board_mgmt, fbe_packet_t *packet)
{
    fbe_status_t status;
    fbe_board_mgmt_set_FlushFilesAndReg_t           flushOptions;

    /*
     * Flush info out if specified
     */
    fbe_zero_memory(&flushOptions, sizeof(fbe_board_mgmt_set_FlushFilesAndReg_t));
    flushOptions.sp_id = board_mgmt->base_environment.spid;;
    status = fbe_api_board_set_FlushFilesAndReg(board_mgmt->object_id, &flushOptions);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to Flush Files & Reg, status %d\n",
                              __FUNCTION__, status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Flush Files & Reg successful\n",
                              __FUNCTION__);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_board_mgmt_flushSystem()
 ******************************************/

/*!***************************************************************
 * fbe_board_mgmt_getCacheStatus(fbe_board_mgmt_t *board_mgmt,
 *                                 fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function used to get the Board Cache Status
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  24-March-2011: Created  Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_getCacheStatus(fbe_board_mgmt_t *board_mgmt, 
                                             fbe_packet_t *packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_common_cache_status_t *boardCacheStatus;
    fbe_common_cache_status_t peerboardCacheStatus = FBE_CACHE_STATUS_OK;
//    fbe_base_environment_t *base_env = (fbe_base_environment_t *)board_mgmt;
    fbe_status_t status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &boardCacheStatus);
    if (boardCacheStatus == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_common_cache_status_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *) board_mgmt,
                                                      &peerboardCacheStatus);

    *boardCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *) board_mgmt,
                                                                 board_mgmt->boardCacheStatus,
                                                                 peerboardCacheStatus,
                                                                 FBE_CLASS_ID_BOARD_MGMT);

    /* 
        Prevent caching on Single SP Trin, KH and KHP Simulators by forcing FBE_CACHE_STATUS_FAILED.
        Otherwise let the cache enable normally. BIOS memory persistence may not be competely supported.
    */
    if(((board_mgmt->platform_info.platformType == SPID_NOVA_S1_HW_TYPE) &&
        (board_mgmt->base_environment.isSingleSpSystem))
       || ((board_mgmt->platform_info.platformType == SPID_MERIDIAN_ESX_HW_TYPE) ||
           (board_mgmt->platform_info.platformType == SPID_TUNGSTEN_HW_TYPE))
       )
    {
        *boardCacheStatus = FBE_CACHE_STATUS_FAILED;
    }
    
    fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, boardCacheStatus %d\n",
                          __FUNCTION__, *boardCacheStatus);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_board_mgmt_getCacheStatus()
 ******************************************/


/*!***************************************************************
 * fbe_board_mgmt_get_resume_prom_info()
 ****************************************************************
 * @brief
 *  This function processes the GET_RESUME_PROM_INFO Control Code.
 *  Copies the resume prom info from board mgmt interface to the ESP.
 *  This function is called for getting the resume_prom_info for
 *  SP's.
 *
 * @param  pContext - pointer to the Enclosure object.
 * @param  packet - packet used to fill in the data requested and
 *                  track the completion of the request.
 *
 * @return fbe_status_t
 *
 * @author
 *  20-Jan-2011 - Created. Arun S
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_get_resume_prom_info(void * pContext, 
                                                 fbe_u64_t deviceType, 
                                                 fbe_device_physical_location_t pLocation, 
                                                 fbe_resume_prom_info_t *resume_prom_info)
{
    fbe_board_mgmt_t              *pBoardMgmt = (fbe_board_mgmt_t *) pContext;

    fbe_base_object_trace((fbe_base_object_t*)pBoardMgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry \n", 
                          __FUNCTION__);

    memcpy(resume_prom_info, 
           &pBoardMgmt->resume_info,
           sizeof(fbe_resume_prom_info_t));

    return FBE_STATUS_OK;
}

/*************************************************
 * end fbe_encl_mgmt_get_resume_prom_info()
 *************************************************/
 /*!**************************************************************
 * fbe_board_mgmt_getPeerBootInfo()
 *****************************************************************
 * @brief
 *  This function used to get the peer boot state.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  21-Feb-2010: Created  Vaibhav Gaonkar
 *
 *****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getPeerBootInfo(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_u32_t       index;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_esp_board_mgmt_peer_boot_info_t *ptrPeerBootInfo = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &ptrPeerBootInfo);
    if (ptrPeerBootInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_board_mgmt_peer_boot_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %x len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ptrPeerBootInfo->peerBootState = board_mgmt->peerBootInfo.peerBootState;
    ptrPeerBootInfo->faultHwType = board_mgmt->peerBootInfo.faultHwType;
    ptrPeerBootInfo->isPeerSpApplicationRunning = board_mgmt->peerBootInfo.isPeerSpApplicationRunning;

    ptrPeerBootInfo->bootCode = board_mgmt->peerBootInfo.peerBootCode;
    for(index = 0; 
        index < fbe_board_mgmt_getBootCodeSubFruCount(ptrPeerBootInfo->bootCode) && index < FBE_MAX_SUBFRU_REPLACEMENTS;
        index++)
    {
        ptrPeerBootInfo->replace[index] = fbe_board_mgmt_getBootCodeSubFruReplacements(ptrPeerBootInfo->bootCode,
                                                                                       index);
    }
    ptrPeerBootInfo->fltRegStatus           = board_mgmt->peerBootInfo.fltRegStatus;
    ptrPeerBootInfo->lastFltRegClearTime    = board_mgmt->peerBootInfo.lastFltRegClearTime;
    ptrPeerBootInfo->lastFltRegStatus       = board_mgmt->peerBootInfo.lastFltRegStatus;
    ptrPeerBootInfo->biosPostFail           = board_mgmt->peerBootInfo.biosPostFail;
    // set environmental status
    if (board_mgmt->peerBootInfo.fltRegEnvInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE)
    {
        ptrPeerBootInfo->dataStale = TRUE;
        ptrPeerBootInfo->environmentalInterfaceFault = FALSE;
    }
    else if (board_mgmt->peerBootInfo.fltRegEnvInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
    {
        ptrPeerBootInfo->dataStale = FALSE;
        ptrPeerBootInfo->environmentalInterfaceFault = TRUE;
    }
    else
    {
        ptrPeerBootInfo->dataStale = FALSE;
        ptrPeerBootInfo->environmentalInterfaceFault = FALSE;
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_board_mgmt_getPeerBootInfo()
 ******************************************/
 /*!**************************************************************
 * fbe_board_mgmt_getPeerCpuStatus()
 *****************************************************************
 * @brief
 *  This function used to get the peer CPU state.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  3-Aug-2012: Chengkai Hu - Created 
 *
 *****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getPeerCpuStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_bool_t *ptrCpuStatus = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &ptrCpuStatus);
    if (ptrCpuStatus == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_bool_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %x len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *ptrCpuStatus = board_mgmt->peerBootInfo.fltRegStatus.cpuFault;

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_board_mgmt_getPeerCpuStatus()
 ******************************************/
 /*!**************************************************************
 * fbe_board_mgmt_getPeerDimmStatus()
 *****************************************************************
 * @brief
 *  This function used to get the peer DIMM state.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  3-Aug-2012: Chengkai Hu - Created 
 *
 *****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getPeerDimmStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_bool_t *ptrDimmStatus = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &ptrDimmStatus);
    if (ptrDimmStatus == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(board_mgmt->peerBootInfo.fltRegStatus.dimmFault))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %x len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memcpy(ptrDimmStatus, board_mgmt->peerBootInfo.fltRegStatus.dimmFault, len);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_board_mgmt_getPeerDimmStatus()
 ******************************************/ 

/*!***************************************************************
 * fbe_board_mgmt_getSuitcaseInfo()
 ****************************************************************
 * @brief
 *  This function used to get the Suitcase info.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  19-Oct-2011: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getSuitcaseInfo(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_esp_board_mgmt_suitcaseInfo_t  *suitcaseInfo_ptr =NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &suitcaseInfo_ptr);
    if (suitcaseInfo_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_board_mgmt_suitcaseInfo_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    suitcaseInfo_ptr->suitcaseCount = board_mgmt->suitcaseCountPerBlade;
    suitcaseInfo_ptr->suitcaseInfo = 
        board_mgmt->suitcase_info[suitcaseInfo_ptr->location.sp][suitcaseInfo_ptr->location.slot];
    // set environmental status
    if (board_mgmt->suitcase_info[suitcaseInfo_ptr->location.sp][suitcaseInfo_ptr->location.slot].envInterfaceStatus ==
        FBE_ENV_INTERFACE_STATUS_DATA_STALE)
    {
        suitcaseInfo_ptr->dataStale = TRUE;
        suitcaseInfo_ptr->environmentalInterfaceFault = FALSE;
    }
    else if (board_mgmt->suitcase_info[suitcaseInfo_ptr->location.sp][suitcaseInfo_ptr->location.slot].envInterfaceStatus ==
             FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
    {
        suitcaseInfo_ptr->dataStale = FALSE;
        suitcaseInfo_ptr->environmentalInterfaceFault = TRUE;
    }
    else
    {
        suitcaseInfo_ptr->dataStale = FALSE;
        suitcaseInfo_ptr->environmentalInterfaceFault = FALSE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_board_mgmt_getSuitcaseInfo()
 ******************************************/

/*!***************************************************************
 * fbe_board_mgmt_getBmcInfo()
 ****************************************************************
 * @brief
 *  This function used to get the Bmc info.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  06-Sep-2012: Created  Eric Zhou
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getBmcInfo(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_esp_board_mgmt_bmcInfo_t  *bmcInfo_ptr =NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &bmcInfo_ptr);
    if (bmcInfo_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_board_mgmt_bmcInfo_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    bmcInfo_ptr->bmcInfo = 
        board_mgmt->bmc_info[bmcInfo_ptr->location.sp][bmcInfo_ptr->location.slot];
    // set environmental status
    if (board_mgmt->bmc_info[bmcInfo_ptr->location.sp][bmcInfo_ptr->location.slot].envInterfaceStatus ==
        FBE_ENV_INTERFACE_STATUS_DATA_STALE)
    {
        bmcInfo_ptr->dataStale = TRUE;
        bmcInfo_ptr->environmentalInterfaceFault = FALSE;
    }
    else if (board_mgmt->bmc_info[bmcInfo_ptr->location.sp][bmcInfo_ptr->location.slot].envInterfaceStatus ==
             FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
    {
        bmcInfo_ptr->dataStale = FALSE;
        bmcInfo_ptr->environmentalInterfaceFault = TRUE;
    }
    else
    {
        bmcInfo_ptr->dataStale = FALSE;
        bmcInfo_ptr->environmentalInterfaceFault = FALSE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * fbe_board_mgmt_getCacheCardCount()
 ****************************************************************
 * @brief
 *  This function used to get the Cache Card Count.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Feb-2013: Created  Rui Chang
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getCacheCardCount(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t  * pCacheCardCount =NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &pCacheCardCount);
    if (pCacheCardCount == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pCacheCardCount = board_mgmt->cacheCardCount;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * fbe_board_mgmt_getCacheCardStatus()
 ****************************************************************
 * @brief
 *  This function used to get the Cache Card info.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Feb-2013: Created  Rui Chang
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getCacheCardStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_board_mgmt_cache_card_info_t   *cacheCardInfo_ptr =NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &cacheCardInfo_ptr);
    if (cacheCardInfo_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_cache_card_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cacheCardInfo_ptr->inserted = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].inserted;
    cacheCardInfo_ptr->uniqueID = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].uniqueID;
    cacheCardInfo_ptr->faultLEDStatus = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].faultLEDStatus;
    cacheCardInfo_ptr->powerEnable = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].powerEnable;
    cacheCardInfo_ptr->powerGood = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].powerGood;
    cacheCardInfo_ptr->powerUpEnable = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].powerUpEnable;
    cacheCardInfo_ptr->powerUpFailure = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].powerUpFailure;
    cacheCardInfo_ptr->reset = board_mgmt->cacheCardInfo[cacheCardInfo_ptr->cacheCardID].reset;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * fbe_board_mgmt_getDimmCount()
 ****************************************************************
 * @brief
 *  This function used to get the DIMM Count.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  17-May-2013: Created  Rui Chang
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getDimmCount(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t  * pDimmCount =NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &pDimmCount);
    if (pDimmCount == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pDimmCount = board_mgmt->dimmCount;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * fbe_board_mgmt_getDimmStatus()
 ****************************************************************
 * @brief
 *  This function used to get the DIMM info.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  17-May-2013: Created  Rui Chang
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getDimmStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_esp_board_mgmt_get_dimm_info_t   *pGetDimmInfo =NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_u32_t                            dimmIndex = 0;
    fbe_char_t                           strTemp[3] = {0};
    fbe_char_t                           emcSerialNumber[2*EMC_DIMM_SERIAL_NUMBER_SIZE+1] = {0};
    fbe_char_t                           oldEmcSerialNumber[2*OLD_EMC_DIMM_SERIAL_NUMBER_SIZE+1] = {0};
    fbe_char_t                           vendorSerialNumber[2*MODULE_SERIAL_NUMBER_SIZE+1] = {0};
    fbe_u32_t                            eachByte = 0;
    fbe_status_t                         status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &pGetDimmInfo);
    if (pGetDimmInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_board_mgmt_get_dimm_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&pGetDimmInfo->dimmInfo, sizeof(pGetDimmInfo->dimmInfo));

    status = fbe_board_mgmt_get_dimm_index(board_mgmt, &pGetDimmInfo->location, &dimmIndex);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, Error in finding dimmIndex, sp %d, slot %d.\n", 
                              __FUNCTION__, pGetDimmInfo->location.sp, pGetDimmInfo->location.slot);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pGetDimmInfo->dimmInfo.associatedSp = board_mgmt->dimmInfo[dimmIndex].associatedSp; 
    pGetDimmInfo->dimmInfo.dimmID = board_mgmt->dimmInfo[dimmIndex].dimmID;
    pGetDimmInfo->dimmInfo.isLocalDimm = board_mgmt->dimmInfo[dimmIndex].isLocalFru;
    pGetDimmInfo->dimmInfo.inserted = board_mgmt->dimmInfo[dimmIndex].inserted;
    pGetDimmInfo->dimmInfo.faulted = board_mgmt->dimmInfo[dimmIndex].faulted;
    pGetDimmInfo->dimmInfo.state = board_mgmt->dimmInfo[dimmIndex].state;
    pGetDimmInfo->dimmInfo.subState = board_mgmt->dimmInfo[dimmIndex].subState;
    pGetDimmInfo->dimmInfo.dimmDensity = board_mgmt->dimmInfo[dimmIndex].dimmDensity;

    if(pGetDimmInfo->dimmInfo.inserted == FBE_MGMT_STATUS_TRUE) 
    {
        pGetDimmInfo->dimmInfo.dimmType = board_mgmt->dimmInfo[dimmIndex].dimmType;
        pGetDimmInfo->dimmInfo.dimmRevision = board_mgmt->dimmInfo[dimmIndex].dimmRevision;
        pGetDimmInfo->dimmInfo.numOfBanks = board_mgmt->dimmInfo[dimmIndex].numOfBanks;
        pGetDimmInfo->dimmInfo.deviceWidth = board_mgmt->dimmInfo[dimmIndex].deviceWidth;
        pGetDimmInfo->dimmInfo.numberOfRanks = board_mgmt->dimmInfo[dimmIndex].numberOfRanks;
        pGetDimmInfo->dimmInfo.vendorRegionValid = board_mgmt->dimmInfo[dimmIndex].vendorRegionValid;
        pGetDimmInfo->dimmInfo.manufacturingLocation = board_mgmt->dimmInfo[dimmIndex].manufacturingLocation;

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.jedecIdCode[0],
                            &board_mgmt->dimmInfo[dimmIndex].jedecIdCode[0],
                            sizeof(pGetDimmInfo->dimmInfo.jedecIdCode));

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.manufacturingDate[0],
                            &board_mgmt->dimmInfo[dimmIndex].manufacturingDate[0],
                            sizeof(pGetDimmInfo->dimmInfo.manufacturingDate));

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.vendorPartNumber[0],
                            &board_mgmt->dimmInfo[dimmIndex].partNumber[0],
                            sizeof(pGetDimmInfo->dimmInfo.vendorPartNumber));

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.EMCDimmPartNumber[0],
                            &board_mgmt->dimmInfo[dimmIndex].EMCDimmPartNumber[0],
                            sizeof(pGetDimmInfo->dimmInfo.EMCDimmPartNumber));

        /*moduleSerialNumber, i.e. vendorSerialNumber needs to be convert to hex.*/
        for (eachByte = 0; eachByte < MODULE_SERIAL_NUMBER_SIZE; eachByte++)
        {
            sprintf(strTemp, "%02X", board_mgmt->dimmInfo[dimmIndex].moduleSerialNumber[eachByte]);
            strcat(&vendorSerialNumber[0], strTemp);
        }

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.vendorSerialNumber[0],
                            &vendorSerialNumber[0],
                            sizeof(vendorSerialNumber));

        /*EMCDimmSerialNumber needs to be convert to hex.*/
        for (eachByte = 0; eachByte < EMC_DIMM_SERIAL_NUMBER_SIZE; eachByte++)
        {
            sprintf(strTemp, "%02X", board_mgmt->dimmInfo[dimmIndex].EMCDimmSerialNumber[eachByte]);
            strcat(&emcSerialNumber[0], strTemp);
        }

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.EMCDimmSerialNumber[0],
                            &emcSerialNumber[0],
                            sizeof(emcSerialNumber));

        /*OldEMCDimmSerialNumber needs to be convert to hex.*/
        for (eachByte = 0; eachByte < OLD_EMC_DIMM_SERIAL_NUMBER_SIZE; eachByte++)
        {
            sprintf(strTemp, "%02X", board_mgmt->dimmInfo[dimmIndex].OldEMCDimmSerialNumber[eachByte]);
            strcat(&oldEmcSerialNumber[0], strTemp);
        }

        fbe_copy_memory(&pGetDimmInfo->dimmInfo.OldEMCDimmSerialNumber[0],
                            &oldEmcSerialNumber[0],
                            sizeof(oldEmcSerialNumber));

        /* Get vendorName from jedecIdCode. */
        fbe_copy_memory(&pGetDimmInfo->dimmInfo.vendorName[0],
                            decodeJedecIDCode(board_mgmt->dimmInfo[dimmIndex].jedecIdCode[0]),
                            sizeof(pGetDimmInfo->dimmInfo.vendorName));

        /* Get assemblyName from deviceType. */
        fbe_copy_memory(&pGetDimmInfo->dimmInfo.assemblyName[0],
                            decodeSpdDimmType(board_mgmt->dimmInfo[dimmIndex].dimmType),
                            sizeof(pGetDimmInfo->dimmInfo.assemblyName));
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_board_mgmt_getSSDCount()
 ****************************************************************
 * @brief
 *  This function used to get the SSD Count.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2014: Created  bphilbin
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getSSDCount(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t  * pSSDCount =NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &pSSDCount);
    if (pSSDCount == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pSSDCount = board_mgmt->ssdCount;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}


/*!***************************************************************
 * fbe_board_mgmt_getSSDStatus()
 ****************************************************************
 * @brief
 *  This function used to get the SSD info.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param packet - Packet
 *
 * @return fbe_status_t
 *
 * @author
 *  17-May-2013: Created  Rui Chang
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_getSSDStatus(fbe_board_mgmt_t *board_mgmt, fbe_packet_t * packet)
{
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_esp_board_mgmt_get_ssd_info_t   *pGetSsdInfo =NULL;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_u32_t                            ssdIndex = 0;
    fbe_status_t                         status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &pGetSsdInfo);
    if (pGetSsdInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_board_mgmt_get_ssd_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&pGetSsdInfo->ssdInfo, sizeof(pGetSsdInfo->ssdInfo));

    status = fbe_board_mgmt_get_ssd_index(board_mgmt, &pGetSsdInfo->location, &ssdIndex);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, Error in finding ssdIndex, sp %d, slot %d.\n", 
                              __FUNCTION__, pGetSsdInfo->location.sp, pGetSsdInfo->location.slot);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pGetSsdInfo->ssdInfo.associatedSp = board_mgmt->ssdInfo[ssdIndex].associatedSp;
    pGetSsdInfo->ssdInfo.slot = board_mgmt->ssdInfo[ssdIndex].slot;
    pGetSsdInfo->ssdInfo.isLocalSsd = board_mgmt->ssdInfo[ssdIndex].isLocalFru;
    pGetSsdInfo->ssdInfo.ssdState = board_mgmt->ssdInfo[ssdIndex].ssdState;
    pGetSsdInfo->ssdInfo.ssdSubState = board_mgmt->ssdInfo[ssdIndex].ssdSubState;
    pGetSsdInfo->ssdInfo.remainingSpareBlkCount = board_mgmt->ssdInfo[ssdIndex].remainingSpareBlkCount;
    pGetSsdInfo->ssdInfo.ssdLifeUsed = board_mgmt->ssdInfo[ssdIndex].ssdLifeUsed;
    pGetSsdInfo->ssdInfo.ssdTemperature = board_mgmt->ssdInfo[ssdIndex].ssdTemperature;
    pGetSsdInfo->ssdInfo.ssdSelfTestPassed = board_mgmt->ssdInfo[ssdIndex].ssdSelfTestPassed;
    fbe_copy_memory(&pGetSsdInfo->ssdInfo.ssdSerialNumber[0], &board_mgmt->ssdInfo[ssdIndex].ssdSerialNumber[0], FBE_SSD_SERIAL_NUMBER_SIZE);
    fbe_copy_memory(&pGetSsdInfo->ssdInfo.ssdPartNumber[0], &board_mgmt->ssdInfo[ssdIndex].ssdPartNumber[0], FBE_SSD_PART_NUMBER_SIZE);
    fbe_copy_memory(&pGetSsdInfo->ssdInfo.ssdAssemblyName[0], &board_mgmt->ssdInfo[ssdIndex].ssdAssemblyName[0], FBE_SSD_ASSEMBLY_NAME_SIZE);
    fbe_copy_memory(&pGetSsdInfo->ssdInfo.ssdFirmwareRevision[0], &board_mgmt->ssdInfo[ssdIndex].ssdFirmwareRevision[0],FBE_SSD_FIRMWARE_REVISION_SIZE);

    fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Slot:%d state:%d SpBlkCnt:%d PctLifeUsd:%d SlfTestPssd:%d\n",
                          __FUNCTION__,
                          pGetSsdInfo->ssdInfo.slot,
                          pGetSsdInfo->ssdInfo.ssdState,
                          pGetSsdInfo->ssdInfo.remainingSpareBlkCount,
                          pGetSsdInfo->ssdInfo.ssdLifeUsed,
                          pGetSsdInfo->ssdInfo.ssdSelfTestPassed);


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_board_mgmt_updateSpFailStatus()
 ****************************************************************
 * @brief
 *  This function will set the appropriate PS failed status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Apr-2015 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_board_mgmt_updateSpFailStatus(fbe_u8_t *mgmtPtr,
                                  fbe_device_physical_location_t *pLocation,
                                  fbe_base_environment_failure_type_t failureType,
                                  fbe_bool_t newValue)
{
    fbe_status_t                    status;
    fbe_board_mgmt_t                *boardMgmtPtr;
    fbe_board_mgmt_suitcase_info_t  *suitcasePtr;
    fbe_u8_t                        deviceStr[FBE_DEVICE_STRING_LENGTH]={0};

    boardMgmtPtr = (fbe_board_mgmt_t *)mgmtPtr;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SP, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)boardMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    suitcasePtr = &boardMgmtPtr->suitcase_info[pLocation->sp][pLocation->slot];

    switch (failureType)
    {
    case FBE_BASE_ENV_RP_READ_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)boardMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, resPromReadFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              suitcasePtr->resumePromReadFailed,
                              newValue);
        suitcasePtr->resumePromReadFailed = newValue;
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t *)boardMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Unsupported failureType %d.\n", 
                              __FUNCTION__, failureType); 
        return FBE_STATUS_COMPONENT_NOT_FOUND;
        break;
    }

    return FBE_STATUS_OK;

}   // end of fbe_board_mgmt_updateSpFailStatus
