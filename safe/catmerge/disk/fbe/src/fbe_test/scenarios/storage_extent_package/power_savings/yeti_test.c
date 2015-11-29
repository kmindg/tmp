/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file yeti_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains disk zeroing tests with hibernation of the PVD
 *   object.
 *
 * @version
 *   05/10/2010 - Created. Amit Dhaduk
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   TEST DESCRIPTION
 *************************/
char * yeti_short_desc = "PVD not getting into hibernation while disk zeroing";
char * yeti_long_desc =
    "\n"
    "The Yeti scenario tests if disk zeroing is in progress then PVD should not getting into hibernation. Once disk  \n"
    " zeroing over, it PVD should hibernate after its configured idle time.\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada boards\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 SAS drives (PDOs)\n"
    "    [PP] 1 logical drives (LDs)\n"
    "    [SEP] 1 provision drives (PVDs)\n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Enable system wide power save\n"
    "\n"
    "STEP 3: Set PVD idle time to hibernate 10 seconds\n" 
    "\n"
    "STEP 4: Start disk zeroing\n" 
    "\n"
    "STEP 5: Enable PVD object for power save\n"
    "\n"
    "STEP 6: While disk zeroing is in progress\n"
    "    - check that PVD object lifecycle state is not in hibernate\n"
    "    - check that PVD object state is not power saving\n"
    "\n"
    "STEP 7: After disk zeroing completed:\n"
    "    - wait for configured idle time\n"
    "    - check that PVD object lifecycle state is in hibernate\n"
    "    - check that PVD object state is power saving\n"
    "Last update: 12/15/11\n"
    "\n"    ;
   


/*!*******************************************************************
 * @def YETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 2 port + 1 encl + 15 pdo) 
 *********************************************************************/
#define YETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS 19

/*!*******************************************************************
 * @def YETI_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/
#define YETI_TEST_DRIVE_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def YETI_TEST_MAX_RETRIES_WHILE_ZEROING
 *********************************************************************
 * @brief  max retry count to verify disk zeroing completed 
 * (number of times to loop)
 *
 *********************************************************************/
#define YETI_TEST_MAX_RETRIES_WHILE_ZEROING     800      


/*!*******************************************************************
 * @def YETI_TEST_MAX_RETRIES_AFTER_ZEROING
 *********************************************************************
 * @brief  max retry count to verify power save state after disk zeroing completed 
 * (number of times to loop)
 *
 *********************************************************************/
#define YETI_TEST_MAX_RETRIES_AFTER_ZEROING     30      


/*!*******************************************************************
 * @def YETI_TEST_RETRY_TIME
 *********************************************************************
 * @brief  wait time in ms, in between retries 
 *
 *********************************************************************/
#define YETI_TEST_RETRY_TIME                    250     



/*!*******************************************************************
 * @def YETI_PVD_IDLE_TIME
 *********************************************************************
 * @brief  PVD idle time for this test 
 *
 *********************************************************************/
#define YETI_PVD_IDLE_TIME 10  /*seconds */


static fbe_object_id_t yeti_pvd_id = FBE_OBJECT_ID_INVALID;

static fbe_object_id_t          expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_lifecycle_state_t    expected_state = FBE_LIFECYCLE_STATE_INVALID;
static fbe_semaphore_t          sem;
static fbe_notification_registration_id_t          reg_id;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
void yeti_build_sep_config(void);
static void yeti_physical_config(void);
static void yeti_power_save_init(void);
static void yeti_verify_power_save_state_while_zeroing(void);
static void yeti_verify_power_save_state_after_zeroing(void);
static void yeti_verify_power_save_state(void);
static void yeti_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context);


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/



/*!**************************************************************
 * yeti_physical_config()
 ****************************************************************
 * @brief
 *  Configure the yeti test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 *
 ****************************************************************/

static void yeti_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  port1_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle;
    fbe_terminator_api_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_object_id_t object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port0_handle);

    fbe_test_pp_util_insert_sas_pmc_port(2, /* io port */
                                         2, /* portal */
                                         1, /* backend number */ 
                                         &port1_handle);

    /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, YETI_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(YETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == YETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    for (object_id = 0 ; object_id < YETI_TEST_NUMBER_OF_PHYSICAL_OBJECTS; object_id++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/******************************************
 * end yeti_physical_config()
 ******************************************/


/*!****************************************************************************
 *  yeti_power_save_test
 ******************************************************************************
 *
 * @brief
 *   This function initialize power save configuration and starts disk zeroing on PVD.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void yeti_power_save_init(void)
{

    fbe_system_power_saving_info_t              power_save_info;
    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t                                status;

    /* verify the pvd get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(yeti_pvd_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* start disk zeroing */
    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing (be patient, it may take a minute or two)===\n");    
    status = fbe_api_provision_drive_initiate_disk_zeroing(yeti_pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for 1 monitor cycle to complete */
    fbe_api_sleep(3500);

    /* enable system wide power save */
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* make sure it worked*/
    power_save_info.enabled = FBE_FALSE;
    status  = fbe_api_get_system_power_save_info(&power_save_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(power_save_info.enabled == FBE_TRUE);

    /* set PVD object idle time */
    status = fbe_api_set_object_power_save_idle_time(yeti_pvd_id, YETI_PVD_IDLE_TIME);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enable the power saving on the PVD*/
    status = fbe_api_enable_object_power_save(yeti_pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* check it's enabled*/
    status = fbe_api_get_object_power_save_info(yeti_pvd_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);

    return;

}
/***************************************************************
 * end yeti_power_save_test()
 ***************************************************************/


/*!**************************************************************************
 *  yeti_verify_power_save_state_while_zeroing
 ***************************************************************************
 * @brief
 *   This function checks that PVD should not get into hibernate while disk 
 *   zeroing in progress.
 * 
 *
 * @param   None
 *
 * @return  None 
 *
 ***************************************************************************/
static void yeti_verify_power_save_state_while_zeroing(void)
{

    fbe_status_t                                status;          //  fbe status
    fbe_u32_t                                   count;           //  loop count
    fbe_u32_t                                   max_retries;     //  maximum number of retries
    fbe_base_config_object_power_save_info_t    object_ps_info;  // object power save info
    fbe_lba_t                                   zero_checkpoint; // disk zero checkpoint   
    fbe_lifecycle_state_t                       object_state;    // PVD object state

    /*  Set the max retries in a local variable. 
     *  (This allows it to be changed within the debugger if needed.)
     */
    max_retries = YETI_TEST_MAX_RETRIES_WHILE_ZEROING;

    /*  Loop until the disk zeroing completed */
    for (count = 0; count < max_retries; count++)
    {

        /* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(yeti_pvd_id, &zero_checkpoint);        
        

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return ;
        }

        /* check if disk zeroing completed */
        if (zero_checkpoint == FBE_LBA_INVALID)
        {
            /* disk zeroing comleted */
            mut_printf(MUT_LOG_LOW, "==   disk zeroing completed   == ");           
            return ;
        }

        /* disk zeroing running
         * check that PVD should not be in hibernate state while disk zeroing running
         */

        /*  Get the PVD lifecycle state */
        status = fbe_api_get_object_lifecycle_state(yeti_pvd_id, &object_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);

        /*  Get the PVD power save info */
        status = fbe_api_get_object_power_save_info(yeti_pvd_id, &object_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);
        MUT_ASSERT_INT_NOT_EQUAL(object_ps_info.power_save_state, FBE_POWER_SAVE_STATE_SAVING_POWER);
           
        /*  Wait before trying again  */
        fbe_api_sleep(YETI_TEST_RETRY_TIME);
    }

    /*  power save state was not set in the the time limit - assert */
    MUT_ASSERT_TRUE(0);

    /*  Return (should never get here)  */
    return;

}   

/*!****************************************************************************
 *  yeti_power_save_test
 ******************************************************************************
 *
 * @brief
 *   This function checks that PVD should be in hibernate after configured idle time.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void yeti_verify_power_save_state_after_zeroing(void)
{

    fbe_base_config_object_power_save_info_t    object_ps_info;
    fbe_status_t                                status;
    fbe_lifecycle_state_t                       object_state;
    DWORD                                       dwWaitResult;

    mut_printf(MUT_LOG_LOW, "=== Yeti verify power save after zeroing ===\n");  

    fbe_semaphore_init(&sem, 0, 1);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                    FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                    FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                    FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                    FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                    FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                    yeti_commmand_response_callback_function,
                                                                    &sem,
                                                                    &reg_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    expected_object_id = yeti_pvd_id;/*we want to get notification from this drive*/
    expected_state = FBE_LIFECYCLE_STATE_HIBERNATE;

    /* wait about 50 seconds from the time disk zero completed(PVD idle time 10 seconds)
     * so unbound PVDs will get into hibernation state 
     */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 50000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);

    /*check it's enabled*/
    status = fbe_api_get_object_power_save_info(yeti_pvd_id, &object_ps_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(object_ps_info.power_saving_enabled, FBE_TRUE);
    mut_printf(MUT_LOG_LOW, "=== Yeti verify power save - state enabled  ===\n");   

    status = fbe_api_get_object_lifecycle_state(yeti_pvd_id, &object_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_HIBERNATE, object_state);
    mut_printf(MUT_LOG_LOW, "=== Yeti verify power save - life cycle state set to hibernet  ===\n");    

    /* Once PVD object lifecycle state set to hibernet, it tooks some msec time to set power saving state
     * hence need to check PVD power save state in wait loop 
     */
    yeti_verify_power_save_state();

    /*we want to clean and go out w/o leaving the power saving persisted on the disks for next test*/
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system wide power saving \n", __FUNCTION__);
    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    status = fbe_api_notification_interface_unregister_notification(yeti_commmand_response_callback_function, reg_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, " %s: Waiting for objects to get out of hibernation \n", __FUNCTION__);
    fbe_thread_delay(20000);/*let all drive get back to ready before we start destroyng the system*/

    fbe_semaphore_destroy(&sem);

    return;

}
/***************************************************************
 * end yeti_power_save_test()
 ***************************************************************/

/*!**************************************************************************
 *  yeti_commmand_response_callback_function
 ***************************************************************************
 * @brief
 *   This function handles callback response for object lifecycle state notification.
 * 
 *
 * @param   None
 *
 * @return  None 
 *
 ***************************************************************************/
static void yeti_commmand_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t     *sem = (fbe_semaphore_t *)context;
	fbe_lifecycle_state_t	state;

	fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);

    if (update_object_msg->object_id == expected_object_id && expected_state == state) {
        fbe_semaphore_release(sem, 0, 1 ,FALSE);
    }
}
/***************************************************************
 * end yeti_commmand_response_callback_function()
 ***************************************************************/


/*!**************************************************************************
 *  yeti_verify_power_save_state
 ***************************************************************************
 * @brief
 *   This function checks PVD's power save state in wait loop. 
 * 
 *
 * @param   None
 *
 * @return  None 
 *
 ***************************************************************************/
static void yeti_verify_power_save_state(void)
{

    fbe_status_t                        status;                         //  fbe status
    fbe_u32_t                           count;                          //  loop count
    fbe_u32_t                           max_retries;                    //  maximum number of retries
    fbe_base_config_object_power_save_info_t    object_ps_info;


    /*  Set the max retries in a local variable. 
     *  (This allows it to be changed within the debugger if needed.)
     */
    max_retries = YETI_TEST_MAX_RETRIES_AFTER_ZEROING;

        
    /*  Loop until the PVD object state set to power save */
    for (count = 0; count < max_retries; count++)
    {
        /* get the PVD power save info */
        status = fbe_api_get_object_power_save_info(yeti_pvd_id, &object_ps_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*  Check if PVD object state  */
        if (object_ps_info.power_save_state == FBE_POWER_SAVE_STATE_SAVING_POWER)
        {
            mut_printf(MUT_LOG_LOW, "=== PVD object state is saving power  ===\n"); 
            return;
        }
        /*  Wait before trying again */
        fbe_api_sleep(YETI_TEST_RETRY_TIME);
    }

    /*  power save state was not set in the the time limit - assert */
    MUT_ASSERT_TRUE(0);

    /*  Return (should never get here)  */
    return;

}   
/***************************************************************
 * end yeti_verify_power_save_state()
 ***************************************************************/
/*!****************************************************************************
 *  yeti_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the yeti test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void yeti_test(void)
{
    fbe_status_t                                status;
    fbe_test_raid_group_disk_set_t            * disk_set = NULL;
    fbe_test_discovered_drive_locations_t       drive_locations;   // available drive location details
    fbe_test_block_size_t                       block_size;
    fbe_drive_type_t                            drive_type;
    fbe_u32_t                                   index;
    fbe_bool_t                                  disk_found = FBE_FALSE;

    mut_printf(MUT_LOG_LOW, "=== Yeti test started ===\n"); 

    /* fill out the available disk locations
     */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    for(block_size =0; (block_size < FBE_TEST_BLOCK_SIZE_LAST) && (disk_found == FBE_FALSE); block_size++)
    {
        for (drive_type = 0; (drive_type < FBE_DRIVE_TYPE_LAST) && (disk_found == FBE_FALSE); drive_type++)
        {
            for (index = 1; index <= drive_locations.drive_counts[block_size][drive_type]; index++)
            {
                /* Fill in the next drive.
                 */
                status = fbe_test_sep_util_get_next_disk_in_set((drive_locations.drive_counts[block_size][drive_type]-index), 
                                                                drive_locations.disk_set[block_size][drive_type],
                                                                &disk_set);
                if (status != FBE_STATUS_OK)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "Yeti_test - can't get next disk set so terminate the test\n");
                    return;
                }

                status = fbe_api_provision_drive_get_obj_id_by_location( disk_set->bus,
                                                                         disk_set->enclosure,
                                                                         disk_set->slot,
                                                                         &yeti_pvd_id
                                                                        );
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                disk_found = FBE_TRUE;
                break;
            }
        }
    }
    
    if(disk_found == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Yeti_test - do not have available disk to run the test\n");
        return;
    }
    
    /* initialize power saving configuration and start disk zeroing */
    yeti_power_save_init();

    /* check that PVD should not be in hibernate state while disk zeroing */
    yeti_verify_power_save_state_while_zeroing();

    /* check that PVD should be in hibernate state after configured idle time */
    yeti_verify_power_save_state_after_zeroing();

    mut_printf(MUT_LOG_LOW, "=== Yeti test completed ===\n");   

    return;
}
/***************************************************************
 * end yeti_test()
 ***************************************************************/
/*!****************************************************************************
 *  yeti_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the yeti test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void yeti_setup(void)
{    

    //fbe_database_transaction_id_t transaction_id;    

    mut_printf(MUT_LOG_LOW, "=== yeti setup ===\n");        

    /* Only load config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        /* load the physical package */
        yeti_physical_config();

        /* load the logical package */
        sep_config_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/***************************************************************
 * end yeti_setup()
 ***************************************************************/

/*!****************************************************************************
 *  yeti_cleanup
 ******************************************************************************
 *
 * @brief
 *   This function unloads the logical and physical packages and destroy the configuration.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void yeti_cleanup(void)
{
    /* Only destroy config in simulation  
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "=== yeti destroy starts ===\n");
        fbe_test_sep_util_destroy_neit_sep_physical();
        mut_printf(MUT_LOG_LOW, "=== yeti destroy completes ===\n");
    }
    return;
}
/***************************************************************
 * end yeti_cleanup()
 ***************************************************************/


/*************************
 * end file yeti_test.c
 *************************/

