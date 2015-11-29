/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file pandavas_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the event notifications for SP Environment changes (HW Cache status).
 * 
 * @version
 *   03/23/2011 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"
/*
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_sps_info.h"
*/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * pandavas_short_desc = "Verify event notification for SP Environment changes (HW Cache status)";
char * pandavas_long_desc ="\
Pandavas\n\
        ??????\n\
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
STEP 2: Validate event notification for SPS Status changes\n\
        - Verify that \n\
";

static fbe_class_id_t                       expected_class_id = FBE_CLASS_ID_INVALID;
static SPS_STATE                            previousSpsState = SPS_STATE_UNKNOWN;
static SP_ID                                currentSp = SP_A;

#define CACHE_POLL_LOOP_COUNT               5

void pandavas_processSpsStateChange(SP_ID spid, update_object_msg_t * update_object_msg);
void pandavas_startupSp(fbe_sim_transport_connection_target_t targetSp,
                        SPID_HW_TYPE platform_type);

static void pandavas_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*callbackSem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, devMsk 0x%llx, classId 0x%x===",
               update_object_msg->object_id,
               update_object_msg->notification_info.notification_data.data_change_info.device_mask,
               update_object_msg->notification_info.class_id);


    if ((update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_SP_ENVIRON_STATE) &&
        (update_object_msg->notification_info.class_id == expected_class_id))
    {
        mut_printf(MUT_LOG_LOW, "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE detected, release semaphore\n", __FUNCTION__);
        fbe_semaphore_release(callbackSem, 0, 1 ,FALSE);
    }

}

void pandavas_waitForCacheStatusChange(fbe_common_cache_status_t expectedCacheStatus)
{
    fbe_status_t                    status;
    fbe_common_cache_status_info_t  cacheStatusInfo;
    fbe_u32_t                       loopIndex, spIndex;
    fbe_bool_t                      cacheStatusMatch = FALSE;

    for (spIndex = 0; spIndex < SP_ID_MAX; spIndex++)
    {
        if (spIndex == SP_A)
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
        else
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        }
        for (loopIndex = 0; loopIndex < CACHE_POLL_LOOP_COUNT; loopIndex++) 
        {
            status = fbe_api_esp_common_getCacheStatus(&cacheStatusInfo);

            /* Handle busy status. Wait and continue*/
            if (status == FBE_STATUS_BUSY)
            {
                fbe_api_sleep (5000);
                continue;
            }

            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            if ((cacheStatusInfo.cacheStatus == expectedCacheStatus) ||
                ((expectedCacheStatus == FBE_CACHE_STATUS_OK) &&
                 (cacheStatusInfo.cacheStatus == FBE_CACHE_STATUS_DEGRADED)))
            {
                cacheStatusMatch = TRUE;
                break;
            }
            fbe_api_sleep (5000);
        }
        mut_printf(MUT_LOG_LOW, "%s, cacheStatus %d, expCacheStatus %d, cacheStatusMatch %d\n", 
                   __FUNCTION__, cacheStatusInfo.cacheStatus, expectedCacheStatus, cacheStatusMatch);
        MUT_ASSERT_TRUE(cacheStatusMatch);
    }

}   // end of pandavas_waitForCacheStatusChange

/*
 * SPS related functions
 */
void pandavas_initSpsEventNotifation(void)
{
    fbe_status_t                            status;
    fbe_object_id_t                         objectId;

    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);
//    sps_object_id = objectId;

}   // end of pandavas_initSpsEventNotifation

void pandavas_processSpsStateChange(SP_ID spid, update_object_msg_t * update_object_msg)
{
    fbe_status_t                            status;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
    fbe_bool_t                              acFail;
    fbe_bool_t                              spsStateChangeDetected = FALSE;
    fbe_u32_t                               psIndex;

    if (spid == SP_A)
    {
        psIndex = 0;
    }
    else
    {
        psIndex = 1;
    }

    // get SPS status
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
    mut_printf(MUT_LOG_LOW, "  %s, status %d\n", __FUNCTION__, status);
    if (status != FBE_STATUS_OK)
    {
        return;
    }

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, previousSpsState %d, spsStatus %d\n",
               __FUNCTION__, spid, previousSpsState, spsStatusInfo.spsStatus);
    if ((spsStatusInfo.spsStatus == SPS_STATE_TESTING) && (previousSpsState != SPS_STATE_TESTING))
    {
        spsStateChangeDetected = TRUE;
        acFail = TRUE;
    }
    else if ((spsStatusInfo.spsStatus != SPS_STATE_TESTING) && (previousSpsState == SPS_STATE_TESTING))
    {
        spsStateChangeDetected = TRUE;
        acFail = FALSE;
    }

    if (spsStateChangeDetected)
    {
        previousSpsState = spsStatusInfo.spsStatus;
        mut_printf(MUT_LOG_LOW, "  %s, SPS is Testing, force AC_FAIL %d\n",
                   __FUNCTION__, acFail);
        sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
        sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
        sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
        fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

        sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
        sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spid].psStatus[0].ACFail = acFail;
        fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);
    }

}   // end of pandavas_processSpsStateChange

void pandavas_bbuRemoveInsert(SP_ID spid, fbe_u32_t spIndex, fbe_u32_t bbuIndex, fbe_bool_t bbuInsert)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "%s, sp %d, bbuIndex %d, insert %d\n",
               __FUNCTION__, spid, bbuIndex, bbuInsert);

    if (spid == SP_A)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    else
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    }

    sfi_mask_data.structNumber = SPECL_SFI_BATTERY_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.batteryStatus.batterySummary[spIndex].batteryStatus[bbuIndex].inserted = bbuInsert;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}

void pandavas_testBbuNotAvailable(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         enclSem;
    fbe_notification_registration_id_t      enclRegId;
    fbe_u32_t                               spIndex;

    fbe_semaphore_init(&enclSem, 0, 1);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  pandavas_commmand_response_callback_function,
																  &enclSem,
																  &enclRegId);

    // turn off BBU simulation
    currentSp = SP_A;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    currentSp = SP_B;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    // turn off BBU simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    expected_class_id = FBE_CLASS_ID_SPS_MGMT;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    /*
     * Remove BBU to cause state change & check for Status change
     */
    mut_printf(MUT_LOG_LOW, "%s, Remove BBU 0 on both SP's\n", __FUNCTION__);
    for (spIndex = 0; spIndex < SP_ID_MAX; spIndex++)
    {
        mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU 0 removal\n", 
                   __FUNCTION__, spIndex);
        pandavas_bbuRemoveInsert(spIndex, 0, 0, FALSE);
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);
    mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU removal - SUCCESSFUL\n", 
               __FUNCTION__, currentSp);

    /*
     * Remove other SPS to cause state change & check for Status change
     */
    mut_printf(MUT_LOG_LOW, "%s, Remove BBU 1 on both SP's\n", __FUNCTION__);
    for (spIndex = 0; spIndex < SP_ID_MAX; spIndex++)
    {
        mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU 1 removal\n", 
                   __FUNCTION__, spIndex);
        pandavas_bbuRemoveInsert(spIndex, 1, 0, FALSE);
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_FAILED);
    mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU removal - SUCCESSFUL\n", 
               __FUNCTION__, currentSp);

    /*
     * Clear SPS Faults
     */
    mut_printf(MUT_LOG_LOW, "%s, Insert BBU 0 on both SP's\n", __FUNCTION__);
    for (spIndex = 0; spIndex < SP_ID_MAX; spIndex++)
    {
        mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU 0 insert\n", 
                   __FUNCTION__, spIndex);
        pandavas_bbuRemoveInsert(spIndex, 0, 0, TRUE);
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);
    mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU insert - SUCCESSFUL\n", 
               __FUNCTION__, currentSp);

    mut_printf(MUT_LOG_LOW, "%s, Insert BBU 1 on both SP's\n", __FUNCTION__);
    for (spIndex = 0; spIndex < SP_ID_MAX; spIndex++)
    {
        mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU 1 insert\n", 
                   __FUNCTION__, spIndex);
        pandavas_bbuRemoveInsert(spIndex, 1, 0, TRUE);
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);
    mut_printf(MUT_LOG_LOW, "%s, SP %d verify BBU removal - SUCCESSFUL\n", 
               __FUNCTION__, currentSp);

    // bring SPS's back (turn on SPS simulation)
    mut_printf(MUT_LOG_LOW, "%s, Turn BBU Simulation On, both SP's\n", __FUNCTION__);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    currentSp = SP_A;
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_esp_sps_mgmt_setSimulateSps(TRUE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(pandavas_commmand_response_callback_function,
                                                                    enclRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&enclSem);

}   // end of pandavas_testBbuNotAvailable

/*
 * Shutdown related functions
 */

void pandavas_updatedSuitcaseShutdownWarning(SP_ID spid, fbe_bool_t shutdownWarning)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
    fbe_u32_t                               suitcaseIndex;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, force shutdownWarning %d\n",
               __FUNCTION__, spid, shutdownWarning);
    if (spid == SP_A)
    {
        suitcaseIndex = 0;
    }
    else
    {
        suitcaseIndex = 1;
    }
    sfi_mask_data.structNumber = SPECL_SFI_SUITCASE_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.suitcaseStatus.suitcaseSummary[spid].suitcaseStatus[0].shutdownWarning = shutdownWarning;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of pandavas_updatedSuitcaseShutdownWarning

void pandavas_testSuitcaseShutdown(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         enclSem;
    fbe_notification_registration_id_t      enclRegId;

    fbe_semaphore_init(&enclSem, 0, 1);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  pandavas_commmand_response_callback_function,
																  &enclSem,
																  &enclRegId);

    expected_class_id = FBE_CLASS_ID_BOARD_MGMT;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    /*
     * Force Shutdown conditions
     */
    mut_printf(MUT_LOG_LOW, "%s, SP %d detect ShutdownWarning\n", 
               __FUNCTION__, currentSp);
    pandavas_updatedSuitcaseShutdownWarning(currentSp, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&enclSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);
    mut_printf(MUT_LOG_LOW, "%s, SP %d detect ShutdownWarning - SUCCESSFUL\n", 
               __FUNCTION__, currentSp);

    /*
     * Clear Shutdown conditions
     */
    mut_printf(MUT_LOG_LOW, "%s, SP %d clear ShutdownWarning\n", 
               __FUNCTION__, currentSp);
    pandavas_updatedSuitcaseShutdownWarning(currentSp, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Clear Shutdown) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&enclSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "%s, SP %d detect ShutdownWarning - SUCCESSFUL\n", 
               __FUNCTION__, currentSp);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(pandavas_commmand_response_callback_function,
                                                                    enclRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&enclSem);

}   // end of pandavas_testSuitcaseShutdown

/*
 * DAE Shutdown related functions
 */
void pandavas_updateEnclShutdownReason(fbe_device_physical_location_t *location,
                                       fbe_u8_t shutdownReason)
{
    ses_pg_emc_encl_stat_struct             emcEnclStatus;
    fbe_api_terminator_device_handle_t      encl_device_id;
    fbe_status_t                            status;

    mut_printf(MUT_LOG_LOW, "  %s, bus %d, encl %d, force ShutdownReason %d\n",
               __FUNCTION__, location->bus, location->enclosure, shutdownReason);

    status = fbe_api_terminator_get_enclosure_handle(location->bus, 
                                                     location->enclosure, 
                                                     &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_emcEnclStatus(encl_device_id, &emcEnclStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // set ShutdownReason
    emcEnclStatus.shutdown_reason = shutdownReason;
    status = fbe_api_terminator_set_emcEnclStatus(encl_device_id, &emcEnclStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_terminator_get_emcEnclStatus(encl_device_id, &emcEnclStatus);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of pandavas_updateEnclShutdownReason

void pandavas_testDae0ShutdownFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_device_physical_location_t          location;
    fbe_semaphore_t                         enclSem;
    fbe_notification_registration_id_t      enclRegId;
     fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = 60 * 1000; /* 1 Minute wait*/
    fbe_esp_encl_mgmt_encl_info_t           enclInfo;

    fbe_semaphore_init(&enclSem, 0, 1);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  pandavas_commmand_response_callback_function,
																  &enclSem,
																  &enclRegId);

    expected_class_id = FBE_CLASS_ID_ENCL_MGMT;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    location.bus = 0;
    location.enclosure = 0;

    /*
     * Force DAE0 Shutdown & verify CacheStatus
     */
    pandavas_updateEnclShutdownReason(&location, FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&enclSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /* Wait for the debouncing logic to run..*/
    while(current_time < timeout_ms)
    {
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if(enclInfo.shutdownReason == FBE_ESES_SHUTDOWN_REASON_CRITICAL_TEMP_FAULT )
        {
            break;
        }
        current_time = current_time + 200;
        fbe_api_sleep(200);
        
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_FAILED_SHUTDOWN);

    /*
     * Clear DAE0 Shutdown & verify CacheStatus
     */
    pandavas_updateEnclShutdownReason(&location, FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&enclSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    current_time = 0;
     /* Wait for the debouncing logic to run..*/
    while(current_time < timeout_ms)
    {
        status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &enclInfo);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if(enclInfo.shutdownReason == FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED )
        {
            break;
        }
        current_time = current_time + 200;
        fbe_api_sleep(200);
        
    }
    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_OK);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(pandavas_commmand_response_callback_function,
                                                                    enclRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&enclSem);

}   // end of pandavas_testDae0ShutdownFaults

/*
 * PS related functions
 */
void pandavas_updatePePsFault(SP_ID spid, fbe_u32_t psIndex, fbe_bool_t psFault)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, ps %d, force psFault %d\n",
               __FUNCTION__, spid, psIndex, psFault);

    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[spid].psStatus[psIndex].generalFault = 
        psFault;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of pandavas_updatePePsFault

void pandavas_testDpePsFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         psSem;
    fbe_notification_registration_id_t      psRegId;
    fbe_u32_t                               dpePsCount;
    fbe_device_physical_location_t          location = {0};

    fbe_semaphore_init(&psSem, 0, 1);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  pandavas_commmand_response_callback_function,
																  &psSem,
																  &psRegId);

    expected_class_id = FBE_CLASS_ID_PS_MGMT;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    status = fbe_api_esp_encl_mgmt_get_ps_count(&location, &dpePsCount);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*
     * Fault both DPE PS's & verify CacheStatus change
     */
    // fault first PS
    pandavas_updatePePsFault(SP_A, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (set PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);

    fbe_api_sleep (5000);

    // fault second PS
    pandavas_updatePePsFault(SP_B, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (set PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    if (dpePsCount > 2)
    {
        // fault another PS
        pandavas_updatePePsFault(SP_B, 1, TRUE);

        mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (set PsFault) ===", __FUNCTION__);
        dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_FAILED);

    fbe_api_sleep (5000);

    /*
     * Clear DPE PS faults
     */
    // clear fault on first PS
    pandavas_updatePePsFault(SP_A, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Clear PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);

    fbe_api_sleep (5000);

    // clear fault on second PS
    pandavas_updatePePsFault(SP_B, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Clear PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    if (dpePsCount > 2)
    {
        // fault another PS
        pandavas_updatePePsFault(SP_B, 1, FALSE);

        mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Clear PsFault) ===", __FUNCTION__);
        dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
        MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    }

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_OK);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(pandavas_commmand_response_callback_function,
                                                                    psRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&psSem);

}   // end of pandavas_testDpePsFaults

void pandavas_testXpePsFaults(void)
{
    fbe_status_t                            status;
    DWORD                                   dwWaitResult;
    fbe_semaphore_t                         psSem;
    fbe_notification_registration_id_t      psRegId;

    fbe_semaphore_init(&psSem, 0, 1);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_ESP,
																  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
																  pandavas_commmand_response_callback_function,
																  &psSem,
																  &psRegId);

    expected_class_id = FBE_CLASS_ID_PS_MGMT;
    mut_printf(MUT_LOG_LOW, "%s, expected_class_id 0x%x\n", 
               __FUNCTION__, expected_class_id);

    /*
     * Fault 3 out of 4 xPE PS's & verify CacheStatus change
     */
    // fault first PS
    pandavas_updatePePsFault(SP_A, 0, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (set PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);

    fbe_api_sleep (5000);

    // fault second & third PS's
    pandavas_updatePePsFault(SP_B, 0, TRUE);

    pandavas_updatePePsFault(SP_A, 1, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (set PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_FAILED);

    fbe_api_sleep (5000);

    /*
     * Clear DPE PS faults
     */
    // clear fault on first PS
    pandavas_updatePePsFault(SP_A, 0, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Clear PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_DEGRADED);

    fbe_api_sleep (5000);

    // clear fault on second & third PS's
    pandavas_updatePePsFault(SP_B, 0, FALSE);

    pandavas_updatePePsFault(SP_A, 1, FALSE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Clear PsFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&psSem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_OK);

    status = fbe_api_notification_interface_unregister_notification(pandavas_commmand_response_callback_function,
                                                                    psRegId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&psSem);

}   // end of pandavas_testXpePsFaults

/*
 * Fan related functions
 */
void pandavas_updateMultiFanFault(SP_ID spid, fbe_bool_t multiFanFlt)
{
    SPECL_SFI_MASK_DATA                     sfi_mask_data;
    fbe_u32_t                               suitcaseIndex;

    mut_printf(MUT_LOG_LOW, "  %s, sp %d, force multiFanFlt %d\n",
               __FUNCTION__, spid, multiFanFlt);
    if (spid == SP_A)
    {
        suitcaseIndex = 0;
    }
    else
    {
        suitcaseIndex = 1;
    }
    sfi_mask_data.structNumber = SPECL_SFI_BLOWER_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.fanStatus.fanSummary[spid].multFanFault = multiFanFlt;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

}   // end of pandavas_updateMultiFanFault

void pandavas_testFanFaults(void)
{

// jap - SPECL SFI multi-fan fault not working
#if FALSE
    /*
     * Force Multi-Fan fault
     */
    mut_printf(MUT_LOG_LOW, "PANDAVAS: SP %d detect MultiFanFault\n", currentSp);
    pandavas_updateMultiFanFault(currentSp, TRUE);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event (Force MultiFanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    status = fbe_api_esp_common_getCacheStatus(&cacheStatus);
    mut_printf(MUT_LOG_LOW, "  PANDAVAS: cacheStatus %d, status 0x%x\n", 
               cacheStatus, status);
// jap - temp
/*
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
//    MUT_ASSERT_TRUE(cacheStatus == FBE_CACHE_STATUS_FAILED);
    mut_printf(MUT_LOG_LOW, "=== %s, wait for CacheStatus change event 2 (Force MultiFanFault) ===", __FUNCTION__);
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    status = fbe_api_esp_common_getCacheStatus(&cacheStatus);
    mut_printf(MUT_LOG_LOW, "  PANDAVAS: cacheStatus %d, status 0x%x\n", 
               cacheStatus, status);
*/
// jap - temp
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(cacheStatus == FBE_CACHE_STATUS_FAILED);

    mut_printf(MUT_LOG_LOW, "PANDAVAS: SP %d detect MultiFanFault - SUCCESSFUL\n", currentSp);
#endif

}   // end of pandavas_testFanFaults

/*!**************************************************************
 * pandavas_dpe_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test Cache Status on DPE based array.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/23/2011 - Created. Joe Perry
 *
 ****************************************************************/

void pandavas_dpe_test(SPID_HW_TYPE platform_type)
{
    currentSp = SP_A;

    /*
     * Start up other SP
     */
    fbe_api_sleep (20000);

    /*
     * Get initial Cache Status (if good, continue)
    */
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): verify initial SP %d SPS status\n", currentSp);
    pandavas_waitForCacheStatusChange(FBE_CACHE_STATUS_OK);

    /*
     * Verify Suitcase Shutdown condition detected
     */
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): SP %d detect ShutdownWarning\n", currentSp);
    pandavas_testSuitcaseShutdown();
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): SP %d detect ShutdownWarning - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);

    /*
     * Verify SPS's faulted condition detected
     */
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): SP %d detect no SPS available\n", currentSp);
    pandavas_testBbuNotAvailable();
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): SP %d detect no SPS available - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);

    /*
     * Verify DPE PS's faulted condition detected
     */
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): SP %d detect DPE PS faults\n", currentSp);
    pandavas_testDpePsFaults();
    mut_printf(MUT_LOG_LOW, "PANDAVAS(DPE): SP %d detect DPE PS faults - SUCCESSFUL\n", currentSp);

    fbe_api_sleep (5000);

    return;
}
/******************************************
 * end pandavas_dpe_test()
 ******************************************/

void pandavas_oberon_test(void)
{
    pandavas_dpe_test(SPID_OBERON_1_HW_TYPE);
}

void pandavas_hyperion_test(void)
{
    pandavas_dpe_test(SPID_HYPERION_1_HW_TYPE);
}

void pandavas_startupSp(fbe_sim_transport_connection_target_t targetSp,
                        SPID_HW_TYPE platform_type)
{
    fbe_test_startEspAndSep_with_common_config(targetSp,
                                        FBE_ESP_TEST_BASIC_CONIG,
                                        platform_type,
                                        NULL,
                                        NULL);
}

/*!**************************************************************
 * pandavas_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/23/2011 - Created. Joe Perry
 *
 ****************************************************************/
void pandavas_setup(SPID_HW_TYPE platform_type)
{

    currentSp = SP_B;
    pandavas_startupSp(FBE_SIM_SP_B, platform_type);

//    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

}
/**************************************
 * end pandavas_setup()
 **************************************/

void pandavas_oberon_setup()
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_FP_CONIG,
                                                      SPID_OBERON_1_HW_TYPE,
                                                      NULL,
                                                      fbe_test_reg_set_persist_ports_true);
}

void pandavas_hyperion_setup()
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_FP_CONIG,
                                                      SPID_HYPERION_1_HW_TYPE,
                                                      NULL,
                                                      fbe_test_reg_set_persist_ports_true);
}

void pandavas_triton_setup()
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_FP_CONIG,
                                                      SPID_TRITON_1_HW_TYPE,
                                                      NULL,
                                                      fbe_test_reg_set_persist_ports_true);
} 


/*!**************************************************************
 * pandavas_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the pandavas test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/23/2011 - Created. Joe Perry
 *
 ****************************************************************/

void pandavas_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end pandavas_destroy()
 ******************************************/
/*************************
 * end file pandavas_test.c
 *************************/


