/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file tico_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of drive glitching tests
 *
 * @version
 *   08/01/2013 - Created. Geng, Han
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:

#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
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
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_bvd_interface.h"
#include "fbe_test_esp_utils.h"
#include "sep_rebuild_utils.h"

//-----------------------------------------------------------------------------------------
// issue1: miss to mark IW_VERIY for metadata area in the drvie glitch case
    // step1: fail the drive on pos 3 to let RG run on degraded mode
    // step2: add hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block rebuilding process
    // step3: swap in a new drive
    // step4: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    // step5: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for write operation on pos5 
    // step6: del hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start rebuilding process
    // step7: make sure at least one hit on the error record 
    // step8: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive glitch case
    // step9: glitch the drive on pos 5 by pulling out and pulling in the drive
    // step10: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to start the EVAL_RL
    // step11: check the incomplete_write_verify_checkpoint after EVAL_RL done
//-----------------------------------------------------------------------------------------
// issue2: request of paged metadata SL is aborted while marking IW_VERIY for MD in evaluate rebuild logging case
    // step1: fail the drive on pos 3 to let RG run on degraded mode
    // step2: add hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block rebuilding process
    // step3: swap in a new drive
    // step4: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    // step5: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR error for write operation on pos5 
    // step6: del hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start rebuilding process
    // step7: make sure at least one hit on the error record 
    // step8: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive dead case
    // step9: fail the drive on pos 5 by pulling out and pulling in the drive
    // step10: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to continue the EVAL_RL
    // step11: check the rebuild_checkpoint
//-----------------------------------------------------------------------------------------
// issue3: fbe_parity_468_state22 force change RETRY_POSSIBLE_ERROR to RETRY_NOT_POSSIBLE_ERROR, which will get RG out of READY in some case
    // step1: degrade the RG by failing the drive on pos 3
    // step2: wait unit all the background verify has been finished
    // step3: add the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block the rebuild
    // step4: swap in a new drive 
    // step5: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    // step6: pull out the drive on pos 5
    // step7: inject a FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for read on pos 3, on the paged metadata region
    // step8: del the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start the rebuild
    // step9: waiting for the the error injection hits 2 times, we need to remove the drive after pending the fruts IO
    //        to simuate the drive dead error. 
    //        the 1st time is the case reading chunk info; the 2nd time is the case updating the MD
    // step10: remove the drive again on pos3 to simulate the drive dead case
    // step11: make sure RG not change to ACTIVATE state




//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* tico_short_desc = "todo: tico_short_desc";
char* tico_long_desc =
"\n\
Todo: tico_long_desc\n\
\n\
Description last updated: 08/01/2013.\n";


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:
extern fbe_u32_t    sep_rebuild_utils_number_physical_objects_g;
extern fbe_u32_t    sep_rebuild_utils_next_hot_spare_slot_g;

#define TICO_WAIT_FOR_VERIFY_SEC 600 /* 10000 This value is in seconds */
#define TICO_WAIT_MSEC 50000


fbe_u32_t tico_doo_library_debug_flags = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                          FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                          FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING  |
                                          FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );
fbe_u32_t tico_doo_object_debug_flags = (FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING |
                                         FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING |
                                         FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR |
                                         FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING);


//  RG configuration for the R6 
fbe_test_rg_configuration_t tico_rg_config_g[] = 
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 1, 0},
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 2, 0},
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 3, 0},
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 4, 0},
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 5, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//  Context objects used for running rdgen I/O
static fbe_api_rdgen_context_t tico_test_rdgen_context_g[SEP_REBUILD_UTILS_NUM_RAID_GROUPS][SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP];
static fbe_u32_t    tico_outstanding_async_io[SEP_REBUILD_UTILS_NUM_RAID_GROUPS][SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP] =  { 0 };


//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:

//  Run all tests
static void tico_run_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  run the test case in which we will continue SIOTS instead of restarting it after glitch drive back
static void tico_run_siots_continue_with_glitch_drive_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  run the test case in which we will populate the retryable error to parent siots when nested_siots failed
static void tico_run_non_retriable_error_tests(fbe_test_rg_configuration_t* config_p, void* context_p);

//  run the test case in which we will check the eval_rl will not run when the removed drive comes back on the pos, on which the rebuild logging has already started
static void tico_run_avoid_eval_rl_test(fbe_test_rg_configuration_t* config_p, void* context_p);

//  run the test case in whitch we will glitch one drive and fail one paged metada write operation, then observe whether the IW_VERIY is marked for MD
static void tico_run_raid_drive_glitch_incomplete_md_write_test(fbe_test_rg_configuration_t* in_rg_config_p, void *context_p);

//  run the test case in whitch we will fail one drive and fail one paged metada write operation, then observe whether the IW_VERIY is marked for MD
static void tico_run_raid_drive_fail_incomplete_md_write_test(fbe_test_rg_configuration_t* in_rg_config_p, void *context_p);

//  run the test case in which we will check the whether the FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD will be cleared after peer panic
static void tico_run_clear_quiesce_hold_flag_test(fbe_test_rg_configuration_t* config_p, void* context_p);


static void tico_send_io(fbe_test_rg_configuration_t *rg_config_p,
                         fbe_api_rdgen_context_t *context_p,
                         fbe_rdgen_operation_t rdgen_operation,
                         fbe_rdgen_pattern_t pattern,
                         fbe_data_pattern_sp_id_t sp_id,
                         fbe_lba_t lba,
                         fbe_block_count_t blks,
                         fbe_u32_t pass_count);



//  Inject an error record in a RG
static void tico_test_inject_error_record(
                                    fbe_test_rg_configuration_t*           in_rg_config_p,
                                    fbe_u16_t                              in_position,
                                    fbe_lba_t                              in_lba,
                                    fbe_lba_t                              in_blocks,
                                    fbe_u32_t                              in_error_limit,
                                    fbe_u32_t                              in_delay_time, /* for IO delay error */
                                    fbe_api_logical_error_injection_type_t in_err_type,
                                    fbe_u32_t                              in_opcode,
                                    fbe_u16_t                              in_rg_width);

// disable error inject service
static void tico_test_disable_error_inject(fbe_test_rg_configuration_t* in_rg_config_p);

//  Start I/O to the LUNs in a RG
static void tico_test_start_one_io(
                               fbe_test_rg_configuration_t*    in_rg_config_p,
                               fbe_u32_t                       in_rdgen_context_rg_index,
                               fbe_lba_t                       in_lba,
                               fbe_block_count_t               in_num_blocks,
                               fbe_rdgen_operation_t           in_rdgen_op,
                               fbe_object_id_t                 in_lun_object_id);

// Stop the asynchronous I/O
static void tico_test_stop_io(     
                              fbe_test_rg_configuration_t*    in_rg_config_p,
                              fbe_u32_t                       in_rdgen_context_rg_index,
                              fbe_lba_t                       in_lba,
                              fbe_block_count_t               in_num_blocks,
                              fbe_rdgen_operation_t           in_rdgen_op,
                              fbe_object_id_t                 in_lun_object_id);

// Cleanup the I/O context
static void tico_test_cleanup_io(     
                                 fbe_test_rg_configuration_t*    in_rg_config_p,
                                 fbe_u32_t                       in_rdgen_context_rg_index,
                                 fbe_lba_t                       in_lba,
                                 fbe_block_count_t               in_num_blocks,
                                 fbe_rdgen_operation_t           in_rdgen_op,
                                 fbe_object_id_t                 in_lun_object_id);

//  Verify rdgen I/O results
static void tico_test_verify_io_results(fbe_u32_t   in_rdgen_context_rg_index);

static void tico_add_debug_hook(fbe_object_id_t rg_object_id, 
                                fbe_u32_t state, 
                                fbe_u32_t substate,
                                fbe_u64_t val1,
                                fbe_u64_t val2,
                                fbe_u32_t check_type,
                                fbe_u32_t action);


static void tico_delete_debug_hook(fbe_object_id_t rg_object_id, 
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action);


static void tico_verify_io_quiesced(fbe_object_id_t rg_object_id);


static void tico_test_wait_for_error_injection_complete(
                                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                                        fbe_u16_t                       in_err_record_cnt);

static fbe_status_t tico_wait_for_target_SP_hook_with_val(fbe_object_id_t rg_object_id, 
                                                          fbe_u32_t state, 
                                                          fbe_u32_t substate,
                                                          fbe_u32_t check_type,
                                                          fbe_u64_t val1,
                                                          fbe_u64_t val2);

static void tico_update_bus_enc_slot_view_of_swapped_drive(fbe_test_rg_configuration_t*    in_rg_config_p,
                                                           fbe_u16_t                       in_position);

static fbe_status_t tico_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                 fbe_u32_t state, 
                                                 fbe_u32_t substate);



static void tico_wait_for_rebuild_logging_set_for_position(fbe_object_id_t rg_object_id, fbe_u32_t position_to_validate);

static fbe_u64_t tico_get_hook_hit_count(fbe_object_id_t rg_object_id, 
                                         fbe_u32_t state, 
                                         fbe_u32_t substate,
                                         fbe_u64_t val1,
                                         fbe_u64_t val2,
                                         fbe_u32_t check_type,
                                         fbe_u32_t action);


/*!**************************************************************************
 * tico_test_stop_io
 ***************************************************************************
 * @brief
 *  This function stops I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 * @param in_lun_object_id          - LUN object ID; FBE_OBJECT_ID_INVALID if want to issue I/O to all LUNs
 *
 * @return void
 *
 ***************************************************************************/
static void tico_test_stop_io(     
                              fbe_test_rg_configuration_t*    in_rg_config_p,
                              fbe_u32_t                       in_rdgen_context_rg_index,
                              fbe_lba_t                       in_lba,
                              fbe_block_count_t               in_num_blocks,
                              fbe_rdgen_operation_t           in_rdgen_op,
                              fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &tico_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                              *outstanding_io_p = &tico_outstanding_async_io[in_rdgen_context_rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;


    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {

        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);

        //  Stop a single I/O to the LU object provided as input
        status = fbe_api_rdgen_stop_tests(context_p, 1);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
 
        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);
           
        //  Stop a single I/O to the LU object
        status = fbe_api_rdgen_stop_tests(context_p, 1);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        // Goto the next LUN
        context_p++;
        outstanding_io_p++;
        logical_unit_configuration_p++;
    }

    return;

}   // End tico_test_stop_io()



/*!****************************************************************************
 *  tico_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Tico test for single-SP testing.
 *   It creates the physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_dualsp_setup(void)
{  
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

        /* Based on the test level determine which configuration to use.  */
        if (test_level > 0)
        {
            /* Run test for normal element size.
            */
            rg_config_p = &tico_rg_config_g[0]; 
        }
        else
        {
            rg_config_p = &tico_rg_config_g[0]; 
        }

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        /* initialize the number of extra drive required by each rg which will be use by
           simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
        */
        mut_printf(MUT_LOG_LOW, "=== %s Loading Physical Config on SPA ===", __FUNCTION__);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /*  Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        mut_printf(MUT_LOG_LOW, "=== %s Loading Physical Config on SPB ===", __FUNCTION__);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);


        sep_config_load_sep_and_neit_load_balance_both_sps(); 
	}

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(180000/*second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;

} //  End tico_dualsp_setup()

/*!****************************************************************************
 *  tico_dualsp_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the common cleanup function for the tico test for single-SP testing.
 *   It destroys the physical configuration and unloads the packages.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_dualsp_cleanup(void)
{  
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

} //  End tico_dualsp_cleanup()

/*!****************************************************************************
 *  tico_dualsp_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the tico test for dual-SP testing.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p;   // pointer to the RG test config specs
    
    mut_printf(MUT_LOG_LOW, "=== Tico Dual SP Test starts ===\n");

    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    //  Run tests
    rg_config_p = &tico_rg_config_g[0];
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p,                          
                                                   NULL,                                 
                                                   tico_run_tests,     
                                                   SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
                                                   SEP_REBUILD_UTILS_CHUNKS_PER_LUN);    
    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s Tico Dual SP Test completed successfully ===", __FUNCTION__);

    //  Return 
    return;

} //  End tico_dualsp_test()


/*!****************************************************************************
 *  tico_run_tests
 ******************************************************************************
 *
 * @brief
 *   Collection of tests to test rebuild logging functionality.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_run_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{

#if 0
    //  Run RAID5 tests
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM IOTS restart test with glitch drives\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    tico_run_siots_continue_with_glitch_drive_tests(&config_p[0], NULL);
    
#endif

    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM drive glitch incomplete write test\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");


    tico_run_raid_drive_glitch_incomplete_md_write_test(&config_p[0], NULL);
        

    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM drive fail incomplete write test\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=======num_raid_groups=%d=================\n", fbe_test_get_rg_array_length(config_p));
    
    
    tico_run_raid_drive_fail_incomplete_md_write_test(&config_p[1], NULL);

    
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM retryable error code changing test\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    tico_run_non_retriable_error_tests(&config_p[2], NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM eval_rl test\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    tico_run_avoid_eval_rl_test(&config_p[3], NULL);
    
#ifdef __TICO_MANUALLY__
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: PERFORM clear FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD test\n", __FUNCTION__);
    mut_printf(MUT_LOG_TEST_STATUS, "========================\n");

    tico_run_clear_quiesce_hold_flag_test(&config_p[4], NULL);
#endif
    
    return;
}

/*!****************************************************************************
 * tico_test_config_hotspares()
 ******************************************************************************
 * @brief
 *  This test is used to init the hs config
 *  .
 *
 * @param None.
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
static fbe_status_t tico_test_config_hotspares(fbe_test_raid_group_disk_set_t *disk_set_p,
                                               fbe_u32_t no_of_spares)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_hs_configuration_t hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t hs_index = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "No of spares required for this raid group %d.",no_of_spares); 
        
    for(hs_index = 0; hs_index < no_of_spares; hs_index ++)
    {
       
        hs_config[hs_index].block_size = 520;
        hs_config[hs_index].disk_set = *(disk_set_p + hs_index) ;
        hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;
    }

    status = fbe_test_sep_util_configure_hot_spares(hs_config,no_of_spares);
    
    return status;
}

/*!**************************************************************************
 * tico_test_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to write background pattern
 *
 * @return void
 *
 ***************************************************************************/
static void tico_test_write_background_pattern( 
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index)
{
    fbe_api_rdgen_context_t*                    context_p = &tico_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                                   lun_index;
    fbe_object_id_t                             lun_object_id;
    fbe_test_logical_unit_configuration_t*      logical_unit_configuration_p = NULL;

    /* Send I/O to each LUN in this raid group
     */
    logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        /* Fill each LUN in the raid group will a fixed pattern
         */
        fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                lun_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_ZERO_ONLY,  /* write zeroes */
                                                FBE_LBA_INVALID, /* use max capacity */
                                                SEP_REBUILD_UTILS_ELEMENT_SIZE);

        /* Goto next LUN.
         */
        context_p++;
        logical_unit_configuration_p++;
    }

    /* Run our I/O test.
     */
    context_p = &tico_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "tico background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End tico_test_write_background_pattern()


// disable error inject service
static void tico_test_disable_error_inject(fbe_test_rg_configuration_t* in_rg_config_p);

/*!**************************************************************
 * tico_test_disable_error_inject()
 ****************************************************************
 * @brief
 *  disable error inject service
 * 
 * @param in_rg_config_p    - raid group config information
 *
 * @return None.
 *
 ****************************************************************/
void tico_test_disable_error_inject(fbe_test_rg_configuration_t*           in_rg_config_p)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;
    
    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

    // disable error injection
    status = fbe_api_logical_error_injection_disable_class(in_rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

/*!**************************************************************
 * tico_test_inject_error_record()
 ****************************************************************
 * @brief
 *  Injects an error record
 * 
 * @param in_rg_config_p    - raid group config information
 * @param in_position       - disk position in RG to inject error
 * @param in_lba            - LBA on disk to inject error
 * @param in_blocks         - Blocks on disks to inject error
 * @param in_err_type       - error type to be injected
 * @param in_error_limit    - error times
 * @param in_delay_time     - only used for FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN, unit ms
 * @param in_opcode         - opcode the error injection applied
 * @param in_rg_width       - width of RG
 *
 * @return None.
 *
 ****************************************************************/
static void tico_test_inject_error_record(
                                          fbe_test_rg_configuration_t*           in_rg_config_p,
                                          fbe_u16_t                              in_position,
                                          fbe_lba_t                              in_lba,
                                          fbe_lba_t                              in_blocks,
                                          fbe_u32_t                              in_error_limit,
                                          fbe_u32_t                              in_delay_time, /* for IO delay error */
                                          fbe_api_logical_error_injection_type_t in_err_type,
                                          fbe_u32_t                              in_opcode,
                                          fbe_u16_t                              in_rg_width)
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_api_logical_error_injection_record_t    error_record = {0};
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    
    

    //  Enable the error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    //  Setup error record for error injection
    //  Inject error on given disk position and LBA
    error_record.pos_bitmap = 1 << in_position;                 //  Injecting error on given disk position
    error_record.width      = in_rg_width;                      //  RG width
    error_record.lba        = in_lba;                           //  Physical address to begin errors
    error_record.blocks     = in_blocks;                                //  Blocks to extend errors
    error_record.opcode     = in_opcode;                                //  Opcodes to error
    error_record.err_type   = in_err_type;   // Error type
    error_record.err_mode   = FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT;          // Error mode
    error_record.err_count  = 0;                                //  Errors injected so far
    error_record.err_limit  = in_error_limit;                   //  Limit of errors to inject
    error_record.skip_count = 0;                                //  Limit of errors to skip
    error_record.skip_limit = 0;                                //  Limit of errors to inject
    error_record.err_adj    = 1;                                //  Error adjacency bitmask
    error_record.start_bit  = 0;                                //  Starting bit of an error
    error_record.num_bits   = 0;                                //  Number of bits of an error
    error_record.bit_adj    = 0;                                //  Indicates if erroneous bits adjacent
    error_record.crc_det    = 0;                                //  Indicates if CRC detectable
    
	if (in_err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN
		|| in_err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_UP)
	{
        error_record.err_limit  = in_delay_time;
	}

    //  Create error injection record
    status = fbe_api_logical_error_injection_create_record(&error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Enable error injection on the RG
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection: pos %d, lba 0x%llx, bloks %llu \n", __FUNCTION__, in_position, in_lba, in_blocks);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Check error injection stats
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
#if 0
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);
#endif

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    return;

}   // End tico_test_inject_error_record()

/*!**************************************************************************
 * tico_test_start_one_io
 ***************************************************************************
 * @brief
 *  This function issues I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 * @param in_lun_object_id          - LUN object ID; FBE_OBJECT_ID_INVALID if want to issue I/O to all LUNs
 *
 * @return void
 *
 ***************************************************************************/
static void tico_test_start_one_io(     
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    fbe_u32_t                       in_rdgen_context_rg_index,
                                    fbe_lba_t                       in_lba,
                                    fbe_block_count_t               in_num_blocks,
                                    fbe_rdgen_operation_t           in_rdgen_op,
                                    fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &tico_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                              *outstanding_io_p = &tico_outstanding_async_io[in_rdgen_context_rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;



    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {
        // Validate that asyncrounous I/O has been stopped
        MUT_ASSERT_INT_EQUAL(0, *outstanding_io_p);

        //  Send a single I/O to the LU object provided as input
        status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                                  in_lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  in_rdgen_op,
                                                  FBE_RDGEN_LBA_SPEC_FIXED,
                                                  in_lba,
                                                  in_num_blocks,
                                                  FBE_RDGEN_OPTIONS_INVALID);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        // Increment the outstanding I/O count.
        *outstanding_io_p = *outstanding_io_p + 1;
        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
   
        // Validate that asyncrounous I/O has been stopped
        MUT_ASSERT_INT_EQUAL(0, *outstanding_io_p);
         
        //  Send a single I/O to the LU object
        status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  in_rdgen_op,
                                                  FBE_RDGEN_LBA_SPEC_FIXED,
                                                  in_lba,
                                                  in_num_blocks,
                                                  FBE_RDGEN_OPTIONS_INVALID);
    
        //  Make sure the I/O was sent successfully
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Increment the outstanding I/O count.
        *outstanding_io_p = *outstanding_io_p + 1;

        // Goto the next LUN
        context_p++;
        outstanding_io_p++;
        logical_unit_configuration_p++;
    }

    return;

}   // End tico_test_start_one_io()

/*!**************************************************************
 * tico_add_debug_hook()
 ****************************************************************
 * @brief
 *  Add the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void tico_add_debug_hook(fbe_object_id_t rg_object_id, 
                                fbe_u32_t state, 
                                fbe_u32_t substate,
                                fbe_u64_t val1,
                                fbe_u64_t val2,
                                fbe_u32_t check_type,
                                fbe_u32_t action)

{
    fbe_status_t    status;
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook: rg obj id 0x%X state %d substate %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_add_debug_hook(rg_object_id, 
                                              state, 
                                              substate, 
                                              val1, 
                                              val2, 
                                              check_type, 
                                              action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}

/*!**************************************************************
 * tico_delete_debug_hook()
 ****************************************************************
 * @brief
 *  Delete the debug hook for the given state and substate.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return None 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void tico_delete_debug_hook(fbe_object_id_t rg_object_id, 
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action)
{
    fbe_status_t                        status = FBE_STATUS_OK;

    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook: rg obj id: 0x%X state: %d, substate: %d", rg_object_id, state, substate);

    status = fbe_api_scheduler_del_debug_hook(rg_object_id, 
                                              state, 
                                              substate, 
                                              val1, 
                                              val2, 
                                              check_type, 
                                              action);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return;
}

/*!**************************************************************************
 * tico_test_cleanup_io
 ***************************************************************************
 * @brief
 *  This function cleansup I/O to the LUNs in the RAID group using rdgen.
 * 
 * @param in_rg_config_p            - raid group config information 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 * @param in_lba                    - LBA to start I/O from
 * @param in_num_blocks             - number of blocks to issue I/O
 * @param in_rdgen_op               - rdgen operation (read,write,etc.)
 * @param in_lun_object_id          - LUN object ID; FBE_OBJECT_ID_INVALID if want to issue I/O to all LUNs
 *
 * @return void
 *
 ***************************************************************************/
static void tico_test_cleanup_io(     
                                 fbe_test_rg_configuration_t*    in_rg_config_p,
                                 fbe_u32_t                       in_rdgen_context_rg_index,
                                 fbe_lba_t                       in_lba,
                                 fbe_block_count_t               in_num_blocks,
                                 fbe_rdgen_operation_t           in_rdgen_op,
                                 fbe_object_id_t                 in_lun_object_id)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t                *context_p = &tico_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_u32_t                              *outstanding_io_p = &tico_outstanding_async_io[in_rdgen_context_rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;


    //  See if the user only wants to issue I/O to a particular LUN in the RG
    if (FBE_OBJECT_ID_INVALID != in_lun_object_id)
    {

        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);

        // Destroy all the contexts (even if we got an error).
        status = fbe_api_rdgen_test_context_destroy(context_p);
    
        // Make sure the I/O was cleaned up successfully.
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        return;
    }

    //  If got this far, user wants to issue I/O to all LUNs in the RG
    for (lun_index = 0; lun_index < SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP; lun_index++)
    {
        //  Get the LUN object ID of a LUN in the RG
        logical_unit_configuration_p = in_rg_config_p->logical_unit_configuration_list;
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
 
        // Validate that there is (1) outstanding async I/O
        MUT_ASSERT_INT_EQUAL(1, *outstanding_io_p);
     
        // Destroy all the contexts (even if we got an error).
        status = fbe_api_rdgen_test_context_destroy(context_p);
    
        // Make sure the I/O was cleaned up successfully.
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);              

        // Decrement the outstanding I/O count
        *outstanding_io_p = *outstanding_io_p - 1;

        // Goto the next LUN
        context_p++;
        outstanding_io_p++;
        logical_unit_configuration_p++;
    }

    return;

}   // End tico_test_cleanup_io()

/*!**************************************************************************
 * tico_test_verify_io_results
 ***************************************************************************
 * @brief
 *  This function verifies the results of issuing I/O to the LUNs in a RG
 *  using rdgen.
 * 
 * @param in_rdgen_context_rg_index - index in context array to issue I/O
 *
 * @return void
 *
 ***************************************************************************/
static void tico_test_verify_io_results(
                                        fbe_u32_t   in_rdgen_context_rg_index)
{
    fbe_api_rdgen_context_t*                context_p = &tico_test_rdgen_context_g[in_rdgen_context_rg_index][0];
    fbe_payload_block_operation_status_t    block_op_status;
    fbe_payload_block_operation_qualifier_t block_op_qualifier;
    fbe_status_t                            status;


    //  Wait for the IO's to complete before checking the status
    status = fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Make sure the rdgen status is success
    MUT_ASSERT_INT_EQUAL(context_p->status, FBE_STATUS_OK);

    //  Make sure the I/O status is success
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK); 
    
    //  Make sure the block status is sucess
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate rdgen I/O ==\n", __FUNCTION__);
    block_op_status = context_p->start_io.status.block_status;

    if (block_op_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                       __FUNCTION__, context_p->start_io.status.block_status);
        MUT_FAIL_MSG("block status from rdgen was unexpected");
    }

    //  Make sure the block qualifier is success
    block_op_qualifier = context_p->start_io.status.block_qualifier;
    if (block_op_qualifier != FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                       __FUNCTION__, context_p->start_io.status.block_status);
        MUT_FAIL_MSG("block qualifier status from rdgen was unexpected");
    }

    return;

}   // End tico_test_verify_io_results()



static void tico_verify_io_quiesced(fbe_object_id_t rg_object_id)
{
    fbe_u32_t           sleep_count;
    fbe_u32_t           sleep_period_ms;
    fbe_u32_t           max_sleep_count;
    fbe_status_t        status;
    

    max_sleep_count = 10;
    sleep_period_ms = 1000;

    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        fbe_api_raid_group_get_io_info_t rg_io_info;

        // Sleep 
        fbe_api_sleep(sleep_period_ms);

        status = fbe_api_raid_group_get_io_info(rg_object_id, &rg_io_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, 
                   "tico test:  rg_io_info.outstanding_io_count = %d, rg_io_info.quiesced_io_count = %d\n", rg_io_info.outstanding_io_count, rg_io_info.quiesced_io_count);
        
        // TODO: how to confirm the SIOTS has been quiesced except delay???
        if (rg_io_info.outstanding_io_count == 1)
        {
            return;
        }

    }
    
    status = FBE_STATUS_GENERIC_FAILURE;
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  timeout of waiting for IO to be quienced\n");
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * tico_test_wait_for_error_injection_complete()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete
 *
 * @param   in_rg_config_p      - raid group config information
 * @param   in_err_record_cnt   - number of errors injected
 *
 * @return  None
 *
 ****************************************************************/
static void tico_test_wait_for_error_injection_complete(
                                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                                        fbe_u16_t                       in_err_record_cnt)
{
    fbe_status_t                                        status;
    fbe_u32_t                                           sleep_count;
    fbe_u32_t                                           sleep_period_ms;
    fbe_u32_t                                           max_sleep_count;
    fbe_object_id_t                                     rg_object_id;
    fbe_api_logical_error_injection_get_stats_t         stats; 
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    //  Get RG object ID
    fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    

    //  Intialize local variables
    sleep_period_ms = 1;
    max_sleep_count = (SEP_REBUILD_UTILS_MAX_ERROR_INJECT_WAIT_TIME_SECS * 1000) / sleep_period_ms;


    //  Wait for errors to be injected
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);
    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(stats.num_errors_injected >= in_err_record_cnt)
        {
            // Break out of the loop
            mut_printf(MUT_LOG_TEST_STATUS, "== %s found %llu errors injected ==\n", __FUNCTION__, (unsigned long long)stats.num_errors_injected);
            break;
        }
                        
        // Sleep 
        fbe_api_sleep(sleep_period_ms);
    }

    //  The error injection took too long and timed out
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    //  Disable the error injection service
#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    tico_test_disable_error_inject(config_p);
#endif

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;

} // End tico_test_wait_for_error_injection_complete()

/*!****************************************************************************
 *  tico_run_siots_continue_with_glitch_drive_tests
 ******************************************************************************
 *
 * @brief
 *   Perform the test case in which we will continue SIOTS instead of restarting it after glitch drive back
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_run_siots_continue_with_glitch_drive_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t        status;
    fbe_api_terminator_device_handle_t      drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_2;                   //  drive info needed for reinsertion 
    fbe_u32_t                               first_rdgen_context_index;      //  rdgen context for first injection
    fbe_u32_t                               second_rdgen_context_index;     //  rdgen context for second injection
    fbe_object_id_t                         lun_object_id;                  //  object ID of a LUN in the RG
    fbe_test_logical_unit_configuration_t*  logical_unit_configuration_p = NULL;
    fbe_object_id_t                         raid_group_id;

    status = fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    
    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test : Try to reproduce incomplete write on a degraded RG, (r5 with Disk 5 dead), caused by drive glitch on Disk 6
    // 
    //    Disk configuration for this RAID-5 RAID Group:
    //
    //                      Disk 4      Disk 5      Disk 6
    //                      ------      ------      ------
    //                      Parity      Data        Data
    //                      Parity      Data        Data
    //    NOTES:
    //          Disk        Position in RG
    //          ----        --------------
    //          4           0
    //          5           1
    //          6           2
    
    // Make sure verify is not running before we begin this test
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, TICO_WAIT_FOR_VERIFY_SEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step1: remove disk 5 to let RG get into degraded mode
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  remove disks on position 1 -- Disk 5\n");
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(config_p, SEP_REBUILD_UTILS_FIRST_REMOVED_DRIVE_POS, &drive_info_1);

    // step2: to simulate drive glitch scenario 
    //     a. inject TIME_OUT error on Disk 5
    //     b. send one IO to disk 5
    //     c. enable Hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN  
    //     d. disable TIME_OUT error injection
    //     e. remove/reinsert disk 5, and make sure the PVD is ready
    //     f. remove Hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  inject error on position 2 -- Disk 6\n");
    //  Disable error injection records in the error injection table (initializes records)
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // inject write error on position 2
    tico_test_inject_error_record(config_p, 
                                  2,  /* position */
                                  0,  /* lba */
                                  1,  /* blocks */
                                  1,  /* error_limit */
                                  0,  /* delay */
                                  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR, 
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                  SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  send one single io to position 2 -- Disk 6\n");
    
    //  Get the object ID of the first LUN in the RG in LBA order. Only need the first LUN in the RG for this test.
    logical_unit_configuration_p = config_p->logical_unit_configuration_list;
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    //  Try to write a new data pattern to the first LUN in the RG in LBA order at Host LBA 0 for 1 block.
    //  Note that the error was injected on disk position 2, disk LBA 0; this translates to Host LBA 0.
    first_rdgen_context_index = 0;
    second_rdgen_context_index = 1;
    tico_test_start_one_io(config_p, first_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    //  Wait for error injections to complete; injected 1 error here
    tico_test_wait_for_error_injection_complete(config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  IO has been quiesced\n");
    //  make sure this IO has been quiesced
    tico_verify_io_quiesced(raid_group_id);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  disable TIME_OUT error injection on position 2\n");
    // disable TIME_OUT error injection on pos 2 
    status = fbe_api_logical_error_injection_disable_object(raid_group_id, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  enable dbg hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN\n");
    // set the debug hook which prevent to go rg in fail state during eval RL 
    tico_add_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  remove the disk on position 2\n");
    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 2;
    sep_rebuild_utils_remove_drive_and_verify(config_p, 2, sep_rebuild_utils_number_physical_objects_g, &drive_info_2);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  reinsert the disk on position 2\n");
    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 2;
    sep_rebuild_utils_reinsert_drive_and_verify(config_p, 2, sep_rebuild_utils_number_physical_objects_g, &drive_info_2);

    
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  disable dbg hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN\n");
    // set the debug hook which prevent to go rg in fail state during eval RL 
    tico_delete_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    
    
    // make sure the IO is restarted, (by checking the restarted hook?)
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  waiting for the IO to finish\n");
    //  Verify results
    tico_test_verify_io_results(first_rdgen_context_index);
    

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  stop the single io to position 2 -- Disk 6\n");
    tico_test_stop_io(config_p, first_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);
#endif
    
    // read on Disk 6 got a media error
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  reading on position 1, expect get a media error\n");
    // Try to read data from the first LUN in the RG in LBA order at Host LBA 128 for 1 block
    tico_test_start_one_io(config_p, second_rdgen_context_index, 128, 1, FBE_RDGEN_OPERATION_READ_ONLY, lun_object_id);
    //  Verify results
    tico_test_verify_io_results(second_rdgen_context_index);
    
#if 0
    tico_test_stop_io(raid_group_id, first_rdgen_context_index, 256, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);
#endif
    
    // I/o is stopped (we waited for it) but we must cleanup
    tico_test_cleanup_io(config_p, first_rdgen_context_index, 0, 1, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);
    tico_test_cleanup_io(config_p, second_rdgen_context_index, 128, 1, FBE_RDGEN_OPERATION_READ_ONLY, lun_object_id);
    
    //  Return 
    return;
} //  End tico_run_siots_continue_with_glitch_drive_tests()

/*!**************************************************************
 * tico_wait_for_target_SP_hook_with_val()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor       
 *        check_type - debug hook type
 *        val1       - rebuild checkpoint
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  9/15/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
static fbe_status_t tico_wait_for_target_SP_hook_with_val(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate,
                                                   fbe_u32_t check_type,
                                                   fbe_u64_t val1,
                                                   fbe_u64_t val2) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = 60000;
    
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = check_type;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);

            status = fbe_api_sim_transport_set_target_server(current_target);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state: %d substate: %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end tico_wait_for_target_SP_hook()
 ******************************************/


/*!**************************************************************
 * tico_wait_for_target_SP_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t tico_wait_for_target_SP_hook(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t state, 
                                                   fbe_u32_t substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = TICO_WAIT_MSEC;
    
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. rg obj id: 0x%X state: %d substate: %d.", 
               rg_object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook %d times", (int)hook.counter);

            status = fbe_api_sim_transport_set_target_server(current_target);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

            return status;
        }
        current_time = current_time + 200;
        fbe_api_sleep (200);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object %d did not hit state: %d substate: %d in %d ms!\n", 
                  __FUNCTION__, rg_object_id, state, substate, timeout_ms);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end tico_wait_for_target_SP_hook()
 ******************************************/

/*!****************************************************************************
 *  tico_update_bus_enc_slot_view_of_swapped_drive
 ******************************************************************************
 *
 * @brief
 *   update the bus, enclosure, slot num of the new swapped in drives
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_update_bus_enc_slot_view_of_swapped_drive(fbe_test_rg_configuration_t*    in_rg_config_p,
                                                    fbe_u16_t                       in_position)
{
    fbe_edge_index_t                edge_index;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_status_t                    status;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_u32_t                       bus, encl, slot;

    /* Get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(in_rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, in_position, &vd_object_id);

    /* Get the edge index where the permanent spare swaps in. */
    fbe_test_sep_drive_get_permanent_spare_edge_index(vd_object_id, &edge_index);
    
    /* Get the spare edge information for reporting purposes. */
    status = fbe_api_get_block_edge_info(vd_object_id,
                                         edge_index,
                                         &edge_info,
                                         FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_location(edge_info.server_id, &bus, &encl, &slot);
    in_rg_config_p->rg_disk_set[in_position].bus = bus;
    in_rg_config_p->rg_disk_set[in_position].enclosure = encl;
    in_rg_config_p->rg_disk_set[in_position].slot = slot;

    return;
}


/*!****************************************************************************
 *  tico_run_raid_drive_glitch_incomplete_md_write_test
 ******************************************************************************
 *
 * @brief
 *   run the test case in whitch we will glitch one drive and fail one paged metada write operation, then observe whether the IW_VERIY is marked for MD
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_run_raid_drive_glitch_incomplete_md_write_test(fbe_test_rg_configuration_t* config_p, void *context_p)
{
    fbe_status_t        status;
    fbe_api_terminator_device_handle_t      drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_2;                   //  drive info needed for reinsertion 
    fbe_object_id_t                         raid_group_id;
    fbe_api_raid_group_get_info_t           raid_group_info;
    
    
    // get the RG number
    status = fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // get the RG info
    status = fbe_api_raid_group_get_info(raid_group_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    // open some debug flags for this RG
    status = fbe_api_raid_group_set_group_debug_flags(raid_group_id, tico_doo_object_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test : run the test case in whitch we will glitch one drive and fail one paged metada write operation, then observe whether the IW_VERIY is marked for MD
    // 
    //          Disk configuration for this RAID-6 RAID Group:
    //
    //
    //                      Disk 4      Disk 5      Disk 6      Disk 7      Disk 8   Disk 9    
    //                      ------      ------      ------      ------      ------   ------
    //                      Parity      Parity      Data        Data        Data     Data
    //                      Parity      Parity      Data        Data        Data     Data
    //    NOTES:
    //          Disk        Position in RG
    //          ----        --------------
    //          4           0
    //          5           1
    //          6           2
    //          7           3
    //          8           4
    //          9           5
    // 
    //          The position of the disk in the RAID Group is passed to the error injection functions.

    // step1: fail the drive on pos 3 to let RG run on degraded mode
    // step2: add hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block rebuilding process
    // step3: swap in a new drive
    // step4: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    // step5: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for write operation on pos5 
    // step6: del hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start rebuilding process
    // step7: make sure at least one hit on the error record 
    // step8: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive glitch case
    // step9: glitch the drive on pos 5 by pulling out and pulling in the drive
    // step10: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to start the EVAL_RL
    // step11: check the incomplete_write_verify_checkpoint after EVAL_RL done


    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  wait unit all the background verify has been finished\n");
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, TICO_WAIT_FOR_VERIFY_SEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    // step1: fail the drive on pos 3 to let RG run on degraded mode
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  degrade the RG by failing the drive on pos 3\n");
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(config_p, 3, &drive_info_1);

    // step2: add hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block rebuilding process
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   add the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block the rebuild\n");
    tico_add_debug_hook(raid_group_id,
                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                 0, /* checkpoint */
                                 NULL,
                                 SCHEDULER_CHECK_VALS_LT,
                                 SCHEDULER_DEBUG_ACTION_PAUSE);
    
    // step3: swap in a new drive
    // Set the spare trigger timer 1 second to swap in immediately.
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /*second */);
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   swap in a new drive\n");
    sep_rebuild_utils_setup_hot_spare(config_p->extra_disk_set[0].bus,
                                      config_p->extra_disk_set[0].enclosure ,
                                      config_p->extra_disk_set[0].slot); 
    fbe_test_sep_drive_wait_permanent_spare_swap_in(config_p, 3);

    // update the rg configure view of the new swapped drive
    tico_update_bus_enc_slot_view_of_swapped_drive(config_p, 3);

    // restore the spare time to prevent swapping 
    fbe_test_sep_util_update_permanent_spare_trigger_timer(18000/*second */);
    
    // step4: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY\n");
    tico_wait_for_target_SP_hook_with_val(raid_group_id,
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                          FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                          SCHEDULER_CHECK_VALS_LT, 
                                          0, /* checkpoint */
                                          NULL); 
    
    // step5: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for write operation on pos5 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR error for write operation on pos5\n");
    tico_test_inject_error_record(config_p, 
                                  5,  /* position */
                                  raid_group_info.paged_metadata_start_lba / raid_group_info.num_data_disk,  /* lba */
                                  raid_group_info.paged_metadata_capacity / raid_group_info.num_data_disk,  /* blocks */
                                  1,  /* error_limit */
                                  6000,  /* delay */
                                  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                  SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH);

    // step6: del hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start rebuilding process
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start the rebuild\n");
    tico_delete_debug_hook(raid_group_id,
                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                    FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                    0, /* checkpoint */
                                    NULL,
                                    SCHEDULER_CHECK_VALS_LT,
                                    SCHEDULER_DEBUG_ACTION_PAUSE);   

    // step7: make sure at least one hit on the error record 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   make sure at least one hit on the error record\n");
    tico_test_wait_for_error_injection_complete(config_p, 1);

    // step8: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive glitch case
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive glitch case\n");
    tico_add_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    

    // step9: glitch the drive on pos 5 by pulling out and pulling in the drive
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   glitch the drive on pos 5 by pulling out and pulling in the drive\n");
    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(config_p, 5, sep_rebuild_utils_number_physical_objects_g, &drive_info_2);

    fbe_api_sleep(7000);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   glitch the drive on pos 5 by pulling out and pulling in the drive\n");
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(config_p, 5, sep_rebuild_utils_number_physical_objects_g, &drive_info_2);
    
    // step10: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED\n");
    tico_delete_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    


    // step12: check the glitch_drive_bitmask
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   check the glitch_drive_bitmask\n");
    tico_add_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    

    tico_wait_for_target_SP_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE);

    // get the RG info
    status = fbe_api_raid_group_get_info(raid_group_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   incomplete_write_verify_checkpoint = 0x%llx\n", raid_group_info.incomplete_write_verify_checkpoint);
    MUT_ASSERT_TRUE_MSG(raid_group_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID, "INCOMPLETE_WRITE checkpoint failed");

    // disable error injection
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del the error record\n");
    tico_test_disable_error_inject(config_p);

    fbe_api_sleep(10000);

    return;
}


/*!****************************************************************************
 *  tico_run_raid_drive_fail_incomplete_md_write_test
 ******************************************************************************
 *
 * @brief
 *   run the test case in whitch we will fail one drive and fail one paged metada write operation, then observe whether the IW_VERIY is marked for MD
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_run_raid_drive_fail_incomplete_md_write_test(fbe_test_rg_configuration_t* config_p, void *context_p)
{
    fbe_status_t        status;
    fbe_api_terminator_device_handle_t      drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_2;                   //  drive info needed for reinsertion 
    fbe_object_id_t                         raid_group_id;
    fbe_api_raid_group_get_info_t           raid_group_info;
    
    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);

    // get the RG number
    status = fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // get the RG info
    status = fbe_api_raid_group_get_info(raid_group_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    // open some debug flags for this RG
    status = fbe_api_raid_group_set_group_debug_flags(raid_group_id, tico_doo_object_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test : run the test case in whitch we will fail one drive and fail one paged metada write operation, then observe whether the IW_VERIY is marked for MD
    // 
    //          Disk configuration for this RAID-6 RAID Group:
    //
    //
    //                      Disk 4      Disk 5      Disk 6      Disk 7      Disk 8   Disk 9    
    //                      ------      ------      ------      ------      ------   ------
    //                      Parity      Parity      Data        Data        Data     Data
    //                      Parity      Parity      Data        Data        Data     Data
    //    NOTES:
    //          Disk        Position in RG
    //          ----        --------------
    //          4           0
    //          5           1
    //          6           2
    //          7           3
    //          8           4
    //          9           5
    // 
    //          The position of the disk in the RAID Group is passed to the error injection functions.

    // step1: fail the drive on pos 3 to let RG run on degraded mode
    // step2: add hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block rebuilding process
    // step3: swap in a new drive
    // step4: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    // step5: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR error for write operation on pos5 
    // step6: del hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start rebuilding process
    // step7: make sure at least one hit on the error record 
    // step8: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive dead case
    // step9: fail the drive on pos 5 by pulling out and pulling in the drive
    // step10: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to continue the EVAL_RL
    // step11: check the rebuild_checkpoint

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  wait unit all the background verify has been finished\n");
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, TICO_WAIT_FOR_VERIFY_SEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step1: fail the drive on pos 3 to let RG run on degraded mode
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  degrade the RG by failing the drive on pos 3\n");
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(config_p, 3, &drive_info_1);

    // step2: add hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block rebuilding process
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   add the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block the rebuild\n");
    tico_add_debug_hook(raid_group_id,
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                        FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                        0, /* checkpoint */
                        NULL,
                        SCHEDULER_CHECK_VALS_LT,
                        SCHEDULER_DEBUG_ACTION_PAUSE);

    // step3: swap in a new drive
    // Set the spare trigger timer 1 second to swap in immediately.
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /*second */);
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   swap in a new drive\n");
    sep_rebuild_utils_setup_hot_spare(config_p->extra_disk_set[0].bus,
                                      config_p->extra_disk_set[0].enclosure ,
                                      config_p->extra_disk_set[0].slot); 
    fbe_test_sep_drive_wait_permanent_spare_swap_in(config_p, 3);

    // update the rg configure view of the new swapped drive
    tico_update_bus_enc_slot_view_of_swapped_drive(config_p, 3);

    // restore the spare time to prevent swapping 
    fbe_test_sep_util_update_permanent_spare_trigger_timer(18000/*second */);
    
    // step4: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY\n");
    tico_wait_for_target_SP_hook_with_val(raid_group_id,
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                          FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                          SCHEDULER_CHECK_VALS_LT, 
                                          0, /* checkpoint */
                                          NULL); 
    
    // step5: inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for write operation on pos5 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR error for write operation on pos5\n");
    tico_test_inject_error_record(config_p, 
                                  5,  /* position */
                                  raid_group_info.paged_metadata_start_lba / raid_group_info.num_data_disk,  /* lba */
                                  raid_group_info.paged_metadata_capacity / raid_group_info.num_data_disk,  /* blocks */
                                  1,  /* error_limit */
                                  6000,  /* delay */
                                  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                  SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH);

    // step6: del hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start rebuilding process
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start the rebuild\n");
    tico_delete_debug_hook(raid_group_id,
                           SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                           0, /* checkpoint */
                           NULL,
                           SCHEDULER_CHECK_VALS_LT,
                           SCHEDULER_DEBUG_ACTION_PAUSE);   

    // step7: make sure at least one hit on the error record 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   make sure at least one hit on the error record\n");
    tico_test_wait_for_error_injection_complete(config_p, 1);

    // step8: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive glitch case
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to pave the way for simulating drive glitch case\n");
    tico_add_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    

    // step9: fail the drive on pos 5 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   fail the drive on pos 5\n");
    //  Adjust the number of physical objects expected
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(config_p, 5, sep_rebuild_utils_number_physical_objects_g, &drive_info_2);

    fbe_api_sleep(7000);

#if 0
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   glitch the drive on pos 5 by pulling out and pulling in the drive\n");
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(config_p, 5, sep_rebuild_utils_number_physical_objects_g, &drive_info_2);
#endif
    
    // step10: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to continue the EVAL_RL
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED to continue the EVAL_RL\n");
    tico_delete_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    
    // step11: check the rebuild_checkpoint
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   check the rebuild checkpoint on pos 5\n");
    tico_add_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_PAUSE);
    

    tico_wait_for_target_SP_hook(raid_group_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE);

    // get the RG info
    status = fbe_api_raid_group_get_info(raid_group_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   rebuild_checkpoint = 0x%llx\n", raid_group_info.incomplete_write_verify_checkpoint);
    MUT_ASSERT_TRUE_MSG(raid_group_info.rebuild_checkpoint[5] != FBE_LBA_INVALID, "check rebuild checkpoint failed");

    // disable error injection
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del the error record\n");
    tico_test_disable_error_inject(config_p);

    fbe_api_sleep(10000);

    return;
}

/*!****************************************************************************
 *  tico_run_non_retriable_error_tests
 ******************************************************************************
 *
 * @brief
 *   Perform the test case in which we will populate the retryable error to parent siots when nested_siots failed
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_run_non_retriable_error_tests(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t        status;
    fbe_api_terminator_device_handle_t      drive_info_1;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_2;                   //  drive info needed for reinsertion 
    fbe_api_terminator_device_handle_t      drive_info_3;                   //  drive info needed for reinsertion 
    fbe_object_id_t                         raid_group_id;
    fbe_api_raid_group_get_info_t           raid_group_info;
    
    //  Get the RG object ID    
    fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, raid_group_id);


    status = tico_test_config_hotspares(config_p->extra_disk_set, 2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    
    // get the RG info
    status = fbe_api_raid_group_get_info(raid_group_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    // open some debug flags for this RG
    status = fbe_api_raid_group_set_group_debug_flags(raid_group_id, tico_doo_object_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Get the number of physical objects in existence at this point in time.  This number is 
    //  used when checking if a drive has been removed or has been reinserted.   
    status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Test : Try to reproduce paged metadata access failed with NON retryable error
    // 
    //          Disk configuration for this RAID-6 RAID Group:
    //
    //
    //                      Disk 4      Disk 5      Disk 6      Disk 7      Disk 8   Disk 9    
    //                      ------      ------      ------      ------      ------   ------
    //                      Parity      Parity      Data        Data        Data     Data
    //                      Parity      Parity      Data        Data        Data     Data
    //    NOTES:
    //          Disk        Position in RG
    //          ----        --------------
    //          4           0
    //          5           1
    //          6           2
    //          7           3
    //          8           4
    //          9           5
    // 
    //          The position of the disk in the RAID Group is passed to the error injection functions.

    // step1: degrade the RG by failing the drive on pos 3
    // step2: wait unit all the background verify has been finished
    // step3: add the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block the rebuild
    // step4: swap in a new drive 
    // step5: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    // step6: pull out the drive on pos 5
    // step7: inject a FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for read on pos 3, on the paged metadata region
    // step8: del the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start the rebuild
    // step9: waiting for the the error injection hits 2 times, we need to remove the drive after pending the fruts IO
    //        to simuate the drive dead error. 
    //        the 1st time is the case reading chunk info; the 2nd time is the case updating the MD
    // step10: remove the drive again on pos3 to simulate the drive dead case
    // step11: make sure RG not change to ACTIVATE state



    // step2: wait unit all the background verify has been finished
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  wait unit all the background verify has been finished\n");
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, TICO_WAIT_FOR_VERIFY_SEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // step1: degrade the RG by failing the drive on pos 3
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  degrade the RG by failing the drive on pos 3\n");
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(config_p, 3, &drive_info_1);
    
    // step3: add the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block the rebuild
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   add the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to block the rebuild\n");
    tico_add_debug_hook(raid_group_id,
                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                 0, /* checkpoint */
                                 NULL,
                                 SCHEDULER_CHECK_VALS_LT,
                                 SCHEDULER_DEBUG_ACTION_PAUSE);

    // step4: swap in a new drive 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   swap in a new drive\n");
    fbe_api_control_automatic_hot_spare(TRUE);
    // Set the spare trigger timer 1 second to swap in immediately.
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1 /*second */);
    sep_rebuild_utils_setup_hot_spare(config_p->extra_disk_set[0].bus,
                                      config_p->extra_disk_set[0].enclosure ,
                                      config_p->extra_disk_set[0].slot); 

    fbe_test_sep_drive_wait_permanent_spare_swap_in(config_p, 3);
    // update the rg configure view of the new swapped drive
    tico_update_bus_enc_slot_view_of_swapped_drive(config_p, 3);

    // restore the spare time to prevent swapping 
    fbe_test_sep_util_update_permanent_spare_trigger_timer(18000/*second */);

    // step5: wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   wait for hitting the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY\n");
    tico_wait_for_target_SP_hook_with_val(raid_group_id,
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                          FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                          SCHEDULER_CHECK_VALS_LT, 
                                          0, /* checkpoint */
                                          NULL); 
    
    // step6: pull out the drive on pos 5
    sep_rebuild_utils_start_rb_logging_r5_r3_r1(config_p, 5, &drive_info_2);

    // step7: inject a FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for read on pos 3, on the paged metadata region
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   inject a FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for read on pos 3\n");
    tico_test_inject_error_record(config_p, 
                                  3,  /* position */
                                  raid_group_info.paged_metadata_start_lba / raid_group_info.num_data_disk,  /* lba */
                                  raid_group_info.paged_metadata_capacity / raid_group_info.num_data_disk,  /* blocks */
                                  2,  /* error_limit, useless in this case */
                                  6000,  /* delay */
                                  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                  SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH);
    
    // step8: del the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start the rebuild
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   del the hook FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY to start the rebuild\n");
    tico_delete_debug_hook(raid_group_id,
                           SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                           0, /* checkpoint */
                           NULL,
                           SCHEDULER_CHECK_VALS_LT,
                           SCHEDULER_DEBUG_ACTION_PAUSE);   

    // step9: waiting for the the error injection hits 2 times, we need to remove the drive after pending the fruts IO
    //        to simuate the drive dead error. 
    //        the 1st time is the case reading chunk info; the 2nd time is the case updating the MD
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   waiting for the the error injection hits 2 times\n");
    tico_test_wait_for_error_injection_complete(config_p, 2);


    // step10: remove the drive again on pos3
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   remove the drive again on pos3\n");
    fbe_api_sleep(4500);
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g - 1;
    sep_rebuild_utils_remove_drive_and_verify(config_p, 3, sep_rebuild_utils_number_physical_objects_g, &drive_info_3);

#if 1
    fbe_api_sleep(6000);
    // disable error injection
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    tico_test_disable_error_inject(config_p);

    //  Adjust the number of physical objects expected 
    sep_rebuild_utils_number_physical_objects_g = sep_rebuild_utils_number_physical_objects_g + 1;
    sep_rebuild_utils_reinsert_drive_and_verify(config_p, 3, sep_rebuild_utils_number_physical_objects_g, &drive_info_3);
#endif
    
    // make sure RG not change to ACTIVATE state

    // clean up
    fbe_api_sleep(10000);

    
    return;
}

void tico_wait_for_rebuild_logging_set_for_position(fbe_object_id_t rg_object_id, fbe_u32_t position_to_validate)
{
    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;

    max_retries = 60;

    for (count = 0; count < max_retries; count++)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* For the position specified check if rebuild logging is cleared
         */ 
        if (raid_group_info.b_rb_logging[position_to_validate] == FBE_TRUE)
        {
            /* No rebuild logging set, waiting is done, just return.
             */
            return;
        }

        /* Continue waiting for rb logging.
         */
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== rebuild logging not set for rg_id: 0x%x pos: %d ==", 
               rg_object_id, position_to_validate);
    status = FBE_STATUS_TIMEOUT;
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}

/*!**************************************************************
 * tico_get_hook_hit_count()
 ****************************************************************
 * @brief
 *  check the hook hit count
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  1/19/2012 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_u64_t tico_get_hook_hit_count(fbe_object_id_t rg_object_id, 
                                     fbe_u32_t state, 
                                     fbe_u32_t substate,
                                     fbe_u64_t val1,
                                     fbe_u64_t val2,
                                     fbe_u32_t check_type,
                                     fbe_u32_t action)
                                                   
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    
    fbe_sim_transport_connection_target_t current_target;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_sim_transport_connection_target_t passive_sp;

    // Preserve current target.        
    current_target = fbe_api_sim_transport_get_target_server();

    /* Get the active/passive info w.r.t the rg */
    fbe_test_sep_util_get_active_passive_sp(rg_object_id,
                                            &active_sp,
                                            &passive_sp);

    status = fbe_api_sim_transport_set_target_server(active_sp);

    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = rg_object_id;
    hook.check_type = check_type;
    hook.action = action;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.counter = 0;


    status = fbe_api_scheduler_get_debug_hook(&hook);

    mut_printf(MUT_LOG_TEST_STATUS, "get the hook status. rg obj id: 0x%X state: %d substate: %d. hook_count: %lld", rg_object_id, state, substate, hook.counter);

    status = fbe_api_sim_transport_set_target_server(current_target);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);      

    return hook.counter;
}


/*!****************************************************************************
 *  tico_run_avoid_eval_rl_test
 ******************************************************************************
 *
 * @brief
 *   run the test case in which we will check the eval_rl will not run when the removed drive comes back on the pos, 
 *   on which the rebuild logging has already started
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void tico_run_avoid_eval_rl_test(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t                            status;
    fbe_u32_t number_physical_objects;
    fbe_u32_t remove_pos = 0;
    fbe_api_terminator_device_handle_t      drive_info_1;
    fbe_object_id_t raid_group_id;
    fbe_u64_t hook_hit_count;
    
    
    // get the RG number
    status = fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    // remove the drive on pos 0
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: Remove the drive on pos %d", remove_pos);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(config_p, remove_pos, number_physical_objects, &drive_info_1);        

    // wait until the rebuild logging bit has been set on the remove_pos
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: wait for rebuild logging started on pos %d", remove_pos);
    tico_wait_for_rebuild_logging_set_for_position(raid_group_id, remove_pos);

    fbe_api_sleep(15000);

    // add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: add hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED");
    tico_add_debug_hook(raid_group_id, 
                        SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                        FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                        NULL,
                        NULL,
                        SCHEDULER_CHECK_STATES,
                        SCHEDULER_DEBUG_ACTION_LOG);

    // reinsert the removed drive 
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: reinsert the drive %d", remove_pos);
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(config_p, remove_pos, number_physical_objects, &drive_info_1);

    // verify that the hook not hit
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: verify that the hook not hit %d", remove_pos);
    hook_hit_count = tico_get_hook_hit_count(raid_group_id, 
                                             SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                                             FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                                             NULL,
                                             NULL,
                                             SCHEDULER_CHECK_STATES,
                                             SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_UINT64_EQUAL(hook_hit_count, 0);

    // remove the hook
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: del hook FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED");
    tico_delete_debug_hook(raid_group_id, 
                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL, 
                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED,
                           NULL,
                           NULL,
                           SCHEDULER_CHECK_STATES,
                           SCHEDULER_DEBUG_ACTION_LOG);
    

}


/*!****************************************************************************
 *  tico_send_io
 ******************************************************************************
 *
 * @brief
 *   send one IO to the first LUN of the rg
 *
 * @param   
 *      rg_config_p -- rg configuration
 *      context_p -- rdgen context used to run this IO
 *      rdgen_operation -- rdgen io operation code
 *      pattern -- rdgen IO pattern
 *      sp_id -- SP ID to which this IO is going to 
 *      lba -- IO starting LBA
 *      blks -- IO block counts
 *      pass_count -- used to generate the proper blk content with FBE_RDGEN_PATTERN_LBA_PASS
 *
 * @return  None 
 *
 *****************************************************************************/
static void tico_send_io(fbe_test_rg_configuration_t *rg_config_p,
                         fbe_api_rdgen_context_t *context_p,
                         fbe_rdgen_operation_t rdgen_operation,
                         fbe_rdgen_pattern_t pattern,
                         fbe_data_pattern_sp_id_t sp_id,
                         fbe_lba_t lba,
                         fbe_block_count_t blks,
                         fbe_u32_t pass_count)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_block_count_t elsz;
    fbe_u32_t idx; 

    /*  */
    elsz = fbe_test_sep_util_get_element_size(rg_config_p);

    /* Get Lun object IDs */
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Initialize rdgen context */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  rdgen_operation,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  elsz /* max blocks.  We override this later with regions.  */);

    /* status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification, peer_options); */

    /* Create one region in rdgen context for this IO.
     */
    idx = context_p->start_io.specification.region_list_length; 
    context_p->start_io.specification.region_list[idx].lba = lba;
    context_p->start_io.specification.region_list[idx].blocks = blks;
    context_p->start_io.specification.region_list[idx].pattern = pattern;
    context_p->start_io.specification.region_list[idx].pass_count = pass_count;
    context_p->start_io.specification.region_list[0].sp_id = sp_id; 
    context_p->start_io.specification.region_list_length++;

    /* Set flag to use regions */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Send IO */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

// for verify the fix of AR752962
/* AR752962:
 * clear the FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD flag 
 * After adding the 4k feature, in the condition fbe_raid_group_clear_rl_cond_function,
 * we will attempt to drain all the IO instead of doing the normal quiesce.
 * But some IO might be waiting for peer to grant SL while peer panic.
 * These IO could not be drained until FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL runs.
 * So draing IO procedure will be stucked.
 * By clearing the HOLD flag, we will transfer to do the normal quiesce.
 * */
static void tico_run_clear_quiesce_hold_flag_test(fbe_test_rg_configuration_t* config_p, void* context_p)
{
    fbe_status_t                            status;
    fbe_u32_t number_physical_objects;
    fbe_u32_t remove_pos = 0;
    fbe_api_terminator_device_handle_t      drive_info_1;
    fbe_object_id_t raid_group_id;
    fbe_object_id_t                         lun_object_id;                  //  object ID of a LUN in the RG
    fbe_u32_t                               first_rdgen_context_index;      //  rdgen context for first injection
    fbe_u32_t                               second_rdgen_context_index;     //  rdgen context for second injection
    fbe_api_raid_group_get_info_t   raid_group_info;
    
    
    
    // get the RG number
    status = fbe_api_database_lookup_raid_group_by_number(config_p->raid_group_id, &raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(raid_group_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get Lun object IDs */
    status = fbe_api_database_lookup_lun_by_number(config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:  wait unit all the background verify has been finished\n");
    status = fbe_test_sep_util_wait_for_raid_group_verify(raid_group_id, TICO_WAIT_FOR_VERIFY_SEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // remove the drive on pos 0
    mut_printf(MUT_LOG_TEST_STATUS, "tico test: Remove the drive on pos %d", remove_pos);
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    number_physical_objects -= 1;
    sep_rebuild_utils_remove_drive_and_verify(config_p, remove_pos, number_physical_objects, &drive_info_1);        
    

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    // inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN error for write operation on pos1
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   inject FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR error for write operation on pos1\n");
    tico_test_inject_error_record(config_p, 
                                  1,  /* position */
                                  raid_group_info.paged_metadata_start_lba / raid_group_info.num_data_disk,  /* lba */
                                  raid_group_info.paged_metadata_capacity / raid_group_info.num_data_disk,  /* blocks */
                                  1,  /* error_limit */
                                  6000,  /* delay */
                                  FBE_API_LOGICAL_ERROR_INJECTION_TYPE_DELAY_IO_DOWN,
                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                  SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH);

    // start one full stripe sized IO
    first_rdgen_context_index = 0;
    second_rdgen_context_index = 1;
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   start one full stripe sized IO on SPA side\n");
    tico_test_start_one_io(config_p, first_rdgen_context_index, 0, 128*4, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    // on SPB side
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   start one full stripe sized IO on SPB side\n");
    tico_test_start_one_io(config_p, second_rdgen_context_index, 0, 128*4, FBE_RDGEN_OPERATION_WRITE_ONLY, lun_object_id);

    // reinsert the drive on POS0
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   reinsert the drive on pos 0\n");
    number_physical_objects += 1;
    sep_rebuild_utils_reinsert_drive_and_verify(config_p, remove_pos, number_physical_objects, &drive_info_1);

    
    // panic SPA
    // while an error inject is injected, we could not automatically panic SP by unloading neit_sep
    // we could simulate the SP panic by closing the process window of SPA
    //mut_printf(MUT_LOG_TEST_STATUS, "tico test:   panic SPA\n");
    //fbe_test_sep_util_destroy_neit_sep();
    //fbe_test_sep_util_destroy_sep();

    mut_printf(MUT_LOG_TEST_STATUS, "tico test:   going to sleep\n");
    fbe_api_sleep(300000);
}


