/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_board_mgmt_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for info related to Board Management
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  07/11/2012:  Lin Lou - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_board_mgmt_cmds.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_peer_boot_utils.h"
#include "fbe_base_environment_debug.h"
#include "fbe_board_mgmt_utils.h"
#include "specl_types.h"

static void fbe_cli_print_ssd_status(void);

/*!**************************************************************
 * fbe_cli_print_boardmgmt_info()
 ****************************************************************
 * @brief
 *      Displays board mgmt info by specific component type.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  05/14/2013:  Chengkai Hu - Created. 
 *
 ****************************************************************/
static void fbe_cli_print_boardmgmt_info(fbe_enclosure_component_types_t component_type)
{
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_status_t                 status = FBE_STATUS_OK;

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
        return;
    }

    /* FBE_ENCL_INVALID_COMPONENT_TYPE indicates print all */
    if (component_type == FBE_ENCL_INVALID_COMPONENT_TYPE)
    {
        fbe_cli_print_board_info();
        fbe_cli_print_board_cache_sataus();
        fbe_cli_print_cache_card_status();
        fbe_cli_print_dimm_status();
        fbe_cli_print_ssd_status();

        fbe_cli_print_suitcase_info(SP_A);
        if (boardInfo.isSingleSpSystem == FALSE)
        {
            fbe_cli_print_suitcase_info(SP_B);
        }
    
        fbe_cli_print_board_bmc_info();
        
        fbe_cli_print_peer_boot_info();
        return;
    }

    switch (component_type)
    {
        case FBE_ENCL_BMC:
            fbe_cli_print_board_bmc_info();
            break;
        case FBE_ENCL_MISC:
            fbe_cli_print_board_info();
            fbe_cli_print_board_cache_sataus();
            break;
        case FBE_ENCL_SUITCASE:
            fbe_cli_print_suitcase_info(SP_A);
            if (boardInfo.isSingleSpSystem == FALSE)
            {
                fbe_cli_print_suitcase_info(SP_B);
            }
            break;
        case FBE_ENCL_FLT_REG:
            fbe_cli_print_peer_boot_info();
            break;
        case FBE_ENCL_CACHE_CARD:
            fbe_cli_print_cache_card_status();
            break;
        case FBE_ENCL_DIMM:
            fbe_cli_print_dimm_status();
            break;
        case FBE_ENCL_SSD:
            fbe_cli_print_ssd_status();
            break;
        default:
            break;
    }

    return;
}
/******************************************************
 * end of fbe_cli_print_boardmgmt_info()
 ******************************************************/

/*************************************************************************
 *          @fn fbe_cli_cmd_boardmgmt ()                                    *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_boardmgmt performs the execution of the BOARDMGMT command
 *   Board Mgmt - Displays board status
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   07/11/2012 Created - Lin Lou
 *
 *************************************************************************/

void fbe_cli_cmd_boardmgmt(fbe_s32_t argc,char ** argv)
{
    fbe_enclosure_component_types_t specified_componentType = FBE_ENCL_INVALID_COMPONENT_TYPE;
    
    /* Parse the command line. */
    while(argc > 0)
    {
        if(strcmp(*argv, "-h") == 0 || strcmp(*argv, "-help") == 0)
        {
            argc--;
            argv++;
            fbe_cli_printf("\n");
            fbe_cli_printf("%s", BOARDMGMT_USAGE);
            return;
        }
        else if(strcmp(*argv, "-c") == 0)
        {
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("too few arguments \n");
                return;
            }
            specified_componentType = fbe_cli_convertStringToComponentType(argv);
            if(specified_componentType == FBE_ENCL_INVALID_COMPONENT_TYPE)
            {
                fbe_cli_error("Invalid component type\n %s", BOARDMGMT_USAGE);
                return;
            }
            fbe_cli_print_boardmgmt_info(specified_componentType);
            return;
        }
        else
        {
            fbe_cli_error("Invalid arguments\n %s", BOARDMGMT_USAGE);
            return;
        }
            
        argc--;
        argv++;
            
    }//end of while

    /* Print all board mgmt info by default */
    fbe_cli_print_boardmgmt_info(FBE_ENCL_INVALID_COMPONENT_TYPE);        
    return;
}
/************************************************************************
 **                     End of fbe_cli_cmd_boardmgmt ()                **
 ************************************************************************/

/*!**************************************************************
 * fbe_cli_print_board_info()
 ****************************************************************
 * @brief
 *      Displays board status and board platform info.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  08/17/2012:  Created. Lin Lou
 *
 ****************************************************************/
static void fbe_cli_print_board_info()
{
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_status_t                      status = FBE_STATUS_OK;
   
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
    }
    else
    {
        fbe_cli_printf("\nSP ID              : %s(%d)\n", fbe_cli_lib_SP_ID_to_string(boardInfo.sp_id), boardInfo.sp_id);
        fbe_cli_printf("Is Single SP Array : %s\n", boardInfo.isSingleSpSystem? "Yes" : "No");
        fbe_cli_printf("Peer Inserted      : %s\n", boardInfo.peerInserted ? "Yes" : "No");
        fbe_cli_printf("Low Battery        : %s\n", boardInfo.lowBattery? "Yes" : "No");
        fbe_cli_printf("Internal Cable     : %s\n", fbe_base_env_decode_connector_cable_status(boardInfo.internalCableStatus));
        fbe_cli_printf("Is Active SP       : %s\n", boardInfo.is_active_sp ? "Yes" : "No");
        fbe_cli_printf("engineIdFault      : %s\n", boardInfo.engineIdFault ? "Yes" : "No");
        fbe_cli_printf("UnsafeToRemoveLED  : %s\n", boardInfo.UnsafeToRemoveLED ? "Yes" : "No");
        if (!boardInfo.isSingleSpSystem)
        {
            fbe_cli_printf("PhysicalMemorySize : SPA(%lu MB) SPB(%lu MB)",
                    (unsigned long)boardInfo.spPhysicalMemorySize[SP_A],
                    (unsigned long)boardInfo.spPhysicalMemorySize[SP_B]);
        }
        else
        {
            fbe_cli_printf("PhysicalMemorySize : SPA(%lu MB)",
                    (unsigned long)boardInfo.spPhysicalMemorySize[SP_A]);
        }

        fbe_cli_printf("\nPlatform Info :\n");
        fbe_cli_printf("  Family Type          : %s\n", decodeFamilyType(boardInfo.platform_info.familyType));
        fbe_cli_printf("  Platform Type        : %s\n", decodeSpidHwType(boardInfo.platform_info.platformType));
        fbe_cli_printf("  Hardware Name        : %s\n", boardInfo.platform_info.hardwareName);
        fbe_cli_printf("  Unique Type          : %s\n", decodeFamilyFruID(boardInfo.platform_info.uniqueType));
        fbe_cli_printf("  CPU Module           : %s\n", decodeCPUModule(boardInfo.platform_info.cpuModule));
        fbe_cli_printf("  Enclosure Type       : %s\n", decodeEnclosureType(boardInfo.platform_info.enclosureType));
        fbe_cli_printf("  Midplane Type        : %s\n", decodeMidplaneType(boardInfo.platform_info.midplaneType));

        fbe_cli_printf("\nVirtual LCC Info :\n");
        fbe_cli_printf("  LCCA : Inserted %d   Faulted %d\n", boardInfo.lccInfo[SP_A].inserted, boardInfo.lccInfo[SP_A].faulted);
        if (!boardInfo.isSingleSpSystem)
        {
            fbe_cli_printf("  LCCB : Inserted %d   Faulted %d\n", boardInfo.lccInfo[SP_B].inserted, boardInfo.lccInfo[SP_B].faulted);
        }
    }

    return;
}
/******************************************************
 * end of fbe_cli_print_board_info()
 ******************************************************/


/*!**************************************************************
 * fbe_cli_print_board_cache_sataus()
 ****************************************************************
 * @brief
 *      Displays board cache status.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  08/17/2012:  Created. Lin Lou
 *
 ****************************************************************/
static void fbe_cli_print_board_cache_sataus()
{
    fbe_common_cache_status_t cacheStatus;
    fbe_status_t              status = FBE_STATUS_OK;

    status = fbe_api_esp_board_mgmt_getCacheStatus(&cacheStatus);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get board cache status, status:%d\n", status);
    }
    else
    {
        fbe_cli_printf("\nCache Status         : %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
    }

    return;
}

/*!**************************************************************
 * fbe_cli_print_cache_card_status()
 ****************************************************************
 * @brief
 *      Displays cache_card cache card status.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  02/17/2013:  Created. Rui Chang
 *
 ****************************************************************/
static void fbe_cli_print_cache_card_status()
{
    fbe_board_mgmt_cache_card_info_t           cacheCardInfo={0};
    fbe_status_t              status = FBE_STATUS_OK;
    fbe_u32_t                 count;
    fbe_u32_t                 i;


    status = fbe_api_esp_board_mgmt_getCacheCardCount(&count);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get Cache Card count, status:%d\n", status);
    }
    else
    {
        fbe_cli_printf("\nCache Card Count : %d\n", count);

        for (i=0;i<count;i++)
        {
            cacheCardInfo.cacheCardID = i;
            status = fbe_api_esp_board_mgmt_getCacheCardStatus(&cacheCardInfo);
        
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to get Cache Card %d status, status:%d\n", i, status);
                return;
            }
            else
            {
                fbe_cli_printf("\nCache Card %d Info :\n", i);
                fbe_cli_printf("  EnvInterfaceStatus:    %s\n", fbe_base_env_decode_env_status(cacheCardInfo.envInterfaceStatus));
                fbe_cli_printf("  uniqueID          :    %s\n", decodeFamilyFruID(cacheCardInfo.uniqueID));
                fbe_cli_printf("  Inserted          :    %s\n", fbe_base_env_decode_status(cacheCardInfo.inserted));
                fbe_cli_printf("  PowerEnable       :    %s\n", fbe_base_env_decode_status(cacheCardInfo.powerEnable));
                fbe_cli_printf("  PowerGood         :    %s\n", fbe_base_env_decode_status(cacheCardInfo.powerGood));
                fbe_cli_printf("  PowerUpEnable     :    %s\n", fbe_base_env_decode_status(cacheCardInfo.powerUpEnable));
                fbe_cli_printf("  PowerUpFailure    :    %s\n", fbe_base_env_decode_status(cacheCardInfo.powerUpFailure));
                fbe_cli_printf("  Reset             :    %s\n", fbe_base_env_decode_status(cacheCardInfo.reset));
            }
        }
    }


    return;
}

/*!**************************************************************
 * fbe_cli_print_dimm_status()
 ****************************************************************
 * @brief
 *      Displays DIMM status.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  05/17/2013:  Created. Rui Chang
 *
 ****************************************************************/
static void fbe_cli_print_dimm_status()
{
    fbe_esp_board_mgmt_dimm_info_t           dimmInfo = {0};
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_u32_t                 count; // total DIMM count in enclosure
    fbe_u32_t                 i;
    fbe_esp_board_mgmt_board_info_t          boardInfo = {0};
    SP_ID                     sp = SP_INVALID;
    fbe_u32_t                 spCount = 0;

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
        return;
    }

    if(boardInfo.isSingleSpSystem)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    status = fbe_api_esp_board_mgmt_getDimmCount(&count);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get DIMM count, status:%d\n", status);
    }
    else
    {
        fbe_cli_printf("\nTotal DIMM Count : %d\n", count);

        for (sp = SP_A; sp < spCount; sp ++) 
        {
            for (i = 0; i < count/spCount; i++) 
            {
                location.sp = sp;
                location.slot = i;
                status = fbe_api_esp_board_mgmt_getDimmInfo(&location, &dimmInfo);
            
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Failed to get %s DIMM %d information, status:%d\n", sp == SP_A ? "SPA" : "SPB", i, status);
                    continue;
                }
                else if (sp == boardInfo.sp_id) 
                {
                    fbe_cli_printf("\n%s DIMM %d Info :\n", sp == SP_A ? "SPA" : "SPB", i);
                    fbe_cli_printf("  associatedSp          :  %s\n", (dimmInfo.associatedSp == 0) ? "SPA" : "SPB");
                    fbe_cli_printf("  dimmID                :  %d\n", dimmInfo.dimmID);
                    fbe_cli_printf("  isLocalDimm           :  %s\n", (dimmInfo.isLocalDimm == FBE_TRUE) ? "Yes" : "No");
                    fbe_cli_printf("  EnvInterfaceStatus    :  %s\n", fbe_base_env_decode_env_status(dimmInfo.envInterfaceStatus));
                    fbe_cli_printf("  Inserted              :  %s\n", fbe_base_env_decode_status(dimmInfo.inserted));
                    fbe_cli_printf("  faulted               :  %s\n", fbe_base_env_decode_status(dimmInfo.faulted));
                    fbe_cli_printf("  state                 :  %s\n", fbe_board_mgmt_decode_dimm_state(dimmInfo.state));
                    fbe_cli_printf("  subState              :  %s\n", fbe_board_mgmt_decode_dimm_subState(dimmInfo.subState));
                    fbe_cli_printf("  dimmType              :  %s\n", decodeSpdDimmType(dimmInfo.dimmType));
                    fbe_cli_printf("  dimmRevision          :  %s\n", decodeSpdDimmRevision(dimmInfo.dimmRevision));
                    fbe_cli_printf("  dimmDensity           :  %d GB\n", dimmInfo.dimmDensity);
                    fbe_cli_printf("  numOfBanks            :  %d\n", dimmInfo.numOfBanks);
                    fbe_cli_printf("  deviceWidth           :  %d\n", dimmInfo.deviceWidth);
                    fbe_cli_printf("  numberOfRanks         :  %d\n", dimmInfo.numberOfRanks);
                    fbe_cli_printf("  vendorRegionValid     :  %s\n", dimmInfo.vendorRegionValid ? "Yes" : "No");
                    fbe_cli_printf("  manufacturingLocation :  0x%x\n", dimmInfo.manufacturingLocation);
                    fbe_cli_printf("  jedecIdCode           :  %s (0x%X)\n", decodeJedecIDCode(dimmInfo.jedecIdCode[0]), dimmInfo.jedecIdCode[0]);
                    fbe_cli_printf("  manufacturingDate     :  ");

                    if(dimmInfo.manufacturingDate[0]== 0)
                    {
                        fbe_cli_printf("Unknown (0x0)\n");
                    }
                    else
                    {
                        /* DDR2 is special. See http://en.wikipedia.org/wiki/Serial_presence_detect */
                        if (dimmInfo.dimmType != DDR2_SDRAM)
                        {
                            fbe_cli_printf("2%03d-Week %d (0x%X 0x%X)\n",  BYTE_BCD_TO_DEC(dimmInfo.manufacturingDate[0]),
                                                                           BYTE_BCD_TO_DEC(dimmInfo.manufacturingDate[1]),
                                                                           dimmInfo.manufacturingDate[0],
                                                                           dimmInfo.manufacturingDate[1]);
                        }
                        else
                        {
                            fbe_cli_printf("2%03d-Week %d (0x%X 0x%X)\n",  dimmInfo.manufacturingDate[0],
                                                                           dimmInfo.manufacturingDate[1],
                                                                           dimmInfo.manufacturingDate[0],
                                                                           dimmInfo.manufacturingDate[1]);
                        }
                    }
          
                    fbe_cli_printf("  VendorSerialNumber    :  %s\n", dimmInfo.vendorSerialNumber);
                    fbe_cli_printf("  VendorPartNumber      :  %s\n", dimmInfo.vendorPartNumber);
                    fbe_cli_printf("  EMCDimmPartNumber     :  %s\n", dimmInfo.EMCDimmPartNumber);
                    fbe_cli_printf("  EMCDimmSerialNumber   :  %s\n", dimmInfo.EMCDimmSerialNumber);
                    fbe_cli_printf("  OldEMCDimmSerialNumber:  %s\n", dimmInfo.OldEMCDimmSerialNumber);
                    fbe_cli_printf("  VendorName            :  %s\n", dimmInfo.vendorName);
                    fbe_cli_printf("  AssemblyName          :  %s\n", dimmInfo.assemblyName);
                }
                else
                {
                    fbe_cli_printf("\n%s DIMM %d Info :\n", sp == SP_A ? "SPA" : "SPB", i);
                    fbe_cli_printf("  associatedSp          :  %s\n", (dimmInfo.associatedSp == 0) ? "SPA" : "SPB");
                    fbe_cli_printf("  dimmID                :  %d\n", dimmInfo.dimmID);
                    fbe_cli_printf("  isLocalDimm           :  %s\n", (dimmInfo.isLocalDimm == FBE_TRUE) ? "Yes" : "No");
                    fbe_cli_printf("  state                 :  %s\n", fbe_board_mgmt_decode_dimm_state(dimmInfo.state));
                    fbe_cli_printf("  subState              :  %s\n", fbe_board_mgmt_decode_dimm_subState(dimmInfo.subState));
                    fbe_cli_printf("  dimmDensity           :  %d GB\n", dimmInfo.dimmDensity);
                }
            }
        }
    }


    return;
}

/******************************************************
 * end of fbe_cli_print_board_cache_sataus()
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_ssd_status()
 ***************************sd*************************************
 * @brief
 *      Displays SSD status.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  10/15/2014:  Created. bphilbin
 *  07-Aug-2015 PHE -Print both local and peer SSD info.
 *
 ****************************************************************/
static void fbe_cli_print_ssd_status()
{
    fbe_esp_board_mgmt_ssd_info_t            ssdInfo = {0};
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_u32_t                 count; // total SSD count in enclosure
    fbe_u32_t                 i;
    fbe_esp_board_mgmt_board_info_t          boardInfo = {0};
    SP_ID                     sp = SP_INVALID;
    fbe_u32_t                 spCount = 0;

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
        return;
    }

    if(boardInfo.isSingleSpSystem)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    status = fbe_api_esp_board_mgmt_getSSDCount(&count);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get SSD count, status:%d\n", status);
    }
    else
    {
        fbe_cli_printf("\nTotal SSD Count : %d\n", count);

        for (sp = SP_A; sp < spCount; sp ++) 
        {
            for (i = 0; i < count/spCount; i++) 
            {
                location.sp = sp;
                location.slot = i;
                status = fbe_api_esp_board_mgmt_getSSDInfo(&location, &ssdInfo);
            
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Failed to get %s SSD %d information, status:%d\n", sp == SP_A ? "SPA" : "SPB", i, status);
                    continue;
                }
                else if (sp == boardInfo.sp_id) 
                {
                    fbe_cli_printf("\n%s SSD %d Info :\n", sp == SP_A ? "SPA" : "SPB", i);
                    fbe_cli_printf("  AssociatedSp          :  %s\n", (ssdInfo.associatedSp == 0) ? "SPA" : "SPB");
                    fbe_cli_printf("  Slot                  :  %d\n", ssdInfo.slot);
                    fbe_cli_printf("  isLocalSsd            :  %s\n", (ssdInfo.isLocalSsd == FBE_TRUE) ? "Yes" : "No");
                    fbe_cli_printf("  State                 :  %s\n", fbe_board_mgmt_decode_ssd_state(ssdInfo.ssdState));
                    fbe_cli_printf("  Self test passed      :  %d\n", ssdInfo.ssdSelfTestPassed);
                    fbe_cli_printf("  Remaining spare blks  :  %d\n", ssdInfo.remainingSpareBlkCount);
                    fbe_cli_printf("  Percent Life Used     :  %d\n", ssdInfo.ssdLifeUsed);
                    fbe_cli_printf("  Temperature (Celsius) :  %d\n", ssdInfo.ssdTemperature);
                    fbe_cli_printf("  SerialNumber          :  %s\n", ssdInfo.ssdSerialNumber);
                    fbe_cli_printf("  PartNumber            :  %s\n", ssdInfo.ssdPartNumber);
                    fbe_cli_printf("  AssemblyName          :  %s\n", ssdInfo.ssdAssemblyName);
                    fbe_cli_printf("  FirmwareRevision      :  %s\n", ssdInfo.ssdFirmwareRevision);
                }
                else
                {
                    fbe_cli_printf("\n%s SSD %d Info :\n", sp == SP_A ? "SPA" : "SPB", i);
                    fbe_cli_printf("  AssociatedSp          :  %s\n", (ssdInfo.associatedSp == 0) ? "SPA" : "SPB");
                    fbe_cli_printf("  Slot                  :  %d\n", ssdInfo.slot);
                    fbe_cli_printf("  isLocalSsd            :  %s\n", (ssdInfo.isLocalSsd == FBE_TRUE) ? "Yes" : "No");
                    fbe_cli_printf("  State                 :  %s\n", fbe_board_mgmt_decode_ssd_state(ssdInfo.ssdState));
                    fbe_cli_printf("  subState              :  %s\n", fbe_board_mgmt_decode_ssd_subState(ssdInfo.ssdSubState));
                }
            }
        }
    }

    return;
}


/*!**************************************************************
 * fbe_cli_print_all_suitcase_info()
 ****************************************************************
 * @brief
 *      Displays board all suitcase info.
 *
 *  @param :
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  08/17/2012:  Created. Lin Lou
 *
 ****************************************************************/
static void fbe_cli_print_suitcase_info(fbe_u8_t which_sp)
{

    fbe_esp_board_mgmt_suitcaseInfo_t suitcaseInfo = {0};
    fbe_status_t                      status = FBE_STATUS_OK;

    suitcaseInfo.location.sp = which_sp;
    suitcaseInfo.location.slot = 0;
    status = fbe_api_esp_board_mgmt_getSuitcaseInfo(&suitcaseInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get suitcase info, status:%d\n", status);
    }
    else if (suitcaseInfo.suitcaseCount > 0)
    {
        fbe_cli_printf("\n%s Suitcase Info:\n", (which_sp==SP_A) ? "SPA" : "SPB");
        fbe_cli_printf("  envInterfaceStatus   : %s\n", fbe_base_env_decode_env_status(suitcaseInfo.suitcaseInfo.envInterfaceStatus));
        fbe_cli_printf("  state                : %s\n", fbe_board_mgmt_decode_suitcase_state(suitcaseInfo.suitcaseInfo.state));
        fbe_cli_printf("  subState             : %s\n", fbe_board_mgmt_decode_suitcase_subState(suitcaseInfo.suitcaseInfo.subState));
        fbe_cli_printf("  Tap12VMissing        : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.Tap12VMissing));
        fbe_cli_printf("  shutdownWarning      : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.shutdownWarning));
        fbe_cli_printf("  ambientOvertempFault : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.ambientOvertempFault));
        fbe_cli_printf("  ambientOvertempWarning : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.ambientOvertempWarning));
        fbe_cli_printf("  fanPackFailure       : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.fanPackFailure));
        fbe_cli_printf("  tempSensor           : %u\n", suitcaseInfo.suitcaseInfo.tempSensor);
        fbe_cli_printf("  DataStale            : %s\n", fbe_base_env_decode_status(suitcaseInfo.dataStale));
        fbe_cli_printf("  EnvIntfFault         : %s\n", fbe_base_env_decode_status(suitcaseInfo.environmentalInterfaceFault));
        fbe_cli_printf("  isCPUFaultRegFail    : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.isCPUFaultRegFail));
        fbe_cli_printf("  hwErrMonFault        : %s\n", fbe_base_env_decode_status(suitcaseInfo.suitcaseInfo.hwErrMonFault));
    }
}
/******************************************************
 * end of fbe_cli_print_suitcase_info()
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_board_bmc_info()
 ****************************************************************
 * @brief
 *      Displays board bmc info.
 *
 *  @param:
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  12/20/2012:  Created. Eric Zhou
 *
 ****************************************************************/
static void fbe_cli_print_board_bmc_info()
{
    fbe_esp_board_mgmt_bmcInfo_t bmcInfo = {0};
    fbe_status_t                 status = FBE_STATUS_OK;
    SP_ID                        sp;
    fbe_u32_t                    bmcIndex, index;
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};
    fbe_u32_t                    spCount=SP_ID_MAX;


    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get boardInfo, status:%d\n", status);
        return;
    }

    if (boardInfo.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    for (sp = SP_A; sp < spCount; sp++)
    {
        for (bmcIndex = 0; bmcIndex < TOTAL_BMC_PER_BLADE; bmcIndex++)
        {
            fbe_zero_memory(&bmcInfo, sizeof(fbe_esp_board_mgmt_bmcInfo_t));
            bmcInfo.location.sp = sp;
            bmcInfo.location.slot = bmcIndex;

            fbe_cli_printf("\n%s BMC %d Info:\n", (sp == SP_A) ? "SPA" : "SPB", bmcIndex);

            status = fbe_api_esp_board_mgmt_getBmcInfo(&bmcInfo);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("Failed to get bmc info, status:%d\n", status);
            }
            else
            {
                fbe_cli_printf("    transactionStatus             : %d\n", bmcInfo.bmcInfo.transactionStatus);
                fbe_cli_printf("    envInterfaceStatus            : %s\n", fbe_base_env_decode_env_status(bmcInfo.bmcInfo.envInterfaceStatus));
                fbe_cli_printf("    DataStale                     : %s\n", BOOLEAN_TO_TEXT(bmcInfo.dataStale));
                fbe_cli_printf("    EnvIntfFault                  : %s\n", BOOLEAN_TO_TEXT(bmcInfo.environmentalInterfaceFault));

                fbe_cli_printf("    shutdownWarning               : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownWarning));
                fbe_cli_printf("    _shutdownReason_\n");
                fbe_cli_printf("      embedIOOverTemp             : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.embedIOOverTemp));
                fbe_cli_printf("      fanFault                    : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.fanFault));
                fbe_cli_printf("      diskOverTemp                : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.diskOverTemp));
                fbe_cli_printf("      slicOverTemp                : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.slicOverTemp));
                fbe_cli_printf("      psOverTemp                  : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.psOverTemp));
                fbe_cli_printf("      memoryOverTemp              : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.memoryOverTemp));
                fbe_cli_printf("      cpuOverTemp                 : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.cpuOverTemp));
                fbe_cli_printf("      ambientOverTemp             : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.ambientOverTemp));
                fbe_cli_printf("      sspHang                     : %s\n", BOOLEAN_TO_TEXT(bmcInfo.bmcInfo.shutdownReason.sspHang));

                fbe_cli_printf("    shutdownTimeRemaining         : %d\n", bmcInfo.bmcInfo.shutdownTimeRemaining);
            
                fbe_cli_printf("    _BIST Report_\n");

                fbe_cli_printf("    cpuTest                       : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.cpuTest));
                fbe_cli_printf("    dramTest                      : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.dramTest));
                fbe_cli_printf("    sramTest                      : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.sramTest));
                for (index = 0; index < I2C_TEST_MAX; index++)
                {
                    fbe_cli_printf("    i2cTest %d                     : %s\n", index, decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.i2cTests[index])); 
                }
            
                for (index = 0; index < FAN_TEST_MAX; index++)
                {
                    fbe_cli_printf("    fanTest %d                     : %s\n", index, decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.fanTests[index]));
                }
            
                for (index = 0; index < BBU_TEST_MAX; index++)
                {
                    fbe_cli_printf("    bbuTest %d                     : %s\n", index, decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.bbuTests[index]));
                }
                fbe_cli_printf("    sspTest                       : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.sspTest));
            
                fbe_cli_printf("    nvramTest                     : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.nvramTest));
                fbe_cli_printf("    sgpioTest                     : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.sgpioTest));
                fbe_cli_printf("    dedicatedLanTest              : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.dedicatedLanTest));
                fbe_cli_printf("    ncsiLanTest                   : %s\n", decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.ncsiLanTest));
            
            
                for (index = 0; index < UART_TEST_MAX; index++)
                {
                    fbe_cli_printf("    uartTest %d                    : %s\n", index, decodeBMCBistResult(bmcInfo.bmcInfo.bistReport.uartTests[index]));
                }

                fbe_cli_printf("    _BMC App Main Firmware Status_\n");
                fbe_cli_printf("      updateStatus                : %s\n", decodeFirmwareUpdateStatus(bmcInfo.bmcInfo.firmwareStatus.bmcAppMain.updateStatus));
                fbe_cli_printf("      Firmware Revision           : %d.%d\n",
                    bmcInfo.bmcInfo.firmwareStatus.bmcAppMain.revisionMajor,
                    bmcInfo.bmcInfo.firmwareStatus.bmcAppMain.revisionMinor);
            
                fbe_cli_printf("    _BMC App Redundant Firmware Status_\n");
                fbe_cli_printf("      updateStatus                : %s\n", decodeFirmwareUpdateStatus(bmcInfo.bmcInfo.firmwareStatus.bmcAppRedundant.updateStatus));
                fbe_cli_printf("      Firmware Revision           : %d.%d\n",
                    bmcInfo.bmcInfo.firmwareStatus.bmcAppRedundant.revisionMajor,
                    bmcInfo.bmcInfo.firmwareStatus.bmcAppRedundant.revisionMinor);
            
                fbe_cli_printf("    _BMC SSP Firmware Status_\n");
                fbe_cli_printf("      updateStatus                : %s\n", decodeFirmwareUpdateStatus(bmcInfo.bmcInfo.firmwareStatus.ssp.updateStatus));
                fbe_cli_printf("      Firmware Revision           : %d.%d\n",
                    bmcInfo.bmcInfo.firmwareStatus.ssp.revisionMajor,
                    bmcInfo.bmcInfo.firmwareStatus.ssp.revisionMinor);
            
                fbe_cli_printf("    _BMC Boot Block Firmware Status_\n");
                fbe_cli_printf("      updateStatus                : %s\n", decodeFirmwareUpdateStatus(bmcInfo.bmcInfo.firmwareStatus.bootBlock.updateStatus));
                fbe_cli_printf("      Firmware Revision           : %d.%d\n",
                    bmcInfo.bmcInfo.firmwareStatus.bootBlock.revisionMajor,
                    bmcInfo.bmcInfo.firmwareStatus.bootBlock.revisionMinor);
            }
        }
    }
}

/*!**************************************************************
 * fbe_cli_print_flt_reg_status()
 ****************************************************************
 * @brief
 *      Displays fault status register info.
 *
 *  @param:
 *      fltRegStatusPtr - fault status register handle.
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  05/14/2013:  Chengkai Hu - Created. 
 *
 ******************************************************/
extern void fbe_cli_print_flt_reg_status(SPECL_FAULT_REGISTER_STATUS       * fltRegStatusPtr)
{
    fbe_u32_t    eachFault = 0;
    fbe_u32_t    width = 25;

    fbe_cli_printf("\t %-*s : %s\n",            width,  "Any Faults Found",         BOOLEAN_TO_TEXT(fltRegStatusPtr->anyFaultsFound));

    fbe_cli_printf("\t %-*s : 0x%08X (%s)\n",   width,  "CPU Status Register",      fltRegStatusPtr->cpuStatusRegister,
                                                                            decodeCpuStatusRegister(fltRegStatusPtr->cpuStatusRegister));    
    for (eachFault = 0; eachFault < MAX_DIMM_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->dimmFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %2d : %s\n",    width - 3,  "Failure DIMM",     eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->dimmFault[eachFault]));
        }
    }

    for (eachFault = 0; eachFault < MAX_DRIVE_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->driveFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %2d : %s\n",    width - 3,  "Failure Drive",    eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->driveFault[eachFault]));
        }
    }

    for (eachFault = 0; eachFault < MAX_SLIC_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->slicFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %2d : %s\n",    width - 3,  "Failure SLIC",     eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->slicFault[eachFault]));
        }
    }

    for (eachFault = 0; eachFault < MAX_POWER_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->powerFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %d : %s\n",     width - 3,  "Failure Power",    eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->powerFault[eachFault]));
        }
    }


    for (eachFault = 0; eachFault < MAX_BATTERY_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->batteryFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %d : %s\n",     width - 3,  "Failure Battery",    eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->batteryFault[eachFault]));
        }
    }

    if (fltRegStatusPtr->superCapFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "SuperCap Failure", BOOLEAN_TO_TEXT(fltRegStatusPtr->superCapFault));
    }

    for (eachFault = 0; eachFault < MAX_FAN_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->fanFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %d : %s\n",     width - 2,  "Failure Fan",      eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->fanFault[eachFault]));
        }
    }

    for (eachFault = 0; eachFault < MAX_I2C_FAULT; eachFault++)
    {
        if (fltRegStatusPtr->i2cFault[eachFault])
        {
            fbe_cli_printf("\t %-*s %d : %s\n",     width - 2,  "Failure I2C Bus",  eachFault,
                                                                            BOOLEAN_TO_TEXT(fltRegStatusPtr->i2cFault[eachFault]));
        }
    }

    if (fltRegStatusPtr->cpuFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "CPU Failure",      BOOLEAN_TO_TEXT(fltRegStatusPtr->cpuFault));
    }

    if (fltRegStatusPtr->mgmtFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "Mgmt Failure",     BOOLEAN_TO_TEXT(fltRegStatusPtr->mgmtFault));
    }

    if (fltRegStatusPtr->bemFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "BEM Failure",      BOOLEAN_TO_TEXT(fltRegStatusPtr->bemFault));
    }

    if (fltRegStatusPtr->eFlashFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "eFlash Failure",   BOOLEAN_TO_TEXT(fltRegStatusPtr->eFlashFault));
    }

    if (fltRegStatusPtr->cacheCardFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "CacheCard Failure",  BOOLEAN_TO_TEXT(fltRegStatusPtr->cacheCardFault));
    }

    if (fltRegStatusPtr->midplaneFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "Midplane Failure", BOOLEAN_TO_TEXT(fltRegStatusPtr->midplaneFault));
    }

    if (fltRegStatusPtr->cmiFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "CMI Failure",      BOOLEAN_TO_TEXT(fltRegStatusPtr->cmiFault));
    }

    if (fltRegStatusPtr->allFrusFault)
    {
        fbe_cli_printf("\t %-*s : %s\n",            width,      "All Frus Faulted", BOOLEAN_TO_TEXT(fltRegStatusPtr->allFrusFault));
    }
    
    return;
}   
/******************************************************
 * end of fbe_cli_print_flt_reg_status()
 ******************************************************/

/*!**************************************************************
 * fbe_cli_print_peer_boot_info()
 ****************************************************************
 * @brief
 *      Displays peer boot info.
 *
 *  @param:
 *      Nothing
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  08/17/2012:  Created. Lin Lou
 *
 ******************************************************/
static void fbe_cli_print_peer_boot_info(void)
{
    fbe_esp_board_mgmt_peer_boot_info_t peerBootInfo = {0};
    fbe_status_t                        status = FBE_STATUS_OK;

    status = fbe_api_esp_board_mgmt_getPeerBootInfo(&peerBootInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get peer boot info, status:%d\n", status);
    }
    else
    {
        fbe_cli_printf("\nPeer Boot Info:\n");
        fbe_cli_printf("  Data Stale            : %s\n", BOOLEAN_TO_TEXT(peerBootInfo.dataStale));
        fbe_cli_printf("  Env Intf Fault        : %s\n", BOOLEAN_TO_TEXT(peerBootInfo.environmentalInterfaceFault));
        fbe_cli_printf("  Peer SP App Running   : %s\n", BOOLEAN_TO_TEXT(peerBootInfo.isPeerSpApplicationRunning));
        fbe_cli_printf("  Peer boot state       : %s(%d)\n", fbe_pbl_decodeBootState(peerBootInfo.peerBootState), peerBootInfo.peerBootState);
        fbe_cli_printf("  BIOS/POST fail        : %s\n\n", BOOLEAN_TO_TEXT(peerBootInfo.biosPostFail));
        fbe_cli_printf("  FSR(Fault Status Register) Info:\n");
        fbe_cli_print_flt_reg_status(&peerBootInfo.fltRegStatus);
        fbe_cli_printf("\n");
        fbe_cli_printf("  Last FSR clear time   : %d/%d/%d   %d:%d:%d\n",  
            peerBootInfo.lastFltRegClearTime.month, peerBootInfo.lastFltRegClearTime.day, peerBootInfo.lastFltRegClearTime.year,
            peerBootInfo.lastFltRegClearTime.hour, peerBootInfo.lastFltRegClearTime.minute, peerBootInfo.lastFltRegClearTime.second);
        fbe_cli_printf("  Last FSR Info:\n");
        fbe_cli_print_flt_reg_status(&peerBootInfo.lastFltRegStatus);
    }

    return;
}
/******************************************************
 * end of fbe_cli_print_peer_boot_info()
 ******************************************************/

