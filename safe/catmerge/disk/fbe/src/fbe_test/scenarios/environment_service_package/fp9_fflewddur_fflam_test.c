/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp9_fflewddur_fflam_test.c
 ***************************************************************************
 *
 * @brief
 *  Setport command test.
 * 
 * @version
 *   09/09/2011 - Created. bphilbin
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
#include "pp_utils.h"
#include "fbe_private_space_layout.h"
#include "fp_test.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


static fbe_status_t fp9_fflewddur_fflam_load_iom_config(void);
fbe_status_t fbe_test_load_fflewddur_fflam_config(void);

char * fp9_fflewddur_fflam_short_desc = "Setport command test";
char * fp9_fflewddur_fflam_long_desc ="\
Fflewddur Fflam \n\
         - is a fictional character in the Disney filk The Black Cauldron.\n\
         - Fflewddur is a wandering bard who posesses a magic harp. \n\
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
STEP 2: Issue setport commands to modify the port configuration \n\
";

/*!**************************************************************
 * fp9_fflewddur_fflam_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/09/2011 - Created. bphilbin
 *
 ****************************************************************/
void fp9_fflewddur_fflam_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_mgmt_set_port_t              set_port_cmd;
    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   port_index, slot, log_num;
    fbe_esp_module_io_module_info_t             io_module_info;

    /*
     * Write configuration for SAS BE ports 1-7
     */

    mut_printf(MUT_LOG_TEST_STATUS, "Setting SAS BE Ports 1-7\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST;
    set_port_cmd.num_ports = 7;
    for(port_index = 0; port_index < set_port_cmd.num_ports; port_index++)
    {
        set_port_cmd.port_config[port_index].iom_group = FBE_IOM_GROUP_K;
        set_port_cmd.port_config[port_index].port_role = IOPORT_PORT_ROLE_BE;
        set_port_cmd.port_config[port_index].port_subrole = FBE_PORT_SUBROLE_NORMAL;
        set_port_cmd.port_config[port_index].logical_number = port_index+1;
        set_port_cmd.port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        set_port_cmd.port_config[port_index].port_location.sp = 0;
        if(port_index < 3) // ports on slic in slot 0
        {
            set_port_cmd.port_config[port_index].port_location.slot = 0;
            set_port_cmd.port_config[port_index].port_location.port = port_index+1;
        }
        else // ports on slic in slot 1
        {
            set_port_cmd.port_config[port_index].port_location.slot = 1;
            set_port_cmd.port_config[port_index].port_location.port = port_index - 3;
        }
    }

    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    for (slot = 0; slot < 2; slot++)
    {
        for(port_index = 0; port_index < 4; port_index++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port_index;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_role, IOPORT_PORT_ROLE_BE);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_subrole, FBE_PORT_SUBROLE_NORMAL);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.iom_group, FBE_IOM_GROUP_K);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, ((slot * 4) + port_index));
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "iSCSI FE Ports 0-3\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST;
    set_port_cmd.num_ports = 4;
    for(port_index = 0; port_index < set_port_cmd.num_ports; port_index++)
    {
        set_port_cmd.port_config[port_index].iom_group = FBE_IOM_GROUP_E;
        set_port_cmd.port_config[port_index].port_role = IOPORT_PORT_ROLE_FE;
        set_port_cmd.port_config[port_index].port_subrole = FBE_PORT_SUBROLE_NORMAL;
        set_port_cmd.port_config[port_index].logical_number = port_index;
        set_port_cmd.port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        set_port_cmd.port_config[port_index].port_location.sp = 0;
        if(port_index < 2) // ports on slic in slot 2
        {
            set_port_cmd.port_config[port_index].port_location.slot = 2;
            set_port_cmd.port_config[port_index].port_location.port = port_index;
        }
        else // ports on slic in slot 3
        {
            set_port_cmd.port_config[port_index].port_location.slot = 3;
            set_port_cmd.port_config[port_index].port_location.port = port_index - 2;
        }
    }

    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    log_num = 0;

    for (slot = 2; slot < 4; slot++)
    {
        for(port_index = 0; port_index < 2; port_index++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port_index;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);
    
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_role, IOPORT_PORT_ROLE_FE);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_subrole, FBE_PORT_SUBROLE_NORMAL);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.iom_group, FBE_IOM_GROUP_E);
            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, log_num);
            log_num++;
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "FC FE Ports 4-7\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST;
    set_port_cmd.num_ports = 4;
    for(port_index = 0; port_index < set_port_cmd.num_ports; port_index++)
    {
        set_port_cmd.port_config[port_index].iom_group = FBE_IOM_GROUP_D;
        set_port_cmd.port_config[port_index].port_role = IOPORT_PORT_ROLE_FE;
        set_port_cmd.port_config[port_index].port_subrole = FBE_PORT_SUBROLE_NORMAL;
        set_port_cmd.port_config[port_index].logical_number = port_index + 4;
        set_port_cmd.port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        set_port_cmd.port_config[port_index].port_location.sp = 0;

        set_port_cmd.port_config[port_index].port_location.slot = 4;
        set_port_cmd.port_config[port_index].port_location.port = port_index;
    }



    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for(port_index = 0; port_index < 4; port_index++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 4;
        esp_io_port_info.header.port = port_index;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_role, IOPORT_PORT_ROLE_FE);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_subrole, FBE_PORT_SUBROLE_NORMAL);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.iom_group, FBE_IOM_GROUP_D);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, log_num);
        log_num++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Negative Test Case FC FE Ports Overwrite\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST;
    set_port_cmd.num_ports = 4;
    for(port_index = 0; port_index < set_port_cmd.num_ports; port_index++)
    {
        set_port_cmd.port_config[port_index].iom_group = FBE_IOM_GROUP_D;
        set_port_cmd.port_config[port_index].port_role = IOPORT_PORT_ROLE_FE;
        set_port_cmd.port_config[port_index].port_subrole = FBE_PORT_SUBROLE_NORMAL;
        set_port_cmd.port_config[port_index].logical_number = port_index + 8;
        set_port_cmd.port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        set_port_cmd.port_config[port_index].port_location.sp = 0;

        set_port_cmd.port_config[port_index].port_location.slot = 4;
        set_port_cmd.port_config[port_index].port_location.port = port_index;
        
    }
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status != FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Success - Invalid setport cmd resulted in status 0x%x\n", status);

    mut_printf(MUT_LOG_TEST_STATUS, "FC FE Ports with overwrite enabled \n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST_WITH_OVERWRITE;
    set_port_cmd.num_ports = 4;
    for(port_index = 0; port_index < set_port_cmd.num_ports; port_index++)
    {
        set_port_cmd.port_config[port_index].iom_group = FBE_IOM_GROUP_D;
        set_port_cmd.port_config[port_index].port_role = IOPORT_PORT_ROLE_FE;
        set_port_cmd.port_config[port_index].port_subrole = FBE_PORT_SUBROLE_NORMAL;
        set_port_cmd.port_config[port_index].logical_number = port_index + 8;
        set_port_cmd.port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        set_port_cmd.port_config[port_index].port_location.sp = 0;

        set_port_cmd.port_config[port_index].port_location.slot = 4;
        set_port_cmd.port_config[port_index].port_location.port = port_index;

    }
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Remove 4 FC FE Ports\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST;
    set_port_cmd.num_ports = 4;
    for(port_index = 0; port_index < set_port_cmd.num_ports; port_index++)
    {
        set_port_cmd.port_config[port_index].port_location.type = FBE_DEVICE_TYPE_IOMODULE;
        set_port_cmd.port_config[port_index].port_location.sp = 0;
        set_port_cmd.port_config[port_index].port_location.slot = 4;
        set_port_cmd.port_config[port_index].port_location.port = port_index;

    }
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for(port_index = 0; port_index < 4; port_index++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        esp_io_port_info.header.slot = 4;
        esp_io_port_info.header.port = port_index;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_role, IOPORT_PORT_ROLE_UNINITIALIZED);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_subrole, FBE_PORT_SUBROLE_UNINTIALIZED);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.iom_group, FBE_IOM_GROUP_UNKNOWN);
        MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, INVALID_PORT_U8);
    }


    mut_printf(MUT_LOG_TEST_STATUS, "Destroying port configuration.\n");

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_REMOVE_ALL_PORTS_CONFIG;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_create_registry_file(FBE_TRUE);

    // on hardware the reboot happens automatically, for test purposes we have to restart manually here.
    fbe_test_esp_restart();

    // BE 0 is auto configured so skip that port when checking uninitialized ports
    for (slot = 1; slot < 5; slot++)
    {
        io_module_info.header.sp = 0;
        io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
        io_module_info.header.slot = slot;
        status = fbe_api_esp_module_mgmt_getIOModuleInfo(&io_module_info);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        for(port_index = 0; port_index < io_module_info.io_module_phy_info.ioPortCount; port_index++)
        {
            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port_index;
        
            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

            MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.logical_number, INVALID_PORT_U8);
        }
    }

    // give the packages time to settle down before destroying things
    fbe_api_sleep (10000);

}


/*!**************************************************************
 * fp9_fflewddur_fflam_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/12/2011 - Created. bphilbin
 *
 ****************************************************************/
void fp9_fflewddur_fflam_setup(void)
{

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp9_fflewddur_fflam_load_iom_config);

    

}
/**************************************
 * end fp9_fflewddur_fflam_setup()
 **************************************/


/*!**************************************************************
 * fp9_fflewddur_fflam_load_iom_config()
 ****************************************************************
 * @brief
 *  Setup the specifics of the SLIC configuration via SPECL SFI.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/12/2011 - Created. bphilbin
 *
 ****************************************************************/
static fbe_status_t fp9_fflewddur_fflam_load_iom_config(void)
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
            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[0].ioControllerInfo[0].ioPortInfo[0].boot_device = TRUE;
            if((slot == 0) || (slot == 1))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = MOONLITE;
            }
            else if((slot == 2) || (slot == 3))
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_REV_D;
            }
            else if(slot == 4)
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

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_test_load_fflewddur_fflam_config()
 ****************************************************************
 * @brief
 *  Setup with mustang default settings, and create one port,
 *  one enclosure, and 15 drives.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   09/12/2011 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fbe_test_load_fflewddur_fflam_config(void)
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
    board_info.platform_type = SPID_PROMETHEUS_M1_HW_TYPE;
    status  = fbe_api_terminator_insert_board (&board_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_private_space_layout_initialize_library(board_info.platform_type);
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


/*!**************************************************************
 * fp9_fflewddur_fflam_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp9_fflewddur_fflam test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/09/2011 - Created. bphilbin
 *
 ****************************************************************/
void fp9_fflewddur_fflam_destroy(void)
{
    fbe_test_esp_delete_registry_file();;
    fbe_test_esp_common_destroy_all();
    return;
}
