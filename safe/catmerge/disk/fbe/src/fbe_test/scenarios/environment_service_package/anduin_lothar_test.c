/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file anduin_lothar_test
 ***************************************************************************
 *
 * @brief
 *  Verify the special port subrole set for iSCSI and FC FE ports.
 *
 * @version
 *   6/18/2012 - Created. dongz
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
#include "mut.h"
#include "fbe_test_esp_utils.h"
#include "fbe_test_common_utils.h"
#include "fp_test.h"

/*************************
 *   GLOBALS
 *************************/

char * anduin_lothar_short_desc = "Verify the special port subrole set for iSCSI and FC FE ports.";
char * anduin_lothar_long_desc ="\
Anduin Lothar \n\
        -Anduin Lothar is the only descendant of the Arathi descent \n\
\n\
Using Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 1 SAS drives/enclosure\n\
        [PP] 15 logical drives/enclosure\n\
\n\
   1) initializes the terminator\n\
   2) loads the test configuration\n\
            a) inserts a board\n\
            b) inserts a port\n\
            c) inserts an enclosure to port 0\n\
            d) inserts 15 sas drives to port 0 encl 0\n\
   3) insert 5 slots, 2 HYPERNOVA, 2 POSEIDON_II_OPTICAL_2PORT, and 1 GLACIER_REV_C \
\n\
   4) Initializes the physical package\n\
   5) initializes esp\n\
   6) wait for the expected number of objects to be created\n\
   7) persist all ioports\n\
   8) reset esp\
   9) verify the port subrole\n\
   ";

void anduin_lothar_test_setup(void);
fbe_status_t fbe_test_load_anduin_lothar_config(void);
fbe_status_t anduin_lothar_load_iom_config(void);

/*!**************************************************************
 * anduin_lothar_test()
 ****************************************************************
 * @brief
 *  Verify the special port subrole set for iSCSI and FC FE ports
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   6/18/2012 - Created. dongz
 *
 ****************************************************************/

void anduin_lothar_test(void)
{
    fbe_status_t                                status;
    fbe_esp_module_mgmt_set_port_t              set_port_cmd;
    fbe_u32_t slot, port_index;
    fbe_esp_module_io_port_info_t               esp_io_port_info;
    /*
     * Persist all ports.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Persist all ports.\n");
    fbe_zero_memory(&set_port_cmd, sizeof(fbe_esp_module_mgmt_set_port_t));

    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_PERSIST_ALL_PORTS_CONFIG;

    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // give the packages time to settle down before destroying things
    fbe_api_sleep (10000);

    mut_printf(MUT_LOG_LOW, "=== Reset ESP ===\n");

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

    fbe_test_esp_restart();

    mut_printf(MUT_LOG_LOW, "=== %s ESP restart completes ===\n", __FUNCTION__);

    fbe_api_sleep (10000);

    for (slot = 0; slot < 3; slot++)
    {
        for(port_index = 0; port_index < 4; port_index++)
        {
            if(((slot == 1) || (slot == 2)) && (port_index > 1))
            {
                // two port SLICs
                continue;
            }
            fbe_zero_memory(&esp_io_port_info, sizeof(fbe_esp_module_io_port_info_t));

            esp_io_port_info.header.sp = 0;
            esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_port_info.header.slot = slot;
            esp_io_port_info.header.port = port_index;

            status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
            mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n", 
                slot, port_index, esp_io_port_info.io_port_info.present, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            //iom 0 should be back end
            if(slot < 1)
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_role, IOPORT_PORT_ROLE_BE);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_role, IOPORT_PORT_ROLE_FE);
            }
            //for each protocol(iSCSI and FC), the first port on FE iom should have a special subrole
            if(port_index == 0 &&(slot == 1 || slot == 3))
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_subrole, FBE_PORT_SUBROLE_SPECIAL);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_subrole, FBE_PORT_SUBROLE_NORMAL);
            }
        }
    }

}
/******************************************
 * end anduin_lothar_test_setup()
 ******************************************/

/*!**************************************************************
 * anduin_lothar_test_setup()
 ****************************************************************
 * @brief
 *  Verify the special port subrole set for iSCSI and FC FE ports
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   06/18/2012 - Created.  dongz
 *
 ****************************************************************/

void anduin_lothar_test_setup(void)
{
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);
    fp_test_load_dynamic_config(SPID_DEFIANT_M1_HW_TYPE, anduin_lothar_load_iom_config);
}
/******************************************
 * end anduin_lothar_test_setup()
 ******************************************/

/*!**************************************************************
 * anduin_lothar_test_destroy()
 ****************************************************************
 * @brief
 *  Verify the special port subrole set for iSCSI and FC FE ports.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   06/18/2012 - Created. dongz
 *
 ****************************************************************/

void anduin_lothar_test_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end anduin_lothar_test_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_test_load_anduin_lothar_config()
 ****************************************************************
 * @brief
 *  Setup with mustang default settings, and create one port,
 *  one enclosure, and 15 drives.
 *  copied from fbe_test_load_fflewddur_fflam_config
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   06/18/2012 - Created. dongz
 *
 ****************************************************************/
fbe_status_t fbe_test_load_anduin_lothar_config(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_board_info_t board_info;
    fbe_terminator_sas_port_info_t sas_port;
    fbe_terminator_sas_encl_info_t sas_encl;

    fbe_api_terminator_device_handle_t port_handle;
    fbe_api_terminator_device_handle_t encl_handle;
    fbe_api_terminator_device_handle_t drive_handle;

    fbe_u32_t no_of_ports = 0;
    fbe_u32_t no_of_encls = 0;
    fbe_u32_t slot = 0;

    /* Specify only the platform_type. Terminator will assign the appropriate board_type for you. */
    board_info.platform_type = SPID_DEFIANT_M1_HW_TYPE;
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
 * anduin_lothar_load_iom_config()
 ****************************************************************
 * @brief
 *  Setup the specifics of the SLIC configuration via SPECL SFI.
 *  copied from fp9_fflewddur_fflam_load_iom_config
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   06/18/2012 - Created. dongz
 *
 ****************************************************************/
fbe_status_t anduin_lothar_load_iom_config(void)
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
            else if( (slot == 1) || (slot == 2) )
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = ERUPTION_2PORT_REV_C_84823;
            }
            else if(slot == 3)
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

/*************************
 * end file anduin_lothar_test.c
 *************************/
