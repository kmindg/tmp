/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file paul_pierce_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test Single Sentry SP board firmware upgrade
 *  common state transition code and queuing logic. 
 *
 * @version
 *   13-Dec-2012 Rui Chang - Created.
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
#include "sep_tests.h"
#include "esp_tests.h"
#include "sep_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_file.h"
#include "pp_utils.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_persist_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_private_space_layout.h"
#include "fp_test.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_port.h"



char * paul_pierce_short_desc = "Test Single Sentry SP board firmware upgrade state transition and queuing logic.";
char * paul_pierce_long_desc =
    "\n"
    "\n"
    "The paul_pierce scenario tests Single Sentry SP board firmware upgrade state transition and queuing logic.\n"
    "It includes: \n"
    "    - Test single enclosure firmare upgrade;\n"
    "    - Test multiple enclosure firmware upgrades;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to react on the download and activate commands.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 1 citedal enclosures\n"
    "    [PP] 3 viper enclosures\n"
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
    "    - Create the jdes firmware image file.\n"
    "    - Create the jdes image path registry entry.\n"
    "\n"
    "STEP 3: Test single enclosure firmare upgrade.\n"
    "    - Initiate the firmware upgrade for the BM.\n"
    "    - Verify the upgrade work state for the BM.\n"
    "    - Verify the upgrade completion status for the BM.\n"
    "\n"
    "STEP 4: Shutdown the Terminator/Physical package/ESP.\n"
    "    - Delete the enclosure firmware image file.\n"
    ;

#define FP4_REKI_TEST_NUMBER_OF_PHYSICAL_OBJECTS       126

typedef enum fp4_reki_test_enclosure_num_e
{
    FP4_REKI_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    FP4_REKI_TEST_ENCL2_WITH_SAS_DRIVES,
    FP4_REKI_TEST_ENCL3_WITH_SAS_DRIVES,
    FP4_REKI_TEST_ENCL4_WITH_SAS_DRIVES
} fp4_reki_test_enclosure_num_t;

typedef enum fp4_reki_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} fp4_reki_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    fp4_reki_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    fp4_reki_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        FP4_REKI_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP4_REKI_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP4_REKI_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP4_REKI_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

#define FP4_REKI_TEST_ENCL_MAX (sizeof(encl_table)/sizeof(encl_table[0]))

/*!**************************************************************
 * paul_pierce_test() 
 ****************************************************************
 * @brief
 *  Main entry point for Single SP enclosure firmware upgrade.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  13-Nov-2012 Rui Chang - Created.
 *
 ****************************************************************/
void paul_pierce_test(void)
{
    fbe_status_t status;
    fbe_device_physical_location_t       location = {0};
//    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);


    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for SPA upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

//    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);


    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_SP);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_SP);
    fbe_test_esp_create_image_file(FBE_DEVICE_TYPE_LCC);
    fbe_test_esp_create_registry_image_path(FBE_DEVICE_TYPE_LCC);


    location.sp = 1;  // for SPB
    edward_and_bella_test_single_device_fup(FBE_DEVICE_TYPE_SP, &location);
//    edward_and_bella_test_multiple_device_fup_in_different_enclosure(FBE_DEVICE_TYPE_LCC);

//    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);


    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_SP);
    fbe_test_esp_delete_image_file(FBE_DEVICE_TYPE_LCC);

    return;
}

fbe_status_t fbe_test_load_paul_pierce_config(SPID_HW_TYPE platform_type)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[FP4_REKI_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;
    fbe_terminator_board_info_t             board_info;
    fbe_terminator_sas_port_info_t	        sas_port;
    fbe_cpd_shim_port_lane_info_t           port_link_info;
    fbe_cpd_shim_hardware_info_t            hardware_info;
    fbe_terminator_sas_encl_info_t      sas_encl;
    fbe_u32_t                           num_handles;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    board_info.platform_type = platform_type;
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 0;
    sas_port.portal_number = 0;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 1;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the link information to a good state*/

    fbe_zero_memory(&port_link_info,sizeof(port_link_info));
    port_link_info.portal_number = 0;
    port_link_info.link_speed = SPEED_SIX_GIGABIT;
    port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
    port_link_info.nr_phys = 2;
    port_link_info.phy_map = 0x03;
    port_link_info.nr_phys_enabled = 2;
    port_link_info.nr_phys_up = 2;
    status = fbe_api_terminator_set_port_link_info(port0_handle, &port_link_info);    
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 0; // some unique ID.
    sas_encl.sas_address = FBE_BUILD_ENCL_SAS_ADDRESS(sas_encl.backend_number, sas_encl.encl_number);
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_CITADEL;


        status  = fbe_test_insert_sas_enclosure (port0_handle, &sas_encl, &port0_encl_handle[0], &num_handles);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 1; enclosure < FP4_REKI_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < FP4_REKI_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
            else if(SATA_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sata_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
        }
    }

    
    return FBE_STATUS_OK;
}
void paul_pierce_setup(void)
{
    fbe_status_t status;

//    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);


    status = fbe_test_load_paul_pierce_config(SPID_DEFIANT_M1_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== SPA config loading is done ===\n");

//    paul_pierce_load_specl_config();

    fp_test_start_physical_package();
    
    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
}

void paul_pierce_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}


/*************************
 * end file paul_pierce_test.c
 *************************/


