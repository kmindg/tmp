/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp14_tick_tock_test.c
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
fbe_status_t fp14_tick_tock_set_persist_ports(fbe_bool_t persist_flag);
static void fp14_tick_tock_load_again(void);
static void fp14_tick_tock_shutdown(void);
static fbe_status_t fp14_tick_tock_specl_config(void);



char * fp14_tick_tock_short_desc = "Verify port removal in ESP";
char * fp14_tick_tock_long_desc ="\
Tick-Tock\n\
         - is the crocodile that Captain Hook is afraid of in Peter Pan. \n\
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
 * fp14_tick_tock_test() 
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

void fp14_tick_tock_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port;
    fbe_u32_t                               slot; 
    fbe_u32_t                               port_index;
    fbe_char_t                              PortParamsKeyStr[255];
    fbe_char_t                              port_param_string[255];
    SPECL_PCI_DATA                          pci_data;
    fbe_char_t                              miniport_driver_name[255];
    fbe_char_t                              port_role_string[255];
    fbe_u32_t                               logical_port_number;

    mut_printf(MUT_LOG_LOW, "*** Tick-Tock Test case 1, Persist Ports ***\n");
    for(slot=0;slot<10;slot++)
    {
        if( (slot != 0) && (slot != 5))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < 4; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "Slot:%d PORT:%d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Tick-Tock Test case 2, Re-Load without the ports persisted but Registry set. ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    // destroy everything including the persistent data on disk
    fbe_test_esp_common_destroy_all();
    fp14_tick_tock_load_again();

    port_index = 0;

    for(slot=0;slot<10;slot++)
    {
        if( (slot != 0) && (slot != 5))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < 4; port++)
        {
            fbe_set_memory(PortParamsKeyStr, 0, 255);
            fbe_set_memory(port_param_string, 0, 255);
            fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d", port_index);
            
            status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                       PortParamsKeyStr,
                                       port_param_string,
                                       255,
                                       FBE_REGISTRY_VALUE_SZ,
                                       0,
                                       0,
                                       FALSE);
            
            mut_printf(MUT_LOG_LOW, "Reading Port:%d, Key: %s, RegString: %s\n",
                       port_index, PortParamsKeyStr, port_param_string);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "Slot:%d PORT:%d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
        
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
            port_index++;
        }
    }
    // check that the default parameter is populated
    fbe_set_memory(PortParamsKeyStr, 0, 255);
    fbe_set_memory(port_param_string, 0, 255);
    fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d", port_index);
    
    status = fbe_test_esp_registry_read(CPD_REG_PATH,
                               PortParamsKeyStr,
                               port_param_string,
                               255,
                               FBE_REGISTRY_VALUE_SZ,
                               0,
                               0,
                               FALSE);
    mut_printf(MUT_LOG_LOW, "Reading Port:%d, Key: %s, RegString: %s\n",
               port_index, PortParamsKeyStr, port_param_string);

    MUT_ASSERT_INT_EQUAL(fbe_compare_string("{[*]J(@,UNC,?)}", (fbe_u32_t)strlen("{[*]J(@,UNC,?)}"), port_param_string, (fbe_u32_t)strlen(port_param_string), FBE_TRUE), 0);

    fbe_test_esp_check_mgmt_config_completion_event();

    
    return;
}
/******************************************
 * end fp14_tick_tock_test()
 ******************************************/
/*!**************************************************************
 * fp14_tick_tock_setup()
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
void fp14_tick_tock_setup(void)
{
    fbe_status_t status;

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    fbe_test_reg_set_persist_ports_true();

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp14_tick_tock_specl_config);
}
/**************************************
 * end fp14_tick_tock_setup()
 **************************************/

fbe_status_t fp14_tick_tock_specl_config(void)
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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = GLACIER_REV_C;
            }
            else if(slot == 5)
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

    return status;
}


/*!**************************************************************
 * fp14_tick_tock_load_again()
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
void fp14_tick_tock_load_again(void)
{
    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp14_tick_tock_specl_config);
}

/*!**************************************************************
 * fp14_tick_tock_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp14_tick_tock test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp14_tick_tock_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp14_tick_tock_cleanup()
 ******************************************/


/*!**************************************************************
 * fp14_tick_tock_shutdown()
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

void fp14_tick_tock_shutdown(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}

fbe_status_t fp14_tick_tock_set_persist_ports(fbe_bool_t persist_flag)
{
    fbe_status_t status;
    status = fbe_test_esp_registry_write(ESP_REG_PATH,
                       FBE_MODULE_MGMT_PORT_PERSIST_KEY,
                       FBE_REGISTRY_VALUE_DWORD,
                       &persist_flag,
                       sizeof(fbe_bool_t));
    return status;
}

/*************************
 * end file fp14_tick_tock_test.c
 *************************/

