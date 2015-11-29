
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file felix_the_cat_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests LCC and PS firmware upgrade in dual SP setup.
 *
 * @version
 *   30-Aug-2010 PHE - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"


char * felix_the_cat_short_desc = "Test dual SP LCC/PS firmware upgrade.";
char * felix_the_cat_long_desc =
    "\n"
    "\n"
    "The Felix The Cat scenario tests dual SP LCC&PS firmware upgrade.\n"
    "\n"
    "Dependencies:\n"
    "   - The terminator should be able to return firmware revision and "
    "   product id information for lcc and ps for SPA and SPB. The terminator\n"
    "   must also support send firmware download, activate and status commands.\n"
    "\n"
    "Starting Config(Naxos Config) loaded on each SP:\n"
    "    [PP] Prometheus platform \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 3 Voyager enclosures\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "    - Image files are creared as part of env setup to allow autoupgrade starts.\n"
    "    - Wait until all the upgrades initiated at power up complete.\n"
    "\n"  
    "STEP 2: For each SP, shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the PS firmware image file.\n"
    "    - Delete the LCC firmware image file.\n"
    ;


void felix_the_cat_setup(void)
{
    fbe_test_startEsp_with_common_config_dualSp(FBE_ESP_TEST_NAXOS_VOYAGER_CONIG,
                                                      SPID_OBERON_1_HW_TYPE,
                                                    NULL,
                                                    NULL);
}

// simulate staggered or delayed boot of spb
void felix_the_cat_staggered_setup(void)
{
    fbe_test_startEsp_with_common_config_dualSp(FBE_ESP_TEST_NAXOS_VOYAGER_CONIG,
                                                      SPID_OBERON_1_HW_TYPE,
                                                    NULL,
                                                    NULL);
    // wait 15s to simulate staggered boot
    fbe_api_sleep(15000);
}

void felix_the_cat_destroy(void)
{
    fbe_api_sleep(10000);
    mut_printf(MUT_LOG_LOW, " SPB===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_FAN);
    fbe_test_esp_delete_registry_file();

    fbe_test_esp_delete_esp_lun();
    
    mut_printf(MUT_LOG_LOW, " SPA===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_FAN);
    fbe_test_esp_delete_registry_file();

    fbe_test_esp_delete_esp_lun();
    
    fbe_test_esp_common_destroy_dual_sp();

    return;
}

/*!**************************************************************
 * felix_test() 
 ****************************************************************
 * @brief
 *  Main entry point for dual sp fup test.
 *
 * @param None
 *
 * @return None
 *
 * @author
 *  08-Aug-2010 PHE - Created
 ****************************************************************/
void felix_the_cat_test(void)
{
    fbe_status_t    status;

    // load the env to start autoupgrade on both SPs
    // wait here until they complete

    // now make sure the other side is also done
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_LOW, "Wait for SPA Upgrades to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(180000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "SPA Upgrade complete failed!");

    // wait until upgrades are complete
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_LOW, "Wait for SPB Upgrades to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(180000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "SPB Upgrade complete failed!");

    mut_printf(MUT_LOG_LOW, "   Destroy this setup.....");
    felix_the_cat_destroy();

    mut_printf(MUT_LOG_LOW, "   Continue to staggered start.....");
    felix_the_cat_staggered_setup();

    // now make sure the other side is also done
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_LOW, "Wait for SPA Upgrades to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(180000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "SPA Upgrade complete failed!");

    // wait until upgrades are complete
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_LOW, "Wait for SPB Upgrades to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(180000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "SPB Upgrade complete failed!");

    mut_printf(MUT_LOG_LOW, "   Done, continue to cleanup.....");

    return;
}

/*!**************************************************************
 * felix_the_cat_load_config() 
 ****************************************************************
 * @brief
 *  Loads the config for this scenario.
 *
 * @param None
 *
 * @return None
 *
 * @author
 *  12-Sep-2011 Arun S - Created
 ****************************************************************/
fbe_status_t felix_the_cat_load_config(void)
{
    fbe_status_t status;
    fbe_u32_t index = 0;
    fbe_u32_t maxEntries = 0;
    fbe_test_params_t *pNaxosTest = NULL;

    /* Load the terminator, the physical package with the naxos config
     * and verify the objects in the physical package.
     */
    maxEntries = fbe_test_get_naxos_num_of_tests() ;
    for(index = 0; index < maxEntries; index++)
    {
        /* Load the configuration for test */
        pNaxosTest =  fbe_test_get_naxos_test_table(index);
        if(pNaxosTest->encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM )
        {
            status = naxos_load_and_verify_table_driven(pNaxosTest);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        }
    }

    return FBE_STATUS_OK;
}

/*************************
 * end file felix_the_cat.c
 *************************/

