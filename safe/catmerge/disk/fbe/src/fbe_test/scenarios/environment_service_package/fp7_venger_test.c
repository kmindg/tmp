/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp7_venger_test.c
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

static fbe_status_t fbe_test_load_fp7_venger_config(void);
static fbe_status_t fp7_venger_load_test1_config(void);
static fbe_status_t fp7_venger_load_test4_config(void);
fbe_status_t fp7_venger_load_test5_config(void);
fbe_status_t fp7_venger_load_test6_config(void);
void fp7_venger_intermediate_destroy(void);

fbe_status_t fp5_kuu_set_persist_ports(fbe_bool_t persist_flag);

char * fp7_venger_short_desc = "SLIC Upgrade test";
char * fp7_venger_long_desc ="\
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
 * fp7_venger_test() 
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
void fp7_venger_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_io_module_info_t             esp_io_module_info = {0};

    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   slot, port;
    fbe_u32_t                                   iom_max=0, io_port_max=0, port_index;
    fbe_esp_module_mgmt_set_port_t              set_port_cmd;
    //fbe_sim_transport_connection_target_t       target_sp;


    /* 
     * Test case 1 - 2 SPs, 1 SAS and 1 10G iSCSI 
     * Step 1 - Persist Ports
     * Step 2 - Issue SLIC Upgrade
     * Step 3 - Restart SP with upgraded configuration 
     * Step 4 - Restart Second SP with upgraded configuration
     */

    iom_max = 2;
    io_port_max = 4;

    /* Check that the appropriate ports are enabled/good and the overlimit ports are faulted */
    mut_printf(MUT_LOG_LOW, "*** Venger Test case 1, 2 Moonlite SLICs ***\n");

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

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_restart();

    fbe_test_wait_for_all_esp_objects_ready();

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

            mut_printf(MUT_LOG_LOW, "state:%d substate:%d role:%d subrole:%d log_num:%d\n",
                       esp_io_port_info.io_port_logical_info.port_state, 
                       esp_io_port_info.io_port_logical_info.port_substate,
                       esp_io_port_info.io_port_logical_info.port_role,
                       esp_io_port_info.io_port_logical_info.port_subrole,
                       esp_io_port_info.io_port_logical_info.logical_number );

            

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }

    mut_printf(MUT_LOG_LOW, "*** FP7 Venger Ports Persisted, Setting Upgrade ***\n");

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

     /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 3 Booting after Upgrade with new SLIC (??? no changes!) ***\n");

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
    
     /* Reboot the SP and verify the ports are still persisted. */
    fp7_venger_intermediate_destroy();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Loading Default Config and Persisting Ports ***\n");

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test1_config);

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 4 Booting after Upgrade with incorrect SLIC ***\n");

    fp7_venger_load_test4_config();

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

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 4 Booting after Upgrade with incorrect SLIC Completed Successfully ***\n");


    fbe_test_esp_check_mgmt_config_completion_event();

    fp7_venger_destroy();

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 5 Persist Eruption Ports ***\n");

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test5_config);

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

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 5 Eruption Ports Persisted Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 6 Convert Eruption to El Nino ***\n");

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
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

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            port_index++;
        }
    }
    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test Case 6 Convert Eruption to El Nino Completed Successfully ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    
}


/*!**************************************************************
 * fp7_venger_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   05/26/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp7_venger_setup(void)
{
    fbe_status_t status;

    /* clear out any configuration that may have been left around from a previous test */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    status = fbe_test_load_fp7_venger_config();
    fp_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

}
/**************************************
 * end fp7_venger_setup()
 **************************************/

/*!**************************************************************
 * fbe_test_load_fp7_venger_config()
 ****************************************************************
 * @brief
 *  Setup creating a configuration using termator and specl APIs.
 *  We start here with test1 configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_test_load_fp7_venger_config(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    if(status == FBE_STATUS_OK)
    {
        status = fp7_venger_load_test1_config();
    }
    return status;
}

/*!**************************************************************
 * fp7_venger_load_test1_config()
 ****************************************************************
 * @brief
 *  This function creates a Prometheus configuration with SAS and
 *  a 10G iSCSI SLIC type.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test1_config(void)
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

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}


/*!**************************************************************
 * fp7_venger_load_test4_config()
 ****************************************************************
 * @brief
 *  This function creates a Prometheus configuration with SAS and
 *  a FCoE type.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test4_config(void)
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

    return status;
}


/*!**************************************************************
 * fp7_venger_load_test5_config()
 ****************************************************************
 * @brief
 *  This function creates a Prometheus configuration with Eruption
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
fbe_status_t fp7_venger_load_test5_config(void)
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

/*!**************************************************************
 * fp7_venger_load_test6_config()
 ****************************************************************
 * @brief
 *  This function creates a Prometheus configuration with El Nino
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
fbe_status_t fp7_venger_load_test6_config(void)
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
 * fp7_venger_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fp7_venger test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   05/26/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp7_venger_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}




void fp7_venger_intermediate_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
