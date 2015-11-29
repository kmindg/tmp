/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fp15_mumra_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the SLIC limits handling in the module_mgmt object
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
fbe_status_t fp15_mumra_set_persist_ports(fbe_bool_t persist_flag);
static void fp15_mumra_shutdown(void);
static fbe_status_t fp15_mumra_specl_config(void);
static void fp15_mumra_test_init_ps_env(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type);
static void fp_15_mumra_test_load_dynamic_config(SPID_HW_TYPE platform_type, fbe_status_t (test_config_function(void)), HW_MODULE_TYPE ps_type);



char * fp15_mumra_short_desc = "Verify SLIC limits in ESP";
char * fp15_mumra_long_desc ="\
Mumra\n\
         - is the villian in the 80s cartoons series Thundercats. \n\
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
 * fp15_mumra_test() 
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

void fp15_mumra_test(void)
{
    fbe_status_t                            status;
    fbe_esp_module_io_port_info_t           esp_io_port_info = {0};
    fbe_u32_t                               port;
    fbe_u32_t                               slot; 
    fbe_esp_module_mgmt_module_status_t     module_status_info = {0};

    mut_printf(MUT_LOG_LOW, "*** Mumra Test case 1, Persist Ports on SLICs Within Limits Only ***\n");
    for(slot=0;slot<10;slot++)
    {
        if(slot > 3)
        {
            // empty slots
            continue;
        }
        if(slot < 3)
        {
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
        else
        {
            module_status_info.header.sp = 0;
            module_status_info.header.type = FBE_DEVICE_TYPE_IOMODULE;
            module_status_info.header.slot = slot;
            status = fbe_api_esp_module_mgmt_get_module_status(&module_status_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            MUT_ASSERT_INT_EQUAL(module_status_info.state, MODULE_STATE_FAULTED);
            MUT_ASSERT_INT_EQUAL(module_status_info.substate, MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS);

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
        
                MUT_ASSERT_TRUE(esp_io_port_info.io_port_logical_info.logical_number == INVALID_PORT_U8);
            }
        }
    }

    fbe_test_esp_check_mgmt_config_completion_event();

    fp15_mumra_destroy();

       /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    fbe_test_reg_set_persist_ports_true();

    fp_15_mumra_test_load_dynamic_config(SPID_DEFIANT_M5_HW_TYPE, fp15_mumra_specl_config, AC_ACBEL_BLASTOFF);

    mut_printf(MUT_LOG_LOW, "*** Mumra Test case 2, Persist Ports on Jetfire M5 SLIC W/O Octane Limits are not checked ***\n");
    for(slot=0;slot<10;slot++)
    {
        if(slot > 3)
        {
            // empty slots
            continue;
        }
        if(slot <= 3)
        {
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
    }
    fbe_test_esp_check_mgmt_config_completion_event();

    
    return;
}
/******************************************
 * end fp15_mumra_test()
 ******************************************/

void fp15_mumra_test_init_ps_env(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type)
{
    PSPECL_SFI_MASK_DATA                 pSfiMaskData = NULL;
    SP_ID                                sp;
    fbe_u32_t                            ps;
    fbe_status_t                         status = FBE_STATUS_OK;

    pSfiMaskData = malloc(sizeof(SPECL_SFI_MASK_DATA));
    MUT_ASSERT_TRUE(pSfiMaskData != NULL);

    fbe_zero_memory(pSfiMaskData, sizeof(SPECL_SFI_MASK_DATA));

    pSfiMaskData->structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    pSfiMaskData->maskStatus = SPECL_SFI_SET_CACHE_DATA;
    pSfiMaskData->sfiSummaryUnions.psStatus.bladeCount = SP_ID_MAX;
 
    for(sp =0; sp< SP_ID_MAX; sp++)
    {
        for(ps = 0; ps < TOTAL_PS_PER_BLADE; ps ++)
        {
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psCount = 1;
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].transactionStatus = EMCPAL_STATUS_SUCCESS;
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].localPSID = (ps == 0) ? PS0 : PS1;
            
            
            /* If we need to run this test on SPB, the following line needs to be updated. */
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].localSupply = (sp == SP_A) ? TRUE : FALSE;
            
            
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].inserted = TRUE;
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].uniqueID = ps_type;
            pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].uniqueIDFinal = TRUE;

        }
    }
  
    status = fbe_api_terminator_send_specl_sfi_mask_data(pSfiMaskData);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    free(pSfiMaskData);

    return;
}

/*!**************************************************************
 * fp15_mumra_setup()
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
void fp15_mumra_setup(void)
{
    fbe_status_t status;

    /* Create or re-create the registry file */
    status = fbe_test_esp_create_registry_file(FBE_TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Setting Port Persist flag in the Registry.\n");
    fbe_test_reg_set_persist_ports_true();

    fp_15_mumra_test_load_dynamic_config(SPID_DEFIANT_M5_HW_TYPE, fp15_mumra_specl_config, AC_ACBEL_OCTANE);
}
/**************************************
 * end fp15_mumra_setup()
 **************************************/

void fp_15_mumra_test_load_dynamic_config(SPID_HW_TYPE platform_type, fbe_status_t (test_config_function(void)), HW_MODULE_TYPE ps_type)
{
    fbe_status_t status;

    status = fbe_test_load_fp_config(platform_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loaded required config ===\n");

    status = test_config_function();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fp15_mumra_test_init_ps_env(platform_type, ps_type);

    mut_printf(MUT_LOG_LOW, "=== loaded specl config ===\n");

    fp_test_start_physical_package();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");


    fbe_test_wait_for_all_esp_objects_ready();
    return;

}

fbe_status_t fp15_mumra_specl_config(void)
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
            if(slot < 4)
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
 * fp15_mumra_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the fp15_mumra test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   09/20/2010 - Created. bphilbin
 *
 ****************************************************************/

void fp15_mumra_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all();
    return;
}
/******************************************
 * end fp15_mumra_cleanup()
 ******************************************/


/*!**************************************************************
 * fp15_mumra_shutdown()
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

void fp15_mumra_shutdown(void)
{
    fbe_test_esp_common_destroy_all();
    return;
}

fbe_status_t fp15_mumra_set_persist_ports(fbe_bool_t persist_flag)
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
 * end file fp15_mumra_test.c
 *************************/


