/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file vito_corleone_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the Periodic SPS Test processing of SPS Management Object of ESP.
 * 
 * @version
 *   04/07/2010 - Created. Joe Perry
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
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_sps_info.h"

#include "fbe_test_configurations.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * vito_corleone_short_desc = "Verify Periodic SPS Testing of ESP's SPS Management Object";
char * vito_corleone_long_desc ="\
Vito Corleone\n\
        -lead character in the movie THE GODFATHER\n\
        -famous quote 'I'm gonna make him an offer he can't refuse.'\n\
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
STEP 2: Validate that the periodic SPS Test time is detected\n\
        - Verify that \n\
";

static fbe_object_id_t	expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_notification_registration_id_t  reg_id;

#define FBE_SPS_MAX_COUNT       2
#define FBE_LOCAL_SPS           0
#define FBE_PEER_SPS            1


static void vito_corleone_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

//	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===",
//        update_object_msg->object_id, expected_object_id);

	if (update_object_msg->object_id == expected_object_id) 
	{
//    	mut_printf(MUT_LOG_LOW, "callback, phys_location: actual 0x%x, expected 0x%x ===",
//            update_object_msg->notification_info.notification_data.data_change_info.device_mask, FBE_DEVICE_TYPE_SPS);
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_SPS)
	    {
        	mut_printf(MUT_LOG_LOW, "=== %s, SPS Event detected ===", __FUNCTION__);
    		fbe_semaphore_release(sem, 0, 1 ,FALSE);
        } 
	}
}

void vito_corleone_getInitialSpsStatus(fbe_sim_transport_connection_target_t spDestination,
                                       fbe_semaphore_t *sem)
{
    fbe_u8_t                                spsIndex, eventCount;
    fbe_status_t                            status;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo;
    fbe_object_id_t                         objectId;

    switch (spDestination)
    {
    case FBE_SIM_SP_A:
        mut_printf(MUT_LOG_LOW, "=== %s, for SPA ===", __FUNCTION__);
        spsIndex = FBE_LOCAL_SPS;
        break;
    case FBE_SIM_SP_B:
        mut_printf(MUT_LOG_LOW, "=== %s, for SPB ===", __FUNCTION__);
        spsIndex = FBE_PEER_SPS;
        break;
    default:
        MUT_ASSERT_TRUE(FALSE);
        return;
    }

    fbe_api_sim_transport_set_target_server(spDestination);

    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);
    expected_object_id = objectId;

    for (eventCount = 0; eventCount < 3; eventCount++)
    {
        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        if (spDestination == FBE_SIM_SP_B)
        {
            spsStatusInfo.spsLocation.slot = 1;
        }
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo);
        mut_printf(MUT_LOG_LOW, "ESP SpsStatus 0x%x, cabling 0x%x, status 0x%x\n", 
            spsStatusInfo.spsStatus, spsStatusInfo.spsCablingStatus, status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo.spsStatus == SPS_STATE_AVAILABLE)
        {
            break;
        }
    }

    // turn off SPS simulation
    status = fbe_api_esp_sps_mgmt_setSimulateSps(FALSE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}   // end of vito_corleone_getInitialSpsStatus

void vito_corleone_setSpsTime(fbe_sim_transport_connection_target_t spDestination)
{
    fbe_status_t                            status;
    fbe_esp_sps_mgmt_spsTestTime_t          spsTestTimeInfo;
    fbe_esp_sps_mgmt_spsTestTime_t          getSpsTestTimeInfo;
    fbe_system_time_t                       currentTime;
    fbe_sim_transport_connection_target_t   peerSp;

    switch (spDestination)
    {
    case FBE_SIM_SP_A:
        mut_printf(MUT_LOG_LOW, "=== %s, for SPA ===", __FUNCTION__);
        peerSp = FBE_SIM_SP_B;
        break;
    case FBE_SIM_SP_B:
        mut_printf(MUT_LOG_LOW, "=== %s, for SPB ===", __FUNCTION__);
        peerSp = FBE_SIM_SP_A;
        break;
    default:
        MUT_ASSERT_TRUE(FALSE);
        return;
    }

    fbe_api_sim_transport_set_target_server(spDestination);

    // verify the initial SPS Test Time
    fbe_set_memory(&spsTestTimeInfo, 0, sizeof(fbe_esp_sps_mgmt_spsTestTime_t));
    status = fbe_api_esp_sps_mgmt_getSpsTestTime(&spsTestTimeInfo);
    mut_printf(MUT_LOG_LOW, "ESP spsTestTime weekday %d, hour %d, mins %d, status 0x%x\n", 
        spsTestTimeInfo.spsTestTime.weekday,
        spsTestTimeInfo.spsTestTime.hour,
        spsTestTimeInfo.spsTestTime.minute,
        status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // set SPS Test Time
    status = fbe_get_system_time(&currentTime);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    currentTime.minute += 5;            // set for a few minutes from now
    fbe_set_memory(&spsTestTimeInfo, 0, sizeof(fbe_esp_sps_mgmt_spsTestTime_t));
    spsTestTimeInfo.spsTestTime = currentTime;
    mut_printf(MUT_LOG_LOW, "ESP new spsTestTime weekday %d, hour %d, mins %d\n", 
        spsTestTimeInfo.spsTestTime.weekday,
        spsTestTimeInfo.spsTestTime.hour,
        spsTestTimeInfo.spsTestTime.minute);
    status = fbe_api_esp_sps_mgmt_setSpsTestTime(&spsTestTimeInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // verify that both SP's have the new SPS Test Time
    fbe_api_sleep (2000);           // delay so any in progress peer-to-peer messages can complete
    fbe_zero_memory(&getSpsTestTimeInfo, sizeof(fbe_esp_sps_mgmt_spsTestTime_t));
    status = fbe_api_esp_sps_mgmt_getSpsTestTime(&getSpsTestTimeInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "ESP sp %d, spsTestTime weekday %d, hour %d, mins %d\n", 
               spDestination,
               getSpsTestTimeInfo.spsTestTime.weekday,
               getSpsTestTimeInfo.spsTestTime.hour,
               getSpsTestTimeInfo.spsTestTime.minute);
    MUT_ASSERT_TRUE(spsTestTimeInfo.spsTestTime.weekday == getSpsTestTimeInfo.spsTestTime.weekday);
    MUT_ASSERT_TRUE(spsTestTimeInfo.spsTestTime.hour == getSpsTestTimeInfo.spsTestTime.hour);
    MUT_ASSERT_TRUE(spsTestTimeInfo.spsTestTime.minute == getSpsTestTimeInfo.spsTestTime.minute);
    mut_printf(MUT_LOG_LOW, "%s, SP %d test time verified\n", 
               __FUNCTION__, spDestination);

    fbe_api_sim_transport_set_target_server(peerSp);
    fbe_zero_memory(&getSpsTestTimeInfo, sizeof(fbe_esp_sps_mgmt_spsTestTime_t));
    status = fbe_api_esp_sps_mgmt_getSpsTestTime(&getSpsTestTimeInfo);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "ESP sp %d, spsTestTime weekday %d, hour %d, mins %d\n", 
               spDestination,
               getSpsTestTimeInfo.spsTestTime.weekday,
               getSpsTestTimeInfo.spsTestTime.hour,
               getSpsTestTimeInfo.spsTestTime.minute);
    MUT_ASSERT_TRUE(spsTestTimeInfo.spsTestTime.weekday == getSpsTestTimeInfo.spsTestTime.weekday);
    MUT_ASSERT_TRUE(spsTestTimeInfo.spsTestTime.hour == getSpsTestTimeInfo.spsTestTime.hour);
    MUT_ASSERT_TRUE(spsTestTimeInfo.spsTestTime.minute == getSpsTestTimeInfo.spsTestTime.minute);
    mut_printf(MUT_LOG_LOW, "%s, SP %d test time verified\n", 
               __FUNCTION__, peerSp);

    fbe_api_sim_transport_set_target_server(spDestination);

}   // end of vito_corleone_setSpsTime


void vito_corleone_verifySpsTestingComplete(fbe_semaphore_t *sem)
{
    fbe_status_t                            status;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo[FBE_SPS_MAX_COUNT];
    fbe_u32_t                               eventCount;

    /*
     * get SPS status from SPA
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for SPA to complete SPS Testing ===", 
               __FUNCTION__);
    for (eventCount = 0; eventCount < 2; eventCount++)
    {
        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        spsStatusInfo[FBE_LOCAL_SPS].spsLocation.slot = FBE_LOCAL_SPS;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo[FBE_LOCAL_SPS]);
        mut_printf(MUT_LOG_LOW, "%s, SpsStatus 0x%x, cabling 0x%x, status 0x%x\n", 
            __FUNCTION__, 
                   spsStatusInfo[FBE_LOCAL_SPS].spsStatus, 
                   spsStatusInfo[FBE_LOCAL_SPS].spsCablingStatus, 
                   status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo[FBE_LOCAL_SPS].spsStatus == SPS_STATE_AVAILABLE)
        {
            break;
        }
    }
    mut_printf(MUT_LOG_LOW, "=== %s, SPSA Tested Successfully ===", __FUNCTION__);

    // get SPS status from SPB
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    mut_printf(MUT_LOG_LOW, "=== %s, wait for SPB to complete SPS Testing ===", 
               __FUNCTION__);
    for (eventCount = 0; eventCount < 2; eventCount++)
    {
        fbe_set_memory(&spsStatusInfo, 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
        spsStatusInfo[FBE_LOCAL_SPS].spsLocation.slot = FBE_LOCAL_SPS;
        status = fbe_api_esp_sps_mgmt_getSpsStatus(&spsStatusInfo[FBE_LOCAL_SPS]);
        mut_printf(MUT_LOG_LOW, "%s, SpsStatus 0x%x, cabling 0x%x, status 0x%x\n", 
            __FUNCTION__, 
                   spsStatusInfo[FBE_LOCAL_SPS].spsStatus, 
                   spsStatusInfo[FBE_LOCAL_SPS].spsCablingStatus, 
                   status);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        if (spsStatusInfo[FBE_LOCAL_SPS].spsStatus == SPS_STATE_AVAILABLE)
        {
            break;
        }
    }
    mut_printf(MUT_LOG_LOW, "=== %s, SPSB Tested Successfully ===", __FUNCTION__);

}   // end of vito_corleone_verifySpsTestingComplete


/*!**************************************************************
 * vito_corleone_test() 
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

void vito_corleone_test(void)
{
    fbe_status_t                            status;
    fbe_semaphore_t                         sem;

    fbe_semaphore_init(&sem, 0, 1);

    // Initialize to get event notification
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
//																  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
																  vito_corleone_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    // Verify initial SPS Status on both SP's (wait for initial SPS Test to complete)
    vito_corleone_getInitialSpsStatus(FBE_SIM_SP_A, &sem);
    vito_corleone_getInitialSpsStatus(FBE_SIM_SP_B, &sem);

    fbe_api_sleep (10000);           // delay so any in progress peer-to-peer messages can complete

    // Verify & set SPS Test Time on both SP's
    vito_corleone_setSpsTime(FBE_SIM_SP_A);
    vito_corleone_setSpsTime(FBE_SIM_SP_B);

    // verify that one SPS is testing
    vito_corleone_verifySpsTestingComplete(&sem);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(vito_corleone_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    fbe_api_sleep (10000);           // delay so any in progress peer-to-peer messages can complete

    return;
}
/******************************************
 * end vito_corleone_test()
 ******************************************/
/*!**************************************************************
 * vito_corleone_setup()
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
void vito_corleone_setup(void)
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_FP_CONIG,
                                                      SPID_DEFIANT_M1_HW_TYPE,
                                                      NULL,
                                                      fbe_test_reg_set_persist_ports_true);
}
/**************************************
 * end vito_corleone_setup()
 **************************************/

/*!**************************************************************
 * vito_corleone_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the vito_corleone test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void vito_corleone_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end vito_corleone_destroy()
 ******************************************/
/*************************
 * end file vito_corleone_test.c
 *************************/


