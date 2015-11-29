
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file pokemon_test.c
 ***************************************************************************
 *
 * @brief
 *   This file exercises the job service synchronization.
 *
 * @version
 * 
 * @author
 *   10/26/2011 - Created. Arun S 
 *
 ***************************************************************************/


/*-----------------------------------------------------------------------------
    INCLUDES OF REQUIRED HEADER FILES:
*/

#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "physical_package_tests.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"

/*-----------------------------------------------------------------------------
    TEST DESCRIPTION:
*/

char* pokemon_short_desc = "Job service synchronization";
char* pokemon_long_desc =
    "\n"
    "\n"
    "The Pokemon scenario tests synchronization of jobs between Active and Passive.\n"
    "\n"
    "\n"
    "*** Pokemon **************************************************************\n"
    "\n"    
    "Dependencies:\n"
    "    - Job Control Service (JCS) API solidified\n"
    "    - JCS creation queue operational\n"
    "    - Database (DB) Service able to persist database both locally and on the peer\n"
    "    - Ability to destroy RG/LUN using fbe_api\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada board\n"
    "    [PP] 1 - SAS PMC port\n"
    "    [PP] 2 - viper enclosures\n"
    "    [PP] 30 - SAS drives (PDOs)\n"
    "    [PP] 30 - Logical drives (LDs)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config on both the SPs.\n"
    "\n" 
    "STEP 2: Bring-up Active SP (SPA) and issue LUN creation.\n"
    "    - Let the Active SP come up. Do not start Passive.\n"
    "    - Disable the Job Queue processing.\n"
    "    - Issue LUN creation request via JOB_SERVICE.\n"
    "        - Verify the LUN requests are queued up and not processed.\n"
    "\n" 
    "STEP 3: Bring-up Passive SP (SPB) and verify the job sync. \n"
    "    - Let the Passive SP come up.\n"
    "    - Verify it triggers the SYNC operation on Passive.\n"
    "    - Verify all the jobs from Active are sync'd up with Passive.\n"
    "    - Release the Queue and Semaphores.\n"
    "\n"
    "STEP 4: Verify the jobs are processed. \n"
    "    - Once the jobs are sync'd, release the Queues on Active side.\n"
    "    - Verify the jobs are processed in order.\n"
    "\n"
    "\n"
    "STEP 5: Destroy raid groups and LUNs.\n"
    "\n"
    "Description last updated: 10/27/2011.\n"
    "\n"
    "\n";
    
/*!*******************************************************************
 * @def POKEMON_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define POKEMON_TEST_RAID_GROUP_COUNT           1

/*!*******************************************************************
 * @def POKEMON_TEST_LUN_COUNT
 *********************************************************************
 * @brief  number of luns to be created
 *
 *********************************************************************/
#define POKEMON_TEST_LUN_COUNT        2

/*!*******************************************************************
 * @def POKEMON_TEST_STARTING_LUN
 *********************************************************************
 * @brief  Starting lun number
 *
 *********************************************************************/
#define POKEMON_TEST_STARTING_LUN       0xF

/*!*******************************************************************
 * @def POKEMON_TEST_MAX_RG_WIDTH
 *********************************************************************
 * @brief Max number of drives in a RG
 *
 *********************************************************************/
#define POKEMON_TEST_MAX_RG_WIDTH               16


/*!*******************************************************************
 * @def POKEMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 2 encl + 30 pdo) 
 *
 *********************************************************************/
#define POKEMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS 34 


/*!*******************************************************************
 * @def POKEMON_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define POKEMON_TEST_DRIVE_CAPACITY             TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def POKEMON_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define POKEMON_TEST_NS_TIMEOUT        30000 /*wait maximum of 30 seconds*/

/*!*******************************************************************
 * @def POKEMON_TEST_RAID_GROUP_ID
 *********************************************************************
 * @brief RAID Group id used by this test
 *
 *********************************************************************/
#define POKEMON_TEST_RAID_GROUP_ID          0

static fbe_test_rg_configuration_t pokemon_rg_configuration[] = 
{
    /* width,                                   capacity,                       raid type,             class,         block size, RAID-id, bandwidth.*/
    {SEP_REBUILD_UTILS_R5_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*-----------------------------------------------------------------------------
    FORWARD DECLARATIONS:
-----------------------------------------------------------------------------*/

static void pokemon_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void *context_p);
static void pokemon_test_load_physical_config(void);
static void pokemon_test_create_rg(fbe_test_rg_configuration_t *rg_config_p);
fbe_status_t pokemon_disable_creation_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t pokemon_enable_creation_queue_processing(fbe_object_id_t in_object_id);
fbe_status_t fbe_pokemon_build_control_element(fbe_job_queue_element_t *job_queue_element, 
                                            fbe_job_service_control_t control_code, 
                                            fbe_object_id_t object_id, 
                                            fbe_u8_t *command_data_p, 
                                            fbe_u32_t command_size);

static void pokemon_test_create_lun_with_job(void);

/*!****************************************************************************
 *  pokemon_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Pokemon test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 *****************************************************************************/
void pokemon_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &pokemon_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, pokemon_run_test);

    return;
}

/*!****************************************************************************
 *  pokemon_run_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Pokemon test.  
 *
 * @param   IN: rg_config_ptr - pointer to the RG configuration.
 *
 * @return  None 
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 *****************************************************************************/
static void pokemon_run_test(fbe_test_rg_configuration_t *rg_config_ptr, void *context_p)
{
    fbe_u32_t                       num_raid_groups = 0;
    fbe_u32_t                       index = 0;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t     *rg_config_p = NULL;

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Pokemon Test ===\n");

    for(index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Create RG */
            pokemon_test_create_rg(rg_config_p);
        }
    }

    pokemon_disable_creation_queue_processing(FBE_OBJECT_ID_INVALID);

    /* Create LUN's */
    pokemon_test_create_lun_with_job();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    pokemon_enable_creation_queue_processing(FBE_OBJECT_ID_INVALID);

    /* Unbind all user LUNs..  */
    status = fbe_test_sep_util_destroy_all_user_luns();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Destroy RGs..  */
    status = fbe_test_sep_util_destroy_raid_group(rg_config_p->raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}

/*!**************************************************************
 * pokemon_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Pokemon test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 ****************************************************************/
static void pokemon_test_load_physical_config(void)
{
    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      encl0_0_handle;
    fbe_api_terminator_device_handle_t      encl0_1_handle;
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port0_handle);

    /* Insert enclosures to port 0 */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);

    /* Insert drives for enclosures. */
    for (slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, POKEMON_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for (slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, POKEMON_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(POKEMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects. */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == POKEMON_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End pokemon_test_load_physical_config() */


/*!****************************************************************************
 *  pokemon_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the Pokemon test in dualsp mode. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 *****************************************************************************/
void pokemon_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Pokemon dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        pokemon_test_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        pokemon_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit();
        fbe_test_wait_for_all_pvds_ready();
    }

    return;

} /* End pokemon_setup() */


/*!****************************************************************************
 *  pokemon_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Pokemon test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 *****************************************************************************/
void pokemon_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Pokemon test ===\n");

    /* Destroy the test configuration on both SPs */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
} /* End pokemon_dualsp_cleanup() */

/*!****************************************************************************
 *  pokemon_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the dual SP test for the Pokemon test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 *****************************************************************************/
void pokemon_dualsp_test(void)
{
    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now run the create tests */
    pokemon_test();

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
}

/*!**************************************************************
 * pokemon_test_create_rg()
 ****************************************************************
 * @brief
 *  Configure the Raid Group for Peep test.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 ****************************************************************/
static void pokemon_test_create_rg(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_job_service_error_type_t                job_error_code;
    fbe_status_t                                job_status;

    /* Initialize local variables */
    rg_object_id = FBE_OBJECT_ID_INVALID;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== create a RAID Group ===\n");
    status = fbe_test_sep_util_create_raid_group(rg_config_p);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

    /* wait for notification from job service. */
    status = fbe_api_common_wait_for_job(rg_config_p->job_number,
                                         POKEMON_TEST_NS_TIMEOUT,
                                         &job_error_code,
                                         &job_status,
                                         NULL);

    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");

    /* Verify the object id of the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(
             rg_config_p->raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Verify the raid group get to ready state in reasonable time */
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                     READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_SEP_0);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group: %d\n", rg_config_p->raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End pokemon_test_create_rg() */

/*!**************************************************************
 * pokemon_disable_creation_queue_processing
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to disable
 *  recovery queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 10/26/2011 - Created. Arun S.
 * 
 ****************************************************************/
fbe_status_t pokemon_disable_creation_queue_processing(fbe_object_id_t in_object_id)
{
    return fbe_test_sep_util_disable_creation_queue(in_object_id);
}
/*******************************************************
 * end pokemon_disable_creation_queue_processing()
 ******************************************************/

/*!**************************************************************
 * pokemon_enable_creation_queue_processing
 ****************************************************************
 * @brief
 *  This function sends a request to the Job service to enable
 *  recovery queue processing
 * 
 * @param object_id  - the ID of the target object
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 10/26/2011 - Created. Arun S.
 * 
 ****************************************************************/
fbe_status_t pokemon_enable_creation_queue_processing(fbe_object_id_t in_object_id)
{
   return fbe_test_sep_util_enable_creation_queue(in_object_id);
}
/******************************************************
 * end pokemon_enable_creation_queue_processing()
 *****************************************************/

/*!**************************************************************
 * fbe_pokemon_build_control_element()
 ****************************************************************
 * @brief
 *  This function builds a test control element 
 * 
 * @param job_queue_element - element to build 
 * @param job_control_code    - code to send to job service
 * @param object_id           - the ID of the target object
 * @param command_data_p      - pointer to client data
 * @param command_size        - size of client's data
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 * 10/26/2011 - Created. Arun S
 * 
 ****************************************************************/
fbe_status_t fbe_pokemon_build_control_element(
        fbe_job_queue_element_t        *job_queue_element,
        fbe_job_service_control_t      control_code,
        fbe_object_id_t                object_id,
        fbe_u8_t                       *command_data_p,
        fbe_u32_t                      command_size)
{
    /* Context size cannot be greater than job element context size. */
    if (command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    job_queue_element->object_id = object_id;

    job_queue_element->job_type = FBE_JOB_TYPE_INVALID;
    job_queue_element->timestamp = 0;
    job_queue_element->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    job_queue_element->current_state  = FBE_JOB_ACTION_STATE_INVALID;
    job_queue_element->queue_access = FBE_FALSE;
    job_queue_element->num_objects = 0;

    if (command_data_p != NULL)
    {
        fbe_copy_memory(job_queue_element->command_data, command_data_p, command_size);
    }

    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_pokemon_build_control_element()
 ***********************************************/


/*!****************************************************************************
 *  pokemon_test_create_lun
 ******************************************************************************
 *
 * @brief
 *  This function creates LUNs for Pokemon test.  
 *
 * @param   IN: is_ndb - Non-destructive flag.
 *
 * @return  None 
 *
 * @author
 *   10/26/2011 - Created. Arun S 
 *****************************************************************************/
static void pokemon_test_create_lun_with_job()
{
    fbe_status_t   status;
    fbe_u32_t      lun_index = 0;
    fbe_api_job_service_lun_create_t fbe_lun_create_req;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Creating LUNs ===\n");

    for (lun_index = 0; lun_index < 5; lun_index++)
    {
        fbe_lun_create_req.raid_group_id = POKEMON_TEST_RAID_GROUP_ID;
        fbe_lun_create_req.lun_number = POKEMON_TEST_STARTING_LUN + lun_index;
        fbe_lun_create_req.capacity = 0x2000;
        fbe_lun_create_req.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
        fbe_lun_create_req.ndb_b = FBE_FALSE;
        fbe_lun_create_req.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
        fbe_lun_create_req.wait_ready = FBE_FALSE;
        fbe_lun_create_req.ready_timeout_msec = READY_STATE_WAIT_TIME;
        fbe_lun_create_req.is_system_lun = FBE_FALSE;
        status = fbe_api_job_service_lun_create(&fbe_lun_create_req);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "Create LUN: %d sent\n", fbe_lun_create_req.lun_number);
    }
  
    return;

}/* End pokemon_test() */


