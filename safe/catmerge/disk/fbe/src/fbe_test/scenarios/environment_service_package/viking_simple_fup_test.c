/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file viking_simple_fup_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test Viking DAE firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @version
 *   13-Nov-2012 Rui Chang - Created.
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



char * viking_simple_fup_short_desc = "Test Viking DAE firmware upgrade state transition and queuing logic.";
char * viking_simple_fup_long_desc =
    "\n"
    "\n"
    "Viking Simple FUP scenario tests Viking DAE firmware upgrade state transition and queuing logic.\n"
    "It includes: \n"
    "    - Test single enclosure firmare upgrade;\n"
    "    - Test FUP to all expanders (1 IO, 4 DRV SXPs);\n"
    "    - Test FUP to all power supplies;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 1 fallback enclosures\n"
    "    [PP] 1 viking enclosures\n"
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
    TEST_DRVSXP1,
    TEST_DRVSXP2,
    TEST_DRVSXP3,
    TEST_PS0,
    TEST_PS1
} test_fup_devices_t;

typedef struct device_fup_details_s
{
    test_fup_devices_t              fupDevice;
    fbe_u64_t               type;
    fbe_device_physical_location_t  location;
} device_fup_details_t;

static device_fup_details_t deviceFupTable[] = 
{
    {   TEST_IOSXP,FBE_DEVICE_TYPE_LCC,
        //processorEnclosure,bus,encl,cid,bank,bankslot,slot,sp,port
        0, 0, 0, 0, 0, 0, 0, 0, 0 
    },
    {   TEST_DRVSXP0,FBE_DEVICE_TYPE_LCC,
       0, 0, 0, 2, 0, 0, 0, 0, 0
    },
    {   TEST_DRVSXP1,FBE_DEVICE_TYPE_LCC,
        0, 0, 0, 3, 0, 0, 0, 0, 0
    },
    {   TEST_DRVSXP2,FBE_DEVICE_TYPE_LCC,
       0, 0, 0, 4, 0, 0, 0, 0, 0
    },
    {   TEST_DRVSXP3,FBE_DEVICE_TYPE_LCC,
       0, 0, 0, 5, 0, 0, 0, 0, 0
    },
    /* ESP has the policy that SPA only does the upgrade for Viking PS in slot 0 and slot 1.
     *  SPB only does the upgrade for Viking PS in slot 2 and slot 3.
     */
    {   TEST_PS0,FBE_DEVICE_TYPE_PS,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {   TEST_PS1,FBE_DEVICE_TYPE_PS,
        0, 0, 0, 0, 0, 0, 1, 0, 0
    }
};

fbe_u32_t viking_simple_fup_test_get_num_of_tests(void)
{
     return  (sizeof(deviceFupTable))/(sizeof(deviceFupTable[0]));
}

/*!**************************************************************
 * viking_simple_fup_test() 
 ****************************************************************
 * @brief
 *  Main entry point for Viking enclosure firmware upgrade.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  13-Nov-2012 Rui Chang - Created.
 *
 ****************************************************************/
void viking_simple_fup_test(void)
{
    fbe_status_t                    status;
    test_fup_devices_t              fupDevice;
    fbe_u64_t               deviceType;
    fbe_device_physical_location_t  location = {0};
    fbe_u32_t                       numOfTests = 0;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for SPA upgrade to complete ===");
    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_PS);

    numOfTests = viking_simple_fup_test_get_num_of_tests();
    // loop through all the devices to download
    for(fupDevice = TEST_IOSXP; fupDevice < numOfTests; fupDevice++)
    {
        deviceType = deviceFupTable[fupDevice].type;
        location = deviceFupTable[fupDevice].location;
        mut_printf(MUT_LOG_LOW, "    Initiate Viking %s FUP %d_%d_%d(%d)",
                   fbe_base_env_decode_device_type(deviceType),
                   location.bus,
                   location.enclosure,
                   location.slot,
                   location.componentId);
        edward_and_bella_test_single_device_fup(deviceType, &location);
    }

    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_PS);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}

void viking_simple_fup_setup(void)
{
    // run on SPA, VIKING DAE0
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_SIMPLE_VIKING_CONFIG,
                                        SPID_PROMETHEUS_M1_HW_TYPE,// megatron 
                                        NULL,
                                        NULL);

    return;
}

void viking_simple_fup_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}


/*************************
 * end file viking_simple_fup_test.c
 *************************/


