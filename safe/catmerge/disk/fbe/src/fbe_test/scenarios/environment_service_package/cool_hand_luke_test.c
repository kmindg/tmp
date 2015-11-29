/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cool_hand_luke_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify the Peer-to-Peer Communications of SPS Management Object of ESP.
 * 
 * @version
 *   06/14/2010 - Created. Joe Perry
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

char * cool_hand_luke_short_desc = "Verify Peer-to-Peer Messaging of ESP's SPS Management Object";
char * cool_hand_luke_long_desc ="\
Vito Corleone\n\
        -lead character in the movie COOL HAND LUKE\n\
        -famous quote 'Sometimes nothing is a real cool hand'\n\
                      'Calling it your job dont make it right, boss'\n\
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


static void cool_hand_luke_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===",
        update_object_msg->object_id, expected_object_id);

	if (update_object_msg->object_id == expected_object_id) 
	{
    	mut_printf(MUT_LOG_LOW, "callback, phys_location: actual 0x%llx, expected 0x%llx ===",
            update_object_msg->notification_info.notification_data.data_change_info.device_mask, FBE_DEVICE_TYPE_SPS);
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_SPS)
	    {
    		fbe_semaphore_release(sem, 0, 1 ,FALSE);
        } 
	}
}

void cool_hand_luke_getInitialSpsStatus(fbe_sim_transport_connection_target_t spDestination,
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
        spsStatusInfo.spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        spsStatusInfo.spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
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

}   // end of cool_hand_luke_getInitialSpsStatus

/*!**************************************************************
 * cool_hand_luke_test() 
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

void cool_hand_luke_test(void)
{
    fbe_status_t                            status;
    fbe_esp_sps_mgmt_spsStatus_t            spsStatusInfo[FBE_SPS_MAX_COUNT];
    fbe_object_id_t                         objectId[FBE_SPS_MAX_COUNT];
	fbe_semaphore_t 						sem;
	DWORD 									dwWaitResult;
    SPECL_SFI_MASK_DATA                     sfi_mask_data;

    fbe_semaphore_init(&sem, 0, 1);

    /*
     * Initialize to get event notification for both SP's
     */
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
																  cool_hand_luke_commmand_response_callback_function,
																  &sem,
																  &reg_id);

    // Verify initial SPS Status on both SP's (wait for initial SPS Test to complete)
    cool_hand_luke_getInitialSpsStatus(FBE_SIM_SP_A, &sem);
    cool_hand_luke_getInitialSpsStatus(FBE_SIM_SP_B, &sem);

    /*
     * Get Board Object ID for both SP's
     */
    // SPA
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_get_board_object_id(&(objectId[FBE_LOCAL_SPS]));
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId[FBE_LOCAL_SPS] != FBE_OBJECT_ID_INVALID);
    // SPB
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_get_board_object_id(&(objectId[FBE_PEER_SPS]));
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId[FBE_PEER_SPS] != FBE_OBJECT_ID_INVALID);

    /*
     * Verify that ESP sps_mgmt detects SPSA Status change
     */
    // Change SPSA Status in Board Object
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sfi_mask_data.structNumber = SPECL_SFI_SPS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.spsStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.spsStatus.state = SPS_STATE_CHARGING;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

	dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
	MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    // verify that SPA gets SPSA notification
    fbe_api_sleep (1000);           // delay so sps_mgmt can process the same event
    fbe_set_memory(&(spsStatusInfo[FBE_LOCAL_SPS]), 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatusInfo[FBE_LOCAL_SPS].spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    spsStatusInfo[FBE_LOCAL_SPS].spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    spsStatusInfo[FBE_LOCAL_SPS].spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo[FBE_LOCAL_SPS]));
    mut_printf(MUT_LOG_LOW, "ESP SPA LOCAL SpsStatus 0x%x, status 0x%x\n", 
        spsStatusInfo[FBE_LOCAL_SPS].spsStatus, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(spsStatusInfo[FBE_LOCAL_SPS].spsStatus, SPS_STATE_CHARGING);

    // verify if SPB got Status update on peer SPS
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_set_memory(&(spsStatusInfo[FBE_PEER_SPS]), 0, sizeof(fbe_esp_sps_mgmt_spsStatus_t));
    spsStatusInfo[FBE_PEER_SPS].spsLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
    spsStatusInfo[FBE_PEER_SPS].spsLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    spsStatusInfo[FBE_PEER_SPS].spsLocation.slot = 0;
    status = fbe_api_esp_sps_mgmt_getSpsStatus(&(spsStatusInfo[FBE_PEER_SPS]));
    mut_printf(MUT_LOG_LOW, "ESP SPB PEER SpsStatus 0x%x, status 0x%x\n", 
        spsStatusInfo[FBE_PEER_SPS].spsStatus, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(spsStatusInfo[FBE_PEER_SPS].spsStatus, SPS_STATE_CHARGING);

    // cleanup
    status = fbe_api_notification_interface_unregister_notification(cool_hand_luke_commmand_response_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}
/******************************************
 * end cool_hand_luke_test()
 ******************************************/
/*!**************************************************************
 * cool_hand_luke_setup()
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
void cool_hand_luke_setup(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");

    /*
     * we do the setup on SPA side
     */
    mut_printf(MUT_LOG_LOW, "=== cool_hand_luke_test Loading Physical Config on SPA ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

    /*
     * we do the setup on SPB side
     */
    mut_printf(MUT_LOG_LOW, "=== cool_hand_luke_test Loading Physical Config on SPB ===");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    status = fbe_test_load_fp_config(SPID_PROMETHEUS_M1_HW_TYPE);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // debug only
    mut_printf(MUT_LOG_LOW, "=== simple config loading is done ===\n");

    fp_test_start_physical_package();

    /* Create or re-create the registry file */
    fbe_test_esp_create_registry_file(FBE_TRUE);

    /* Initialize registry parameter to persist ports */
    fp_test_set_persist_ports(TRUE);

    sep_config_load_sep_and_neit_no_esp();

    status = fbe_test_sep_util_wait_for_database_service(20000);

    status = fbe_test_startEnvMgmtPkg(TRUE);        // wait for ESP object to become Ready
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    fbe_test_wait_for_all_esp_objects_ready();

}
/**************************************
 * end cool_hand_luke_setup()
 **************************************/

/*!**************************************************************
 * cool_hand_luke_destroy()
 ****************************************************************
 * @brief
 *  Cleanup the cool_hand_luke test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void cool_hand_luke_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy_all_dual_sp();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    return;
}
/******************************************
 * end cool_hand_luke_destroy()
 ******************************************/
/*************************
 * end file cool_hand_luke_test.c
 *************************/


