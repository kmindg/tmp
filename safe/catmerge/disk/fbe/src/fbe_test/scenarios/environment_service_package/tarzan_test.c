/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file tarzan_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions to test the voyager enclosure removal and insertion.
 *
 * @version
 *   26-Mar-2013 PHE - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "esp_tests.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_utils.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * tarzan_short_desc = "Test the voyager enclosure addition and removal.";
char * tarzan_long_desc =
    "\n"
    "\n"
    "The tarzan test scenario tests the voyager enclosure addition and removal.\n"
    "It includes: \n"
    "    - Test the voyager enclosure addition;\n"
    "    - Test the voyager enclosure removal;\n"
    "\n"
    "Dependencies:\n"
    "    - The terminator should be able to simulate the voyager enclosure LCC removal (not available yet).\n"
    "\n"
    "Starting Config(Naxos Config):\n"
    "    [PP] armada board \n"
    "    [PP] 1 SAS PMC port\n"
    "    [PP] 3 voyager enclosures\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create the initial physical package config.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Start the ESP.\n"
    "    - Verify that all ESP objects are ready.\n"
    "    - Wait until all the upgrade initiated at power up completes.\n"
    "\n"  
    "STEP 2: Verify the enclosure attributes.\n"
    "    - Verify the drive slot count for the all enclosures.\n"
    "    - Verify the subEnclosure count for all the enclosures.\n"
    "\n"
    "STEP 3: Test the voyager enclosure removal.\n"
    "    - Remove the voyager enclosure 0_2.\n"
    "    - Verify the voyager enclosure 0_2 is not present.\n"
    "\n"
    "STEP 4: Shutdown the Terminator/Physical package/ESP.\n"
    ;

static fbe_semaphore_t tarzan_sem;
static fbe_object_id_t tarzan_expected_objectId;
static fbe_u64_t  tarzan_expected_device_mask = FBE_DEVICE_TYPE_INVALID;
static fbe_device_physical_location_t tarzan_expected_phys_location = {0};
static fbe_bool_t tarzan_expected_encl_present;

 /*!**************************************************************
 *  tarzan_esp_object_data_change_callback_function() 
 ****************************************************************
 * @brief
 *  Notification callback function for data change
 *
 * @param update_object_msg: Object message
 * @param context: Callback context.               
 *
 * @return None.
 *
 * @author
 *  26-Mar-2013 PHE - Created.
 *
 ****************************************************************/
static void 
tarzan_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                    void *context)
{
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t device_mask;
    fbe_bool_t enclPresent = FBE_TRUE;
    fbe_status_t status;

    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    if (update_object_msg->object_id == tarzan_expected_objectId && 
        (device_mask & tarzan_expected_device_mask) == tarzan_expected_device_mask) 
    {
        if(tarzan_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
           tarzan_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure)
        {
            mut_printf(MUT_LOG_LOW, "Get the notification for encl(%d_%d) ...\n", 
                       tarzan_expected_phys_location.bus,
                       tarzan_expected_phys_location.enclosure);

            mut_printf(MUT_LOG_LOW, "Getting Encl presence for Bus:%d Encl: %d, expected enclPresent %d...\n", 
                       tarzan_expected_phys_location.bus,
                       tarzan_expected_phys_location.enclosure,
                       tarzan_expected_encl_present);
            status = fbe_api_esp_encl_mgmt_get_encl_presence(&tarzan_expected_phys_location, &enclPresent);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            if(enclPresent == tarzan_expected_encl_present) 
            {
                tarzan_expected_encl_present = !tarzan_expected_encl_present;
                fbe_semaphore_release(sem, 0, 1 ,FALSE);
            }
        }
    }
   
    return;
}


/*!**************************************************************
 * tarzan_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  26-Mar-2013 PHE - Created.
 *
 ****************************************************************/
void tarzan_test(void)
{
    fbe_esp_encl_mgmt_encl_info_t encl_info;
    fbe_device_physical_location_t location;
    fbe_u32_t bus, encl;
    fbe_status_t status;
    fbe_api_terminator_device_handle_t enclosure_handle;
    fbe_api_terminator_device_handle_t port_handle = NULL;
    fbe_u32_t object_handle;
    fbe_u8_t  display_char;
    fbe_bool_t enclPresent = FBE_FALSE;

    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_registration_id_t reg_id;
    DWORD           dwWaitResult;

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    mut_printf(MUT_LOG_LOW, "tarzan test: Started...\n");


    for (bus = 0; bus < 1; bus ++)
    {
        for ( encl = 0; encl < 2; encl++ )
        {
            location.bus = bus;
            location.enclosure = encl;

            mut_printf(MUT_LOG_LOW, "Getting Encl presence for Bus:%d Encl: %d...\n", bus, encl);
            status = fbe_api_esp_encl_mgmt_get_encl_presence(&location, &enclPresent);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, enclPresent, "enclPresent does not match\n");

            mut_printf(MUT_LOG_LOW, "Getting Encl Info for Bus:%d Encl: %d...\n", bus, encl);
            
    
            status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_TYPE_VOYAGER, encl_info.encl_type, "Enclosure Type does not match\n");
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_OK, encl_info.enclState, 
                                     "enclState does not match\n");
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                                     "enclFaultSymptom does not match\n");
            MUT_ASSERT_INT_EQUAL_MSG(2, encl_info.subEnclCount, 
                                     "subEnclCount does not match\n");
            MUT_ASSERT_INT_EQUAL_MSG(60, encl_info.driveSlotCount, 
                                     "driveSlotCount does not match\n");

            if(!encl_info.processorEncl)
            {
                /*
                 * Check that the enclosure display LEDs are set properly
                 */
                status = fbe_api_get_enclosure_object_id_by_location(bus, encl, &object_handle);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    
                display_char = 0x30;
                status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                                        display_char, FBE_ENCL_DISPLAY, 0, 5000);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                display_char = 0x30+bus;
                status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                                        display_char, FBE_ENCL_DISPLAY, 1, 5000);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                display_char = 0x30+encl;
                status = fbe_api_wait_for_encl_attr(object_handle, FBE_DISPLAY_CHARACTER_STATUS, 
                                                        display_char, FBE_ENCL_DISPLAY, 2, 5000);
                MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                
            }

        }
    }

    /* Init the Semaphore */
    fbe_semaphore_init(&tarzan_sem, 0, 1);

    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  tarzan_esp_object_data_change_callback_function,
                                                                  &tarzan_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    tarzan_expected_objectId = object_id;
    tarzan_expected_device_mask = FBE_DEVICE_TYPE_ENCLOSURE;
   
    /* TO DO: Remove the LCC of the Voyager enclosure. Need the terminator support. */

    /* Remove an enclosure */
    location.bus = 0;
    location.enclosure = 1;
    tarzan_expected_phys_location.bus = location.bus;
    tarzan_expected_phys_location.enclosure = location.enclosure;
    tarzan_expected_encl_present = FBE_FALSE;
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &enclosure_handle);
    fbe_api_terminator_pull_enclosure(enclosure_handle);

    mut_printf(MUT_LOG_LOW, "=== Waiting for enclosure 0_1 to be removed ====\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tarzan_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
    mut_printf(MUT_LOG_LOW, "Getting Encl presence for Bus:%d Encl: %d...\n", location.bus, location.enclosure);
    status = fbe_api_esp_encl_mgmt_get_encl_presence(&location, &enclPresent);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, enclPresent, "enclPresent does not match\n");

    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_MISSING, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom does not match\n");
    mut_printf(MUT_LOG_LOW, "enclPresent, enclState and enclFaultSymptom check passed.\n");

    /* Re-insert the enclosure */
    location.bus = 0;
    location.enclosure = 1;
    tarzan_expected_phys_location.bus = location.bus;
    tarzan_expected_phys_location.enclosure = location.enclosure;
    tarzan_expected_encl_present = FBE_TRUE;

    status = fbe_api_terminator_get_port_handle(0, &port_handle);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to get port handle!");

    status = fbe_api_terminator_reinsert_enclosure(port_handle, enclosure_handle);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to re-insert the enclosure!");

    mut_printf(MUT_LOG_LOW, "=== Waiting for enclosure 0_1 to be re-inserted ===\n");
    dwWaitResult = fbe_semaphore_wait_ms(&tarzan_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
    mut_printf(MUT_LOG_LOW, "Getting Encl presence for Bus:%d Encl: %d...\n", location.bus, location.enclosure);
    status = fbe_api_esp_encl_mgmt_get_encl_presence(&location, &enclPresent);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, enclPresent, "enclPresent does not match\n");

    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_OK, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(2, encl_info.subEnclCount, 
                             "subEnclCount does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(60, encl_info.driveSlotCount, 
                             "driveSlotCount does not match\n");
    mut_printf(MUT_LOG_LOW, "The check passed.\n");

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(tarzan_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&tarzan_sem);

    return;
}


void tarzan_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                        FBE_ESP_TEST_NAXOS_VOYAGER_CONIG,
                                        SPID_OBERON_3_HW_TYPE,
                                        NULL,
                                        NULL);
}

void tarzan_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== Deleting the registry file ===");
    fbe_test_esp_delete_esp_lun();
    fbe_test_esp_common_destroy();
    return;
}


/*************************
 * end file tarzan_test.c
 *************************/
