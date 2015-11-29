/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_energy_stats_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli functions for  info related to statistics reporting.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  02/25/2011:  Rashmi Sawale - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_lib_energy_stats_cmds.h"
#include "fbe_api_esp_ps_mgmt_interface.h"
#include "fbe_api_esp_board_mgmt_interface.h"

/*************************************************************************
 *                            @fn fbe_cli_cmd_eirstat ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_eirstat performs the execution of the EIRSTAT command
 *   Energy Info Reporting stats - Displays summary of EIR stats
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   03/01/2011 Created - Rashmi Sawale
 *   05/08/2012 Added "-v" and "-support" option support - Dongz
 *
 *************************************************************************/

void fbe_cli_cmd_eirstat(fbe_s32_t argc,char ** argv)
{
    /****************************
    **    VARIABLE DECLARATION   **
    ****************************/
    BOOL    displayInputPower = TRUE;
    BOOL    displayAirInletTemp = TRUE;
    BOOL    displayArrayUtil = TRUE;
    BOOL    displayVerbose = FALSE;

    /*****************
    **    BEGIN    **
    *****************/
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", EIR_USAGE);
        return;
    }

    if (argc > 3)
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_print_usage();
        return;
    }

    while (*argv)
    {
        if (!strcmp(*argv, "-ip"))                // Input Power stats only
        {
            displayAirInletTemp = FALSE;
            displayArrayUtil = FALSE;
            argv++;
        }
        else if (!strcmp(*argv, "-ait"))          // Air Inlet Temperature stats only
        {
            displayInputPower = FALSE;
            displayArrayUtil = FALSE;
            argv++;
        }
        else if (!strcmp(*argv, "-v"))                 // Verbose (show samples)
        {
            displayVerbose = TRUE;
            argv++;
        }
        else if (!strcmp(*argv, "-support"))          // show PS supported status
        {
            fbe_cli_displayPsSupportedStatus();
            return;
        }
        else
        {
            fbe_cli_print_usage();
            return;
        }
    }   // end of while
        /*
     * Display General Statistics info
     */
    fbe_cli_printf("Statistics Information\n");
    fbe_cli_printf("\tPolling Interval (in seconds) : %d\n", FBE_STATS_SAMPLE_INTERVAL);
    fbe_cli_printf("\tSliding Window Duration (in seconds) : %d\n", FBE_STATS_WINDOW_SIZE);
    fbe_cli_printf("\tSample Count : %d\n", FBE_SAMPLE_DATA_SIZE);
    //TODO sample index?

    /*
     * Display Input Power Stats
     */
    if (displayInputPower)
    {
        fbe_cli_displayInputPowerStats(displayVerbose);
    }

    /*
     * Display Air Inlet Temperature Stats
     */
    if (displayAirInletTemp)
    {
        fbe_cli_displayAirInletTempStats(displayVerbose);
    }
}

/************************************************************************
 **                       End of fbe_cli_cmd_ioports ()                **
 ************************************************************************/

/*!**************************************************************
 * fbe_cli_displayInputPowerStats()
 ****************************************************************
 * @brief
 *      Displays Input Power statistics of Array, SPS and Enclosures.
 *
 *  @param :
 *      BOOL                  displayVerbose
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/01/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_displayInputPowerStats(BOOL displayVerbose)
{
    fbe_eir_data_t                                     arrayEirInfo;
    fbe_eir_input_power_sample_t                       arraySampleDataInfo[FBE_SAMPLE_DATA_SIZE];
    fbe_esp_encl_mgmt_eir_input_power_sample_t         enclInputSampleData;
    fbe_u32_t                                          sampleIndex;
    fbe_status_t                                       status = FBE_STATUS_OK;
    fbe_u8_t                                           enclIndex;
    fbe_status_t                                       enclStatus = FBE_STATUS_OK;
    fbe_status_t                                       enclLocationStatus = FBE_STATUS_OK;
    fbe_u32_t                                          enclCount;
    fbe_esp_encl_mgmt_get_encl_loc_t                   enclLocationInfo;
    fbe_esp_encl_mgmt_get_eir_info_t                   enclEirInfo;
    fbe_esp_sps_mgmt_spsCounts_t                       spsCountInfo;
    fbe_device_physical_location_t                     location;
    HW_ENCLOSURE_TYPE                                  enclosureType;
    fbe_esp_board_mgmt_board_info_t                    boardInfo;
    char                                               enclEirStatusString[EIR_STAT_STAUS_STRING_LENGTH];

    //get enclosure type
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, error code: %d.\n", __FUNCTION__, status);
        return;
    }
    if(boardInfo.platform_info.enclosureType > MAX_ENCLOSURE_TYPE)
    {
        fbe_cli_printf("Illegal enclosure type: %d.\n", boardInfo.platform_info.enclosureType);
        return;
    }

    enclosureType = boardInfo.platform_info.enclosureType;

    status = fbe_api_esp_eir_getData(&arrayEirInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to retrive data !!! status: %d\n", status);
        return;
    }

    /*
     * Display Array statistics
     */
    fbe_cli_printf("\n\nInput Power Statistics\n");
    fbe_cli_printf("\tArray stats (Input Power in watts)\n");

    fbe_cli_decodeStatsStatus(arrayEirInfo.arrayInputPower.status, enclEirStatusString);
    fbe_cli_printf("\t\tCurrent : %d\tstatus : %s\n",
            arrayEirInfo.arrayInputPower.inputPower,
            enclEirStatusString);

    fbe_cli_decodeStatsStatus(arrayEirInfo.arrayMaxInputPower.status, enclEirStatusString);
    fbe_cli_printf("\t\tMaximum : %d\tstatus : %s\n",
            arrayEirInfo.arrayMaxInputPower.inputPower,
            enclEirStatusString);

    fbe_cli_decodeStatsStatus(arrayEirInfo.arrayAvgInputPower.status, enclEirStatusString);
    fbe_cli_printf("\t\tAverage : %d\tstatus : %s\n",
            arrayEirInfo.arrayAvgInputPower.inputPower,
            enclEirStatusString);
    if (displayVerbose)
    {
        fbe_zero_memory(arraySampleDataInfo, sizeof(fbe_eir_input_power_sample_t)*FBE_SAMPLE_DATA_SIZE);
        status = fbe_api_esp_eir_getSample(arraySampleDataInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to retrive sample data !!! status: %d\n", status);
            return;
        }
        fbe_cli_printf("\t\tSamples for Array\n");
        for (sampleIndex = 0; sampleIndex < FBE_SAMPLE_DATA_SIZE; sampleIndex++)
        {
            if (arraySampleDataInfo[sampleIndex].status != FBE_ENERGY_STAT_UNINITIALIZED)
            {
                fbe_cli_decodeStatsStatus(arraySampleDataInfo[sampleIndex].status, enclEirStatusString);
                fbe_cli_printf("\t\t\tsampleIndex %d, inputPower %d, status %s\n",
                    sampleIndex,
                    arraySampleDataInfo[sampleIndex].inputPower,
                    enclEirStatusString);
            }
        }
    }

    /*
     * Display SPS statistics
     */
    fbe_cli_printf("\tSPS stats (Input Power in watts)\n");
    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);

    /* This is for Megatron PE. Megatron has 2 enclosures with SPS */
    if (spsCountInfo.enclosuresWithSps > 1)
    {
        fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        fbe_cli_displaySpsInputPowerStatus(location, displayVerbose);
    }

    /* If some platform has enclosures with SPS, there must has one in DAE0 */
    if (spsCountInfo.enclosuresWithSps > 0)
    {
        fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
        location.bus = 0;
        location.enclosure = 0;
        fbe_cli_displaySpsInputPowerStatus(location, displayVerbose);
    }

    /*
     * Display Enclosure statistics
     */
    enclStatus = fbe_api_esp_encl_mgmt_get_total_encl_count(&enclCount);
    if(enclStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to retrive data !!!\n");
        return;
    }

    for (enclIndex = 0; enclIndex < enclCount; enclIndex++)
    {
        enclLocationInfo.enclIndex = enclIndex;
        enclLocationStatus = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);

        if(enclLocationStatus != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to retrive data !!!\n");
            return;
        }

        enclEirInfo.eirInfoType = FBE_ENCL_MGMT_EIR_INDIVIDUAL;
        enclEirInfo.eirEnclInfo.eirEnclIndividual.location = enclLocationInfo.location;
        enclStatus = fbe_api_esp_encl_mgmt_get_eir_info(&enclEirInfo);
        if(enclStatus != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to retrive data !!!\n");
            return;
        }

       if(((enclosureType == XPE_ENCLOSURE_TYPE) && 
            (enclLocationInfo.location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (enclLocationInfo.location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
           ((enclosureType == DPE_ENCLOSURE_TYPE) && 
            (enclLocationInfo.location.bus == 0) && (enclLocationInfo.location.enclosure == 0)))
       {
           fbe_cli_printf("\n\tProcessor Enclosure, stats (Input Power in watts)\n");
       }
       else
       {
           fbe_cli_printf("\n\tEnclosure %d, stats (Input Power in watts)\n",enclLocationInfo.enclIndex);
       }

       fbe_cli_decodeStatsStatus(enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status, enclEirStatusString);
       fbe_cli_printf("\t\tCurrent : %d\tstatus : %s\n", 
                              enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower,
                              enclEirStatusString);

       fbe_cli_decodeStatsStatus(enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status, enclEirStatusString);
       fbe_cli_printf("\t\tMaximum : %d\tstatus : %s\n", 
                              enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower,
                              enclEirStatusString);

       fbe_cli_decodeStatsStatus(enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status, enclEirStatusString);
       fbe_cli_printf("\t\tAverage : %d\tstatus : %s\n", 
                              enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower,
                              enclEirStatusString);
    }

    /*
     * Display individual samples
     */
    if (displayVerbose)
    {
        fbe_cli_printf("\t\tSamples for Array\n");
        for (enclIndex = 0; enclIndex < enclCount; enclIndex++)
        {
            enclLocationInfo.enclIndex = enclIndex;
            enclLocationStatus = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);
            if(enclLocationStatus != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nFailed to retrive data !!!\n");
                return;
            }

            //get encl input power samples
            fbe_zero_memory(&enclInputSampleData, sizeof(fbe_esp_encl_mgmt_eir_input_power_sample_t));

            enclInputSampleData.location = enclLocationInfo.location;

            status = fbe_api_esp_encl_mgmt_get_eir_input_power_sample(&enclInputSampleData);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nFailed to retrive encl input power sample data !!! status: %d\n", status);
                return;
            }

            if(((enclosureType == XPE_ENCLOSURE_TYPE) && 
                 (enclLocationInfo.location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (enclLocationInfo.location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
                ((enclosureType == DPE_ENCLOSURE_TYPE) && 
                 (enclLocationInfo.location.bus == 0) && (enclLocationInfo.location.enclosure == 0)))
            {
                fbe_cli_printf("\tSamples for Processor Enclosure\n");
            }
            else
            {
                fbe_cli_printf("\tSamples for Enclosure %d\n",enclLocationInfo.enclIndex);
            }

            for (sampleIndex = 0; sampleIndex < FBE_SAMPLE_DATA_SIZE; sampleIndex++)
            {
                if (enclInputSampleData.inputSample[sampleIndex].status != FBE_ENERGY_STAT_UNINITIALIZED)
                {
                    fbe_cli_decodeStatsStatus(enclInputSampleData.inputSample[sampleIndex].status, enclEirStatusString);
                    fbe_cli_printf("\t\t\tsampleIndex %d, inputPower %d, status %s\n",
                        sampleIndex,
                        enclInputSampleData.inputSample[sampleIndex].inputPower,
                        enclEirStatusString);
                }
            }
        }
    }
}
/******************************************************
 * end fbe_cli_displayInputPowerStats() 
 ******************************************************/

/*!**************************************************************
 * fbe_cli_displaySpsInputPowerStatus()
 ****************************************************************
 * @brief
 *      Displays SPS Input Power statistics.
 *
 *  @param :
 *      location       - sps location
 *      displayVerbose - if sample data should be displayed
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/13/2013: Lin Lou Created.
 *
 ****************************************************************/
static void fbe_cli_displaySpsInputPowerStatus(fbe_device_physical_location_t location, BOOL displayVerbose)
{
    fbe_u32_t                                          sampleIndex;
    fbe_status_t                                       status = FBE_STATUS_OK;
    fbe_u8_t                                           spsIndex;
    fbe_esp_sps_mgmt_spsInputPower_t                   spsInputPowerInfo;
    fbe_esp_sps_mgmt_spsInputPowerSample_t             spsInputSampleData;
    char                                               enclEirStatusString[EIR_STAT_STAUS_STRING_LENGTH];

    for (spsIndex = FBE_LOCAL_SPS; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
    {
        location.slot = spsIndex;
        spsInputPowerInfo.spsLocation = location;
        status = fbe_api_esp_sps_mgmt_getSpsInputPower(&spsInputPowerInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to retrive data !!!\n");
            return;
        }

        fbe_cli_printf("\t%s\n", fbe_sps_mgmt_getSpsString(&location));

        fbe_cli_decodeStatsStatus(spsInputPowerInfo.spsCurrentInputPower.status, enclEirStatusString);
        fbe_cli_printf("\t\tCurrent : %d\tstatus : %s\n",
                               spsInputPowerInfo.spsCurrentInputPower.inputPower,
                               enclEirStatusString);
        fbe_cli_decodeStatsStatus(spsInputPowerInfo.spsMaxInputPower.status, enclEirStatusString);
        fbe_cli_printf("\t\tMaximum : %d\tstatus : %s\n",
                               spsInputPowerInfo.spsMaxInputPower.inputPower,
                               enclEirStatusString);
        fbe_cli_decodeStatsStatus(spsInputPowerInfo.spsAverageInputPower.status, enclEirStatusString);
        fbe_cli_printf("\t\tAverage : %d\tstatus : %s\n",
                               spsInputPowerInfo.spsAverageInputPower.inputPower,
                               enclEirStatusString);

        if (displayVerbose)
        {
            fbe_zero_memory(&spsInputSampleData, sizeof(fbe_esp_sps_mgmt_spsInputPowerSample_t));
            spsInputSampleData.spsLocation = location;
            status = fbe_api_esp_sps_mgmt_getSpsInputPowerSample(&spsInputSampleData);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nFailed to retrive sps sample data !!! status: %d\n", status);
                continue;
            }
            fbe_cli_printf("\t\tSamples for %s\n", fbe_sps_mgmt_getSpsString(&location));

            for (sampleIndex = 0; sampleIndex < FBE_SAMPLE_DATA_SIZE; sampleIndex++)
            {
                if (spsInputSampleData.spsInputPowerSamples[sampleIndex].status != FBE_ENERGY_STAT_UNINITIALIZED)
                {
                    fbe_cli_decodeStatsStatus(spsInputSampleData.spsInputPowerSamples[sampleIndex].status, enclEirStatusString);
                    fbe_cli_printf("\t\t\tsampleIndex %d, inputPower %d, status %s\n",
                            sampleIndex,
                            spsInputSampleData.spsInputPowerSamples[sampleIndex].inputPower,
                            enclEirStatusString);
                }
            }
        }
    }
}
/******************************************************
 * end fbe_cli_displaySpsInputPowerStatus()
 ******************************************************/

/*!**************************************************************
 * fbe_cli_displayAirInletTempStats()
 ****************************************************************
 * @brief
 *      Displays Air Inlet temp statistics of Enclosures.
 *
 *  @param :
 *      BOOL                  displayVerbose
 *
 *  @return:
 *      Nothing
 *
 * @author
 *  03/01/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
static void fbe_cli_displayAirInletTempStats(BOOL displayVerbose)
{
    fbe_status_t                          enclStatus = FBE_STATUS_OK;
    fbe_status_t                          enclLocationStatus = FBE_STATUS_OK;
    fbe_u32_t                             enclCount, enclIndex;
    fbe_esp_encl_mgmt_get_encl_loc_t      enclLocationInfo;
    fbe_esp_encl_mgmt_get_eir_info_t      enclEirInfo;
    fbe_esp_encl_mgmt_eir_temp_sample_t   enclEirTempInfo;
    HW_ENCLOSURE_TYPE                     enclosureType;
    fbe_esp_board_mgmt_board_info_t       boardInfo;
    fbe_u32_t                             sampleIndex;
    char                                  enclEirStatusString[EIR_STAT_STAUS_STRING_LENGTH];

    fbe_cli_printf("\nAir Inlet Temperature Statistics\n");

    //get enclosure type
    enclStatus = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if(enclStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, error code: %d.\n", __FUNCTION__, enclStatus);
        return;
    }
    if(boardInfo.platform_info.enclosureType > MAX_ENCLOSURE_TYPE)
    {
        fbe_cli_printf("Illegal enclosure type: %d.\n", boardInfo.platform_info.enclosureType);
        return;
    }

    enclosureType = boardInfo.platform_info.enclosureType;

    enclStatus = fbe_api_esp_encl_mgmt_get_total_encl_count(&enclCount);
    if(enclStatus != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to retrive data !!!\n");
        return;
    }

    for (enclIndex = 0; enclIndex < enclCount; enclIndex++)
    {
        enclLocationInfo.enclIndex = enclIndex;
        enclLocationStatus = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);
        if(enclLocationStatus != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to retrive data !!!\n");
            return;
        }

        enclEirInfo.eirInfoType = FBE_ENCL_MGMT_EIR_INDIVIDUAL;
        enclEirInfo.eirEnclInfo.eirEnclIndividual.location = enclLocationInfo.location;
        enclStatus = fbe_api_esp_encl_mgmt_get_eir_info(&enclEirInfo);
        if(enclStatus != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed to retrive data !!!\n");
            return;
        }
        fbe_cli_printf("Index %d, bus %d, encl %d\n",
                enclIndex, enclLocationInfo.location.bus, enclLocationInfo.location.enclosure);
        if(((enclosureType == XPE_ENCLOSURE_TYPE) && 
             (enclLocationInfo.location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (enclLocationInfo.location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
            ((enclosureType == DPE_ENCLOSURE_TYPE) && 
             (enclLocationInfo.location.bus == 0) && (enclLocationInfo.location.enclosure == 0)))
        {
            fbe_cli_printf("\tProcessor Enclosure, stats (Air Inlet Temperature in degrees C)\n");
        }
        else
        {
            fbe_cli_printf("\tEnclosure %d, stats (Air Inlet Temperature in degrees C)\n",enclLocationInfo.enclIndex);
        }

        fbe_cli_decodeStatsStatus(enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentAirInletTemp.status, enclEirStatusString);
        fbe_cli_printf("\t\tCurrent : %d\tstatus : %s\n", 
                              enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentAirInletTemp.airInletTemp,
                              enclEirStatusString);

        fbe_cli_decodeStatsStatus(enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxAirInletTemp.status, enclEirStatusString);
        fbe_cli_printf("\t\tMaximum : %d\tstatus : %s\n", 
                              enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxAirInletTemp.airInletTemp,
                              enclEirStatusString);

        fbe_cli_decodeStatsStatus(enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageAirInletTemp.status, enclEirStatusString);
        fbe_cli_printf("\t\tAverage : %d\tstatus : %s\n", 
                              enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageAirInletTemp.airInletTemp,
                              enclEirStatusString);
        
    }

    /*
     * Display individual samples
     */
    if (displayVerbose)
    {
        fbe_cli_printf("\t\tSamples for Array\n");
        for (enclIndex = 0; enclIndex < enclCount; enclIndex++)
        {
            enclLocationInfo.enclIndex = enclIndex;
            enclLocationStatus = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);
            if(enclLocationStatus != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nFailed to retrive data !!!\n");
                return;
            }

            //get encl input power samples
            fbe_zero_memory(&enclEirTempInfo, sizeof(fbe_esp_encl_mgmt_eir_temp_sample_t));

            enclEirTempInfo.location = enclLocationInfo.location;

            enclStatus = fbe_api_esp_encl_mgmt_get_eir_temp_sample(&enclEirTempInfo);
            if(enclStatus != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nFailed to retrive encl temp sample data !!! status: %d\n", enclStatus);
                return;
            }

            if(((enclosureType == XPE_ENCLOSURE_TYPE) && 
                 (enclLocationInfo.location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (enclLocationInfo.location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
                ((enclosureType == DPE_ENCLOSURE_TYPE) && 
                 (enclLocationInfo.location.bus == 0) && (enclLocationInfo.location.enclosure == 0)))
            {
                fbe_cli_printf("\tSamples for Processor Enclosure\n");
            }
            else
            {
                fbe_cli_printf("\tSamples for Enclosure %d\n",enclLocationInfo.enclIndex);
            }

            for (sampleIndex = 0; sampleIndex < FBE_SAMPLE_DATA_SIZE; sampleIndex++)
            {
                if (enclEirTempInfo.airInletTempSamples[sampleIndex].status != FBE_ENERGY_STAT_UNINITIALIZED)
                {
                    fbe_cli_decodeStatsStatus(enclEirTempInfo.airInletTempSamples[sampleIndex].status, enclEirStatusString);
                    fbe_cli_printf("\t\t\tsampleIndex %d, airInletTemp %d, status %s\n",
                            sampleIndex,
                            enclEirTempInfo.airInletTempSamples[sampleIndex].airInletTemp,
                            enclEirStatusString);
                }
            }
        }
    }
}
/******************************************************
 * end fbe_cli_displayAirInletTempStats() 
 ******************************************************/


/*!***************************************************************
 * @fn fbe_cli_decodeStatsStatus(fbe_u32_t statsStatus;)
 ****************************************************************
 * @brief
 *      This function decodes the EIR stats status
 *
 * @param
 *      statsStatus (INPUT) EIR stats status
 *      stringPtr (OUTPUT) char string of statsStatus.
 *
 * @return
 *      none.
 *
 * @author
 *  03/01/2011:  Created. Rashmi Sawale
 *  02/22/2013:  Modified. Lin Lou
 *
 ****************************************************************/
void fbe_cli_decodeStatsStatus(fbe_u32_t statsStatus, char *stringPtr)
{
    memset (stringPtr, 0, EIR_STAT_STAUS_STRING_LENGTH);
    if (statsStatus == FBE_ENERGY_STAT_INVALID)
    {
        strncpy(stringPtr, "Invalid ", EIR_STAT_STAUS_STRING_LENGTH);
    }
    else if (statsStatus == FBE_ENERGY_STAT_UNINITIALIZED)
    {
        strncpy(stringPtr, "UnInitialized ", EIR_STAT_STAUS_STRING_LENGTH);
    }
    else
    {
        if (statsStatus & FBE_ENERGY_STAT_VALID)
        {
            strncat(stringPtr, "Valid ", EIR_STAT_STAUS_STRING_LENGTH-strlen(stringPtr)-1);
        }
        if (statsStatus & FBE_ENERGY_STAT_UNSUPPORTED)
        {
            strncat(stringPtr, "Unsupported ", EIR_STAT_STAUS_STRING_LENGTH-strlen(stringPtr)-1);
        }
        if (statsStatus & FBE_ENERGY_STAT_FAILED)
        {
            strncat(stringPtr, "Failed ", EIR_STAT_STAUS_STRING_LENGTH-strlen(stringPtr)-1);
        }
        if (statsStatus & FBE_ENERGY_STAT_NOT_PRESENT)
        {
            strncat(stringPtr, "NotPresent ", EIR_STAT_STAUS_STRING_LENGTH-strlen(stringPtr)-1);
        }
        if (statsStatus & FBE_ENERGY_STAT_SAMPLE_TOO_SMALL)
        {
            strncat(stringPtr, "SampleTooSmall ", EIR_STAT_STAUS_STRING_LENGTH-strlen(stringPtr)-1);
        }
    }
}
/****************************************************************
 * end fbe_cli_decodeStatsStatus()
 ****************************************************************/

/**************************************************************************
 *  fbe_cli_displayPsSupportedStatus()
 **************************************************************************
 *
 *  DESCRIPTION:
 *      This function displays whether the PS's support Energy Star
 *      statistics.
 *
 *  PARAMETERS:
 *      None
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *      Displays status for each PS's in the array.
 *
 *  HISTORY:
 *      4-Sep-2010:  Created by Joe Perry
 *      8-May-2012:  Port to fbecli by Dongz
 *
 **************************************************************************/
void fbe_cli_displayPsSupportedStatus(void)
{
    fbe_u16_t                                          enclIndex;
    fbe_status_t                                       status = FBE_STATUS_OK;
    fbe_u32_t                                          enclCount;
    fbe_u8_t                                          psIndex, psCount;
    fbe_esp_encl_mgmt_encl_info_t                      enclInfo;
    fbe_esp_encl_mgmt_get_encl_loc_t                   enclLocationInfo;
    HW_ENCLOSURE_TYPE                                  enclosureType;
    fbe_esp_board_mgmt_board_info_t                    boardInfo;
    fbe_esp_ps_mgmt_ps_info_t                          psInfo;

    //get enclosure type
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s: Error when get board management info, error code: %d.\n", __FUNCTION__, status);
        return;
    }
    if(boardInfo.platform_info.enclosureType > MAX_ENCLOSURE_TYPE)
    {
        fbe_cli_printf("Illegal enclosure type: %d.\n", boardInfo.platform_info.enclosureType);
        return;
    }

    enclosureType = boardInfo.platform_info.enclosureType;

    status = fbe_api_esp_encl_mgmt_get_total_encl_count(&enclCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nFailed to retrive data !!!\n");
        return;
    }

    fbe_cli_printf("Power Supply - Energy Star Support Status\n");

    /*
     * Display PS status
     */
    for (enclIndex = 0; enclIndex < enclCount; enclIndex++)
    {
        enclLocationInfo.enclIndex = enclIndex;
        status = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nFailed get encl location !!!\n");
            return;
        }
        status = fbe_api_esp_encl_mgmt_get_encl_info(&enclLocationInfo.location, &enclInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_printf("%s: Get enclosure info fail, location: sp %d, bus %d, enclosure %d.\n",
                    __FUNCTION__,
                    enclLocationInfo.location.sp,
                    enclLocationInfo.location.bus,
                    enclLocationInfo.location.enclosure);
            return;
        }

        if(!enclInfo.enclPresent)
        {
            continue;
        }

        psCount = enclInfo.psCount;

        // a processor enclosure
        if ((enclosureType == DPE_ENCLOSURE_TYPE) && 
            (enclLocationInfo.location.bus == 0) && (enclLocationInfo.location.enclosure == 0))
        {
            fbe_cli_printf("\tDPE Power Supply Status (PS count %d)\n", psCount);
        }
        else if ((enclosureType == XPE_ENCLOSURE_TYPE) && 
                 (enclLocationInfo.location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (enclLocationInfo.location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
        {
            fbe_cli_printf("\txPE Power Supply Status (PS count %d)\n", psCount);
        }
        else
        {
            fbe_cli_printf("\tEnclosure %d Power Supply Status (PS count %d)\n", enclLocationInfo.enclIndex, psCount);
        }

        for (psIndex = 0; psIndex < psCount; psIndex++)
        {
            //get ps info
            fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_info_t));
            fbe_copy_memory(&psInfo.location, &enclLocationInfo.location, sizeof(fbe_device_physical_location_t));
            psInfo.location.slot = psIndex;

            status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_printf("%s: Get ps info fail, status: %d.\n", __FUNCTION__, status);
                return;
            }

            if (!psInfo.psInfo.bInserted)
            {
                fbe_cli_printf("\t\tPS %d - not inserted\n", psIndex);
                continue;
            }

            if (psInfo.psInfo.psSupported)
            {
                fbe_cli_printf("\t\tPS %d - Supports Energy Star\n", psIndex);
            }
            else
            {
                fbe_cli_printf("\t\tPS %d - Does NOT Support Energy Star\n", psIndex);
            }
        }
    }

}   // end of fcli_displayPsSupportedStatus
/******************************************************
 * end fbe_cli_displayStatsStatus() 
 ******************************************************/

