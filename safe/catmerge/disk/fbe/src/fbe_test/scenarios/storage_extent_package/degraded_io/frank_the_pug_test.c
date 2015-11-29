/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    frank_the_pug_test.c
 *****************************************************************************
 *
 * @brief   This file contains tests for raid group deferred memory request 
 *          handling.  The following cases are tested:
 *              o Degraded raid groups
 *              o Raid groups that go broken (shutdown)
 *
 * @version
 *  02/07/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
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
#include "sep_hook.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_dps_memory_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * frank_the_pug_short_desc = "This file contains tests for raid group deferred memory request handling.";
char * frank_the_pug_long_desc = "\
This scenario tests that the RAID Group can handle the following scenarios:\n"\
"\n\
  - Deferred memory requests with degraded I/O.\n\
  - Deferred memory requests during shutdown. \n\
\n\
Description last updated: 2/07/2014.\n";

/*!*******************************************************************
 * @def FRANK_THE_PUG_NUM_OF_RAID_GROUPS
 *********************************************************************
 * @brief Number of raid groups to tests
 *
 * @note    Due to the fact that we are running with reduced memory
 *          background operations can take a VERY long time.  There
 *          should be EXACTLY (1) raid group config.
 *********************************************************************/
#define FRANK_THE_PUG_NUM_OF_RAID_GROUPS    1

/*!*******************************************************************
 * @def FRANK_THE_PUG_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define FRANK_THE_PUG_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def FRANK_THE_PUG_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FRANK_THE_PUG_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def     FRANK_THE_PUG_HEAVY_IO_THREADS
 *********************************************************************
 * @brief   Thread count for a heavy load test.
 *
 *********************************************************************/
#define FRANK_THE_PUG_HEAVY_IO_THREADS          (32)

/*!*******************************************************************
 * @def     FRANK_THE_PUG_LIGHT_IO_THREADS
 *********************************************************************
 * @brief   Thread count for a light load test.
 *
 *********************************************************************/
#define FRANK_THE_PUG_LIGHT_IO_THREADS          (16)

/*!*******************************************************************
 * @def     FRANK_THE_PUG_HEAVY_IO_MAX_REQUEST_SIZE
 *********************************************************************
 * @brief   Maximum request size for a heavy load test.
 *
 *********************************************************************/
#define FRANK_THE_PUG_HEAVY_IO_MAX_REQUEST_SIZE (0x400)

/*!*******************************************************************
 * @def     FRANK_THE_PUG_NUM_OF_DEFERRED_MEMORY_REQUEST_TO_WAIT_FOR
 *********************************************************************
 * @brief   Number of deferred memory request to wait for (for all
 *          raid groups) before we declare success.
 *
 *********************************************************************/
#define FRANK_THE_PUG_NUM_OF_DEFERRED_MEMORY_REQUEST_TO_WAIT_FOR    (50)

/*!*******************************************************************
 * @def     FRANK_THE_PUG_NUM_OF_ABORTED_MEMORY_REQUEST_TO_WAIT_FOR
 *********************************************************************
 * @brief   Number of aborted memory request to wait for (for all
 *          raid groups) before we declare success.
 *
 *********************************************************************/
#define FRANK_THE_PUG_NUM_OF_ABORTED_MEMORY_REQUEST_TO_WAIT_FOR     (5)

/*!*******************************************************************
 * @def FRANK_THE_PUG_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define FRANK_THE_PUG_WAIT_SEC 120

/*!*******************************************************************
 * @def FRANK_THE_PUG_WAIT_MSEC
 *********************************************************************
 * @brief Number of milli-seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define FRANK_THE_PUG_WAIT_MSEC     (FRANK_THE_PUG_WAIT_SEC * 1000)

/*!*******************************************************************
 * @def     FRANK_THE_PUG_NUM_OF_IO_PATTERNS
 *********************************************************************
 * @brief   The number of active I/O patterns to run against each lun.
 *
 *********************************************************************/
#define FRANK_THE_PUG_NUM_OF_IO_PATTERNS    1

/*!*******************************************************************
 * @var frank_the_pug_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t frank_the_pug_io_contexts[FRANK_THE_PUG_LUNS_PER_RAID_GROUP * FRANK_THE_PUG_NUM_OF_IO_PATTERNS];

/*!*******************************************************************
 * @var frank_the_pug_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t frank_the_pug_private_io_contexts[FRANK_THE_PUG_LUNS_PER_RAID_GROUP * FRANK_THE_PUG_NUM_OF_IO_PATTERNS];

/*!*******************************************************************
 * @var frank_the_pug_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 * @note    Due to the fact that we are running with reduced memory
 *          background operations can take a VERY long time.  There
 *          should be EXACTLY (1) raid group config.
 *********************************************************************/
fbe_test_rg_configuration_t frank_the_pug_raid_group_config_extended[FRANK_THE_PUG_NUM_OF_RAID_GROUPS + 1] = 
{
    /* width,   capacity    raid type,                  class,                  block size     RAID-id.    bandwidth.*/
    {16,        0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var frank_the_pug_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 * @note    Due to the fact that we are running with reduced memory
 *          background operations can take a VERY long time.  There
 *          should be EXACTLY (1) raid group config.
 *
 *********************************************************************/
fbe_test_rg_configuration_t frank_the_pug_raid_group_config_qual[FRANK_THE_PUG_NUM_OF_RAID_GROUPS + 1] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {16,        0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,     520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var     frank_the_pug_private_raid_group_config
 *********************************************************************
 * @brief   This is the array of private raid groups we will loop over.
 *
 * @note    Currently only the vault lun.
 *
 * @note    The capacity of the LUN is huge.  We use the vault so that
 *          writes consume a lot of memory.
 *
 *********************************************************************/
fbe_test_rg_configuration_t frank_the_pug_private_raid_group_config[] = 
{
    /*! @note Although all fields must be valid only the system id is used to populate the test config+
     *                                                                                                |
     *                                                                                                v    */
    /*width capacity    raid type                   class               block size  system id                               bandwidth.*/
    {5,     0x0F003000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,      FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,   0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,   520,  2,  FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*************************
 *   EXTERNAL FUNCTIONS
 *************************/
extern void big_bird_wait_for_rgs_rb_logging(fbe_test_rg_configuration_t * const rg_config_p,
                                             fbe_u32_t raid_group_count,
                                             fbe_u32_t num_rebuilds);

/*!**************************************************************
 *          frank_the_pug_set_reduced_dps_data_values()
 ****************************************************************
 * @brief
 *  Set the reduced data values.
 *
 * @param dps_data_params_p - Pointer to dps data params
 * 
 * @return none
 * 
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static void frank_the_pug_set_reduced_dps_data_values(fbe_memory_dps_init_parameters_t *dps_data_params_p)
{
    dps_data_params_p->packet_number_of_chunks            =  4; //  4
    dps_data_params_p->block_64_number_of_chunks          =  2; // 56 TODO: Change back to 1
    dps_data_params_p->block_1_number_of_chunks           =  0; //  0

    /*! @note Currently we don't modify the number of reserved chunks.*/
    dps_data_params_p->reserved_packet_number_of_chunks   =  4; //  4
    dps_data_params_p->reserved_block_64_number_of_chunks = 70; // 70
    dps_data_params_p->reserved_block_1_number_of_chunks  =  0; //  0

    dps_data_params_p->fast_packet_number_of_chunks       =  0; //  1
    dps_data_params_p->fast_block_64_number_of_chunks     =  0; // 15
//                                                          83<==150
    dps_data_params_p->number_of_main_chunks = dps_data_params_p->packet_number_of_chunks +
                                               dps_data_params_p->block_64_number_of_chunks +
                                               dps_data_params_p->block_1_number_of_chunks +
                                               dps_data_params_p->reserved_packet_number_of_chunks +
                                               dps_data_params_p->reserved_block_64_number_of_chunks +
                                               dps_data_params_p->fast_packet_number_of_chunks +
                                               dps_data_params_p->fast_block_64_number_of_chunks; 
    
    return;   
}
/************************************************* 
 * end frank_the_pug_set_reduced_dps_data_values()
 *************************************************/

/*!**************************************************************
 *          frank_the_pug_restore_dps_data_values()
 ****************************************************************
 * @brief
 *  Restore the dps data values.
 *
 * @param dps_data_params_p - Pointer to dps data params
 * 
 * @return none
 * 
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static void frank_the_pug_restore_dps_data_values(fbe_memory_dps_init_parameters_t *dps_data_params_p)
{
    /*! @note A value of 0 for dps_data_params_p->number_of_main_chunks
     *        indicates that sep init should use the default values.
     */
    fbe_zero_memory(dps_data_params_p, sizeof(*dps_data_params_p));
    
    return;   
}
/************************************************* 
 * end frank_the_pug_restore_dps_data_values()
 *************************************************/
 
/*!***************************************************************************
 *          frank_the_pug_start_io()
 *****************************************************************************
 *
 * @brief   Start rdgen I/O as requested:
 *              o The type of I/O (w/r/c, zero, read only etc)
 *              o How many I/O threads
 *              o The maximum block size to use
 *              o The abort mode (or none)
 *              o If applicable the dualsp I/O mode (load balanced etc)
 *
 * @param       rdgen_operation - The type of I/O to have rdgen issue  
 * @param       threads - How many I/O threads to run
 * @param       io_size_blocks - The maximum I/O size to use  
 * @param       ran_abort_msecs - If enabled the abort mode     
 * @param       requested_dualsp_mode - If dualsp mode peer options
 *
 * @return      status
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_start_io
 *
 *****************************************************************************/
static fbe_status_t frank_the_pug_start_io(fbe_rdgen_operation_t rdgen_operation,
                                       fbe_u32_t threads,
                                       fbe_u32_t io_size_blocks,
                                       fbe_test_random_abort_msec_t ran_abort_msecs,
                                       fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &frank_the_pug_io_contexts[0];
    fbe_bool_t                  b_ran_aborts_enabled;
    fbe_bool_t                  b_dualsp_mode;

    mut_printf(MUT_LOG_TEST_STATUS, 
           "== %s Start I/O ==", 
           __FUNCTION__);

	/* This will reduce DPS memory to reserved pool ONLY */
	fbe_api_dps_memory_reduce_size(NULL, NULL);

    /* Setup the abort mode
     */
    b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    status = fbe_test_sep_io_configure_abort_mode(b_ran_aborts_enabled, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

    /* Setup the dualsp mode
     */
    b_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_test_sep_io_configure_dualsp_io_mode(b_dualsp_mode, requested_dualsp_mode);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Configure the rdgen test I/O context
     */
    fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           rdgen_operation,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           fbe_test_sep_io_get_threads(threads), /* threads */
                                           io_size_blocks,
                                           b_ran_aborts_enabled, /* Inject aborts if requested*/
                                           requested_dualsp_mode  /* If dualsp mode  */);

    /* inject random aborts if we are asked to do so
     */
    if (ran_abort_msecs!= FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s random aborts inserted %d msecs", __FUNCTION__, ran_abort_msecs);
        status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification, ran_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Only abort after this period
     */
    fbe_api_rdgen_io_specification_set_msecs_to_abort(&context_p->start_io.specification,
                                                      FRANK_THE_PUG_WAIT_MSEC);

    /* Run our I/O test.  Since we are running against all LUs,
     * there is only (1) rdgen context.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, FRANK_THE_PUG_NUM_OF_IO_PATTERNS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/******************************************
 * end frank_the_pug_start_io()
 ******************************************/

/*!***************************************************************************
 *          frank_the_pug_start_private_rg_io()
 *****************************************************************************
 *
 * @brief   Start rdgen I/O as requested:
 *              o The type of I/O (w/r/c, zero, read only etc)
 *              o How many I/O threads
 *              o The maximum block size to use
 *              o The abort mode (or none)
 *              o If applicable the dualsp I/O mode (load balanced etc)
 *
 * @param       rdgen_operation - The type of I/O to have rdgen issue  
 * @param       threads - How many I/O threads to run
 * @param       io_size_blocks - The maximum I/O size to use  
 * @param       ran_abort_msecs - If enabled the abort mode     
 * @param       peer_options - rdgne peer i/o options
 *
 * @return      status
 *
 * @author
 *  02/20/2014  Ron Proulx  - Created from scooby_doo_start_io
 *
 *****************************************************************************/
static fbe_status_t frank_the_pug_start_private_rg_io(fbe_test_rg_configuration_t *private_rg_config_p,
                                                      fbe_rdgen_operation_t rdgen_operation,
                                                      fbe_u32_t threads,
                                                      fbe_u32_t io_size_blocks,
                                                      fbe_test_random_abort_msec_t ran_abort_msecs,
                                                      fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &frank_the_pug_private_io_contexts[0];
    fbe_u32_t                   num_luns;

    status = fbe_test_sep_io_get_num_luns(private_rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE_MSG(num_luns <= FRANK_THE_PUG_LUNS_PER_RAID_GROUP, "too many luns to test");

    mut_printf(MUT_LOG_TEST_STATUS, 
           "== %s Start I/O ==", 
           __FUNCTION__);

    /* Setup the test contexts
     */
    status = fbe_test_rg_setup_rdgen_test_context(private_rg_config_p,
                                                  context_p,
                                                  FBE_FALSE,        /* This is random */
                                                  rdgen_operation,
                                                  FBE_RDGEN_PATTERN_LBA_PASS,
                                                  fbe_test_sep_io_get_threads(threads), /* Threads */
                                                  io_size_blocks,   /* max I/O size */
                                                  FBE_FALSE,        /* Don't inject aborts */
                                                  FBE_FALSE,        /* Don't run peer i/o */
                                                  peer_options, 
                                                  0,                /* Starting lba */
                                                  FBE_LBA_INVALID   /* max lba is capacity */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Only abort after this period
     */
    status = fbe_api_rdgen_io_specification_set_msecs_to_abort(&context_p->start_io.specification,
                                                      FRANK_THE_PUG_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Use non-cached writes.
     */
    //status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
    //                                                    FBE_RDGEN_OPTIONS_NON_CACHED);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Start the I/O
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************
 * end frank_the_pug_start_private_rg_io()
 ******************************************/

/*!***************************************************************************
 *          frank_the_pug_wait_for_deferred_memory_request_count()
 *****************************************************************************
 *
 * @brief   Wait for the requested number of deferred memory allocations.  This
 *          value is for all raid groups under test.
 *
 * @param   deferred_memory_allocation_count - Number of defferred memory
 *              allocations (for all raid groups under test) to wait for.
 * @param   timeout_secs - How long to wait
 * 
 * @return  none
 *
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void frank_the_pug_wait_for_deferred_memory_request_count(fbe_u32_t deferred_memory_allocation_count,
                                                          fbe_u32_t timeout_secs)
{
    fbe_status_t                status;
    fbe_api_raid_memory_stats_t raid_memory_stats;
    fbe_u32_t                   time_waited_secs = 0;

    /* While we are below the requested count and timeout
     */
    do {
        status = fbe_api_raid_group_get_raid_memory_stats(&raid_memory_stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS,
                   "waited %d seconds current deferred: %llx expected: %d\n",
                   time_waited_secs, 
                   raid_memory_stats.deferred_allocations,
                   deferred_memory_allocation_count);

       /* Check if done.
        */
       if (raid_memory_stats.deferred_allocations >= deferred_memory_allocation_count) {
           break;
       }

       /* Wait and retry. */
       fbe_api_sleep(1000);
       time_waited_secs++;

    } while ((raid_memory_stats.deferred_allocations < deferred_memory_allocation_count) &&
             (time_waited_secs < timeout_secs)                                              );

    /* Failed */
    if (raid_memory_stats.deferred_allocations < deferred_memory_allocation_count) {
		fbe_api_panic_sp();
        MUT_FAIL_MSG("exceeded wait time");
    }

    return;
}
/************************************************************
 * end frank_the_pug_wait_for_deferred_memory_request_count()
 ************************************************************/

/*!***************************************************************************
 *          frank_the_pug_wait_for_aborted_memory_request_count()
 *****************************************************************************
 *
 * @brief   Wait for the requested number of aborted memory allocations.  This
 *          value is for all raid groups under test.
 *
 * @param   aborted_memory_allocation_count - Number of aborted memory
 *              allocations (for all raid groups under test) to wait for.
 * @param   timeout_secs - How long to wait
 * 
 * @return  none
 *
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void frank_the_pug_wait_for_aborted_memory_request_count(fbe_u32_t aborted_memory_allocation_count,
                                                          fbe_u32_t timeout_secs)
{
    fbe_status_t                status;
    fbe_api_raid_memory_stats_t raid_memory_stats;
    fbe_u32_t                   time_waited_secs = 0;

    /* While we are below the requested count and timeout
     */
    do {
        status = fbe_api_raid_group_get_raid_memory_stats(&raid_memory_stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS,
                   "waited %d seconds current aborted: %llx expected: %d\n",
                   time_waited_secs, 
                   raid_memory_stats.aborted_allocations,
                   aborted_memory_allocation_count);

       /* Check if done.
        */
       if (raid_memory_stats.aborted_allocations >= aborted_memory_allocation_count) {
           break;
       }

       /* Wait and retry. */
       fbe_api_sleep(1000);
       time_waited_secs++;

    } while ((raid_memory_stats.aborted_allocations < aborted_memory_allocation_count) &&
             (time_waited_secs < timeout_secs)                                            );

    /* Failed
     */
    if (raid_memory_stats.aborted_allocations < aborted_memory_allocation_count) 
    {
        /* after talking to Ron and Rob, we decide not fail the test anymore if not enough aborted memory request waited.
           Since the when you remove a drive, at the edge path state change event context, it will abort stripe lock and 
           stop IO coming to the RAID library. So the aborted memory request count depends only on how many IO are waiting for
           the memory at the point of removing the drive.
           the mumber is not a deterministic one
         */ 
#if 0
        MUT_FAIL_MSG("exceeded wait time");
#endif
        mut_printf(MUT_LOG_TEST_STATUS, "there are %lld memory request was aborted\n", raid_memory_stats.aborted_allocations);
    }

    return;
}
/************************************************************
 * end frank_the_pug_wait_for_aborted_memory_request_count()
 ************************************************************/

/*!***************************************************************************
 *          frank_the_pug_test_shutdown_io_with_reduced_memory()
 *****************************************************************************
 *
 * @brief   Start heavy I/O and wait for a certian number of deferred memory
 *          allocations.  Then shutdown the raid groups by removing sufficient
 *          drives.  Wait for all I/O to stop.  Then validate suffecient 
 *          number of aborted, defferred memory requests.
 *
 * @param   rg_config_p - Array of raid groups under test
 * @param   requested_dualsp_mode - The request peer option (for dualsp test mode)
 * 
 * @return  None
 * 
 * @author
 *  02/12/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void frank_the_pug_wait_for_rg_shutdown(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       rg_index;
    fbe_object_id_t rg_object_id;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    /* Wait for all the raid groups to goto Failed (shutdown).
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* Wait for the raid group to shutdown
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              rg_object_id, 
                                                                              FBE_LIFECYCLE_STATE_FAIL,
                                                                              FRANK_THE_PUG_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end frank_the_pug_wait_for_rg_shutdown()
 ******************************************/

/*!***************************************************************************
 *          frank_the_pug_test_degraded_io_with_reduced_memory()
 *****************************************************************************
 *
 * @brief   Start I/O degraded raid groups, wait for the number of pending 
 *          allocations to reach expected value.  Re-insert drives.
 *
 * @param   rg_config_p - Array of raid groups under test.
 * @param   requested_dualsp_mode - The request peer option (for dualsp test mode)
 * 
 * @return fbe_status_t
 * 
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  02/07/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void frank_the_pug_test_degraded_io_with_reduced_memory(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = &frank_the_pug_io_contexts[0];
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       num_drives_to_remove = 1;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start ==", __FUNCTION__);

    /* First reset the stats
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reset raid memory stats==", __FUNCTION__);
    status = fbe_test_sep_reset_raid_memory_statistics(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: Remove one drive from each raid group under test.
     */
    big_bird_remove_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* This code tests that the rg get info code does the right things to return 
     * the rebuild logging status for these drive(s). 
     */
    big_bird_wait_for_rgs_rb_logging(rg_config_p, raid_group_count, num_drives_to_remove);

    /* Step 2: Start heavy write I/O
     */
    status = frank_the_pug_start_io(FBE_RDGEN_OPERATION_WRITE_ONLY,
                                    FRANK_THE_PUG_HEAVY_IO_THREADS,
                                    FRANK_THE_PUG_HEAVY_IO_MAX_REQUEST_SIZE,
                                    FBE_TEST_RANDOM_ABORT_TIME_INVALID /* Don't inject aborts */,
                                    requested_dualsp_mode);

    /* Wait for the expected number of deferred memory allocations.
     */
    frank_the_pug_wait_for_deferred_memory_request_count(FRANK_THE_PUG_NUM_OF_DEFERRED_MEMORY_REQUEST_TO_WAIT_FOR,
                                                         FRANK_THE_PUG_WAIT_SEC);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, FRANK_THE_PUG_NUM_OF_IO_PATTERNS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* We don't expect any errors
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, FRANK_THE_PUG_NUM_OF_IO_PATTERNS, FBE_STATUS_OK, 
                                                   0,    /* err count */
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

    /* Wait for the rebuilds to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, num_drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    return;
}
/**********************************************************
 * end frank_the_pug_test_degraded_io_with_reduced_memory()
 **********************************************************/

/*!***************************************************************************
 *          frank_the_pug_test_shutdown_io_with_reduced_memory()
 *****************************************************************************
 *
 * @brief   Start heavy I/O and wait for a certian number of deferred memory
 *          allocations.  Then shutdown the raid groups by removing sufficient
 *          drives.  Wait for all I/O to stop.  Then validate suffecient 
 *          number of aborted, defferred memory requests.
 *
 * @param   rg_config_p - Array of raid groups under test
 * @param   requested_dualsp_mode - The request peer option (for dualsp test mode)
 * 
 * @return  None
 * 
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void frank_the_pug_test_shutdown_io_with_reduced_memory(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = &frank_the_pug_io_contexts[0];
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       num_drives_to_remove_to_degraded_rg = 1;
    fbe_u32_t       num_drives_to_remove_to_shutdown_rg = 3;
    fbe_u32_t       num_drives_to_remove;
    fbe_u32_t       expected_error_count;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start ==", __FUNCTION__);

    /* First reset the stats
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reset raid memory stats==", __FUNCTION__);
    status = fbe_test_sep_reset_raid_memory_statistics(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First degrade the raid groups
     */
    num_drives_to_remove = num_drives_to_remove_to_degraded_rg;
    big_bird_remove_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Step 1: Start heavy write I/O
     */
    status = frank_the_pug_start_io(FBE_RDGEN_OPERATION_WRITE_ONLY,
                                    FRANK_THE_PUG_HEAVY_IO_THREADS,
                                    FRANK_THE_PUG_HEAVY_IO_MAX_REQUEST_SIZE,
                                    FBE_TEST_RANDOM_ABORT_TIME_INVALID /* Don't inject aborts */,
                                    requested_dualsp_mode);

    /* Wait for the expected number of deferred memory allocations.
     */
    frank_the_pug_wait_for_deferred_memory_request_count(FRANK_THE_PUG_NUM_OF_DEFERRED_MEMORY_REQUEST_TO_WAIT_FOR,
                                                         FRANK_THE_PUG_WAIT_SEC);

    /* Remove drives to force raid group shutdown
     */
    num_drives_to_remove = num_drives_to_remove_to_shutdown_rg - num_drives_to_remove_to_degraded_rg;
    big_bird_remove_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Wait for expected number of aborted memory requests
     */
    frank_the_pug_wait_for_aborted_memory_request_count(FRANK_THE_PUG_NUM_OF_ABORTED_MEMORY_REQUEST_TO_WAIT_FOR,
                                                        FRANK_THE_PUG_WAIT_SEC);

    /* Wait for the all the raid groups to be shutdown
     */
    frank_the_pug_wait_for_rg_shutdown(rg_config_p);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, FRANK_THE_PUG_NUM_OF_IO_PATTERNS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* We expect failures.  Set the expected error count to the actual value.
     */
    expected_error_count = context_p[0].start_io.statistics.error_count;
    status = fbe_api_rdgen_context_check_io_status(context_p, FRANK_THE_PUG_NUM_OF_IO_PATTERNS, FBE_STATUS_OK, 
                                                   expected_error_count,    /* err count */
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_IO_ERROR,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, num_drives_to_remove_to_shutdown_rg,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

    /* Wait for the rebuilds to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, num_drives_to_remove_to_shutdown_rg);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    return;
}
/**********************************************************
 * end frank_the_pug_test_shutdown_io_with_reduced_memory()
 **********************************************************/

/*!***************************************************************************
 *          frank_the_pug_run_user_rg_tests()
 *****************************************************************************
 *
 * @brief   Reduce the amount of memory available to sep and run heavy I/O.
 *          Then confirm a certain number of deferred and/or aborted memory
 *          requests occur.
 *
 * @param   rg_config_p - Config to create.
 * @param   context_p - Not used
 *
 * @return  none
 *
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void frank_the_pug_run_user_rg_tests(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    FBE_UNREFERENCED_PARAMETER(context_p);

    /* Disable sparing of failed drives.
     */
    status = fbe_api_provision_drive_disable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for zeroing to complete...", __FUNCTION__);
    fbe_test_rg_wait_for_zeroing(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Zeroing complete...", __FUNCTION__);

    /* We need to wait for initial verify complete so that raid group will not go into
     * quiesced mode to update verify checkpoint.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for initial verify(s) to complete ==", __FUNCTION__);
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initial verify(s) - complete ==", __FUNCTION__);

    /* Write the background pattern
     */
    big_bird_write_background_pattern();

    /* Test 1:  Run heavy I/O and degraded the raid groups.  Wait for a certain
     *          number of deferred allocations.  Verify and that there are no
     *          aborted allocations.
     */
    frank_the_pug_test_degraded_io_with_reduced_memory(rg_config_p,
                                                       FBE_TEST_SEP_IO_DUAL_SP_MODE_LOCAL_ONLY /* Run all I/O on one SP */);

    /* Test 2:  Run heavy I/O and shutdown the raid groups. We expected a
     *          certain number of deferred and aborted memory allocations.
     */
    frank_the_pug_test_shutdown_io_with_reduced_memory(rg_config_p,
                                                       FBE_TEST_SEP_IO_DUAL_SP_MODE_LOCAL_ONLY /* Run all I/O on one SP */);

    /* Enable automatic sparing.
     */
    status = fbe_api_provision_drive_enable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}   
/******************************************
 * end frank_the_pug_run_tests()
 ******************************************/
/*!***************************************************************************
 *          frank_the_pug_test_nonpaged_lun_reduced_memory_test()
 *****************************************************************************
 *
 * @brief   Start heavy I/O to private raid group. Start a verify to user raid
 *          group.  Set a debug 
 *
 * @param   rg_config_p - Array of raid groups under test
 * @param   requested_dualsp_mode - The request peer option (for dualsp test mode)
 * 
 * @return  None
 * 
 * @author
 *  02/20/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void frank_the_pug_test_nonpaged_lun_reduced_memory_test(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_test_rg_configuration_t *private_rg_config_p,
                                                                fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t                *private_context_p = &frank_the_pug_private_io_contexts[0];
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         lun_object_id;              /* LUN object to test */
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;
    fbe_u32_t                               monitor_substate;
    fbe_api_lun_get_status_t                lun_verify_status;
    fbe_scheduler_debug_hook_t              hook;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start ==", __FUNCTION__);

    /* First reset the stats
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reset raid memory stats==", __FUNCTION__);
    status = fbe_test_sep_reset_raid_memory_statistics(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the raid group object id so that we can check for the verify completion.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run test against first LUN.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);

    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

    /* Step 1: Set a debug hook so that we don't miss the verify start.
     */
    fbe_test_verify_get_verify_start_substate(FBE_LUN_VERIFY_TYPE_USER_RW, &monitor_substate);
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 2: Run a R/W verify.  This will cause the errors to be detected.
     */
    fbe_test_sep_util_lun_initiate_verify(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW,
                                          FBE_TRUE, /* Verify the entire lun */   
                                          FBE_LUN_VERIFY_START_LBA_LUN_START,     
                                          FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Wait for the raid group to start the verify.
     */ 
    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                 monitor_substate,
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4: Wait for the lun verify to start.
     */
    status = fbe_test_sep_util_wait_for_verify_start(lun_object_id, 
                                                     FBE_LUN_VERIFY_TYPE_USER_RW,
                                                     &lun_verify_status);                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5:  Set a debug hook when a non-paged lazy clean occurs.
     */
    status = fbe_test_add_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA, 
                                            SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY,
                                            FBE_LUN_SUBSTATE_LAZY_CLEAN_START,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Start light write I/O on private raid groups
     */
    status = frank_the_pug_start_private_rg_io(private_rg_config_p,
                                               FBE_RDGEN_OPERATION_WRITE_ONLY,
                                               FRANK_THE_PUG_LIGHT_IO_THREADS,
                                               FRANK_THE_PUG_HEAVY_IO_MAX_REQUEST_SIZE,
                                               FBE_TEST_RANDOM_ABORT_TIME_INVALID /* Don't inject aborts */,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Add the verify complete hook and remove the start
     */
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Wait for the expected number of deferred memory allocations.
     */
    frank_the_pug_wait_for_deferred_memory_request_count(FRANK_THE_PUG_NUM_OF_DEFERRED_MEMORY_REQUEST_TO_WAIT_FOR,
                                                         FRANK_THE_PUG_WAIT_SEC);

    /* Step 10: Wait for the R/W Verify to complete.
     */
    status = fbe_test_verify_wait_for_verify_completion(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_get_bgverify_status(lun_object_id, &lun_verify_status, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY, 
                                                 FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_LOG,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_LOG);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Validate that the non-paged LUN hook was not hit.
     */
    status = fbe_test_get_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA, 
                                            SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY,
                                            FBE_LUN_SUBSTATE_LAZY_CLEAN_START,
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE,
                                            0,0, 
                                            &hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(hook.counter == 0)
    status = fbe_test_del_debug_hook_active(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAID_METADATA,
                                            SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY,
                                            FBE_LUN_SUBSTATE_LAZY_CLEAN_START,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(private_context_p, FRANK_THE_PUG_NUM_OF_IO_PATTERNS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* We don't expect any errors
     */
    status = fbe_api_rdgen_context_check_io_status(private_context_p, FRANK_THE_PUG_NUM_OF_IO_PATTERNS, FBE_STATUS_OK, 
                                                   0,    /* err count */
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/**********************************************************
 * end frank_the_pug_test_nonpaged_lun_reduced_memory_test()
 **********************************************************/

/*!***************************************************************************
 *          frank_the_pug_run_special_rg_tests()
 *****************************************************************************
 *
 * @brief   Reduce the amount of memory available to sep and run heavy I/O.
 *          Then confirm a certain number of deferred and/or aborted memory
 *          requests occur.
 *
 * @param   rg_config_p - User raid group array
 * @param   opaque_private_rg_config_p - Private raid group config
 *
 * @return  none
 *
 * @author
 *  02/20/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
void frank_the_pug_run_special_rg_tests(fbe_test_rg_configuration_t *rg_config_p, void *opaque_private_rg_config_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *private_rg_config_p = (fbe_test_rg_configuration_t *)opaque_private_rg_config_p;

    /* Disable sparing of failed drives.
     */
    status = fbe_api_provision_drive_disable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(private_rg_config_p,
                                                1   /* There is only (1) system LUN */,
                                                FRANK_THE_PUG_CHUNKS_PER_LUN);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(private_rg_config_p, 1);

    /* Test 1: Run heavy I/O to BOTH the user and vault luns.  Cause non-paged
     *         writes and validate the I/Os don't hang.
     */
    frank_the_pug_test_nonpaged_lun_reduced_memory_test(rg_config_p, private_rg_config_p,
                                                        FBE_TEST_SEP_IO_DUAL_SP_MODE_LOCAL_ONLY /* Run all I/O on one SP */);

    /* Enable automatic sparing.
     */
    status = fbe_api_provision_drive_enable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}   
/******************************************
 * end frank_the_pug_run_special_rg_tests()
 ******************************************/

/*!**************************************************************
 * frank_the_pug_test()
 ****************************************************************
 * @brief
 *  Run the tests that exercise the code that allows raid to
 *  reject I/Os if the client allows it.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/10/2014  Ron Proulx  - Created.
 *
 ****************************************************************/

void frank_the_pug_test(void)
{
    fbe_status_t    status;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Wait for the database to be Ready */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for database Ready ==", __FUNCTION__);
    status = fbe_test_sep_util_wait_for_database_service(30000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sleep(5000);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for database Ready - complete ==", __FUNCTION__);

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0){
        rg_config_p = &frank_the_pug_raid_group_config_extended[0];
    } else {
        rg_config_p = &frank_the_pug_raid_group_config_qual[0];
    }


    /* Phase 1:  Run user raid group tests.
     */
    fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL,
                                                  frank_the_pug_run_user_rg_tests,
                                                  FRANK_THE_PUG_LUNS_PER_RAID_GROUP,
                                                  FRANK_THE_PUG_CHUNKS_PER_LUN,
                                                  FBE_TRUE /* Don't destroy rg_config_p when done */);

    /* Phase 2:  Run combined user and private raid group tests.
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    fbe_test_create_raid_group_params_init_for_time_save(&params, (void *)&frank_the_pug_private_raid_group_config[0],
                                                         frank_the_pug_run_special_rg_tests,
                                                         FRANK_THE_PUG_LUNS_PER_RAID_GROUP,
                                                         FRANK_THE_PUG_CHUNKS_PER_LUN,
                                                         FBE_FALSE /* don't save config destroy config */);
    params.b_encrypted = FBE_FALSE;
    params.b_skip_create = FBE_TRUE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);

    return;
}
/******************************************
 * end frank_the_pug_test()
 ******************************************/
/*!**************************************************************
 * frank_the_pug_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  02/14/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
void frank_the_pug_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        if (test_level > 0)
        {
            /* Run test for normal element size.
            */
            rg_config_p = &frank_the_pug_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &frank_the_pug_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //frank_the_pug_set_reduced_dps_data_values(&frank_the_pug_boot_params.init_data_params);
        //fbe_test_common_reboot_save_sep_params(&frank_the_pug_boot_params);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();
    return;
}
/**************************************
 * end frank_the_pug_setup()
 **************************************/

/*!**************************************************************
 * frank_the_pug_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/14/2013  Ron Proulx  - Created.
 *
 ****************************************************************/

void frank_the_pug_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end frank_the_pug_cleanup()
 ******************************************/

void frank_the_pug_dualsp_test(void)
{
    frank_the_pug_test();
}
/*!**************************************************************
 * frank_the_pug_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 degraded I/O test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  02/14/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void frank_the_pug_dualsp_setup(void)
{
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    if (fbe_test_util_is_simulation())
    { 
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &frank_the_pug_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &frank_the_pug_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //frank_the_pug_set_reduced_dps_data_values(&frank_the_pug_boot_params.init_data_params);
        //fbe_test_common_reboot_save_sep_params(&frank_the_pug_boot_params);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        //fbe_test_common_reboot_save_sep_params(&frank_the_pug_boot_params);

        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    return;
}
/******************************************
 * end frank_the_pug_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * frank_the_pug_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/14/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
void frank_the_pug_dualsp_cleanup(void)
{
    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;
}
/******************************************
 * end frank_the_pug_dualsp_cleanup()
 ******************************************/
/*************************
 * end file frank_the_pug_test.c
 *************************/


