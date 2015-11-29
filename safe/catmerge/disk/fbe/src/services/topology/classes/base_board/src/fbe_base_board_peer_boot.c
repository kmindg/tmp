/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_board_peer_boot.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the function related to peer boot fault detection functionality
 *  Most of it is moved from board_mgmt/fbe_board_mgmt_peer_boot.c
 *
 * @ingroup base_board_class_files
 * 
 * @version
 *  23-Apr-2012: Created  Chengkai Hu
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "base_board_private.h"
#include "generic_utils_lib.h"
#include "edal_processor_enclosure_data.h"
#include "fbe_pe_types.h"

/*--- local function prototypes --------------------------------------------------------*/
static fbe_bool_t fbe_base_board_validate_component(fbe_base_board_t               * pBaseBoard, 
                                                    fbe_enclosure_component_types_t  componentType,
                                                    fbe_u32_t                        slotNum);
static fbe_status_t fbe_base_board_peerBoot_updateEdal(fbe_base_board_t               * pBaseBoard, 
                                                       fbe_enclosure_component_types_t  componentType,
                                                       fbe_u32_t                        slotNum,
                                                       fbe_bool_t                       isFaultRegFail);

/* START: Fault expander stuff */
static fbe_peer_boot_states_t getBootCodeState(fbe_u8_t bootCode);
static fbe_bool_t validateBootCode(fbe_u8_t bootCode);
static fbe_u32_t getBootCodeSubFruCount(fbe_u8_t bootCode);
static fbe_subfru_replacements_t getBootCodeSubFruReplacements(fbe_u8_t bootCode, fbe_u32_t num);
/* END: Fault expander stuff */
/*--- local function prototypes --------------------------------------------------------*/

/* START: Fault expander stuff */
/*!**************************************************************
 * getBootCodeState()
 ****************************************************************
 * @brief
 *   This function return boot code state.
 *
 * @param bootCode - Boot code that received from Fault Expander 
 *
 * @return fbe_peer_boot_states_t - Boot state
 * 
 * @author
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *  20-July-2012: Chengkai Hu, Moved from fbe_board_mgmt_peer_boot.c 
 ****************************************************************/
static fbe_peer_boot_states_t getBootCodeState(fbe_u8_t bootCode)
{
    if(bootCode == FBE_PEER_BOOT_CODE_LAST)
    {
        return(fbe_PeerBootEntries[FBE_PEER_BOOT_CODE_LIMIT1+1].state);
    }
    return(fbe_PeerBootEntries[bootCode].state);
}

/************************************************
* end of getBootCodeState()
************************************************/
/*!**************************************************************
 * validateBootCode()
 ****************************************************************
 * @brief
 *   This function verify whether the boot code and it state
 *   within valid range.
 *
 * @param bootCode - Boot code that received from Fault Expander 
 *
 * @return fbe_bool_t - TRUE if boot code valid
 *         otherwise FALSE
 * 
 * @author
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *  20-July-2012: Chengkai Hu, Moved from fbe_board_mgmt_peer_boot.c 
 ****************************************************************/
static fbe_bool_t
validateBootCode(fbe_u8_t bootCode)
{
    if(((bootCode <= FBE_PEER_BOOT_CODE_LIMIT1) || (bootCode == FBE_PEER_BOOT_CODE_LAST))
        && (getBootCodeState(bootCode) != FBE_PEER_INVALID_CODE))
    {
        return TRUE;
    }
    return FALSE;
}
/********************************************
* end of validateBootCode()
********************************************/
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
 *  23-July-2012: Chengkai Hu, Moved from fbe_board_mgmt_peer_boot.c 
 ****************************************************************/
static fbe_u32_t 
getBootCodeSubFruCount(fbe_u8_t bootCode)
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
 * getBootCodeSubFruReplacements()
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
 *  23-July-2012: Chengkai Hu, Moved from fbe_board_mgmt_peer_boot.c 
 ****************************************************************/
static fbe_subfru_replacements_t
getBootCodeSubFruReplacements(fbe_u8_t bootCode, fbe_u32_t num)
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
* end of getBootCodeSubFruReplacements()
*******************************************************/

/*!**************************************************************
 * fbe_base_board_validate_component()
 ****************************************************************
 * @brief
 *   This function check for specific type of component is valid
 *
 * @param pBaseBoard - handle to base_board object
 *
 * @return fbe_bool_t - true: component is valid
 *                      falise: component is invalid
 * @author
 *  05-Aug-2012: Created  Chengkai Hu
 *
 ****************************************************************/
static fbe_bool_t
fbe_base_board_validate_component(fbe_base_board_t               * pBaseBoard, 
                                   fbe_enclosure_component_types_t  componentType,
                                   fbe_u32_t                        slotNum)
{
    fbe_edal_status_t                   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u32_t                           componentCount = 0;
    fbe_edal_block_handle_t             edalBlockHandle;


    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    /* Get component count */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     componentType, 
                                                     &componentCount);
    if(!EDAL_STAT_OK(edalStatus))
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "PBL: %s, Failed get component count\n", 
                              __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(slotNum < componentCount/SP_ID_MAX)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/***********************************************
* end of fbe_base_board_validate_component()
***********************************************/
/*!**************************************************************
 * fbe_base_board_peerBoot_updateEdal()
 ****************************************************************
 * @brief
 *   This function check for specific type of component and update 
 *      EDAL entry accordingly
 *
 * @param pBaseBoard - handle to base_board object
 *        componentType
 *        slotNum - number of slot where component locate on blade
 *        isFaultRegFail - bool flag indicate component fault detected
 *                         in fault register 
 *
 * @return fbe_status_t
 * 
 * @author
 *  07-Jun-2012: Created  Chengkai Hu
 *
 ****************************************************************/
static fbe_status_t
fbe_base_board_peerBoot_updateEdal(fbe_base_board_t               * pBaseBoard, 
                                   fbe_enclosure_component_types_t  componentType,
                                   fbe_u32_t                        slotNum,
                                   fbe_bool_t                       isFaultRegFail)
{
    fbe_edal_status_t                   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_u32_t                           componentCount = 0, componentIndex = 0;
    fbe_edal_block_handle_t             edalBlockHandle;
    fbe_bool_t                          newFault = FBE_FALSE;
    fbe_bool_t                          clearFault = FBE_FALSE;
    edal_pe_io_comp_sub_info_t          peIoModuleSubInfo = {0}; 
    fbe_board_mgmt_io_comp_info_t     * pExtIoModuleInfo = NULL;
    edal_pe_mgmt_module_sub_info_t      peMgmtModuleSubInfo = {0}; 
    fbe_board_mgmt_mgmt_module_info_t * pExtMgmtModuleInfo = NULL;   
    edal_pe_power_supply_sub_info_t     pePowerSupplySubInfo = {0};       
    fbe_power_supply_info_t           * pExtPowerSupplyInfo = NULL;
    edal_pe_cooling_sub_info_t          peCoolingSubInfo = {0};       
    fbe_cooling_info_t                * pExtCoolingInfo = NULL;
    edal_pe_battery_sub_info_t          peBatterySubInfo = {0};
    fbe_base_battery_info_t           * pExtBatteryInfo = NULL; 
    edal_pe_suitcase_sub_info_t         peSuitcaseSubInfo = {0}; 
    fbe_board_mgmt_suitcase_info_t    * pExtSuitcaseInfo = NULL; 
    edal_pe_dimm_sub_info_t             peDimmSubInfo = {0};     
    fbe_board_mgmt_dimm_info_t        * pExtDimmInfo = NULL;
    fbe_board_mgmt_ssd_info_t           SSDInfo = {0};
    fbe_board_mgmt_ssd_info_t          *pSSDInfo = &SSDInfo; 

    fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s, entry \n", 
                                __FUNCTION__);

    fbe_base_board_get_edal_block_handle(pBaseBoard, &edalBlockHandle);

    /* Get component count */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     componentType, 
                                                     &componentCount);

    if (componentType == FBE_ENCL_SSD) {
    
    fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, componentType:%s(%d)  Count:%d Status:0x%x\n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, componentCount, edalStatus);
    }

    /* Skip non exist component */
    if(!EDAL_STAT_OK(edalStatus) || 0 == componentCount)
    {
        if (componentType == FBE_ENCL_SSD)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "PBL: %s, Non Existant Component - componentType:%s(%d)\n", 
                                    __FUNCTION__, enclosure_access_printComponentType(componentType),
                                    componentType);
        }
        return FBE_STATUS_OK;  
    }    

    // special handling for Dimms since they are the peer and being reported via suitcase
    if ((componentType == FBE_ENCL_DIMM) ||
        (componentType == FBE_ENCL_SSD))
    {
        componentIndex = slotNum;

        // check only faults for valid components
        if(slotNum >= componentCount)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Non Existant Component - componentType:%s(%d)\n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType);
            return FBE_STATUS_OK;
        }
    }
    else 
    {
    /* Check if fault component index/slot in fault register is valid */
        if(slotNum >= componentCount/SP_ID_MAX)
        {
            if (componentType == FBE_ENCL_SSD)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "PBL: %s, Invalid Slot - componentType:%s(%d), slotNum:%d, count/SP:%d \n", 
                                    __FUNCTION__, enclosure_access_printComponentType(componentType),
                                    componentType, slotNum, componentCount/SP_ID_MAX);
            }
            return FBE_STATUS_OK;
        }

        /* Find component on peer SP blade */

        componentIndex = (pBaseBoard->spid == SP_A) ? (slotNum + (componentCount/SP_ID_MAX)):slotNum;
    }

    switch(componentType)
    {
        case FBE_ENCL_IO_MODULE:
        case FBE_ENCL_BEM:
        case FBE_ENCL_MEZZANINE:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peIoModuleSubInfo);  // buffer pointer

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }
            pExtIoModuleInfo = &peIoModuleSubInfo.externalIoCompInfo;
            
            /* Check if component is in peer SP */
            if (pBaseBoard->spid == pExtIoModuleInfo->associatedSp)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Invalid side - componentType:%s(%d), slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex);
                return FBE_STATUS_INVALID;  
            }

            /* Update peer boot fault status */
            if(!pExtIoModuleInfo->isFaultRegFail && isFaultRegFail)
            {
                newFault = FBE_TRUE;
            }
            else if(pExtIoModuleInfo->isFaultRegFail && !isFaultRegFail)
            {
                clearFault = FBE_TRUE;
            }
            pExtIoModuleInfo->isFaultRegFail= isFaultRegFail;

            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_IO_COMP_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_io_comp_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peIoModuleSubInfo);  // buffer pointer

        break;

        case FBE_ENCL_MGMT_MODULE:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peMgmtModuleSubInfo);  // buffer pointer
            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }    
            pExtMgmtModuleInfo = &peMgmtModuleSubInfo.externalMgmtModuleInfo;
             
            /* Check if component is in peer SP */
            if (pBaseBoard->spid == pExtMgmtModuleInfo->associatedSp)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Invalid side - componentType:%s(%d), slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex); 
                return FBE_STATUS_INVALID;  
            }
            /* Update peer boot fault status */
            if(!pExtMgmtModuleInfo->isFaultRegFail && isFaultRegFail)
            {
                newFault = FBE_TRUE;
            }
            else if(pExtMgmtModuleInfo->isFaultRegFail && !isFaultRegFail)
            {
                clearFault = FBE_TRUE;
            }
            pExtMgmtModuleInfo->isFaultRegFail = isFaultRegFail;
            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_MGMT_MODULE_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_mgmt_module_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peMgmtModuleSubInfo);  // buffer pointer

        break;
        
        case FBE_ENCL_POWER_SUPPLY:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_POWER_SUPPLY_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_power_supply_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&pePowerSupplySubInfo);  // buffer pointer
            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }    
            pExtPowerSupplyInfo = &pePowerSupplySubInfo.externalPowerSupplyInfo;
            /* Check if component is in peer SP */
            if (pBaseBoard->spid == pExtPowerSupplyInfo->associatedSp)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Invalid side - componentType:%s(%d), slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex); 
                return FBE_STATUS_INVALID;  
            }
            /* Update peer boot fault status */
            if(!pExtPowerSupplyInfo->isFaultRegFail && isFaultRegFail)
            {
                newFault = FBE_TRUE;
            }
            else if(pExtPowerSupplyInfo->isFaultRegFail && !isFaultRegFail)
            {
                clearFault = FBE_TRUE;
            }
            pExtPowerSupplyInfo->isFaultRegFail = isFaultRegFail;
            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_POWER_SUPPLY_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_power_supply_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&pePowerSupplySubInfo);  // buffer pointer
        break;

        case FBE_ENCL_COOLING_COMPONENT:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peCoolingSubInfo);  // buffer pointer
            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }    
            pExtCoolingInfo = &peCoolingSubInfo.externalCoolingInfo;
            
            /* Check if component is in peer SP */
            if (pBaseBoard->spid == pExtCoolingInfo->associatedSp)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Invalid side - componentType:%s(%d), slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex); 
                return FBE_STATUS_INVALID;  
            }
            /* Update peer boot fault status */
            if(!pExtCoolingInfo->isFaultRegFail && isFaultRegFail)
            {
                newFault = FBE_TRUE;
            }
            else if(pExtCoolingInfo->isFaultRegFail && !isFaultRegFail)
            {
                clearFault = FBE_TRUE;
            }
            pExtCoolingInfo->isFaultRegFail = isFaultRegFail;
            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_COOLING_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_cooling_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peCoolingSubInfo);  // buffer pointer

        break;

        case FBE_ENCL_BATTERY:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_BATTERY_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_battery_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peBatterySubInfo);  // buffer pointer
            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }    
            pExtBatteryInfo = &peBatterySubInfo.externalBatteryInfo;
            
            /* Check if component is in peer SP */
            if (pBaseBoard->spid == pExtBatteryInfo->associatedSp)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Invalid side - componentType:%s(%d), slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex); 
                return FBE_STATUS_INVALID;  
            }
            /* Update peer boot fault status */
            if(!pExtBatteryInfo->isFaultRegFail && isFaultRegFail)
            {
                newFault = FBE_TRUE;
            }
            else if(pExtBatteryInfo->isFaultRegFail && !isFaultRegFail)
            {
                clearFault = FBE_TRUE;
            }

            pExtBatteryInfo->isFaultRegFail = isFaultRegFail;
            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_BATTERY_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_battery_sub_info_t),  // buffer length
                                            (fbe_u8_t *)&peBatterySubInfo);  // buffer pointer
        break;
       
    case FBE_ENCL_DIMM:

            // DIMM faults are being reported through the suitcase
            componentType = FBE_ENCL_DIMM;

            /* We only have the EDAL for local DIMM info. So we just use the peerFaultRegFault field of the local DIMM to 
             * save the fault reported for the peer DIMM
             */
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_DIMM_INFO,          //Attribute 
                                            componentType,                  // component type
                                            componentIndex,                 // component index
                                            sizeof(edal_pe_dimm_sub_info_t),  // buffer length 
                                            (fbe_u8_t *)&peDimmSubInfo);    // buffer pointer 

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }

            pExtDimmInfo = &peDimmSubInfo.externalDimmInfo; 
            
            /* Update peer boot fault status */
            if(!pExtDimmInfo->peerFaultRegFault && isFaultRegFail) 
            {
                newFault = FBE_TRUE;
            }
            else if(pExtDimmInfo->peerFaultRegFault && !isFaultRegFail) 
            {
                clearFault = FBE_TRUE;
            }

            pExtDimmInfo->peerFaultRegFault = isFaultRegFail;

            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_DIMM_INFO,          //Attribute 
                                            componentType,                  // component type
                                            componentIndex,                 // component index
                                            sizeof(edal_pe_dimm_sub_info_t),  // buffer length 
                                            (fbe_u8_t *)&peDimmSubInfo);    // buffer pointer 
        break;

        case FBE_ENCL_SUITCASE:
            edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_SUITCASE_INFO,  //Attribute 
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_suitcase_sub_info_t),  // buffer length 
                                            (fbe_u8_t *)&peSuitcaseSubInfo);  // buffer pointer 

            if(!EDAL_STAT_OK(edalStatus))
            {
                return FBE_STATUS_OK;  
            }

            pExtSuitcaseInfo = &peSuitcaseSubInfo.externalSuitcaseInfo; 

            /* Check if component is in peer SP */
            if (pBaseBoard->spid == pExtSuitcaseInfo->associatedSp) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, Invalid side - componentType:%s(%d), slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex);
                return FBE_STATUS_INVALID;  
            }

            /* Update peer boot fault status */
            if(!pExtSuitcaseInfo->isCPUFaultRegFail && isFaultRegFail) 
            {
                newFault = FBE_TRUE;
            }
            else if(pExtSuitcaseInfo->isCPUFaultRegFail && !isFaultRegFail) 
            {
                clearFault = FBE_TRUE;
            }

            pExtSuitcaseInfo->isCPUFaultRegFail= isFaultRegFail; 

            /* Update EDAL with new info. */
            edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                            FBE_ENCL_PE_SUITCASE_INFO,  //Attribute
                                            componentType,  // component type
                                            componentIndex,  // component index
                                            sizeof(edal_pe_suitcase_sub_info_t),  // buffer length 
                                            (fbe_u8_t *)&peSuitcaseSubInfo);  // buffer pointer 

        break;

    case FBE_ENCL_SSD:

        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, SSD FLT Status Change, slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, slotNum, componentIndex);
        edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SSD_INFO,  //Attribute 
                                        componentType,  // component type
                                        componentIndex,  // component index
                                        sizeof(fbe_board_mgmt_ssd_info_t),  // buffer length 
                                        (fbe_u8_t *)pSSDInfo);  // buffer pointer 
        if(!EDAL_STAT_OK(edalStatus))
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "PBL: %s, SSD BAD EDAL STAT:0x%x slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, edalStatus, slotNum, componentIndex);
            return FBE_STATUS_OK;  
        }

        if (componentIndex != 0) 
        {
            // Just one ssd slot for now only use this one
            return FBE_STATUS_OK;
        }
        
        /* Update peer boot fault status */
        if(!pSSDInfo->isPeerFaultRegFail && isFaultRegFail) 
        {
            newFault = FBE_TRUE;
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, componentType:%s(%d), slotNum:%d, componentIndex:%d New SSD Fault \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex);
        }
        else if(pSSDInfo->isPeerFaultRegFail && !isFaultRegFail) 
        {
            clearFault = FBE_TRUE;
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, componentType:%s(%d), slotNum:%d, componentIndex:%d Clear SSD Fault \n", 
                                __FUNCTION__, enclosure_access_printComponentType(componentType),
                                componentType, slotNum, componentIndex);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PBL: %s, SSD No Change isPeerFltRegFail:%d isFltRegFail:%d slotNum:%d, componentIndex:%d \n", 
                                __FUNCTION__, pSSDInfo->isPeerFaultRegFail, isFaultRegFail, slotNum, componentIndex);
        }

        pSSDInfo->isPeerFaultRegFail= isFaultRegFail;

        /* Update EDAL with new info. */
        edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_SSD_INFO,  //Attribute 
                                        componentType,  // component type
                                        componentIndex,  // component index
                                        sizeof(fbe_board_mgmt_ssd_info_t),  // buffer length 
                                        (fbe_u8_t *)pSSDInfo);  // buffer pointer 

        if (clearFault || newFault) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL: Setting/Clearing SSD Fault so Updating state.\n"); 
            edalStatus = fbe_edal_setBool(edalBlockHandle, FBE_ENCL_COMP_STATE_CHANGE, FBE_ENCL_SSD, componentIndex, TRUE); 

            if (!EDAL_STAT_OK(edalStatus)) {
                fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "PBL: Changing SSD State failed EdalStat:0x%x \n", 
                                  edalStatus);
            }


        }


        default:
        break;
    }//switch(componentType)
    
    if(!EDAL_STAT_OK(edalStatus))
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "PBL: Update EDAL failed. type:%s(%d), slot:%d, fail:%d, index:%d \n", 
                              enclosure_access_printComponentType(componentType),componentType,
                              slotNum, isFaultRegFail, componentIndex); 
        return FBE_STATUS_GENERIC_FAILURE;  
    }
    else if(newFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL: New fault updated in EDAL. %s(%d), slot:%d, index:%d \n", 
                              enclosure_access_printComponentType(componentType),componentType,
                              slotNum, componentIndex); 
    }
    else if(clearFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL: Fault cleared in EDAL. %s(%d), slot:%d, index:%d \n", 
                              enclosure_access_printComponentType(componentType),componentType,
                              slotNum, componentIndex); 
    }

    return FBE_STATUS_OK;
}
/***********************************************
* end of fbe_base_board_peerBoot_updateEdal()
***********************************************/

/*!**************************************************************
 * fbe_base_board_peerBoot_fltReg_checkFru()
 ****************************************************************
 * @brief
 *   This function check for fault register peer boot code and update IO module
 *      EDAL entry accordingly
 *
 * @param bootCode - handle to base_board object
 *
 * @return 
 * 
 * @author
 *  25-Jun-2012: Created  Chengkai Hu
 *
 ****************************************************************/
fbe_status_t
fbe_base_board_peerBoot_fltReg_checkFru(fbe_base_board_t *pBaseBoard, 
                                        fbe_peer_boot_flt_exp_info_t  *fltexpInfoPtr)
{
    fbe_status_t                    status;               
    PSPECL_FAULT_REGISTER_STATUS    fltRegStatusPtr;
    fbe_u32_t                       eachFault = 0;

    fltRegStatusPtr = &fltexpInfoPtr->faultRegisterStatus;
    if (!fltRegStatusPtr->anyFaultsFound)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL: %s, No fault in fault register.(RegId:%d)\n", 
                              __FUNCTION__, fltexpInfoPtr->fltExpID); 
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseBoard,  
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL: %s, Fault found in fault register.(RegId:%d)\n", 
                              __FUNCTION__, fltexpInfoPtr->fltExpID); 
    }

    /* Fan,PS,BBU are set by BMC */
    for(eachFault = 0;eachFault < MAX_POWER_FAULT;eachFault++)
    {
        status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                    FBE_ENCL_POWER_SUPPLY,
                                                    eachFault,
                                                    fltRegStatusPtr->powerFault[eachFault]);
    }

    for(eachFault = 0;eachFault < MAX_BATTERY_FAULT;eachFault++)
    {
        status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                    FBE_ENCL_BATTERY,
                                                    eachFault,
                                                    fltRegStatusPtr->batteryFault[eachFault]);
    }

    for(eachFault = 0;eachFault < MAX_FAN_FAULT;eachFault++)
    {
        status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                    FBE_ENCL_COOLING_COMPONENT,
                                                    eachFault,
                                                    fltRegStatusPtr->fanFault[eachFault]);
    }

    for (eachFault = 0;eachFault < MAX_DRIVE_FAULT; eachFault++) 
    {
        status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                    FBE_ENCL_SSD,
                                                    eachFault,
                                                    fltRegStatusPtr->driveFault[eachFault]);
    }
    /* Other faults are set by BIOS/POST */
    for(eachFault = 0;eachFault < MAX_SLIC_FAULT;eachFault++)
    {
        status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                    FBE_ENCL_IO_MODULE,
                                                    eachFault,
                                                    fltRegStatusPtr->slicFault[eachFault]);
    }
   
    for(eachFault = 0;eachFault < MAX_DIMM_FAULT;eachFault++)
    {
        status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                    FBE_ENCL_DIMM,
                                                    eachFault,
                                                    fltRegStatusPtr->dimmFault[eachFault]);
    }

    status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                FBE_ENCL_MGMT_MODULE,
                                                0,
                                                fltRegStatusPtr->mgmtFault);

    status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                FBE_ENCL_BEM,
                                                0,
                                                fltRegStatusPtr->bemFault);

    status = fbe_base_board_peerBoot_updateEdal(pBaseBoard,
                                                FBE_ENCL_SUITCASE,
                                                0,
                                                fltRegStatusPtr->cpuFault);

    return status;
}
/***********************************************
* end of fbe_base_board_peerBoot_fltReg_checkFru()
***********************************************/

/*!*************************************************************************
 *   @fn fbe_base_board_pe_get_fault_reg_status(fbe_base_board_t *base_board)     
 **************************************************************************
 *  @brief
 *      This function returns the fault register status
 *
 *  @param base_board - board object context
 *
 *  @return fbe_u8_t - fault register status code.
 *
 *  @version
 *    28-July-2012: Chengkai Hu - Created.
 *    29-May-2012: PHE - Looped through all the fault Registers to find the target fault register.
 *
 **************************************************************************/
fbe_u32_t fbe_base_board_pe_get_fault_reg_status(fbe_base_board_t *base_board)
{
    edal_pe_flt_exp_sub_info_t PeFltExpSubInfo;
    fbe_u32_t component_count = 0;
    fbe_u32_t component_index = 0;
    fbe_edal_status_t edal_status;
    fbe_edal_block_handle_t edal_block_handle = base_board->edal_block_handle;
    FLT_REG_IDS fltExpID = PRIMARY_FLT_REG;
    SP_ID associatedSp = SP_ID_MAX;

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                    FBE_ENCL_FLT_REG, 
                    &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Can not get fltReg cnt, edal_status %d.\n",
                        __FUNCTION__, edal_status);
    
        return FBE_PEER_STATUS_UNKNOW;
    }    

    if(component_count == 0)
    {
        return FBE_PEER_STATUS_UNKNOW;
    }

    /* Only check the Peer primary fault register  */
    associatedSp = ( base_board->spid == SP_A) ? SP_B : SP_A;
    fltExpID = PRIMARY_FLT_REG;

    for(component_index = 0; component_index < component_count; component_index++)
    {
        edal_status = fbe_edal_getBuffer(edal_block_handle,
                                        FBE_ENCL_PE_FLT_REG_INFO,  //Attribute
                                        FBE_ENCL_FLT_REG,  // component type
                                        component_index,  // component index
                                        sizeof(edal_pe_flt_exp_sub_info_t),  // buffer length
                                        (fbe_u8_t *)&PeFltExpSubInfo);  // buffer pointer

        if(edal_status != FBE_EDAL_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, compIndex %d, Can not get fltReg Info.\n",
                            __FUNCTION__, component_index);
        
            continue;  
        }    

        if(( PeFltExpSubInfo.externalFltExpInfo.associatedSp == associatedSp) &&
            (PeFltExpSubInfo.externalFltExpInfo.fltExpID == fltExpID))
        {
            break;
        }
    }

    if(PeFltExpSubInfo.externalFltExpInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        return (fbe_u32_t)PeFltExpSubInfo.externalFltExpInfo.faultRegisterStatus.cpuStatusRegister;
    }
    
    // no valid fault register status, returning a default here.
    return FBE_PEER_STATUS_UNKNOW;
}



