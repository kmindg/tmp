/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp7_venger_test2.c
 ***************************************************************************
 *
 * @brief
 *  Verify slic upgrade support in ESP.
 *
 * @version
 *   05/26/2010 - Created. bphilbin
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

static fbe_status_t fp7_venger_load_test17_config(void);
static fbe_status_t fp7_venger_load_test18_config(void);
void fp7_venger_check_module_port_state(fbe_u32_t sp_id);

char * fp7_venger_test3_short_desc = "SLIC Upgrade test part 3";
char * fp7_venger_test3_long_desc ="\
Venger\n\
         - is a fictional character in the 80s cartoon series Dungeons & Dragons.\n\
         - Venger was the villian of the series. \n\
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
 * fp7_venger_test2()
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
void fp7_venger_test3(void)
{
    fbe_status_t                                status;

    fbe_esp_module_mgmt_set_port_t              set_port_cmd;

    /*
     * Test case 3 - 2 SPs, 1 SAS and 1 FC Slic
     * 1 load system with 1SAS and 1 8G FC slic, persist ports
     * 2 issue slic upgrade on spa, load SPA with 1 SAS and 1 16G FC Slic, check all ports status ok on both SPs
     * 5 Reboot SPB with 1 SAS and 1 16G FC Slic
     * 6 check all ports status ok on both SPs
     */

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test3 Case 1 Persist GLACIER Ports ***\n");

    //Check status OK on SPA
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fp7_venger_check_module_port_state(0);

    //Check status OK on SPB
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fp7_venger_check_module_port_state(1);

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test3 Case 1 GLACIER Ports Persisted Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test3 Case 2 Upgrade Glacier to Vortex on SPA ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    fp7_venger_load_test18_config();

    fbe_test_esp_reload_with_existing_terminator_config();

    fp7_venger_check_module_port_state(0);

    fbe_test_esp_check_mgmt_config_completion_event();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fp7_venger_check_module_port_state(1);

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test3 Case 2 Upgrade Glacier to Vortex on SPA Completed Successfully ***\n");

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test3 Case 3 Upgrade Glacier to Vortex on SPB ***\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    /* Issue upgrade request */
    set_port_cmd.opcode = FBE_ESP_MODULE_MGMT_UPGRADE_PORTS;
    status = fbe_api_esp_module_mgmt_set_port_config(&set_port_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_esp_check_mgmt_config_completion_event();

    /* Reboot the SP and verify the ports are still persisted. */
    fbe_test_esp_unload();

    fp7_venger_load_test18_config();

    fbe_test_esp_reload_with_existing_terminator_config();

    fp7_venger_check_module_port_state(1);

    fbe_test_esp_check_mgmt_config_completion_event();

    //Check peer report IO Module status OK
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fp7_venger_check_module_port_state(0);

    mut_printf(MUT_LOG_LOW, "*** fp7_venger Test3 Case 3 Upgrade Glacier to Vortex on SPB Completed Successfully ***\n");
}

void fp7_venger_test3_setup(void)
{
    fbe_status_t status;

    //load config on SPA
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test17_config);

    //load config on SPB
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fp_test_set_persist_ports(TRUE);

    fp_test_load_dynamic_config(SPID_PROMETHEUS_M1_HW_TYPE, fp7_venger_load_test17_config);
}




/*!**************************************************************
 * fp7_venger_load_test10_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with Supercell
 *  SLICs inserted.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test17_config(void)
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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[0].boot_device = TRUE;
            }
            else if(slot == 1)
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

/*!**************************************************************
 * fp7_venger_load_test11_config()
 ****************************************************************
 * @brief
 *  This function creates a Spitfire configuration with 2 El Nino
 *  SLICs inserted.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   09/01/2010 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t fp7_venger_load_test18_config(void)
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
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[0].ioPortInfo[0].boot_device = TRUE;
            }
            else if(slot == 1)
            {
                sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].uniqueID = VORTEX16Q;
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

void fp7_venger_check_module_port_state(fbe_u32_t sp_id)
{
    fbe_status_t                                status;
    fbe_esp_module_io_module_info_t             esp_io_module_info = {0};

    fbe_esp_module_io_port_info_t               esp_io_port_info = {0};
    fbe_u32_t                                   slot, port, sp;
    fbe_u32_t                                   iom_max=0, io_port_max=0, port_index, sp_max;

    /*
     * Test case 3 - 2 SPs, 1 SAS and 1 FC Slic
     * 1 load system with 1SAS and 1 8G FC slic, persist ports
     * 2 issue slic upgrade on spa, load SPA with 1 SAS and 1 16G FC Slic, check all ports status ok on both SPs
     * 5 Reboot SPB with 1 SAS and 1 16G FC Slic
     * 6 check all ports status ok on both SPs
     */

    iom_max = 2;
    io_port_max = 4;
    sp_max = 2;

    for(sp = 0; sp < sp_max; sp++)
    {
        port_index = 0;
        for (slot = 0; slot < iom_max; slot++)
        {
            esp_io_module_info.header.sp = sp;
            esp_io_module_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            esp_io_module_info.header.slot = slot;
            status = fbe_api_esp_module_mgmt_getIOModuleInfo(&esp_io_module_info);
            mut_printf(MUT_LOG_LOW, "Io module inserted 0x%x, status 0x%x\n",
                    esp_io_module_info.io_module_phy_info.inserted, status);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(esp_io_module_info.io_module_phy_info.inserted == FBE_MGMT_STATUS_TRUE);

            if(sp == sp_id)
            {
                // get the port info for that IO module
                for(port = 0; port < io_port_max; port++)
                {
                    esp_io_port_info.header.sp = sp;
                    esp_io_port_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
                    esp_io_port_info.header.slot = slot;
                    esp_io_port_info.header.port = port;

                    status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
                    mut_printf(MUT_LOG_LOW, "IOM: %d, PORT: %d, present 0x%x, status 0x%x\n",
                            slot, port, esp_io_port_info.io_port_info.present, status);
                    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                    MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

                    MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_state, FBE_PORT_STATE_ENABLED);
                    MUT_ASSERT_INT_EQUAL(esp_io_port_info.io_port_logical_info.port_substate, FBE_PORT_SUBSTATE_GOOD);
                    port_index++;
                }
            }
        }
    }
}
