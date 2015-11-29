
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file drive_remove_insert_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the funcctions to test drive debug control
 *  comamnds (cru on/off).
 *
 * @version
 *   22-Jul-2011 GB - Created.
 *
 ***************************************************************************/
#include "esp_tests.h"
#include "physical_package_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_file.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe_test_esp_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_base_environment_debug.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

char * drive_remove_insert_short_desc = "Test drive remove/insert in mixed enclosures.";
char * drive_remove_insert_long_desc =
    "\n"
    "\n"
    "This test exercises drive removal and insertion using the drive debug control command.\n"
    "For each enclosure: \n"
    "    - Find the first drive and issue drive remove/insert\n"
    "    - Find the last drive and issue drive remove/insert\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator must support Voyager enclosure.\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 1 each viper, derringer, voyager enclosures\n"
    "    [PP] per enclosure 15 SAS drives (PDO)\n"
    "    [PP] per enclosure 15 logical drives (LDO)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "\n"  
    "STEP 3: Test drive remove/insert in each enclosure.\n"
    "    - Initiate the drive removal for first drive.\n"
    "    - Verify the drive removal for first drive.\n"
    "    - Initiate the drive insertion for first drive.\n"
    "    - Verify the drive removal for first drive.\n"
    "    - Initiate the drive removal for last drive.\n"
    "    - Verify the drive removal for last drive.\n"
    "    - Initiate the drive insertion for last drive.\n"
    "    - Verify the drive removal for last drive.\n"
    "\n"
    "STEP 5: Shutdown the Terminator/Physical package/ESP.\n"
    "\n"
    ;

extern mixed_encl_config_type_t encl_config_list[];

static fbe_object_id_t target_object_id = FBE_OBJECT_ID_INVALID;
static fbe_device_physical_location_t target = {0};
fbe_semaphore_t drive_insert_sem;

static void drive_insert_esp_data_change_callback_function(update_object_msg_t * pObjNotifyMsg, 
                                                            void *context);

/*!***************************************************************
 * @fn drive_test_find_first_drive()
 *****************************************************************
 * @brief
 *  Find the first inserted drive in the enclosure.
 *
 * @param lastSlot - the number of slots in the enclosure
 * @param pLocation - contains bus-encl to search, slot field is
 *                    updated with first drive
 * 
 * @return slot position of the first drive in the enclosure
 *
 ****************************************************************/
fbe_status_t drive_test_find_first_drive(fbe_u32_t lastSlot, fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                        status;
    fbe_u32_t                           slot;
    fbe_u32_t                           numDrives;
    fbe_esp_drive_mgmt_drive_info_t     getDriveInfo = {0};

    // find first slot in the enclosure with drive present
    for(slot = 0; slot < lastSlot; slot++)
    {
        pLocation->slot = slot;
        *(fbe_device_physical_location_t *)&(getDriveInfo.location) = *pLocation;
        // get drive info
        status = fbe_api_esp_drive_mgmt_get_drive_info(&getDriveInfo);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        // if drive present, found first drive
        if (getDriveInfo.inserted)
        {
            break;
        }
    }

    numDrives = mixed_config_number_of_configured_drives(pLocation->enclosure, lastSlot);
    
    if((slot == lastSlot) && (numDrives > 1))
    {
        mut_printf(MUT_LOG_LOW, "First Drive Present == Last Drive %d_%d_%d ",
                   pLocation->bus, pLocation->enclosure, pLocation->slot);
        // assert if no drive found
        MUT_ASSERT_TRUE(getDriveInfo.inserted);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // if drive state not ready fail
    MUT_ASSERT_FALSE_MSG(getDriveInfo.state != FBE_LIFECYCLE_STATE_READY,"Drive Not Ready!");

    // check if we fell out of the loop without finding an inserted drive
    MUT_ASSERT_TRUE(getDriveInfo.inserted);

    return(status);
} //drive_test_find_first_drive

/*!***************************************************************
 * @fn drive_test_find_last_drive()
 *****************************************************************
 * @brief
 *  Find the last inserted drive in the enclosure.
 *
 * @param lastSlot - the number of slots in the enclosure
 * @param pLocation - first drive location, slot field is
 *                    written with last drive
 * 
 * @return slot position of the last drive in the enclosure
 *
 ****************************************************************/
fbe_status_t drive_test_find_last_drive(fbe_u32_t lastSlot, fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                        status;
    fbe_u32_t                           slot;
    fbe_u32_t                           numDrives;
    fbe_u32_t                           firstDrive;
    fbe_esp_drive_mgmt_drive_info_t     getDriveInfo = {0};

    // start at the last drive slot and loop until inserted drive is found
    // or first drive is found
    for(firstDrive = pLocation->slot, slot = lastSlot-1; 
         firstDrive < slot; 
         slot--)
    {
        // set the location of the drive info we want
        pLocation->slot = slot;
        *(fbe_device_physical_location_t *)&(getDriveInfo.location) = *pLocation;
        // get drive info
        status = fbe_api_esp_drive_mgmt_get_drive_info(&getDriveInfo);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if(status != FBE_STATUS_OK)
        {
            return(status);
        }
        // if drive present, found last drive
        if (getDriveInfo.inserted)
        {
            //driveNum = slot;
            break;
        }
    }

    numDrives = mixed_config_number_of_configured_drives(pLocation->enclosure, lastSlot);

    if ((slot == firstDrive) && (numDrives > 1))
    {
        mut_printf(MUT_LOG_LOW, "Last Drive Present == First Drive %d_%d_%d ",
                   pLocation->bus, pLocation->enclosure, pLocation->slot);
        // assert if no drive found
        MUT_ASSERT_TRUE(getDriveInfo.inserted);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // if not ready, return fail
    MUT_ASSERT_FALSE_MSG(getDriveInfo.state != FBE_LIFECYCLE_STATE_READY,"Drive Not Ready!");

    return(FBE_STATUS_OK);

} //drive_test_find_last_drive

void drive_test_remove_drive(fbe_device_physical_location_t *pLocation,
                             fbe_u32_t lastSlot)
{
    DWORD                               dwWaitResult;
    fbe_status_t                        status;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_registration_id_t  registration_id;

   // Get drive_mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    target_object_id = object_id;

    // copy the drive location so it can be referenced in the callback notification routine
    *(fbe_device_physical_location_t*)&target = *pLocation;

    fbe_semaphore_init(&drive_insert_sem, 0, 1);
    // Register to get event notification from ESP
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  drive_insert_esp_data_change_callback_function,
                                                                  &drive_insert_sem,
                                                                  &registration_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //send comamnd to remove the drive
    status = fbe_api_esp_drive_mgmt_send_debug_control(pLocation->bus, 
                                                       pLocation->enclosure, 
                                                       pLocation->slot, 
                                                       lastSlot, 
                                                       FBE_DRIVE_ACTION_REMOVE,
                                                       FBE_FALSE, FBE_FALSE);

    // wait for the data change notification
    dwWaitResult = fbe_semaphore_wait_ms(&drive_insert_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
    // Unregister the notification
    status = fbe_api_notification_interface_unregister_notification(drive_insert_esp_data_change_callback_function,
                                                                    registration_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&drive_insert_sem);
    return;
}

void drive_test_insert_drive(fbe_device_physical_location_t *pLocation,
                             fbe_u32_t lastSlot)
{
    fbe_status_t                        status;
    DWORD                               dwWaitResult;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_registration_id_t  registration_id;

   // Get drive_mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    target_object_id = object_id;

    // copy the drive location so it can be referenced in the callback notification routine
    *(fbe_device_physical_location_t*)&target = *pLocation;

    fbe_semaphore_init(&drive_insert_sem, 0, 1);
    // Register to get event notification from ESP
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  drive_insert_esp_data_change_callback_function,
                                                                  &drive_insert_sem,
                                                                  &registration_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //send comamnd to remove the drive
    status = fbe_api_esp_drive_mgmt_send_debug_control(pLocation->bus, 
                                                       pLocation->enclosure, 
                                                       pLocation->slot, 
                                                       lastSlot, 
                                                       FBE_DRIVE_ACTION_INSERT,
                                                       FBE_FALSE, FBE_FALSE);
   // wait for the data change notification
    dwWaitResult = fbe_semaphore_wait_ms(&drive_insert_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
    // Unregister the notification
    status = fbe_api_notification_interface_unregister_notification(drive_insert_esp_data_change_callback_function,
                                                                    registration_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&drive_insert_sem);
    return;
}

fbe_status_t drive_test_get_drive_info(fbe_esp_drive_mgmt_drive_info_t *pGetDriveInfo, fbe_device_physical_location_t *pLocation)
{
    fbe_status_t    status;
    *(fbe_device_physical_location_t *)&(pGetDriveInfo->location) = *pLocation;
    status = fbe_api_esp_drive_mgmt_get_drive_info(pGetDriveInfo);
    return(status);
}
/*!***************************************************************
 * @fn drive_test_using_insert_remove_commands()
 *****************************************************************
 * @brief
 *  This function performs cru on/off on the drives and validates
 *  that the behaviour is as expected. Drives are removed and inserted
 *  back.
 *
 * @param test - parameters for configuration on which test is to
 *               executed.
 * @return FBE_STATUS_OK if no errors.
 *
 ****************************************************************/
static fbe_status_t drive_test_using_insert_remove_commands(void)
{
    fbe_device_physical_location_t      location = {0};
    fbe_u32_t                           lastSlot = 0;
    fbe_u32_t                           enclNumber=0;
    fbe_u32_t                           enclNumberLimit=0;
    fbe_esp_drive_mgmt_drive_info_t     getDriveInfo = {0};
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sas_enclosure_type_t            enclType=FBE_SAS_ENCLOSURE_TYPE_INVALID;

    mut_printf(MUT_LOG_LOW, "**************************************************");
    mut_printf(MUT_LOG_LOW, "**** Testing ESP Drive Insert/Remove (cru on/off) ");
    mut_printf(MUT_LOG_LOW, "**************************************************\n");

    // get the number of configured enclosures from the setup function
    enclNumberLimit = mixed_config_get_encl_config_list_size();

    for (enclNumber = 0; enclNumber < enclNumberLimit; enclNumber++)
    {
        // update the encl number (but bus remains constant at 0)
        location.enclosure = enclNumber;

        // need the number of slots in this enclosure
        status = fbe_api_esp_encl_mgmt_get_drive_slot_count(&location, &lastSlot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        enclType = mixed_config_get_enclosure_type(encl_config_list[enclNumber]);
        mut_printf(MUT_LOG_LOW, "Encl:%d_%d (%s) DiskSlots:%d",
                   location.bus, 
                   location.enclosure, 
                   fbe_sas_encl_type_to_string(enclType),
                   lastSlot);

        status = drive_test_find_first_drive(lastSlot, &location);

        mut_printf(MUT_LOG_LOW, "  First Drive Present %d_%d_%d, sending remove", 
                   location.bus, location.enclosure, location.slot);

        // remove the drive
        drive_test_remove_drive(&location,lastSlot);
        // allow a couple of sec for drive status to propagate through esp
        fbe_api_sleep (4000);
        // copy the drive location and get drive info to check if inserted
        status = drive_test_get_drive_info(&getDriveInfo, &location);
        // if drive state == inserted, return fail
        MUT_ASSERT_FALSE(getDriveInfo.inserted);
        mut_printf(MUT_LOG_LOW, "  %d_%d_%d is Removed, sending insert",
                   location.bus, location.enclosure, location.slot);

        // insert drive
        drive_test_insert_drive(&location,lastSlot);
        fbe_api_sleep (4000);
        // copy the drive location and get drive info to check if inserted
        status = drive_test_get_drive_info(&getDriveInfo, &location);
        // if drive state != inserted, return fail
        MUT_ASSERT_TRUE(getDriveInfo.inserted);
        mut_printf(MUT_LOG_LOW, "  %d_%d_%d is inserted", 
                   location.bus, location.enclosure, location.slot);

        // find last slot with drive present
        status = drive_test_find_last_drive(lastSlot, &location);
        mut_printf(MUT_LOG_LOW, "  Last Drive Present %d_%d_%d, sending remove...",location.bus, location.enclosure, location.slot);

        // remove the drive
        drive_test_remove_drive(&location,lastSlot);
        // allow a couple of sec for drive status to propagate through esp
        fbe_api_sleep (4000);
        // copy the drive location and get drive info to check if inserted
        status = drive_test_get_drive_info(&getDriveInfo, &location);
        // if drive state == inserted, return fail
        MUT_ASSERT_FALSE(getDriveInfo.inserted);
        mut_printf(MUT_LOG_LOW, "  %d_%d_%d is removed, sending insert", 
                   location.bus, location.enclosure, location.slot);

        // insert drive
        drive_test_insert_drive(&location,lastSlot);
        // allow a couple of sec for drive status to propagate through esp
        fbe_api_sleep (4000);        //status = fbe_test_esp_senddrive_debug_control(&location, lastSlot, FBE_DRIVE_ACTION_INSERT);
        // copy the drive location and get drive info to check if inserted

        status = drive_test_get_drive_info(&getDriveInfo, &location);
        // if drive state != inserted, return fail
        MUT_ASSERT_TRUE(getDriveInfo.inserted);
        mut_printf(MUT_LOG_LOW, "  %d_%d_%d is inserted",
                   location.bus, location.enclosure, location.slot);

    }

    return FBE_STATUS_OK;
} // drive_test_using_insert_remove_commands


/*!**************************************************************
 * drive_remove_insert_test() 
 ****************************************************************
 * @brief
 *  Main entry point for the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  22-JUL-2011 GB - Created. 
 *
 ****************************************************************/
void drive_remove_insert_test(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");
    status = drive_test_using_insert_remove_commands();
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Failed Drive Insert/Remove Test!");

    return;
}


void drive_remove_insert_test_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_MIXED_CONIG,
                                        SPID_UNKNOWN_HW_TYPE,
                                        NULL,
                                        NULL);
}

void drive_remove_insert_test_destroy(void)
{
    fbe_test_esp_common_destroy();
    return;
}

static void drive_insert_esp_data_change_callback_function(update_object_msg_t * pObjNotifyMsg, 
                                                           void *context)
{
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;

    if(target_object_id == pObjNotifyMsg->object_id)
    {
        if(pObjNotifyMsg->notification_info.notification_type == FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED)
        {
            if (pObjNotifyMsg->notification_info.notification_data.data_change_info.data_type == FBE_DEVICE_DATA_TYPE_GENERIC_INFO )
            {
                if(pObjNotifyMsg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_DRIVE)
                {
                    if((target.bus == pObjNotifyMsg->notification_info.notification_data.data_change_info.phys_location.bus) &&
                       (target.enclosure == pObjNotifyMsg->notification_info.notification_data.data_change_info.phys_location.enclosure) &&
                       (target.slot == pObjNotifyMsg->notification_info.notification_data.data_change_info.phys_location.slot))
                    {
                        // got notified, release the sem
                        mut_printf(MUT_LOG_LOW, "  Event Notification for Drive %d_%d_%d", target.bus, target.enclosure, target.slot);
                        fbe_semaphore_release(sem, 0, 1 ,FALSE);
                    }
                }
            }
        }
    }
    return;
}

/*************************
 * end file drive_remove_insert_test.c
 *************************/
