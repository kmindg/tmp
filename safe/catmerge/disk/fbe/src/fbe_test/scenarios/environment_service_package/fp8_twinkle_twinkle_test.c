/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp8_twinkle_twinkle_test.c
 ***************************************************************************
 *
 * @brief
 *  This test performs in family port configuration conversions.
 * 
 * @version
 *   03/11/2011 - Created. bphilbin
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



char * fp8_twinkle_twinkle_short_desc = "In Family Port Conversion Test";
char * fp8_twinkle_twinkle_long_desc ="\
Twinkle-Twinkle\n\
         - is the planet the fictional character FX, a pull string alien doll,\n\
         - originated in the cartoon series Fantastic Max. \n\
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
STEP 2: Persist Ports \n\
STEP 3: Set in Family Conversion Flag in Registry \n\
STEP 4: Load New Hardware Config \n\
";


/*
 *  LOCAL FUNCTIONS
 */
static fbe_status_t fp8_twinkle_twinkle_load_mezzanine_only_port_config(void);
static fbe_status_t fp8_twinkle_twinkle_load_mezzanine_and_single_fc_slic_config(void);
static fbe_status_t fp8_twinkle_twinkle_load_sas_and_4fc_slics_config(void);
static fbe_status_t fbe_test_load_fp8_twinkle_twinkle_config(void);
static void fp8_twinkle_twinkle_intermediate_destroy(void);


/*!**************************************************************
 * fp8_twinkle_twinkle_test() 
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
void fp8_twinkle_twinkle_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port;
    fbe_u32_t                               io_port_max;
    fbe_u32_t                               slot;
    fbe_char_t                              PortParamsKeyStr[255];
    fbe_char_t                              port_param_string[255];
    SPECL_PCI_DATA                          pci_data;
    fbe_char_t                              miniport_driver_name[255];
    fbe_char_t                              port_role_string[255];
    fbe_u32_t                               logical_port_number;
    fbe_module_mgmt_conversion_type_t       conversion_flag;

    fbe_test_esp_check_mgmt_config_completion_event();


    mut_printf(MUT_LOG_LOW, "*** Reki Test case 1, Persist Ports ***\n");
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
    

    mut_printf(MUT_LOG_LOW, "*** Twinkle Twinkle Test case 1, Persist Ports ***\n");
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
    mut_printf(MUT_LOG_LOW, "*** Twinkle-Twinkle - Test Case 1 Converting Hellcat Lite to Lightning ***\n");

    conversion_flag = FBE_MODULE_MGMT_CONVERSION_HCL_TO_SENTRY;

    fbe_test_esp_registry_write(ESP_REG_PATH,
                                FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_KEY,
                                FBE_REGISTRY_VALUE_DWORD,
                                &conversion_flag,
                                sizeof(fbe_u32_t));

    fbe_test_esp_unload();

    fp8_twinkle_twinkle_load_mezzanine_and_single_fc_slic_config();

    fbe_test_esp_reload_with_existing_terminator_config();

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

        if(port < 2)
        {
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
        }
        else
        {
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
        }
    }

    io_port_max = 4;

    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "IOM:%d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   esp_io_port_info.header.slot, port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
       
    }

    mut_printf(MUT_LOG_LOW, "*** Twinkle-Twinkle - Test Case 1 Completed Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** Twinkle-Twinkle - Test Case 2 Convert Sentry to Argonaut ***\n");

    /* clear out any configuration that may have been left around from a previous test */

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fbe_test_esp_unload();

    fp8_twinkle_twinkle_load_mezzanine_and_single_fc_slic_config();

    fbe_test_esp_reload_with_existing_terminator_config();

    conversion_flag = FBE_MODULE_MGMT_CONVERSION_SENTRY_TO_ARGONAUT;

    fbe_test_esp_registry_write(ESP_REG_PATH,
                                FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_KEY,
                                FBE_REGISTRY_VALUE_DWORD,
                                &conversion_flag,
                                sizeof(fbe_u32_t));

    fbe_test_esp_unload();

    fp8_twinkle_twinkle_load_sas_and_4fc_slics_config();

    fbe_test_esp_reload_with_existing_terminator_config();

    fbe_test_esp_check_mgmt_config_completion_event();
    /*
     * Check that the ports have moved to the expected locations 
     * SAS ports go to SLIC in slot 0.
     * 3 FC Mezzanine ports go to slic in slot 1.
     * 1 FC Mezzanine port (last one) goes to SLIC in slot 4 
     * SLIC ports are shifted over 2 spots.
     */

    io_port_max = 4;

    for(slot=0;slot<=4;slot++)
    {
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM:%d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       esp_io_port_info.header.slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            if(slot == 0)
            {
                if(port<2)
                {
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
                }
                else
                {
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
                }
            }
            else if(slot == 1)
            {
                if(port<3)
                {
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
                }
                else
                {
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
                }

            }
            else if(slot == 2)
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
            }
            else if(slot == 3)
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
            }
            else if(slot == 4)
            {
                if(port==0)
                {
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
                }
                else
                {
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
                }
            }
           
        }
    }
    mut_printf(MUT_LOG_LOW, "*** Twinkle-Twinkle - Test Case 2 Completed Successfully ***\n");
}


/*!**************************************************************
 * fp8_twinkle_twinkle_setup() 
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
void fp8_twinkle_twinkle_setup(void)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_OBERON_1_HW_TYPE, fp8_twinkle_twinkle_load_mezzanine_only_port_config);

    return;
}

void fp8_twinkle_twinkle_intermediate_destroy(void)
{
    fbe_test_esp_common_destroy();
    return;
}


void fp8_twinkle_twinkle_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy();
    return;
}


fbe_status_t fp8_twinkle_twinkle_load_mezzanine_only_port_config(void)
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

    return status;
}


fbe_status_t fp8_twinkle_twinkle_load_mezzanine_and_single_fc_slic_config(void)
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

    return status;
}


fbe_status_t fp8_twinkle_twinkle_load_sas_and_4fc_slics_config(void)
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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = HYPERNOVA;
            }
            else if( (slot == 1) || (slot == 2) || (slot == 3) || (slot == 4) )
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

    return status;
}

