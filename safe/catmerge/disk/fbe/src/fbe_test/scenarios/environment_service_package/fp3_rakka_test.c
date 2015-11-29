/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp3_rakka_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify that SFP info is gathered in ESP.
 * 
 * @version
 *   07/08/2010 - Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe/fbe_module_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "EspMessages.h"
#include "sep_tests.h"
#include "pp_utils.h"
#include "fp_test.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_test_rakka_specl_config(void);
static fbe_status_t fbe_test_load_rakka_config(SPID_HW_TYPE platform_type);
static void fbe_rakka_test_start_physical_package(void);

char * fp3_rakka_short_desc = "Verify the Gathering of SFP information in ESP";
char * fp3_rakka_long_desc ="\
Rakka\n\
        - is a fictional character in the Japanese animated series Haibane Renmei.\n\
        - Rakka's name means \"falling\" and she is in constant search for herself. \n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
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
STEP 2: Insert SFP into Port 0, Verify SFP status is inserted in ESP\n\
STEP 3: Remove SFP from Port 0, Verify SFP status is removed in ESP\n\
";

/*
 * Globals
*/
static fbe_api_terminator_device_handle_t      port0_handle;
static fbe_api_terminator_device_handle_t      port1_handle;

/*
 * Definitions
*/

#define FP3_RAKKA_TEST_NUMBER_OF_PHYSICAL_OBJECTS       67

typedef enum fp3_rakka_test_enclosure_num_e
{
    FP3_RAKKA_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    FP3_RAKKA_TEST_ENCL2_WITH_SAS_DRIVES,
    FP3_RAKKA_TEST_ENCL3_WITH_SAS_DRIVES,
    FP3_RAKKA_TEST_ENCL4_WITH_SAS_DRIVES
} fp3_rakka_test_enclosure_num_t;

typedef enum fp3_rakka_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} fp3_rakka_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    fp3_rakka_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    fp3_rakka_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;
/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        FP3_RAKKA_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP3_RAKKA_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP3_RAKKA_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP3_RAKKA_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

#define FP3_RAKKA_TEST_ENCL_MAX (sizeof(encl_table)/sizeof(encl_table[0]))


/*!**************************************************************
 * fp3_rakka_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/08/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp3_rakka_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_sfp_info_t               returned_sfp_info;
    fbe_esp_module_sfp_info_t               returned_fcoe_sfp_info;
    fbe_cpd_shim_sfp_media_interface_info_t configured_sfp_info;
    fbe_cpd_shim_sfp_media_interface_info_t configured_fcoe_sfp_info;
    fbe_event_log_msg_count_t               eventLogMsgCount;
    fbe_device_physical_location_t          location;
    fbe_esp_encl_mgmt_encl_info_t           enclInfo;
    fbe_cpd_shim_hardware_info_t            sfp_terminator_hardware_info;

    /*
     * Get the handle and object id for port 0 for future reference.
     * This test is making the assumption that port index 0 is 
     * IO Module 0 Port 0.  This may be incorrect.
     */
    
    status = fbe_api_terminator_get_hardware_info(port1_handle,  &sfp_terminator_hardware_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "Port 1 Vendor:0x%x Device:0x%x \n", sfp_terminator_hardware_info.vendor, sfp_terminator_hardware_info.device);


    mut_printf(MUT_LOG_LOW, "*** Verifying SFP Status ***\n");

    fbe_zero_memory(&returned_sfp_info, sizeof(fbe_esp_module_sfp_info_t));
    returned_sfp_info.header.sp = 0;
    returned_sfp_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    returned_sfp_info.header.slot = 0;
    returned_sfp_info.header.port = 0;

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(returned_sfp_info.sfp_info.inserted == TRUE);

    fbe_zero_memory(&returned_fcoe_sfp_info, sizeof(fbe_esp_module_sfp_info_t));
    returned_fcoe_sfp_info.header.sp = 0;
    returned_fcoe_sfp_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    returned_fcoe_sfp_info.header.slot = 1;
    returned_fcoe_sfp_info.header.port = 0;

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_fcoe_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(returned_fcoe_sfp_info.sfp_info.inserted == TRUE);
    


    /*
     * STEP 3 - Remove SFP from Mezzanine Port 0 verify ESP
     * reports this as removed.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to REMOVED...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_REMOVED;
    configured_sfp_info.condition_additional_info = 0;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);

    mut_printf(MUT_LOG_LOW, "*** Verifying SFP Status ***\n");

    /* Re-use the header (for iom 0 port 0) from step 1, simply zero out the data portion of the structure */
    fbe_zero_memory(&returned_sfp_info.sfp_info, sizeof(fbe_esp_sfp_physical_info_t));

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(returned_sfp_info.sfp_info.inserted == FALSE);

    eventLogMsgCount.msg_id = ESP_INFO_SFP_REMOVED;
    eventLogMsgCount.msg_count = 0;
    status = fbe_api_get_event_log_msg_count(&eventLogMsgCount, FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "REMOVED eventLogMsgCount.msg_count %d\n", 
        eventLogMsgCount.msg_count);
    MUT_ASSERT_TRUE(eventLogMsgCount.msg_count == 1);

    /*
     * STEP 3 - Insert SFP into SLIC 0 Port 0 verify ESP
     * reports this as inserted.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to INSERTED...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_INSERTED;
    configured_sfp_info.condition_additional_info = 0;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);

    mut_printf(MUT_LOG_LOW, "*** Verifying SFP Status ***\n");

    /* Re-use the header (for iom 0 port 0) from step 1, simply zero out the data portion of the structure */
    fbe_zero_memory(&returned_sfp_info.sfp_info, sizeof(fbe_esp_sfp_physical_info_t));

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(returned_sfp_info.sfp_info.inserted == TRUE);

    eventLogMsgCount.msg_id = ESP_INFO_SFP_INSERTED;
    eventLogMsgCount.msg_count = 0;
    status = fbe_api_get_event_log_msg_count(&eventLogMsgCount, FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "INSERTED eventLogMsgCount.msg_count %d\n", 
        eventLogMsgCount.msg_count);
    MUT_ASSERT_TRUE(eventLogMsgCount.msg_count == 1);

    /*
     * STEP 3 - Fault SFP into Slic 0 Port 0 verify ESP
     * reports this as faulted and sets the enclosure fault and reason.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to FAULTED/CHECKSUM FAIL...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_FAULT;
    configured_sfp_info.condition_additional_info = FBE_CPD_SHIM_SFP_BAD_CHKSUM;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_zero_memory(&configured_fcoe_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_fcoe_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_FAULT;
    configured_fcoe_sfp_info.condition_additional_info = FBE_CPD_SHIM_SFP_BAD_CHKSUM;
    status = fbe_api_terminator_set_sfp_media_interface_info(port1_handle,&configured_fcoe_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);

    mut_printf(MUT_LOG_LOW, "*** Verifying SFP Status ***\n");

    /* Re-use the header (for iom 0 port 0) from step 1, simply zero out the data portion of the structure */
    fbe_zero_memory(&returned_sfp_info.sfp_info, sizeof(fbe_esp_sfp_physical_info_t));

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(returned_sfp_info.sfp_info.inserted == TRUE);

    fbe_zero_memory(&returned_fcoe_sfp_info, sizeof(fbe_esp_module_sfp_info_t));
    returned_fcoe_sfp_info.header.sp = 0;
    returned_fcoe_sfp_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
    returned_fcoe_sfp_info.header.slot = 1;
    returned_fcoe_sfp_info.header.port = 0;

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_fcoe_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "SFP Condition %d, Subcondition %d",
               returned_fcoe_sfp_info.sfp_info.condition_type,
               returned_fcoe_sfp_info.sfp_info.sub_condition_type);
    MUT_ASSERT_TRUE(returned_fcoe_sfp_info.sfp_info.inserted == TRUE);
    MUT_ASSERT_TRUE(returned_fcoe_sfp_info.sfp_info.condition_type == FBE_PORT_SFP_CONDITION_FAULT);


    eventLogMsgCount.msg_id = ESP_ERROR_SFP_FAULTED;
    eventLogMsgCount.msg_count = 0;
    status = fbe_api_get_event_log_msg_count(&eventLogMsgCount, FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "FAULT eventLogMsgCount.msg_count %d\n", 
        eventLogMsgCount.msg_count);
    MUT_ASSERT_TRUE(eventLogMsgCount.msg_count == 2);

    
    /* check that the enclosure fault LED is turned on for this fault */

    location.sp = 0;
    location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "XPE Encl FaultLed:%d FaultLEDReason:0x%llx\n", 
               enclInfo.enclFaultLedStatus,enclInfo.enclFaultLedReason);

    MUT_ASSERT_INT_EQUAL(FBE_LED_STATUS_ON, enclInfo.enclFaultLedStatus);
    MUT_ASSERT_INT_EQUAL(FBE_ENCL_FAULT_LED_IO_PORT_FLT, (enclInfo.enclFaultLedReason&FBE_ENCL_FAULT_LED_IO_PORT_FLT));

     /*
     * STEP 3 - Insert SFP into SLIC 0 Port 0 verify ESP
     * clears the enclosure fault reason.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to INSERTED...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_INSERTED;
    configured_sfp_info.condition_additional_info = 0;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Provide sufficient time for information to be updated. */
    EmcutilSleep(4500);

    mut_printf(MUT_LOG_LOW, "*** Verifying SFP Status ***\n");

    /* Re-use the header (for iom 0 port 0) from step 1, simply zero out the data portion of the structure */
    fbe_zero_memory(&returned_sfp_info.sfp_info, sizeof(fbe_esp_sfp_physical_info_t));

    status = fbe_api_esp_module_mgmt_get_sfp_info(&returned_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(returned_sfp_info.sfp_info.inserted == TRUE);

    fbe_api_sleep(6000);  // We need to delay to allow the notification of the LED info to get to ESP

    fbe_zero_memory(&enclInfo, sizeof(fbe_esp_encl_mgmt_encl_info_t));

    location.sp = 0;
    location.bus = FBE_XPE_PSEUDO_BUS_NUM;
    location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "XPE Encl FaultLed:%d FaultLEDReason:0x%llx\n", 
               enclInfo.enclFaultLedStatus,enclInfo.enclFaultLedReason);

    MUT_ASSERT_TRUE(FBE_ENCL_FAULT_LED_IO_PORT_FLT != (enclInfo.enclFaultLedReason&FBE_ENCL_FAULT_LED_IO_PORT_FLT));

    mut_printf(MUT_LOG_LOW, "*** Rakka test passed. ***\n");


    return;
}
/******************************************
 * end fp3_rakka_test()
 ******************************************/
/*!**************************************************************
 * fp3_rakka_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   07/08/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp3_rakka_setup(void)
{
    fbe_status_t status;

    status = fbe_test_load_rakka_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    status = fbe_test_rakka_specl_config();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    fbe_rakka_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
}
/**************************************
 * end fp3_rakka_setup()
 **************************************/

fbe_status_t fbe_test_load_rakka_config(SPID_HW_TYPE platform_type)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_api_terminator_device_handle_t      port0_encl_handle[FP3_RAKKA_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;
    fbe_terminator_board_info_t             board_info;
    fbe_terminator_sas_port_info_t	        sas_port;
    fbe_terminator_fcoe_port_info_t         fcoe_port;
    fbe_cpd_shim_port_lane_info_t           port_link_info;
    fbe_cpd_shim_hardware_info_t            hardware_info;
    fbe_cpd_shim_sfp_media_interface_info_t configured_sfp_info;

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
    hardware_info.bus = 256;
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


    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < FP3_RAKKA_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < FP3_RAKKA_TEST_ENCL_MAX; enclosure++)
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

    /*insert an fcoe port PCI 2.0.0 will match this in specl config*/
    fcoe_port.backend_number = 1;
    fcoe_port.io_port_number = 1;
    fcoe_port.portal_number = 0;
    fcoe_port.port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
    fcoe_port.port_type = FBE_PORT_TYPE_FCOE;
    status  = fbe_api_terminator_insert_fcoe_port (&fcoe_port, &port1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0xF001;
    hardware_info.device = 0xE001;
    hardware_info.bus = 266;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port1_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to INSERTED...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
    configured_sfp_info.condition_additional_info = 0;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    /* Do the same for the FCoE port */
    status = fbe_api_terminator_set_sfp_media_interface_info(port1_handle,&configured_sfp_info);
    
    return FBE_STATUS_OK;
}

fbe_status_t fbe_test_rakka_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "*** Rakka Test Creating Large SPECL Config ***\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if(slot == 0)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[0].boot_device = TRUE;
            }
            else if(slot == 1)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = HEATWAVE;
            }
            else
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = NO_MODULE;
            }
        }
    }
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_LOAD_GOOD_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Rakka Test Large Config SFI Completed ***\n");

    return status;
}

void fbe_rakka_test_start_physical_package(void)
{
    fbe_status_t                            status;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(FP3_RAKKA_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == FP3_RAKKA_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");
 }

/*!**************************************************************
 * fp3_rakka_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp3_rakka test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/08/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp3_rakka_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp3_rakka_cleanup()
 ******************************************/
/*************************
 * end file fp3_rakka_test.c
 *************************/



