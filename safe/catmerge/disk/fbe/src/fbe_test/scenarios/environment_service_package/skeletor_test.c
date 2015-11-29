/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file skeletor_test.c
 ***************************************************************************
 *
 * @brief
 *  This file verifies intial PS mode for the processor enclosure power supplies. 
 *
 * @version
 *   29-Oct-2013 bphilbin - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"


static void skeletor_test_load_test_env(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type);
static void skeletor_test_load_test_env_single_ps(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type);

char * skeletor_short_desc = "Verify the Octane PS Support for the processor enclosure.";
char * skeletor_long_desc =
    "\n"
    "\n"
    "The skeletor scenario verifies the Octane PS support handling for jetfire SPs.\n"
    "It includes: \n"
    "    - Verify the expected PS type;\n"
    "\n"
    "Dependencies:\n"
    "    - The SPECL SFI is able to inject the error before ESP starts.\n"
    "\n"
    "Starting Config(simple Config):\n"
    "    [PP] LIGHTNING_HW \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 1 viper enclosures\n"
    "    [PP] 2 SAS drives (PDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Initialize the terminator.\n"
    "    - Load the simple configuration.\n"
    "    - Use the SPECL SFI to set the PS mode to OKTOSHUTDOWN.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "\n"  
    "STEP 2: Verify PS mode for the power supplies in the processor enclosure.\n"
    "    - Verify the local PS mode is NOTOKTOSHUTDOWN in the processor enclosure.\n"
    "    - Verify the peer PS mode is OKTOSHUTDOWN in the processor enclosure.\n"
    "\n"
    "STEP 9: Shutdown the Terminator/Physical package/ESP.\n"
    ;


void skeletor_test_verify_expected_ps_type(void)
{
    fbe_esp_ps_mgmt_ps_info_t                 getPsInfo = {0};
    fbe_esp_board_mgmt_board_info_t           boardInfo = {0};
    fbe_u32_t                                 psIndex = 0;
    fbe_u32_t                                 psCount = 0; 
    fbe_device_physical_location_t            location = {0};
    fbe_status_t                              status;
    fbe_ps_mgmt_expected_ps_type_t            ps_type = {0};

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
            boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        mut_printf(MUT_LOG_LOW, "=== Verify XPE power supply ===");
    }
    else
    {
        location.bus = 0;
        location.enclosure = 0;
        mut_printf(MUT_LOG_LOW, "=== Verify DPE power supply ===");
    }

    mut_printf(MUT_LOG_LOW, "=== Bring a New System Up with Octane PS for the First Time ===");
  
    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    for(psIndex = 0; psIndex < psCount; psIndex ++)
    {
        fbe_zero_memory(&getPsInfo, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        getPsInfo.location.bus = location.bus;
        getPsInfo.location.enclosure = location.enclosure;
        getPsInfo.location.slot = psIndex;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW, "=== Verify Expected PS Type for ps %d ===", psIndex);

        MUT_ASSERT_INT_EQUAL(getPsInfo.expected_ps_type, FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE);
        mut_printf(MUT_LOG_LOW, "=== Verify The Octane PS is Supported in Slot:%d ===", psIndex);
        MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.psSupported, FBE_TRUE);
    }

    mut_printf(MUT_LOG_LOW, "=== change the expected ps type and verify we fault the octane power supplies ===");

    ps_type = FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER;
    status = fbe_api_esp_ps_mgmt_set_expected_ps_type(ps_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_api_sleep(5000);

    fbe_test_esp_unload();
    fbe_test_esp_reload_with_existing_terminator_config();

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
            boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        mut_printf(MUT_LOG_LOW, "=== Verify XPE power supply mode ===");
    }
    else
    {
        location.bus = 0;
        location.enclosure = 0;
        mut_printf(MUT_LOG_LOW, "=== Verify DPE power supply mode ===");
    }
  
    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    for(psIndex = 0; psIndex < psCount; psIndex ++)
    {
        fbe_zero_memory(&getPsInfo, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        getPsInfo.location.bus = location.bus;
        getPsInfo.location.enclosure = location.enclosure;
        getPsInfo.location.slot = psIndex;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW, "=== Verify Expected PS Type for ps %d ===", psIndex);

        MUT_ASSERT_INT_EQUAL(getPsInfo.expected_ps_type, FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER);
        mut_printf(MUT_LOG_LOW, "=== Verify The Octane PS is Now Unsupported in Slot:%d ===", psIndex);
        MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.psSupported, FBE_FALSE);
    }

    // Test a different platform to confirm the expected PS type is not set

    skeletor_destroy();

    skeletor_test_load_test_env(SPID_DEFIANT_M1_HW_TYPE, AC_ACBEL_BLASTOFF);

    mut_printf(MUT_LOG_LOW, "=== Verify The Blastoff PS Support is not affected for other Jetfires ===");

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
            boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        mut_printf(MUT_LOG_LOW, "=== Verify XPE power supply mode ===");
    }
    else
    {
        location.bus = 0;
        location.enclosure = 0;
        mut_printf(MUT_LOG_LOW, "=== Verify DPE power supply mode ===");
    }
  
    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    for(psIndex = 0; psIndex < psCount; psIndex ++)
    {
        fbe_zero_memory(&getPsInfo, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        getPsInfo.location.bus = location.bus;
        getPsInfo.location.enclosure = location.enclosure;
        getPsInfo.location.slot = psIndex;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW, "=== Verify Expected PS Type for ps %d ===", psIndex);

        MUT_ASSERT_INT_EQUAL(getPsInfo.expected_ps_type, FBE_PS_MGMT_EXPECTED_PS_TYPE_NONE);
        mut_printf(MUT_LOG_LOW, "=== Verify The Blastoff PS is Supported in Slot:%d ===", psIndex);
        MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.psSupported, FBE_TRUE);
    }


    skeletor_destroy();

    skeletor_test_load_test_env_single_ps(SPID_DEFIANT_M4_HW_TYPE, AC_ACBEL_OCTANE);

    mut_printf(MUT_LOG_LOW, "=== Verify We Can Set Expected Type When PS is Only Present On the Peer ===");

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE ||
            boardInfo.platform_info.enclosureType == DPE_ENCLOSURE_TYPE);

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        mut_printf(MUT_LOG_LOW, "=== Verify XPE power supply mode ===");
    }
    else
    {
        location.bus = 0;
        location.enclosure = 0;
        mut_printf(MUT_LOG_LOW, "=== Verify DPE power supply mode ===");
    }
  
    status = fbe_api_esp_ps_mgmt_getPsCount(&location, &psCount);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    for(psIndex = 0; psIndex < psCount; psIndex ++)
    {
        fbe_zero_memory(&getPsInfo, sizeof(fbe_esp_ps_mgmt_ps_info_t));
        getPsInfo.location.bus = location.bus;
        getPsInfo.location.enclosure = location.enclosure;
        getPsInfo.location.slot = psIndex;

        status = fbe_api_esp_ps_mgmt_getPsInfo(&getPsInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        mut_printf(MUT_LOG_LOW, "=== Verify Expected PS Type for ps %d ===", psIndex);

        MUT_ASSERT_INT_EQUAL(getPsInfo.expected_ps_type, FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE);
        mut_printf(MUT_LOG_LOW, "=== Verify The Blastoff PS is Supported in Slot:%d ===", psIndex);
        if(getPsInfo.psInfo.bInserted == FBE_TRUE)
        {
            MUT_ASSERT_INT_EQUAL(getPsInfo.psInfo.psSupported, FBE_TRUE);
        }
    }


    return;
}

/*!**************************************************************
 * skeletor_test_init_test_env() 
 ****************************************************************
 * @brief
 *  Sets all the processor enclosure power supply mode to OKTOSHUTDOWN.
 *  This needs to be done before the physical package starts.
 *
 * @param platform_type.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  24-Mar-2011 PHE - Created. 
 *
 ****************************************************************/
void skeletor_test_init_test_env(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type)
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
 * skeletor_test_init_test_env() 
 ****************************************************************
 * @brief
 *  Sets all the processor enclosure power supply mode to OKTOSHUTDOWN.
 *  This needs to be done before the physical package starts.
 *
 * @param platform_type.               
 *
 * @return None.
 * 
 * @note 
 * 
 * @author
 *  24-Mar-2011 PHE - Created. 
 *
 ****************************************************************/
void skeletor_test_init_test_env_single_ps(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type)
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
            
            if(sp == 0)
            {
                pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].inserted = TRUE;
                pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].uniqueID = ps_type;
                pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].uniqueIDFinal = TRUE;
            }
            else
            {
                pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].inserted = FALSE;
                pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].uniqueID = 0;
                pSfiMaskData->sfiSummaryUnions.psStatus.psSummary[sp].psStatus[ps].uniqueIDFinal = TRUE;
            }

        }
    }
  
    status = fbe_api_terminator_send_specl_sfi_mask_data(pSfiMaskData);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    free(pSfiMaskData);

    return;
}

void skeletor_test_load_test_env(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    status = fbe_test_load_fp_config(platform_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    skeletor_test_init_test_env(platform_type, ps_type);

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");

    fbe_test_wait_for_all_esp_objects_ready();

    if(platform_type == SPID_DEFIANT_M4_HW_TYPE ||
       platform_type == SPID_DEFIANT_M5_HW_TYPE)
    {
        /* For for mgmt module configure the mgmt port speed */
        fbe_test_esp_check_mgmt_config_completion_event();
    }
}

void skeletor_test_load_test_env_single_ps(SPID_HW_TYPE platform_type, HW_MODULE_TYPE ps_type)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    status = fbe_test_load_fp_config(platform_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    skeletor_test_init_test_env_single_ps(platform_type, ps_type);

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    /* Load sep and neit packages */
    sep_config_load_esp_sep_and_neit();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");
    fbe_test_wait_for_all_esp_objects_ready();

    if(platform_type == SPID_DEFIANT_M4_HW_TYPE ||
       platform_type == SPID_DEFIANT_M5_HW_TYPE)
    {
        /* For for mgmt module configure the mgmt port speed */
        fbe_test_esp_check_mgmt_config_completion_event();
    }
}

void skeletor_jetfire_m4_test(void)
{
    mut_printf(MUT_LOG_LOW, "=== Testing Jetfire M4 platform  ===\n");
    skeletor_test_verify_expected_ps_type();

    return;
}

void skeletor_jetfire_m4_setup(void)
{
    skeletor_test_load_test_env(SPID_DEFIANT_M4_HW_TYPE, AC_ACBEL_OCTANE);
    return;
}


void skeletor_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file skeletor_test.c
 *************************/



