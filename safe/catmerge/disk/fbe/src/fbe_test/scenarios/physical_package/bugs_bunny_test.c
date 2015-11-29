/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file bugs_bunny_test.c
 ***************************************************************************
 *
 * @brief
 *  This file tests SPS Object of the Physical Package.  This will test the
 *  various IOCTL's/commands that can be sent to the SPS Object.
 *
 * @version
 *   11/30/2009 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * bugs_bunny_short_desc = "SPS Test (status directly from Board Object only";
char * bugs_bunny_long_desc ="\
Tests the SPS Object's processing of all available IOCTL's/commands:\n\
    GetStatus command - returns the status of the SPS\n\
    GetSN command - returns the SN of the SPS\n\
    GetPN command - returns the PN of the SPS\n\
    GetMFG command - returns the Manufacturing info of the SPS\n\
    GetREV command - returns the FW Revision of the SPS\n\
    GetModelId command - returns the Model ID of the SPS\n\
    StartTest command - initiates an SPS Battery Test\n\
    StopTest command - terminates an SPS Battery Test\n\
    Shutdown command - initiates a shutdown of the SPS\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] 1 SPS\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n"
"STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Validate that all services in ESP is started correctly\n\
        - Validate that ERP is able to see 3 Viper enclosures\n\
STEP 2: Get Resume PROM type data from SPS\n\
        - Using TAPI retrieve SPS SN, PN, Mfg info, FW Rev info\n\
STEP 3: Status Reporting\n\
        - Verify that the various SPS statuses are reported correctly\n\
STEP 4: SPS Battery Test\n\
        - Start an SPS Test (need PS support to simulate AC_FAIL on PS's)\n\
        - Stop the SPS Test when cabling has been verified\n\
STEP 5: SPS Shutdown\n\
        - Send a Shutdown command to SPS & verify that the SPS goes away\n";


static fbe_object_id_t         expectedObjectId = FBE_OBJECT_ID_INVALID;
static fbe_notification_registration_id_t  reg_id;

static void bugs_bunny_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                            void *context)
{
    fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

    if (update_object_msg->object_id == expectedObjectId)
    {
        mut_printf(MUT_LOG_LOW, "  %s, obj 0x%x, device_mask %lld, SPS 0x%llx\n",
                   __FUNCTION__,
                   update_object_msg->object_id,
                   update_object_msg->notification_info.notification_data.data_change_info.device_mask, FBE_DEVICE_TYPE_SPS);
        if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_SPS) 
        {
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }

}

/*!**************************************************************
 * bugs_bunny_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the SPS Object test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/30/2009 - Created. Joe Perry
 *
 ****************************************************************/

void bugs_bunny_test(void)
{
    fbe_status_t                                fbeStatus;
    fbe_object_id_t                             objectId;
    fbe_sps_get_sps_status_t                    spsStatus= {0};
    fbe_sps_get_sps_manuf_info_t                spsManufInfo = {0};
    fbe_sps_action_type_t                       spsCommand= FBE_SPS_ACTION_NONE;
    fbe_semaphore_t                             sem;
    DWORD                                       dwWaitResult;

    fbe_semaphore_init(&sem, 0, 1);

    // Get handle to enclosure object
    fbeStatus = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);
    expectedObjectId = objectId;

    // Initialize to get event notification
    fbeStatus = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                     FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                     FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                                     bugs_bunny_commmand_response_callback_function,
                                                                     &sem,
                                                                     &reg_id);

    /*
     * Get Initial SPS Status
     */
    fbeStatus = fbe_api_board_get_sps_status(objectId, &spsStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "Initial SPS Status 0x%x\n",
        spsStatus.spsInfo.spsStatus);
    MUT_ASSERT_TRUE(spsStatus.spsInfo.spsStatus == SPS_STATE_AVAILABLE);

    /*
     * Get Manufacturing Info for SPS
     */
    fbeStatus = fbe_api_board_get_sps_manuf_info(objectId, &spsManufInfo);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "SPS Manufacturing Info\n");
    mut_printf(MUT_LOG_LOW, "  SerialNumber    %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsSerialNumber);
    mut_printf(MUT_LOG_LOW, "  PartNumber      %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsPartNumber);
    mut_printf(MUT_LOG_LOW, "  PartNumRevision %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsPartNumRevision);
    mut_printf(MUT_LOG_LOW, "  Vendor          %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsVendor);
    mut_printf(MUT_LOG_LOW, "  VendorModelNum  %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsVendorModelNumber);
    mut_printf(MUT_LOG_LOW, "  FwRevision      %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsFirmwareRevision);
    mut_printf(MUT_LOG_LOW, "  ModelString     %s\n", 
               spsManufInfo.spsManufInfo.spsModuleManufInfo.spsModelString);

    /*
     * Perform SPS Testing & verify SPS Status changes
     */

    // Start SPS Test
    mut_printf(MUT_LOG_LOW, "BUGS: Test starting SPS Test\n");
    spsCommand = FBE_SPS_ACTION_START_TEST;
    fbeStatus = fbe_api_board_send_sps_command(objectId, spsCommand);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbeStatus = fbe_api_board_get_sps_status(objectId, &spsStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "SPS Testing started, SPS Status 0x%x\n",
        spsStatus.spsInfo.spsStatus);
    MUT_ASSERT_TRUE(spsStatus.spsInfo.spsStatus == SPS_STATE_TESTING);
    mut_printf(MUT_LOG_LOW, "BUGS: Test starting SPS Test - verified\n");

    // Negative Test - verify that trying to start an SPS Test will fail if already testing
    spsCommand = FBE_SPS_ACTION_START_TEST;
    fbeStatus = fbe_api_board_send_sps_command(objectId, spsCommand);
    MUT_ASSERT_TRUE(fbeStatus != FBE_STATUS_OK);

    // Stop SPS Test
    mut_printf(MUT_LOG_LOW, "BUGS: Test stoping SPS Test\n");
    spsCommand = FBE_SPS_ACTION_STOP_TEST;
    fbeStatus = fbe_api_board_send_sps_command(objectId, spsCommand);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);

    dwWaitResult = fbe_semaphore_wait_ms(&sem, 150000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    fbeStatus = fbe_api_board_get_sps_status(objectId, &spsStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "SPS Testing stopped, SPS Status 0x%x\n",
        spsStatus.spsInfo.spsStatus);
    MUT_ASSERT_TRUE(spsStatus.spsInfo.spsStatus == SPS_STATE_AVAILABLE);
    mut_printf(MUT_LOG_LOW, "BUGS: Test stoping SPS Test - verified\n");

    // Disable (must be in BatteryOnline state to do)
#if FALSE
    // Shutdown SPS
    mut_printf(MUT_LOG_LOW, "BUGS: Test shutting down SPS\n");
    spsCommand.spsAction = FBE_SPS_ACTION_SHUTDOWN;
    fbeStatus = fbe_api_board_send_sps_command(objectId, &spsCommand);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    fbeStatus = fbe_api_board_get_sps_status(objectId, &spsStatus);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    mut_printf(MUT_LOG_LOW, "SPS Shutdown, SPS Status 0x%x\n",
        spsStatus.spsStatus);
    MUT_ASSERT_TRUE(!spsStatus.spsInserted);
    mut_printf(MUT_LOG_LOW, "BUGS: Test shutting down SPS - verified\n");
#endif

    // cleanup
    fbeStatus = fbe_api_notification_interface_unregister_notification(bugs_bunny_commmand_response_callback_function,
                                                                       reg_id);
    MUT_ASSERT_TRUE(fbeStatus == FBE_STATUS_OK);
    fbe_semaphore_destroy(&sem);

    return;
}
/******************************************
 * end bugs_bunny_test()
 ******************************************/
/*************************
 * end file bugs_bunny_test.c
 *************************/


