/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file hobbes_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests EIR (Energy Info Reporting) from enclosures.
 *
 * @version
 *   12/22/2010 - Created. Joe Perry
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
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_power_supply_interface.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * hobbes_short_desc = "Hobbes_Test: ESP Energy Information Reporting for Enclosures";
char * hobbes_long_desc ="\
hobbes: \n\
\n\
        This tests validates that the EIR service's retrieving of Enclosure InputPower. \n\
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
 * hobbes_test()
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

void hobbes_test(void)
{
    fbe_status_t status;
//    fbe_eir_data_t fbe_eir_data;
//    fbe_base_board_mgmt_set_sps_status_t    setSpsInfo;
    fbe_object_id_t                         objectId;
//    fbe_u8_t                                spsIndex;
//    fbe_u32_t                               index;
//    fbe_power_supply_info_t                 psInfo;
    fbe_esp_encl_mgmt_get_eir_info_t        eirInfo;
//    fbe_u32_t                               psCount;
//    fbe_eir_input_power_sample_t            expectedInputPower[FBE_SPS_MAX_COUNT];
//    fbe_device_physical_location_t          location;
    fbe_eir_data_t                          arrayEirInfo;
    fbe_esp_board_mgmt_board_info_t         boardInfo = {0};
    fbe_u32_t                               expectedEirStatus = 
        (FBE_ENERGY_STAT_VALID | FBE_ENERGY_STAT_UNSUPPORTED );


    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES: start hobbes_test\n");

    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

/*
    // set SPS to OK
    setSpsInfo.spsStatus = SPS_STATE_AVAILABLE;
    setSpsInfo.spsFaults = FBE_SPS_FAULT_NO_CHANGE;
    status = fbe_api_board_set_sps_status(objectId, &setSpsInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "HOBBES:Board Object SpsStatus set 0x%x\n", 
        setSpsInfo.spsStatus);
*/

    mut_printf(MUT_LOG_LOW, "HOBBES:Wait for initial Encl EIR Readings\n");
    fbe_api_sleep (70000);

    /*
     * Get EIR Info from DPE/xPE (from Board Object)
     */
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
                    boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) 
    {
        eirInfo.eirEnclInfo.eirEnclIndividual.location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        eirInfo.eirEnclInfo.eirEnclIndividual.location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:request EIR info for xPE\n");
    }
    else if(boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE)  
    {
        eirInfo.eirEnclInfo.eirEnclIndividual.location.bus = 0;
        eirInfo.eirEnclInfo.eirEnclIndividual.location.enclosure = 0;
        mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:request EIR info for DPE\n");
    }

    eirInfo.eirInfoType = FBE_ENCL_MGMT_EIR_INDIVIDUAL;
    status = fbe_api_esp_encl_mgmt_get_eir_info(&eirInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:EIR Data, PE InputPower:\n");
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tCurrent %d, status 0x%x\n",
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower,
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tAverage %d, status 0x%x\n",
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower,
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status);
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tMax %d, status 0x%x\n",
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower,
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status);
    MUT_ASSERT_TRUE(FBE_ENERGY_STAT_VALID == 
                    eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
//        MUT_ASSERT_TRUE(expectedInputPower[spsIndex].inputPower == 
//                        eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower);
    MUT_ASSERT_TRUE((FBE_ENERGY_STAT_SAMPLE_TOO_SMALL | FBE_ENERGY_STAT_VALID) == 
                    eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status);

    /*
     * Get EIR Info from DAE's (from Enclosure Objects)
     */
    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE) 
    {
        eirInfo.eirEnclInfo.eirEnclIndividual.location.bus = 0;
        eirInfo.eirEnclInfo.eirEnclIndividual.location.enclosure = 0;
        eirInfo.eirInfoType = FBE_ENCL_MGMT_EIR_INDIVIDUAL;
        status = fbe_api_esp_encl_mgmt_get_eir_info(&eirInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:EIR Data, Encl 0 0 InputPower:\n");
        mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tCurrent %d, status 0x%x\n",
                   eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower,
                   eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
        mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tAverage %d, status 0x%x\n",
                   eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower,
                   eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status);
        mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tMax %d, status 0x%x\n",
                   eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower,
                   eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status);
        /* Terminator does not support the PS element type in the EMC specific page. */
    //    MUT_ASSERT_TRUE(FBE_ENERGY_STAT_VALID == 
    //                       eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
    //        MUT_ASSERT_TRUE(expectedInputPower[spsIndex].inputPower == 
    //                        eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower);

    }
    eirInfo.eirEnclInfo.eirEnclIndividual.location.bus = 0;
    eirInfo.eirEnclInfo.eirEnclIndividual.location.enclosure = 1;
    eirInfo.eirInfoType = FBE_ENCL_MGMT_EIR_INDIVIDUAL;
    status = fbe_api_esp_encl_mgmt_get_eir_info(&eirInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:EIR Data, Encl 0 1 InputPower:\n");
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tCurrent %d, status 0x%x\n",
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower,
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tAverage %d, status 0x%x\n",
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower,
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status);
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:\tMax %d, status 0x%x\n",
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower,
               eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status);
//    MUT_ASSERT_TRUE(FBE_ENERGY_STAT_VALID == 
//                       eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status);
//        MUT_ASSERT_TRUE(expectedInputPower[spsIndex].inputPower == 
//                        eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower);

    /*
     * Get EIR Info for whole array (from EIR Service)
     */
    status = fbe_api_esp_eir_getData(&arrayEirInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "HOBBES:EIR Data, Array InputPower %d, status 0x%x, expStatus 0x%x\n",
               arrayEirInfo.arrayInputPower.inputPower,
               arrayEirInfo.arrayInputPower.status,
               expectedEirStatus);
   MUT_ASSERT_TRUE(expectedEirStatus == arrayEirInfo.arrayInputPower.status);
//        MUT_ASSERT_TRUE(expectedInputPower[spsIndex].inputPower == 
//                        eirInfo.eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower);

    mut_printf(MUT_LOG_LOW, "HOBBES: Enclosure EIR Readings Successful\n");
    return;
}
/******************************************
 * end hobbes_test()
 ******************************************/
void hobbes_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                               FBE_ESP_TEST_FP_CONIG,
                                               SPID_OBERON_1_HW_TYPE,
                                               NULL,
                                               fbe_test_reg_set_persist_ports_true);
}

void hobbes_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file hobbes_test.c
 *************************/


