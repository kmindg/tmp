/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file johnny_tractor_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test Cayenne 12 DAE LCC and PS firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @version
 *   27-Aug-2014 PHE - Created.
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
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "sep_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "fbe_base_environment_debug.h"
#include "pp_utils.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_persist_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_private_space_layout.h"
#include "fp_test.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_port.h"



char * johnny_tractor_short_desc = "Test Cayenne 12G DAE LCC and PS firmware upgrade state transition and queuing logic.";
char * johnny_tractor_long_desc =
    "\n"
    "\n"
    "The johnny_tractor scenario tests Cayenne DAE firmware upgrade state transition and queuing logic.\n"
    "It includes: \n"
    "    - Test single enclosure firmare upgrade;\n"
    "    - Test FUP to all expanders (1 IOSXP, 1 DRVSXP);\n"
    "    - Test FUP to all power supplies;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 1 Cayenne enclosures\n"
    "    [PP] 15 SAS drives (PDO)\n"
    "    [PP] 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "    - Wait until all the upgrade initiated at power up completes.\n"
    "\n"  
    "STEP 2: Set up the test environment.\n"
    "    - Create the SXP firmware image file.\n"
    "    - Create the SXP image path registry entry.\n"
    "    - Create the Power Supply firmware image file.\n"
    "    - Create the Power Supply image path registry entry.\n"
    "\n"
    "STEP 3: Test single enclosure firmare upgrade.\n"
    "    - Initiate the firmware upgrade for each expander.\n"
    "    - Verify the upgrade work state for the expander.\n"
    "    - Verify the upgrade completion status for the expander.\n"
    "    - Repeat steps for power supply.\n"
    "\n"
    "STEP 4: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the enclosure firmware image files.\n"
    ;

typedef enum test_fup_devices_e
{
    TEST_IOSXP = 0,
    TEST_DRVSXP0,
} test_fup_devices_t;

typedef struct device_fup_details_s
{
    test_fup_devices_t              fupDevice;
    fbe_u64_t                       type;
    fbe_device_physical_location_t  location;
} device_fup_details_t;

static device_fup_details_t deviceFupTable[] = 
{
    {   TEST_DRVSXP0,FBE_DEVICE_TYPE_LCC,
       0, 0, 0, 4, 0, 0, 0, 0, 0
    },
    /* IOSXP firmware upgrade needs to be done after its DRVSXP firmware upgrade is done. */
    {   TEST_IOSXP,FBE_DEVICE_TYPE_LCC,
        //processorEnclosure,bus,encl,cid,bank,bankslot,slot,sp,port
        0, 0, 0, 0, 0, 0, 0, 0, 0 
    },
};

fbe_u32_t johnny_tractor_test_get_num_of_tests(void)
{
     return  (sizeof(deviceFupTable))/(sizeof(deviceFupTable[0]));
}

/*!**************************************************************
 * johnny_tractor_test() 
 ****************************************************************
 * @brief
 *  Main entry point for Cayenne 12G enclosure firmware upgrade.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  27-Aug-2014 PHE - Created.
 *
 ****************************************************************/
void johnny_tractor_test(void)
{
    fbe_status_t                    status;
    test_fup_devices_t              fupDevice;
    fbe_u64_t                       deviceType;
    fbe_device_physical_location_t  location = {0};
    fbe_u32_t                       numOfTests = 0;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for SPA upgrade to complete ===");
    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_manifest_file(FBE_DEVICE_TYPE_LCC);

    numOfTests = johnny_tractor_test_get_num_of_tests();
    // loop through all the devices to download
    for(fupDevice = TEST_IOSXP; fupDevice < numOfTests; fupDevice++)
    {
        deviceType = deviceFupTable[fupDevice].type;
        location = deviceFupTable[fupDevice].location;
        mut_printf(MUT_LOG_LOW, "    Initiate Cayenne %s FUP %d_%d_%d(%d)",
                   fbe_base_env_decode_device_type(deviceType),
                   location.bus,
                   location.enclosure,
                   location.slot,
                   location.componentId);
        edward_and_bella_test_single_device_fup(deviceType, &location);
    }

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_delete_manifest_file(FBE_DEVICE_TYPE_LCC);

    return;
}

void johnny_tractor_setup(void)
{
    // run on SPA, Cayenne 
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_SIMPLE_CAYENNE_CONFIG,
                                        SPID_PROMETHEUS_M1_HW_TYPE,// megatron 
                                        NULL,
                                        NULL);

    return;
}

void johnny_tractor_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}


/*************************
 * end file johnny_tractor_test.c
 *************************/



