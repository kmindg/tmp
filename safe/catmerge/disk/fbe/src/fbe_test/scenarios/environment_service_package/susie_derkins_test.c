/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file susie_derkins_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests if all the related services are getting
 *  started correctly and FBE API getting initialized
 *
 * @version
 *   11/24/2010 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_board_interface.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * susie_derkins_short_desc = "susie_derkins_Test: ESP Energy Information Reporting for SPS's";
char * susie_derkins_long_desc ="\
susie_derkins: \n\
\n\
        This tests validates that the eir service's retrieving of SPS InputPower. \n\
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
STEP 2: Validate the starting of Service\n\
        - Verify that all the services are getting initalized\n\
        - Verify that FBE API object map is initiaized\n\
        - Verify that all the classes mentioned above are initialized and loaded";



/*!**************************************************************
 * susie_derkins_test()
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

void susie_derkins_test(void)
{
    fbe_status_t status;
    fbe_object_id_t                         objectId;
    fbe_u8_t                                spsIndex;
    fbe_eir_input_power_sample_t            expectedInputPower[FBE_SPS_MAX_COUNT];
    fbe_esp_sps_mgmt_spsInputPower_t        spsInputPowerInfo;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;

    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_LOW, "SUSIE DERKINS:Wait for initial SPS EIR Readings\n");
    fbe_api_sleep (60000);

    fbe_zero_memory(&spsStatusInfo, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "SUSIE DERKINS:ESP, SpsStatus %d\n", 
        spsStatusInfo.spsStatus);
    // set expected SPS InputPower values
    expectedInputPower[0].status = FBE_ENERGY_STAT_VALID;
    expectedInputPower[1].inputPower = 0;
    expectedInputPower[1].status = FBE_ENERGY_STAT_NOT_PRESENT;
    if (!spsStatusInfo.spsModuleInserted)
    {
        expectedInputPower[0].inputPower = 0;
    }
    else
    {
        switch (spsStatusInfo.spsStatus)
        {
        case SPS_STATE_AVAILABLE:
            switch(spsStatusInfo.spsModel)
            {
            case SPS_TYPE_LI_ION_2_2_KW:
                expectedInputPower[0].inputPower = 12;
                break;
            case SPS_TYPE_2_2_KW:
                expectedInputPower[0].inputPower = 20;
                break;
            case SPS_TYPE_1_0_KW:
            case SPS_TYPE_1_2_KW:
                expectedInputPower[0].inputPower = 6;
                break;
            default:
                expectedInputPower[0].inputPower = 0;
                break;
            }
            break;
        case SPS_STATE_CHARGING:
            switch(spsStatusInfo.spsModel)
            {
            case SPS_TYPE_LI_ION_2_2_KW:
                expectedInputPower[0].inputPower = 270;
                break;
            case SPS_TYPE_2_2_KW:
                expectedInputPower[0].inputPower = 150;
                break;
            case SPS_TYPE_1_0_KW:
            case SPS_TYPE_1_2_KW:
                expectedInputPower[0].inputPower = 40;
                break;
            default:
                expectedInputPower[0].inputPower = 0;
                break;
            }
            break;
        default:
            expectedInputPower[0].inputPower = 0;
            break;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:spsModel %d, spsState %d, expInputPower %d\n",
                   spsStatusInfo.spsModel,
                   spsStatusInfo.spsStatus,
                   expectedInputPower[0].inputPower);
    }

    for (spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
    {
        spsInputPowerInfo.spsLocation.bus = spsInputPowerInfo.spsLocation.enclosure = 0;
        spsInputPowerInfo.spsLocation.slot = spsIndex;
        status = fbe_api_esp_sps_mgmt_getSpsInputPower(&spsInputPowerInfo);
        if (status == FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:SPS EIR Data, sps %d, InputPower:\n",
                       spsIndex);
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:\tExpected Current %d, status 0x%x\n",
                       expectedInputPower[spsIndex].inputPower,
                       expectedInputPower[spsIndex].status);
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:\tCurrent %d, status 0x%x\n",
                       spsInputPowerInfo.spsCurrentInputPower.inputPower,
                       spsInputPowerInfo.spsCurrentInputPower.status);
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:\tExpected Average %d, status 0x%x\n",
                       expectedInputPower[spsIndex].inputPower,
                       expectedInputPower[spsIndex].status);
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:\tAverage %d, status 0x%x\n",
                       spsInputPowerInfo.spsAverageInputPower.inputPower,
                       spsInputPowerInfo.spsAverageInputPower.status);
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:\tExpected Max %d, status 0x%x\n",
                       expectedInputPower[spsIndex].inputPower,
                       expectedInputPower[spsIndex].status);
            mut_printf(MUT_LOG_TEST_STATUS, "SUSIE DERKINS:\tMax %d, status 0x%x\n",
                       spsInputPowerInfo.spsMaxInputPower.inputPower,
                       spsInputPowerInfo.spsMaxInputPower.status);

            MUT_ASSERT_TRUE(expectedInputPower[spsIndex].status == 
                            spsInputPowerInfo.spsCurrentInputPower.status);
            MUT_ASSERT_TRUE(expectedInputPower[spsIndex].inputPower == 
                            spsInputPowerInfo.spsCurrentInputPower.inputPower);
        }
    }

    mut_printf(MUT_LOG_LOW, "SUSIE DERKINS: SPS EIR Readings Successful\n");
    return;
}
/******************************************
 * end susie_derkins_test()
 ******************************************/
void susie_derkins_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_FP_CONIG,
                                        SPID_PROMETHEUS_M1_HW_TYPE,
                                        NULL,
                                        fbe_test_reg_set_persist_ports_true);
}

void susie_derkins_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file susie_derkins_test.c
 *************************/


