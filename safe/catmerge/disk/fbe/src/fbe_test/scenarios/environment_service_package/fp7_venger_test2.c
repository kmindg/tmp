/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp7_venger_test2.c
 ***************************************************************************
 *
 * @brief
 *  Verify slic upgrade support in ESP.
 * 
 * @version
 *   05/26/2010 - Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
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
#include "fp_test.h"
#include "fbe_test_esp_utils.h"
#include "sep_tests.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fp7_venger_load_test10_config(void);
static fbe_status_t fp7_venger_load_test11_config(void);
static fbe_status_t fp7_venger_load_test13_config(void);
static fbe_status_t fp7_venger_load_test14_config(void);
static fbe_status_t fp7_venger_load_test16_config(void);
fbe_status_t fp7_venger_load_test6_config(void);
fbe_status_t fp7_venger_load_test5_config(void);
void fp7_venger_intermediate_destroy(void);

fbe_status_t fp5_kuu_set_persist_ports(fbe_bool_t persist_flag);

char * fp7_venger_test2_short_desc = "SLIC Upgrade test part 2";
char * fp7_venger_test2_long_desc ="\
Venger\n\
         - is a fictional character in the 80s cartoon series Dungeons & Dragons.\n\
         - Venger was the villian of the series. \n\
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
STEP 2: Check the limits \n\
";


/*!**************************************************************
 * fp7_venger_test2() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   05/26/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp7_venger_test2(void)
{
    fbe_status_t                                status;
    fbe_esp_module_io_module_info_t             esp_io_module_info = {0};

    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   slot, port;
    fbe_u32_t                                   iom_max=0, io_port_max=0, port_index;
    fbe_esp_module_mgmt_set_port_t              set_port_cmd;
    fbe_esp_module_mgmt_module_status_t         module_status_info;


    /* 
     * Test case 1 - 2 SPs, 1 SAS and 1 10G iSCSI 
     * Step 1 - Persist Ports
     * Step 2 - Issue SLIC Upgrade
     * Step 3 - Restart SP with upgraded configuration 
     * Step 4 - Restart Second SP with upgraded configuration
     */

    iom_max = 2;
    io_port_max = 4;

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 7 Persist El Nino Ports ***\n");



    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 7 El Nino Ports Persisted Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 8 Convert El Nino to Eruption ***\n");

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    fp7_venger_load_test5_config();

    fbe_test_esp_reload_with_existing_terminator_config();

    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }
    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 8 Convert El Nino to Eruption Completed Successfully***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 9 Booting with incorrect SLIC ***\n");

    /* Reboot the SP but keep the configuration. */
    fbe_test_esp_unload();

    fp7_venger_load_test6_config();

    fbe_test_esp_reload_with_existing_terminator_config();

    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            if(slot == 1)
            {
                /*
                 * We intentionally inserted an incompatible SLIC here in slot 1 to make sure we don't
                 * attempt to upgrade it and we actually do fault the ports.
                 */
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_INCORRECT_MODULE);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            }
            port_index++;
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 9 Booting with incorrect SLIC Completed Successfully ***\n");

    fp7_venger_destroy();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 10 Persist Supercell Ports ***\n");

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test10_config);

    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 4;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 10 Supercell Ports Persisted Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 11 Convert Supercell to 2 El Ninos ***\n");

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    fp7_venger_load_test11_config();

    fbe_api_sleep(3000);

    fbe_test_esp_reload_with_existing_terminator_config();

    iom_max = 3;
    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 11 Convert Supercell to 2 El Ninos Completed Successfully ***\n");

    fp7_venger_destroy();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 12 Persist Supercell Ports ***\n");

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test10_config);

    iom_max = 2;
    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 4;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 12 Supercell Ports Persisted Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 13 Convert Supercell to 2 Eruptions ***\n");

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    fp7_venger_load_test13_config();

    fbe_api_sleep(3000);

    fbe_test_esp_reload_with_existing_terminator_config();

    iom_max = 3;
    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 13 Convert Supercell to 2 Eruptions Completed Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 14 Incorrect SLIC on Peer SP ***\n");

    /*
     * It would be nice to detect this on the fly in simulation but it seems
     * changing this SPECL data and getting an update of it through the 
     * physical package takes a couple minutes in simulation.  Reboots force
     * reads of things and the change is detected much faster here.
     */
    fbe_test_esp_unload();

    fp7_venger_load_test14_config();

    fbe_api_sleep(3000);

    fbe_test_esp_reload_with_existing_terminator_config();

    // load the config on SPB as well

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test14_config);

    /* Issue upgrade request to SPB to complete the upgrade process */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // now check for faults reported by SPA about SPB's SLICs

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_esp_unload();

    fp7_venger_load_test14_config();

    fbe_api_sleep(3000);

    fbe_test_esp_reload_with_existing_terminator_config();

    iom_max = 3;
    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }
    for (slot = 1; slot < iom_max; slot++)
    {
        /* Check that the peer's slics are faulted */
        esp_io_module_info.header.sp = 1;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        module_status_info.header.sp = 1;
        module_status_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        module_status_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_get_module_status(&module_status_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(module_status_info.state == MODULE_STATE_FAULTED);
        MUT_ASSERT_TRUE(module_status_info.substate == MODULE_SUBSTATE_INCORRECT_MODULE);

    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 14 Incorrect SLIC on Peer SP Completed Successfully ***\n");

    fbe_test_esp_common_destroy_all_dual_sp();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 15 Persist Supercell Ports ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_esp_delete_registry_file();

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test10_config);

    iom_max = 2;
    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        io_port_max = 4;

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 15 Supercell Ports Persisted Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 16 Fail Convert Supercell to 1 Eruption ***\n");

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    fp7_venger_load_test16_config();

    fbe_api_sleep(3000);

    fbe_test_esp_reload_with_existing_terminator_config();

    iom_max = 2;
    port_index = 0;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        if(slot == 0)
        {
            io_port_max = 4;
        }
        else
        {
            io_port_max = 2;
        }

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            if(slot == 0)
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            }
            else // slot 1 should have the faulted eruption slic in it, all ports should be marked faulted
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_INCORRECT_MODULE);
            }
            port_index++;
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test2 Case 16 Fail Convert Supercell to 1 Eruption Completed Successfully ***\n");

}

void fp7_venger_test2_setup(void)
{
    fbe_status_t status;
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test6_config);
}




/*!**************************************************************
 * fp7_venger_load_test10_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with Supercell
 *  SLICs inserted.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test10_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = SUPERCELL;
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

    return status;
}

/*!**************************************************************
 * fp7_venger_load_test11_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with 2 El Nino
 *  SLICs inserted.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test11_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

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
            else if((slot == 1) || (slot == 2))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ELNINO;
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

    return status;
}


/*!**************************************************************
 * fp7_venger_load_test13_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with 2 Eruption
 *  SLICs inserted.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test13_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

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
            else if((slot == 1) || (slot == 2))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_2PORT_REV_C_84823;
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

    return status;
}

/*!**************************************************************
 * fp7_venger_load_test14_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with 2 Eruption
 *  SLICs inserted.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test14_config(void)
{

    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, controller = 0, port = 0, sp = 0;
    fbe_u32_t                       iom_max, io_controller_max, io_port_max;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }


    iom_max = 11;
    

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.bladeCount = 2;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = iom_max;
    
        for(slot = 0; slot < iom_max; slot++)
        {
            if(slot > 2) // nothing in these slots
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].diplexFPGAPresent = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].inserted = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].moduleType = IO_MODULE_TYPE_SLIC_1_0;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type = 1;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.faultLEDStatus = LED_BLINK_OFF;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerEnable = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerGood = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerUpEnable = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerUpFailure = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].transactionStatus = EMCPAL_STATUS_SUCCESS;
                continue;
            }
            else
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].diplexFPGAPresent = FALSE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].inserted = TRUE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].moduleType = IO_MODULE_TYPE_SLIC_1_0;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type = 1;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.faultLEDStatus = LED_BLINK_OFF;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerEnable = TRUE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerGood = TRUE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerUpEnable = TRUE;
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].type1.powerUpFailure = FALSE;
            }
            if(slot == 0)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
                io_port_max = 4;
                io_controller_max = 1;
            }
            if((slot == 1) || (slot == 2))
            {
                if(sp == 1)
                {
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ELNINO;
                }
                else
                {
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_2PORT_REV_C_84823;
                }
                io_port_max = 2;
                io_controller_max = 1;
            }
            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].transactionStatus = EMCPAL_STATUS_SUCCESS;

            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerCount = io_controller_max;
            for(controller=0;controller<io_controller_max;controller++)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortCount = io_port_max;
                if(slot == 0)
                {
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].protocol = IO_CONTROLLER_PROTOCOL_SAS;
                }
                else if( (slot == 1) || (slot == 2) )
                {
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].protocol = IO_CONTROLLER_PROTOCOL_ISCSI;
                }
                for(port = 0; port < io_port_max; port++)
                {
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortInfo[port].pciDataCount = 1;
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortInfo[port].portPciData[0].pciData.bus = (10 * slot) + PCI_MAX_BRIDGE_NUMBER + 1;
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortInfo[port].portPciData[0].pciData.device = 0x0;
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortInfo[port].portPciData[0].pciData.function = port;
                    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortInfo[port].portPciData[0].protocol = sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].protocol;
                    if( (slot == 0) && (port == 0) )
                    {
                        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller].ioPortInfo[port].boot_device = TRUE;
                    }
                }
            }
        }
    }
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}


/*!**************************************************************
 * fp7_venger_load_test16_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with 1 Eruption
 *  SLIC inserted.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test16_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_2PORT_REV_C_84823;
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

    return status;
}

