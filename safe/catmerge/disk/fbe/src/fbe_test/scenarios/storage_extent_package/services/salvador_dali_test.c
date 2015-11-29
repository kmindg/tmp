/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file salvador_dali_test.c
 ***************************************************************************
 *
 * @brief
 * 
 * 
 * @version
 *  2011-10-31 Matthew Ferson - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_cms_interface.h"

/*************************
 *    DEFINITIONS
 *************************/

char * salvador_dali_short_desc = "Memory persistence test.";
char * salvador_dali_long_desc = "\
The Salvador Dali test validates memory persistence in a simulation environment.\n\
\n\
STEP 1:  Load the physical configuration and SEP/NEIT packages.\n\
STEP 2:  Validate that memory was not persisted, and is not requested.\n\
STEP 3:  Request memory persistence via CMS.\n\
STEP 4:  Simulate a reboot by unloading and reloading SEP/NEIT.\n\
STEP 5:  Validate that memory was successfully persisted\n\
STEP 6:  Simulate a reboot where memory persistence failed\n\
STEP 7:  Validate that memory was not persisted\n\
STEP 8:  Simulate a reboot where BIOS reported success, but memory was actually lost\n\
STEP 9:  Validate that memory was not persisted\n\
STEP 10: Destroy the configuration.\n\
\n\
Description last updated: 2011-11-04.\n\
";


/*!*******************************************************************
 * @def SALVADOR_DALI_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive
 *
 *********************************************************************/
#define SALVADOR_DALI_TEST_DRIVE_CAPACITY TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY


/*!*******************************************************************
 * @def SALVADOR_DALI_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects to create
 *        (1 board + 1 port + 2 encl + 30 pdo) 
 *
 *********************************************************************/
#define SALVADOR_DALI_TEST_NUMBER_OF_PHYSICAL_OBJECTS 34 


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void salvador_dali_test_load_physical_config(void);
static void salvador_dali_run_test(void);


/*!**************************************************************
 *  salvador_dali_test()
 ****************************************************************
 * @brief
 *  Run a series of I/O tests to a raw mirror.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 10/2011   Created.   Susan Rundbaken
 *
 ****************************************************************/
void salvador_dali_test(void)
{
    fbe_status_t                    status                          = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                      terminator_persistence_request  = FBE_FALSE;
    fbe_cms_memory_persist_status_t cms_persistence_status;
    fbe_cms_memory_persist_status_t terminator_persistence_status;

    mut_printf(MUT_LOG_HIGH, "%s Checking Initial Terminator Persistence Status", __FUNCTION__);
    status = fbe_api_terminator_persistent_memory_get_persistence_status(&terminator_persistence_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(terminator_persistence_status, FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT);

    mut_printf(MUT_LOG_HIGH, "%s Checking Initial CMS Persistence Status", __FUNCTION__);
    status = fbe_api_cms_service_get_persistence_status(&cms_persistence_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(cms_persistence_status, FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL);

    mut_printf(MUT_LOG_HIGH, "%s Requesting Memory Persistence via CMS", __FUNCTION__);
    status = fbe_api_cms_service_request_persistence(FBE_CMS_CLIENT_INTERNAL, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s Checking Terminator Persistence Request", __FUNCTION__);
    status = fbe_api_terminator_persistent_memory_get_persistence_request(&terminator_persistence_request);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(terminator_persistence_request);

    //DbgBreakPoint();

    mut_printf(MUT_LOG_HIGH, "*** %s Simulating a successful persistence", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "%s Simulating a reboot", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "%s Unloading SEP and NEIT", __FUNCTION__);
    fbe_test_sep_util_destroy_neit_sep();

    mut_printf(MUT_LOG_HIGH, "%s Loading SEP and NEIT", __FUNCTION__);
    sep_config_load_sep_and_neit();

    mut_printf(MUT_LOG_HIGH, "%s Checking Terminator Persistence Status after reboot", __FUNCTION__);
    status = fbe_api_terminator_persistent_memory_get_persistence_status(&terminator_persistence_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(terminator_persistence_status, FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL);

    mut_printf(MUT_LOG_HIGH, "%s Checking CMS Persistence Status after reboot", __FUNCTION__);
    status = fbe_api_cms_service_get_persistence_status(&cms_persistence_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(cms_persistence_status, FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL);

    mut_printf(MUT_LOG_HIGH, "*** %s Simulating a failed persistence", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "%s Requesting Memory Persistence via CMS", __FUNCTION__);
    status = fbe_api_cms_service_request_persistence(FBE_CMS_CLIENT_INTERNAL, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s Simulating a reboot", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "%s Unloading SEP and NEIT", __FUNCTION__);
    fbe_test_sep_util_destroy_neit_sep();

    status = fbe_api_terminator_persistent_memory_set_persistence_status(FBE_CMS_MEMORY_PERSIST_STATUS_POWER_FAIL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s Loading SEP and NEIT", __FUNCTION__);
    sep_config_load_sep_and_neit();

    mut_printf(MUT_LOG_HIGH, "%s Checking CMS Persistence Status after reboot", __FUNCTION__);
    status = fbe_api_cms_service_get_persistence_status(&cms_persistence_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(cms_persistence_status, FBE_CMS_MEMORY_PERSIST_STATUS_POWER_FAIL);

    mut_printf(MUT_LOG_HIGH, "*** %s Simulating corrupted / moved memory", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "%s Requesting Memory Persistence via CMS", __FUNCTION__);
    status = fbe_api_cms_service_request_persistence(FBE_CMS_CLIENT_INTERNAL, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s Simulating a reboot", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "%s Unloading SEP and NEIT", __FUNCTION__);
    fbe_test_sep_util_destroy_neit_sep();

    status = fbe_api_terminator_persistent_memory_zero_memory();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_HIGH, "%s Loading SEP and NEIT", __FUNCTION__);
    sep_config_load_sep_and_neit();

    mut_printf(MUT_LOG_HIGH, "%s Checking CMS Persistence Status after reboot", __FUNCTION__);
    status = fbe_api_cms_service_get_persistence_status(&cms_persistence_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(cms_persistence_status, FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL);


}
/* end salvador_dali_test() */


/*!**************************************************************
 *  salvador_dali_setup()
 ****************************************************************
 * @brief
 *  Setup the Salvador Dali test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  2011-10-31 Matthew Ferson - Created
 *
 ****************************************************************/
void salvador_dali_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        salvador_dali_test_load_physical_config();
        sep_config_load_sep_and_neit();
    }

    /* The following utility function does some setup for hardware. 
     */
    fbe_test_common_util_test_setup_init();
}
/* end salvador_dali_setup() */


/*!**************************************************************
 *  salvador_dali_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup after the Salvador Dali test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 2011-10-31 Matthew Ferson - Created 
 *
 ****************************************************************/
void salvador_dali_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/* end salvador_dali_cleanup () */


/*!**************************************************************
 *  salvador_dali_test_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the Salvador Dali test physical configuration.
 *  This code comes from Larry T Lobster physical setup.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/
static void salvador_dali_test_load_physical_config(void)
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
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);
    fbe_test_pp_util_insert_viper_enclosure(0, 1, port0_handle, &encl0_1_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, SALVADOR_DALI_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    for ( slot = 0; slot < 15; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, SALVADOR_DALI_TEST_DRIVE_CAPACITY, &drive_handle);
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
    status = fbe_api_wait_for_number_of_objects(SALVADOR_DALI_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
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
    MUT_ASSERT_TRUE(total_objects == SALVADOR_DALI_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

    return;

} 
/* end salvador_dali_test_load_physical_config() */

/* end salvador_dali_test.c */

