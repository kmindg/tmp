
/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cdes2_ps_fup.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test Single SP Tabasco PS firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @version
 *   18-Oct-2014 GB - Created.
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

char * cdes2_ps_fup_short_desc = "Test Tabasco PS firmware upgrade ";
char * cdes2_ps_fup_long_desc =
    "\n"
    "\n"
    "The cdes2_ps_fup scenario tests Tabasco PS firmware upgrade.\n"
    "It includes: \n"
    "    - Test single enclosure firmare upgrade;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    FBE_ESP_TEST_SIMPLE_TABASCO_CONFIG \n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "    - Wait until all the upgrade initiated at power up completes.\n"
    "\n"  
    "STEP 2: Set up the test environment.\n"
    "    - Create the firmware image file.\n"
    "    - Create the image path registry entry.\n"
    "    - Create the manifest file.\n"
    "\n"
    "STEP 3: Test single enclosure firmare upgrade.\n"
    "    - Initiate the firmware upgrade using edward_and_bella_test_single_device_fup.\n"
    "\n"
    "\n"
    "STEP 5: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the enclosure firmware image file and manifest file.\n"
    ;


/*!**************************************************************
 * cdes2_ps_fup_test() 
 ****************************************************************
 * @brief
 *  Create imafge file, registry entry, and manifest, then call
 *  funstions to test PS firmware upgrade.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  18-Oct-2014 GB - Created.
 ****************************************************************/
void cdes2_ps_fup_test(void)
{
    fbe_status_t status;
    fbe_device_physical_location_t       location = {0};


    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_manifest_file(FBE_DEVICE_TYPE_PS);

    // set location.slot = 1 when using FBE_SIM_SP_B
    edward_and_bella_test_single_device_fup(FBE_DEVICE_TYPE_PS, &location);

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_manifest_file(FBE_DEVICE_TYPE_PS);

    return;
}


void cdes2_ps_fup_setup(void)
{
    // xpe with single tabasco encl 0_0
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_SIMPLE_TABASCO_CONFIG,
                                        SPID_PROMETHEUS_M1_HW_TYPE,
                                        NULL,
                                        NULL);
}

void cdes2_ps_fup_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
// end file cdes2_ps_fup.c

