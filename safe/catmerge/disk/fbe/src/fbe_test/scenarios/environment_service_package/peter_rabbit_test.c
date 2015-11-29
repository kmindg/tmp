/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file peter_rabbit_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test Hyperion DPE Base Module firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @vers
 *   28-Aug-2014 PHE - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * peter_rabbit_short_desc = "Test Hyperion DPE Base Module firmware upgrade state transition and queuing logic.";
char * peter_rabbit_long_desc =
    "\n"
    "\n"
    "The peter_rabbit scenario tests Hyperion DPE Base Module firmware upgrade state transition and queuing logic.\n"
    "It includes: \n"
    "    - Test single enclosure firmare upgrade;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Simple Config:\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 1 mirander enclosures (25 drive Hyperion)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "    - Wait until all the upgrade initiated at power up completes.\n"
    "\n"  
    "STEP 2: Set up the test environment.\n"
    "    - Create the enclosure firmware image file.\n"
    "    - Create the enclosure image path registry entry.\n"
    "\n"
    "STEP 3: Test single enclosure firmare upgrade.\n"
    "    - Initiate the firmware upgrade for the Base Module(0_0_0).\n"
    "    - Verify the upgrade work state for the Base Module(0_0_0).\n"
    "    - Verify the upgrade completion status for the Base Module(0_0_0).\n"
    "\n"
    "STEP 5: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the enclosure firmware image file.\n"
    ;


/*!**************************************************************
 * peter_rabbit_test() 
 ****************************************************************
 * @brief
 *  Main entry point for Hyperion DPE LCC firmware upgrade.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  28-Aug-2014 PHE - Created.
 *
 ****************************************************************/
void peter_rabbit_test(void)
{
    fbe_status_t status;
    fbe_device_physical_location_t       location = {0};


    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    /* Hyperion BM has the same image as other DAE LCC. So we can just create the image using FBE_DEVICE_TYPE_LCC */
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);

    /* Hyperion BM has the same image as other DAE LCC. So the image path is the same. */
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);

    /* Hyperion LCC has the same manifest as other DAE LCC. So we can just create the manifest file using FBE_DEVICE_TYPE_LCC. */
    fbe_test_esp_create_manifest_file(FBE_DEVICE_TYPE_LCC);

    // set location.slot = 1 when using FBE_SIM_SP_B
    edward_and_bella_test_single_device_fup(FBE_DEVICE_TYPE_BACK_END_MODULE, &location);

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_delete_manifest_file(FBE_DEVICE_TYPE_LCC);

    return;
}

void peter_rabbit_setup(void)
{
    // run on SPA, Hyperion 
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_SIMPLE_CALYPSO_CONFIG,
                                        SPID_HYPERION_1_HW_TYPE,// Hyperion 
                                        NULL,
                                        NULL);

    return;
}

void peter_rabbit_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}


/*************************
 * end file peter_rabbit_test.c
 *************************/



