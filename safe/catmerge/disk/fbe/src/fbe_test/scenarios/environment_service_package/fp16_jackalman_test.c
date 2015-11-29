/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp16_jackalman_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the SLIC limits handling in the module_mgmt object
 * 
 * @version
 *   12/18/2014 - Created. bphilbin
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
fbe_status_t fp16_jackalman_set_persist_ports(fbe_bool_t persist_flag);
static void fp16_jackalman_shutdown(void);
static fbe_status_t fp16_jackalman_specl_config(void);
fbe_status_t fp13_orko_load_terminator_config(SPID_HW_TYPE platform_type);
void fp13_orko_test_start_physical_package(void);



char * fp16_jackalman_short_desc = "Verify Multi-Protocol Port Support in ESP";
char * fp16_jackalman_long_desc ="\
Jackalman\n\
         - is a villian in the 80s cartoons series Thundercats. \n\
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
STEP 2: Check the SLIC limits \n\
";


/*!**************************************************************
 * fp16_jackalman_test() 
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

void fp16_jackalman_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port;
    fbe_u32_t                               slot; 
 
    mut_printf(MUT_LOG_LOW, "*** Jackalman Test case 1, Persist Ports on SLICs Within Limits Only ***\n");

    slot = 0;
    for (port=0;port<6;port++)
    {
        esp_io_port_info.header.sp = 0;
        esp_io_port_info.header.type = FBE_DEVICE_TYPE_MEZZANINE;
        esp_io_port_info.header.slot = slot;
        esp_io_port_info.header.port = port;
    
        status = fbe_api_esp_module_mgmt_getIOModulePortInfo(&esp_io_port_info);
        mut_printf(MUT_LOG_LOW, "BuiltIn PORT:%d, present:0x%x, role:%d logical number:%d, protocol:%d\n", 
                   port, esp_io_port_info.io_port_info.present, 
                   esp_io_port_info.io_port_logical_info.port_role,
                   esp_io_port_info.io_port_logical_info.logical_number, 
                   esp_io_port_info.io_port_info.protocol);

        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(esp_io_port_info.io_port_info.present == FBE_MGMT_STATUS_TRUE);

        MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number != INVALID_PORT_U8);
    }

    fbe_test_esp_check_mgmt_config_completion_event();
   
    return;
}
/******************************************
 * end fp16_jackalman_test()
 ******************************************/

/*!**************************************************************
 * fp16_jackalman_setup()
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
void fp16_jackalman_setup(void)
{
    fbe_status_t status;
    fbe_test_load_fp_config(SPID_OBERON_2_HW_TYPE);

    fp_test_start_physical_package();

	/* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    fbe_test_reg_set_persist_ports_true();

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();
    return;
}
/**************************************
 * end fp16_jackalman_setup()
 **************************************/

fbe_status_t fp16_jackalman_specl_config(void)
{
    fbe_status_t                    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       slot = 0, controller_num = 0, port = 0, sp = 0;
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

    // now set up the SFP info
    sfi_mask_data->structNumber = SPECL_SFI_IO_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    
    for(sp=0;sp<2;sp++)
    {
        for(slot = 0; slot < sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioModuleCount; slot++)
        {
            if (sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].moduleType == IO_MODULE_TYPE_ONBOARD)
            {
                for(controller_num = 0; controller_num < sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerCount; controller_num++)
                {
                    if(sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
                    {
                        for(port = 0; port < sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortCount; port++)
                        {
                            // the multi protocol ports default to FC
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].portPciData[0].protocol = IO_CONTROLLER_PROTOCOL_FIBRE;
                            #if 0
                            // emulate an FC SFP     
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_CONNECTOR_INDEX] = SFP_CONN_TYPE_LC;
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_10G_ETH_COMPLIANCE_CODE_INDEX] = 0;
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_FC_SPEEDS] = (SFP_FC_SPEED_200MBps | SFP_FC_SPEED_400MBps | SFP_FC_SPEED_800MBps | SFP_FC_SPEED_1600MBps);
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_BIT_RATE_INDEX] = SFP_BIT_RATE_16G;
                            #endif
                            // emulate an iSCSI SFP     
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_CONNECTOR_INDEX] |= SFP_CONN_TYPE_LC;
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_10G_ETH_COMPLIANCE_CODE_INDEX] |= (SFP_TEN_GIG_BASE_SR | SFP_TEN_GIG_BASE_LR | SFP_TEN_GIG_BASE_LRM);
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_FC_SPEEDS] |= 0;
                            sfi_mask_data->sfiSummaryUnions.ioModStatus.ioSummary[sp].ioStatus[slot].ioControllerInfo[controller_num].ioPortInfo[port].sfpStatus.type1.serialID.sfpEEPROM[SFP_BIT_RATE_INDEX] |= SFP_BIT_RATE_10G;

                        }
                    }
                }
            }
        }
    }
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_zero_memory(sfi_mask_data, sizeof(SPECL_SFI_MASK_DATA));
    sfi_mask_data->structNumber = SPECL_SFI_MISC_STRUCT_NUMBER;
    sfi_mask_data->maskStatus = SPECL_SFI_GET_CACHE_DATA;
    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    sfi_mask_data->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data->sfiSummaryUnions.miscStatus.type = 2;
    sfi_mask_data->sfiSummaryUnions.miscStatus.type2.localCnaMode = CNA_MODE_ETHERNET;

    status = fbe_api_terminator_send_specl_sfi_mask_data(sfi_mask_data);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    return status;

}


/*!**************************************************************
 * fp16_jackalman_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp16_jackalman test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp16_jackalman_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp16_jackalman_cleanup()
 ******************************************/


/*!**************************************************************
 * fp16_jackalman_shutdown()
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

void fp16_jackalman_shutdown(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}

fbe_status_t fp16_jackalman_set_persist_ports(fbe_bool_t persist_flag)
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
 * end file fp16_jackalman_test.c
 *************************/



