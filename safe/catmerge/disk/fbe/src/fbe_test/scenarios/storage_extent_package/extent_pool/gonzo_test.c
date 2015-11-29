/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file gonzo_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for extent pool objects.
 *
 * @version
 *  6/13/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_extent_pool_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "fbe_private_space_layout.h"
#include "pp_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * gonzo_short_desc = "This scenario will drive I/O to an extent pool.";
char * gonzo_long_desc ="\
\n\
Drive I/O to extent pool.\n";

enum gonzo_defines_e
{
    GONZO_NUM_DRIVES = 20,
    GONZO_TEST_EXTENT_POOL_ID = 1,
    GONZO_TEST_EXTENT_POOL_DRIVE_COUNT = 20,
    GONZO_TEST_EXTENT_POOL_LUN_CAPACITY = 0x10000 * 4,
    GONZO_TEST_EXTENT_POOL_LUN_COUNT = 2,
    GONZO_TEST_ELEMENT_SIZE = 128,
    GONZO_DATA_DISKS = 4,
    GONZO_STRIPE_SIZE = GONZO_TEST_ELEMENT_SIZE * GONZO_DATA_DISKS,
    GONZO_EXTRA_BLOCKS = 0x800 * 10,
};
static fbe_api_rdgen_context_t gonzo_test_contexts[GONZO_TEST_EXTENT_POOL_LUN_COUNT * 2];

/*!**************************************************************
 * gonzo_test_create_extent_pool()
 ****************************************************************
 * @brief
 *  This function creates an exent pool.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  6/16/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void 
gonzo_test_create_extent_pool(fbe_u32_t pool_id, fbe_u32_t drive_count, fbe_drive_type_t drive_type)
{
    fbe_api_job_service_create_extent_pool_t create_pool;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"gonzo_test: creating extent pool 0x%x drives: %u\n", pool_id, drive_count);

    create_pool.drive_count = drive_count;
    create_pool.drive_type = drive_type;
    create_pool.pool_id = pool_id;
	status = fbe_api_job_service_create_extent_pool(&create_pool);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end gonzo_test_create_extent_pool()
 ******************************************/

/*!**************************************************************
 * gonzo_test_create_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function creates an exent pool lun.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  6/16/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void 
gonzo_test_create_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t lun_id, fbe_lba_t capacity)
{
    fbe_api_job_service_create_ext_pool_lun_t create_lun;
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS,"gonzo_test: creating extent pool lun 0x%x\n", lun_id);

    create_lun.lun_id = lun_id;
    create_lun.pool_id = pool_id;
    create_lun.capacity = capacity;
	status = fbe_api_job_service_create_ext_pool_lun(&create_lun);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************
 * end gonzo_test_create_extent_pool_lun()
 ******************************************/

/*!**************************************************************
 * gonzo_test_check_extent_pool()
 ****************************************************************
 * @brief
 *  This function checks an exent pool after creation.
 *
 * @param pool_id - pool id.               
 *
 * @return None.   
 *
 * @author
 *  6/16/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void 
gonzo_test_check_extent_pool(fbe_u32_t pool_id)
{
    fbe_status_t status;
    fbe_object_id_t pool_object_id;

    status = fbe_api_database_lookup_ext_pool_by_number(pool_id, &pool_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pool_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end gonzo_test_check_extent_pool()
 ******************************************/

/*!**************************************************************
 * gonzo_test_check_extent_pool_lun()
 ****************************************************************
 * @brief
 *  This function checks an exent pool lun after creation.
 *
 * @param lun_id - lun id.               
 *
 * @return None.   
 *
 * @author
 *  6/16/2014 - Created. Rob Foley
 *
 ****************************************************************/
static void 
gonzo_test_check_extent_pool_lun(fbe_u32_t lun_id)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;

    status = fbe_api_database_lookup_ext_pool_lun_by_number(lun_id, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end gonzo_test_check_extent_pool_lun()
 ******************************************/

/*!**************************************************************
 * gonzo_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

void gonzo_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &gonzo_test_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_EXTENT_POOL_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             GONZO_STRIPE_SIZE,    /* Number of stripes to write. */
                                             GONZO_STRIPE_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);
    return;
}
/******************************************
 * end gonzo_write_background_pattern()
 ******************************************/
/*!**************************************************************
 * gonzo_read_background_pattern()
 ****************************************************************
 * @brief
 *  Check all luns for a pattern.
 *
 * @param pattern - the pattern to check for.               
 *
 * @return None.
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

void gonzo_read_background_pattern(fbe_rdgen_pattern_t pattern)
{
    fbe_api_rdgen_context_t *context_p = &gonzo_test_contexts[0];
    fbe_status_t status;

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_EXTENT_POOL_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             GONZO_STRIPE_SIZE,    /* Number of stripes to write. */
                                             GONZO_STRIPE_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);

    /* Set the sector trace stop on error off.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    return;
}
/******************************************
 * end gonzo_read_background_pattern()
 ******************************************/

/*!**************************************************************
 * gonzo_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

void gonzo_test(void)
{
    fbe_status_t status;
    fbe_u32_t index;


    /* Create extent pool */
    gonzo_test_create_extent_pool(GONZO_TEST_EXTENT_POOL_ID, GONZO_TEST_EXTENT_POOL_DRIVE_COUNT, FBE_DRIVE_TYPE_SAS);
    gonzo_test_check_extent_pool(GONZO_TEST_EXTENT_POOL_ID);

    /* Create extent pool luns */
    for (index = 0; index < GONZO_TEST_EXTENT_POOL_LUN_COUNT; index++) {
        gonzo_test_create_extent_pool_lun(GONZO_TEST_EXTENT_POOL_ID, index+1, GONZO_TEST_EXTENT_POOL_LUN_CAPACITY);
        gonzo_test_check_extent_pool_lun(index+1);
    }

    gonzo_test_check_extent_pool_lun(0 /* MD lun */);

    fbe_test_check_launch_cli();
    mut_printf(MUT_LOG_TEST_STATUS, "== Write Pattern ==");
    gonzo_write_background_pattern();
    mut_printf(MUT_LOG_TEST_STATUS, "== Write Pattern Success==");

    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern ==");
    gonzo_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern Success==");

    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(120000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_database_set_load_balance(FBE_TRUE);
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate pool exists ");
    gonzo_test_check_extent_pool(GONZO_TEST_EXTENT_POOL_ID);
    for (index = 0; index < GONZO_TEST_EXTENT_POOL_LUN_COUNT; index++) {

        mut_printf(MUT_LOG_TEST_STATUS, "== Validate pool lun %u exists ", index+1);
        gonzo_test_check_extent_pool_lun(index+1);
    }
    gonzo_test_check_extent_pool_lun(0 /* MD lun */);
    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern ==");
    gonzo_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern Success==");
    mut_pause();
}
/******************************************
 * end gonzo_test()
 ******************************************/
/*!**************************************************************
 * gonzo_dualsp_test()
 ****************************************************************
 * @brief
 *  Run extent pool I/O.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

void gonzo_dualsp_test(void)
{
    fbe_status_t status;
    fbe_u32_t index;
    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_check_launch_cli();

    /* Create extent pool */
    gonzo_test_create_extent_pool(GONZO_TEST_EXTENT_POOL_ID, GONZO_TEST_EXTENT_POOL_DRIVE_COUNT, FBE_DRIVE_TYPE_SAS);
    gonzo_test_check_extent_pool(GONZO_TEST_EXTENT_POOL_ID);

    /* Create extent pool luns */
    for (index = 0; index < GONZO_TEST_EXTENT_POOL_LUN_COUNT; index++) {
        gonzo_test_create_extent_pool_lun(GONZO_TEST_EXTENT_POOL_ID, index+1, GONZO_TEST_EXTENT_POOL_LUN_CAPACITY);
        gonzo_test_check_extent_pool_lun(index+1);
    }
    gonzo_test_check_extent_pool_lun(0 /* MD lun */);
    mut_printf(MUT_LOG_TEST_STATUS, "== Write Pattern ==");
    gonzo_write_background_pattern();
    mut_printf(MUT_LOG_TEST_STATUS, "== Write Pattern Success==");

    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern ==");
    gonzo_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern Success==");

    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    mut_pause();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    fbe_test_sep_util_destroy_neit_sep();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_destroy_neit_sep();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit();

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_util_wait_for_database_service(120000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_database_set_load_balance(FBE_TRUE);
    fbe_api_provision_drive_set_slf_enabled(FBE_TRUE);
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate pool exists ");
    gonzo_test_check_extent_pool(GONZO_TEST_EXTENT_POOL_ID);
    for (index = 0; index < GONZO_TEST_EXTENT_POOL_LUN_COUNT; index++) {

        mut_printf(MUT_LOG_TEST_STATUS, "== Validate pool lun %u exists ", index+1);
        gonzo_test_check_extent_pool_lun(index+1);
    }
    gonzo_test_check_extent_pool_lun(0 /* MD lun */);
    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern ==");
    gonzo_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    mut_printf(MUT_LOG_TEST_STATUS, "== Check Pattern Success==");
    mut_pause();
    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end gonzo_dualsp_test()
 ******************************************/

/*!**************************************************************
 * gonzo_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void gonzo_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation()) {

        /* Setup the slice size to be appropriate for simulation.
         */
        fbe_test_set_ext_pool_slice_blocks_sim();
        fbe_test_pp_util_create_config_vary_capacity(GONZO_NUM_DRIVES,/* 520 drives */ 0, /* 4k drives */
                                                 TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                 GONZO_EXTRA_BLOCKS);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end gonzo_setup()
 **************************************/

/*!**************************************************************
 * gonzo_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test to run on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void gonzo_dualsp_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        /* Setup the slice size to be appropriate for simulation.
         */
        fbe_test_set_ext_pool_slice_blocks_sim();

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_pp_util_create_config_vary_capacity(GONZO_NUM_DRIVES,/* 520 drives */ 0, /* 4k drives */
                                                 TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                 GONZO_EXTRA_BLOCKS);
        elmo_load_sep_and_neit();
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_pp_util_create_config_vary_capacity(GONZO_NUM_DRIVES,/* 520 drives */ 0, /* 4k drives */
                                                 TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY,
                                                 GONZO_EXTRA_BLOCKS);
        elmo_load_sep_and_neit();
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/**************************************
 * end gonzo_dualsp_setup()
 **************************************/

/*!**************************************************************
 * gonzo_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the gonzo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void gonzo_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end gonzo_cleanup()
 ******************************************/

/*!**************************************************************
 * gonzo_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the gonzo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void gonzo_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end gonzo_dualsp_cleanup()
 ******************************************/

/*************************
 * end file gonzo_test.c
 *************************/


