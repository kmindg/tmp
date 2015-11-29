/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file ezio_test.c
 ***************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *  11/1/2011 - Created. He Wei
 *  11/21/2011 - Add dualsp test . Gaohp 
 *  
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "sep_utils.h"
#include "fbe_database.h"
#include "pp_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe_test_common_utils.h"
#include "neit_utils.h"

/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def EZIO_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 4 encl + 60 pdo) 
 *
 *********************************************************************/
#define EZIO_TEST_NUMBER_OF_PHYSICAL_OBJECTS 66 


/*!*******************************************************************
 * @def EZIO_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define EZIO_TEST_DRIVE_CAPACITY TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY



/************************************************************************************
 * Components to support Table driven test methodology.
 * Following this approach, the parameters for the test will be stored
 * as rows in a table.
 * The test will then be executed for each row(i.e. set of parameters)
 * in the table.
 ***********************************************************************************/

/* The different test numbers.
 */
typedef enum ezio_test_enclosure_num_e
{
    EZIO_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    EZIO_TEST_ENCL2_WITH_SAS_DRIVES,
    EZIO_TEST_ENCL3_WITH_SAS_DRIVES,
    EZIO_TEST_ENCL4_WITH_SAS_DRIVES
} ezio_test_enclosure_num_t;

/* The different drive types.
 */
typedef enum ezio_test_drive_type_e
{
    SAS_DRIVE
} ezio_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    ezio_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    ezio_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;

/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        EZIO_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        EZIO_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        EZIO_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        EZIO_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

/* Count of rows in the table.
 */
#define EZIO_TEST_ENCL_MAX sizeof(encl_table)/sizeof(encl_table[0])

#define EZIO_TEST_WAIT_MSEC                 30000
char * ezio_test_short_desc = "Database service mode(degraded mode) test";
char * ezio_test_long_desc =
    "Database degraded mode test\n"
    "    - After boot, save the metadata lun object info of system db\n"
    "    - Erase the lun object info of in system db\n"
    "    - Reboot SEP\n"
    "    - The database should be in degraded mode.\n"
    "    - Restore the metadata lun object info of system db\n"
    "    - Reload database service and wait for its Ready.\n"
    ;

char * ezio_dualsp_test_short_desc = "dualsp Database service mode(degraded mode) test";
char * ezio_dualsp_test_long_desc = 
    "Dual sp Database degraded mode test"
    "   - case 1: Destroy SPA's sep, inject errors into system PDO; then SPA becomes degraded, SPB is active\n"
    "   - case 2: reboot SPB. SPB will be reboot successful. "
    "   - case 3: reload SPA's database. Then SPB should be active, SPA will be passive. "
    "   - case 4: both SPA and SPB are degraded. then reload SPA's database, next reload SPB's database. make sure all the check passed "
    ;

static fbe_status_t ezio_fail_system_drive(void);
static void ezio_reinsert_failed_drive(void);
static void ezio_test_load_physical_config(void);
static void ezio_test_sep_util_destroy_sep(void);
static void ezio_sep_config_load_sep(void);
static void ezio_sep_config_load_dual_sp_sep(void);
static void ezio_test_sp_reboot(void);

/*********************************************************************************
 * BEGIN Test, Setup, Cleanup.
 *********************************************************************************/
 
/*!**************************************************************
 * ezio_test()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/1/2011 
 *
 ****************************************************************/
void ezio_test(void)
{
    fbe_status_t status;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_database_control_system_db_op_t system_db_cmd;
    database_object_entry_t object_entry;

    mut_printf(MUT_LOG_TEST_STATUS, "Start ezio_test\n");

    //read & store the FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA object 
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_READ_OBJECT;
    system_db_cmd.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA;
    
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    fbe_copy_memory(&object_entry, system_db_cmd.cmd_data, sizeof(database_object_entry_t));

    //zero the FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_OBJECT;
    system_db_cmd.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_DELETE;

    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    //reboot
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();


    //check state if it's in mantenance
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_SERVICE_MODE)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    //restore the object
    mut_printf(MUT_LOG_TEST_STATUS, "Fixing the problem ......\n");
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_OBJECT;
    system_db_cmd.object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA;
    system_db_cmd.persist_type = FBE_DATABASE_SYSTEM_DB_PERSIST_UPDATE;
    fbe_copy_memory(system_db_cmd.cmd_data, &object_entry, sizeof(database_object_entry_t));
    
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);


    /*
     * reload database
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the SP ......\n");
    ezio_test_sp_reboot();

    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "End ezio_test\n");
}
/******************************************
 * end ezio_test()
 ******************************************/

static void ezio_test_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[EZIO_TEST_ENCL_MAX];
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
    for ( enclosure = 0; enclosure < EZIO_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < EZIO_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
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
    status = fbe_api_wait_for_number_of_objects(EZIO_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == EZIO_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End ezio_test_load_physical_config() */

/*!**************************************************************
 * ezio_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  11/1/2011 - Created. He Wei
 *
 ****************************************************************/
void ezio_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Ezio test ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Load the physical configuration */
    ezio_test_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();

    return;
}
/**************************************
 * end ezio_setup()
 **************************************/


/*!**************************************************************
 * ezio_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ezio test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/1/2011 - Created. He Wei
 *
 ****************************************************************/

void ezio_cleanup(void)
{
    /* Give sometime to let the object go away */
    EmcutilSleep(1000);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Ezio test ===\n");

//    fbe_test_sep_util_sep_neit_physical_reset_error_counters();
   
    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end ezio_cleanup()
 ******************************************/

/*!**************************************************************
 * ezio_dualsp_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  11/16/2011 - Created. 
 *
 ****************************************************************/
void ezio_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t sp;

    if (fbe_test_util_is_simulation()) {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Ezio dualsp test ===\n");

        /* set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "sp B");
        /* Load the physical configuration */
        ezio_test_load_physical_config();

        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
        /* set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);

        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n",
                   sp == FBE_SIM_SP_A ? "SP A" : "sp B");
        /* Load the physical configuration */
        ezio_test_load_physical_config();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* Load sep and neit packages */
        sep_config_load_sep_and_neit_both_sps();
    }
    fbe_test_common_util_test_setup_init();
    return;
}
/**************************************
 * end ezio_dualsp_setup()
 **************************************/


/*!**************************************************************
 * ezio_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ezio test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/16/2011 - Created. 
 *
 ****************************************************************/

void ezio_dualsp_cleanup(void)
{
    /* Clear the dual sp mode*/
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if ((fbe_test_util_is_simulation())) {
        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* In test, we failed the 3 drives of the raw mirror, and send IO to the raw mirror.
         * If the retry operation to 3 failed drives is more than one time, raid library will report trace error.
         * Here we ignore the error trace and reset the error counter
         */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();

        mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPB ===\n");
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        /* In test, we failed the 3 drives of the raw mirror, and send IO to the raw mirror.
         * If the retry operation to 3 failed drives is more than one time, raid library will report trace error.
         * Here we ignore the error trace and reset the error counter
         */
        fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
        /* Destroy the test configuration */
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end ezio_dualsp_cleanup()
 ******************************************/

/*!**************************************************************
 * ezio_dualsp_test()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  11/16/2011 
 *
 ****************************************************************/
void ezio_dualsp_test(void)
{
    fbe_sim_transport_connection_target_t active_target;
    fbe_database_state_t state = FBE_DATABASE_STATE_INVALID;
    fbe_cmi_service_get_info_t spa_cmi_info;
    fbe_cmi_service_get_info_t spb_cmi_info;
    fbe_status_t status;

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* get the active SP, Make sure SPA is active in the beginning */
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);

    /* Test start */
    mut_printf(MUT_LOG_TEST_STATUS, "---Start ezio_test---\n");
    
    /* Case 1: */
    mut_printf(MUT_LOG_TEST_STATUS, "case 1: --failed the first 3 drives of SPA, then SPA enter degraded mode, SPB becomes active \n");

    /* destory SPA and SPB's sep */
    mut_printf(MUT_LOG_TEST_STATUS, "destroy SPB's sep \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    ezio_test_sep_util_destroy_sep();
    mut_printf(MUT_LOG_TEST_STATUS, "destroy SPA's sep \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    ezio_test_sep_util_destroy_sep();

	/* Fail the first three system drives by injecting protocol errors */
    mut_printf(MUT_LOG_TEST_STATUS, "Fail the first 3 system drives of SPA \n");
    ezio_fail_system_drive();

    /* Load SPA and SPB's sep*/
    mut_printf(MUT_LOG_TEST_STATUS, "load SPA's sep \n");
    ezio_sep_config_load_sep();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "load SPB's sep \n");
    ezio_sep_config_load_sep();

	/*check the result */
    mut_printf(MUT_LOG_TEST_STATUS, "check the result \n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_SERVICE_MODE)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA's database is in degraded mode state\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPB's database is in ready state\n");

    /* Check the SP's CMI state*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_cmi_service_get_info(&spa_cmi_info);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_cmi_service_get_info(&spb_cmi_info);
    MUT_ASSERT_TRUE(spa_cmi_info.sp_state == FBE_CMI_STATE_SERVICE_MODE);
    MUT_ASSERT_TRUE(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA is in degraded mode state, and SPB is in active state\n");
    MUT_ASSERT_TRUE(spa_cmi_info.peer_alive == FBE_TRUE);
    MUT_ASSERT_TRUE(spb_cmi_info.peer_alive == FBE_FALSE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. to SPA : peer is alive. to SPB: peer is died\n");


    /* Stop protocol error injection */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We return the system drives as soon as posible. 
     * Because ESP will panic the system after 30s if all the first three DB drives are disppeared. 2013/01/16 */
    ezio_reinsert_failed_drive();

    /* case 2: reboot SPB, then check SPB state, it should be active(SPA is service mode)*/
    mut_printf(MUT_LOG_TEST_STATUS, "case 2: --reboot SPB -- \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    ezio_test_sep_util_destroy_sep();
    mut_printf(MUT_LOG_TEST_STATUS, "load SPB's sep \n");
    ezio_sep_config_load_sep();          

    /* check SPB should be active*/
    fbe_api_cmi_service_get_info(&spb_cmi_info);
    MUT_ASSERT_TRUE(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPB in active state\n");

    MUT_ASSERT_TRUE(spb_cmi_info.peer_alive == FBE_FALSE); 
    mut_printf(MUT_LOG_TEST_STATUS, "OK. to SPB: peer is died\n");


    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPB's database is in ready state\n");
    
    /* check SPA should be service mode state*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_cmi_service_get_info(&spa_cmi_info);
    MUT_ASSERT_TRUE(spa_cmi_info.sp_state == FBE_CMI_STATE_SERVICE_MODE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA is still in degraded mode state\n");
    MUT_ASSERT_TRUE(spa_cmi_info.peer_alive == FBE_TRUE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. to SPA: peer is alive\n");

    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_SERVICE_MODE)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA's database is still in degraded mode state\n");


    /* case 3: reload SPA, then SPA should become passive */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* Destroy SEP package */
	mut_printf(MUT_LOG_TEST_STATUS, "Now Destory SEP package......\n");
	/*The reason that reset the error counter is :
	    the error trace is introduced by error injection*/
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    ezio_test_sep_util_destroy_sep();
    mut_printf(MUT_LOG_TEST_STATUS, "case 3: --reboot SPA -- \n");

	ezio_sep_config_load_sep();

    fbe_api_cmi_service_get_info(&spa_cmi_info);
    MUT_ASSERT_TRUE(spa_cmi_info.sp_state == FBE_CMI_STATE_PASSIVE);
    MUT_ASSERT_TRUE(spa_cmi_info.peer_alive == FBE_TRUE);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA's database is in ready state\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_api_cmi_service_get_info(&spb_cmi_info);
    MUT_ASSERT_TRUE(spb_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA is passive, SPB is active\n");
    MUT_ASSERT_TRUE(spb_cmi_info.peer_alive == FBE_TRUE);

    /* case 4: both SPA and SPB become service mode, then reload SPA, next reload SPB */
    /* after case 3, SPB is active */
    mut_printf(MUT_LOG_TEST_STATUS, "destroy SPB's sep \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    ezio_test_sep_util_destroy_sep();
    /* inject error */
    ezio_fail_system_drive();
    fbe_test_sep_get_active_target_id(&active_target);
    MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, active_target);
    /* destroy sep */
    mut_printf(MUT_LOG_TEST_STATUS, "destroy active SP's sep \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* In test, we failed the 3 drives of the raw mirror, and send IO to the raw mirror.
     * If the retry operation to 3 failed drives is more than one time, raid library will report trace error.
     * Here we ignore the error trace and reset the error counter
     */
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    ezio_test_sep_util_destroy_sep();
    /* inject error */
    ezio_fail_system_drive();

    /* reboot SPA's sep*/
    mut_printf(MUT_LOG_TEST_STATUS, "load SPA's sep \n");
    ezio_sep_config_load_sep();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "load SPB's sep \n");
    ezio_sep_config_load_sep();

    mut_printf(MUT_LOG_TEST_STATUS, "check the result \n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
   
    /* SPA's datagbase should be service mode*/
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_SERVICE_MODE)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA's database is in degraded mode state\n");
    /* SPA: service mode. */
    fbe_api_cmi_service_get_info(&spa_cmi_info);
    MUT_ASSERT_TRUE(spa_cmi_info.sp_state == FBE_CMI_STATE_SERVICE_MODE);
    /* To SPA: peer is alive*/
    MUT_ASSERT_TRUE(spa_cmi_info.peer_alive == FBE_TRUE);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    /* SPB's database should be service mode*/
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_SERVICE_MODE)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPB's database is also in degraded mode state\n");
    /* SPB: service mode. peer died*/
    fbe_api_cmi_service_get_info(&spb_cmi_info);
    MUT_ASSERT_TRUE(spb_cmi_info.sp_state == FBE_CMI_STATE_SERVICE_MODE);
    /* to SPB: now peer is died*/
    MUT_ASSERT_TRUE(spb_cmi_info.peer_alive == FBE_FALSE);


	/* reinsert failed drive */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	/* destroy SPA's SEP package*/
	mut_printf(MUT_LOG_TEST_STATUS, "Prepare reinsert drives on SPA......\n");
	mut_printf(MUT_LOG_TEST_STATUS, "Now Destory SEP package......\n");
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    ezio_test_sep_util_destroy_sep();
	mut_printf(MUT_LOG_TEST_STATUS, "Now reinsert failed drives......\n");
	status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	ezio_reinsert_failed_drive();
    /* reboot SPA */
    ezio_sep_config_load_sep();    
    fbe_api_cmi_service_get_info(&spa_cmi_info);
    MUT_ASSERT_TRUE(spa_cmi_info.sp_state == FBE_CMI_STATE_ACTIVE);
    /* after reload SPA, SPB is still degraded. To SPA: peer is died */
    MUT_ASSERT_TRUE(spa_cmi_info.peer_alive == FBE_FALSE);
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA's database is in ready state\n");


	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	mut_printf(MUT_LOG_TEST_STATUS, "Prepare reinsert drives on SPB......\n");
	mut_printf(MUT_LOG_TEST_STATUS, "Now Destory SEP package......\n");
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    ezio_test_sep_util_destroy_sep();
	mut_printf(MUT_LOG_TEST_STATUS, "Now reinsert failed drives......\n");
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    ezio_reinsert_failed_drive();
    /* reboot SPB*/
    ezio_sep_config_load_sep();

    fbe_api_cmi_service_get_info(&spb_cmi_info);
    MUT_ASSERT_TRUE(spb_cmi_info.sp_state == FBE_CMI_STATE_PASSIVE);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPA is active, SPB is passive\n");
    /* to SPB: now peer is alive */
    MUT_ASSERT_TRUE(spb_cmi_info.peer_alive == FBE_TRUE);
    /* check SPA's database state*/
    status = fbe_api_database_get_state(&state);
    if(status == FBE_STATUS_OK && state != FBE_DATABASE_STATE_READY)
        status = FBE_STATUS_GENERIC_FAILURE;
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "OK. SPB's database is in ready state\n");

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_api_cmi_service_get_info(&spa_cmi_info);
    /* to SPA: now SPB is also alive */
    MUT_ASSERT_TRUE(spa_cmi_info.peer_alive == FBE_TRUE);

    /* end of tests */
    /* Clear the dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
}



static fbe_status_t ezio_fail_system_drive(void)
{
    fbe_object_id_t                                     object_id[4];
    fbe_status_t                                        status;
    fbe_u32_t                                           i;
    fbe_protocol_error_injection_error_record_t         record = {0};
    fbe_protocol_error_injection_record_handle_t        out_record_handle_p[4];
    fbe_private_space_layout_region_t region_info;
    /* We insert error into the Homewrecker region, which can trigger the degraded mode by homewrecker more quickly(2013/01/16) */
    fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_HOMEWRECKER_DB, &region_info);

    fbe_test_neit_utils_init_error_injection_record(&record);
    record.lba_start                = region_info.starting_block_address;
    record.lba_end                  = region_info.starting_block_address + region_info.size_in_blocks;
    record.num_of_times_to_insert   = FBE_U32_MAX;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_READ;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = FBE_SCSI_READ_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[2] = FBE_SCSI_READ_12;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[3] = FBE_SCSI_READ_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_HARDWARE_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code =   FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    for (i = 0; i < PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++) {
        if (i <= (record.lba_end - record.lba_start)) {
            record.b_active[i] = FBE_TRUE;
        }
    } 
    record.frequency = 0;
    record.b_test_for_media_error = FBE_FALSE;
    record.reassign_count = 0;

    for (i = 0; i < 3; i++) {
        status = fbe_api_get_physical_drive_object_id_by_location(0, 0, i, &object_id[i]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        record.object_id = object_id[i];
        status = fbe_api_protocol_error_injection_add_record(&record, &out_record_handle_p[i]);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    }

    status = fbe_api_protocol_error_injection_start();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}

static void ezio_reinsert_failed_drive(void)
{
    fbe_status_t        status;
    fbe_object_id_t     pdo_object_id;
    fbe_u32_t           i;
    fbe_api_terminator_device_handle_t drive_handle;

    for (i = 0; i < 3; i++) {
        status = fbe_api_get_physical_drive_object_id_by_location(0,
                                                                  0,
                                                                  i,
                                                                  &pdo_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);
        status = fbe_test_pp_util_pull_drive(0, 
                                             0, 
                                             i,
                                             &drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        status = fbe_api_wait_till_object_is_destroyed(pdo_object_id,FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_pp_util_reinsert_drive(0, 
                                                 0, 
                                                 i,
                                                 drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
       
        status = fbe_test_pp_util_verify_pdo_state(0, 
                                                   0,
                                                   i,
                                                   FBE_LIFECYCLE_STATE_READY,
                                                   60000);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}

static void ezio_test_sep_util_destroy_sep(void)
{

    fbe_test_package_list_t package_list;
    fbe_zero_memory(&package_list, sizeof(package_list)); 
	
    mut_printf(MUT_LOG_LOW, "=== %s starts ===", __FUNCTION__);

    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_SEP_0;

    fbe_test_common_util_package_destroy(&package_list);
    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);

    mut_printf(MUT_LOG_LOW, "=== %s completes ===", __FUNCTION__);

    return;
}

static void ezio_sep_config_load_sep(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* sep_config_load_sep_setup_package_entries */
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);

    fbe_test_sep_util_enable_pkg_trace_limits(FBE_PACKAGE_ID_SEP_0);
    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_io_flags();
}

static void ezio_sep_config_load_dual_sp_sep(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_LOW, "%s === starting Storage Extent Package(SEP) ===", __FUNCTION__);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* sep_config_load_sep_setup_package_entries */
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete", __FUNCTION__);
}


void ezio_test_sp_reboot(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the SP ......\n");
    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    ezio_test_sep_util_destroy_sep();
    ezio_sep_config_load_sep();    
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the SP Done ......\n");
}

/*************************
 * end file ezio_test.c
 *************************/


