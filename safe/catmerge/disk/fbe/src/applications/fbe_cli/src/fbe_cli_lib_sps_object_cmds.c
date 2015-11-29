/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_sps_object_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for the SPS object.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   02/08/2010:  Vishnu Sharma - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_sps_info.h"
#include "fbe_cli_sps_obj.h"
#include "fbe_cli_lib_energy_stats_cmds.h"
#include "generic_utils_lib.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe_cli_private.h"

static void fbe_cli_processTestTime(fbe_u16_t day, fbe_u16_t hour, fbe_u16_t minute);

/*************************************************************************
 *                            @fn fbe_cli_cmd_spsinfo ()                 *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_spsinfo performs the execution of the SPSINFO command
 *   Display SPS related information
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   02-Aug-2010 Created - Vishnu Sharma
 *
 *************************************************************************/

void fbe_cli_cmd_spsinfo(fbe_s32_t argc,char ** argv)
{
    fbe_esp_sps_mgmt_spsCounts_t    spsCountInfo;
    fbe_device_physical_location_t  spsLocation;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       spsIndex = 0;
    fbe_u16_t                       simulateSps;
    fbe_u16_t                       day, hour, minute;
    fbe_bool_t                      status_flag = FBE_FALSE;
    fbe_bool_t                      power_flag = FBE_FALSE;
    fbe_bool_t                      manuf_flag = FBE_FALSE;

    if (argc == 1)
    {
        if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
                */
            fbe_cli_printf("\n");
            fbe_cli_printf("%s", SPSINFO_USAGE);
            return;
        }
        else if (strcmp(*argv, "-status") == 0)
        {
            status_flag = FBE_TRUE;
        }
        else if (strcmp(*argv, "-manuf") == 0)
        {
            manuf_flag = FBE_TRUE;
        }
        else if (strcmp(*argv, "-power") == 0)
        {
            power_flag = FBE_TRUE;
        }
        else if (strcmp(*argv, "-setSimulateSps") == 0)
        {
            fbe_cli_printf("Missing argurments\n");
            fbe_cli_printf("spsinfo -setSimulateSps simulateSps\n");
            fbe_cli_printf("\tsimulateSps: TRUE(1)/FALSE(0)\n");
            return;
        }
        else if (strcmp(*argv, "-settesttime") == 0)
        {
            /* If they are asking for help, just display help and exit.
                */
            fbe_cli_printf("Missing argurments\n");
            fbe_cli_printf("spsinfo -settesttime Day Hour Minute\n");
            fbe_cli_printf("\tDay: day of week (0-Sun, 1-Mon, ...)\n");
            fbe_cli_printf("\tHour: hour of day (0-23)\n");
            fbe_cli_printf("\tMinute: minute (0-59)\n");
            return;
        }
    }

    if ((argc == 2 )&&((strcmp(*argv, "-setSimulateSps") == 0)))
    {
        argv++;
        simulateSps = atoi(*argv);
        if ((simulateSps != 0) && (simulateSps != 1))
        {
            fbe_cli_printf("Invalid simulateSps parameter (0-1)\n");
            return;
        }

        status = fbe_api_esp_sps_mgmt_setSimulateSps((simulateSps == 1) ? TRUE : FALSE);
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("simulateSps successfully set\n");
        }
        else
        {
            fbe_cli_printf("Failed to set simulateSps, status %d\n", status);
        }

        return;
    }
    if ((argc == 4 )&&((strcmp(*argv, "-settesttime") == 0)))
    {
        // get day, hour, minute arguemnts
        argc--;
        argv++;
        day = atoi(*argv);
        argc--;
        argv++;
        hour = atoi(*argv);
        argc--;
        argv++;
        minute = atoi(*argv);

        fbe_cli_processTestTime(day, hour, minute);
        return;
    }
       
    if (argc > 1)
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_printf("%s", SPSINFO_USAGE);
        return;
    }
    
    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get SPS Counts, status %d\n", status);
        return;
    }
    if (spsCountInfo.totalNumberOfSps == 0)
    {
        fbe_cli_printf("This array does not have any SPS's\n");
        return;
    }

    if (argc == 0)
    {
        fbe_cli_print_overall_spsinfo();
    }

    /* This is for Megatron PE. Megatron has 2 enclosures with SPS */
    if (spsCountInfo.enclosuresWithSps > 1)
    {
        fbe_set_memory(&spsLocation, 0, sizeof(fbe_device_physical_location_t));
        spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        for(spsIndex = FBE_LOCAL_SPS; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
        {
            spsLocation.slot = spsIndex;
            if(argc == 0 || status_flag)
            {
                fbe_cli_print_spsinfo(&spsLocation);
            }
            if(argc == 0)
            {
                // whne argc == 0, location is displayed in spsinfo, don't display here
                fbe_cli_print_sps_manuf_info(&spsLocation, FALSE);
            }
            if(manuf_flag)
            {
                fbe_cli_print_sps_manuf_info(&spsLocation, TRUE);
            }
            if(power_flag)
            {
                fbe_cli_print_sps_input_power(&spsLocation);
            }
        }
    }
    /* If some platform has enclosure with SPS, there must has one in DAE0 */
    if (spsCountInfo.enclosuresWithSps > 0)
    {
        fbe_set_memory(&spsLocation, 0, sizeof(fbe_device_physical_location_t));
        spsLocation.bus = 0;
        spsLocation.enclosure = 0;
        for(spsIndex = FBE_LOCAL_SPS; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
        {
            spsLocation.slot = spsIndex;
            if(argc == 0 || status_flag)
            {
                fbe_cli_print_spsinfo(&spsLocation);
            }
            if(argc == 0)
            {
                // whne argc == 0, location is displayed in spsinfo, don't display here
                fbe_cli_print_sps_manuf_info(&spsLocation, FALSE);
            }
            if (manuf_flag)
            {
                fbe_cli_print_sps_manuf_info(&spsLocation, TRUE);
            }
            if(power_flag)
            {
                fbe_cli_print_sps_input_power(&spsLocation);
            }
        }
    }
}
/************************************************************************
 **                       End of fbe_cli_cmd_spsinfo ()                **
 ************************************************************************/

/*************************************************************************
 *                 @fn fbe_cli_print_overall_spsinfo()                   *
 *************************************************************************
 *
 *  @brief
 *   Print ovarall sps information
 *
 * @param :
 *   None
 *
 * @return:
 *   None
 *
 * @author
 *   26-Feb-2013 Created - Lin Lou
 *
 *************************************************************************/
static void fbe_cli_print_overall_spsinfo(void)
{
    fbe_esp_sps_mgmt_spsCounts_t    spsCountInfo;
    fbe_u32_t                       spsEnclIndex;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      simulateSps = FBE_FALSE;
    fbe_esp_sps_mgmt_spsTestTime_t  spsTestTimeInfo = {0};
    fbe_s8_t                        timeString[8] = {'\0'};
    fbe_common_cache_status_t       cacheStatus;
    fbe_esp_sps_mgmt_objectData_t   spsMgmtObjectData;
    fbe_u32_t                       elapsedTimeInSecs;
    fbe_system_time_t               decodedTime;

    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get SPS Counts, status:0x%X\n", status);
        return;
    }
    status = fbe_api_esp_sps_mgmt_getObjectData(&spsMgmtObjectData);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get ObjectData, status:0x%X\n", status);
        return;
    }

    // print private data
    fbe_cli_printf("SPS_MGMT Private Object Data:\n");
    fbe_cli_printf("    testingState : %d\n", spsMgmtObjectData.testingState);
    elapsedTimeInSecs = fbe_get_elapsed_seconds(spsMgmtObjectData.testStartTime);
    fbe_cli_decodeElapsedTime(elapsedTimeInSecs, &decodedTime);
    fbe_cli_printf("    testStartTime : %d Hours, %d Minutes, %d Seconds\n", 
                   decodedTime.hour, decodedTime.minute, decodedTime.second);

    fbe_cli_printf("SPS Info:\n");
    if(spsCountInfo.totalNumberOfSps > 0)
    {
        // print private SPS data
        fbe_cli_printf("    needToTest : %d\n",
                       spsMgmtObjectData.privateObjectData.privateSpsData.needToTest);
        if (spsMgmtObjectData.privateObjectData.privateSpsData.spsTestType == FBE_SPS_CABLING_TEST_TYPE)
        {
            fbe_cli_printf("    spsTestType : Cable\n");
        }
        else
        {
            fbe_cli_printf("    spsTestType : Battery\n");
        }
        fbe_cli_printf("    spsBatteryOnlineCount : %d\n",
                       spsMgmtObjectData.privateObjectData.privateSpsData.spsBatteryOnlineCount);
        fbe_cli_printf("    testingCompleted : %s\n",
                       spsMgmtObjectData.privateObjectData.privateSpsData.testingCompleted ? "TRUE" : "FALSE");
        fbe_cli_printf("    spsTestStartTime : %lld\n",
                       spsMgmtObjectData.privateObjectData.privateSpsData.spsTestStartTime);
        fbe_cli_printf("    spsTestTimeDetected : %s\n",
                       spsMgmtObjectData.privateObjectData.privateSpsData.spsTestTimeDetected ? "TRUE" : "FALSE");
        for (spsEnclIndex = 0; spsEnclIndex < spsCountInfo.enclosuresWithSps; spsEnclIndex++)
        {
            fbe_cli_printf("    spsEnclIndex %d\n", spsEnclIndex);
            fbe_cli_printf("      mfgInfoNeeded : %s\n",
                           spsMgmtObjectData.privateObjectData.privateSpsData.mfgInfoNeeded[spsEnclIndex] ? "TRUE" : "FALSE");
            fbe_cli_printf("      debouncedSpsFault Info : %s, Count : %d\n",
                           fbe_sps_mgmt_getSpsFaultString(&spsMgmtObjectData.privateObjectData.privateSpsData.debouncedSpsFaultInfo[spsEnclIndex]),
                           spsMgmtObjectData.privateObjectData.privateSpsData.debouncedSpsFaultsCount[spsEnclIndex]);
            fbe_cli_printf("      debouncedSpsStatus Status : %s, Count : %d\n",
                           fbe_sps_mgmt_getSpsStatusString(spsMgmtObjectData.privateObjectData.privateSpsData.debouncedSpsStatus[spsEnclIndex]),
                           spsMgmtObjectData.privateObjectData.privateSpsData.debouncedSpsStatusCount[spsEnclIndex]);
        }

        fbe_cli_printf("\nOverall SPS Info:\n");
        fbe_cli_printf("    Total Number Of SPS : %d\n", spsCountInfo.totalNumberOfSps);
        fbe_cli_printf("    Enclosures With SPS : %d\n", spsCountInfo.enclosuresWithSps);
        fbe_cli_printf("    SPS Per Enclosure   : %d\n", spsCountInfo.spsPerEnclosure);
        status = fbe_api_esp_sps_mgmt_getSimulateSps(&simulateSps);
        if (status == FBE_STATUS_OK)
        {

            fbe_cli_printf("    Simulate SPS        : %s\n", simulateSps ? "YES" : "NO");
        }
        else
        {
            fbe_cli_printf("    Simulate SPS        : UNKOWN\n");
        }

        status = fbe_api_esp_sps_mgmt_getCacheStatus(&cacheStatus);
        if(status == FBE_STATUS_OK)
        {
            fbe_cli_printf("    SPS Cache Status    : %s\n", fbe_cli_print_HwCacheStatus(cacheStatus));
        }
        else
        {
            fbe_cli_printf("    SPS Cache Status    : UNKNOWN\n");
        }

        status = fbe_api_esp_sps_mgmt_getSpsTestTime(&spsTestTimeInfo);
        if(status == FBE_STATUS_OK)
        {
            fbe_getTimeString(&(spsTestTimeInfo.spsTestTime), timeString);
            fbe_cli_printf("    Weekly SPS Test Time: %s %s\n",
                        fbe_getWeekDayString(spsTestTimeInfo.spsTestTime.weekday),
                        timeString);
        }
        else
        {
            fbe_cli_printf("    Weekly SPS Test Time: UNKNOWN\n");
        }
    }

}

/*************************************************************************
 *                @fn fbe_cli_print_overall_spsinfo()                    *
 *************************************************************************
 *
 *  @brief
 *      Prints SPS related information viz.serial number,part number,vendor,model number etc
 *      of local & Peer SP. 
 *      Called by FBE cli spsinfo command
 *
 *  @param :
 *      fbe_u32_t which_SP.
 *
 *  @return:
 *      Nothing.
 *
 *  @author
 *    02-July-2010   Vishnu Sharma - Created.
 *
 **********************************************************************/

static void fbe_cli_print_spsinfo(fbe_device_physical_location_t *location) 
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_esp_sps_mgmt_spsStatus_t              spsStatusInfo = {0};

    /*  Get SPS  status information  */
    spsStatusInfo.spsLocation = *location;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    
    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\n");
        fbe_cli_printf("%s information:\n", fbe_sps_mgmt_getSpsString(&spsStatusInfo.spsLocation));
        fbe_cli_printf("    SPS Inserted:             %s \n", spsStatusInfo.spsModuleInserted ? "Yes" : "No");
        fbe_cli_printf("    SPS Battery Present:      %s \n", spsStatusInfo.spsBatteryInserted ? "Yes" : "No");
        fbe_cli_printf("    SPS Model:                %s \n",fbe_sps_mgmt_getSpsModelString(spsStatusInfo.spsModel));
        fbe_cli_printf("    SPS Dual Component :      %d \n",spsStatusInfo.dualComponentSps);
        fbe_cli_printf("    SPS Status:               %s \n",fbe_sps_mgmt_getSpsStatusString(spsStatusInfo.spsStatus));
        fbe_cli_printf("    SPS Fault Status:         %s \n",fbe_sps_mgmt_getSpsFaultString(&spsStatusInfo.spsFaultInfo));
        fbe_cli_printf("    SPS Cabling Status:       %s \n",fbe_sps_mgmt_getSpsCablingStatusString(spsStatusInfo.spsCablingStatus));
        fbe_cli_printf("    Primary Firmware Rev:     %s \n", spsStatusInfo.primaryFirmwareRev);
        fbe_cli_printf("    Secondary Firmware Rev:   %s \n", spsStatusInfo.secondaryFirmwareRev);
        fbe_cli_printf("    Battery Firmware Rev:     %s \n", spsStatusInfo.batteryFirmwareRev);
    }
}
/************************************************************************
 **                       End of fbe_cli_print_spsinfo ()              **
 ************************************************************************/

/*************************************************************************
 *                  @fn fbe_cli_print_sps_manuf_info()                   *
 *************************************************************************
 *
 *  @brief
 *   Print sps manufacturing information
 *
 * @param :
 *   location - sps location
 *
 * @return:
 *   None
 *
 * @author
 *   26-Feb-2013 Created - Lin Lou
 *
 *************************************************************************/
static void fbe_cli_print_sps_manuf_info(fbe_device_physical_location_t *location, fbe_bool_t displayLocation)
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_esp_sps_mgmt_spsStatus_t              spsStatusInfo = {0};
    fbe_esp_sps_mgmt_spsManufInfo_t           spsManufInfo = {0};

    spsStatusInfo.spsLocation = *location;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);

    /*  Get manufacturing information for SPS    */
    spsManufInfo.spsLocation = *location;
    status = fbe_api_esp_sps_mgmt_getManufInfo(&spsManufInfo);
    if(status == FBE_STATUS_OK)
    {
        if (spsStatusInfo.spsModuleInserted)
        {
            if(displayLocation)
            {
                fbe_cli_printf("\n%s", fbe_sps_mgmt_getSpsString(&spsStatusInfo.spsLocation));
            }
            fbe_cli_printf("\n    Module Manufacturing Info\n");
            fbe_cli_printf("      SerialNumber:           %.16s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsSerialNumber);
            fbe_cli_printf("      PartNumber:             %.16s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsPartNumber);
            fbe_cli_printf("      PartNumRevision:        %.16s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsPartNumRevision);
            fbe_cli_printf("      Vendor:                 %.32s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsVendor);
            fbe_cli_printf("      VendorModelNum:         %.32s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsVendorModelNumber);
            fbe_cli_printf("      FwRevision:             %.8s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsFirmwareRevision);
            fbe_cli_printf("      SecondaryFwRevision:    %.8s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsSecondaryFirmwareRevision);
            fbe_cli_printf("      ModelString:            %.12s\n",
                           spsManufInfo.spsManufInfo.spsModuleManufInfo.spsModelString);
            fbe_cli_printf("      SPS Module FFID:        %s(0x%x)\n", decodeFamilyFruID(spsStatusInfo.spsModuleFfid), spsStatusInfo.spsModuleFfid);
        }
        if (spsStatusInfo.spsBatteryInserted)
        {
            fbe_cli_printf("\n    Battery Manufacturing Info\n");
            fbe_cli_printf("      SerialNumber:           %.16s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsSerialNumber);
            fbe_cli_printf("      PartNumber:             %.16s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsPartNumber);
            fbe_cli_printf("      PartNumRevision:        %.16s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsPartNumRevision);
            fbe_cli_printf("      Vendor:                 %.32s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsVendor);
            fbe_cli_printf("      VendorModelNum:         %.32s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsVendorModelNumber);
            fbe_cli_printf("      FwRevision:             %.8s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsFirmwareRevision);
            fbe_cli_printf("      ModelString:            %.12s\n",
                           spsManufInfo.spsManufInfo.spsBatteryManufInfo.spsModelString);
            fbe_cli_printf("      SPS Battery FFID:       %s(0x%x)\n", decodeFamilyFruID(spsStatusInfo.spsBatteryFfid), spsStatusInfo.spsBatteryFfid);
        }
    }
    else
    {
        fbe_cli_printf("Unable to obtain manufacturing info from sps mgmt object \n");
    }

}
/************************************************************************
 **                End of fbe_cli_print_sps_manuf_info()               **
 ************************************************************************/

/*************************************************************************
 *                 @fn fbe_cli_print_sps_input_power()                   *
 *************************************************************************
 *
 *  @brief
 *   Print sps energy information
 *
 * @param :
 *   location - sps location
 *
 * @return:
 *   None
 *
 * @author
 *   26-Feb-2013 Created - Lin Lou
 *
 *************************************************************************/
static void fbe_cli_print_sps_input_power(fbe_device_physical_location_t *location)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_esp_sps_mgmt_spsInputPower_t spsInputPowerInfo = {0};
    char                             statusString[EIR_STAT_STAUS_STRING_LENGTH];

    spsInputPowerInfo.spsLocation = *location;
    status = fbe_api_esp_sps_mgmt_getSpsInputPower(&spsInputPowerInfo);

    if(status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\n%s Input Power Info\n", fbe_sps_mgmt_getSpsString(&spsInputPowerInfo.spsLocation));

        fbe_cli_decodeStatsStatus(spsInputPowerInfo.spsCurrentInputPower.status, statusString);
        fbe_cli_printf ("    Current Input Power : %u, status: %s\n",
                spsInputPowerInfo.spsCurrentInputPower.inputPower, statusString);

        fbe_cli_decodeStatsStatus(spsInputPowerInfo.spsAverageInputPower.status, statusString);
        fbe_cli_printf ("    Average Input Power : %u, status: %s\n",
                spsInputPowerInfo.spsAverageInputPower.inputPower, statusString);

        fbe_cli_decodeStatsStatus(spsInputPowerInfo.spsMaxInputPower.status, statusString);
        fbe_cli_printf ("    Max Input Power     : %u, status: %s\n",
                spsInputPowerInfo.spsMaxInputPower.inputPower, statusString);
    }
    else
    {
        fbe_cli_error("Failed to get SPS input power info, status:0x%X\n", status);
    }
}
/************************************************************************
 **               End of fbe_cli_print_sps_input_power()               **
 ************************************************************************/

/*************************************************************************
 *                            @fn fbe_cli_cmd_bbuinfo ()                 *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_bbuinfo performs the execution of the BBUINFO command
 *   Display BBU related information
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   11-Nov-2012 Created - Joe Perry
 *
 *************************************************************************/

void fbe_cli_cmd_bbuinfo(fbe_s32_t argc,char ** argv)
{

    /*********************************
    **    VARIABLE DECLARATIONS    **
    *********************************/
    fbe_esp_sps_mgmt_spsTestTime_t  spsTestTimeInfo;
    fbe_esp_sps_mgmt_bobStatus_t    bbu_status_info;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       bbuIndex, bbuCount;
    fbe_s8_t                        timeString[8] = {'\0'};
    fbe_u16_t                       simulateBbu;
    fbe_u16_t                       day, hour, minute;
    fbe_object_id_t                 objectId;
    fbe_bool_t                      diskBatteryBackedSet = FALSE;
    fbe_esp_sps_mgmt_objectData_t   spsMgmtObjectData;
    fbe_u32_t                       elapsedTimeInSecs;
    fbe_system_time_t               decodedTime;
    fbe_u32_t                       bbuSideIndex;

    /*****************
    **    BEGIN    **
    *****************/
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.
            */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", BBUINFO_USAGE);
        return;
    }

    if ((argc == 1 )&&((strcmp(*argv, "-setSimulateBbu") == 0)))
    {
        fbe_cli_printf("Missing argurments\n");
        fbe_cli_printf("bbuinfo -setSimulateBbu simulateBbu\n");
        fbe_cli_printf("\tsimulateBbu: TRUE(1)/FALSE(0)\n");
        return;
    }

    if ((argc == 2 )&&((strcmp(*argv, "-setSimulateBbu") == 0)))
    {
        argv++;
        simulateBbu = atoi(*argv);
        if ((simulateBbu != 0) && (simulateBbu != 1))
        {
            fbe_cli_printf("Invalid simulateBbu parameter (0-1)\n");
            return;
        }

        status = fbe_api_esp_sps_mgmt_setSimulateSps((simulateBbu == 1) ? TRUE : FALSE);
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("simulateBbu successfully set\n");
        }
        else
        {
            fbe_cli_printf("Failed to set simulateBbu, status %d\n", status);
        }

        return;
    }

    if ((argc == 1 )&&((strcmp(*argv, "-runSelfTest") == 0)))
    {
        status = fbe_api_esp_sps_mgmt_initiateSelfTest();
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("runSelfTest successfully set\n");
        }
        else
        {
            fbe_cli_printf("Failed to runSelfTest, status %d\n", status);
        }
        return;
    }

    if ((argc == 1 )&&((strcmp(*argv, "-settesttime") == 0)))
    {
        /* If they are asking for help, just display help and exit.
            */
        fbe_cli_printf("Missing argurments\n");
        fbe_cli_printf("bbuinfo -settesttime Day Hour Minute\n");
        fbe_cli_printf("\tDay: day of week (0-Sun, 1-Mon, ...)\n");
        fbe_cli_printf("\tHour: hour of day (0-23)\n");
        fbe_cli_printf("\tMinute: minute (0-59)\n");
        return;
    }
    if ((argc == 4 )&&((strcmp(*argv, "-settesttime") == 0)))
    {
        // get day, hour, minute arguemnts
        argc--;
        argv++;
        day = atoi(*argv);
        argc--;
        argv++;
        hour = atoi(*argv);
        argc--;
        argv++;
        minute = atoi(*argv);

        fbe_cli_processTestTime(day, hour, minute);
        return;
    }
       
    if (argc >= 1)
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_printf("%s", BBUINFO_USAGE);
        return;
    }
    
    status = fbe_api_esp_sps_mgmt_getBobCount(&bbuCount);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get BBU Counts, status %d\n", status);
        return;
    }
    if (bbuCount == 0)
    {
        fbe_cli_printf("This array does not have any BBU's\n");
        return;
    }

    status = fbe_api_esp_sps_mgmt_getObjectData(&spsMgmtObjectData);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get ObjectData, status:0x%X\n", status);
        return;
    }

    // print private data
    fbe_cli_printf("SPS_MGMT Private Object Data:\n");
    fbe_cli_printf("    testingState : %d\n", spsMgmtObjectData.testingState);
    elapsedTimeInSecs = fbe_get_elapsed_seconds(spsMgmtObjectData.testStartTime);
    fbe_cli_decodeElapsedTime(elapsedTimeInSecs, &decodedTime);
    fbe_cli_printf("    testStartTime : %d Hours, %d Minutes, %d Seconds\n", 
                   decodedTime.hour, decodedTime.minute, decodedTime.second);
    // print private BBU data
    fbe_cli_printf("    simulateBobInfo : %d\n",
                   spsMgmtObjectData.privateObjectData.privateBbuData.simulateBobInfo);
    fbe_cli_printf("    needToTest : %s\n",
                   spsMgmtObjectData.privateObjectData.privateBbuData.needToTest ? "TRUE" : "FALSE");
    fbe_cli_printf("    testingCompleted : %s\n",
                   spsMgmtObjectData.privateObjectData.privateBbuData.testingCompleted ? "TRUE" : "FALSE");

    fbe_cli_printf("BBU Info:\n");
    for (bbuIndex = 0; bbuIndex < bbuCount; bbuIndex++)
    {
        fbe_cli_printf("    bbuIndex %d, \n", bbuIndex);
        fbe_cli_printf("      needToTest : %s\n",
                       spsMgmtObjectData.privateObjectData.privateBbuData.needToTestPerBbu[bbuIndex] ? "TRUE" : "FALSE");
        fbe_cli_printf("      notBatteryReadyTimeStart : %lld\n",
                       spsMgmtObjectData.privateObjectData.privateBbuData.notBatteryReadyTimeStart[bbuIndex]);
        fbe_cli_printf("      debouncedBobOnBattery, State : %s, Count : %d\n",
                       spsMgmtObjectData.privateObjectData.privateBbuData.debouncedBobOnBatteryState[bbuIndex] ? "TRUE" : "FALSE",
                       spsMgmtObjectData.privateObjectData.privateBbuData.debouncedBobOnBatteryStateCount[bbuIndex]);
    }

    for (bbuIndex = 0; bbuIndex < bbuCount; bbuIndex++)
    {
        fbe_set_memory(&bbu_status_info, 0, sizeof(fbe_esp_sps_mgmt_bobStatus_t));
        bbu_status_info.bobIndex = bbuIndex;
        status = fbe_api_esp_sps_mgmt_getBobStatus(&bbu_status_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Error in get bbu status, error code: %d\n", __FUNCTION__, status);
            return;
        }

        if (bbu_status_info.bobInfo.associatedSp == SP_A)
        {
            bbuSideIndex = bbuIndex;
        }
        else
        {
            bbuSideIndex = bbuIndex - (bbuCount/2);
        }
        fbe_cli_printf("BBU%s%d\n", 
                       (bbu_status_info.bobInfo.associatedSp == SP_A ? "A":"B"),
                       bbuSideIndex);

        fbe_cli_printf("\tSlotNumOnSpBlade    : %d\n", bbu_status_info.bobInfo.slotNumOnSpBlade);
        fbe_cli_printf("\tAssociatedSp        : %d\n", bbu_status_info.bobInfo.associatedSp);
        fbe_cli_printf("\tInserted            : %d\n", bbu_status_info.bobInfo.batteryInserted);
        if (bbu_status_info.bobInfo.batteryInserted)
        {
            fbe_cli_printf("\tState               : %s\n", fbe_sps_mgmt_decode_bbu_state(bbu_status_info.bobInfo.state));
            fbe_cli_printf("\tSubState            : %s\n", fbe_sps_mgmt_decode_bbu_subState(bbu_status_info.bobInfo.subState));
            fbe_cli_printf("\tBatteryEnabled      : %d\n", bbu_status_info.bobInfo.batteryEnabled);
            fbe_cli_printf("\tBatteryState        : %s\n", 
                           fbe_sps_mgmt_getBobStateString(bbu_status_info.bobInfo.batteryState));
            fbe_cli_printf("\tBatteryChargeState  : %s\n", 
                           decodeBatteryChargeState(bbu_status_info.bobInfo.batteryChargeState));
            fbe_cli_printf("\tBatteryFaults       : %s\n", 
                           fbe_sps_mgmt_getBobFaultString(bbu_status_info.bobInfo.batteryFaults));
            fbe_cli_printf("\tBatteryEnvStatus    : %s\n",
                           fbe_base_env_decode_env_status(bbu_status_info.bobInfo.envInterfaceStatus));
            fbe_cli_printf("\tTestStatus          : %s\n", 
                           fbe_sps_mgmt_getBobTestStatusString(bbu_status_info.bobInfo.batteryTestStatus));
            fbe_cli_printf("\tDeliverableCapacity : %d.%02d WHr\n", bbu_status_info.bobInfo.batteryDeliverableCapacity / 100, bbu_status_info.bobInfo.batteryDeliverableCapacity % 100);
            fbe_cli_printf("\tStorageCapacity     : %d.%02d WHr\n", bbu_status_info.bobInfo.batteryStorageCapacity / 100, bbu_status_info.bobInfo.batteryStorageCapacity % 100);
            fbe_cli_printf("\tEnergyRequirements \n");
            fbe_cli_printf("\t  Energy            : %d.%02d WHr\n", bbu_status_info.bobInfo.energyRequirement.energy / 100, bbu_status_info.bobInfo.energyRequirement.energy % 100);
            fbe_cli_printf("\t  MaxLoad           : %d.%1d W\n", bbu_status_info.bobInfo.energyRequirement.maxLoad / 10, bbu_status_info.bobInfo.energyRequirement.maxLoad % 10);
            fbe_cli_printf("\t  Power             : %d.%1d W\n", bbu_status_info.bobInfo.energyRequirement.power / 10, bbu_status_info.bobInfo.energyRequirement.power % 10);
            fbe_cli_printf("\t  Time              : %d Sec\n", bbu_status_info.bobInfo.energyRequirement.time);
            fbe_cli_printf("\tFirmwareRevision    : %02d.%02d\n", bbu_status_info.bobInfo.firmwareRevMajor, bbu_status_info.bobInfo.firmwareRevMinor);
            fbe_cli_printf("\tFaultReg Fault      : %s\n", (bbu_status_info.bobInfo.isFaultRegFail ? "YES" : "NO"));

            if (bbu_status_info.bobInfo.hasResumeProm)
            {
    
            }
        }
        
    }

    // get status of setting Drive BatteryBacked attribute
    status = fbe_api_get_enclosure_object_id_by_location(0,0, &objectId);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error in getEnclObjIdByLoc, error code: %d\n", __FUNCTION__, status);
        return;
    }

    status = fbe_api_enclosure_get_disk_battery_backed_status(objectId, &diskBatteryBackedSet);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("\n\tDiskBatteryBacked : %d\n", diskBatteryBackedSet);
    }
    else
    {
        fbe_cli_printf("%s: Error in getDiskBatteryBackedStatus, error code: %d\n", __FUNCTION__, status);
        return;
    }

    /*  Get TestTimeInfo for SPS    */
    fbe_set_memory(&spsTestTimeInfo, 0, sizeof(fbe_esp_sps_mgmt_spsTestTime_t));
    status = fbe_api_esp_sps_mgmt_getSpsTestTime(&spsTestTimeInfo);
    if(status == FBE_STATUS_OK)
    {
        fbe_getTimeString(&(spsTestTimeInfo.spsTestTime),timeString);
        fbe_cli_printf ("\nWeekly BBU Test Time: %s %s\n",fbe_getWeekDayString(spsTestTimeInfo.spsTestTime.weekday),
                        timeString);
    }

}
/************************************************************************
 **                       End of fbe_cli_cmd_bbuinfo ()                **
 ************************************************************************/


/*************************************************************************
 *                            @fn fbe_cli_processTestTime ()                 *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_processTestTime - take time parameters, validates them &
 *   saves the new test time.
 *
 * @param :
 *   day      (INPUT) day value
 *   hour     (INPUT) hour value
 *   minute   (INPUT) minute value
 *
 * @return:
 *   None
 *
 * @author
 *   11-Nov-2012 Created - Joe Perry
 *
 *************************************************************************/
static void fbe_cli_processTestTime(fbe_u16_t day, fbe_u16_t hour, fbe_u16_t minute)
{
    fbe_esp_sps_mgmt_spsTestTime_t  spsTestTimeInfo;
    fbe_status_t                    status;

    // validate day, hour, minute values
    if (day > 6)
    {
        fbe_cli_printf("Invalid Day parameter (0-6)\n");
        return;
    }
    if (hour > 23)
    {
        fbe_cli_printf("Invalid Hour parameter (0-23)\n");
        return;
    }
    if (minute > 59)
    {
        fbe_cli_printf("Invalid Minute parameter (0-59)\n");
        return;
    }

    // send new test time to ESP
    fbe_zero_memory(&spsTestTimeInfo, sizeof (fbe_esp_sps_mgmt_spsTestTime_t));
    spsTestTimeInfo.spsTestTime.weekday = day;
    spsTestTimeInfo.spsTestTime.hour = hour;
    spsTestTimeInfo.spsTestTime.minute = minute;
    status = fbe_api_esp_sps_mgmt_setSpsTestTime(&spsTestTimeInfo);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("SPS/BBU Test Time successfully set\n");
    }
    else
    {
        fbe_cli_printf("Failed to set SPS/BBU Test Time, status %d\n", status);
    }

}   // end of fbe_cli_processTestTime


