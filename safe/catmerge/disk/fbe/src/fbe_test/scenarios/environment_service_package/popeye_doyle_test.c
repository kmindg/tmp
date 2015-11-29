/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file popeye_doyle_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify SPS Test Cabling of SPS Management Object of ESP.
 * 
 * @version
 *   07/26/2010 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_sps_info.h"
#include "fbe_test_esp_utils.h"

#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * popeye_doyle_short_desc = "Verify SPS Testing Cabling of ESP's SPS Management Object";
char * popeye_doyle_long_desc ="\
Popeye Doyle\n\
        -lead character in the movie THE FRENCH CONNECTION\n\
        -famous quote 'Ever pick your feet in Poughkeepsie?'\n\
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
STEP 2: Validate that SPS Cabling cases are properly identified\n\
        - Verify Correct SPS Cabling\n\
        - Verify Invalid Power Cabling\n\
        - Verify Invalid Serial Cabling\n\
";


fbe_test_params_t voyager_test_table =
{
     0,
     "VOYAGER",
     FBE_BOARD_TYPE_FLEET,
     FBE_PORT_TYPE_SAS_PMC,
     0, // 3,
     0, // 5,
     0,
     FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
     0,
     FBE_SAS_DRIVE_CHEETA_15K,
     INVALID_DRIVE_NUMBER,
     MAX_DRIVE_COUNT_VOYAGER,
     MAX_PORTS_IS_NOT_USED,
     MAX_ENCLS_IS_NOT_USED,
     DRIVE_MASK_IS_NOT_USED,
};


static fbe_notification_registration_id_t   reg_id;
static fbe_esp_sps_mgmt_spsStatus_t         expectedSpsStatusInfo = {0};


static void popeye_doyle_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
    fbe_status_t status = FBE_STATUS_OK;

    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_SPS)
    {
        fbe_zero_memory(&spsStatusInfo, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        spsStatusInfo.spsLocation = expectedSpsStatusInfo.spsLocation;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
 
        if (status != FBE_STATUS_OK)
        {
            mut_printf(MUT_LOG_LOW, "Error in getting spsStatusInfo, status 0x%X.\n", status);
            return;
        }
            
        if(((spsStatusInfo.spsModuleInserted == expectedSpsStatusInfo.spsModuleInserted) && 
            (expectedSpsStatusInfo.spsModuleInserted == FALSE)) ||
            ((spsStatusInfo.spsModuleInserted == expectedSpsStatusInfo.spsModuleInserted) &&
             (spsStatusInfo.spsStatus == expectedSpsStatusInfo.spsStatus) &&
             (spsStatusInfo.spsCablingStatus == expectedSpsStatusInfo.spsCablingStatus)))
        {
            mut_printf(MUT_LOG_LOW, "    %s, semaphore released (SPS event from ESP detected)\n",
                   __FUNCTION__);
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
            return;
        }
    }

    return;
}


void popeye_doyle_removeInsertSps(fbe_bool_t xpeArray)
{
    fbe_status_t                            status;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
        DWORD                                                                   dwWaitResult;
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
        fbe_semaphore_t                                                 sem;
    fbe_u32_t                               retryIndex;

    fbe_semaphore_init(&sem, 0, 1);
  
    // Initialize to get event notification
        status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                                                                                  popeye_doyle_commmand_response_callback_function,
                                                                                                                                  &sem,
                                                                                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
  
    /*
     * Verify that SPS is present
     */
    fbe_zero_memory(&spsStatusInfo, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    if (xpeArray)
    {
        spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    spsStatusInfo.spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo));
    mut_printf(MUT_LOG_LOW, "ESP LOCAL SpsStatus 0x%x, inserted %d, status 0x%x\n", 
        spsStatusInfo.spsStatus, spsStatusInfo.spsModuleInserted, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(spsStatusInfo.spsModuleInserted);
    mut_printf(MUT_LOG_LOW, "ESP LOCAL SPS Present\n");

    /*
     * Remove SPS & verify
     */
    expectedSpsStatusInfo.spsModuleInserted = FALSE;
    expectedSpsStatusInfo.spsLocation = spsStatusInfo.spsLocation;
    
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.inserted = FALSE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    /*
     * Wait for the expected notification.  
     */
    for (retryIndex = 0; retryIndex < 3; retryIndex++)
    {
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
        mut_printf(MUT_LOG_LOW, "ESP LOCAL SPS Removed successfully\n");

        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if (xpeArray)
        {
            spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo));
        mut_printf(MUT_LOG_LOW, "ESP LOCAL SpsStatus 0x%x, inserted %d, status 0x%x\n", 
            spsStatusInfo.spsStatus, spsStatusInfo.spsModuleInserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (!spsStatusInfo.spsModuleInserted)
        {
            break;
        }
    }
    MUT_ASSERT_FALSE(spsStatusInfo.spsModuleInserted);
    mut_printf(MUT_LOG_LOW, "ESP LOCAL SPS Removed\n");

    /*
     * Reinsert SPS & verify
     */
    expectedSpsStatusInfo.spsModuleInserted = TRUE;
    expectedSpsStatusInfo.spsStatus = SPS_STATE_TESTING;
    expectedSpsStatusInfo.spsCablingStatus = FBE_SPS_CABLING_UNKNOWN;

    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.inserted = TRUE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    /*
     * Wait for the expected notification.  
     */
    for (retryIndex = 0; retryIndex < 3; retryIndex++)
    {
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
//        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
        mut_printf(MUT_LOG_LOW, "ESP LOCAL SPS Inserted successfully\n");

        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if (xpeArray)
        {
            spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo));
        mut_printf(MUT_LOG_LOW, "ESP LOCAL SpsStatus 0x%x, inserted %d, status 0x%x\n", 
            spsStatusInfo.spsStatus, spsStatusInfo.spsModuleInserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo.spsModuleInserted)
        {
            break;
        }
    }
    MUT_ASSERT_TRUE(spsStatusInfo.spsModuleInserted);
    mut_printf(MUT_LOG_LOW, "ESP LOCAL SPS Inserted\n");

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(popeye_doyle_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}   // end of popeye_doyle_removeInsertSps


void popeye_doyle_processSpsTestCabling(fbe_bool_t xpeArray,
                                        fbe_bool_t voyagerDae0,
                                        fbe_bool_t dae0UniqueSps,
                                        fbe_sps_cabling_status_t spsCablingStatus)
{
    DWORD                               dwWaitResult;
    SPECL_SFI_MASK_DATA                 sfi_mask_data;
    fbe_status_t                        status;
    fbe_api_terminator_device_handle_t  encl_device_id;  
    ses_stat_elem_ps_struct             ps_struct;
    fbe_bool_t                          forcePeAcFail = FALSE;
    fbe_bool_t                          forceDae0AcFail = FALSE;
    fbe_bool_t                          invalidSerial = FALSE;
    fbe_bool_t                          localSps = TRUE;
    fbe_u32_t                           psIndex;
    fbe_u32_t                           daePsIndex0, daePsIndex1;
    fbe_u32_t                           spIndex1, spIndex2;
    fbe_semaphore_t                     sem;
    fbe_esp_ps_mgmt_ps_info_t           psInfo;
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           retryIndex;
    fbe_esp_sps_mgmt_spsStatus_t        spsStatusInfo;

    location.bus = 0;
    location.enclosure = 0;

    fbe_semaphore_init(&sem, 0, 1);
  
    // Initialize to get event notification
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  popeye_doyle_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
  
    expectedSpsStatusInfo.spsLocation.slot = 0;
    expectedSpsStatusInfo.spsModuleInserted = TRUE;
    expectedSpsStatusInfo.spsStatus = SPS_STATE_AVAILABLE;
    expectedSpsStatusInfo.spsCablingStatus = spsCablingStatus;
    if (xpeArray)
    {
        expectedSpsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        expectedSpsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        expectedSpsStatusInfo.spsLocation.bus = 0;
        expectedSpsStatusInfo.spsLocation.enclosure = 0;
    }
    expectedSpsStatusInfo.spsLocation.slot = 0;

    /* 
     * Forced status (for specific test results) 
     */
    switch (spsCablingStatus)
    {
    case FBE_SPS_CABLING_VALID:
        forcePeAcFail = TRUE;
        if (xpeArray)
        {
            forceDae0AcFail = TRUE;
        }
        break;

    case FBE_SPS_CABLING_INVALID_PE:
        if (xpeArray && !dae0UniqueSps)
        {
            forceDae0AcFail = TRUE;
        }
        else
        {
            mut_printf(MUT_LOG_LOW, "%s, no AC_FAILS forced or cleared\n", __FUNCTION__);
        }
        break;

    case FBE_SPS_CABLING_INVALID_SERIAL:
        // force opposite AC_FAIL's for this failure
        invalidSerial = TRUE;
        localSps = FALSE;
        forcePeAcFail = TRUE;
        if (xpeArray)
        {
            forceDae0AcFail = TRUE;
        }
        break;

    case FBE_SPS_CABLING_INVALID_DAE0:
        if (xpeArray && !dae0UniqueSps)
        {
            forcePeAcFail = TRUE;
        }
        else
        {
            mut_printf(MUT_LOG_LOW, "spsCablingStatus 0x%x not supported\n", spsCablingStatus);
            return;
        }
        break;

    default:
        mut_printf(MUT_LOG_LOW, "spsCablingStatus 0x%x not supported\n", spsCablingStatus);
        MUT_ASSERT_TRUE(spsCablingStatus == FBE_SPS_CABLING_VALID);
        break;
    }

    /*
     * Force AC_FAIL's based on flags set above
     */
    if (localSps)
    {
        spIndex1 = SP_A;
        spIndex2 = SP_B;
        if (invalidSerial && xpeArray)
        {
            psIndex = PS_1;
        }
        else
        {
            psIndex = PS_0;
        }
    }
    else
    {
        spIndex1 = SP_B;
        spIndex2 = SP_A;
        if (invalidSerial && xpeArray)
        {
            psIndex = PS_1;
        }
        else
        {
            psIndex = PS_0;
        }
    }
    // force AC_FAIL on PE (if applicable)
    if (forcePeAcFail)
    {
        mut_printf(MUT_LOG_LOW, "%s, force PE PS %d, AC_FAIL, SP %d\n", 
                   __FUNCTION__, psIndex, spIndex1);
        sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
        sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
        sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
        fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    
        sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spIndex1].psStatus[psIndex].ACFail = TRUE;
//        sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[psIndex].psStatus->ACFail = TRUE;
        if (xpeArray)
        {
            mut_printf(MUT_LOG_LOW, "%s, force PE PS %d, AC_FAIL, SP %d\n", 
                       __FUNCTION__, psIndex, spIndex2);
            sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spIndex2].psStatus[psIndex].ACFail = TRUE;
        }
        fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    }

    // force AC_FAIL on DAE0 (if applicable)
    if (forceDae0AcFail)
    {
        // for Voyager, must adjust psIndex (PSA 0 & 1, PSB 2 & 3)
        daePsIndex0 = psIndex;
        if (voyagerDae0)
        {
            if (psIndex == 1)
            {
                daePsIndex0 = 2;
            }
            daePsIndex1 = daePsIndex0 + 1;
        }
        mut_printf(MUT_LOG_LOW, "%s, force DAE0 PS %d, AC_FAIL, SP %d\n", 
                   __FUNCTION__, daePsIndex0, spIndex1);
        status = fbe_api_terminator_get_enclosure_handle(0, 0, &encl_device_id);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        status = fbe_api_terminator_get_ps_eses_status(encl_device_id, daePsIndex0, &ps_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        ps_struct.ac_fail = TRUE;
        status = fbe_api_terminator_set_ps_eses_status(encl_device_id, daePsIndex0, ps_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (voyagerDae0)
        {
            mut_printf(MUT_LOG_LOW, "%s, Voyager force DAE0 PS %d, AC_FAIL, SP %d\n", 
                       __FUNCTION__, daePsIndex1, spIndex1);
            status = fbe_api_terminator_get_enclosure_handle(0, 0, &encl_device_id);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            status = fbe_api_terminator_get_ps_eses_status(encl_device_id, daePsIndex1, &ps_struct);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            ps_struct.ac_fail = TRUE;
            status = fbe_api_terminator_set_ps_eses_status(encl_device_id, daePsIndex1, ps_struct);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    /*
     * Wait for the expected notification.  
     */
    mut_printf(MUT_LOG_LOW, "%s, Verify AC_FAIL set\n", __FUNCTION__);
    for (retryIndex = 0; retryIndex < 3; retryIndex++)
    {
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 100000);

        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if (xpeArray)
        {
            spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo));
        mut_printf(MUT_LOG_LOW, "%s, LOCAL SpsStatus 0x%x, inserted %d, status 0x%x\n", 
            __FUNCTION__, spsStatusInfo.spsStatus, spsStatusInfo.spsModuleInserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo.spsStatus == SPS_STATE_AVAILABLE)
        {
            break;
        }
    }
    MUT_ASSERT_TRUE(spsStatusInfo.spsStatus == SPS_STATE_AVAILABLE);
    mut_printf(MUT_LOG_LOW, "%s, spsCablingStatus Current 0x%x, Expected 0x%x\n", 
        __FUNCTION__, spsStatusInfo.spsCablingStatus, spsCablingStatus);
    // we cannot determine Invalid Serial, so it will be marked as Invalid Multiple
    if (spsCablingStatus == FBE_SPS_CABLING_INVALID_SERIAL)
    {
        MUT_ASSERT_TRUE(spsStatusInfo.spsCablingStatus == FBE_SPS_CABLING_INVALID_MULTI);
    }
    else
    {
        MUT_ASSERT_TRUE(spsStatusInfo.spsCablingStatus == spsCablingStatus);
    }
    
    /*
     * Clear any forced status (for specific test results)
     */
    // clear AC_FAIL on PE (if applicable)
    if (forcePeAcFail)
    {
        mut_printf(MUT_LOG_LOW, "%s, clear PE PS %d, AC_FAIL, SP %d\n", 
                   __FUNCTION__, psIndex, spIndex1);
        sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
        sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
        sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
        fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    
        sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spIndex1].psStatus[psIndex].ACFail = FALSE;
        if (xpeArray)
        {
            mut_printf(MUT_LOG_LOW, "%s, clear PE PS %d, AC_FAIL, SP %d\n", 
                       __FUNCTION__, psIndex, spIndex2);
            sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spIndex2].psStatus[psIndex].ACFail = FALSE;
        }
        fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    }

    // clear AC_FAIL on DAE0 (if applicable)
    if (forceDae0AcFail)
    {
        // for Voyager, must adjust psIndex (PSA 0 & 1, PSB 2 & 3)
/*
        daePsIndex0 = psIndex;
        if (voyagerDae0)
        {
            if (psIndex == 1)
            {
                daePsIndex0 = 2;
            }
            daePsIndex1 = daePsIndex0 + 1;
        }
*/
        mut_printf(MUT_LOG_LOW, "%s, clear DAE0 PS %d, AC_FAIL, SP %d\n", 
                   __FUNCTION__, daePsIndex0, spIndex1);
        status = fbe_api_terminator_get_ps_eses_status(encl_device_id, daePsIndex0, &ps_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        ps_struct.ac_fail = FALSE;
        status = fbe_api_terminator_set_ps_eses_status(encl_device_id, daePsIndex0, ps_struct);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (voyagerDae0)
        {
            mut_printf(MUT_LOG_LOW, "%s, clear Voyager DAE0 PS %d, AC_FAIL, SP %d\n", 
                       __FUNCTION__, daePsIndex1, spIndex1);
            status = fbe_api_terminator_get_ps_eses_status(encl_device_id, daePsIndex1, &ps_struct);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            ps_struct.ac_fail = FALSE;
            status = fbe_api_terminator_set_ps_eses_status(encl_device_id, daePsIndex1, ps_struct);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    /*
     * Wait for the expected notification.  
     */
    mut_printf(MUT_LOG_LOW, "%s, Verify AC_FAIL cleared\n", __FUNCTION__);
    for (retryIndex = 0; retryIndex < 3; retryIndex++)
    {
        fbe_api_sleep(20000);

        fbe_set_memory(&psInfo, 0, sizeof(fbe_esp_ps_mgmt_ps_count_t));

        psInfo.location.slot = psIndex;
        status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        mut_printf(MUT_LOG_LOW, "%s, ps %d, psInserted %d, acFail %d\n", 
                   __FUNCTION__,
                   psIndex,
                   psInfo.psInfo.bInserted,
                   psInfo.psInfo.ACFail);
        MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
        if (!psInfo.psInfo.ACFail)
        {
            break;
        }
    }
    MUT_ASSERT_FALSE(psInfo.psInfo.ACFail);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(popeye_doyle_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;

}   // end of popeye_doyle_processSpsTestCabling

void popeye_doyle_scheduleSpsTest(fbe_bool_t xpeArray,
                                  fbe_bool_t dae0UniqueSps)
{
    fbe_esp_sps_mgmt_spsTestTime_t  spsTestTimeInfo;
    fbe_esp_sps_mgmt_spsStatus_t    spsStatusInfo;
    fbe_status_t                    status;
    fbe_system_time_t               currentTime;
    fbe_u32_t                       retryIndex;
    DWORD                           dwWaitResult;
    fbe_semaphore_t                 sem;

    fbe_semaphore_init(&sem, 0, 1);
  
    // Initialize to get event notification
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  popeye_doyle_commmand_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_get_system_time(&currentTime);

    spsTestTimeInfo.spsTestTime = currentTime;
    spsTestTimeInfo.spsTestTime.minute +=2;
    if (spsTestTimeInfo.spsTestTime.minute >= 60)
    {
        spsTestTimeInfo.spsTestTime.hour++;
        spsTestTimeInfo.spsTestTime.minute = 2;
    }
    if (spsTestTimeInfo.spsTestTime.hour >= 24)
    {
        spsTestTimeInfo.spsTestTime.day++;
        spsTestTimeInfo.spsTestTime.hour = 0;
    }
    if (spsTestTimeInfo.spsTestTime.day >= 7)
    {
        spsTestTimeInfo.spsTestTime.day = 0;
    }
    status = fbe_api_esp_sps_mgmt_setSpsTestTime(&spsTestTimeInfo);
    if (status == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_LOW, "%s, SPS Test time set, day %d, hour %d, min %d\n", 
                   __FUNCTION__,
                   spsTestTimeInfo.spsTestTime.weekday,
                   spsTestTimeInfo.spsTestTime.hour,
                   spsTestTimeInfo.spsTestTime.minute);
    }

    // wait 2 minutes for SPS Test to start
    mut_printf(MUT_LOG_LOW, "Wait for Scheduled SPS Test to run\n");
    expectedSpsStatusInfo.spsModuleInserted = TRUE;
    expectedSpsStatusInfo.spsStatus = SPS_STATE_TESTING;
    expectedSpsStatusInfo.spsCablingStatus = FBE_SPS_CABLING_VALID; //FBE_SPS_CABLING_UNKNOWN;
    if (xpeArray)
    {
        expectedSpsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        expectedSpsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        expectedSpsStatusInfo.spsLocation.bus = 0;
        expectedSpsStatusInfo.spsLocation.enclosure = 0;
    }
    expectedSpsStatusInfo.spsLocation.slot = 0;

    for (retryIndex = 0; retryIndex < 6; retryIndex++)
    {
//        dwWaitResult = fbe_semaphore_wait_ms(&sem, 100000);
        dwWaitResult = fbe_semaphore_wait_ms(&sem, 60000);

        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if (xpeArray)
        {
            spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
            spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        spsStatusInfo.spsLocation.slot = 0;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo));
        mut_printf(MUT_LOG_LOW, "ESP LOCAL SpsStatus 0x%x, inserted %d, status 0x%x\n", 
            spsStatusInfo.spsStatus, spsStatusInfo.spsModuleInserted, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo.spsStatus == SPS_STATE_TESTING)
        {
            break;
        }
    }
    MUT_ASSERT_TRUE(spsStatusInfo.spsStatus == SPS_STATE_TESTING);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(popeye_doyle_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    popeye_doyle_processSpsTestCabling(xpeArray, FALSE, dae0UniqueSps, FBE_SPS_CABLING_VALID);

}   // end of popeye_doyle_scheduleSpsTest

/*!**************************************************************
 * popeye_doyle_megatron_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void popeye_doyle_megatron_test(void)
{
    fbe_status_t                            status;

    /*
     * Wait for initial SPS Test to complete
     */
    mut_printf(MUT_LOG_LOW, "POPEYE: wait for Initial SPS Testing to complete\n");
    
    // turn off SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        
//    status = fbe_test_esp_wait_for_sps_test_to_complete(FALSE, FBE_LOCAL_SPS, 150000);
//    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*
     * Verify that ESP sps_mgmt detects SPSA removal & re-tests (Valid Cabling)
     */
    mut_printf(MUT_LOG_LOW, "******* Test Valid Cabling *******\n");
    popeye_doyle_removeInsertSps(TRUE);
    popeye_doyle_processSpsTestCabling(TRUE, FALSE, TRUE, FBE_SPS_CABLING_VALID);
    fbe_api_sleep (10000);

    /*
     * Verify that ESP sps_mgmt detects SPSA removal & re-tests (Invalid Power Cabling)
     */
    mut_printf(MUT_LOG_LOW, "******* Test Invalid PE Cabling *******\n");
    popeye_doyle_removeInsertSps(TRUE);
    popeye_doyle_processSpsTestCabling(TRUE, FALSE, TRUE, FBE_SPS_CABLING_INVALID_PE);
    fbe_api_sleep (10000);

    mut_printf(MUT_LOG_LOW, "******* Test Invalid Serial Cabling *******\n");
    popeye_doyle_removeInsertSps(TRUE);
    popeye_doyle_processSpsTestCabling(TRUE, FALSE, TRUE, FBE_SPS_CABLING_INVALID_SERIAL);
    fbe_api_sleep (10000);

    mut_printf(MUT_LOG_LOW, "******* Test Scheduled SPS Test *******\n");
    popeye_doyle_scheduleSpsTest(TRUE, TRUE);
    
    // turn on SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    return;
}
/******************************************
 * end popeye_doyle_megatron_test()
 ******************************************/

/*!**************************************************************
 * popeye_doyle_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/
static void popeye_doyle_test_load_test_env(SPID_HW_TYPE platform_type, fbe_bool_t voyagerDae0)
{
    fbe_status_t status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== loading configure ===\n");

    /* Starting Physical Package */
    if (voyagerDae0)
    {
        status = fbe_test_load_maui_config(platform_type, &voyager_test_table);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }
    else
    {
        status = fbe_test_esp_load_simple_config(platform_type, FBE_SAS_ENCLOSURE_TYPE_VIPER);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    status = fbe_test_startPhyPkg(TRUE, SIMPLE_CONFIG_MAX_OBJECTS);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit_no_esp();
    mut_printf(MUT_LOG_LOW, "=== sep and neit are started ===\n");

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "=== ESP has started ===\n");

    fbe_test_wait_for_all_esp_objects_ready();

    fbe_api_sleep(15000);

    return;

}   // end of popeye_doyle_test_load_test_env

/*!**************************************************************
 * popeye_doyle_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/
void popeye_doyle_megatron_setup(void)
{
    popeye_doyle_test_load_test_env(SPID_PROMETHEUS_M1_HW_TYPE, FALSE);
    return;
}


/*!**************************************************************
 * popeye_doyle_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the popeye_doyle test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void popeye_doyle_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end popeye_doyle_destroy()
 ******************************************/
/*************************
 * end file popeye_doyle_test.c
 *************************/




