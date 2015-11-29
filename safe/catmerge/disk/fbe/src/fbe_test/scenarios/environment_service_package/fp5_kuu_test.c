/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp5_kuu_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify that ports persist in ESP module mgmt.
 * 
 * @version
 *   09/20/2010 - Created. bphilbin
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
fbe_status_t fp5_kuu_set_persist_ports(fbe_bool_t persist_flag);
static void fp5_kuu_load_again(void);
static void fp5_kuu_shutdown(void);
static fbe_status_t fbe_test_kuu_load_asymmetric_config(void);



char * fp5_kuu_short_desc = "Verify the port persistence by ESP";
char * fp5_kuu_long_desc ="\
Kuu\n\
         - is a blah blah blah, wikipedia blah blah. \n\
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
 * fp5_kuu_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp5_kuu_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port;
    fbe_u32_t                               io_port_max;
    //fbe_esp_module_mgmt_set_mgmt_port_config_t mgmt_port_config;
    fbe_esp_module_mgmt_general_status_t    general_status = {0};



    status = fbe_api_esp_module_mgmt_general_status(&general_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(general_status.port_configuration_loaded, FBE_FALSE);

    /* Initialize registry parameter to persist ports */
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_restart();

    mut_printf(MUT_LOG_LOW, "*** Kuu Test case 1, Persist Ports ***\n");
    /* Verify that the Mezzanine ports have been configured. */
    io_port_max = 6;

    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "Mezzanine PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

    }

    fbe_api_sleep(10000);

    status = fbe_api_esp_module_mgmt_general_status(&general_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(general_status.port_configuration_loaded, FBE_TRUE);

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Kuu Test case 2, Re-Load port configuration. ***\n");

    fbe_test_esp_restart();

    /* Verify we still have the same config as the previous test case. */
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "Mezzanine PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }
    status = fbe_api_esp_module_mgmt_general_status(&general_status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(general_status.port_configuration_loaded, FBE_TRUE);

    fbe_test_esp_check_mgmt_config_completion_event();

    #if 0 // the mgmt module count isn't working correctly for SPB at the moment, disabling and will resolve the issue ASAP

    fbe_test_esp_common_destroy_all();

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_LOW, "*** Kuu Test case 3, SPA Loads First, SPB Persists Additional Ports***\n");
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_kuu_load_asymmetric_config);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    status = fp5_kuu_set_persist_ports(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_kuu_load_asymmetric_config);

    fbe_api_sleep(10000);

    mut_printf(MUT_LOG_LOW, "*** Kuu Test case 3, Configuring SPB management port speed***\n");

    // Update the management port speed on the first SP to boot to kick off a persistent write
     /* Init MGMT ID*/
    mgmt_port_config.phys_location.slot = 0;

    /* Verify the mgmt port speed change */
    mgmt_port_config.revert = FBE_TRUE;
    mgmt_port_config.mgmtPortRequest.mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_OFF;
    mgmt_port_config.mgmtPortRequest.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_100MBPS;
    mgmt_port_config.mgmtPortRequest.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_HALF;
    status = fbe_api_esp_module_mgmt_set_mgmt_port_config(&mgmt_port_config);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Kuu Test case 3, Waiting for management port configuration completion event***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    // restart both SPs
    fbe_test_esp_unload();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_esp_unload();

    fbe_test_esp_reload_with_existing_terminator_config();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_esp_reload_with_existing_terminator_config();

     for(port = 0; port < 4; port++)
    {
        esp_io_port_info.header.sp = 1;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 1;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "SLIC 1 PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }

     mut_printf(MUT_LOG_LOW, "*** Kuu Test case 3, Completed Successfully ***\n");

    // verify all ports are configured on SPB
#endif

    return;
}
/******************************************
 * end fp5_kuu_test()
 ******************************************/
/*!**************************************************************
 * fp5_kuu_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp5_kuu_setup(void)
{
    fbe_status_t status;
    fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

}
/**************************************
 * end fp5_kuu_setup()
 **************************************/


/*!**************************************************************
 * fp5_kuu_load_again()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure but leaving persistent
 *  storage information intact.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp5_kuu_load_again(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //debug only
    mut_printf(MUT_LOG_LOW, "=== configured terminator ===\n");

    status = fbe_test_load_simple_config();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    status = fbe_test_startPhyPkg(TRUE, SIMPLE_CONFIG_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== phy pkg is started ===\n");

    /* Load the logical packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== env mgmt pkg started ===\n");

}

/*!**************************************************************
 * fp5_kuu_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp5_kuu test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp5_kuu_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp5_kuu_cleanup()
 ******************************************/


/*!**************************************************************
 * fp5_kuu_shutdown()
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

void fp5_kuu_shutdown(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}

fbe_status_t fp5_kuu_set_persist_ports(fbe_bool_t persist_flag)
{
    fbe_status_t status;
    status = fbe_test_esp_registry_write(ESP_REG_PATH,
                       FBE_MODULE_MGMT_PORT_PERSIST_KEY,
                       FBE_REGISTRY_VALUE_DWORD,
                       &persist_flag,
                       sizeof(fbe_bool_t));
    return status;
}

fbe_status_t fbe_test_kuu_load_asymmetric_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "*** Kuu Test creating asymmetrical specl SLIC config ***\n");

    // first load a normal config
    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if(slot == 5)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else if(slot < 2)
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

    // now remove one of the SLICs
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].diplexFPGAPresent = FALSE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].inserted = FALSE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].moduleType = IO_MODULE_TYPE_SLIC_1_0;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].uniqueID = NO_MODULE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].type = 1;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].type1.faultLEDStatus = LED_BLINK_OFF;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].type1.powerEnable = FALSE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].type1.powerGood = FALSE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].type1.powerUpEnable = FALSE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].type1.powerUpFailure = FALSE;
    sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[0].ioStatus[1].transactionStatus = EMCPAL_STATUS_SUCCESS;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Kuu Test Asymmetric Config SFI Completed ***\n");

    // initialize the mgmt port settings
    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Set the vLan mode */
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].vLanConfigMode = CUSTOM_MGMT_ONLY_VLAN_MODE;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].vLanConfigMode = CUSTOM_MGMT_ONLY_VLAN_MODE;

    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].portStatus[0].portSpeed = SPEED_10_Mbps;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].portStatus[0].portSpeed = SPEED_10_Mbps;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].portStatus[0].portDuplex = HALF_DUPLEX;
    sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_B].mgmtStatus[0].portStatus[0].portDuplex = HALF_DUPLEX;

    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Get data and verify it*/
    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));

    sfi_mask_data->structNumber = SPECL_SFI_MGMT_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(sfi_mask_data->sfiSummaryUnions.mgmtStatus.mgmtSummary[SP_A].mgmtStatus[0].vLanConfigMode == CUSTOM_MGMT_ONLY_VLAN_MODE);

    return status;
}

/*************************
 * end file fp5_kuu_test.c
 *************************/
