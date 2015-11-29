/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp6_nemu_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify port limitations handled by ESP.
 * 
 * @version
 *   05/26/2010 - Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
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
#include "fp_test.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_test_load_nemu_config(void);
static fbe_status_t fbe_test_nemu_load_test1_config(void);
static fbe_status_t fbe_test_nemu_load_test2_config(void);
static fbe_status_t fbe_test_nemu_load_test3_config(void);
static fbe_status_t fbe_test_nemu_load_test4_config(void);
static fbe_status_t fbe_test_nemu_load_test5_config(void);
static fbe_status_t fbe_test_nemu_load_test6_config(void);
static fbe_status_t fbe_test_nemu_load_test8_config(void);
static fbe_status_t fbe_test_nemu_load_test9_config(void);
static fbe_status_t fbe_test_nemu_load_test10_config(void);
fbe_status_t fp5_kuu_set_persist_ports(fbe_bool_t persist_flag);
static void fp6_nemu_intermediate_destroy(void);


char * fp6_nemu_short_desc = "Verify the port limitations by ESP";
char * fp6_nemu_long_desc ="\
Sud Sakorn\n\
         - is a fictional character in the Japanese animated series Haibane Renmei.\n\
         - Nemu's name means \"sleep\". \n\
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
 * fp6_nemu_test() 
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

void fp6_nemu_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_io_module_info_t             esp_io_module_info = {0};

    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   slot, port;
    fbe_u32_t                                   iom_max=0, io_port_max=0;
    fbe_esp_module_limits_info_t                limits_info;

    fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    mut_printf(MUT_LOG_LOW, "Discovered Limits -  Mezz:%d, SLIC:%d, Ports:%d \n",
               limits_info.discovered_hw_limit.num_mezzanine_slots,
               limits_info.discovered_hw_limit.num_slic_slots,
               limits_info.discovered_hw_limit.num_ports);

    mut_printf(MUT_LOG_LOW, "Platform HW Limits -  Mezz:%d, SLIC:%d\n",
               limits_info.platform_hw_limit.max_mezzanines,
               limits_info.platform_hw_limit.max_slics);

    mut_printf(MUT_LOG_LOW, "Platform Port Limits -  SAS BE:%d, SAS FE:%d, FC FE:%d\n",
               limits_info.platform_port_limit.max_sas_be,
               limits_info.platform_port_limit.max_sas_fe,
               limits_info.platform_port_limit.max_fc_fe);

    mut_printf(MUT_LOG_LOW, "Platform Port Limits -  1G iSCSI:%d, 10G iSCSI:%d, FCoE:%d\n",
               limits_info.platform_port_limit.max_iscsi_1g_fe,
               limits_info.platform_port_limit.max_iscsi_10g_fe,
               limits_info.platform_port_limit.max_fcoe_fe);


    /* Test case 1 - Default Config with 2 SAS SLICs */

    /* Check that the appropriate ports are enabled/good and the overlimit ports are faulted */
    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 1, 2 SnowdevilX SLICs in a Oberon 4 ***\n");

    // 6 ports onboard the mezzanine
    io_port_max = 6;
    // get the port info for that IO module
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "Mezzanine PORT: %d, present 0x%x, status 0x%x\n", 
            port, esp_io_port_info.io_port_info.present, status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.present, FBE_MGMT_STATUS_TRUE);

        if(port == 0)
        {
            // be 0 is auto-persisted
             MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
             MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_UNINITIALIZED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_UNINITIALIZED);
        }
    }

    iom_max = 2;
    io_port_max = 4;

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
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_UNINITIALIZED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_UNINITIALIZED);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);
            }
        }
    }
    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 1 Successful ***\n");

    // destroy the config
    fp6_nemu_intermediate_destroy();

    /* Test Case 2 Onboard Ports - No Faults */

    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 2, Just the Onboard Ports ***\n");

    fp_test_load_dynamic_config(SPID_OBERON_4_HW_TYPE, fbe_test_nemu_load_test2_config);

    // 6 ports onboard the mezzanine
    io_port_max = 6;
    // get the port info for that IO module
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "Mezzanine PORT: %d, present 0x%x, status 0x%x\n", 
            port, esp_io_port_info.io_port_info.present, status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.present, FBE_MGMT_STATUS_TRUE);

        if(port == 0)
        {
            // be 0 is auto-persisted
             MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
             MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_UNINITIALIZED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_UNINITIALIZED);
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 2 Successful ***\n");

    fp6_nemu_intermediate_destroy();


    /* Test Case 2 Onboard Ports - No Faults FC SLICs Faulted */

    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 3, Onboard Ports and 2 FC SLICs ***\n");

    fp_test_load_dynamic_config(SPID_OBERON_4_HW_TYPE, fbe_test_nemu_load_test3_config);

    // 6 ports onboard the mezzanine
    io_port_max = 6;
    // get the port info for that IO module
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "Mezzanine PORT: %d, present 0x%x, status 0x%x\n", 
            port, esp_io_port_info.io_port_info.present, status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.present, FBE_MGMT_STATUS_TRUE);
        
        if(port == 0)
        {
            // be 0 is auto-persisted
             MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
             MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_UNINITIALIZED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_UNINITIALIZED);
        }
    }

    iom_max = 2;
    io_port_max = 4;
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
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_UNINITIALIZED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_UNINITIALIZED);
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 3 Successful ***\n");

    fp6_nemu_intermediate_destroy();


    /* Test Case 4 Defiant Limits, with persisted ports. */

    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 4, Defiant M1 Lotsa SAS ***\n");

    fp_test_load_dynamic_config(SPID_DEFIANT_M1_HW_TYPE, fbe_test_nemu_load_test4_config);

    fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    mut_printf(MUT_LOG_LOW, "Discovered Limits -  Mezz:%d, SLIC:%d, Ports:%d \n",
               limits_info.discovered_hw_limit.num_mezzanine_slots,
               limits_info.discovered_hw_limit.num_slic_slots,
               limits_info.discovered_hw_limit.num_ports);

    mut_printf(MUT_LOG_LOW, "Platform HW Limits -  Mezz:%d, SLIC:%d\n",
               limits_info.platform_hw_limit.max_mezzanines,
               limits_info.platform_hw_limit.max_slics);

    mut_printf(MUT_LOG_LOW, "Platform Port Limits -  SAS BE:%d, SAS FE:%d, FC FE:%d\n",
               limits_info.platform_port_limit.max_sas_be,
               limits_info.platform_port_limit.max_sas_fe,
               limits_info.platform_port_limit.max_fc_fe);

    mut_printf(MUT_LOG_LOW, "Platform Port Limits -  1G iSCSI:%d, 10G iSCSI:%d, FCoE:%d\n",
               limits_info.platform_port_limit.max_iscsi_1g_fe,
               limits_info.platform_port_limit.max_iscsi_10g_fe,
               limits_info.platform_port_limit.max_fcoe_fe);

    iom_max = 5;
    io_port_max = 4;
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
            if(slot == 0) // only the first SLIC is within the limits
            {
                // all the FC ports in the SLICs should be faulted/overlimit
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            }
            else
            {
                // all the FC ports in the SLICs should be faulted/overlimit
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);
            }
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 4 Successful ***\n");

    fp6_nemu_intermediate_destroy();


    /* Test Case 5 DefiantM1 Limits, with persisted ports. */

    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 5, Mixed Bag  of 2 and 4-port SLICs no Faults ***\n");

    fp_test_load_dynamic_config(SPID_DEFIANT_M1_HW_TYPE, fbe_test_nemu_load_test5_config);


    iom_max = 3;
    io_port_max = 4;
    for (slot = 0; slot < iom_max; slot++)
    {
        if(slot == 1)
        {
            // slot 1 has the only 2 port slic
            io_port_max = 2;
        }
        else
        {
            io_port_max = 4;
        }
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

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
            
            // no overlimit ports all persisted/enabled/good
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    fp6_nemu_intermediate_destroy();

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 5 Successful ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_esp_delete_esp_lun();

    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 6, Mixed Bag 4-port SLICs no Faults on SPB ***\n");

    fp_test_load_dynamic_config(SPID_DEFIANT_M1_HW_TYPE, fbe_test_nemu_load_test5_config);


    iom_max = 3;
    io_port_max = 4;
    for (slot = 0; slot < iom_max; slot++)
    {
        if(slot == 1)
        {
            // slot 1 has the only 2 port slic
            io_port_max = 2;
        }
        else
        {
            io_port_max = 4;
        }
        esp_io_module_info.header.sp = SP_B;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n", 
            esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = SP_B;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
            
            // no overlimit ports all persisted/enabled/good
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fp6_nemu_intermediate_destroy();

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 6 Successful ***\n");

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_nemu_load_test6_config);
    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 7, 11 4-port SLICs no Faults on SPB ***\n");

    iom_max = 11;
    io_port_max = 4;
    for (slot = 0; slot < iom_max; slot++)
    {
        if(slot != 3) // slot 3 was empty
        {
            esp_io_module_info.header.sp = SP_B;
            esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_module_info.header.slot = slot;
            status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
            mut_printf(MUT_LOG_LOW, "Io module %d inserted 0x%x, status 0x%x\n", 
                slot, esp_io_module_info.io_module_phy_info.inserted, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

            // get the port info for that IO module
            for(port = 0; port < io_port_max; port++)
            {
                esp_io_port_info.header.sp = SP_B;
                esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
                esp_io_port_info.header.slot = slot;
                esp_io_port_info.header.port = port;
            
                status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
                mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                    slot, port, esp_io_port_info.io_port_info.present, status);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
                
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
                
            }
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fp6_nemu_intermediate_destroy();

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 7 Successful ***\n");

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_nemu_load_test8_config);
    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 8, Overlimit iSCSI ***\n");

    iom_max = 11;
    io_port_max = 4;
    for (slot = 0; slot < iom_max; slot++)
    {
        if(slot != 3) // slot 3 was empty
        {
            esp_io_module_info.header.sp = SP_B;
            esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_module_info.header.slot = slot;
            status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
            mut_printf(MUT_LOG_LOW, "Io module %d inserted 0x%x, status 0x%x\n", 
                slot, esp_io_module_info.io_module_phy_info.inserted, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

            // get the port info for that IO module
            for(port = 0; port < io_port_max; port++)
            {
                esp_io_port_info.header.sp = SP_B;
                esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
                esp_io_port_info.header.slot = slot;
                esp_io_port_info.header.port = port;
            
                status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
                mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                    slot, port, esp_io_port_info.io_port_info.present, status);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

                if( (esp_io_module_info.io_module_phy_info.uniqueID == SUPERCELL) &&
                    (slot > 6))
                {
                    // these should be overlimit
                    MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                    MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);

                }
                else
                {
                    // no overlimit ports all 4 ports on each slic persisted/enabled/good
                    MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                    MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
                }
                
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 7 Successful ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fp6_nemu_intermediate_destroy();

    #if 0 // disable this test
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_nemu_load_test9_config);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_nemu_load_test9_config);
    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 9, 11 SLICs Overlimit iSCSI ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    iom_max = 11;
    io_port_max = 4;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        io_port_max = esp_io_module_info.io_module_phy_info.ioPortCount;
        
        esp_io_module_info.header.sp = SP_A;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module %d inserted 0x%x, status 0x%x\n", 
            slot, esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = SP_A;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
            if( (slot != 7) && (slot != 8) && (slot != 9)) // slots 8 & 9 should be faulted
            {
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            }
            else
            {
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);
            }
        }
    }

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = SP_B;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        io_port_max = esp_io_module_info.io_module_phy_info.ioPortCount;
        
        esp_io_module_info.header.sp = SP_B;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module %d inserted 0x%x, status 0x%x\n", 
            slot, esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = SP_B;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
            if( (slot != 7) && (slot != 8) && (slot != 9)) // slots 8 and 9 should be faulted
            {
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            }
            else
            {
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 9 Successful ***\n");


    

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fp6_nemu_intermediate_destroy();
    #endif

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_load_dynamic_config(SPID_HYPERION_1_HW_TYPE, fbe_test_nemu_load_test10_config);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fp_test_load_dynamic_config(SPID_HYPERION_1_HW_TYPE, fbe_test_nemu_load_test10_config);

    mut_printf(MUT_LOG_LOW, "*** Nemu Test case 10, Overlimit SAS SLICs in Hyperion ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_api_esp_module_mgmt_get_limits_info(&limits_info);
    mut_printf(MUT_LOG_LOW, "Discovered Limits -  Mezz:%d, SLIC:%d, Ports:%d \n",
               limits_info.discovered_hw_limit.num_mezzanine_slots,
               limits_info.discovered_hw_limit.num_slic_slots,
               limits_info.discovered_hw_limit.num_ports);

    mut_printf(MUT_LOG_LOW, "Platform HW Limits -  Mezz:%d, SLIC:%d\n",
               limits_info.platform_hw_limit.max_mezzanines,
               limits_info.platform_hw_limit.max_slics);

    mut_printf(MUT_LOG_LOW, "Platform Port Limits -  SAS BE:%d, SAS FE:%d, FC FE:%d\n",
               limits_info.platform_port_limit.max_sas_be,
               limits_info.platform_port_limit.max_sas_fe,
               limits_info.platform_port_limit.max_fc_fe);

    mut_printf(MUT_LOG_LOW, "Platform Port Limits -  1G iSCSI:%d, 10G iSCSI:%d, FCoE:%d\n",
               limits_info.platform_port_limit.max_iscsi_1g_fe,
               limits_info.platform_port_limit.max_iscsi_10g_fe,
               limits_info.platform_port_limit.max_fcoe_fe);

    iom_max = 3;
    io_port_max = 4;
    for (slot = 0; slot < iom_max; slot++)
    {
        esp_io_module_info.header.sp = 0;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        io_port_max = esp_io_module_info.io_module_phy_info.ioPortCount;
        
        esp_io_module_info.header.sp = SP_A;
        esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
        mut_printf(MUT_LOG_LOW, "Io module %d inserted 0x%x, status 0x%x\n", 
            slot, esp_io_module_info.io_module_phy_info.inserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

        // get the port info for that IO module
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = SP_A;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
            if(slot == 0) // only slot 0 should be within limits
            {
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
            }
            else
            {
                // no overlimit ports all 4 ports on each slic persisted/enabled/good
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Nemu Test Case 10 Successful ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();


    return;
}
/******************************************
 * end fp6_nemu_test()
 ******************************************/
/*!**************************************************************
 * fp6_nemu_setup()
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
void fp6_nemu_setup(void)
{
    
    fbe_status_t status;

    fbe_test_load_nemu_config();

    fp_test_start_physical_package();

	/* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load the logical packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");

    fbe_test_wait_for_all_esp_objects_ready();
    #if 0
    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    fbe_test_reg_set_persist_ports_true();
    fp_test_load_dynamic_config(SPID_HYPERION_1_HW_TYPE, fbe_test_nemu_load_test10_config);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fp_test_load_dynamic_config(SPID_HYPERION_1_HW_TYPE, fbe_test_nemu_load_test10_config);
    #endif

}
/**************************************
 * end fp6_nemu_setup()
 **************************************/



/*!**************************************************************
 * fbe_test_load_nemu_config()
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
fbe_status_t fbe_test_load_nemu_config(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    status = fbe_test_load_fp_config(SPID_OBERON_4_HW_TYPE);
    if(status == FBE_STATUS_OK)
    {
        status = fbe_test_nemu_load_test1_config();
    }
    return status;
}

/*!**************************************************************
 * fbe_fp_test_load_required_config()
 ****************************************************************
 * @brief
 *  This function creates a basic configuration required to run
 *  IO in the physical package as part of a basic sanity for
 *  these tests.  This configuration is not actually used by
 *  ESP in this test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_fp_test_load_required_config(SPID_HW_TYPE platform_type)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t         board_info;
    fbe_terminator_sas_port_info_t	    sas_port;
    fbe_terminator_sas_encl_info_t	    sas_encl;
    fbe_terminator_sas_drive_info_t     sas_drive;

    fbe_api_terminator_device_handle_t	port0_handle;
    fbe_api_terminator_device_handle_t	encl0_0_handle;
    fbe_api_terminator_device_handle_t	drive_handle;
    fbe_cpd_shim_hardware_info_t        hardware_info;
    fbe_u32_t                           slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = platform_type;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*insert a port*/
    sas_port.backend_number = 0;
    sas_port.io_port_number = 3;
    sas_port.portal_number = 5;
    sas_port.sas_address = 0x5000097A7800903F;
    sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
    status  = fbe_api_terminator_insert_sas_port (&sas_port, &port0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure port hardware info */
    fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
    hardware_info.vendor = 0x11F8;
    hardware_info.device = 0x8001;
    hardware_info.bus = 0x34;
    hardware_info.slot = 0;
    hardware_info.function = 0;

    status = fbe_api_terminator_set_hardware_info(port0_handle,&hardware_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*insert an enclosure to port 0*/
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 0;
    sas_encl.uid[0] = 1; // some unique ID.
    sas_encl.sas_address = 0x5000097A78009071;
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port0_handle, &sas_encl, &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    

    for (slot = 0; slot < SIMPLE_CONFIG_DRIVE_MAX; slot ++) {
		/*insert a sas_drive to port 0 encl 0 slot */
		sas_drive.backend_number = 0;
		sas_drive.encl_number = 0;
		sas_drive.capacity = 0x10B5C731;
		sas_drive.block_size = 520;
		sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number) << 16) + slot;
		sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;
		status  = fbe_api_terminator_insert_sas_drive (encl0_0_handle, slot, &sas_drive, &drive_handle);
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return status;
}

void fp_test_load_dynamic_config(SPID_HW_TYPE platform_type, fbe_status_t (test_config_function(void)))
{
    fbe_status_t status;

    status = fbe_test_load_fp_config(platform_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    status = test_config_function();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    fp_test_start_physical_package();

    /* Load the logical packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK)

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");

    fbe_test_wait_for_all_esp_objects_ready();
    return;
}

/*!**************************************************************
 * fbe_test_nemu_load_test1_config()
 ****************************************************************
 * @brief
 *  This function follows on the basic configuration to create
 *  a hardware configuration required by the test.  In this case
 *  it uses the default config and adds 2 SAS SLICs to it.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_test_nemu_load_test1_config(void)
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
            if(slot < 2)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = SNOWDEVIL_X;
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
    return status;
}

fbe_status_t fbe_test_nemu_load_test2_config(void)
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
            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = NO_MODULE;
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
    return status;
}

fbe_status_t fbe_test_nemu_load_test3_config(void)
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
            if(slot < 2)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = VORTEXQ_X;
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
    return status;
}

fbe_status_t fbe_test_nemu_load_test4_config(void)
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
            if(slot < 5)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
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


fbe_status_t fbe_test_nemu_load_test5_config(void)
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
            }
            else if(slot == 1)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ELNINO;
            }
            else if(slot == 2)
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

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}

fbe_status_t fbe_test_nemu_load_test6_config(void)
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
            if( (slot == 5) || (slot == 4) || (slot == 9) || (slot == 10) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else if( (slot == 0) || (slot == 1) || (slot == 2) || (slot == 6) || (slot == 7) || (slot == 8))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = GLACIER_REV_C;
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

fbe_status_t fbe_test_nemu_load_test8_config(void)
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
            if( (slot == 5) || (slot == 4) || (slot == 9) || (slot == 10) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else if( (slot == 0) || (slot == 1) || (slot == 2) || (slot == 6) || (slot == 7) || (slot == 8))
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

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}

fbe_status_t fbe_test_nemu_load_test9_config(void)
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
            if( (slot == 4) || (slot == 5) || (slot == 10) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else if( (slot == 2) || (slot == 7) || (slot == 9) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ELNINO_REV_B;
            }
            else if((slot == 1) || (slot == 8))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_2PORT_REV_C_84823;
            }
            else if((slot == 3) || (slot == 0) || (slot == 6))
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

fbe_status_t fbe_test_nemu_load_test10_config(void)
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
            if( (slot == 0) || (slot == 1) || (slot == 2) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = SNOWDEVIL_X;
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
 * fp6_nemu_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fp6_nemu test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   05/26/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp6_nemu_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end fp6_nemu_cleanup()
 ******************************************/

void fp6_nemu_intermediate_destroy(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;
    fbe_zero_memory(&package_list, sizeof(package_list));
	
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    package_list.number_of_packages = 4;
    package_list.package_list[0] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[1] = FBE_PACKAGE_ID_SEP_0;
    package_list.package_list[2] = FBE_PACKAGE_ID_ESP;
    package_list.package_list[3] = FBE_PACKAGE_ID_PHYSICAL;

    fbe_test_common_util_package_destroy(&package_list);
    status = fbe_api_terminator_destroy();
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "destroy terminator api failed");

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;
}
/*************************
 * end file fp6_nemu_test.c
 *************************/


