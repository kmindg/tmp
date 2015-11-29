/*!**************************************************************************
 * @file snake_ica_test.c
 ***************************************************************************
 *
 * @brief
 *  Test relative operations after set ICA flags.
 *
 * @version
 *  21/2/2013 - Created. Song Yingying
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_trace_interface.h"


/*!*******************************************************************
 * @def SNAKE_ICA_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SNAKE_ICA_TEST_NS_TIMEOUT        120000 /*wait maximum of 120 seconds*/
static void snake_ica_load_physical_config(void);
void snake_ica_test1(void);
void snake_ica_test2(void);

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION

char * snake_ica_test_short_desc = 
    "Test operation after set ICA flags and pull drive 0_0_1 and then reboot SP";
char * snake_ica_test_long_desc=    
    "\n"
    "\n"
    "The snake_ica_test tests operation after set ICA flags and pull drive 0_0_1 and then reboot SP.\n"
    "\n"
    "Dependencies:\n"
    "\n"
    "Starting Config:\n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "\n"
    "STEP 2: Set ICA flags of 0_0_0, 0_0_1, 0_0_2\n"
    "\n"
    "STEP 3: Pull the drive 0_0_1\n" 
    "\n"
    "STEP 4: Reboot the SP\n"
    "\n"
    "STEP 5: Reinsert the drive 0_0_1\n"
    "\n"
    "STEP 6: Reboot the SP\n"
    "\n";

/*!**************************************************************
 * snake_ica_setup()
 ****************************************************************
 * @brief
 * This is the setup function for the snake_ica_test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/8/2011 - Created. Song Yingying
 *
 ****************************************************************/
void snake_ica_setup(void)
{   
    mut_printf(MUT_LOG_LOW, "=== snake_ica_setup starts ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Load the physical configuration */
    snake_ica_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    mut_printf(MUT_LOG_LOW, "=== snake_ica_setup completes ===\n");
}
/**************************************
 * end snake_ica_setup()
 **************************************/



/*!**************************************************************
 * snake_ica_load_physical_config()
 ****************************************************************/
static void snake_ica_load_physical_config(void)
{
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[1];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < 1; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < 1; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
           fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY, &drive_handle);
            
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    /*1  board, 1 port, 1 encl, 15 pdo = 18*/
    status = fbe_api_wait_for_number_of_objects(18, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == 18);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");
    return;
}
/**************************************
 * end snake_ica_load_physical_config()
 **************************************/



void snake_ica_test_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== snake_ica_test_cleanup starts ===\n");	
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_sep_util_destroy_neit_sep_physical();
   
    mut_printf(MUT_LOG_LOW, "=== snake_ica_test_cleanup completes ===\n");
    return;
}


/*!****************************************************************************
 *  snake_ica_test1
 ******************************************************************************
 *
 * @brief
 *   This is the steps for the snake ica test1.  The test does the 
 *   following:
 *
 *   Step1. Set ICA flags of 0_0_0, 0_0_1, 0_0_2. 
 *   Step2. Pull the drive 0_0_1.
 *   Step3. Reboot SP.
 *   Step4. Reinsert the drive 0_0_1.
 *   Step5. Reboot SP.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void snake_ica_test1(void)
{
   fbe_api_terminator_device_handle_t drive_handle;
   fbe_status_t    status;
   fbe_database_state_t   state;

   /*Set ICA flags of 0_0_0, 0_0_1, 0_0_2*/
   mut_printf(MUT_LOG_TEST_STATUS, "Set ICA flags ......\n");
   status = fbe_api_database_set_invalidate_config_flag();
   MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

   /*Pull the drive 0_0_1*/
   mut_printf(MUT_LOG_TEST_STATUS, "Pull the drive 0_0_1 ......\n");
   status = fbe_test_pp_util_pull_drive(0,0,1,&drive_handle);
   MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

   /*Reboot the SP*/
   mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
   fbe_test_sep_util_destroy_neit_sep();
   sep_config_load_sep_and_neit(); 

   /*Check if the system is in service mode*/
   status = fbe_api_database_get_state(&state);
   MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK,status,"snake_ica_test: fail to get db state of SP.");
   MUT_ASSERT_INT_EQUAL_MSG(FBE_DATABASE_STATE_SERVICE_MODE,state,"snake_ica_test: SP's state is not in service mode.");
   mut_printf(MUT_LOG_TEST_STATUS, "The system is in service mode now......\n");

   /*Check the service mode reason*/
   if (state == FBE_DATABASE_STATE_SERVICE_MODE) {

        fbe_database_control_get_service_mode_reason_t ctrl_reason;

        status = fbe_api_database_get_service_mode_reason(&ctrl_reason);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(ctrl_reason.reason, FBE_DATABASE_SERVICE_MODE_REASON_NOT_ALL_DRIVE_SET_ICA_FLAGS);
        mut_printf(MUT_LOG_TEST_STATUS,"Database degraded reason is : %s\n", ctrl_reason.reason_str);
        
    }
  
    /*Reinsert the drive 0_0_1*/
    mut_printf(MUT_LOG_TEST_STATUS, "Reinsert the drive 0_0_1 ......\n");
    status = fbe_test_pp_util_reinsert_drive(0, 0, 1, drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

   /*Reboot the SP again*/
   mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
   fbe_test_sep_util_destroy_neit_sep();
   sep_config_load_sep_and_neit();
   status = fbe_test_sep_util_wait_for_database_service(SNAKE_ICA_TEST_NS_TIMEOUT);
   MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/**************************************
 * end snake_ica_test1()
 **************************************/


/*!****************************************************************************
 *  snake_ica_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry for the snake ica test.  The test does the 
 *   following:
 *   
 *   Step1. snake_ica_test1.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void snake_ica_test(void)
{
    snake_ica_test1();
}
/**************************************
 * end snake_ica_test()
 **************************************/
