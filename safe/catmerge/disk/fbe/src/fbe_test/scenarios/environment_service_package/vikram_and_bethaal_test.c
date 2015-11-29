/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file vikram_and_bethaal_test.c
 ***************************************************************************
 *
 * @brief
 *  This file detects the presence of enclosure and handle this
 *  appropriately
 *
 * @version
 *   05/16/2010 - Created. Ashok Tamilarasan
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
#include "fbe_encl_mgmt_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe_test_esp_utils.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * vikram_and_bethaal_short_desc = "Adding, Removal and Fail of Enclosure ";
char * vikram_and_bethaal_long_desc ="\
This tests validates the enclosure management object handle Addition, Removal and Fail of enclosure during run time. \n\
This test also validates the enclosure's bus/encl display when the enclosures are added.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
 STEP 2: Validate the starting of Enclosure Management Object\n\
        - Verify that enclosure Management Object is started\n\
          by checking the lifecycle state\n\
 STEP 3: Validate that enclosure object gets the data from the physical package\n\
        - Verify that Enclosure Management Object sees 3 Viper Enclosures\n\
        - Issue FBE API from the test code to enclosure management object to \n\
          validate that data(type, serial number, location) stored in the \n\
          enclosure management object is correct";

static fbe_semaphore_t vikram_and_bethaal_sem;
static fbe_object_id_t vikram_and_bethaal_expected_objectId;
static fbe_u64_t  vikram_and_bethaal_expected_device_mask = FBE_DEVICE_TYPE_INVALID;
static fbe_device_physical_location_t vikram_and_bethaal_expected_phys_location = {0};

 /*!**************************************************************
 *  vikram_and_bethaal_esp_object_data_change_callback_function() 
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
 *  05-Mar-2012 PHE - Created.
 *
 ****************************************************************/
static void 
vikram_and_bethaal_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                    void *context)
{
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t device_mask;
    
    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    if (update_object_msg->object_id == vikram_and_bethaal_expected_objectId && 
        (device_mask & vikram_and_bethaal_expected_device_mask) == vikram_and_bethaal_expected_device_mask) 
    {
        if(vikram_and_bethaal_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
           vikram_and_bethaal_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure)
        {
            mut_printf(MUT_LOG_LOW, "vikram_and_bethaal: Get the notification for encl(%d_%d) ...\n", 
                       vikram_and_bethaal_expected_phys_location.bus,
                       vikram_and_bethaal_expected_phys_location.enclosure);
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
   
    return;
}


/*!**************************************************************
 * vikram_and_bethaal_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  05/16/2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
void vikram_and_bethaal_test(void)
{
    fbe_esp_encl_mgmt_encl_info_t encl_info;
    fbe_device_physical_location_t location;
    fbe_u32_t bus, encl;
    fbe_u32_t status;
    fbe_api_terminator_device_handle_t enclosure_handle;
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

    mut_printf(MUT_LOG_LOW, "vikram_and_bethaal test: Started...\n");


    for (bus = 0; bus < CHAUTAUQUA_MAX_PORTS; bus ++)
    {
        for ( encl = 0; encl < CHAUTAUQUA_MAX_ENCLS; encl++ )
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
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_TYPE_VIPER, encl_info.encl_type, "Enclosure Type does not match\n");
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_OK, encl_info.enclState, 
                                     "enclState does not match\n");
            MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                                     "enclFaultSymptom does not match\n");

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
    fbe_semaphore_init(&vikram_and_bethaal_sem, 0, 1);

    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  vikram_and_bethaal_esp_object_data_change_callback_function,
                                                                  &vikram_and_bethaal_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    vikram_and_bethaal_expected_objectId = object_id;
    vikram_and_bethaal_expected_device_mask = FBE_DEVICE_TYPE_ENCLOSURE;


    /* Remove an enclosure */
    location.bus = 0;
    location.enclosure = 7;
    vikram_and_bethaal_expected_phys_location.bus = location.bus;
    vikram_and_bethaal_expected_phys_location.enclosure = location.enclosure;
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &enclosure_handle);
    fbe_api_terminator_remove_sas_enclosure(enclosure_handle);

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure 0_7 to be removed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&vikram_and_bethaal_sem, 20000);
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

    /* Set an enclosure to fail state
     */
    location.bus = 0;
    location.enclosure = 6;
    vikram_and_bethaal_expected_phys_location.bus = location.bus;
    vikram_and_bethaal_expected_phys_location.enclosure = location.enclosure;
    status = fbe_api_get_enclosure_object_id_by_location(location.bus, location.enclosure, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);
    
    status = fbe_api_set_object_to_state(object_handle, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Unable to fail enclosure!");

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure 0_6 to fail...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&vikram_and_bethaal_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_FAILED, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom not match\n");
    mut_printf(MUT_LOG_LOW, "enclState and enclFaultSymptom check passed.\n");

    mut_printf(MUT_LOG_LOW, "Vikram_and_bethaal test passed...\n");

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(vikram_and_bethaal_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&vikram_and_bethaal_sem);

    return;
}
/******************************************
 * end vikram_and_bethaal_test()
 ******************************************/

void vikram_and_bethaal_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                    SPID_PROMETHEUS_M1_HW_TYPE,
                                    NULL,
                                    NULL);
}

void vikram_and_bethaal_destroy(void)
{
    fbe_test_sep_util_destroy_neit_sep_physical();
    fbe_test_esp_delete_registry_file();
    return;
}
/*************************
 * end file vikram_and_bethaal_test.c
 *************************/

