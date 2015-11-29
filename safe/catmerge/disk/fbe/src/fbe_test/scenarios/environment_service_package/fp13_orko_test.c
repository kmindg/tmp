/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp13_orko_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify module_mgmt object combined port handling.
 * 
 * @version
 *   09/12/2012 - Created. bphilbin
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
#include "sep_tests.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_port.h"

char * fp13_orko_short_desc = "Verify combined port handling in ESP";
char * fp13_orko_long_desc ="\
Orko\n\
        - was the little troolan character on the cartoon He-Man\n\
        - and the masters of the universe \n\
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


fbe_status_t fp13_orko_load_terminator_config(SPID_HW_TYPE platform_type);
static fbe_status_t fp13_orko_load_specl_config(void);
static fbe_status_t fp13_orko_load_task2_specl_config(void);
static fbe_status_t fp13_orko_load_task3_specl_config(void);
void fp13_orko_test_start_physical_package(void);
static void fp13_orko_task2_setup(void);
static void fp13_orko_task3_setup(void);
static fbe_status_t fp13_orko_load_task2_terminator_config(SPID_HW_TYPE platform_type);
static fbe_status_t fp13_orko_load_task3_terminator_config(SPID_HW_TYPE platform_type);


#define FP13_ORKO_TEST_NUMBER_OF_PHYSICAL_OBJECTS       69

typedef enum fp13_orko_test_enclosure_num_e
{
    FP13_ORKO_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    FP13_ORKO_TEST_ENCL2_WITH_SAS_DRIVES,
    FP13_ORKO_TEST_ENCL3_WITH_SAS_DRIVES,
    FP13_ORKO_TEST_ENCL4_WITH_SAS_DRIVES
} fp13_orko_test_enclosure_num_t;

typedef enum fp13_orko_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} fp13_orko_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    fp13_orko_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    fp13_orko_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;
/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        FP13_ORKO_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP13_ORKO_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP13_ORKO_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        FP13_ORKO_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

#define FP13_ORKO_TEST_ENCL_MAX (sizeof(encl_table)/sizeof(encl_table[0]))


#define COMBINED_CONNECTOR_MINI_SAS_HD_PAGE_00 \
{ \
0x10, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x10, 0x00, 0x24, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0xa0, 0x4d, 0x6f, 0x6c, 0x65, 0x78, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x20, 0x20, \
0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x60, 0x16, 0x30, 0x33, 0x38, 0x2d, 0x30, 0x30, 0x34, 0x2d, \
0x30, 0x39, 0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x55, 0x61, \
0x00, 0x00, 0x00, 0x00, 0x4d, 0x4c, 0x45, 0x31, 0x30, 0x31, 0x32, 0x31, 0x36, 0x30, 0x31, 0x34, \
0x31, 0x39, 0x20, 0x20, 0x32, 0x30, 0x31, 0x32, 0x30, 0x34, 0x32, 0x30, 0x00, 0x00, 0x00, 0xd3, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
};

static fbe_u8_t combined_connector_mini_sas_hd_page_00_contents[MAX_SERIAL_ID_BYTES]= COMBINED_CONNECTOR_MINI_SAS_HD_PAGE_00;

/*!**************************************************************
 * fp13_orko_test() 
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

void fp13_orko_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   port;
    fbe_u32_t                                   io_port_max = 0;
    fbe_u32_t                                   slot;
    fbe_u32_t                                   port_index;

    fbe_char_t                                  PortParamsKeyStr[255];
    fbe_char_t                                  port_param_string[255];
    SPECL_PCI_DATA                              pci_data;
    fbe_char_t                                  miniport_driver_name[255];
    fbe_char_t                                  port_role_string[255];
    fbe_u32_t                                   logical_port_number;
    fbe_esp_module_mgmt_set_port_t              set_port_cmd;



    // STEP 1 - verify the io module info
    mut_printf(MUT_LOG_LOW, "*** Step 1: got IO Port Info into ESP ***\n");

    port_index = 0;
    io_port_max = 4;
    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3) || (slot == 4)|| (slot == 9) || (slot == 10))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IO Module %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

            // check that combined ports are set correctly
            if((slot == 5)&&((port==2)||(port==3)))
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_TRUE);
            }
            else
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_FALSE);
            }

            if(!( (slot == 5) && (port == 3))) // the second combined connector has no registry entry
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
        
                mut_printf(MUT_LOG_LOW, "Reading Slot:%d Port:%d Key: %s, RegString: %s\n",
                           slot, port, PortParamsKeyStr, port_param_string);
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
                port_index++;
            }
        }
    }




    mut_printf(MUT_LOG_LOW, "*** Step 1: verified ports were persisted properly ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    fp13_orko_destroy();
    fp13_orko_task2_setup();

    // STEP 2 - verify combination port in ports 1 and 2 is faulted
    mut_printf(MUT_LOG_LOW, "*** Step 2: Check that invalid combination port location is faulted ***\n");

    port_index = 0;
    io_port_max = 4;
    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3) || (slot == 4)|| (slot == 9) || (slot == 10))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;

            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IO Module %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            if( (slot == 5) && ((port == 1) || (port == 2)) )
            {
                // these ports should be faulted and not persisted
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.port_state == FBE_PORT_STATE_FAULTED);
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.port_substate == FBE_PORT_SUBSTATE_FAULTED_SFP);
            }
            else
            {
    
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    
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
            port_index++;
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Step 2: verified invalid combination port faulted ***\n");

    fp13_orko_destroy();
    fp13_orko_task3_setup();

    mut_printf(MUT_LOG_LOW, "*** Step 3: Check that boot combined port is configured ***\n");

    port_index = 0;
    io_port_max = 4;
    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3) || (slot == 4)|| (slot == 9) || (slot == 10))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IO Module %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

            // check that combined ports are set correctly
            if((slot == 5)&&((port==0)||(port==1)))
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_TRUE);
            }
            else
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_FALSE);
            }

            if(!( (slot == 5) && (port == 1)))// the second combined connector does not have a registry entry
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
                mut_printf(MUT_LOG_LOW, "PortInfo Portal:%d, PCI_DATA Function:%d\n",
                           esp_io_port_info.io_port_info.portal_number, pci_data.function);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_info.portal_number, pci_data.function);
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, logical_port_number);
                port_index++;
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Step 3: completed successfully ***\n");

    mut_printf(MUT_LOG_TEST_STATUS, "*** Step 4: Destroying port configuration.\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_REMOVE_ALL_PORTS_CONFIG;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_esp_restart();


    port_index = 0;
    io_port_max = 4;
    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3) || (slot == 4)|| (slot == 9) || (slot == 10))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IO Module %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            if( (slot== 5) && (port == 0) )
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
            }
            else
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
            }

            // all ports combination should now be cleared.
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_FALSE);

            if(!( (slot == 5) && (port == 3)))// the third port is now out of order at the end of the parameters
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
                port_index++;
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Step 4: completed successfully ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Step 5: Re-Persist the Ports ***\n");

    fp_test_set_persist_ports(TRUE);

    fbe_test_esp_restart();


     port_index = 0;
    io_port_max = 4;
    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3) || (slot == 4)|| (slot == 9) || (slot == 10))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IO Module %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       slot, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

            // check that combined ports are set correctly
            if((slot == 5)&&((port==0)||(port==1)))
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_TRUE);
            }
            else
            {
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.is_combined_port == FBE_FALSE);
            }

            if(!( (slot == 5) && (port == 1)))// the second combined connector does not have a registry entry
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
                port_index++;
            }
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Step 5: completed successfully ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    mut_printf(MUT_LOG_LOW, "*** Orko test passed. ***\n");


    return;
}
/******************************************
 * end fp13_orko_test()
 ******************************************/
/*!**************************************************************
 * fp13_orko_setup()
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
void fp13_orko_setup(void)
{
    fbe_status_t status;

    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    status = fp13_orko_load_terminator_config(SPID_PROMETHEUS_M1_HW_TYPE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    status = fp13_orko_load_specl_config();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    fp13_orko_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
    return;
}
/**************************************
 * end fp13_orko_setup()
 **************************************/

void fp13_orko_task2_setup(void)
{
    fbe_status_t status;

    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    status = fp13_orko_load_task2_terminator_config(SPID_PROMETHEUS_M1_HW_TYPE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    status = fp13_orko_load_task2_specl_config();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    fp13_orko_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
    return;
}

void fp13_orko_task3_setup(void)
{
    fbe_status_t status;

    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    status = fp13_orko_load_task3_terminator_config(SPID_PROMETHEUS_M1_HW_TYPE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    status = fp13_orko_load_task3_specl_config();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    fp13_orko_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
    return;
}

void fp13_orko_test_start_physical_package()
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
    status = fbe_api_wait_for_number_of_objects(FP13_ORKO_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == FP13_ORKO_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");
    return;
}

fbe_status_t fp13_orko_load_terminator_config(SPID_HW_TYPE platform_type)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_api_terminator_device_handle_t      temp_port_handle;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[FP13_ORKO_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;
    fbe_terminator_board_info_t             board_info;
    fbe_terminator_sas_port_info_t	        sas_port;
    fbe_cpd_shim_port_lane_info_t           port_link_info;
    fbe_cpd_shim_hardware_info_t            hardware_info;
    fbe_cpd_shim_sfp_media_interface_info_t sfp_media_info;
    fbe_u32_t                               port_num;

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

    for(port_num = 0; port_num < 4; port_num++)
    {
        /*insert a port*/
        sas_port.backend_number = port_num;
        sas_port.io_port_number = 0;
        sas_port.portal_number = port_num;
        sas_port.sas_address = 0x5000097A7800903F;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &temp_port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if(port_num == 0)
        {
            port0_handle = temp_port_handle;
        }
    
        /* configure port hardware info */
        fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
        hardware_info.vendor = 0x11F8;
        hardware_info.device = 0x8001;
        hardware_info.bus = 0x35;
        hardware_info.slot = 0;
        hardware_info.function = port_num;
    
        status = fbe_api_terminator_set_hardware_info(temp_port_handle, &hardware_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        /* Set the link information to a good state*/
    
        fbe_zero_memory(&port_link_info,sizeof(port_link_info));
        port_link_info.portal_number = 0;
        port_link_info.link_speed = SPEED_SIX_GIGABIT;
        port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
        port_link_info.nr_phys = 2;
        port_link_info.phy_map = 0xF << (4 * port_num);
        port_link_info.nr_phys_enabled = 2;
        port_link_info.nr_phys_up = 2;
        status = fbe_api_terminator_set_port_link_info(temp_port_handle, &port_link_info);    
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        fbe_zero_memory(&sfp_media_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
        sfp_media_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
        sfp_media_info.condition_additional_info = 1; // speed len avail

        if(port_num == 2 || port_num == 3)
        {
            // 2 and 3 are a combined port, they share serial # and part #
            strncpy(sfp_media_info.emc_part_number, "038-004-096     ", FBE_PORT_SFP_EMC_DATA_LENGTH);
            strncpy(sfp_media_info.emc_serial_number, "MLE10121601419", FBE_PORT_SFP_EMC_DATA_LENGTH);
        }
        else
        {
            strncpy(sfp_media_info.emc_part_number, "0", FBE_PORT_SFP_EMC_DATA_LENGTH);
            fbe_sprintf(sfp_media_info.emc_serial_number, FBE_PORT_SFP_EMC_DATA_LENGTH, "%d", port_num);
        }
    
        status = fbe_api_terminator_set_sfp_media_interface_info(temp_port_handle, &sfp_media_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    
    }
    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < FP13_ORKO_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < FP13_ORKO_TEST_ENCL_MAX; enclosure++)
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

static fbe_status_t fp13_orko_load_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "*** fp13 Orko Test Creating Large SPECL Config ***\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if( (slot == 5) || (slot == 6) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
                if(slot == 5)
                {
                    // populate the sfp information for the combined connector
                    EmcpalCopyMemory(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[2].sfpStatus.type2.miniSasPage00.sfpEEPROM, 
                                     combined_connector_mini_sas_hd_page_00_contents, MAX_SERIAL_ID_BYTES);
                    EmcpalCopyMemory(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[3].sfpStatus.type2.miniSasPage00.sfpEEPROM, 
                                     combined_connector_mini_sas_hd_page_00_contents, MAX_SERIAL_ID_BYTES);
                }
            }
            else if( (slot == 0) || (slot == 1) || (slot == 7) || (slot == 8) )
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

    mut_printf(MUT_LOG_LOW, "*** Reki Test Large Config SFI Completed ***\n");

    return status;
}

static fbe_status_t fp13_orko_load_task2_terminator_config(SPID_HW_TYPE platform_type)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_api_terminator_device_handle_t      temp_port_handle;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[FP13_ORKO_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;
    fbe_terminator_board_info_t             board_info;
    fbe_terminator_sas_port_info_t	        sas_port;
    fbe_cpd_shim_port_lane_info_t           port_link_info;
    fbe_cpd_shim_hardware_info_t            hardware_info;
    fbe_cpd_shim_sfp_media_interface_info_t sfp_media_info;
    fbe_u32_t                               port_num;

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

    for(port_num = 0; port_num < 4; port_num++)
    {
        /*insert a port*/
        sas_port.backend_number = port_num;
        sas_port.io_port_number = 0;
        sas_port.portal_number = port_num;
        sas_port.sas_address = 0x5000097A7800903F;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &temp_port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if(port_num == 0)
        {
            port0_handle = temp_port_handle;
        }
    
        /* configure port hardware info */
        fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
        hardware_info.vendor = 0x11F8;
        hardware_info.device = 0x8001;
        hardware_info.bus = 306;
        hardware_info.slot = 0;
        hardware_info.function = port_num;
    
        status = fbe_api_terminator_set_hardware_info(temp_port_handle, &hardware_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        /* Set the link information to a good state*/
    
        fbe_zero_memory(&port_link_info,sizeof(port_link_info));
        port_link_info.portal_number = 0;
        port_link_info.link_speed = SPEED_SIX_GIGABIT;
        port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
        port_link_info.nr_phys = 2;
        port_link_info.phy_map = 0xF << (4 * port_num);
        port_link_info.nr_phys_enabled = 2;
        port_link_info.nr_phys_up = 2;
        status = fbe_api_terminator_set_port_link_info(temp_port_handle, &port_link_info);    
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        fbe_zero_memory(&sfp_media_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
        sfp_media_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
        sfp_media_info.condition_additional_info = 1; // speed len avail

        if(port_num == 1 || port_num == 2)
        {
            // 1 and 2 are a combined port, they share serial # and part #
            strncpy(sfp_media_info.emc_part_number, "038-004-096     ", FBE_PORT_SFP_EMC_DATA_LENGTH);
            strncpy(sfp_media_info.emc_serial_number, "MLE10121601419", FBE_PORT_SFP_EMC_DATA_LENGTH);
        }
        else
        {
            strncpy(sfp_media_info.emc_part_number, "0", FBE_PORT_SFP_EMC_DATA_LENGTH);
            fbe_sprintf(sfp_media_info.emc_serial_number, FBE_PORT_SFP_EMC_DATA_LENGTH, "%d", port_num);
        }
    
        status = fbe_api_terminator_set_sfp_media_interface_info(temp_port_handle, &sfp_media_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    
    }
    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < FP13_ORKO_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < FP13_ORKO_TEST_ENCL_MAX; enclosure++)
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

static fbe_status_t fp13_orko_load_task2_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "*** fp13 Orko Test Creating Large SPECL Config ***\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if( (slot == 5) || (slot == 6) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
                if(slot == 5)
                {
                    // populate the sfp information for the combined connector
                    EmcpalCopyMemory(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[1].sfpStatus.type2.miniSasPage00.sfpEEPROM, 
                                     combined_connector_mini_sas_hd_page_00_contents, MAX_SERIAL_ID_BYTES);
                    EmcpalCopyMemory(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[2].sfpStatus.type2.miniSasPage00.sfpEEPROM, 
                                     combined_connector_mini_sas_hd_page_00_contents, MAX_SERIAL_ID_BYTES);
                }
            }
            else if( (slot == 0) || (slot == 1) || (slot == 7) || (slot == 8) )
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

    mut_printf(MUT_LOG_LOW, "*** Reki Test Large Config SFI Completed ***\n");

    return status;
}


static fbe_status_t fp13_orko_load_task3_terminator_config(SPID_HW_TYPE platform_type)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_api_terminator_device_handle_t      temp_port_handle;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[FP13_ORKO_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;
    fbe_terminator_board_info_t             board_info;
    fbe_terminator_sas_port_info_t	        sas_port;
    fbe_cpd_shim_port_lane_info_t           port_link_info;
    fbe_cpd_shim_hardware_info_t            hardware_info;
    fbe_cpd_shim_sfp_media_interface_info_t sfp_media_info;
    fbe_u32_t                               port_num;

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

    for(port_num = 0; port_num < 4; port_num++)
    {
        /*insert a port*/
        sas_port.backend_number = port_num;
        sas_port.io_port_number = 0;
        sas_port.portal_number = port_num;
        sas_port.sas_address = 0x5000097A7800903F;
        sas_port.port_type = FBE_PORT_TYPE_SAS_PMC;
        status  = fbe_api_terminator_insert_sas_port (&sas_port, &temp_port_handle);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if(port_num == 0)
        {
            port0_handle = temp_port_handle;
        }
    
        /* configure port hardware info */
        fbe_zero_memory(&hardware_info,sizeof(fbe_cpd_shim_hardware_info_t));
        hardware_info.vendor = 0x11F8;
        hardware_info.device = 0x8001;
        hardware_info.bus = 306;
        hardware_info.slot = 0;
        hardware_info.function = port_num;
    
        status = fbe_api_terminator_set_hardware_info(temp_port_handle, &hardware_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
        /* Set the link information to a good state*/
    
        fbe_zero_memory(&port_link_info,sizeof(port_link_info));
        port_link_info.portal_number = 0;
        port_link_info.link_speed = SPEED_SIX_GIGABIT;
        port_link_info.link_state = CPD_SHIM_PORT_LINK_STATE_UP;
        port_link_info.nr_phys = 2;
        port_link_info.phy_map = 0xF << (4 * port_num);
        port_link_info.nr_phys_enabled = 2;
        port_link_info.nr_phys_up = 2;
        status = fbe_api_terminator_set_port_link_info(temp_port_handle, &port_link_info);    
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        fbe_zero_memory(&sfp_media_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
        sfp_media_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
        sfp_media_info.condition_additional_info = 1; // speed len avail

        if(port_num == 0 || port_num == 1)
        {
            // 0 and 1 are a combined port, they share serial # and part #
            strncpy(sfp_media_info.emc_part_number, "038-004-095     ", FBE_PORT_SFP_EMC_DATA_LENGTH);
            strncpy(sfp_media_info.emc_serial_number, "MLE10121601419", FBE_PORT_SFP_EMC_DATA_LENGTH);
        }
        else
        {
            strncpy(sfp_media_info.emc_part_number, "0", FBE_PORT_SFP_EMC_DATA_LENGTH);
            fbe_sprintf(sfp_media_info.emc_serial_number, FBE_PORT_SFP_EMC_DATA_LENGTH, "%d", port_num);
        }
    
        status = fbe_api_terminator_set_sfp_media_interface_info(temp_port_handle, &sfp_media_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    
    }
    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < FP13_ORKO_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < FP13_ORKO_TEST_ENCL_MAX; enclosure++)
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

static fbe_status_t fp13_orko_load_task3_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }

    mut_printf(MUT_LOG_LOW, "*** fp13 Orko Test Creating Large SPECL Config ***\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if( (slot == 5) || (slot == 6) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
                if(slot == 5)
                {
                    // populate the sfp information for the combined connector
                    EmcpalCopyMemory(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[0].sfpStatus.type2.miniSasPage00.sfpEEPROM, 
                                     combined_connector_mini_sas_hd_page_00_contents, MAX_SERIAL_ID_BYTES);
                    EmcpalCopyMemory(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[1].sfpStatus.type2.miniSasPage00.sfpEEPROM, 
                                     combined_connector_mini_sas_hd_page_00_contents, MAX_SERIAL_ID_BYTES);
                }
            }
            else if( (slot == 0) || (slot == 1) || (slot == 7) || (slot == 8) )
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

    mut_printf(MUT_LOG_LOW, "*** Reki Test Large Config SFI Completed ***\n");

    return status;
}

/*!**************************************************************
 * fp13_orko_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the fp13_orko test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   07/05/2012 - Created. bphilbin
 *
 ****************************************************************/

void fp13_orko_destroy(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp13_orko_cleanup()
 ******************************************/
/*************************
 * end file fp13_orko_test.c
 *************************/
