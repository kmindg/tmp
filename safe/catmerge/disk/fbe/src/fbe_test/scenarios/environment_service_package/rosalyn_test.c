/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file rosalyn_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests EIR (Energy Info Reporting) from enclosures.
 *
 * @version
 *   02/09/2011 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_power_supply_interface.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * rosalyn_short_desc = "Rosalyn_Test: ESP Energy Information Reporting for Array";
char * rosalyn_long_desc ="\
rosalyn: \n\
\n\
        This tests validates ESP's EIR Statistics. \n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n"
"STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. SPS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Validate the EIR Stats\n\
        - Verify that all the services are getting initalized\n\
        - Verify that FBE API object map is initiaized\n\
        - Verify that all the classes mentioned above are initialized and loaded\n\
        - Wait a few minutes for EIR Samples\n\
        - Request all EIR Stats (Enclosures, SPS's, Array)";

#define ROSALYN_NEW_INPUT_POWER_0       1500
#define ROSALYN_NEW_INPUT_POWER_1       500
#define ROSALYN_NEW_INPUT_POWER_TOTAL   (ROSALYN_NEW_INPUT_POWER_0 + ROSALYN_NEW_INPUT_POWER_1)
#define ROSALYN_NEW_AIR_INLET_TEMP      28

fbe_u32_t rosalyn_getEnclosureEirData(void)
{
    fbe_status_t                        status;
    fbe_u32_t                           enclCount, enclIndex;
    fbe_esp_encl_mgmt_get_encl_loc_t    enclLocationInfo;
    fbe_esp_encl_mgmt_get_eir_info_t    enclEirInfo;
    fbe_u32_t                           inputPowerSum = 0;

    status = fbe_api_esp_encl_mgmt_get_total_encl_count(&enclCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for (enclIndex = 0; enclIndex < enclCount; enclIndex++)
    {
        enclLocationInfo.enclIndex = enclIndex;
        mut_printf(MUT_LOG_LOW, "  %s, Getting enclosure location for index %d\n", 
                   __FUNCTION__, enclIndex);
        status = fbe_api_esp_encl_mgmt_get_encl_location(&enclLocationInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        enclEirInfo.eirInfoType = FBE_ENCL_MGMT_EIR_INDIVIDUAL;
        enclEirInfo.eirEnclInfo.eirEnclIndividual.location = enclLocationInfo.location;
        status = fbe_api_esp_encl_mgmt_get_eir_info(&enclEirInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "    EIR Data, Location %d %d InputPower:\n",
                                        enclLocationInfo.location.bus,
                                        enclLocationInfo.location.enclosure);
        mut_printf(MUT_LOG_TEST_STATUS, "  \tCurrent %d, status 0x%x\n",
                   enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower,
                   enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
        mut_printf(MUT_LOG_TEST_STATUS, "  \tAverage %d, status 0x%x\n",
                   enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower,
                   enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status);
        mut_printf(MUT_LOG_TEST_STATUS, "  \tMax %d, status 0x%x\n",
                   enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower,
                   enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status);
        if (enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status
            == FBE_ENERGY_STAT_VALID)
        {
            inputPowerSum += 
                enclEirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower;
        }
    }

    return inputPowerSum;
}

fbe_u32_t rosalyn_getSpsEirData(void)
{
    fbe_status_t                        status;
    fbe_u32_t                           inputPowerSum = 0;
    fbe_u8_t                            spsIndex;
    fbe_esp_sps_mgmt_spsInputPower_t    spsInputPowerInfo;

    for (spsIndex = 0; spsIndex < FBE_SPS_ARRAY_MAX_COUNT; spsIndex++)
    {
        // determine if SPS in xPE or DAE0 (2 SPS's per enclosure)
        if (spsIndex <  2)
        {
            spsInputPowerInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM; 
            spsInputPowerInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        else
        {
            spsInputPowerInfo.spsLocation.bus = 0; 
            spsInputPowerInfo.spsLocation.enclosure = 0;
        }
        spsInputPowerInfo.spsLocation.slot = (spsIndex % 2);

        status = fbe_api_esp_sps_mgmt_getSpsInputPower(&spsInputPowerInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "  %s:SPS EIR Data, sps %d, InputPower:\n",
                   __FUNCTION__, spsIndex);
        mut_printf(MUT_LOG_TEST_STATUS, "  \tCurrent %d, status 0x%x\n",
                   spsInputPowerInfo.spsCurrentInputPower.inputPower,
                   spsInputPowerInfo.spsCurrentInputPower.status);
        mut_printf(MUT_LOG_TEST_STATUS, "  \tAverage %d, status 0x%x\n",
                   spsInputPowerInfo.spsAverageInputPower.inputPower,
                   spsInputPowerInfo.spsAverageInputPower.status);
        mut_printf(MUT_LOG_TEST_STATUS, "  \tMax %d, status 0x%x\n",
                   spsInputPowerInfo.spsMaxInputPower.inputPower,
                   spsInputPowerInfo.spsMaxInputPower.status);
        if (spsInputPowerInfo.spsCurrentInputPower.status == FBE_ENERGY_STAT_VALID)
        {
            inputPowerSum += spsInputPowerInfo.spsCurrentInputPower.inputPower;
        }
    }

    return inputPowerSum;

}


/*!**************************************************************
 * rosalyn_test()
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   05/05/2010 - Created. Dan McFarland
 *
 ****************************************************************/

void rosalyn_test(void)
{
    fbe_status_t                        status;
    fbe_u32_t                           inputPowerSum = 0;
    fbe_u32_t                           originalInputPower = 0;
    fbe_eir_data_t                      arrayEirInfo;
    SPECL_SFI_MASK_DATA                 sfi_mask_data;

    mut_printf(MUT_LOG_TEST_STATUS, "ROSALYN: start rosalyn_test\n");

    mut_printf(MUT_LOG_LOW, "ROSALYN:Wait for initial SPS EIR Readings\n");
    fbe_api_sleep (120000);

    /*
     * Request Enclosure EIR Data
     */
    inputPowerSum = rosalyn_getEnclosureEirData();
    mut_printf(MUT_LOG_TEST_STATUS, "ROSALYN:\tinputPowerSum (Enclosures) %d\n", inputPowerSum);
    originalInputPower = inputPowerSum;

    /*
     * Request SPS EIR Data
     */
    inputPowerSum += rosalyn_getSpsEirData();
    mut_printf(MUT_LOG_TEST_STATUS, "ROSALYN:\tinputPowerSum (Enclosures + SPS's) %d\n", inputPowerSum);

    /*
     * Request Array EIR Data & verify against other EIR Data
     */
    status = fbe_api_esp_eir_getData(&arrayEirInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "ROSALYN:EIR Data, Array InputPower %d, status 0x%x\n",
               arrayEirInfo.arrayInputPower.inputPower,
               arrayEirInfo.arrayInputPower.status);
   MUT_ASSERT_TRUE((FBE_ENERGY_STAT_VALID | FBE_ENERGY_STAT_UNSUPPORTED) == 
                   arrayEirInfo.arrayInputPower.status);
   MUT_ASSERT_TRUE(inputPowerSum == arrayEirInfo.arrayInputPower.inputPower);

   /*
    * Change PE EIR values
    */
    mut_printf(MUT_LOG_TEST_STATUS, "ROSALYN: Update PE EIR Data\n");
    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[0].psStatus[0].inputPower = ROSALYN_NEW_INPUT_POWER_0;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[0].psStatus[1].inputPower = ROSALYN_NEW_INPUT_POWER_0;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[1].psStatus[0].inputPower = ROSALYN_NEW_INPUT_POWER_1;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[1].psStatus[1].inputPower = ROSALYN_NEW_INPUT_POWER_1;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    mut_printf(MUT_LOG_LOW, "ROSALYN: delay for EIR change to be detected\n");
    fbe_api_sleep (90000);
    mut_printf(MUT_LOG_LOW, "ROSALYN: delay for EIR change to be detected - Done\n");

    inputPowerSum = rosalyn_getEnclosureEirData();
    mut_printf(MUT_LOG_TEST_STATUS, "ROSALYN:EIR Data, Confirm updated Enclosure InputPower %d\n",
               inputPowerSum);
    MUT_ASSERT_TRUE(inputPowerSum != originalInputPower)

    mut_printf(MUT_LOG_LOW, "ROSALYN: SPS EIR Readings Successful\n");
    return;
}
/******************************************
 * end rosalyn_test()
 ******************************************/
void rosalyn_setup(void)
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_FP_CONIG,
                                                      SPID_PROMETHEUS_M1_HW_TYPE,
                                                      NULL,
                                                      fbe_test_reg_set_persist_ports_true);
}

void rosalyn_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file rosalyn_test.c
 *************************/


