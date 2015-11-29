/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file presto_test.c
 ***************************************************************************
 *
 * @brief
 *  This test verifies the limits handling of enclosures
 *
 * @version
 *   02/15/2013 - Created. Brion Philbin
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
#include "fbe_test_configurations.h"
#include "fbe_private_space_layout.h"
#include "pp_utils.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * presto_short_desc = "Enclosure limits handling ";
char * presto_long_desc ="\
This tests validates the enclosure management object handles the enclosure limits. \n\
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

static fbe_semaphore_t presto_sem;
static fbe_object_id_t presto_expected_objectId;
static fbe_u64_t  presto_expected_device_mask = FBE_DEVICE_TYPE_INVALID;
static fbe_device_physical_location_t presto_expected_phys_location = {0};
static void presto_test_add_overlimit_enclosures(void);

 /*!**************************************************************
 *  presto_esp_object_data_change_callback_function() 
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
presto_esp_object_data_change_callback_function(update_object_msg_t * update_object_msg, 
                                                    void *context)
{
    fbe_semaphore_t *sem = (fbe_semaphore_t *)context;
    fbe_u64_t device_mask;
    
    device_mask = update_object_msg->notification_info.notification_data.data_change_info.device_mask;
    if (update_object_msg->object_id == presto_expected_objectId && 
        (device_mask & presto_expected_device_mask) == presto_expected_device_mask) 
    {
        if(presto_expected_phys_location.bus == update_object_msg->notification_info.notification_data.data_change_info.phys_location.bus &&
           presto_expected_phys_location.enclosure == update_object_msg->notification_info.notification_data.data_change_info.phys_location.enclosure)
        {
            mut_printf(MUT_LOG_LOW, "presto: Get the notification for encl(%d_%d) ...\n", 
                       presto_expected_phys_location.bus,
                       presto_expected_phys_location.enclosure);
            fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
    }
   
    return;
}


/*!**************************************************************
 * presto_test() 
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
void presto_test(void)
{
    fbe_esp_encl_mgmt_encl_info_t encl_info;
    fbe_device_physical_location_t location;
    fbe_u32_t bus, encl;
    fbe_u32_t status;
    fbe_u32_t max_encl;
    fbe_api_terminator_device_handle_t	port_handle;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_api_terminator_device_handle_t	enclosure_handle;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_registration_id_t reg_id;
    DWORD           dwWaitResult;

    presto_test_add_overlimit_enclosures();

    fbe_api_sleep(3000);

    /* Wait util there is no firmware upgrade in progress. */
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);

    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");

    /* Wait util there is no resume prom read in progress. */
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);

    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    mut_printf(MUT_LOG_LOW, "presto test: Started...\n");



    for (bus = 0; bus < 1; bus ++)
    {
        if(bus==0)
        {
            max_encl = 8;
        }
        for ( encl = 0; encl < max_encl; encl++ )
        {
            mut_printf(MUT_LOG_LOW, "Getting Encl Info for Bus:%d Encl: %d...\n", bus, encl);
            location.bus = bus;
            location.enclosure = encl;
    
            status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            if( (bus == 0) && (encl == 7) )
            {
                MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_FAILED, encl_info.enclState, 
                                     "enclState does not match\n");
                MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX, encl_info.enclFaultSymptom, 
                                         "enclFaultSymptom does not match\n");
            }
            else
            {
                MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_TYPE_VIPER, encl_info.encl_type, "Enclosure Type does not match\n");
                MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_OK, encl_info.enclState, 
                                     "enclState does not match\n");
                MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                                         "enclFaultSymptom does not match\n");
            }
        }
    }

     /* Init the Semaphore */
    fbe_semaphore_init(&presto_sem, 0, 1);

    /* Register to get event notification from ESP */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_ESP,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT,
                                                                  presto_esp_object_data_change_callback_function,
                                                                  &presto_sem,
                                                                  &reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_id != FBE_OBJECT_ID_INVALID);
    presto_expected_objectId = object_id;
    presto_expected_device_mask = FBE_DEVICE_TYPE_ENCLOSURE;


    /* Remove an enclosure */
    location.bus = 0;
    location.enclosure = 7;
    presto_expected_phys_location.bus = location.bus;
    presto_expected_phys_location.enclosure = location.enclosure;
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &enclosure_handle);
    fbe_api_terminator_remove_sas_enclosure(enclosure_handle);

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure 0_7 to be removed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&presto_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    //fbe_api_sleep(10000);
    
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_MISSING, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom does not match\n");
    mut_printf(MUT_LOG_LOW, "enclState and enclFaultSymptom check passed.\n");

    /* Remove an enclosure */
    location.bus = 0;
    location.enclosure = 6;
    presto_expected_phys_location.bus = location.bus;
    presto_expected_phys_location.enclosure = location.enclosure;
    status = fbe_api_terminator_get_enclosure_handle(location.bus, location.enclosure, &enclosure_handle);
    fbe_api_terminator_remove_sas_enclosure(enclosure_handle);

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure 0_6 to be removed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&presto_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    //fbe_api_sleep(15000);
    
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_MISSING, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom does not match\n");
    mut_printf(MUT_LOG_LOW, "enclState and enclFaultSymptom check passed.\n");

    /* Re-insert 0_6 */
    fbe_api_terminator_get_port_handle(0, &port_handle);
    sas_encl.backend_number = 0;
    sas_encl.encl_number = 6;
    sas_encl.uid[0] = 6; // some unique ID.
    sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &enclosure_handle);

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure 0_6 to be re-inserted...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&presto_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    //fbe_api_sleep(10000);
    
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_OK, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_NO_FAULT, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom does not match\n");
    mut_printf(MUT_LOG_LOW, "enclState and enclFaultSymptom check passed.\n");

    sas_encl.backend_number = 0;
    sas_encl.encl_number = 7;
    sas_encl.uid[0] = 6; // some unique ID.
    sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
    sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
    status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &enclosure_handle);

    fbe_api_sleep(3000); // we are getting notifications for a while after adding an enclosure, let this settle out

    mut_printf(MUT_LOG_LOW, "Waiting for enclosure 0_7 to be re-inserted and failed...\n");
    dwWaitResult = fbe_semaphore_wait_ms(&presto_sem, 20000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    //fbe_api_sleep(10000);

    location.bus = 0;
    location.enclosure = 7;
    
    status = fbe_api_esp_encl_mgmt_get_encl_info(&location, &encl_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_STATE_FAILED, encl_info.enclState, 
                             "enclState does not match\n");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX, encl_info.enclFaultSymptom, 
                             "enclFaultSymptom does not match\n");
    mut_printf(MUT_LOG_LOW, "enclState and enclFaultSymptom check passed.\n");



    mut_printf(MUT_LOG_LOW, "presto test passed...\n");

    /* Unregistered the notification*/
    status = fbe_api_notification_interface_unregister_notification(presto_esp_object_data_change_callback_function,
                                                                    reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_semaphore_destroy(&presto_sem);

    return;
}
/******************************************
 * end presto_test()
 ******************************************/

void presto_setup(void)
{
    fbe_test_startEsp_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_LAPA_RIOS_CONIG,
                                    SPID_OBERON_1_HW_TYPE,
                                    NULL,
                                    NULL);
}

void presto_destroy(void)
{
    fbe_test_esp_common_destroy();
    fbe_test_esp_delete_registry_file();
    return;
}


void presto_test_add_overlimit_enclosures(void)
{
    fbe_api_terminator_device_handle_t	port_handle;
    fbe_u32_t no_of_ports;
    fbe_u32_t no_of_encls;
    fbe_u32_t slot;
    fbe_terminator_sas_encl_info_t	sas_encl;
    fbe_api_terminator_device_handle_t	encl_handle;
    fbe_api_terminator_device_handle_t	drive_handle;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t max_encls = 0;

    for (no_of_ports = 0; no_of_ports < LAPA_RIO_MAX_PORTS; no_of_ports ++)
    {
        fbe_api_terminator_get_port_handle(no_of_ports, &port_handle);

        if(no_of_ports == 0)
        {
            max_encls = 9;
        }
        else
        {
            max_encls = 8;
        }
    
        for ( no_of_encls = LAPA_RIO_MAX_ENCLS; no_of_encls < max_encls; no_of_encls++ )
        {
            /*insert an enclosure to port 0*/
            sas_encl.backend_number = no_of_ports;
            sas_encl.encl_number = no_of_encls;
            sas_encl.uid[0] = no_of_encls; // some unique ID.
            sas_encl.sas_address = 0x5000097A79000000 + ((fbe_u64_t)(sas_encl.encl_number) << 16);
            sas_encl.encl_type = FBE_SAS_ENCLOSURE_TYPE_VIPER;
            status  = fbe_api_terminator_insert_sas_enclosure (port_handle, &sas_encl, &encl_handle);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
           for (slot = 0; slot < LAPA_RIO_MAX_DRIVES; slot ++)
           {   
               if (((no_of_ports < 2) && (no_of_encls < 2) && (slot < 15))||
                   ((no_of_ports < 2) && (no_of_encls == 2) && (slot == 4))||
                   ((no_of_ports < 2) && (no_of_encls == 2) && (slot == 6))||
                   ((no_of_ports < 2) && (no_of_encls == 2) && (slot == 10)))
               {
                    status  = fbe_test_pp_util_insert_sas_drive(no_of_ports,
                                                                no_of_encls,
                                                                slot,
                                                                520,
                                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                                &drive_handle);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
               }
           }
           fbe_api_sleep(3000); // let the insert of each enclosure be processed individually.
        }
    }

}
/*************************
 * end file presto_test.c
 *************************/


