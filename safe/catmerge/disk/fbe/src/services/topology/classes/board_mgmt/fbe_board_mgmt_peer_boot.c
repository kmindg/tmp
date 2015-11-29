/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_board_mgmt_peer_boot.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the function related to peer boot logging functionality
 *
 * @ingroup board_mgmt_class_files
 * 
 * @version
 *  04-Jan-2011: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "fbe_board_mgmt_private.h"
#include "generic_utils_lib.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_database_interface.h"

static void fbe_board_mgmt_setExtPeerBootInfo(fbe_board_mgmt_t                  *board_mgmt,
                                              fbe_board_mgmt_slave_port_info_t  *slavePortInfoPtr);
static void fbe_board_mgmt_fltReg_logFaultByFruType(fbe_board_mgmt_t  * board_mgmt,
                                                    fbe_peer_boot_fru_t fruType,
                                                    fbe_u32_t           slotNum,
                                                    fbe_bool_t          faultStatus);
static void fbe_board_mgmt_fltReg_logHungState(fbe_board_mgmt_t          *board_mgmt);
static void fbe_board_mgmt_fltReg_logBootState(fbe_board_mgmt_t              *board_mgmt,
                                               fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr);
static fbe_bool_t fbe_board_mgmt_fltReg_checkNewBiosPostFault(fbe_board_mgmt_t             *board_mgmt,
                                                              fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr);
static void fbe_board_mgmt_fltReg_logFault(fbe_board_mgmt_t              *board_mgmt,
                                           fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr);
static void fbe_board_mgmt_fltReg_clearPeerFault(fbe_board_mgmt_t              *board_mgmt,
                                                         fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr);
static void fbe_board_mgmt_fltReg_processBiosPostFault(fbe_board_mgmt_t              *board_mgmt,
                                                     fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr);
static fbe_status_t fbe_board_mgmt_fltReg_updateEnclFaultLed(fbe_board_mgmt_t *board_mgmt,
                                                             fbe_encl_fault_led_reason_t enclFaultLedReason);

/*!**************************************************************
 * fbe_board_mgmt_peer_boot_log_fail()
 ****************************************************************
 * @brief
 *  This function write event log while peer boot fail
 *
 * @param faultStatus - fault bool 
 *        faultStr - fault decoded string to log
 *        peerSpStr - Peer SP string to log  
 *
 * @return void - 
 * 
 * @author
 *  17-Feb-2013: Chengkai Hu - Created
 *
 ****************************************************************/
static __forceinline void
fbe_board_mgmt_peer_boot_log_fail(fbe_bool_t    faultStatus,
                                  const char  * faultStr,
                                  const char  * peerSpStr)
{
    if (faultStatus)
    {
        fbe_event_log_write(ESP_CRIT_PEERSP_POST_FAIL, 
                            NULL, 
                            0, 
                            "%s %s", 
                            peerSpStr, 
                            faultStr); 
    }
}
/***********************************************************
* end of fbe_board_mgmt_peer_boot_log_fail_fru()
***********************************************************/
/*!**************************************************************
 * fbe_board_mgmt_peer_boot_log_fail_fru()
 ****************************************************************
 * @brief
 *  This function write event log while peer boot fail in FRU
 *
 * @param faultStatus - fault bool 
 *        faultStr - fault decoded string to log
 *        peerSpStr - Peer SP string to log  
 *        cpuStatusRegister - CPU status register value in fault register
 *
 * @return void - 
 * 
 * @author
 *  07-Aug-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static __forceinline void
fbe_board_mgmt_peer_boot_log_fail_fru(fbe_bool_t    faultStatus,
                                      const char  * faultStr,
                                      const char  * peerSpStr)
{
    if (faultStatus)
    {
        fbe_event_log_write(ESP_CRIT_PEERSP_POST_FAIL_FRU, 
                            NULL, 
                            0, 
                            "%s %s", 
                            peerSpStr, 
                            faultStr); 
    }
}
/***********************************************************
* end of fbe_board_mgmt_peer_boot_log_fail_fru()
***********************************************************/

/*!**************************************************************
 * fbe_board_mgmt_peer_boot_log_fail_fru_slot()
 ****************************************************************
 * @brief
 *  This function write event log while peer boot fail in FRU with
 *  FRU slot number
 *
 * @param slotNum - slot number of FRU 
 *        faultStatus - fault bool 
 *        faultStr - fault decoded string to log
 *        peerSpStr - Peer SP string to log  
 *        cpuStatusRegister - CPU status register value in fault register
 *
 * @return void - 
 * 
 * @author
 *  07-Aug-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static __forceinline void
fbe_board_mgmt_peer_boot_log_fail_fru_slot(fbe_u32_t     slotNum,
                                           fbe_bool_t    faultStatus,
                                           const char  * faultStr,
                                           const char  * peerSpStr)
{
    if (faultStatus)
    {
        fbe_event_log_write(ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_NUM, 
                            NULL, 
                            0, 
                            "%s %s %d", 
                            peerSpStr, 
                            faultStr,
                            slotNum); 
    }
}
/***********************************************************
* end of fbe_board_mgmt_peer_boot_log_fail_fru_slot()
***********************************************************/

/*!**************************************************************
 * fbe_board_mgmt_peer_boot_log_fail_fru_partnum()
 ****************************************************************
 * @brief
 *  This function write event log while peer boot fail in FRU with
 *  FRU part number
 *
 * @param objectId - object arg for get part num
 *        deviceType - FRU device type
 *        spId - SP id where FRU locate
 *        slotNum - FRU slot number
 *        faultStatus - fault bool 
 *        faultStr - fault decoded string to log
 *        peerSpStr - Peer SP string to log  
 *        cpuStatusRegister - CPU status register value in fault register
 *
 * @return void - 
 * 
 * @author
 *  07-Aug-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static __forceinline void
fbe_board_mgmt_peer_boot_log_fail_fru_partnum(fbe_board_mgmt_t* board_mgmt,
                                              fbe_u64_t deviceType,
                                              fbe_u8_t          slotNum,
                                              fbe_bool_t        faultStatus,
                                              const char      * faultStr,
                                              const char      * peerSpStr)
{
    fbe_object_id_t                 objectId;
    fbe_u8_t                        spId;
    fbe_status_t                    status;
    fbe_device_physical_location_t  device_location = {0};
    fbe_u8_t                        tla_num[FBE_BOARD_MGMT_TLA_FULL_LENGTH];

    objectId = board_mgmt->object_id;
    spId = board_mgmt->peerBootInfo.peerSp;
    
    if (faultStatus)
    {
        device_location.slot = slotNum;
        device_location.sp = spId;
        status = fbe_api_resume_prom_get_tla_num(objectId,
                                       deviceType, 
                                       &device_location, 
                                       tla_num,
                                       sizeof(tla_num));

        if (status != FBE_STATUS_OK || !strncmp(tla_num, "", sizeof(tla_num)))
        {
            strncpy(&tla_num[0], "Not Found", sizeof(tla_num));
            tla_num[sizeof(tla_num)-1] = '\0';
        }

        fbe_event_log_write(ESP_CRIT_PEERSP_POST_FAIL_FRU_PART_NUM, 
                            NULL, 
                            0, 
                            "%s %s %s", 
                            peerSpStr, 
                            faultStr,
                            &tla_num[0]); 
    }
}
/***********************************************************
* end of fbe_board_mgmt_peer_boot_log_fail_fru_partnum()
***********************************************************/
/*!**************************************************************
 * fbe_board_mgmt_peer_boot_log_fail_fru_slot_partnum()
 ****************************************************************
 * @brief
 *  This function write event log while peer boot fail in FRU with
 *  FRU slot and part number
 *
 * @param objectId - object arg for get part num
 *        deviceType - FRU device type
 *        spId - SP id where FRU locate
 *        slotNum - FRU slot number
 *        faultStatus - fault bool 
 *        faultStr - fault decoded string to log
 *        peerSpStr - Peer SP string to log  
 *        cpuStatusRegister - CPU status register value in fault register
 *
 * @return void - 
 * 
 * @author
 *  24-Dec-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static __forceinline void
fbe_board_mgmt_peer_boot_log_fail_fru_slot_partnum(fbe_board_mgmt_t* board_mgmt,
                                                   fbe_u64_t deviceType,
                                                   fbe_u8_t          slotNum,
                                                   fbe_bool_t        faultStatus,
                                                   const char      * faultStr,
                                                   const char      * peerSpStr)
{
    fbe_object_id_t                 objectId;
    fbe_u8_t                        spId;
    fbe_status_t                    status;
    fbe_device_physical_location_t  device_location = {0};
    fbe_u8_t                        tla_num[FBE_BOARD_MGMT_TLA_FULL_LENGTH];
    
    objectId = board_mgmt->object_id;
    spId = board_mgmt->peerBootInfo.peerSp;

    if (faultStatus)
    {
        device_location.slot = slotNum;
        device_location.sp = spId;
        status = fbe_api_resume_prom_get_tla_num(objectId,
                                       deviceType, 
                                       &device_location, 
                                       tla_num,
                                       sizeof(tla_num));
        if (status != FBE_STATUS_OK || !strncmp(tla_num, "", sizeof(tla_num)))
        {
            strncpy(&tla_num[0], "Not Found", sizeof(tla_num));
            tla_num[sizeof(tla_num)-1] = '\0';
        }

        fbe_event_log_write(ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_PART_NUM, 
                            NULL, 
                            0, 
                            "%s %s %d %s", 
                            peerSpStr, 
                            faultStr,
                            slotNum,
                            &tla_num[0]); 
    }
}
/***********************************************************
* end of fbe_board_mgmt_peer_boot_log_fail_fru_slot_partnum()
***********************************************************/

/*!**************************************************************
 * fbe_board_mgmt_getBootCodeSubFruCount()
 ****************************************************************
 * @brief
 *   Depend upon boot code.this function return the subFru 
 *   count from pre-defined table fbe_PeerBootEntries. 
 *
 * @param bootCode - Boot code that received from Fault Expander 
 *
 * @return fbe_u32_t - Number of subFru count
 * 
 * @author
 *  04-Jan-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_u32_t 
fbe_board_mgmt_getBootCodeSubFruCount(fbe_u8_t bootCode)
{
    if(bootCode == FBE_PEER_BOOT_CODE_LAST)
    {
        return(fbe_PeerBootEntries[FBE_PEER_BOOT_CODE_LIMIT1+1].numberOfSubFrus);
    }
    return(fbe_PeerBootEntries[bootCode].numberOfSubFrus);;
}
/************************************************
* end of fbe_board_mgmt_getBootCodeSubFruCount()
************************************************/

/*!**************************************************************
 * fbe_board_mgmt_getBootCodeSubFruReplacements()
 ****************************************************************
 * @brief
 *   Depend upon boot code.this function return the subFru which 
 *   need to replace.
 *
 * @param bootCode - Boot code that received from Fault Expander 
 *        num - The subFru number
 *
 * @return fbe_subfru_replacements_t - SubFru
 * 
 * @author
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_subfru_replacements_t
fbe_board_mgmt_getBootCodeSubFruReplacements(fbe_u8_t bootCode, fbe_u32_t num)
{
    if(num >= FBE_MAX_SUBFRU_REPLACEMENTS)
    {
        return FBE_SUBFRU_NONE;
    }
    
    if (bootCode == FBE_PEER_BOOT_CODE_LAST)
    {
        return(fbe_PeerBootEntries[FBE_PEER_BOOT_CODE_LIMIT1+1].replace[num]);
    }
    return(fbe_PeerBootEntries[bootCode].replace[num]);
}
/*******************************************************
* end of fbe_board_mgmt_getBootCodeSubFruReplacements()
*******************************************************/

/*!**************************************************************
 * fbe_board_mgmt_check_usb_loader()
 ****************************************************************
 * @brief
 *   This function check for USB Boot Loader.
 *
 * @param bootCode - Boot code 
 *
 * @return TRUE - USB Loder.
 *         FALSE - Otherwise
 * 
 * @author
 *  10-Mar-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_bool_t
fbe_board_mgmt_check_usb_loader(fbe_u8_t boot_code)
{
    if((boot_code >= FBE_PEER_BOOT_USB_START) &&
       (boot_code <= FBE_PEER_BOOT_USB_SUCESS))
    {
        return TRUE;
    }
    return FALSE;
}
/***********************************************
* end of fbe_board_mgmt_check_usb_loader()
***********************************************/

/*!**************************************************************
 * fbe_board_mgmt_handle_peer_boot_policy()
 ****************************************************************
 * @brief
 *  This function adjusts the queue depth on system drives and 
 *  enables/disables BGS in response to the peer starting or
 *  completing a reboot.
 *
 * @param board_mgmt - The handle to the board mgmt object 

 * @return N/A 
 * 
 * @author
 *  23-May-2013: fersom - Created
 *
 ****************************************************************/
void
fbe_board_mgmt_handle_peer_boot_policy(fbe_board_mgmt_t * board_mgmt)
{
    fbe_status_t            status = FBE_STATUS_FAILED;
    FBE_TRI_STATE           peerPriorityDriveAccess = FBE_TRI_STATE_FALSE;
    FBE_TRI_STATE           oldPeerPriorityDriveAccess = board_mgmt->peerPriorityDriveAccess;
    fbe_peer_boot_states_t  bootState;
    fbe_database_state_t    state = FBE_DATABASE_STATE_INVALID;

    bootState = board_mgmt->peerBootInfo.peerBootState;

    if(bootState == FBE_PEER_BOOTING ||
       bootState == FBE_PEER_DUMPING ||
       bootState == FBE_PEER_SW_LOADING)
    {
        peerPriorityDriveAccess = FBE_TRI_STATE_TRUE;
    }

    status  = fbe_api_database_get_state(&state);
    if((status != FBE_STATUS_OK) || (state != FBE_DATABASE_STATE_READY)) {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "ESP : Peer boot : SEP is not up yet\n");
        return;
    }

    if((peerPriorityDriveAccess == FBE_TRI_STATE_FALSE) &&
       (oldPeerPriorityDriveAccess == FBE_TRI_STATE_INVALID))
    {
        board_mgmt->peerPriorityDriveAccess = peerPriorityDriveAccess;
        return;
    }

    if(peerPriorityDriveAccess != oldPeerPriorityDriveAccess)
    {
        board_mgmt->peerPriorityDriveAccess = peerPriorityDriveAccess;

        if(peerPriorityDriveAccess)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "ESP : Peer has started booting; reducing q depth and stopping bgs\n");

            status = fbe_api_esp_drive_mgmt_reduce_system_drive_queue_depth();
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "ESP : Peer boot : Reduce q depth failed 0x%X\n",
                                      status);
            }

            status = fbe_api_disable_all_bg_services_single_sp();
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "ESP : Peer boot : start bgs failed 0x%X\n",
                                      status);
            }
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "ESP : Peer has stopped booting; restoring q depth and starting bgs\n");

            status = fbe_api_esp_drive_mgmt_restore_system_drive_queue_depth();
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "ESP : Peer boot : Restore q depth failed 0x%X\n",
                                      status);
            }

            status = fbe_api_enable_all_bg_services_single_sp();
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "ESP : Peer boot : start bgs failed 0x%X\n",
                                      status);
            }
        }
    }
}
/***********************************************
* end of fbe_board_mgmt_handle_peer_boot_policy()
***********************************************/

/*START: Fault Register part */
/*!**************************************************************
 * fbe_board_mgmt_setPeerBootInfo()
 ****************************************************************
 * @brief
 *  This function update board_mgmt peer boot info from fault register.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *        fltRegInfoPtr - This is fault register handle
 * @return N/A 
 * 
 * @author
 *  28-July-2012: Chengkai Hu - Created
 *
 ****************************************************************/
void
fbe_board_mgmt_setPeerBootInfo(fbe_board_mgmt_t             *board_mgmt,
                               fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr)
{
    fbe_status_t    status = FBE_STATUS_FAILED;
        
    board_mgmt->peerBootInfo.peerSp = fltRegInfoPtr->associatedSp;
    board_mgmt->peerBootInfo.faultHwType = fltRegInfoPtr->fltHwType;
    /* Update peer boot state with cpu register status */
    board_mgmt->peerBootInfo.peerBootState = fbe_pbl_getPeerBootState(&fltRegInfoPtr->faultRegisterStatus);
   
    /* If peer cpu status changed, start timer to to check peer boot state. e.g. hung. */
    if(board_mgmt->peerBootInfo.fltRegStatus.cpuStatusRegister != fltRegInfoPtr->faultRegisterStatus.cpuStatusRegister)
    {
        board_mgmt->peerBootInfo.StateChangeStartTime = fbe_get_time();
        
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(board_mgmt), 
                                                (fbe_base_object_t*)board_mgmt, 
                                                FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "PBL: %s, can't clear timer condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        
    }

    memcpy(&board_mgmt->peerBootInfo.fltRegStatus, &fltRegInfoPtr->faultRegisterStatus,
           sizeof(SPECL_FAULT_REGISTER_STATUS));

    if(board_mgmt->peerBootInfo.fltRegStatus.cpuStatusRegister == CSR_OS_APPLICATION_RUNNING)
    {
        board_mgmt->peerBootInfo.isPeerSpApplicationRunning = FBE_TRUE;
    }
    else
    {
        board_mgmt->peerBootInfo.isPeerSpApplicationRunning = FBE_FALSE;
    }

    fbe_board_mgmt_handle_peer_boot_policy(board_mgmt);
}
/***********************************************************
* end of fbe_board_mgmt_setPeerBootInfo()
***********************************************************/
/*!**************************************************************
 * fbe_board_mgmt_setExtPeerBootInfo()
 ****************************************************************
 * @brief
 *  This function update board_mgmt extended peer boot info from Slave Port.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *        slavePortInfoPtr - This is Slave Port handle
 * @return N/A 
 * 
 * @author
 *  05-Sep-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static void
fbe_board_mgmt_setExtPeerBootInfo(fbe_board_mgmt_t                  *board_mgmt,
                                  fbe_board_mgmt_slave_port_info_t  *slavePortInfoPtr)
{
    board_mgmt->extPeerBootInfo.peerSp = slavePortInfoPtr->associatedSp;
    board_mgmt->extPeerBootInfo.generalStatus = slavePortInfoPtr->generalStatus;
    board_mgmt->extPeerBootInfo.componentStatus = slavePortInfoPtr->componentStatus;
    board_mgmt->extPeerBootInfo.componentExtStatus = slavePortInfoPtr->componentExtStatus;
}
/***********************************************************
* end of fbe_board_mgmt_setExtPeerBootInfo()
***********************************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_logFaultByFruType()
 ****************************************************************
 * @brief
 *   This function do 
 *   event log according to fault type,slot and fault value.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return N/A
 * 
 * @author
 *  06-Nov-2013: Chengkai Hu - Created
 *
 ****************************************************************/
static void
fbe_board_mgmt_fltReg_logFaultByFruType(fbe_board_mgmt_t  * board_mgmt,
                                        fbe_peer_boot_fru_t fruType,
                                        fbe_u32_t           slotNum,
                                        fbe_bool_t          faultStatus)
{
    fbe_board_mgmt_peer_boot_info_t * boardPeerBootInfo = NULL;
    const fbe_char_t                * peerSpStr = NULL;
    fbe_u8_t                          faultStr[128];

    boardPeerBootInfo = &board_mgmt->peerBootInfo;
    peerSpStr = decodeSpId(boardPeerBootInfo->peerSp);

    switch(fruType)
    {
        //Log FRU fault with slot number
        case PBL_FRU_DIMM:
        case PBL_FRU_DRIVE:
        case PBL_FRU_FAN:
            fbe_board_mgmt_peer_boot_log_fail_fru_slot(slotNum,
                                                       faultStatus,
                                                       fbe_pbl_decodeFru(fruType),
                                                       peerSpStr);
        break;

        //Log FRU fault with slot number and part number
        case PBL_FRU_SLIC:
        case PBL_FRU_POWER:
            fbe_board_mgmt_peer_boot_log_fail_fru_slot_partnum(board_mgmt,
                                                               fbe_pbl_mapFbeDeviceType(fruType),
                                                               slotNum,
                                                               faultStatus,
                                                               fbe_pbl_decodeFru(fruType),
                                                               peerSpStr);
        break;

        //Log FRU fault with part number
        case PBL_FRU_BATTERY:
        case PBL_FRU_CPU:
        case PBL_FRU_MGMT:
        case PBL_FRU_BM:
        case PBL_FRU_MIDPLANE:
            fbe_board_mgmt_peer_boot_log_fail_fru_partnum(board_mgmt,
                                                          fbe_pbl_mapFbeDeviceType(fruType),
                                                          slotNum,
                                                          faultStatus,
                                                          fbe_pbl_decodeFru(fruType),
                                                          peerSpStr);
        break;

        //Log only FRU fault 
        case PBL_FRU_SUPERCAP:
        case PBL_FRU_EFLASH:
        case PBL_FRU_CACHECARD:
        case PBL_FRU_CMI:
        case PBL_FRU_ALL_FRU:
        case PBL_FRU_EXTERNAL_FRU:
            fbe_board_mgmt_peer_boot_log_fail_fru(faultStatus,
                                                  fbe_pbl_decodeFru(fruType),
                                                  peerSpStr);
        break;

        //Misc type log
        case PBL_FRU_I2C:
            _snprintf(faultStr, sizeof(faultStr), "Detected a fault isolated to I2C Bus %d", slotNum);
            faultStr[sizeof(faultStr)-1] = '\0';
            fbe_board_mgmt_peer_boot_log_fail(faultStatus,
                                              faultStr,
                                              peerSpStr);
        break;
        case PBL_FRU_ALL_DIMM:
            _snprintf(faultStr, sizeof(faultStr), "Recommend replacement of all DIMMs. Please contact your service provider.");
            faultStr[sizeof(faultStr)-1] = '\0';
            fbe_board_mgmt_peer_boot_log_fail(faultStatus,
                                              faultStr,
                                              peerSpStr);
        break;
        case PBL_FRU_ONBOARD_DRIVE:
            _snprintf(faultStr, sizeof(faultStr), "Recommend replacement of onboard drive %d. Please contact your service provider.", slotNum);
            faultStr[sizeof(faultStr)-1] = '\0';
            fbe_board_mgmt_peer_boot_log_fail(faultStatus,
                                              faultStr,
                                              peerSpStr);
        break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "PBL:%s Invalid type of fru %d.\n",
                                   __FUNCTION__,
                                   fruType);
        break;
    }//switch
}
/********************************************
* end of fbe_board_mgmt_fltReg_logFaultByFruType()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_logHungState()
 ****************************************************************
 * @brief
 *   This function do event log according to CPU status register.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  16-Feb-2013: Chengkai Hu - Created
 *  06-Nov-2013: Chengkai Hu - Add bad fru replacement event per hung code
 *
 ****************************************************************/
static void
fbe_board_mgmt_fltReg_logHungState(fbe_board_mgmt_t              *board_mgmt)
{
    fbe_status_t                    fbe_status = FBE_STATUS_FAILED;
    fbe_u32_t                       eventCode = 0;
    fbe_board_mgmt_peer_boot_info_t *boardPeerBootInfo = NULL;
    SPECL_FAULT_REGISTER_STATUS   * boardFltRegStatus = NULL;
    fbe_u32_t                       cpuStatus;
    const fbe_char_t              * peerSpStr = NULL;
    fbe_peer_boot_states_t          bootState;
    fbe_u32_t                       badFruNum = 0, eachBadFru = 0;
    fbe_peer_boot_bad_fru_t         badFru;

    boardPeerBootInfo = &board_mgmt->peerBootInfo;
    boardFltRegStatus = &board_mgmt->peerBootInfo.fltRegStatus;
    bootState = board_mgmt->peerBootInfo.peerBootState;
    peerSpStr = decodeSpId(boardPeerBootInfo->peerSp);
    cpuStatus = boardFltRegStatus->cpuStatusRegister;

    if (bootState == FBE_PEER_HUNG)
    {
        /*
         * Log peer sp hung event.
         */
        eventCode = ESP_CRIT_PEER_SP_POST_HUNG;
        fbe_event_log_write(eventCode, 
                            NULL, 
                            0, 
                            "%s %s %x", 
                            peerSpStr, 
                            decodeCpuStatusRegister(cpuStatus),
                            cpuStatus); 

        /* 
         * Get all bad fru to raise replacement event, according to hung status code.
         */
        badFruNum = fbe_pbl_getBadFruNum(cpuStatus);
        if (badFruNum > FBE_MAX_SUBFRU_REPLACEMENTS)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "PBL:Hung:Invalid bad fru number %d,cpu status:0x%x(%s).\n",
                                   badFruNum,
                                   cpuStatus,
                                   decodeCpuStatusRegister(cpuStatus));
            return;
        }
        
        for(eachBadFru=0;eachBadFru<badFruNum;eachBadFru++)
        {
            fbe_status = fbe_pbl_getBadFru(cpuStatus,
                                           eachBadFru,
                                           &badFru);
            if (fbe_status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                       FBE_TRACE_LEVEL_INFO,
                                       FBE_TRACE_MESSAGE_ID_INFO, 
                                       "PBL:Hung:Failed to get bad fru, index %d,cpu status:0x%x(%s).\n",
                                       eachBadFru,
                                       cpuStatus,
                                       decodeCpuStatusRegister(cpuStatus));
                continue;
            }
            
            fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                                    badFru.type,
                                                    badFru.id,
                                                    FBE_TRUE);
        }
        
    }
}
/********************************************
* end of fbe_board_mgmt_fltReg_logHungState()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_logBootState()
 ****************************************************************
 * @brief
 *   This function do event log according to CPU status register.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  16-Feb-2013: Chengkai Hu - Created
 *
 ****************************************************************/
static void
fbe_board_mgmt_fltReg_logBootState(fbe_board_mgmt_t              *board_mgmt,
                                   fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr)
{
    fbe_board_mgmt_peer_boot_info_t *boardPeerBootInfo = NULL;
    SPECL_FAULT_REGISTER_STATUS   * boardFltRegStatus = NULL;
    fbe_u32_t                       newCpuStatus, oldCpuStatus;
    const fbe_char_t              * peerSpStr = NULL;
    fbe_peer_boot_states_t          oldBootState;
    fbe_peer_boot_states_t          newBootState;
    fbe_device_physical_location_t phys_location = {0};

    phys_location.sp = ((board_mgmt->base_environment.spid == SP_A) ? SP_B : SP_A);

    boardPeerBootInfo = &board_mgmt->peerBootInfo;
    boardFltRegStatus = &board_mgmt->peerBootInfo.fltRegStatus;
    peerSpStr = decodeSpId(boardPeerBootInfo->peerSp);
    oldCpuStatus = board_mgmt->peerBootInfo.fltRegStatus.cpuStatusRegister;
    newCpuStatus = fltRegInfoPtr->faultRegisterStatus.cpuStatusRegister;
    oldBootState = boardPeerBootInfo->peerBootState;
    newBootState = fbe_pbl_getPeerBootState(&fltRegInfoPtr->faultRegisterStatus);
    

    /* Log boot state and trace CPU status change */
    if (newCpuStatus != oldCpuStatus)
    {
        switch(newCpuStatus)
        {
            case CSR_OS_DEGRADED_MODE:
            case CSR_OS_APPLICATION_RUNNING:
            case CSR_OS_BLADE_BEING_SERVICED:
                fbe_event_log_write(ESP_INFO_PEER_SP_BOOT_OTHER, 
                                    NULL, 
                                    0, 
                                    "%s %s %x", 
                                    peerSpStr, 
                                    decodeCpuStatusRegister(newCpuStatus),
                                    newCpuStatus); 
            break;
            default:
            break;
        }
        
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL:%s - %s(0x%x)\n",
                               peerSpStr,
                               decodeCpuStatusRegister(newCpuStatus),
                               newCpuStatus);
    }

    if (oldBootState == FBE_PEER_SUCCESS &&
        newBootState != FBE_PEER_SUCCESS)
    {
        fbe_event_log_write(ESP_ERROR_PEER_SP_DOWN, 
                            NULL, 
                            0, 
                            "%s",
                            decodeSpId(boardPeerBootInfo->peerSp));
    }
    if (oldBootState != FBE_PEER_SUCCESS &&
        newBootState == FBE_PEER_SUCCESS)
    {
        fbe_board_mgmt_fltReg_updateEnclFaultLed(board_mgmt, FBE_ENCL_FAULT_LED_PEER_SP_FLT);

        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)board_mgmt, 
                                                          FBE_CLASS_ID_BOARD_MGMT, 
                                                          FBE_DEVICE_TYPE_SP, 
                                                          FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                          &phys_location);    
    }
}
/********************************************
* end of fbe_board_mgmt_fltReg_logBootState()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_checkBiosPostFault()
 ****************************************************************
 * @brief
 *   This function tranverse all BIOS/POST fault values in fault register.
 *   Return bool value if any BIOS/POST fault is found.
 *   All FRUs' fault except Fan,PS,BBU are set by BIOS/POST, these 3 FRUs are set by BMC firmware
 *   Only Fan,PS,BBU are continously updated and logged during software runtime, in the meantime 
 *   their fault is set to fault register by BMC firmware.
 *
 * @param board_mgmt         - This handle to board mgmt object 
 *
 * @return fbe_bool_t: Bool value of new bios/post fault.
 *         
 * 
 * @author
 *  22-Mar-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_bool_t
fbe_board_mgmt_fltReg_checkBiosPostFault(fbe_board_mgmt_t             *board_mgmt,
                                         fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr)
{
    fbe_u32_t                           eachFault = 0;
    SPECL_FAULT_REGISTER_STATUS       * newRegStatus = NULL;
    fbe_board_mgmt_peer_boot_info_t   * peerBootInfo = NULL;

    newRegStatus = &fltRegInfoPtr->faultRegisterStatus;
    peerBootInfo = &board_mgmt->peerBootInfo;

    for (eachFault = 0; eachFault < MAX_DIMM_FAULT; eachFault++)
    {
        if(newRegStatus->dimmFault[eachFault])
        {
            return FBE_TRUE;
        }
    }
    
    for (eachFault = 0; eachFault < MAX_DRIVE_FAULT; eachFault++)
    {
        if(newRegStatus->driveFault[eachFault])
        {
            return FBE_TRUE;
        }
    }

    for (eachFault = 0; eachFault < MAX_SLIC_FAULT; eachFault++)
    {
        if(newRegStatus->slicFault[eachFault])
        {
            return FBE_TRUE;
        }
    }

    if(newRegStatus->superCapFault)
    {
        return FBE_TRUE;
    }

    // Do not check for I2C Bus Faults (if there is a problem, other faults will be generated
//    for (eachFault = 0; eachFault < MAX_I2C_FAULT; eachFault++)
//    {
//        if(newRegStatus->i2cFault[eachFault])
//        {
//            return FBE_TRUE;
//        }
//    }
    
    if(newRegStatus->cpuFault)
    {
        return FBE_TRUE;
    }

    if(newRegStatus->mgmtFault)
    {
        return FBE_TRUE;
    }

    if(newRegStatus->bemFault)
    {
        return FBE_TRUE;
    }

    if(newRegStatus->eFlashFault)
    {
        return FBE_TRUE;
    }
    if(newRegStatus->cacheCardFault)
    {
        return FBE_TRUE;
    }
    if(newRegStatus->midplaneFault)
    {
        return FBE_TRUE;
    }
    if(newRegStatus->cmiFault)
    {
        return FBE_TRUE;
    }
    if(newRegStatus->allFrusFault)
    {
        return FBE_TRUE;
    }
    if(newRegStatus->externalFruFault)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/********************************************
* end of fbe_board_mgmt_fltReg_checkBiosPostFault()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_checkNewBiosPostFault()
 ****************************************************************
 * @brief
 *   This function tranverse all BIOS/POST fault values in fault register.
 *   Return bool value if any BIOS/POST new fault is found.
 *
 * @param board_mgmt         - This handle to board mgmt object 
 *
 * @return fbe_bool_t: Bool value of new bios/post fault.
 *         
 * 
 * @author
 *  19-Mar-2013: Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_bool_t
fbe_board_mgmt_fltReg_checkNewBiosPostFault(fbe_board_mgmt_t             *board_mgmt,
                                         fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr)
{
    fbe_u32_t                           eachFault = 0;
    SPECL_FAULT_REGISTER_STATUS       * oldRegStatus = NULL;
    SPECL_FAULT_REGISTER_STATUS       * newRegStatus = NULL;
    fbe_board_mgmt_peer_boot_info_t   * peerBootInfo = NULL;
    const fbe_char_t                  * peerSpStr = NULL;
    fbe_bool_t                          oldFault = FBE_FALSE;
    fbe_bool_t                          newFault = FBE_FALSE;

    oldRegStatus = &board_mgmt->peerBootInfo.fltRegStatus;
    newRegStatus = &fltRegInfoPtr->faultRegisterStatus;
    peerBootInfo = &board_mgmt->peerBootInfo;
    peerSpStr = decodeSpId(peerBootInfo->peerSp);

    for (eachFault = 0; eachFault < MAX_DIMM_FAULT; eachFault++)
    {
        oldFault = oldRegStatus->dimmFault[eachFault];
        newFault = newRegStatus->dimmFault[eachFault];
        if(newFault)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: %s %s %d is fault(%s).\n",
                                  peerSpStr,
                                  fbe_pbl_decodeFru(PBL_FRU_DIMM),
                                  eachFault,
                                  (!oldFault)?"new":"old");
            if(!oldFault)
            {
                return FBE_TRUE;
            }
        }
    }
    
    for (eachFault = 0; eachFault < MAX_DRIVE_FAULT; eachFault++)
    {
        oldFault = oldRegStatus->driveFault[eachFault];
        newFault = newRegStatus->driveFault[eachFault];
        if(newFault)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: %s %s %d is fault(%s).\n",
                                  peerSpStr,
                                  fbe_pbl_decodeFru(PBL_FRU_DRIVE),
                                  eachFault,
                                  (!oldFault)?"new":"old");
            if(!oldFault)
            {
                return FBE_TRUE;
            }
        }
    }

    for (eachFault = 0; eachFault < MAX_SLIC_FAULT; eachFault++)
    {
        oldFault = oldRegStatus->slicFault[eachFault];
        newFault = newRegStatus->slicFault[eachFault];
        if(newFault)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: %s %s %d is fault(%s).\n",
                                  peerSpStr,
                                  fbe_pbl_decodeFru(PBL_FRU_SLIC),
                                  eachFault,
                                  (!oldFault)?"new":"old");
            if(!oldFault)
            {
                return FBE_TRUE;
            }
        }
    }

    oldFault = oldRegStatus->superCapFault;
    newFault = newRegStatus->superCapFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_SUPERCAP),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    // Do not check for I2C Bus Faults (if there is a problem, other faults will be generated
//    for (eachFault = 0; eachFault < MAX_I2C_FAULT; eachFault++)
//    {
//        if(!oldRegStatus->i2cFault[eachFault] && newRegStatus->i2cFault[eachFault])
//        {
//            return FBE_TRUE;
//        }
//    }
    
    oldFault = oldRegStatus->cpuFault;
    newFault = newRegStatus->cpuFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_CPU),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->mgmtFault;
    newFault = newRegStatus->mgmtFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_MGMT),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->bemFault;
    newFault = newRegStatus->bemFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_BM),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->eFlashFault;
    newFault = newRegStatus->eFlashFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_EFLASH),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }
    if(!oldRegStatus->cacheCardFault && newRegStatus->cacheCardFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_CACHECARD),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->midplaneFault;
    newFault = newRegStatus->midplaneFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_MIDPLANE),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->cmiFault;
    newFault = newRegStatus->cmiFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_CMI),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->allFrusFault;
    newFault = newRegStatus->allFrusFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_ALL_FRU),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    oldFault = oldRegStatus->externalFruFault;
    newFault = newRegStatus->externalFruFault;
    if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PBL: %s %s is fault(%s).\n",
                              peerSpStr,
                              fbe_pbl_decodeFru(PBL_FRU_EXTERNAL_FRU),
                              (!oldFault)?"new":"old");
        if(!oldFault)
        {
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}
/********************************************
* end of fbe_board_mgmt_fltReg_checkNewBiosPostFault()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_checkNewFault()
 ****************************************************************
 * @brief
 *   This function tranverse all fault values in fault register.
 *   Set bool value if any new fault is found.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  16-Feb-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_bool_t
fbe_board_mgmt_fltReg_checkNewFault(fbe_board_mgmt_t             *board_mgmt,
                                    fbe_peer_boot_flt_exp_info_t *fltRegInfoPtr)
{
    fbe_u32_t                           eachFault = 0;
    SPECL_FAULT_REGISTER_STATUS       * oldRegStatus = NULL;
    SPECL_FAULT_REGISTER_STATUS       * newRegStatus = NULL;
    fbe_board_mgmt_peer_boot_info_t   * peerBootInfo = NULL;
    const fbe_char_t                  * peerSpStr = NULL;
    fbe_bool_t                          oldFault = FBE_FALSE;
    fbe_bool_t                          newFault = FBE_FALSE;

    oldRegStatus = &board_mgmt->peerBootInfo.fltRegStatus;
    newRegStatus = &fltRegInfoPtr->faultRegisterStatus;
    peerBootInfo = &board_mgmt->peerBootInfo;
    peerSpStr = decodeSpId(peerBootInfo->peerSp);

    if (!fltRegInfoPtr->faultRegisterStatus.anyFaultsFound)
    {
        return FBE_FALSE;
    }

    if (fbe_board_mgmt_fltReg_checkNewBiosPostFault(board_mgmt, fltRegInfoPtr))
    {
        return FBE_TRUE;
    }

    for (eachFault = 0; eachFault < MAX_POWER_FAULT; eachFault++)
    {
        oldFault = oldRegStatus->powerFault[eachFault];
        newFault = newRegStatus->powerFault[eachFault];
        if(newFault)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: %s %s %d is fault(%s).\n",
                                  peerSpStr,
                                  fbe_pbl_decodeFru(PBL_FRU_POWER),
                                  eachFault,
                                  (!oldFault)?"new":"old");
            if(!oldFault)
            {
                return FBE_TRUE;
            }
        }
    }



    for (eachFault = 0; eachFault < MAX_BATTERY_FAULT; eachFault++)
    {

        if(!oldRegStatus->batteryFault[eachFault] && newRegStatus->batteryFault[eachFault])
        {
            return FBE_TRUE;
        }
    }

    for (eachFault = 0; eachFault < MAX_FAN_FAULT; eachFault++)
    {
        oldFault = oldRegStatus->fanFault[eachFault];
        newFault = newRegStatus->fanFault[eachFault];
        if(newFault)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: %s %s %d is fault(%s).\n",
                                  peerSpStr,
                                  fbe_pbl_decodeFru(PBL_FRU_FAN),
                                  eachFault,
                                  (!oldFault)?"new":"old");
            if(!oldFault)
            {
                return FBE_TRUE;
            }
        }
    }

    return FBE_FALSE;
}
/********************************************
* end of fbe_board_mgmt_fltReg_checkNewFault()
********************************************/

/*!**************************************************************
 * fbe_board_mgmt_fltReg_logFault()
 ****************************************************************
 * @brief
 *   This function do 
 *   event log according to fault type. Various fault components
 *   are extraced from SPECL_FAULT_REGISTER_STATUS. Some of them
 *   are logged in relative managment module in ESP, and not logged
 *   here, such as slic fault is logged in module managment and 
 *   power supply is logged in ps managment.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  28-July-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static void
fbe_board_mgmt_fltReg_logFault(fbe_board_mgmt_t              *board_mgmt,
                               fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr)
{
    fbe_u32_t                       eachFault = 0;
    fbe_board_mgmt_peer_boot_info_t *boardPeerBootInfo = NULL;
    SPECL_FAULT_REGISTER_STATUS   * newRegStatus = NULL;
    const fbe_char_t              * peerSpStr = NULL;

    boardPeerBootInfo = &board_mgmt->peerBootInfo;
    newRegStatus = &fltRegInfoPtr->faultRegisterStatus;
    peerSpStr = decodeSpId(boardPeerBootInfo->peerSp);

    for (eachFault = 0; eachFault < MAX_DIMM_FAULT; eachFault++)
    {
        fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                                PBL_FRU_DIMM,
                                                eachFault,
                                                newRegStatus->dimmFault[eachFault]);
    }
    
    for (eachFault = 0; eachFault < MAX_DRIVE_FAULT; eachFault++)
    {
        fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                                PBL_FRU_DRIVE,
                                                eachFault,
                                                newRegStatus->driveFault[eachFault]);
    }

    for (eachFault = 0; eachFault < MAX_SLIC_FAULT; eachFault++)
    {
        fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                                PBL_FRU_SLIC,
                                                eachFault,
                                                newRegStatus->slicFault[eachFault]);
    }

    for (eachFault = 0; eachFault < MAX_POWER_FAULT; eachFault++)
    {
        fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                                PBL_FRU_POWER,
                                                eachFault,
                                                newRegStatus->powerFault[eachFault]);
    }


    for (eachFault = 0; eachFault < MAX_BATTERY_FAULT; eachFault++)
    {
        fbe_board_mgmt_peer_boot_log_fail_fru_slot_partnum(board_mgmt,
                                                           FBE_DEVICE_TYPE_BATTERY,
                                                           eachFault,
                                                           newRegStatus->batteryFault[eachFault],
                                                           fbe_pbl_decodeFru(PBL_FRU_BATTERY),
                                                           peerSpStr);
    }


    fbe_board_mgmt_peer_boot_log_fail_fru(newRegStatus->superCapFault,
                                          fbe_pbl_decodeFru(PBL_FRU_SUPERCAP),
                                          peerSpStr);



    for (eachFault = 0; eachFault < MAX_FAN_FAULT; eachFault++)
    {
        fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                                PBL_FRU_FAN,
                                                eachFault,
                                                newRegStatus->fanFault[eachFault]);
    }

    // check for I2C faults & only ktrace (no event generation)
    for (eachFault = 0; eachFault < MAX_I2C_FAULT; eachFault++)
    {
        if (newRegStatus->i2cFault[eachFault] && !boardPeerBootInfo->fltRegStatus.i2cFault[eachFault])
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s, PBL Detected I2C Bus %d Fault\n",
                                  __FUNCTION__, eachFault);
        }
    }
    
    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_CPU,
                                            0,
                                            newRegStatus->cpuFault);

    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_MGMT,
                                            0,
                                            newRegStatus->mgmtFault);

    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_BM,
                                            0,
                                            newRegStatus->bemFault);

    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_EFLASH,
                                            0,
                                            newRegStatus->eFlashFault);
    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_CACHECARD,
                                            0,
                                            newRegStatus->cacheCardFault);
    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_MIDPLANE,
                                            0,
                                            newRegStatus->midplaneFault);
    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_CMI,
                                            0,
                                            newRegStatus->cmiFault);
    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_ALL_FRU,
                                            0,
                                            newRegStatus->allFrusFault);
    fbe_board_mgmt_fltReg_logFaultByFruType(board_mgmt,
                                            PBL_FRU_EXTERNAL_FRU,
                                            0,
                                            newRegStatus->externalFruFault);
}
/********************************************
* end of fbe_board_mgmt_fltReg_logFault()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_clearPeerFault()
 ****************************************************************
 * @brief
 *   Since BIOS/POST only set fault in fault register(BMC VEEPROM)
 *   But doesn't clear them even the fault cleared. It causes fault
 *   persist in VEEPROM until power cycle. This function
 *   let ESP clear faults in peer fault register by calling SPECL API.
 *   The clear action occured after ESP reported peer boot log and
 *   peer BIOS/POST is end.It result in data mismatch between ESP
 *   object and VEEPROM and it's acceptable.
 *   After fault SP reboot, the data mismatch will disapear.
 *   (BIOS/POST only set but never read/check fault values)
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  11-Nov-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static void
fbe_board_mgmt_fltReg_clearPeerFault(fbe_board_mgmt_t              *board_mgmt,
                                     fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr)
{
    fbe_base_object_t             * base_object = (fbe_base_object_t *)board_mgmt;
    const fbe_char_t              * peerSpStr = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;

    /* Call Physical Package clear peer fault in VEEPROM */
    status = fbe_api_board_clear_flt_reg_fault(board_mgmt->object_id, fltRegInfoPtr);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Error in clearing the peer Fault Register fault, status: 0x%X\n",
                            __FUNCTION__, status);
        return;
    }

    /* Update data of clearance for debug */
    status = fbe_get_system_time(&board_mgmt->peerBootInfo.lastFltRegClearTime);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Error in getting system time, status: 0x%X\n",
                            __FUNCTION__, status);
        return;
    }
    
    /* Write Event Log */
    peerSpStr = decodeSpId(board_mgmt->peerBootInfo.peerSp);
    fbe_event_log_write(ESP_INFO_PEERSP_CLEAR, 
                        NULL, 
                        0, 
                        "%s", 
                        peerSpStr); 

}
/********************************************
* end of fbe_board_mgmt_fltReg_clearPeerFault()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_processBiosPostFault()
 ****************************************************************
 * @brief
 *  Porcess after peer fault is set by BIOS/POST.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  25-Dec-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static void
fbe_board_mgmt_fltReg_processBiosPostFault(fbe_board_mgmt_t              *board_mgmt,
                                           fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr)

{
    fbe_u32_t                           newCpuStatus;
    fbe_board_mgmt_peer_boot_info_t   * peerBootInfo = NULL;

    peerBootInfo = &board_mgmt->peerBootInfo;
    newCpuStatus = fltRegInfoPtr->faultRegisterStatus.cpuStatusRegister;

    if (fbe_board_mgmt_fltReg_checkBiosPostFault(board_mgmt, fltRegInfoPtr))
    {
        if (!peerBootInfo->biosPostFail)
        {
            peerBootInfo->biosPostFail = FBE_TRUE;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: Setting peer biosPostFail flag.\n");
        }
        /* Save last fault status reg while BIOS/POST fail happened. */
        if (newCpuStatus < CSR_OS_DEGRADED_MODE)
        {
            peerBootInfo->lastFltRegStatus = fltRegInfoPtr->faultRegisterStatus;
        }
    }
    else
    {
        /* No any BIOS/POST fault */
        if (peerBootInfo->biosPostFail)
        {
            peerBootInfo->biosPostFail = FBE_FALSE;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "PBL: Clearing peer biosPostFail flag with no fault.No need do clearance.\n");
        }
    }
    
    /* Do clear faults by the end of peer SP BIOS/POST process and during OS starting.
     * At that time, BIOS/POST is resumed(FRU fault fixed).But BIOS/POST doesn't clear the fault. 
     * It's ESP's responsibility to clear the fault in peer SP's FSR(Fault Status Register).
     */ 
    if (newCpuStatus == CSR_OS_APPLICATION_RUNNING)
    {
        if (peerBootInfo->biosPostFail)
        {
            fbe_board_mgmt_fltReg_clearPeerFault(board_mgmt, fltRegInfoPtr);
            
            board_mgmt->peerBootInfo.biosPostFail = FBE_FALSE;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "PBL: Clearing peer biosPostFail flag with OS booted.Do clearance.\n");
        }
    }
}
/********************************************
* end of fbe_board_mgmt_fltReg_processBiosPostFault()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_updateEnclFaultLed()
 ****************************************************************
 * @brief
 *  Update SP enclosure fault led based on peer fault status.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  26-Dec-2012: Chengkai Hu - Created
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_fltReg_updateEnclFaultLed(fbe_board_mgmt_t *board_mgmt,
                                         fbe_encl_fault_led_reason_t enclFaultLedReason )
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_device_physical_location_t    location = {0};
    
    location.sp = board_mgmt->peerBootInfo.peerSp;

    /* Check if Enclosure Fault LED needs updating  */
    status = fbe_board_mgmt_update_encl_fault_led(board_mgmt, &location, enclFaultLedReason);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:error updating EnclFaultLed, status 0x%X.\n",
                              __FUNCTION__,
                              status);
    }

    return status;
}
/********************************************
* end of fbe_board_mgmt_fltReg_updateEnclFaultLed()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_checkBootTimeOut()
 ****************************************************************
 * @brief
 *  Check if peer SP is timed out in booting and set it to HUNG status.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t
 * 
 * @author
 *  30-Jan-2013: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_bool_t
fbe_board_mgmt_fltReg_checkBootTimeOut(fbe_board_mgmt_t              *board_mgmt)
{
    fbe_u32_t                       cpuStatus;
    fbe_u32_t                       elapsedTime;//elapsed time from last cpu register state change
    fbe_device_physical_location_t  phys_location = {0};
    const fbe_char_t              * peerSpStr = NULL;
    fbe_bool_t                      isPeerTimeOut = FBE_FALSE;
    fbe_u32_t                       timeout = 0;

    phys_location.sp = (board_mgmt->peerBootInfo.peerSp);
    peerSpStr = decodeSpId(board_mgmt->peerBootInfo.peerSp);
    cpuStatus = board_mgmt->peerBootInfo.fltRegStatus.cpuStatusRegister;

    /* Get timeout value by current CPU status code */
    timeout = fbe_pbl_getPeerBootTimeout(&board_mgmt->peerBootInfo.fltRegStatus);
    /* Calculate elapsed time from last status change */
    elapsedTime = fbe_get_elapsed_seconds(board_mgmt->peerBootInfo.StateChangeStartTime);
    
    if(timeout != 0 && elapsedTime > timeout)
    {
        isPeerTimeOut = FBE_TRUE;
        board_mgmt->peerBootInfo.peerBootState = FBE_PEER_HUNG;

        /* Do event log for boot state*/
        fbe_board_mgmt_fltReg_logHungState(board_mgmt);

        /* Light enclosure fault led while peer SP is hung */        
        fbe_board_mgmt_fltReg_updateEnclFaultLed(board_mgmt, FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT);

        /* Send the notification for state change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                            FBE_CLASS_ID_BOARD_MGMT,
                                                            FBE_DEVICE_TYPE_FLT_REG,
                                                            FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                            &phys_location);

        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                  FBE_TRACE_LEVEL_INFO,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "PBL: Set peer(%s) to HUNG state.Timeout:%d sec.Elapsed time:%d.Status:%s(0x%x)\n", 
                  peerSpStr,
                  timeout,
                  elapsedTime,
                  decodeCpuStatusRegister(cpuStatus),
                  cpuStatus); 
    }
    else  
    {
        isPeerTimeOut = FBE_FALSE;
    }

    return isPeerTimeOut;
}
/********************************************
* end of fbe_board_mgmt_fltReg_checkBootTimeOut()
********************************************/
/*!**************************************************************
 * fbe_board_mgmt_fltReg_statusChange()
 ****************************************************************
 * @brief
 *   This function copy fault register info to baord_mgmt and do 
 *   logging and other operations with fault register.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *        fltRegInfoPtr- fault register handle
 *
 * @return fbe_status_t
 * 
 * @author
 *  12-Nov-2012: Chengkai Hu - Created
 *  16-Feb-2013: Chengkai Hu - Add check of new fault before log report.
 *
 ****************************************************************/
fbe_status_t
fbe_board_mgmt_fltReg_statusChange(fbe_board_mgmt_t              *board_mgmt,
                                   fbe_peer_boot_flt_exp_info_t  *fltRegInfoPtr)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    
    /* Do event log for boot state */
    fbe_board_mgmt_fltReg_logBootState(board_mgmt, fltRegInfoPtr);
    
    /* Do event log for fault values while new fault detected */
    if (fbe_board_mgmt_fltReg_checkNewFault(board_mgmt, fltRegInfoPtr))
    {
        fbe_board_mgmt_fltReg_logFault(board_mgmt, fltRegInfoPtr);
    }
    
    /* Process peer fault reported by BIOS or POST */
    fbe_board_mgmt_fltReg_processBiosPostFault(board_mgmt, fltRegInfoPtr);

    /* Update peer boot info in board mgmt */
    fbe_board_mgmt_setPeerBootInfo(board_mgmt, fltRegInfoPtr);
    
    /* Check peer fault and update SP enclosure fault LED */
    status = fbe_board_mgmt_fltReg_updateEnclFaultLed(board_mgmt, FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT);

    return status;
}
/********************************************
* end of fbe_board_mgmt_fltReg_statusChange()
********************************************/
/*END: Fault Register part */

/*START: Slave Port part */
/*!**************************************************************
 * fbe_board_mgmt_slavePortStatusChange()
 ****************************************************************
 * @brief
 *   This function copy extended peer boot software status info from 
 *   Slave Port and do event log according to status type. 
 *
 * @param board_mgmt        - This handle to board mgmt object 
 *        slavePortInfoPtr  - Slave Port handle
 *
 * @return fbe_status_t
 * 
 * @author
 *  04-Sep-2012: Chengkai Hu - Created
 *
 ****************************************************************/
fbe_status_t
fbe_board_mgmt_slavePortStatusChange(fbe_board_mgmt_t                  *board_mgmt,
                                     fbe_board_mgmt_slave_port_info_t  *slavePortInfoPtr)
{
    fbe_u8_t                        lastGenStatus       = 0;
    fbe_u8_t                        currGenStatus       = 0;
    fbe_u8_t                        lastCompStatus      = 0;
    fbe_u8_t                        currCompStatus      = 0;
    fbe_u8_t                        lastCompExtStatus   = 0;
    fbe_u8_t                        currCompExtStatus   = 0;
    fbe_u16_t                       extStatusCode       = 0;
    fbe_board_mgmt_ext_peer_boot_info_t * boardExtPeerBootInfo = NULL;
    const fbe_char_t                    * peerSpStr = NULL;

    currGenStatus = slavePortInfoPtr->generalStatus;
    currCompStatus = slavePortInfoPtr->componentStatus;
    currCompExtStatus = slavePortInfoPtr->componentExtStatus;

    boardExtPeerBootInfo = &board_mgmt->extPeerBootInfo;
    lastGenStatus = boardExtPeerBootInfo->generalStatus;
    lastCompStatus = boardExtPeerBootInfo->componentStatus;
    lastCompExtStatus = boardExtPeerBootInfo->componentExtStatus;

    if (currCompStatus != lastCompStatus || currCompExtStatus != lastCompExtStatus)
    {
        /* Copy fault register status to board mgmt */
        fbe_board_mgmt_setExtPeerBootInfo(board_mgmt, slavePortInfoPtr);

        /* Do not log POST state, only trace it. */
        peerSpStr = decodeSpId(boardExtPeerBootInfo->peerSp);
        if ((((board_mgmt->platform_info.familyType == TRANSFORMERS_FAMILY) ||
              (board_mgmt->platform_info.familyType == MOONS_FAMILY)) &&
              ((currGenStatus == 0xC0) ||(currGenStatus == 0xCE))) ||
            (((board_mgmt->platform_info.familyType != TRANSFORMERS_FAMILY) &&
              (board_mgmt->platform_info.familyType != MOONS_FAMILY)) &&
             ((currGenStatus == 0x41) ||(currGenStatus == 0x61))))
        {
            extStatusCode =  currCompStatus;
            extStatusCode |= (currCompExtStatus << 8);
        
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "EPBL:%s 0x%x(%s), Component:0x%x(%s), Status:0x%x(%s).",
                                peerSpStr,
                                currGenStatus,
                                ((board_mgmt->platform_info.familyType == TRANSFORMERS_FAMILY) ||
                                 (board_mgmt->platform_info.familyType == MOONS_FAMILY)) ?
                                  decodeAmiPort80Status(currGenStatus) : decodePort80Status(currGenStatus),
                                SW_COMPONENT_POST,
                                decodeSoftwareComponentStatus(SW_COMPONENT_POST),
                                extStatusCode,
                                decodePOSTExtStatus(extStatusCode)); 
        }
        /* Log event for core software boot status, and also trace it. */
        else if (currGenStatus == 0x0)
        {
            fbe_event_log_write(ESP_INFO_EXT_PEER_BOOT_STATUS_CHANGE, 
                                NULL, 
                                0, 
                                "%s %s %s %x", 
                                peerSpStr,
                                decodeSoftwareComponentStatus(currCompStatus),
                                decodeSoftwareComponentExtStatus(currCompExtStatus),
                                currCompExtStatus);
            
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "EPBL:%s 0x%x(%s), Component:0x%x(%s), Status:0x%x(%s).",
                                peerSpStr,
                                currGenStatus,
                                ((board_mgmt->platform_info.familyType == TRANSFORMERS_FAMILY) ||
                                 (board_mgmt->platform_info.familyType == MOONS_FAMILY)) ?
                                  decodeAmiPort80Status(currGenStatus) : decodePort80Status(currGenStatus),
                                currCompStatus,
                                decodeSoftwareComponentStatus(currCompStatus),
                                currCompExtStatus,
                                decodeSoftwareComponentExtStatus(currCompExtStatus)); 
        }
    }

    return FBE_STATUS_OK;
}
/********************************************
* end of fbe_board_mgmt_slavePortStatusChange()
********************************************/
/*END: Slave Port part */

