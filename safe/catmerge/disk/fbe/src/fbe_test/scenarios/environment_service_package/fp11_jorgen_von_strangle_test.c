/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp11_jorgen_von_strangle_test.c
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

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
void fp11_jorgen_von_strangle_set_persist_ports(fbe_bool_t persist_flag);
static void fp11_jorgen_von_strangle_load_again(void);
static void fp11_jorgen_von_strangle_shutdown(void);
static void fp11_jorgen_von_strangle_setup_task2(void);
static fbe_status_t fp11_jorgen_von_strangle_load_specl_config(void);
static void fp11_jorgen_von_strangle_spb_setup(void);



char * fp11_jorgen_von_strangle_short_desc = "Verify BE 0 is auto persisted by ESP";
char * fp11_jorgen_von_strangle_long_desc ="\
Jorgen Von Strangle\n\
         - is a fairy in the Fairly Odd Parents cartoon series. \n\
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
 * fp11_jorgen_von_strangle_test() 
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

void fp11_jorgen_von_strangle_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port;
    fbe_char_t                              PortParamsKeyStr[255];
    fbe_char_t                              port_param_string[255];
    SPECL_PCI_DATA                          pci_data;
    fbe_char_t                              miniport_driver_name[255];
    fbe_char_t                              port_role_string[255];
    fbe_u32_t                               logical_port_number;

    mut_printf(MUT_LOG_LOW, "*** Jorgen Von Strangle Test case 1, Check BE 0 is configured ***\n");
    /* Verify that the Mezzanine ports have been configured. */

    for(port=0;port<6;port++)
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

    if(port == 0)
    {
        // port 0 is configured
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }
    else
    {
        // all other ports are not configured
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
    }


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
        if(port == 0)
        {
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, logical_port_number);
        }
    }
    mut_printf(MUT_LOG_LOW, "*** Jorgen Von Strangle Test case 1, Successful ***\n");

    

    return;


}
/******************************************
 * end fp11_jorgen_von_strangle_test()
 ******************************************/
/*!**************************************************************
 * fp11_jorgen_von_strangle_setup()
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
void fp11_jorgen_von_strangle_setup(void)
{

    fp_test_load_dynamic_config(SPID_OBERON_3_HW_TYPE, fp11_jorgen_von_strangle_load_specl_config);

}
/**************************************
 * end fp11_jorgen_von_strangle_setup()
 **************************************/

static fbe_status_t fp11_jorgen_von_strangle_load_specl_config(void)
{
    /* The default good data should be good enough for this test
     * Nothing to do here, just return.
     */

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fp11_jorgen_von_strangle_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp11_jorgen_von_strangle test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/
void fp11_jorgen_von_strangle_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end fp11_jorgen_von_strangle_cleanup()
 ******************************************/


/*!**************************************************************
 * fp11_jorgen_von_strangle_shutdown()
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

void fp11_jorgen_von_strangle_shutdown(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}

void fp11_jorgen_von_strangle_set_persist_ports(fbe_bool_t persist_flag)
{
    fbe_test_esp_registry_write(ESP_REG_PATH,
                       FBE_MODULE_MGMT_PORT_PERSIST_KEY,
                       FBE_REGISTRY_VALUE_DWORD,
                       &persist_flag,
                       sizeof(fbe_bool_t));
    return;
}



/*************************
 * end file fp11_jorgen_von_strangle_test.c
 *************************/






