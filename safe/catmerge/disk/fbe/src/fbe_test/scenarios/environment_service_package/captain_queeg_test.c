/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file captain_queeg_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify Transformers SPS & BoB status in ESP.
 * 
 * @version
 *   04/18/2012 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_base_environment_debug.h"

#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * captain_queeg_short_desc = "Verify Transformers SPS & BoB status processing in ESP";
char * captain_queeg_long_desc ="\
Captain Queeg\n\
        -lead character in the movie THE CAINE MUTINY\n\
        -famous quote 'Aboard my ship, excellent performance is standard, standard performance is sub-standard-standard'\n\
                      'and sub-standard performance is not permitted to exist - that, I warn you.'\n\
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
STEP 2: Validate that the SPS & BoB status\n\
        - Verify SPS Count & Status\n\
        - Verify BoB Count & Status\n\
";

static fbe_object_id_t	                    expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_u64_t                    expected_device_type = FBE_DEVICE_TYPE_INVALID;

#define RM_RETRY_COUNT      5


static void captain_queeg_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===",
        update_object_msg->object_id, expected_object_id);

	if (update_object_msg->object_id == expected_object_id) 
	{
    	mut_printf(MUT_LOG_LOW, "callback, device_mask: actual 0x%llx, expected 0x%llx ===",
            update_object_msg->notification_info.notification_data.data_change_info.device_mask, expected_device_type);
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == expected_device_type)
	    {
    		fbe_semaphore_release(sem, 0, 1 ,FALSE);
        } 
	}
}

void captain_queeg_testSpsBobCounts(HW_CPU_MODULE cpuModuleType)
{
    fbe_backup_type_t   backupType;
    fbe_u32_t                       expectedPeSpsCount, expectedDae0SpsCount, totalSpsCount = 0;
    fbe_esp_sps_mgmt_spsCounts_t    spsCountInfo;
    fbe_u32_t           bobCount = 0, expectedBobCount;
    fbe_status_t        status;
    fbe_device_physical_location_t  location={0};
    fbe_esp_encl_mgmt_encl_info_t   enclInfo;

    switch (cpuModuleType)
    {
    case MEGATRON_CPU_MODULE:
    case TRITON_ERB_CPU_MODULE:
        backupType = FBE_BACKUP_TYPE_SPS;
        expectedPeSpsCount = 2;
        expectedDae0SpsCount = 2;
        expectedBobCount = 0;
        break;

    case JETFIRE_CPU_MODULE:
        backupType = FBE_BACKUP_TYPE_BOB;
        expectedPeSpsCount = 0;
        expectedDae0SpsCount = 0;
        expectedBobCount = 2;
        break;

    default:
    	mut_printf(MUT_LOG_LOW, "%s, unsupported cpuModuleType %d\n", 
                   __FUNCTION__, cpuModuleType);
        return;
        break;
    }

    // request SPS counts from sps_mgmt
    fbe_set_memory(&spsCountInfo, 0, sizeof(fbe_esp_sps_mgmt_spsCounts_t));
    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
    mut_printf(MUT_LOG_LOW, "%s, totalNumberOfSps %d, status %d\n", 
               __FUNCTION__, spsCountInfo.totalNumberOfSps, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE((expectedPeSpsCount + expectedDae0SpsCount) == spsCountInfo.totalNumberOfSps);

    //request SPS counts from encl_mgmt
    fbe_set_memory(&enclInfo, 0, sizeof(fbe_esp_encl_mgmt_encl_info_t));
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s, bus 0x%x, encl 0x%x, spsCount %d, status %d\n", 
               __FUNCTION__, location.bus, location.enclosure, enclInfo.spsCount, status);
    totalSpsCount += enclInfo.spsCount;
    if ((cpuModuleType == MEGATRON_CPU_MODULE) ||
        (cpuModuleType == TRITON_ERB_CPU_MODULE))
    {
        fbe_set_memory(&enclInfo, 0, sizeof(fbe_esp_encl_mgmt_encl_info_t));
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
        mut_printf(MUT_LOG_LOW, "%s, bus 0x%x, encl 0x%x, spsCount %d, status %d\n", 
                   __FUNCTION__, location.bus, location.enclosure, enclInfo.spsCount, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        totalSpsCount += enclInfo.spsCount;
    }
    MUT_ASSERT_TRUE((expectedPeSpsCount + expectedDae0SpsCount) == totalSpsCount);

    // request BoB counts
    status = fbe_api_esp_sps_mgmt_getBobCount(&bobCount);
    mut_printf(MUT_LOG_LOW, "%s, bobCount %d, status %d\n", 
               __FUNCTION__, bobCount, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(expectedBobCount == bobCount);

}   // end of captain_queeg_testSpsBobCounts


void captain_queeg_testSpsBobStatus(HW_CPU_MODULE cpuModuleType)
{
    fbe_esp_sps_mgmt_spsStatus_t    spsStatus;
    fbe_u32_t                       totalSpsCount = 0;
    fbe_esp_sps_mgmt_bobStatus_t    bobStatus;
    fbe_u32_t                       enclIndex, spsIndex;
    fbe_u32_t                       bobIndex;
    fbe_esp_sps_mgmt_spsCounts_t    spsCountInfo;
    fbe_u32_t                       bobCount = 0;
    fbe_status_t                    status;

    // get the SPS count & request status if SPS's present
    // request SPS counts
    fbe_set_memory(&spsCountInfo, 0, sizeof(fbe_esp_sps_mgmt_spsCounts_t));
    status = fbe_api_esp_sps_mgmt_getSpsCount(&spsCountInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (spsCountInfo.totalNumberOfSps > 0)
    {
        for (enclIndex = 0; enclIndex < spsCountInfo.enclosuresWithSps; enclIndex++)
        {
            for (spsIndex = 0; spsIndex < spsCountInfo.spsPerEnclosure; spsIndex++)
            {
                fbe_zero_memory(&spsStatus, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
                if (((cpuModuleType == MEGATRON_CPU_MODULE) ||
                     (cpuModuleType == TRITON_ERB_CPU_MODULE)) && (enclIndex == 0))
                {
                    spsStatus.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
                    spsStatus.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                }
                else
                {
                    spsStatus.spsLocation.bus = 0;
                    spsStatus.spsLocation.enclosure = 0;
                }
                spsStatus.spsLocation.slot = spsIndex;
                status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatus);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                mut_printf(MUT_LOG_LOW, "  %s, enclIndex %d, spsIndex %d, inserted %d, status %d\n", 
                           __FUNCTION__, enclIndex, spsIndex, spsStatus.spsModuleInserted, spsStatus.spsStatus);
            }
        }
    }
    totalSpsCount += spsCountInfo.spsPerEnclosure;


    // get the BoB count & request status if BoB's present
    status = fbe_api_esp_sps_mgmt_getBobCount(&bobCount);
    mut_printf(MUT_LOG_LOW, "%s, bobCount %d, status %d\n", 
               __FUNCTION__, bobCount, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    if (bobCount > 0)
    {
        mut_printf(MUT_LOG_LOW, "%s, Request BoB Status, bobCount %d,\n", 
                   __FUNCTION__, bobCount);
        for (bobIndex = 0; bobIndex < bobCount; bobIndex++)
        {
            bobStatus.bobIndex = bobIndex;
            status = fbe_api_esp_sps_mgmt_getBobStatus(&bobStatus);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            mut_printf(MUT_LOG_LOW, "  %s, bobIndex %d, inserted %d, status %d\n", 
                       __FUNCTION__, bobIndex, bobStatus.bobInfo.batteryInserted, bobStatus.bobInfo.batteryState);
        }
    }

}   // end of captain_queeg_testSpsBobStatus

void captain_queeg_testLiionSpsEvents(void)
{
    fbe_status_t                            status;
//    DWORD                                   dwWaitResult;
    fbe_device_physical_location_t          spsLocation;
    fbe_bool_t                              isMsgPresent = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatus;

    /*
     * Remove SPS Module & verify events
     */
    // remove module
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.inserted = FALSE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    fbe_api_sleep(5000);

    fbe_zero_memory(&spsStatus, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatus.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    spsStatus.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    spsStatus.spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "  %s, slot %d, inserted %d, status %d\n", 
               __FUNCTION__, 
               spsStatus.spsLocation.slot, 
               spsStatus.spsModuleInserted, 
               spsStatus.spsStatus);

    // check for events
    fbe_set_memory(&spsLocation, 0, sizeof(fbe_device_physical_location_t));
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &spsLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_SPS_REMOVED,
                                         &deviceStr[0]);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "SPS Removed Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s fault Removed Event log msg found\n", &deviceStr[0]);

    // re-insert module
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.inserted = TRUE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    /*
     * Remove SPS Battery (in SPS Status)
     */
    // remove battery
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.batteryPackModuleNotEngagedFault = TRUE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    fbe_api_sleep(5000);

    fbe_zero_memory(&spsStatus, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatus.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    spsStatus.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    spsStatus.spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "  %s, slot %d, inserted %d, status %d\n", 
               __FUNCTION__, 
               spsStatus.spsLocation.slot, 
               spsStatus.spsModuleInserted, 
               spsStatus.spsStatus);

    // check for events
    fbe_set_memory(&spsLocation, 0, sizeof(fbe_device_physical_location_t));
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &spsLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    isMsgPresent = FALSE;
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_ESP, 
                                         &isMsgPresent, 
                                         ESP_ERROR_SPS_REMOVED,
                                         &deviceStr[0]);
//    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    MUT_ASSERT_INT_EQUAL_MSG(TRUE, isMsgPresent, "SPS Removed Event log msg is not present!");
    mut_printf(MUT_LOG_LOW, "%s fault Removed Event log msg found\n", &deviceStr[0]);


    // insert battery
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.batteryPackModuleNotEngagedFault = FALSE;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of captain_queeg_testLiionSpsEvents

/*!**************************************************************
 * captain_queeg_megatron_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Transformer platforms that
 *  use SPS 's.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/
void captain_queeg_megatron_test(void)
{
    fbe_status_t                            status;

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    /*
     * Verify the SPS & BoB counts
     */
    mut_printf(MUT_LOG_LOW, "QUEEG: Verify SPS & BoB Counts\n");
    captain_queeg_testSpsBobCounts(MEGATRON_CPU_MODULE);

    /*
     * Verify the SPS & BoB status
     */
    mut_printf(MUT_LOG_LOW, "QUEEG: Verify SPS & BoB Status\n");
    captain_queeg_testSpsBobStatus(MEGATRON_CPU_MODULE);

    /*
     * Verify SPS events for Module & Battery
     */
//    mut_printf(MUT_LOG_LOW, "QUEEG: Verify Li-ion SPS events\n");
//    captain_queeg_testLiionSpsEvents();

    return;

}
/******************************************
 * end captain_queeg_megatron_test()
 ******************************************/

/*!**************************************************************
 * captain_queeg_jetfire_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test for Transformer platforms that
 *  use SPS 's.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/
void captain_queeg_jetfire_test(void)
{
    fbe_status_t                            status;

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    /*
     * Verify the SPS & BoB counts
     */
    mut_printf(MUT_LOG_LOW, "QUEEG: Verify SPS & BoB Counts\n");
    captain_queeg_testSpsBobCounts(JETFIRE_CPU_MODULE);

    /*
     * Verify the SPS & BoB status
     */
    mut_printf(MUT_LOG_LOW, "QUEEG: Verify SPS & BoB Status\n");
    captain_queeg_testSpsBobStatus(JETFIRE_CPU_MODULE);

    return;

}
/******************************************
 * end captain_queeg_jetfire_test()
 ******************************************/


/*!**************************************************************
 * captain_queeg_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/
void captain_queeg_setup(SPID_HW_TYPE platform_type)
{
    fbe_status_t    status;
    fbe_bool_t      simulateSps;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    fbe_test_startEspAndSep(FBE_SIM_SP_A, platform_type);

    // simulate SPS to stop testing
    status = fbe_api_esp_sps_mgmt_getSimulateSps(&simulateSps);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}
/**************************************
 * end captain_queeg_setup()
 **************************************/

/*!**************************************************************
 * captain_queeg_lightning_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/
void captain_queeg_lightning_setup(void)
{
    captain_queeg_setup(SPID_DEFIANT_M1_HW_TYPE);
}
/**************************************
 * end captain_queeg_lightning_setup()
 **************************************/

/*!**************************************************************
 * captain_queeg_megatron_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/
void captain_queeg_megatron_setup(void)
{
    captain_queeg_setup(SPID_PROMETHEUS_M1_HW_TYPE);
}
/**************************************
 * end captain_queeg_megatron_setup()
 **************************************/

/*!**************************************************************
 * captain_queeg_jetfire_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/
void captain_queeg_jetfire_setup(void)
{
    captain_queeg_setup(SPID_DEFIANT_M1_HW_TYPE);
}
/**************************************
 * end captain_queeg_jetfire_setup()
 **************************************/

/*!**************************************************************
 * captain_queeg_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the captain_queeg test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   04/18/2012 - Created. Joe Perry
 *
 ****************************************************************/

void captain_queeg_destroy(void)
{
    fbe_api_sleep(20000);
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end captain_queeg_destroy()
 ******************************************/
/*************************
 * end file captain_queeg_test.c
 *************************/

