/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp12_snarf_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify that Back End Module info is gathered in ESP and it's ports can 
 *  be configured.
 * 
 * @version
 *   07/05/2012 - Created. bphilbin
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
#include "pp_utils.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_persist_interface.h"
#include "fbe/fbe_api_persist_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_private_space_layout.h"
#include "fp_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fp12_snarf_load_specl_config(void);
static fbe_status_t fp12_snarf_load_specl_config2(void);
static fbe_status_t fp12_snarf_load_specl_config3(void);
static fbe_status_t fp12_snarf_load_specl_config4(void);
static fbe_status_t fp12_snarf_load_specl_config5(void);
fbe_status_t fbe_test_load_defiant_config(void);

char * fp12_snarf_short_desc = "Verify the module mgmt object support for the Jetfire and Back End Module";
char * fp12_snarf_long_desc ="\
Snarf\n\
        - is a fictional character from the 80s cartoon series Thundercats.\n\
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
STEP 2: Get IO module info from physical package into management module object\n\
STEP 3: Get IO port info for that IO module from PP into management module object\n\
STEP 4: Get IO module info into ESP\n\
        - Verify that the module is inserted\n\
STEP 5: Get IO port info for that io module into ESP \n\
        - Verify that the port is present\n\
";



/*!**************************************************************
 * fp12_snarf_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/05/2012 - Created. bphilbin
 *
 ****************************************************************/

void fp12_snarf_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_io_module_info_t             esp_io_module_info = {0};
    fbe_esp_module_mgmt_module_status_t         module_status = {0};

    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   port;
    fbe_u32_t                                   componentCountPerBlade = 0;
    fbe_status_t                                fbeStatus = FBE_STATUS_OK;
    fbe_object_id_t                             objectId;
    fbe_board_mgmt_io_port_info_t               ioPortInfo = {0};
    fbe_board_mgmt_io_comp_info_t               io_comp_info = {0};
    fbe_u32_t                                   io_port_max = 0;

    fbe_char_t                                  PortParamsKeyStr[255];
    fbe_char_t                                  port_param_string[255];
    SPECL_PCI_DATA                              pci_data;
    fbe_char_t                                  miniport_driver_name[255];
    fbe_char_t                                  port_role_string[255];
    fbe_u32_t                                   logical_port_number;



    // STEP 1 - verify the io module info
    mut_printf(MUT_LOG_LOW, "*** Verifying IO Module status ***\n");

    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);
    fbeStatus = fbe_api_board_get_bem_count_per_blade(objectId, &componentCountPerBlade);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(componentCountPerBlade == 1);
   
    mut_printf(MUT_LOG_LOW, "*** Get IO Module count Successfully, Count: %d. ***\n", componentCountPerBlade * SP_ID_MAX);    

    fbeStatus = fbe_api_board_get_bem_info(objectId, &io_comp_info);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(io_comp_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD);
    MUT_ASSERT_TRUE(io_comp_info.inserted == FBE_MGMT_STATUS_TRUE);     
    MUT_ASSERT_TRUE(io_comp_info.faultLEDStatus == LED_BLINK_OFF);
    MUT_ASSERT_TRUE(io_comp_info.powerStatus == FBE_POWER_STATUS_POWER_ON);   


    mut_printf(MUT_LOG_LOW, "*** Step 1: got BEM Info from Physical Package ***\n");

   // STEP -2 get the io port info for SPA BEM port 0
    ioPortInfo.associatedSp = SP_A;
    ioPortInfo.slotNumOnBlade = 0;
    ioPortInfo.portNumOnModule = 0;
    ioPortInfo.deviceType = FBE_DEVICE_TYPE_BACK_END_MODULE;
    fbeStatus = fbe_api_board_get_io_port_info(objectId, &ioPortInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "*** Step 2: got IO Port Info from PP ***\n");

    //specify to get information about Back End Module 0 on SP A
    esp_io_module_info.header.sp = 0;
    esp_io_module_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
    esp_io_module_info.header.slot = 0;

    // verify the io module info
    status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
    mut_printf(MUT_LOG_LOW, "BEM inserted 0x%x, status 0x%x\n", 
        esp_io_module_info.io_module_phy_info.inserted, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);
    
    mut_printf(MUT_LOG_LOW, "*** Step 3: got BEM Info into ESP ***\n");


    // get the port info for that IO module
    esp_io_port_info.header.sp = 0;
    esp_io_port_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
    esp_io_port_info.header.slot = 0;
    esp_io_port_info.header.port = 0;

    status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
    mut_printf(MUT_LOG_LOW, "Io port present 0x%x, status 0x%x\n", 
        esp_io_port_info.io_port_info.present, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

    mut_printf(MUT_LOG_LOW, "*** Step 4: got IO Port Info into ESP ***\n");

    io_port_max = 2;
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "BEM PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

        fbe_set_memory(PortParamsKeyStr, 0, 255);
        fbe_set_memory(port_param_string, 0, 255);
        fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d",port);

        status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                   PortParamsKeyStr,
                                   port_param_string,
                                   255,
                                   FBE_REGISTRY_VALUE_SZ,
                                   0,
                                   0,
                                   FALSE);

        mut_printf(MUT_LOG_LOW, "Reading Port:%d, Key: %s, RegString: %s\n",
                   port, PortParamsKeyStr, port_param_string);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        sscanf(port_param_string, "{[%d.%d.%d] J(%[^,],%[^,],%d)}",
               &pci_data.bus,             
               &pci_data.device, 
               &pci_data.function,
               miniport_driver_name, 
               port_role_string, 
               &logical_port_number);

        /* Validate the entry in the Registry matches the configuration */
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.pciData.bus, pci_data.bus);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.pciData.device, pci_data.device);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.pciData.function, pci_data.function);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, logical_port_number);
    }




    mut_printf(MUT_LOG_LOW, "*** Step 5: verified ports were persisted properly ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    fp12_snarf_destroy();

    mut_printf(MUT_LOG_LOW, "*** Step 6: Check that Defiant M4 Does not support Moonlite SLICs ***\n");

    fbe_test_esp_create_registry_file(FBE_TRUE);

    fp_test_load_dynamic_config(SPID_DEFIANT_M4_HW_TYPE, fp12_snarf_load_specl_config2);

    //specify to get information about SLIC 0 on SP A
    module_status.header.sp = 0;
    module_status.header.type = FBE_DEVICE_TYPE_IOMODULE;
    module_status.header.slot = 0;

    // verify the io module info
    fbe_api_esp_module_mgmt_get_module_status(&module_status);

    MUT_ASSERT_INT_EQUAL(module_status.state, MODULE_STATE_UNSUPPORTED_MODULE);
    MUT_ASSERT_INT_EQUAL(module_status.substate, MODULE_SUBSTATE_UNSUPPORTED_MODULE);

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Step 6: Defiant M4 Does not support Moonlite SLICs completed Successfully ***\n");

    fp12_snarf_destroy();

    mut_printf(MUT_LOG_LOW, "*** Step 7: Defiant M3 Supports only 6 Back Ends ***\n");

    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_DEFIANT_M3_HW_TYPE, fp12_snarf_load_specl_config3);

    // check the BEM ports are configured

    io_port_max = 2;
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "BEM PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }

    //check that SLIC in slot 0 is configured
    io_port_max = 4;
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "SLIC 0 PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }

    mut_printf(MUT_LOG_LOW, "*** Step 7: Defiant M3 6 Back Ends Configured, Insert another Moonlite ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_esp_unload();

    fp12_snarf_load_specl_config4();

    fbe_api_sleep(4000);

    fbe_test_esp_reload_with_existing_terminator_config();

    // check the BEM ports are configured

    io_port_max = 2;
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "BEM PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }

    //check that SLIC in slot 0 is configured
    io_port_max = 4;
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "SLIC 0 PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }


    //check that SLIC in slot 1 is faulted exceeds limits

    io_port_max = 4;
    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 1;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "SLIC 1 PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_FAULTED);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);

    }



    mut_printf(MUT_LOG_LOW, "*** Step 7: Defiant M3 Supports only 6 Back Ends Completed Successfully***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    fp12_snarf_destroy();
    mut_printf(MUT_LOG_LOW, "*** Step 8: Defiant M3 Replace SLIC in slot 0 ***\n");


    // create initial config with Moonlite in Slot 0
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);
    fp_test_load_dynamic_config(SPID_DEFIANT_M3_HW_TYPE, fp12_snarf_load_specl_config2);
    fbe_test_esp_check_mgmt_config_completion_event();

    io_port_max = 11;
    for(port = 0; port < io_port_max; port++)
    {

        fbe_set_memory(PortParamsKeyStr, 0, 255);
        fbe_set_memory(port_param_string, 0, 255);
        fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d",port);

        status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                   PortParamsKeyStr,
                                   port_param_string,
                                   255,
                                   FBE_REGISTRY_VALUE_SZ,
                                   0,
                                   0,
                                   FALSE);

        mut_printf(MUT_LOG_LOW, "Reading Port:%d, Key: %s, RegString: %s\n",
                   port, PortParamsKeyStr, port_param_string);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);      
    }

    // restart now with an Eruption in Slot 0
    fbe_test_esp_unload();
    fp12_snarf_load_specl_config5();
    fbe_api_sleep(4000);
    fbe_test_esp_reload_with_existing_terminator_config();

    // check the registry

    io_port_max = 11;
    for(port = 0; port < io_port_max; port++)
    {
        fbe_set_memory(PortParamsKeyStr, 0, 255);
        fbe_set_memory(port_param_string, 0, 255);
        fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d",port);

        status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                   PortParamsKeyStr,
                                   port_param_string,
                                   255,
                                   FBE_REGISTRY_VALUE_SZ,
                                   0,
                                   0,
                                   FALSE);

        mut_printf(MUT_LOG_LOW, "Reading Port:%d, Key: %s, RegString: %s\n",
                   port, PortParamsKeyStr, port_param_string);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);       
    }

    //check that the default parameter is the last entry to be populated in the registry

    MUT_ASSERT_INT_EQUAL(strncmp("{[*]J(@,UNC,?)}", port_param_string, strlen("{[*]J(@,UNC,?)}")), 0);

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Step 8: Defiant M3 Replace SLIC in slot 0 completed successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** Snarf test passed. ***\n");

    return;
}
/******************************************
 * end fp12_snarf_test()
 ******************************************/
/*!**************************************************************
 * fp12_snarf_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   07/05/2012 - Created. bphilbin
 *
 ****************************************************************/
void fp12_snarf_setup(void)
{

    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_DEFIANT_M4_HW_TYPE, fp12_snarf_load_specl_config);
}
/**************************************
 * end fp12_snarf_setup()
 **************************************/

fbe_status_t fbe_test_load_defiant_config(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t	sas_port;
    fbe_terminator_sas_encl_info_t	sas_encl;

    fbe_api_terminator_device_handle_t	port_handle;
    fbe_api_terminator_device_handle_t	encl_handle;
    fbe_api_terminator_device_handle_t	drive_handle;

    fbe_u32_t no_of_ports = 0;
    fbe_u32_t no_of_encls = 0;
    fbe_u32_t slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M4_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (no_of_ports = 0; no_of_ports < FP_MAX_PORTS; no_of_ports ++)
    {
        /*insert a port*/
        sas_port.backend_number = no_of_ports;
        sas_port.io_port_number = 3 + no_of_ports;
        sas_port.portal_number = 5 + no_of_ports;
        sas_port.sas_address = 0x5000097A7800903F + no_of_ports;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        for ( no_of_encls = 0; no_of_encls < FP_MAX_ENCLS; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
            sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
            for (slot = 0; slot < FP_MAX_DRIVES; slot ++)
            {
                if(no_of_encls < 2 || (no_of_encls == 2 && slot >=2 && slot <= 13))
                {
                    status  = fbe_test_pp_util_insert_sas_drive(no_of_ports,
                                                                 no_of_encls,
                                                                 slot,
                                                                 520,
                                                                 0x10B5C730,
                                                                 &drive_handle);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
    }

    return status;
}

static fbe_status_t fp12_snarf_load_specl_config(void)
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

    return FBE_STATUS_OK;
}

static fbe_status_t fp12_snarf_load_specl_config2(void)
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

    return FBE_STATUS_OK;
}

static fbe_status_t fp12_snarf_load_specl_config3(void)
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

    return FBE_STATUS_OK;
}

static fbe_status_t fp12_snarf_load_specl_config4(void)
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

    return FBE_STATUS_OK;
}

static fbe_status_t fp12_snarf_load_specl_config5(void)
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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_2PORT_REV_C_84833;
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

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fp12_snarf_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fp12_snarf test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/05/2012 - Created. bphilbin
 *
 ****************************************************************/

void fp12_snarf_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp12_snarf_cleanup()
 ******************************************/
/*************************
 * end file fp12_snarf_test.c
 *************************/



