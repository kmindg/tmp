/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file hamburger_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test for verify for system power failure during
 *  non cached writes
 *
 *
 ***************************************************************************/

#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe_test.h"
#include "EmcPAL_Misc.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"


char * hamburger_short_desc = "Test automatic full verify of dirty LUNs (RAID-5)";
char * hamburger_long_desc =
    "\n"
    "\n"
    "The Hamburger scenario tests that when a RAID group goes to SHUTDOWN with outstanding\n"
    "non-cached write I/O, that it verifies the entire extent of any LUN that had\n"
    "outstanding non-cached writes when the RAID group comes back up.\n"
    "\n"
    "Dependencies:\n"
    "    - Persistent meta-data storage.\n"
    "    - The LUN object must support marking itself as 'LUN dirty' while non-cached writes are outstanding.\n"
    "    - The raid object must support verify-before write operations and any required verify meta-data.\n"
    " \n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 2 SAS drives (PDO)\n"
    "    [PP] 2 logical drives (LD)\n"
    "    [SEP] 2 provision drives (PD)\n"
    "    [SEP] 5 virtual drives (VD)\n"
    "    [SEP] 1 raid object (RAID-1)\n"
    "    [SEP] 3 lun objects (LU)\n"
    "\n"
    "STEP 1: Bring up the initial topology on one SP.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create a raid object with I/O edges attached to all VDs.\n"
    "    - Create each lun object with an I/O edge attached to the RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Shutdown RG during non-cached write I/O\n"
    "    - Start non-cached write I/O only to the first LUN object\n"
    "    - Shutdown the RG by pulling multiple drives\n"
    "    - Stop I/O\n"
    "    - Restore drives in RG\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - TODO: Verify that the first LUN's entire extent is marked as \n"
    "      'needing to be verified before write' in the raid BV map.\n"
    "    - Verify that the RAID object verifies the first LUN extent.\n"
    "\n"
    "TODO: STEP 3: Power cycle array during non-cached write I/O\n"
    "    - Start non-cached write I/O only to the first LUN object\n"
    "    - Kill both SP\n"
    "    - Restart both SP\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "    - Verify that the first LUN's entire extent is marked as \n"
    "      'needing to be verified before write' in the raid BV map.\n"
    "    - Verify that the RAID object verifies the first LUN extent.\n"
    "\n"
    "TODO: STEP 4: Power cycle SP during non-cached write I/O\n"
    "    - Start non-cached write I/O only to the first LUN object\n"
    "    - Kill the SP\n"
    "    - Confirm peer takes over\n"
    "       - Verify that all configured objects are in the READY state.\n"
    "       - Verify that the first LUN's entire extent is marked as \n"
    "         'needing to be verified before write' in the raid BV map.\n"
    "       - Verify that the RAID object verifies the first LUN extent.\n"
    "    - Restart failed SP.\n"
    "=======================COMMENTS==========================\n"
    "1. More RAID types need to be tested. Currently only 1 RAID5 raid \n"
    "   group is tested (Perhaps for rtl 1)\n"
    "2. The test does not check that the verify actually occured, only \n"
    "   that the verify checkpoint in invalid\n"
    "3. Test cases purposely adding errors and checking that the numbers\n"
    "   increment accordingly should be added. The test should also run\n"
    "   multiple iterations, so that the total count is incremented\n"
    "   while the previous and current change accordingly. Test cases for\n"
    "   both correctables and uncorrectables should be considered.\n"
    "4. Add a test case for something that could potentially interrupt a\n"
    "   verify and after that interruption, the verify numbers are still correct.\n"
    "5. Perhaps it could be useful to only write to a single LUN and check\n"
    "   that the other luns in the RAID group do not need to be verified.\n"
    "6. Perhaps RAID group parameters could be checked if applicable.\n"
    "7. TODO: Currently the SP crash and dualsp crash tests aren't implemented\n"
    "8. The function for remove and reinsert drives can probably be replaced\n"
    "   with appropriate calls to functions in the sep_rebuild_utils\n"
    "9. TODO: Verify tht the first LUN's entire extent is marked as needing\n"
    "   to be verified before write\n"
    "\n"
    "Description last updated: 10/28/2011.\n"
    ;



/*!*******************************************************************
 * @def HAMBURGER_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define HAMBURGER_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def HAMBURGER_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define HAMBURGER_RAID_GROUP_COUNT   1

/*!*******************************************************************
 * @def HAMBURGER_RAID_GROUP_CHUNK_SIZE
 *********************************************************************
 * @brief Number of blocks in a raid group bitmap chunk.
 *
 *********************************************************************/
#define HAMBURGER_RAID_GROUP_CHUNK_SIZE  (2 * FBE_RAID_SECTORS_PER_ELEMENT)

/*!*******************************************************************
 * @def HAMBURGER_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define HAMBURGER_LUNS_PER_RAID_GROUP    3

/*!*******************************************************************
 * @def HAMBURGER_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of the LUNs.
 *********************************************************************/
#define HAMBURGER_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def HAMBURGER_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS
 *********************************************************************
 * @brief Capacity of the virtual drives.
 *********************************************************************/
    /* Treat the drive capacity as small to end up with a raid group
 * capacity that is relatively small also. 
 * For comparison, a typical 32 mb sim drive is about 0xF000 blocks. 
 */
// #define HAMBURGER_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/*!*******************************************************************
 * @def HAMBURGER_MAX_VERIFY_WAIT_TIME_SECS
 *********************************************************************
 * @brief Maximum time in seconds to wait for a verify operation to complete.
 *********************************************************************/
#define HAMBURGER_MAX_VERIFY_WAIT_TIME_SECS  120  

/*!*******************************************************************
 * @def BIG_BIRD_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define HAMBURGER_MAX_IO_SIZE_BLOCKS (HAMBURGER_TEST_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 
#define HAMBURGER_MIN_IO_SIZE_BLOCKS (2 * FBE_RAID_MAX_BE_XFER_SIZE) // 4096 

/*!*******************************************************************
 * @def HAMBURGER_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.
 *
 *********************************************************************/
#define HAMBURGER_FIRST_POSITION_TO_REMOVE 0

/*!*******************************************************************
 * @def HAMBURGER_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define HAMBURGER_SECOND_POSITION_TO_REMOVE 1

#define HAMBURGER_CHUNKS_PER_LUN            3

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/


static void hamburger_raid5_verify_test(fbe_u32_t index,
                                        fbe_test_rg_configuration_t*);

static void hamburger_fail_rg_test(fbe_test_rg_configuration_t * rg_config_p);
void hamburger_write_background_pattern(void);

void hamburger_get_lun_verify_status(fbe_object_id_t in_object_id,
                                    fbe_lun_get_verify_status_t* out_verify_status);

static void hamburger_validate_verify_results(fbe_u32_t in_index,
                                              fbe_test_rg_configuration_t*);

static void hamburger_start_io(fbe_test_rg_configuration_t* in_current_rg_config_p);

static void hamburger_remove_and_reinsert_drives(fbe_test_rg_configuration_t*);

static void hamburger_wait_for_verify_completion(fbe_object_id_t            in_lun_object_id,
                                                 fbe_u32_t                  in_lun_number,
                                                 fbe_lun_verify_report_t*   in_verify_report_p,
                                                 fbe_object_id_t            in_rg_object_id);

static void hamburger_clear_verify_reports(fbe_test_rg_configuration_t* in_current_rg_config_p);
void hamburger_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

static void hamburger_reinsert_sp(fbe_sim_transport_connection_target_t which_sp, fbe_test_rg_configuration_t* in_current_rg_config_p);
static void hamburger_remove_sp(fbe_sim_transport_connection_target_t which_sp);

/*****************************************
 * GLOBAL DATA
 *****************************************/

/*!*******************************************************************
 * @var hamburger_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t hamburger_raid_group_config[] =  
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    //{3,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,  FBE_BE_BYTES_PER_BLOCK,            0,          0},
    {3,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,  FBE_4K_BYTES_PER_BLOCK,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};

/*!*******************************************************************
 * @var fbe_HAMBURGER_test_context_g
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t fbe_hamburger_test_context_g[HAMBURGER_LUNS_PER_RAID_GROUP * HAMBURGER_RAID_GROUP_COUNT];


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 * hamburger_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void hamburger_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * hamburger_dualsp_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ******************************************************************************/

void hamburger_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }
    return;
}

/*!****************************************************************************
 * hamburger_test()
 ******************************************************************************
 * @brief
 *  Run lun zeroing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void hamburger_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    rg_config_p = &hamburger_raid_group_config[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,hamburger_run_tests,
                                   HAMBURGER_LUNS_PER_RAID_GROUP,
                                   HAMBURGER_CHUNKS_PER_LUN);
    return;    

}

/*!****************************************************************************
 * hamburger_dualsp_test()
 ******************************************************************************
 * @brief
 *
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ******************************************************************************/
void hamburger_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    rg_config_p = &hamburger_raid_group_config[0];

    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,hamburger_run_tests,
                                   HAMBURGER_LUNS_PER_RAID_GROUP,
                                   HAMBURGER_CHUNKS_PER_LUN);

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;    

}


/*!**************************************************************
 * hamburger_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the Hamburger test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 * @author
 *
 ****************************************************************/
void hamburger_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &hamburger_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, HAMBURGER_LUNS_PER_RAID_GROUP, HAMBURGER_CHUNKS_PER_LUN);

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* The following utility function does some setup for hardware; it's a noop for simulation. */
    fbe_test_common_util_test_setup_init();

    return;

}   // End hamburger_setup()

/*!**************************************************************
 * hamburger_dualsp_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the Hamburger test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 * @author
 *  04/04/2012 - Created. MFerson
 ****************************************************************/
void hamburger_dualsp_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t  raid_group_count = 0;
        //const fbe_char_t *raid_type_string_p = NULL;
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        rg_config_p = &hamburger_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
        //fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);

        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* The following utility function does some setup for hardware; it's a noop for simulation. */
    fbe_test_common_util_test_setup_init();

    return;

}   // End hamburger_dualsp_setup()

/*!**************************************************************
 * hamburger_test()
 ****************************************************************
 * @brief
 *  This is the entry point into the hamburger test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void hamburger_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t            index;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   num_raid_groups = 0;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    num_raid_groups = HAMBURGER_RAID_GROUP_COUNT;

    mut_printf(MUT_LOG_LOW, "=== Hamburger test start ===\n");

    // STEP 2: Write initial data pattern to all configured LUNs.
    // This is necessary because bind is not working yet.
    hamburger_write_background_pattern();

    // Perform the write verify test on all raid groups.
    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
        
            mut_printf(MUT_LOG_TEST_STATUS, "hamburger verify test iteration %d of %d.\n", 
                        index+1, num_raid_groups);
            hamburger_fail_rg_test(rg_config_p);
            hamburger_raid5_verify_test(index, rg_config_p);
        }
    }

    return;

}   // End hamburger_test()
/*!**************************************************************
 * hamburger_fail_rg_test()
 ****************************************************************
 * @brief
 *  Run a test where we send non-cached write I/O to a LUN.
 *  The first non-cached write does a dirty, but gets delayed.
 *  The second non-cached write to the LUN gets queued behind the first.
 * 
 *  Meanwhile, the raid group shuts down and transitions to pending fail.
 *  The LUN sees this and also starts to drain.
 *  The LUN should complete the second I/O with failure first.
 *  And it should wait for the second I/O to finish.
 * 
 *  After we will stop injecting errors to allow the second I/O to finish.
 * 
 * @param rg_config_p - The raid group to test with.               
 *
 * @return None.
 *
 * @author
 *  7/13/2012 - Created. Rob Foley
 *
 ****************************************************************/

static void hamburger_fail_rg_test(fbe_test_rg_configuration_t * rg_config_p)
{
    fbe_api_logical_error_injection_get_object_stats_t obj_stats;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_rdgen_context_t *context_p   = &fbe_hamburger_test_context_g[0];
    fbe_api_rdgen_context_t *context1_p   = &fbe_hamburger_test_context_g[1];
    fbe_status_t status;
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_api_logical_error_injection_record_t error_record =
    { 
        0x1,  /* pos_bitmap */
        0x10, /* width */
        0x0,  /* lba */
        0xFFFFFFFF,  /* blocks */
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,  /* error type */
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
        0x0,  /* error count */
        FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_MS, /* error limit is seconds to wait*/
        0x0,  /* skip count */
        0x0 , /* skip limit */
        0x1,  /* error adjcency */
        0x0,  /* start bit */
        0x0,  /* number of bits */
        0x0,  /* erroneous bits */
        0x0,  /* crc detectable */
    };

    /* Wait for the zeroing and initial verify to be complete since the initial verify will get in 
     * the way of our test. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    /* Send the I/O asyncronously. 
     */
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = 0;
    params.blocks = 1;
    params.b_async = FBE_TRUE;
    params.options = FBE_RDGEN_OPTIONS_NON_CACHED;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* We need to wait for the first IO being sent out before issuing the second one */
    do {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (stats.num_errors_injected!=1) {
            mut_printf(MUT_LOG_TEST_STATUS, "first IO was not out, let's wait\n");
            EmcutilSleep(100);
        }
    } while(stats.num_errors_injected!=1);

    /* Send a second write.
     * The second write gets queued up behind the first one.
     */
    params.lba = 0x10;
    status = fbe_api_rdgen_send_one_io_params(context1_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Fail the rg with the I/O still outstanding.
     * This will cause the RG and LUN to start to drain I/Os. 
     * We use the fail method since it causes the raid group to immediately transition its state.
     */
#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "Move rg obj: 0x%x to fail", rg_object_id);
    status = fbe_api_set_object_to_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#else
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING,
                                              FBE_RAID_GROUP_SUBSTATE_PENDING,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    big_bird_remove_all_drives(rg_config_p, 1, 2, /* drives to remove. */
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
#endif

    /* Disable error injection, which allows raid group to quiesce and go thru pending.
     */
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    /* Objects will get stuck in pending fail since the I/Os cannot complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for rg obj: 0x%x to Fail", rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_PENDING_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUN rg obj: 0x%x to Fail", lun_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_PENDING_FAIL, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* The second I/O should have been stuck in the LUN, it should complete first.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for second issued I/O to fail");
    fbe_api_rdgen_wait_for_ios(context1_p, FBE_PACKAGE_ID_SEP_0, 1);
    status = fbe_api_rdgen_test_context_destroy(context1_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if ((context1_p->start_io.status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        (context1_p->start_io.status.status == FBE_STATUS_OK))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "bl_status: 0x%x status: 0x%x", 
                   context1_p->start_io.status.block_status, context1_p->start_io.status.status);
        MUT_FAIL_MSG("Write 1 was unexpectedly successful");
    }
#if 1
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING,
                                              FBE_RAID_GROUP_SUBSTATE_PENDING,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif
    
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for first issued I/O to fail");
    fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_SEP_0, 1);
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if ((context_p->start_io.status.block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        (context_p->start_io.status.status == FBE_STATUS_OK))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "bl_status: 0x%x status: 0x%x", 
                   context_p->start_io.status.block_status, context_p->start_io.status.status);
        MUT_FAIL_MSG("Write 0 was unexpectedly successful");
    }

    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    status = fbe_api_logical_error_injection_get_object_stats(&obj_stats, rg_object_id);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "Move rg obj: 0x%x to activate", rg_object_id);
    status = fbe_api_set_object_to_state(rg_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#else
    big_bird_insert_all_drives(rg_config_p, 1, 2, /* two drives to insert */
                               FBE_TRUE    /* We are pulling and reinserting same drive */);
#endif
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for rg obj: 0x%x to become ready", rg_object_id);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);

}
/******************************************
 * end hamburger_fail_rg_test()
 ******************************************/

/*!**************************************************************************
 * hamburger_raid5_verify_test
 ***************************************************************************
 * @brief
 *  This function performs the write verify test. It does the following
 *  on a raid group:
 *  - Writes an initial data pattern.
 *  - Writes correctable and uncorrectable CRC error patterns on two VDs.
 *  - Performs a write verify operation.
 *  - Validates the error counts in the verify report.
 *  - Performs another write verify operation.
 *  - Validates that there are no new error counts in the verify report.
 * 
 * @param in_index     - Index into the test configuration data table
 *
 * @return void
 *
 ***************************************************************************/
static void hamburger_raid5_verify_test(fbe_u32_t in_index,
                                        fbe_test_rg_configuration_t* in_current_rg_config_p)
{

    fbe_api_rdgen_context_t *   context_p   = &fbe_hamburger_test_context_g[0];
    fbe_bool_t                  b_dualsp    = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_status_t                status      = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   msg_count;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    mut_printf(MUT_LOG_HIGH, "==== Testing automatic dirty verify ====");

    if(b_dualsp)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        hamburger_start_io(in_current_rg_config_p);
        EmcutilSleep(5000);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        hamburger_clear_verify_reports(in_current_rg_config_p);
        hamburger_remove_sp(FBE_SIM_SP_A);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        hamburger_validate_verify_results(in_index, in_current_rg_config_p);
        hamburger_clear_verify_reports(in_current_rg_config_p);
        hamburger_reinsert_sp(FBE_SIM_SP_A, in_current_rg_config_p);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        fbe_api_rdgen_test_context_destroy(context_p);
    }
    else
    {
        
        hamburger_start_io(in_current_rg_config_p);
        EmcutilSleep(5000);
        hamburger_clear_verify_reports(in_current_rg_config_p);
        hamburger_remove_and_reinsert_drives(in_current_rg_config_p);
        hamburger_validate_verify_results(in_index, in_current_rg_config_p);
        /* Since we have incomplete write verify running, coherency errors are expected and so look for that
         * message
         */
       status = fbe_test_utils_get_event_log_error_count(SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR, &msg_count);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       MUT_ASSERT_INT_NOT_EQUAL_MSG(msg_count, 0, "Expected Coherency Error Message not found\n");

       /* We should NOT be seeing the Error message since Incomplete write verify should be running */
       status = fbe_test_utils_get_event_log_error_count(SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR, &msg_count);
       MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
       MUT_ASSERT_INT_EQUAL_MSG(msg_count, 0, "Unexpected Coherency Error Message found\n");

       hamburger_clear_verify_reports(in_current_rg_config_p);
    }

    return;

}   // End hamburger_raid5_verify_test()


/*!**************************************************************************
 * hamburger_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param void
 *
 * @return void
 *
 ***************************************************************************/
void hamburger_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &fbe_hamburger_test_context_g[0];

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);


    /* First fill the raid group with a pattern.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            HAMBURGER_ELEMENT_SIZE);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "hamburger background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End hamburger_write_background_pattern()


/*!**************************************************************
 * hamburger_start_io(fbe_test_rg_configuration_t* in_current_rg_config_p)
 ****************************************************************
 * @brief
 *  Run I/O.
 *  Degrade the raid group.
 *  Allow I/O to continue degraded.
 *  Bring drive back and allow it to rebuild.
 *  Stop I/O.
 *
 * @param None.               
 *
 * @return None.
 *
 *
 ****************************************************************/

static void hamburger_start_io(fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_api_rdgen_context_t *context_p = &fbe_hamburger_test_context_g[0];
    fbe_status_t status;
    fbe_database_raid_group_info_t      rg_info;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                           lun_index;
    
    fbe_object_id_t                             lun_object_id; 
    fbe_test_logical_unit_configuration_t *     logical_unit_configuration_p = NULL;

    fbe_bool_t non_cache_io = TRUE;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);


    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    
    status = fbe_api_database_get_raid_group(rg_object_id, &rg_info);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    for (lun_index = 0; lun_index < HAMBURGER_LUNS_PER_RAID_GROUP; lun_index++)
    {
        logical_unit_configuration_p = &in_current_rg_config_p->logical_unit_configuration_list[lun_index];
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
        
        /* make sure all the IO use MR3 alg */
        status = fbe_api_rdgen_test_context_init(&context_p[lun_index], 
                                                 lun_object_id,
                                                 FBE_CLASS_ID_LUN,
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                 FBE_RDGEN_PATTERN_LBA_PASS,    /* Standard rdgen data pattern */
                                                 0,                             /* pass_count */
                                                 0,                             /* io count not used */
                                                 0,                             /* time */
                                                 1,                             /* threads */
                                                 FBE_RDGEN_LBA_SPEC_FIXED,      /* Standard I/O mode fixed */
                                                 0,                             /* Start lba*/
                                                 0,                             /* Min lba */
                                                 FBE_LBA_INVALID,               /* max lba */
                                                 FBE_RDGEN_BLOCK_SPEC_CONSTANT, /* Standard I/O transfer size is random */
                                                 (rg_info.rg_info.element_size * (rg_info.rg_info.width - 1)),  /* Min blocks per request */
                                                 (rg_info.rg_info.element_size * (rg_info.rg_info.width - 1))   /* Max blocks per request */ );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    

        if(non_cache_io)
        {
            status = fbe_api_rdgen_io_specification_set_options(&context_p[lun_index].start_io.specification,
                                                                FBE_RDGEN_OPTIONS_NON_CACHED);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* Run our I/O test.
        */
        status = fbe_api_rdgen_start_tests(&context_p[lun_index], FBE_PACKAGE_ID_NEIT, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/******************************************
 * end hamburger_start_io()
 ******************************************/


/*!**************************************************************
 * hamburger_remove_and_reinsert_drives()
 ****************************************************************
 * @brief
 *  Remove drives and reinsert drives
 *
 *
 *
 * @return None.
 *
 *
 ****************************************************************/

static void hamburger_remove_and_reinsert_drives(fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_status_t            status;
    fbe_object_id_t         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t               lun_index;
    fbe_api_rdgen_context_t *context_p   = &fbe_hamburger_test_context_g[0];
    fbe_test_rg_configuration_t *rg_config_p = in_current_rg_config_p;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_logical_error_injection_record_t error_record =
    { 
        0x3,  /* pos_bitmap */
        0x3,  /* width */
        0x0,  /* lba */
        0xFFFFFFFF,  /* blocks */
        FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,  /* error type */
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
        0x0,  /* error count */
        FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_MS, /* error limit is seconds to wait*/
        0x0,  /* skip count */
        0x0 , /* skip limit */
        0x1,  /* error adjcency */
        0x0,  /* start bit */
        0x0,  /* number of bits */
        0x0,  /* erroneous bits */
        0x0,  /* crc detectable */
    };


    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    
    /* Remove drives.
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s inject delay down error. ==\n", __FUNCTION__);
        
        status = fbe_api_logical_error_injection_disable_records(0, 255);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_create_record(&error_record);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* We need to wait for hitting the error injection record */
        do {
            status = fbe_api_logical_error_injection_get_stats(&stats);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            if (stats.num_errors_injected == 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "no delay error hit, let's wait\n");
                EmcutilSleep(100);
            }
        } while(stats.num_errors_injected == 0);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drive. ==\n", __FUNCTION__);
        fbe_test_sep_util_remove_drives(HAMBURGER_FIRST_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drive successful. ==\n", __FUNCTION__);
    
    
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
        fbe_test_sep_util_remove_drives(HAMBURGER_SECOND_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive successful. ==\n", __FUNCTION__);
    }
    else
    {
        /* Not in simulation */
        fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;

        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drive. ==\n", __FUNCTION__);
        drive_to_remove_p = &(rg_config_p->rg_disk_set[HAMBURGER_FIRST_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                drive_to_remove_p->enclosure, 
                                                drive_to_remove_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
        drive_to_remove_p = &(rg_config_p->rg_disk_set[HAMBURGER_SECOND_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                drive_to_remove_p->enclosure, 
                                                drive_to_remove_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable delay down error. ==\n", __FUNCTION__);
    
    /* Disable error injection, which allows raid group to quiesce and go thru pending.  */
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, 20000, FBE_PACKAGE_ID_SEP_0);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==\n", __FUNCTION__);

    for (lun_index = 0; lun_index < HAMBURGER_LUNS_PER_RAID_GROUP; lun_index++)
    {
        status = fbe_api_rdgen_stop_tests(&context_p[lun_index], 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==\n", __FUNCTION__);

    /* Put the drives back in.
     */
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive ==\n", __FUNCTION__);
        fbe_test_sep_util_insert_drives(HAMBURGER_FIRST_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive successful. ==\n", __FUNCTION__);
    
    
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive ==\n", __FUNCTION__);
        fbe_test_sep_util_insert_drives(HAMBURGER_SECOND_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive successful. ==\n", __FUNCTION__);
    }
    else
    {
        /* Not in simulation */
        fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive ==\n", __FUNCTION__);
        drive_to_insert_p = &(rg_config_p->rg_disk_set[HAMBURGER_FIRST_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                    drive_to_insert_p->enclosure, 
                                                    drive_to_insert_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive ==\n", __FUNCTION__);
        drive_to_insert_p = &(rg_config_p->rg_disk_set[HAMBURGER_SECOND_POSITION_TO_REMOVE]);

        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                    drive_to_insert_p->enclosure, 
                                                    drive_to_insert_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Wait for pdo to be ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for first drive to be ready ==\n", __FUNCTION__);
    fbe_test_sep_util_wait_for_pdo_ready(HAMBURGER_FIRST_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  waiting for first drive to be ready successful. ==\n", __FUNCTION__);


    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for second drive to be ready ==\n", __FUNCTION__);
    fbe_test_sep_util_wait_for_pdo_ready(HAMBURGER_SECOND_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  waiting for second drive to be ready successful. ==\n", __FUNCTION__);


    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==\n", __FUNCTION__);

    fbe_test_sep_util_wait_for_all_objects_ready(in_current_rg_config_p);
    
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==\n", __FUNCTION__);

    return;

}/******************************************
 * end hamburger_remove_and_reinsert_drives()
 ******************************************/

/*!**************************************************************
 * hamburger_remove_sp()
 ****************************************************************
 * @brief
 *  Remove SP
 *
 *
 *
 * @return None.
 *
 *
 ****************************************************************/

static void hamburger_remove_sp(fbe_sim_transport_connection_target_t which_sp)
{
    fbe_status_t            status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    status = fbe_api_sim_transport_set_target_server(which_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_destroy_client(which_sp);
    fbe_test_sp_sim_stop_single_sp(which_sp == FBE_SIM_SP_A? FBE_TEST_SPA: FBE_TEST_SPB);

    return;
}
/******************************************
 * end hamburger_reinsert_sp()
 ******************************************/

static void hamburger_reinsert_sp(fbe_sim_transport_connection_target_t which_sp, fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_status_t                    status;
    fbe_test_rg_configuration_t *   rg_config_p = NULL;
    fbe_u32_t num_raid_groups;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    rg_config_p = in_current_rg_config_p;

    fbe_test_base_suite_startup_single_sp(which_sp);

    status = fbe_api_sim_transport_set_target_server(which_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

    sep_config_load_sep_and_neit();

    //elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);

    return;
}
/******************************************
 * end hamburger_reinsert_sp()
 ******************************************/

/*!**************************************************************
 * hamburger_validate_verify_results()
 ****************************************************************
 * @brief
 *  Validate the background verify that was initiated for the dirty luns
 *
 * @param position_to_insert - The array index to insert.               
 *
 * @return None.
 *
 *
 ****************************************************************/

static void hamburger_validate_verify_results(fbe_u32_t in_index,
                                              fbe_test_rg_configuration_t* in_current_rg_config_p)
{

    fbe_object_id_t                             lun_object_id;              // LUN object to test
    fbe_u32_t                                   index;
    fbe_lun_verify_report_t                     verify_report[HAMBURGER_LUNS_PER_RAID_GROUP] = {0}; // the verify report data
    fbe_test_logical_unit_configuration_t *     logical_unit_configuration_p = NULL;
    fbe_object_id_t                             rg_object_id;
    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);


    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (index =0; index < HAMBURGER_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        hamburger_wait_for_verify_completion(lun_object_id, logical_unit_configuration_p->lun_number, &verify_report[index], rg_object_id);
       
        mut_printf(MUT_LOG_TEST_STATUS, "coherency errors for lun %d is %d\n",index,
                   verify_report[index].current.correctable_coherency);

        // Validate that the correctable coherency error is not zero
        //MUT_ASSERT_TRUE((verify_report[index].current.correctable_coherency) != 0);

        logical_unit_configuration_p++;
    }
    return;

} // End hamburger_validate_verify_results


/*!**************************************************************************
 * hamburger_clear_verify_reports()
 ***************************************************************************
 * @brief
 *  This function clears the verify report
 * 
 * @param in_current_rg_config_p   - Pointer to the raid group
 *
 * @return void
 *
 ***************************************************************************/
static void hamburger_clear_verify_reports(fbe_test_rg_configuration_t* in_current_rg_config_p)
                                                 
{
    fbe_object_id_t                             lun_object_id; 
    fbe_u32_t                                   index; 
    fbe_test_logical_unit_configuration_t *     logical_unit_configuration_p = NULL;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (index =0; index < HAMBURGER_LUNS_PER_RAID_GROUP; index++)
    {
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        // Initialize the verify report data.
        fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
        logical_unit_configuration_p++;
    }

    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    return;

}   // End hamburger_clear_verify_reports()

/*!**************************************************************
 * hamburger_wait_for_verify_completion()
 ****************************************************************
 * @brief
 *  This function waits for the user verify operation to complete
 *  on the specified LUN.
 *
 * @param in_object_id      - pointer to LUN object
 *
 * @return void
 *
 ****************************************************************/
static void hamburger_wait_for_verify_completion(fbe_object_id_t in_lun_object_id,
                                                     fbe_u32_t in_lun_number,
                                                     fbe_lun_verify_report_t*   verify_report_p,
                                                     fbe_object_id_t in_rg_object_id)
                                                    
{
    fbe_u32_t                              sleep_time;    
    fbe_bool_t                             is_msg_present;
    fbe_bool_t                             started = FBE_TRUE;
    fbe_status_t                           status;

    mut_printf(MUT_LOG_HIGH, "%s entry (LUN %d / Object 0x%X)", 
               __FUNCTION__, in_lun_number, in_lun_object_id);

    for (sleep_time = 0; sleep_time < (HAMBURGER_MAX_VERIFY_WAIT_TIME_SECS*10); sleep_time++)
    {
        if(! started)
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                                 &is_msg_present, 
                                                 SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED,
                                                 in_lun_number,
                                                 in_lun_object_id
                                                 );

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if(is_msg_present)
            {
                mut_printf(MUT_LOG_HIGH, "%s LUN %d automatic verify started", __FUNCTION__, in_lun_number);
                started = FBE_TRUE;
            }
        }

        if(started)
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                                 &is_msg_present, 
                                                 SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE,
                                                 in_lun_number,
                                                 in_lun_object_id
                                                 );

            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if(is_msg_present)
            {
                /* Now get the verify report for the LUN 
                 */
                status = fbe_test_sep_util_lun_get_verify_report(in_lun_object_id, verify_report_p);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                mut_printf(MUT_LOG_HIGH, "%s LUN %d automatic verify completed after %d msecs",
                           __FUNCTION__, in_lun_number, (sleep_time * 100));
                return;
            }
        }

        EmcutilSleep(100);
    }

    //  The verify operation took too long and timed out.
    //while(1) {
    //    EmcutilSleep(100);
    //}
    MUT_ASSERT_TRUE(0)
    return;

}   // End hamburger_wait_for_verify_completion()

