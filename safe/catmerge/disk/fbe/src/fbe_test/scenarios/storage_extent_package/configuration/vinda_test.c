/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file vinda_test.c
 ***************************************************************************
 *
 * @brief
 *  
 *
 * @version
 *  1/21/2015 - Created. Hongpo Gao
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
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_test_common_utils.h"
#include "neit_utils.h"

#define READY_STATE_WAIT_TIME       20000
void vinda_test_reboot_sp(void);
fbe_status_t vinda_test_get_number_of_elements_in_job_creation_queue(fbe_u32_t *num_of_creation_jobs);
fbe_status_t vinda_test_generate_update_db_on_peer_job(void);


/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def VINDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 2 encl + 30 pdo) 
 *
 *********************************************************************/
#define VINDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS  34 

/*!*******************************************************************
 * @def VINDA_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define VINDA_TEST_DRIVE_CAPACITY TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY

char* vinda_dualsp_short_desc = "Vinda scenario tests the update DB on peer job. ";
char* vinda_dualsp_long_desc =
    "\n"
    "\n"
    "The Vinda scenario tests the update DB on peer job .\n"
    "the update DB on peer job will be abort when peer SP is not alive \n"
    "\n";


/*!**************************************************************
 * vinda_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Vinda test physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 ****************************************************************/
static void vinda_test_load_physical_config(void)
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
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, VINDA_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for (slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, VINDA_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(VINDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS, READY_STATE_WAIT_TIME, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == VINDA_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} /* End vinda_test_load_physical_config() */

/*!****************************************************************************
 *  vinda_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *  This is the setup function for the vinda test in dualsp mode. It is responsible
 *  for loading the physical and logical configuration for this test suite on both SPs.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *****************************************************************************/
void vinda_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Vinda dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        vinda_test_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        vinda_test_load_physical_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        /* Load sep and neit packages */
        sep_config_load_sep_and_neit_both_sps();
        fbe_test_sep_util_wait_for_database_service(20000);
        fbe_test_wait_for_all_pvds_ready();
    }
    fbe_test_common_util_test_setup_init();

    return;
}        
    
/*!****************************************************************************
 *  vinda_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *  This is the cleanup function for the Vinda test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *****************************************************************************/
void vinda_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_TEST_STATUS, "=== Cleanup for Vinda test ===\n");

    /* Destroy the test configuration on both SPs */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep_physical();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
} /* End vinda_dualsp_cleanup() */

/*!****************************************************************************
 *  vinda_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   shutdown peer SP. Generate a job of update_db_on_peer 
 *   verify the job will be rollback because peer SP is not alive.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *****************************************************************************/
void vinda_test_generate_and_verify_update_db_on_peer_job(void)
{
    fbe_status_t status;
    fbe_job_service_debug_hook_t            job_hook;

    //shutdown SPB then check if the job will be executed.
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_job_service_add_debug_hook(FBE_OBJECT_ID_INVALID,
                                                FBE_JOB_TYPE_UPDATE_DB_ON_PEER,
                                                FBE_JOB_ACTION_STATE_ROLLBACK,
                                                FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START,
                                                FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);

    status = vinda_test_generate_update_db_on_peer_job();

    /* Boot up SPB again*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(30000);
    
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_job_service_get_debug_hook(FBE_OBJECT_ID_INVALID,                     
                                                FBE_JOB_TYPE_UPDATE_DB_ON_PEER,
                                                FBE_JOB_ACTION_STATE_ROLLBACK,
                                                FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START,
                                                FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG,
                                                &job_hook);
                                                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(job_hook.hit_counter == 1);
    
    return;
}

/*!****************************************************************************
 *  vinda_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the dual SP test for the Vinda test.
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *****************************************************************************/
void vinda_dualsp_test(void)
{
    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    vinda_test_generate_and_verify_update_db_on_peer_job();

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}

void vinda_test_reboot_sp(void)
{
    fbe_status_t status;

    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(30000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

fbe_status_t vinda_test_get_number_of_elements_in_job_creation_queue(fbe_u32_t *num_of_creation_jobs)
{
    fbe_job_queue_element_t               job_queue_element;
    fbe_status_t                          status = FBE_STATUS_OK;

    /* Build job_queue_element */
    fbe_zero_memory(&job_queue_element, sizeof(fbe_job_queue_element_t));

    status = fbe_api_job_service_process_command(&job_queue_element,
            FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_CREATION_QUEUE,
            FBE_PACKET_FLAG_NO_ATTRIB);

    if (num_of_creation_jobs != NULL)
        *num_of_creation_jobs = job_queue_element.num_objects;

    return status;
}

fbe_status_t vinda_test_generate_update_db_on_peer_job(void)
{
    fbe_job_service_update_db_on_peer_t     update_db_on_peer;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info = {0};

    fbe_zero_memory(&update_db_on_peer, sizeof(update_db_on_peer));
    status = fbe_api_common_send_control_packet_to_service (FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER,
                                                            &update_db_on_peer,
                                                            sizeof(fbe_job_service_update_db_on_peer_t),
                                                            FBE_SERVICE_ID_JOB_SERVICE,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                status,
                status_info.packet_qualifier, 
                status_info.control_operation_status, 
                status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
