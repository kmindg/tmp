/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp4_reki_test.c
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
#include "physical_package_tests.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void fp4_reki_load_again(void);
static void fp4_reki_shutdown(void);
static void fp4_reki_setup_task2(void);
static void fp4_reki_load_specl_config(void);
static void fp4_reki_spb_setup(void);
static fbe_status_t fbe_test_reki_load_large_specl_config(void);
void fbe_test_create_persistent_lun(void);
static void fbe_test_set_persistent_lun(fbe_lun_number_t lun_number);
static fbe_status_t fbe_test_set_persistent_lun_completion_function(fbe_status_t commit_status, fbe_persist_completion_context_t context);
static void fp4_reki_dualsp_setup(void);

char * fp4_reki_short_desc = "Verify the port persistence by ESP";
char * fp4_reki_long_desc ="\
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


#define FP4_REKI_TEST_NUMBER_OF_PHYSICAL_OBJECTS       66

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
/* Table containing actual enclosure and drive details to be inserted.
 */
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
 * fp4_reki_test() 
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

void fp4_reki_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port, slot;
    fbe_u32_t                               io_port_max;
    fbe_char_t                              PortParamsKeyStr[255];
    fbe_char_t                              port_param_string[255];
    SPECL_PCI_DATA                          pci_data;
    fbe_char_t                              miniport_driver_name[255];
    fbe_char_t                              port_role_string[255];
    fbe_u32_t                               logical_port_number;
    fbe_u8_t                                sp=0;
    fbe_u32_t                               port_index;

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
    mut_printf(MUT_LOG_LOW, "*** Reki Test case 1, Successful ***\n");

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 2, Missing Registry Parameters ***\n");
    
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    // unload and re-load ESP without the registry file this time
    fbe_test_esp_restart();

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
    mut_printf(MUT_LOG_LOW, "*** Reki Test case 2, Missing Registry Parameters Completed ***\n");


    mut_printf(MUT_LOG_LOW, "*** Reki Test case 3, Missing Registry Parameters Peer SP ***\n");
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp4_reki_spb_setup();

    io_port_max = 6;

    for(sp=0;sp<2;sp++)
    {
        if(sp == 0)
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
        else
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        }
        for(port = 0; port < io_port_max; port++)
        {
            esp_io_port_info.header.sp = sp;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
            esp_io_port_info.header.slot = 0;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "SP%d Mezzanine PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                       sp, port, esp_io_port_info.io_port_info.present, 
                       esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    
            fbe_set_memory(PortParamsKeyStr, 0, 255);
            fbe_set_memory(port_param_string, 0, 255);
            fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d", port);
    
            status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                       PortParamsKeyStr,
                                       port_param_string,
                                       255,
                                       FBE_REGISTRY_VALUE_SZ,
                                       0,
                                       0,
                                       FALSE);
    
            mut_printf(MUT_LOG_LOW, "SP%d Reading Port:%d, Key: %s, RegString: %s\n",
                       sp, port, PortParamsKeyStr, port_param_string);
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
    }
    mut_printf(MUT_LOG_LOW, "*** Reki Test case 3, Missing Registry Parameters Peer SP Completed ***\n");

    /*
     * SEP was still zeroing and did not unload fast enough causing the test to fail, give it a few extra
     * seconds to finish up before tearing down.
     */
    fbe_api_sleep(10000); 


    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fbe_test_esp_common_destroy_all_dual_sp();

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 4, SPB Only Persist ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);
    fp4_reki_spb_setup();
    
    sp = 1;

    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = sp;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "SP%d Mezzanine PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   sp, port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

        fbe_set_memory(PortParamsKeyStr, 0, 255);
        fbe_set_memory(port_param_string, 0, 255);
        fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d", port);

        status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                   PortParamsKeyStr,
                                   port_param_string,
                                   255,
                                   FBE_REGISTRY_VALUE_SZ,
                                   0,
                                   0,
                                   FALSE);

        mut_printf(MUT_LOG_LOW, "SP%d Reading Port:%d, Key: %s, RegString: %s\n",
                   sp, port, PortParamsKeyStr, port_param_string);
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

    fbe_test_esp_restart();

    for(port = 0; port < io_port_max; port++)
    {
        esp_io_port_info.header.sp = sp;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = 0;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "SP%d Mezzanine PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   sp, port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);

        fbe_set_memory(PortParamsKeyStr, 0, 255);
        fbe_set_memory(port_param_string, 0, 255);
        fbe_sprintf(PortParamsKeyStr, 255, "PortParams%d", port);

        status = fbe_test_esp_registry_read(CPD_REG_PATH,
                                   PortParamsKeyStr,
                                   port_param_string,
                                   255,
                                   FBE_REGISTRY_VALUE_SZ,
                                   0,
                                   0,
                                   FALSE);

        mut_printf(MUT_LOG_LOW, "SP%d Reading Port:%d, Key: %s, RegString: %s\n",
                   sp, port, PortParamsKeyStr, port_param_string);
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

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 4, completed successfully ***\n");

    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 5, 4BE and 4FE SLICs no Faults Ports Configured***\n");
    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_reki_load_large_specl_config);

    io_port_max = 4;
    port_index = 0;
    sp = 0;

    for(slot=0;slot<10;slot++)
    {
        if((slot == 2) || (slot == 3))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            if((slot == 9) && (port > 1))
            {
                // two port SLIC
                continue;
            }
            esp_io_port_info.header.sp = sp;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "SP%d SLIC Slot %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   sp, slot, port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
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
    
            mut_printf(MUT_LOG_LOW, "SP%d Reading PortIndex:%d, Key: %s, RegString: %s\n",
                       sp, port_index, PortParamsKeyStr, port_param_string);
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

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 5 completed successfully ***\n");

    fbe_test_esp_restart();

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 6 reboot with large config ports still persisted ***\n");

    io_port_max = 4;
    port_index = 0;
    sp = 0;

    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            if((slot == 9) && (port > 1))
            {
                // two port SLIC
                continue;
            }
            esp_io_port_info.header.sp = sp;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "SP%d SLIC Slot %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   sp, slot, port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
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
    
            mut_printf(MUT_LOG_LOW, "SP%d Reading PortIndex:%d, Key: %s, RegString: %s\n",
                       sp, port_index, PortParamsKeyStr, port_param_string);
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

    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 7, Secondary SP Does Persist Operation ***\n");
    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fbe_test_reki_load_large_specl_config);

     fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

     /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_set_persist_ports(TRUE);

    fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);

    fbe_test_reki_load_large_specl_config();

    fp_test_start_physical_package();

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();


    io_port_max = 4;
    port_index = 0;
    sp = 1;

    for(slot=0;slot<10;slot++)
    {
        if((slot ==2) || (slot == 3))
        {
            // empty slots
            continue;
        }
        for(port = 0; port < io_port_max; port++)
        {
            if((slot == 9) && (port > 1))
            {
                // two port SLIC
                continue;
            }
            esp_io_port_info.header.sp = sp;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "SP%d SLIC Slot %d PORT: %d, present 0x%x, logical number %d, status 0x%x\n", 
                   sp, slot, port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.logical_number, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    
            
            port_index++;
    
        }
    }

    mut_printf(MUT_LOG_LOW, "*** Reki Test case 7 completed successfully ***\n");

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end fp4_reki_test()
 ******************************************/
/*!**************************************************************
 * fp4_reki_setup()
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
void fp4_reki_setup(void)
{
    fbe_status_t status;


    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE); 
    status = fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    fp4_reki_load_specl_config();

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);
}

void fp4_reki_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    /* we expect some error reported from raw read, and database should rebuild it */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for fp4_reki dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        //chromatic_create_physical_config(FBE_FALSE);
        fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        //chromatic_create_physical_config(FBE_FALSE);
        fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);
    
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
        fbe_test_wait_for_all_esp_objects_ready();
    }

}
/**************************************
 * end fp4_reki_setup()
 **************************************/


void fp_test_start_physical_package(void)
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
    status = fbe_api_wait_for_number_of_objects(FP4_REKI_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == FP4_REKI_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");
 }

static void fp4_reki_load_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return;
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
}

fbe_status_t fbe_test_reki_load_large_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, sp = 0;
    PSPECL_SFI_MASK_DATA            sfi_mask_data = {0};

    sfi_mask_data = (PSPECL_SFI_MASK_DATA) malloc (sizeof (SPECL_SFI_MASK_DATA));
    if(sfi_mask_data == NULL)
    {
        return status;
    }
    mut_printf(MUT_LOG_LOW, "*** Reki Test Creating Large SPECL Config ***\n");

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    for(sp=0;sp<2;sp++)
    {
        sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount = TOTAL_IO_MOD_PER_BLADE;
        for(slot = 0; slot < TOTAL_IO_MOD_PER_BLADE; slot++)
        {
            if( (slot == 4) || (slot == 5) || (slot == 6) || (slot == 10) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else if( (slot == 0) || (slot == 1) || (slot == 7) || (slot == 8))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = GLACIER_REV_C;
            }
            else if(slot == 9)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MAELSTROM;
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
 * fp4_reki_setup_task2()
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
void fp4_reki_setup_task2(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //debug only
    mut_printf(MUT_LOG_LOW, "=== configured terminator ===\n");

    status = fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== fp config loading is done ===\n");

    fp4_reki_load_specl_config();

    mut_printf(MUT_LOG_LOW, "=== SPECL SFI Mezzanine config is done ===\n");


    status = fbe_test_startPhyPkg(TRUE, FP_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== phy pkg is started ===\n");

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== env mgmt pkg started ===\n");

}
/**************************************
 * end fp4_reki_setup()
 **************************************/



/*!**************************************************************
 * fp4_reki_load_again()
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
void fp4_reki_load_again(void)
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

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    //
    mut_printf(MUT_LOG_LOW, "=== env mgmt pkg started ===\n");

}

/*!**************************************************************
 * fp4_reki_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp4_reki test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp4_reki_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end fp4_reki_cleanup()
 ******************************************/


/*!**************************************************************
 * fp4_reki_shutdown()
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

void fp4_reki_shutdown(void)
{
    fbe_test_esp_common_destroy();
    return;
}

void fp_test_set_persist_ports(fbe_bool_t persist_flag)
{
    fbe_test_esp_registry_write(ESP_REG_PATH,
                       FBE_MODULE_MGMT_PORT_PERSIST_KEY,
                       FBE_REGISTRY_VALUE_DWORD,
                       &persist_flag,
                       sizeof(fbe_bool_t));
    return;
}

void fbe_test_reg_set_persist_ports_true(void)
{
    fp_test_set_persist_ports(TRUE);
}

void fp4_reki_spb_setup(void)
{
    fbe_status_t status;

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    status = fbe_test_load_fp_config(SPID_OBERON_3_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== SPB config loading is done ===\n");

    fp4_reki_load_specl_config();

    fp_test_start_physical_package();
    
    /* Load sep and neit packages */

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
}




fbe_status_t fbe_test_load_fp_config(SPID_HW_TYPE platform_type)
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
    fbe_u32_t                               slotCount = 15;
    fbe_u32_t                               enclosure;
    fbe_terminator_board_info_t             board_info;
    fbe_terminator_sas_port_info_t          sas_port;
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


    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < FP4_REKI_TEST_ENCL_MAX; enclosure++)
    { 
        if ((enclosure == 0) && (platform_type == SPID_OBERON_1_HW_TYPE))
        {
            fbe_test_pp_util_insert_dpe_enclosure(0,
                                                  enclosure,
                                                  FBE_SAS_ENCLOSURE_TYPE_RHEA,
                                                  port0_handle,
                                                  &port0_encl_handle[enclosure]);
        }
        else if ((enclosure == 0) && (platform_type == SPID_HYPERION_1_HW_TYPE))
        {
            fbe_test_pp_util_insert_dpe_enclosure(0,
                                                  enclosure,
                                                  FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
                                                  port0_handle,
                                                  &port0_encl_handle[enclosure]);
        }
/*
        else if ((enclosure == 0) && (platform_type == SPID_TRITON_1_HW_TYPE))
        {
            fbe_test_pp_util_insert_dpe_enclosure(0,
                                                  enclosure,
                                                  FBE_SAS_ENCLOSURE_TYPE_CALYPSO,
                                                  port0_handle,
                                                  &port0_encl_handle[enclosure]);
        }
 */
        else
        {
            fbe_test_pp_util_insert_viper_enclosure(0, 
                                                    enclosure,
                                                    port0_handle,
                                                    &port0_encl_handle[enclosure]);
        }
    }

    /* Insert drives for enclosures.
     */
    if (platform_type == SPID_OBERON_1_HW_TYPE)
    {
        slotCount = 12;
    }
    for (enclosure = 0; enclosure < FP4_REKI_TEST_ENCL_MAX; enclosure++) 
    {
        for ( slot = 0; slot < slotCount; slot++)
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

    mut_printf(MUT_LOG_TEST_STATUS, "Setting SFP status to INSERTED...");
    fbe_zero_memory(&configured_sfp_info,sizeof(fbe_cpd_shim_sfp_media_interface_info_t));
    configured_sfp_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_GOOD;
    configured_sfp_info.condition_additional_info = 0;
    status = fbe_api_terminator_set_sfp_media_interface_info(port0_handle,&configured_sfp_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    return FBE_STATUS_OK;
}

void fbe_test_esp_restart(void)
{
    fbe_status_t status;
    fbe_test_package_list_t package_list;
    fbe_zero_memory(&package_list, sizeof(package_list));

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    package_list.number_of_packages = 3;
    package_list.package_list[0] = FBE_PACKAGE_ID_ESP;
    package_list.package_list[1] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[2] = FBE_PACKAGE_ID_SEP_0;
    
    fbe_test_common_util_package_destroy(&package_list);
    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);

    return;
}

void fbe_test_esp_unload(void)
{

    fbe_status_t status;
    fbe_test_package_list_t package_list;
    fbe_zero_memory(&package_list, sizeof(package_list));

    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    package_list.number_of_packages = 3;
    package_list.package_list[0] = FBE_PACKAGE_ID_ESP;
    package_list.package_list[1] = FBE_PACKAGE_ID_NEIT;
    package_list.package_list[2] = FBE_PACKAGE_ID_SEP_0;

    fbe_test_common_util_package_destroy(&package_list);
    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);

    return;

}

void fbe_test_esp_reload_with_existing_terminator_config(void)
{
    fbe_status_t status;
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);
}

void fbe_test_startEspAndSep(fbe_sim_transport_connection_target_t targetSp, SPID_HW_TYPE platformType)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: targetSp %d, platformType %d\n",
               __FUNCTION__, targetSp, platformType);

    /*
     * we do the setup on SPA side
     */
    fbe_api_sim_transport_set_target_server(targetSp);

    status = fbe_test_load_fp_config(platformType);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_test_startPhyPkg(TRUE, FP_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

}   // end of fbe_test_startEspAndSep

fbe_esp_test_common_config_info_t common_config_info[] = {
        {FBE_ESP_TEST_FP_CONIG, TRUE, FALSE, FP4_REKI_TEST_NUMBER_OF_PHYSICAL_OBJECTS},
        {FBE_ESP_TEST_SIMPLE_CONFIG, FALSE, FALSE, SIMPLE_CONFIG_MAX_OBJECTS},
        {FBE_ESP_TEST_SIMPLE_VOYAGER_CONFIG, FALSE, FALSE, 14}, /* 1 board, 1 port, 1 enclosure, SIMPLE_CONFIG_DRIVE_MAX*2 (two half of voyager) */
        {FBE_ESP_TEST_SIMPLE_VIKING_CONFIG, FALSE, FALSE, 15},  /* 1 board, 1 port, 1 IOSXP, 4 DRVSXP, SIMPLE_CONFIG_DRIVE_MAX PDOs and SIMPLE_CONFIG_DRIVE_MAX LDOs */
        {FBE_ESP_TEST_SIMPLE_CAYENNE_CONFIG, FALSE, FALSE, 12}, /* 1 board, 1 port, 1 IOSXP, 1 DRVSXP, SIMPLE_CONFIG_DRIVE_MAX PDOs and SIMPLE_CONFIG_DRIVE_MAX LDOs */
        {FBE_ESP_TEST_SIMPLE_NAGA_CONFIG, FALSE, FALSE, 13}, /* 1 board, 1 port, 1 IOSXP, 2 DRVSXP, SIMPLE_CONFIG_DRIVE_MAX PDOs and SIMPLE_CONFIG_DRIVE_MAX LDOs */
        {FBE_ESP_TEST_SIMPLE_TABASCO_CONFIG, FALSE, FALSE, 11}, // 1 board, 1 port, 1 IOSXP, SIMPLE_CONFIG_DRIVE_MAX PDOs and SIMPLE_CONFIG_DRIVE_MAX LDOs
        {FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_LAPA_RIOS_CONIG, FALSE, FALSE, SIMPLE_CONFIG_MAX_OBJECTS},
        {FBE_ESP_TEST_NAXOS_VIPER_CONIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_NAXOS_VOYAGER_CONIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_MIXED_CONIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_BASIC_CONIG, FALSE, FALSE, SIMPLE_CONFIG_MAX_OBJECTS},
        {FBE_ESP_TEST_CHAUTAUQUA_ANCHO_CONFIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_NAXOS_ANCHO_CONIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_NAXOS_TABASCO_CONIG, TRUE, TRUE, 0},
        {FBE_ESP_TEST_SIMPLE_MIRANDA_CONFIG, FALSE, FALSE, 11}, /* 1 board, 1 port, 1 enclosure, SIMPLE_CONFIG_DRIVE_MAX PDOs and SIMPLE_CONFIG_DRIVE_MAX LDOs */
        {FBE_ESP_TEST_SIMPLE_CALYPSO_CONFIG, FALSE, FALSE, 11}, /* 1 board, 1 port, 1 enclosure, SIMPLE_CONFIG_DRIVE_MAX PDOs and SIMPLE_CONFIG_DRIVE_MAX LDOs */
};

const fbe_u32_t common_config_table_size =  (sizeof(common_config_info)/sizeof(common_config_info[0]));

void fbe_test_startEspAndSep_with_common_config_dualSp(fbe_test_common_config config,
                                                       SPID_HW_TYPE platform_type,
                                                       void specl_config_func(void),
                                                       void reg_config_func(void))
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A, config, platform_type, specl_config_func, reg_config_func);
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_B, config, platform_type, specl_config_func, reg_config_func);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
}


void fbe_test_startEspAndSep_with_common_config(fbe_sim_transport_connection_target_t target_sp,
                                    fbe_test_common_config config,
                                    SPID_HW_TYPE platform_type,
                                    void specl_config_func(void),
                                    void reg_config_func(void))
{
    fbe_status_t status;
    fbe_u32_t maxEntries = 0;
    fbe_u32_t config_table_index;
    fbe_esp_test_common_config_info_t *config_info = NULL;
    fbe_test_params_t * p_test_param = NULL;
    fbe_u32_t index;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: targetSp %d, platformType %d\n",
               __FUNCTION__, target_sp, platform_type);

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /*
     * we do the setup on SPA side
     */
    fbe_api_sim_transport_set_target_server(target_sp);

    for( config_table_index = 0; config_table_index < common_config_table_size; config_table_index++ )
    {
        if(common_config_info[config_table_index].config == config)
        {
            config_info = common_config_info + config_table_index;
            break;
        }
    }
    MUT_ASSERT_INT_EQUAL(config_info != NULL, TRUE);

    if(!config_info->config_init_terminator)
    {
        status = fbe_api_terminator_init();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    switch(config)
    {
        case FBE_ESP_TEST_FP_CONIG:
            status = fbe_test_load_fp_config(platform_type);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_VIPER);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_VOYAGER_CONFIG:
            status = fbe_test_esp_load_simple_config_with_voyager(platform_type);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG:
            maxEntries = fbe_test_get_chautauqua_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_chautauqua_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
                {
                    status = chautauqua_load_and_verify_with_platform(p_test_param, platform_type);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                }
            }
            break;
        case FBE_ESP_TEST_LAPA_RIOS_CONIG:
            fbe_test_load_lapa_rios_config();
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_NAXOS_VIPER_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;                      
        case FBE_ESP_TEST_CHAUTAUQUA_ANCHO_CONFIG:
            maxEntries = fbe_test_get_chautauqua_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_chautauqua_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_ANCHO)
                {
                    status = chautauqua_load_and_verify_with_platform(p_test_param, platform_type);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                }
            }
            break;
        case FBE_ESP_TEST_NAXOS_ANCHO_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if (p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_ANCHO)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;
        case FBE_ESP_TEST_NAXOS_VOYAGER_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;
        case FBE_ESP_TEST_MIXED_CONIG:
            status = fbe_test_load_mixed_config();
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_BASIC_CONIG:
            status = fbe_test_esp_load_basic_terminator_config(platform_type);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_VIKING_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_CAYENNE_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_NAGA_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_TABASCO_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_TABASCO);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_MIRANDA_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_MIRANDA);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_CALYPSO_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_CALYPSO);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_NAXOS_TABASCO_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if (p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_TABASCO)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;
        default:
            MUT_ASSERT_TRUE(FALSE);
    }

    if(specl_config_func != NULL)
    {
        specl_config_func();
    }

    if(reg_config_func != NULL)
    {
        reg_config_func();
    }

    if(!config_info->config_load_pp)
    {
        status = fbe_test_startPhyPkg(TRUE, config_info->num_ojbects);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    /* Load the logical packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
    
    fbe_test_common_util_test_setup_init();

    fbe_api_base_config_disable_all_bg_services();
}   // end of fbe_test_startEspAndSep

void fbe_test_startEsp_with_common_config_dualSp(fbe_test_common_config config,
                                                       SPID_HW_TYPE platform_type,
                                                       void specl_config_func(void),
                                                       void reg_config_func(void))
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A, config, platform_type, specl_config_func, reg_config_func);
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_B, config, platform_type, specl_config_func, reg_config_func);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
}



void fbe_test_startEsp_with_common_config(fbe_sim_transport_connection_target_t target_sp,
                                    fbe_test_common_config config,
                                    SPID_HW_TYPE platform_type,
                                    void specl_config_func(void),
                                    void reg_config_func(void))
{
    fbe_status_t status;
    fbe_u32_t maxEntries = 0;
    fbe_u32_t config_table_index;
    fbe_esp_test_common_config_info_t *config_info = NULL;
    fbe_test_params_t * p_test_param = NULL;
    fbe_u32_t index;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: targetSp %d, platformType %d\n",
               __FUNCTION__, target_sp, platform_type);

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /*
     * we do the setup on SPA side
     */
    fbe_api_sim_transport_set_target_server(target_sp);

    for( config_table_index = 0; config_table_index < common_config_table_size; config_table_index++ )
    {
        if(common_config_info[config_table_index].config == config)
        {
            config_info = common_config_info + config_table_index;
            break;
        }
    }
    MUT_ASSERT_INT_EQUAL(config_info != NULL, TRUE);

    if(!config_info->config_init_terminator)
    {
        status = fbe_api_terminator_init();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    switch(config)
    {
        case FBE_ESP_TEST_FP_CONIG:
            status = fbe_test_load_fp_config(platform_type);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_VIPER);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_VOYAGER_CONFIG:
            status = fbe_test_esp_load_simple_config_with_voyager(platform_type);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG:
            maxEntries = fbe_test_get_chautauqua_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_chautauqua_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
                {
                    status = chautauqua_load_and_verify_with_platform(p_test_param, platform_type);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                }
            }
            break;
        case FBE_ESP_TEST_LAPA_RIOS_CONIG:
            fbe_test_load_lapa_rios_config();
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_NAXOS_VIPER_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_VIPER)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;                      
        case FBE_ESP_TEST_CHAUTAUQUA_ANCHO_CONFIG:
            maxEntries = fbe_test_get_chautauqua_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_chautauqua_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_ANCHO)
                {
                    status = chautauqua_load_and_verify_with_platform(p_test_param, platform_type);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                }
            }
            break;
        case FBE_ESP_TEST_NAXOS_ANCHO_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if (p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_ANCHO)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;
        case FBE_ESP_TEST_NAXOS_VOYAGER_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if(p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;
        case FBE_ESP_TEST_MIXED_CONIG:
            status = fbe_test_load_mixed_config();
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_BASIC_CONIG:
            status = fbe_test_esp_load_basic_terminator_config(platform_type);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_VIKING_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_CAYENNE_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_NAGA_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_TABASCO_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_TABASCO);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_MIRANDA_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_MIRANDA);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_SIMPLE_CALYPSO_CONFIG:
            status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_CALYPSO);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break;
        case FBE_ESP_TEST_NAXOS_TABASCO_CONIG:
            maxEntries = fbe_test_get_naxos_num_of_tests() ;
            for(index = 0; index < maxEntries; index++)
            {
                /* Load the configuration for test */
                p_test_param =  fbe_test_get_naxos_test_table(index);
                if (p_test_param->encl_type == FBE_SAS_ENCLOSURE_TYPE_TABASCO)
                {
                    status = naxos_load_and_verify_table_driven(p_test_param);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    break;
                }
            }
            break;
        default:
            MUT_ASSERT_TRUE(FALSE);
    }

    if(specl_config_func != NULL)
    {
        specl_config_func();
    }

    if(reg_config_func != NULL)
    {
        reg_config_func();
    }

    if(!config_info->config_load_pp)
    {
        status = fbe_test_startPhyPkg(TRUE, config_info->num_ojbects);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
}   // end of fbe_test_startEsp


/*************************
 * end file fp4_reki_test.c
 *************************/





