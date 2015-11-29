/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file big_bird_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a degraded I/O test for raid 5 objects.
 *
 * @version
 *   9/10/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_encryption_interface.h"
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
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "sep_hook.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * big_bird_short_desc = "raid 5 degraded read and write I/O test.";
char * big_bird_long_desc = "\
The Big Bird scenario tests degraded parity raid group I/O.  It also tests shutdowns with I/O for parity raid groups.\n"\
"\n\
  The Big Bird scenario is written so it can use any configuration, and thus it can be used by multiple scenarios.\n\
  The following tests are based upon Big Bird and thus have the exact same flow, but they configure\n\
  different raid types.\n\
  - Big Bird - Raid-5\n\
  - Bert - Raid-3\n\
  - Ernie - Raid-6\n\
  - Rosita - Raid-10\n\
\n\
-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: configure all raid groups and LUNs.\n\
        - We always test at least one rg with 512 block size drives.\n\
        - All raid groups have 3 LUNs each\n\
\n\
STEP 2: Quiesce/unquiesce test:\n\
        - run write/read/compare I/O and send a quiesce to the raid group.  \n\
        - Then send an unquiesce to the raid group. \n\
        - Stop I/O and make sure there were no errors.\n\
\n\
STEP 3: write a background pattern to all luns and then read it back and make sure it was written successfully\n\
\n\
Test descriptions:\n\
  Advanced rebuild test:\n\
        - Write a background pattern to all LUNs.\n\
        - Degrade all raid groups. R5/R3 pull one drive, R6 pull 2 drives.\n\
        - Write a secondary pattern to random areas with random block sizes throughout the LUN.\n\
        - Read and check the background and secondary pattern.\n\
        - Kick off a repeating constant read/check of background and secondary patterns\n\
        - Insert either the same drive or configure a spare for all removed drives.\n\
        - This causes rebuilds to occur.  Wait for rebuilds to finish.\n\
        - Stop read/check and make sure there were no errors.\n"\
"\n\
  Rebuild read only test:\n\
        - Write a background pattern to all LUNs.\n\
        - Write a secondary pattern to random areas with random block sizes throughout the LUN.\n\
        - Read and check the background and secondary pattern.\n\
        - Kick off a repeating constant read/check of background and secondary patterns\n\
        - Degrade all raid groups. R5/R3 pull one drive, R6 pull 2 drives.\n\
        - Insert either the same drive or configure a spare for all removed drives.\n\
        - This causes rebuilds to occur.  Wait for rebuilds to finish.\n\
        - Stop read/check and make sure there were no errors.\n\
\n\
STEP 3: Run advanced rebuild test \n\
        - background pattern is randomly chosen of either zeros or lba/pass with pass count of 0\n\
        - reinsert the same drive so that only a partial rebuild is performed.\n\
\n\
STEP 4: Run advanced rebuild test \n\
        - background pattern is randomly chosen of either zeros or lba/pass with pass count of 0\n\
        - configure spares so permanent spares will swap in and a full rebuild is performed.\n\
\n\
STEP 5: Run rebuild read only test \n\
        - background pattern is randomly chosen of either zeros or lba/pass with pass count of 0\n\
\n\
STEP 6: Run simple rebuild logging test \n\
        - Degrade the raid group.  R5/R3 pull one drive, R6 pull 2 drives. \n\
        - Do a single write/read/compare operation to one stripe of the LUN at the beginning and one\n\
        - stripe at the end.\n\
        - Insert the same drive for all removed drives.\n\
        - This causes rebuilds to occur.  Wait for rebuilds to finish.\n\
        - Do a single read/compare operation to one stripe of the LUN at the beginning and one\n\
        - stripe at the end to check the areas written.\n\
        \n"\
"\n\
STEP 7: Run rebuild logging test \n\
        - Start multiple threads of write/read/compare on all LUNs.\n\
        - Allow I/O to continue for a random time up to a second.\n\
        - Degrade all raid groups. R5/R3 pull one drive, R6 pull 2 drives.\n\
        - Allow I/O to continue for a random time up to 2 seconds.\n\
        - Stop I/O and make sure there were no errors.\n\
        - Start a single thread of write/read/compare on all LUNs.  This light I/O allows rebuilds to make progress. \n\
        - reinsert all removed drives.\n\
        - This causes rebuilds to occur.  Wait for rebuilds to finish.\n\
        - Stop I/O.  Make sure there were no errors.\n\
        \n"\
"\n\
STEP 8: Run degraded full rebuild test \n\
        - Start multiple threads of write/read/compare on all LUNs.\n\
        - Allow I/O to continue for a random time up to a second.\n\
        - Degrade all raid groups.  R5/R3 pull one drive, R6 pull 2 drives.\n\
        - Allow I/O to continue for a random time up to 2 seconds.\n\
        - Stop I/O and make sure there were no errors.\n\
        - Start a single thread of write/read/compare on all LUNs.  This light I/O allows rebuilds to make progress. \n\
        - Configure enough spares for all removed drives.  Eventually the system will swap in permanent spares.\n\
        - This causes rebuilds to occur.  Wait for rebuilds to finish.\n\
        - Stop I/O.  Make sure there were no errors.\n\
        \n"\
"\n\
STEP 9: Run degraded full rebuild test using zero/read/compare instead of write/read/compare.\n\
\n\
STEP 10: Run abort tests\n\
         - Run the rebuild logging test with aborting I/Os.\n\
         - Run the degraded full rebuild test with aborting write/read/check I/Os.\n\
         - Run the degraded full rebuild test with aborting zero/read/check I/Os.\n\
\n\
STEP 11: Degraded shutdown test\n\
        - Start multiple threads of write/read/compare on all LUNs.\n\
        - Allow I/O to continue for a random time up to a second.\n\
        - Remove enough drives (2 - R5/R3, 3 - R6) to shutdown all raid groups.\n\
        - Wait for all raid groups transition to the failed lifecycle state.\n\
        - Allow all I/Os to fail and make sure there were errors.\n\
        - Reinsert all removed drives.\n\
        - Wait for all raid groups transition to the ready lifecycle state.\n\
        - This causes rebuilds to occur since there were delays in between drive pulls.\n\
        - Wait for rebuilds to finish.\n\
        - Stop I/O.  Make sure there were no errors.\n\
\n\
STEP 11: Write log test\n\
        - Start multiple threads of write/read/compare on all LUNs.\n\
        - Allow I/O to continue for a random time up to a second.\n\
        - Remove enough drives (1 - R5/R3, 2 - R6) to degrade all raid groups.\n\
        - Allow I/O to continue for a random time up to 5 seconds.\n\
        - Remove enough drives (1) to shutdown all raid groups.\n\
        - Wait for all raid groups transition to the failed lifecycle state.\n\
        - Allow all I/Os to fail and make sure there were errors.\n\
        - Reinsert the last removed drive.\n\
        - Wait for all raid groups transition to the ready lifecycle state.\n\
        - Reinsert the other removed drive.\n\
        - This causes rebuilds to occur.\n\
        - Wait for rebuilds to finish.\n\
\n\
Test Specific Switches:\n\
          -abort_cases_only - Only run the abort tests.\n\
          -zero_only - Only run the zero tests.\n\
          -shutdown_only - Only run the shutdown tests.\n\
\n\
Description last updated: 9/26/2011.\n";

/*!*******************************************************************
 * @def BIG_BIRD_WAIT_IO_COUNT
 *********************************************************************
 * @brief Number of I/Os per thread we need to wait for after starting
 *        I/O.  Ensures that at least some I/O completes.
 *
 *********************************************************************/
#define BIG_BIRD_WAIT_IO_COUNT 1

/*!*******************************************************************
 * @def BIG_BIRD_TEST_MAX_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define BIG_BIRD_TEST_MAX_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def BIG_BIRD_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define BIG_BIRD_TEST_LUNS_PER_RAID_GROUP 3


/* Element size of our LUNs.
 */
#define BIG_BIRD_TEST_ELEMENT_SIZE 128 

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define BIG_BIRD_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def BIG_BIRD_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BIG_BIRD_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def BIG_BIRD_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BIG_BIRD_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE 

/*!*******************************************************************
 * @def BIG_BIRD_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BIG_BIRD_SMALL_IO_SIZE_BLOCKS 1024

/*!*******************************************************************
 * @def BIG_BIRD_LIGHT_THREAD_COUNT
 *********************************************************************
 * @brief Thread count for a light load test.
 *
 *********************************************************************/
#define BIG_BIRD_LIGHT_THREAD_COUNT 1

/*!*******************************************************************
 * @def BIG_BIRD_THIRD_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 3rd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define BIG_BIRD_THIRD_POSITION_TO_REMOVE 2

/*!*******************************************************************
 * @def BIG_BIRD_LONG_IO_SEC
 *********************************************************************
 * @brief Min time we will sleep for.
 *********************************************************************/
#define BIG_BIRD_LONG_IO_SEC 5

/*!*******************************************************************
 * @def BIG_BIRD_QUIESCE_TEST_ITERATIONS
 *********************************************************************
 * @brief The number of iterations we will run the quiesce/unquiesce test.
 *
 *********************************************************************/
#define BIG_BIRD_QUIESCE_TEST_ITERATIONS 1

/*!*******************************************************************
 * @def BIG_BIRD_QUIESCE_TEST_MAX_ITERATIONS
 *********************************************************************
 * @brief The number of iterations we will run the quiesce/unquiesce test.
 *
 *********************************************************************/
#define BIG_BIRD_QUIESCE_TEST_MAX_ITERATIONS 50

/*!*******************************************************************
 * @def BIG_BIRD_QUIESCE_TEST_MAX_SLEEP_TIME
 *********************************************************************
 * @brief Max time we will sleep for in the quiesce test.
 *        Each iteration of the queisce test does a sleep for a random
 *        amount of time from 0..N seconds.
 *********************************************************************/
#define BIG_BIRD_QUIESCE_TEST_MAX_SLEEP_TIME 1000

/*!*******************************************************************
 * @def BIG_BIRD_QUIESCE_TEST_THREADS_PER_LUN
 *********************************************************************
 * @brief The number of threads per lun for the quiesce/unquiesce test.
 *
 *********************************************************************/
#define BIG_BIRD_QUIESCE_TEST_THREADS_PER_LUN 2

/*!*******************************************************************
 * @def BIG_BIRD_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define BIG_BIRD_WAIT_SEC 60

/*!*******************************************************************
 * @def BIG_BIRD_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define BIG_BIRD_WAIT_MSEC 1000 * BIG_BIRD_WAIT_SEC

/*!*******************************************************************
 * @def BIG_BIRD_MAX_DRIVES_TO_REMOVE 
 *********************************************************************
 * @brief Max number of drives we will remove to make the raid group degraded.
 *
 *********************************************************************/
#define BIG_BIRD_MAX_DRIVES_TO_REMOVE 2

/*!*******************************************************************
 * @var big_bird_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t big_bird_test_contexts[BIG_BIRD_TEST_MAX_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var big_bird_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t big_bird_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,               block size     RAID-id.    bandwidth.*/
    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            3,          0},

    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,          1},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            5,          1},
    {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            6,          1},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var big_bird_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t big_bird_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var big_bird_io_msec_long
 *********************************************************************
 * @brief The default I/O time we will run for long durations.
 * 
 *********************************************************************/
static fbe_u32_t big_bird_io_msec_long = EMCPAL_MINIMUM_TIMEOUT_MSECS;

/*!*******************************************************************
 * @var big_bird_io_msec_short
 *********************************************************************
 * @brief The default I/O time we will run for short durations.
 * 
 *********************************************************************/
static fbe_u32_t big_bird_io_msec_short = EMCPAL_MINIMUM_TIMEOUT_MSECS;

/**************************************
 * Local Static Functions.
 **************************************/
void big_bird_validate_rebuilds_complete(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t raid_group_count);
void big_bird_check_background_operation_status(fbe_bool_t b_expected_value, 
                                                fbe_bool_t b_check_peer);

/*!**************************************************************
 * big_bird_check_background_operation_status()
 ****************************************************************
 * @brief
 *  Just make sure both SPs have the correct status for background ops.
 *
 * @param  b_expected_value - TRUE to check if enabled FALSE otherwise.               
 * @param  b_check_peer     - TRUE to check both SPs
 *
 * @return None.   
 *
 * @author
 *  8/14/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_check_background_operation_status(fbe_bool_t b_expected_value, 
                                                fbe_bool_t b_check_peer)
{
    fbe_bool_t b_enabled;

    b_enabled = fbe_api_all_bg_services_are_enabled();
    MUT_ASSERT_INT_EQUAL(b_enabled, b_expected_value);

    if ((b_check_peer == FBE_TRUE) && (fbe_test_sep_util_get_dualsp_test_mode()))
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        b_enabled = fbe_api_all_bg_services_are_enabled();
        MUT_ASSERT_INT_EQUAL(b_enabled, b_expected_value);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
}
/******************************************
 * end big_bird_check_background_operation_status()
 ******************************************/
/*!**************************************************************
 * big_bird_bg_operation_control_test()
 ****************************************************************
 * @brief
 *  Run a test where we enable and disable background ops.
 *  We check to make sure it is enable and then disabled for both SPs.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  8/14/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_bg_operation_control_test(void)
{
    fbe_status_t status;

    /* Initially we should have enabled background ops.
     */
    big_bird_check_background_operation_status(FBE_TRUE, FBE_TRUE);

    /* Now disable and make sure it is disabled on both SPs.
     */
    status = fbe_api_database_stop_all_background_service(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    big_bird_check_background_operation_status(FBE_FALSE, FBE_TRUE);

    /* Now re-enable and make sure it is enabled.
     */
    status = fbe_api_database_restart_all_background_service();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    big_bird_check_background_operation_status(FBE_TRUE, FBE_TRUE);

    /* Now disable and make sure it is disabled on Local SP only.
     */
    status = fbe_api_database_stop_all_background_service(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    big_bird_check_background_operation_status(FBE_FALSE, FBE_FALSE);

    /* Now re-enable and make sure it is enabled.
     */
    status = fbe_api_database_restart_all_background_service();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    big_bird_check_background_operation_status(FBE_TRUE, FBE_FALSE);

}
/******************************************
 * end big_bird_bg_operation_control_test()
 ******************************************/
/*!**************************************************************
 * big_bird_set_io_seconds()
 ****************************************************************
 * @brief
 *  Set the io seconds for this test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();
    if (long_io_time_seconds == 0)
    {
        long_io_time_seconds = BIG_BIRD_LONG_IO_SEC;
    }
    big_bird_io_msec_long = (fbe_random() % (long_io_time_seconds * 1000)) + EMCPAL_MINIMUM_TIMEOUT_MSECS;
    big_bird_io_msec_short = (fbe_random() % 1000) + EMCPAL_MINIMUM_TIMEOUT_MSECS;
    return;
}
/******************************************
 * end big_bird_set_io_seconds()
 ******************************************/

/*!**************************************************************
 * big_bird_set_group_debug_flags()
 ****************************************************************
 * @brief
 *  Set the raid group debug flags for all raid groups
 *  in our configuration.
 *
 * @param flags - raid group debug flags to set.               
 *
 * @return None.
 *
 * @author
 *  3/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_set_group_debug_flags(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_u32_t raid_group_count,
                                    fbe_raid_group_debug_flags_t flags)
{
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t index;

    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        fbe_api_raid_group_set_group_debug_flags(rg_object_id, flags);
        rg_config_p++;
    }
    return;
}
/******************************************
 * end big_bird_set_group_debug_flags()
 ******************************************/
/*!**************************************************************
 * big_bird_set_sp_for_single_io()
 ****************************************************************
 * @brief
 *  Determine which SP to use for individual I/Os.
 *
 * @param peer_options - rdgen peer options for sending I/O.
 *                       This tells rdgen which SP to use.             
 *
 * @return None.
 *
 * @author
 *  8/22/2011 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_set_sp_for_single_io(fbe_api_rdgen_peer_options_t peer_options)
{
    if ((peer_options == FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER) ||
        (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE))
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    }
    return;
}
/******************************************
 * end big_bird_set_sp_for_single_io()
 ******************************************/
/*!**************************************************************
 * big_bird_wait_for_raid_group_flags()
 ****************************************************************
 * @brief
 *  Wait for this raid group's flags to reach the input value.
 * 
 *  We will check the flags first.
 *  If the flags match the expected value, we return immediately.
 *  If the flags are not what we expect, we will wait for
 *  half a second, before checking again.
 * 
 *  When the total wait time matches the input wait seconds
 *  we will return with failure.
 *
 * @param rg_object_id - Object id to wait on flags for
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t big_bird_wait_for_raid_group_flags(fbe_object_id_t rg_object_id,
                                                fbe_u32_t wait_seconds,
                                                fbe_raid_group_flags_t flags,
                                                fbe_bool_t b_cleared)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * 1000;

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                             FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if (b_cleared)
        {
            /* Check the flag is cleared.
             */
            if ((rg_info.flags & flags) == 0)
            {
                /* Flags are cleared, just break out and return.
                 */
                break;
            }
        }
        else
        {
            /* Just check that the flag is set.
             */
            if ((rg_info.flags & flags) == flags)
            {
                /* Flags we expected are set, just break out and return.
                 */
                break;
            }
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= (wait_seconds * 1000))
    {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end big_bird_wait_for_raid_group_flags()
 ******************************************/

/*!**************************************************************
 * big_bird_wait_for_all_raid_group_flags()
 ****************************************************************
 * @brief
 *  Wait for all raid group flags to reach the input value.
 * 
 *  We will check the flags first.
 *  If the flags match the expected value, we return immediately.
 *  If the flags are not what we expect, we will wait for
 *  half a second, before checking again.
 * 
 *  When the total wait time matches the input wait seconds
 *  we will return with failure.
 *
 * @param class_id - Class id to wait on flags for
 * @param wait_seconds - max seconds to wait for the flags to change.
 * @param flags - Flag value we are expecting.
 * @param b_cleared - FBE_TRUE if we want to wait for this flag
 *                    to get cleared.
 *                    FBE_FALSE to wait for this flag to get set.
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t big_bird_wait_for_all_raid_group_flags(fbe_class_id_t class_id,    
                                                    fbe_u32_t wait_seconds,
                                                    fbe_raid_group_flags_t flags,
                                                    fbe_bool_t b_cleared)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;

    /* First get all members of this class.
     */
    status = fbe_api_enumerate_class(class_id, FBE_PACKAGE_ID_SEP_0, &obj_list_p, &num_objects);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Iterate over each of the objects we found.
     */
    for ( index = 0; index < num_objects; index++)
    {
        /* Wait for the flags to match on this object.
         */
        status = big_bird_wait_for_raid_group_flags(obj_list_p[index],
                                                    wait_seconds,
                                                    flags, b_cleared);

        if (status != FBE_STATUS_OK)
        {
            /* Failure, break out and return the failure.
             */
            break;
        }
    }
    fbe_api_free_memory(obj_list_p);
    return status;
}
/****************************************************************
 * end big_bird_wait_for_all_raid_group_flags()
 ****************************************************************/

/*!**************************************************************
 * big_bird_quiesce_test()
 ****************************************************************
 * @brief
 *  This test runs a quiesce test with I/O.
 * 
 * @param rg_config_p - Raid group config to test.
 * @param iterations - max number of quiesce/unquiesce iterations.
 *
 * @return None.
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

static void big_bird_quiesce_test(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count, fbe_u32_t iterations)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t test_count;

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();

    /*! @note Currently this test doesn't support peer I/O
     */
    status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode to false
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure there were no errors.
     */
    fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       BIG_BIRD_QUIESCE_TEST_THREADS_PER_LUN, /* threads per lun */
                                       BIG_BIRD_MAX_IO_SIZE_BLOCKS,
                                       FBE_FALSE, /* Don't inject aborts */
                                       FBE_FALSE  /* Peer I/O isn't enabled */);
    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* For each iteration of our test, quiesce and unquiesce with I/O running. 
     * We also validate that we go quiesced and come out of quiesced. 
     */
    for ( test_count = 0; test_count < iterations; test_count++)
    {
        fbe_u32_t sleep_time = 1 + (fbe_random() % BIG_BIRD_QUIESCE_TEST_MAX_SLEEP_TIME);

        mut_printf(MUT_LOG_TEST_STATUS, "=== starting %s iteration %d sleep_time: %d msec===", 
                   __FUNCTION__, test_count, sleep_time);
        fbe_api_sleep(sleep_time);

        fbe_test_sep_util_quiesce_raid_groups(rg_config_p, raid_group_count);
        /* If there is a heavy load on the system it can take a while for the monitor to 
         * run to process the quiesce condition.  Thus we allow 60 seconds here for 
         * getting all the quiesce flags set. 
         */
        fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                        BIG_BIRD_WAIT_SEC, 
                                                        FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED,
                                                        FBE_FALSE    /* Don't wait for clear, wait for set */);

        fbe_test_sep_util_unquiesce_raid_groups(rg_config_p, raid_group_count);

        fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                        BIG_BIRD_WAIT_SEC, 
                                                        FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                        FBE_TRUE    /* Yes, wait for cleared */);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Halt I/O and make sure there were no errors.
     */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end big_bird_quiesce_test()
 ******************************************/
/*!**************************************************************
 * big_bird_write_zero_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param none.      
 *
 * @return None.
 *
 * @author
 *  4/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_write_zero_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    handy_manny_test_setup_fill_rdgen_test_context(context_p,
                                                   FBE_OBJECT_ID_INVALID,
                                                   FBE_CLASS_ID_LUN,
                                                   FBE_RDGEN_OPERATION_ZERO_ONLY,
                                                   FBE_LBA_INVALID, /* use max capacity */
                                                   BIG_BIRD_TEST_ELEMENT_SIZE,
                                                   FBE_RDGEN_PATTERN_ZEROS,
                                                   1, /* passes */
                                                   0 /* I/O count not used */);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    handy_manny_test_setup_fill_rdgen_test_context(context_p,
                                                   FBE_OBJECT_ID_INVALID,
                                                   FBE_CLASS_ID_LUN,
                                                   FBE_RDGEN_OPERATION_READ_CHECK,
                                                   FBE_LBA_INVALID,    /* use max capacity */
                                                   BIG_BIRD_TEST_ELEMENT_SIZE,
                                                   FBE_RDGEN_PATTERN_ZEROS,
                                                   1,    /* passes */
                                                   0    /* I/O count not used */);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end big_bird_write_zero_background_pattern()
 ******************************************/

/*!**************************************************************
 * big_bird_write_background_pattern()
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

void big_bird_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            BIG_BIRD_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            BIG_BIRD_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end big_bird_write_background_pattern()
 ******************************************/
/*!**************************************************************
 * big_bird_read_background_pattern()
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

void big_bird_read_background_pattern(fbe_rdgen_pattern_t pattern)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;

    /* Read back the pattern and check it was written OK.
     */
    handy_manny_test_setup_fill_rdgen_test_context(context_p,
                                                   FBE_OBJECT_ID_INVALID,
                                                   FBE_CLASS_ID_LUN,
                                                   FBE_RDGEN_OPERATION_READ_CHECK,
                                                   FBE_LBA_INVALID, /* use max capacity */
                                                   BIG_BIRD_TEST_ELEMENT_SIZE,
                                                   pattern,
                                                   1, /* passes */
                                                   0 /* I/O count not used */);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end big_bird_read_background_pattern()
 ******************************************/

/*!**************************************************************
 * big_bird_remove_sas_drive()
 ****************************************************************
 * @brief
 *  Remove a sas drive and wait until the pvd and vd transition
 *  to the failed state.
 *
 * @param drive_to_remove_p - Ptr to description of drive to remove.
 * @param pvd_object_id - object id of pvd. Assumed to exist.
 * @param vd_object_id - object id of vd. Assumed to exist.
 *
 * @return None.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/
static void big_bird_remove_sas_drive(fbe_test_raid_group_disk_set_t *drive_to_remove_p,
                                      fbe_object_id_t pvd_object_id,
                                      fbe_object_id_t vd_object_id)
{
    fbe_status_t status;
    /* Remove the drive.
     */
    status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                      drive_to_remove_p->enclosure, 
                                      drive_to_remove_p->slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: vd: %d pvd: %d waiting for PDO and LDO to be destroyed", 
               __FUNCTION__, pvd_object_id, vd_object_id);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end big_bird_remove_sas_drive()
 ******************************************/

/*!**************************************************************
 * big_bird_remove_sata_drive()
 ****************************************************************
 * @brief
 *  Remove a sas drive and wait until the pvd and vd transition
 *  to the failed state.
 *
 * @param drive_to_remove_p - Ptr to description of drive to remove.
 * @param pvd_object_id - object id of pvd. Assumed to exist.
 * @param vd_object_id - object id of vd. Assumed to exist.
 *
 * @return None.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/
static void big_bird_remove_sata_drive(fbe_test_raid_group_disk_set_t *drive_to_remove_p,
                                      fbe_object_id_t pvd_object_id,
                                      fbe_object_id_t vd_object_id)
{
    fbe_status_t status;
    /* Remove the drive.
     */
    status = fbe_test_pp_util_remove_sata_drive(drive_to_remove_p->bus, 
                                                drive_to_remove_p->enclosure, 
                                                drive_to_remove_p->slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: vd: %d pvd: %d waiting for PDO and LDO to be destroyed", 
               __FUNCTION__, pvd_object_id, vd_object_id);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                     BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end big_bird_remove_sata_drive()
 ******************************************/


fbe_u32_t big_bird_get_next_position_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_test_drive_removal_mode_t removal_mode)
{
    /* Remove a random position.
     */
    return fbe_test_sep_util_get_next_position_to_remove(rg_config_p, removal_mode);
}
fbe_u32_t big_bird_get_next_position_to_insert(fbe_test_rg_configuration_t *rg_config_p)
{
    /*! @todo Use fbe_test_sep_util_get_random_position_to_insert()
     */
    return fbe_test_sep_util_get_next_position_to_insert(rg_config_p);
}
fbe_u32_t big_bird_get_position_to_spare(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t index)
{
    return fbe_test_sep_util_get_needing_spare_position_from_index(rg_config_p, index);
}


/*!**************************************************************
 * big_bird_remove_drives()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param b_transition_pvd_fail - FBE_TRUE - Force PVD to fail 
 * @param b_pull_drive - FBE_TRUE - Pull and re-insert same drive
 *                       FBE_FALSE - Replace drive 
 * @param removal_mode - random or sequential
 *
 * @return None.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *  05/23/2011 - Modified. Amit Dhaduk
 *
 ****************************************************************/

void big_bird_remove_drives(fbe_test_rg_configuration_t *rg_config_p, 
                            fbe_u32_t raid_group_count,
                            fbe_bool_t b_transition_pvd_fail,
                            fbe_bool_t b_pull_drive,
                            fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_u32_t rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t position_to_remove;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    /* For every raid group remove one drive.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        position_to_remove = big_bird_get_next_position_to_remove(current_rg_config_p, removal_mode);
        fbe_test_sep_util_add_removed_position(current_rg_config_p, position_to_remove);        
        drive_to_remove_p = &current_rg_config_p->rg_disk_set[position_to_remove];

        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Removing drive raid group: %d position: 0x%x (%d_%d_%d)", 
                   __FUNCTION__, rg_index, position_to_remove, 
                   drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot);

        if (b_transition_pvd_fail)
        {
            fbe_object_id_t pvd_id;
            status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus,
                                                                    drive_to_remove_p->enclosure,
                                                                    drive_to_remove_p->slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pvd_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else 
        {

           /* We are going to pull a drive.
             */
            if(fbe_test_util_is_simulation())
            {
                /* We only ever pull drives since on hardware and simulation we 
                 * test the same way.  Pull the drive out and then re-insert it. 
                 * Or pull the drive out, let a spare swap in and then re-insert the pulled drive. 
                 * This is because we cannot insert "new" drives on hardware and we would like to 
                 * test the same way on hardware and in simulation. 
                 */
                if (1 || b_pull_drive)
                {
                    status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                         drive_to_remove_p->enclosure, 
                                                         drive_to_remove_p->slot,
                                                         &current_rg_config_p->drive_handle[position_to_remove]);
                }
                else
                {
                    status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                               drive_to_remove_p->enclosure, 
                                                               drive_to_remove_p->slot);
                }
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if (fbe_test_sep_util_get_dualsp_test_mode() && fbe_test_sep_util_should_remove_on_both_sps())
                {
                    fbe_api_sim_transport_set_target_server(other_sp);
                    if (1 || b_pull_drive)
                    {
                        status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                             drive_to_remove_p->enclosure, 
                                                             drive_to_remove_p->slot,
                                                             &current_rg_config_p->peer_drive_handle[position_to_remove]);
                    }
                    else
                    {
                        status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                                   drive_to_remove_p->enclosure, 
                                                                   drive_to_remove_p->slot);
                    }
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    fbe_api_sim_transport_set_target_server(this_sp);
                }
            }
            else
            {
                status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode() && fbe_test_sep_util_should_remove_on_both_sps())
                {
                    /* Set the target server to SPB and remove the drive there. */
                    fbe_api_sim_transport_set_target_server(other_sp);
                    status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure, 
                                                            drive_to_remove_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(this_sp);
                }
            }
        }
        current_rg_config_p++;
    }

    /* Now wait for the drives to be destroyed.
     * We intentionally separate out the waiting from the actions to allow all the 
     * transitioning of drives to occur in parallel. 
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

        /* The last drive removed for this raid group is at the end of the removed drive array.
         */
        position_to_remove = current_rg_config_p->drives_removed_array[current_rg_config_p->num_removed - 1];

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        if (fbe_test_sep_util_should_remove_on_both_sps())
        {
            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(
                fbe_test_sep_util_get_dualsp_test_mode(),
                vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            /* VD stays ready if we don't remove on both SPs.
             */
            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(
                fbe_test_sep_util_get_dualsp_test_mode(),
                vd_object_id, FBE_LIFECYCLE_STATE_READY,
                BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end big_bird_remove_drives()
 ******************************************/

/*!**************************************************************
 * big_bird_insert_drives()
 ****************************************************************
 * @brief
 *  insert drives, one per raid group.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 * @param b_transition_pvd - True to not use terminator to reinsert.
 *
 * @return None.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_u32_t raid_group_count,
                            fbe_bool_t b_transition_pvd)
{
    fbe_u32_t index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t status;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_insert;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    /* For every raid group insert one drive.
     */
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        position_to_insert = big_bird_get_next_position_to_insert(current_rg_config_p);
        fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert);
        drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];

        mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting rg %d position %d (%d_%d_%d). ==", 
                   __FUNCTION__, index, position_to_insert, 
                   drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);

        if (b_transition_pvd)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus,
                                                                    drive_to_insert_p->enclosure,
                                                                    drive_to_insert_p->slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pvd_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else 
        {
            /* inserting the same drive back */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot,
                                                     current_rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if (fbe_test_sep_util_get_dualsp_test_mode() && fbe_test_sep_util_should_remove_on_both_sps())
                {
                    fbe_api_sim_transport_set_target_server(other_sp);
                    status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                             drive_to_insert_p->enclosure, 
                                                             drive_to_insert_p->slot,
                                                             current_rg_config_p->peer_drive_handle[position_to_insert]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    fbe_api_sim_transport_set_target_server(this_sp);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode() && fbe_test_sep_util_should_remove_on_both_sps())
                {
                    /* Set the target server to SPB and insert the drive there. */
                    fbe_api_sim_transport_set_target_server(other_sp);
                    status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                                drive_to_insert_p->enclosure, 
                                                                drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(this_sp);
                }
            }
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end big_bird_insert_drives()
 ******************************************/

/*!**************************************************************
 * big_bird_configure_hot_spares()
 ****************************************************************
 * @brief
 *  Wait for the PVDs to become ready and make them sparable.
 *  This will allow these PVDs to permanent spare into our
 *  raid groups.  Note we assume that the PVDs have been inserted,
 *  but are not necessarily ready when we enter this function.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 *
 * @return None.
 *
 * @author
 *  11/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_configure_hot_spares(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t raid_group_count)
{
    fbe_u32_t index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_spare;

    /* For every raid group spare the drives we had removed.
     */
    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_u32_t drive_index;
        fbe_u32_t num_needing_spare;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        num_needing_spare = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);

        for ( drive_index = 0; drive_index < num_needing_spare; drive_index++)
        {
            position_to_spare = big_bird_get_position_to_spare(current_rg_config_p, drive_index);
            fbe_test_sep_util_delete_needing_spare_position(current_rg_config_p, position_to_spare);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

            mut_printf(MUT_LOG_TEST_STATUS, "== %s sparing rg %d position %d (%d_%d_%d). ==", 
                       __FUNCTION__, index, position_to_spare, 
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Wait for the PVD to be ready.  We need to wait at a minimum for the object to be created,
             * but the PVD is not spareable until it is ready. 
             */
            status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                          drive_to_spare_p->enclosure, 
                                                          drive_to_spare_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          BIG_BIRD_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            /* Configure it as a hot spare so it will become part of the raid group.
             */
            status = fbe_test_sep_util_configure_drive_as_hot_spare(drive_to_spare_p->bus, 
                                                                    drive_to_spare_p->enclosure, 
                                                                    drive_to_spare_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end big_bird_configure_hot_spares()
 ******************************************/

/*!**************************************************************
 * big_bird_wait_for_pvds_to_fail()
 ****************************************************************
 * @brief
 *  Wait for the PVDs to go offline.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 *
 * @return None.
 *
 * @author
 *  11/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_wait_for_pvds_to_fail(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_u32_t raid_group_count)
{
    fbe_u32_t index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_spare;

    /* For every raid group wait for the drive to fail.
     */
    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_u32_t drive_index;
        fbe_u32_t num_needing_spare;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        num_needing_spare = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);

        for ( drive_index = 0; drive_index < num_needing_spare; drive_index++)
        {
            position_to_spare = big_bird_get_position_to_spare(current_rg_config_p, drive_index);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

            /* Wait for the PVD to fail.
             */
            status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                          drive_to_spare_p->enclosure, 
                                                          drive_to_spare_p->slot,
                                                          FBE_LIFECYCLE_STATE_FAIL,
                                                          BIG_BIRD_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end big_bird_wait_for_pvds_to_fail()
 ******************************************/
/*!**************************************************************
 * big_bird_remove_all_drives()
 ****************************************************************
 * @brief
 *  Remove all the drives for this test.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_to_remove - Number of drives we are removing for each raid group
 * @param msecs_between_removals - Milliseconds to wait in between removals.
 * @param removal_mode - random or sequential
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *  5/23/2011 - Modified. Amit Dhaduk
 *
 ****************************************************************/

void big_bird_remove_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                fbe_u32_t raid_group_count, 
                                fbe_u32_t drives_to_remove,
                                fbe_bool_t b_reinsert_same_drive,
                                fbe_u32_t msecs_between_removals,
                                fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_u32_t drive_number;

    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);


    if (b_reinsert_same_drive == FBE_FALSE)
    {
        /* Refresh the locations of the all extra drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);
    }

    /* For each raid group remove the specified number of drives.
     */
    for (drive_number = 0; drive_number < drives_to_remove; drive_number++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive index %d of %d. ==", __FUNCTION__, drive_number, drives_to_remove);
        big_bird_remove_drives(rg_config_p, 
                               raid_group_count, 
                               FBE_FALSE,    /* don't fail pvd */
                               b_reinsert_same_drive,
                               removal_mode);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive index %d of %d. Completed. ==", __FUNCTION__, drive_number, drives_to_remove);
    }
    return;
}
/******************************************
 * end big_bird_remove_all_drives()
 ******************************************/
/*!**************************************************************
 * big_bird_insert_all_drives()
 ****************************************************************
 * @brief
 *  Remove all the drives for this test.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_to_insert - Number of drives we are removing.
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_insert_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                fbe_u32_t raid_group_count, 
                                fbe_u32_t drives_to_insert,
                                fbe_bool_t b_reinsert_same_drive)
{
    fbe_u32_t drive_number;
    fbe_u32_t num_to_insert = 0;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            break;
        }
        current_rg_config_p++;
    }
    MUT_ASSERT_NOT_NULL(current_rg_config_p);

    /* Assumption is that all raid groups under test have the same number of
     * drives removed.
     */
    num_to_insert = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);


    if (b_reinsert_same_drive == FBE_FALSE)
    {

        /* to insert new drive, configure extra drive as spare 
         */
         fbe_test_sep_drive_configure_extra_drives_as_hot_spares(rg_config_p, raid_group_count);

        /* make sure that hs swap in complete 
         */        
         fbe_test_sep_drive_wait_extra_drives_swap_in(rg_config_p, raid_group_count);

         /* set target server to SPA (test expects SPA will be left as target server and previous call may change that) 
          */
         fbe_api_sim_transport_set_target_server(this_sp);
    }
    else
    {

        /* insert the same drive back 
         */
        for (drive_number = 0; drive_number < num_to_insert; drive_number++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive index %d of %d. ==", __FUNCTION__, drive_number, num_to_insert);
            big_bird_insert_drives(rg_config_p, 
                                   raid_group_count, 
                                   FBE_FALSE);    /* don't fail pvd */

            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive index %d of %d. Complete. ==", __FUNCTION__, drive_number, num_to_insert);
        }
    }

    return;
}
/******************************************
 * end big_bird_insert_all_drives()
 ******************************************/

fbe_rdgen_pattern_t big_bird_get_pattern_for_operation(fbe_rdgen_operation_t rdgen_operation)
{
    fbe_rdgen_pattern_t pattern;

    if ( (rdgen_operation == FBE_RDGEN_OPERATION_WRITE_READ_CHECK) || 
         (rdgen_operation == FBE_RDGEN_OPERATION_WRITE_ONLY) || 
         (rdgen_operation == FBE_RDGEN_OPERATION_READ_ONLY))
    {
        pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    }
    else if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        pattern = FBE_RDGEN_PATTERN_ZEROS;
    }
    else
    {
        MUT_FAIL_MSG("Unexpected rdgen operation");
    }
    return pattern;
}

/*!**************************************************************
 * big_bird_rg_wait_for_all_drive_rebuilds()
 ****************************************************************
 * @brief
 *  Wait for all the rebuilds in a raid group to complete.
 *
 * @param rg_config_p - Our configuration.
 * @param drives_removed - Number of drives we removed.
 *
 * @return None 
 *
 * @author
 *  7/26/2010 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_rg_wait_for_all_drive_rebuilds(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t drives_removed)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_u32_t drive_num;
    fbe_u32_t index = drives_removed - 1;
    fbe_u32_t position;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Loop over a set of positions to wait for.
     */
    for (drive_num = 0; drive_num < drives_removed; drive_num++)
    {
        position = fbe_test_sep_get_drive_remove_history(rg_config_p, index);

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            /* For Raid 10, we need to get the object id of the mirror.
             * Issue the control code to RAID group to get its downstream edge list. 
             */  
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* The first and second positions to remove will be considered to land in the first pair.
             */
            status = fbe_test_sep_util_wait_rg_pos_rebuild(downstream_object_list.downstream_object_list[position / 2], 
                                                           position % 2,
                                                           BIG_BIRD_WAIT_SEC * 10/*Temporary*/   /* Time to wait */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "waiting for rebuild rg_id: %d obj: 0x%x position: %d",
                       rg_config_p->raid_group_id, rg_object_id, position);
            status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position,
                                                           BIG_BIRD_WAIT_SEC  * 10/*Temporary*/  /* Time to wait */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        index--;
    }
    return;
}
/**************************************
 * end big_bird_rg_wait_for_all_drive_rebuilds()
 **************************************/
/*!**************************************************************
 * big_bird_wait_for_rebuilds()
 ****************************************************************
 * @brief
 *  Wait for all the rebuilds in a raid group to complete.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_removed - Number of drives we removed.
 *
 * @return None 
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_wait_for_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_u32_t raid_group_count,
                                fbe_u32_t drives_removed)
{
    fbe_u32_t index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        big_bird_rg_wait_for_all_drive_rebuilds(current_rg_config_p, drives_removed);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(other_sp);
            big_bird_rg_wait_for_all_drive_rebuilds(current_rg_config_p, drives_removed);
            fbe_api_sim_transport_set_target_server(this_sp);
        }
        
        current_rg_config_p++;
    }
    big_bird_validate_rebuilds_complete(rg_config_p, raid_group_count); 
    return;
}
/******************************************
 * end big_bird_wait_for_rebuilds()
 ******************************************/

/*!**************************************************************
 * big_bird_validate_rebuilds_complete()
 ****************************************************************
 * @brief
 *  Validate the rebuilds have completed by looking at the bitmap.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_removed - Number of drives we removed.
 *
 * @return None 
 *
 * @author
 *  4/26/2011 - Created. Rob Foley
 *  7/14/2011 - Modified Sandeep Chaudhari
 ****************************************************************/

void big_bird_validate_rebuilds_complete(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A; 

    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
   
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* This is here for debugging.  If the below check fails we will want to have this info handy.
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If the rebuild is done, no chunks should be needing a rebuild.
         */
        if (paged_info.num_nr_chunks > 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x chunks %llu bitmap 0x%x", 
                       __FUNCTION__, rg_object_id,
		       (unsigned long long)paged_info.num_nr_chunks,
		       paged_info.needs_rebuild_bitmask);
            MUT_FAIL_MSG("There are chunks needing rebuild on SP A");
        }

        /* In the case of a dual sp test we will also check the peer since 
         * some of this information can be cached on the peer.  We want to make sure it is correct there also. 
         */
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if (paged_info.num_nr_chunks > 0)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x chunks %llu bitmap 0x%x", 
                           __FUNCTION__, rg_object_id,
			   (unsigned long long)paged_info.num_nr_chunks,
			   paged_info.needs_rebuild_bitmask);
                MUT_FAIL_MSG("There are chunks needing rebuild on SP B");				
            }
            fbe_api_sim_transport_set_target_server(this_sp);
        }
        
        
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end big_bird_validate_rebuilds_complete()
 ******************************************/
/*!**************************************************************
 * big_bird_setup_test_contexts()
 ****************************************************************
 * @brief
 *  Setup the test context for a given LUN.
 *
 * @param context_p - Context to init.
 * @param rdgen_operation - Operation to use.
 * @param lun_object_id - lun to send I/O to.
 * @param threads - Threads of I/O to use.
 * @param io_size_blocks - Max blocks to use.
 * @param ran_abort_msecs - invalid if we're not aborting,
 *                          otherwise it is the number of msec
 *                          to abort after.
 * @param peer_options - Invalid if we do not need to use the peer,
 *                       otherwise it indicates how we should use the peer.
 *
 * @return None.
 *
 * @author
 *  8/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_setup_test_contexts(fbe_api_rdgen_context_t *context_p,
                                  fbe_rdgen_operation_t rdgen_operation,
                                  fbe_object_id_t lun_object_id, 
                                  fbe_u32_t threads, 
                                  fbe_u32_t io_size_blocks, 
                                  fbe_test_random_abort_msec_t ran_abort_msecs, 
                                  fbe_api_rdgen_peer_options_t peer_options)
{   
    /* Max number of lba specs we will randomize over: cat inc, cat dec, random.
     */
    #define BIG_BIRD_MAX_LBA_SPECS 3 
    fbe_status_t status;
    fbe_rdgen_pattern_t pattern = big_bird_get_pattern_for_operation(rdgen_operation);
    
    fbe_rdgen_lba_specification_t lba_spec = FBE_RDGEN_LBA_SPEC_RANDOM;
    fbe_u32_t random_number = fbe_random() % BIG_BIRD_MAX_LBA_SPECS;

    if ((peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE) ||
        (peer_options == FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER))
    {
        /* We randomly pick increasing, decreasing or random.
         * This helps give variability to the test. 
         */
        switch (random_number)
        {
            case 0:
                lba_spec = FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING;
                mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x using lba spec of DECreasing peer options of %d", 
                           __FUNCTION__, lun_object_id, peer_options);
                break;
            case 1:
                lba_spec = FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING;
                mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x using lba spec of INCreasing peer options of %d", 
                           __FUNCTION__, lun_object_id, peer_options);
                break;
            default:
                lba_spec = FBE_RDGEN_LBA_SPEC_RANDOM;
                mut_printf(MUT_LOG_TEST_STATUS, "%s obj: 0x%x using lba spec of RANDOM peer options of %d", 
                           __FUNCTION__, lun_object_id, peer_options);
                break;
        };
    }

#ifdef __SAFE__
    /* on unity, we have less DPS memory. we have to limit the threads number up to 4.
     * otherwise heavy IO will cause memory contention, which will dramatically increase the IO processing time.
     * rdgen will cancel the IO when it could not be completed in 60s.
     */
    threads = fbe_test_sep_io_get_threads(threads);
    threads = (threads >= 5 ? 4 : threads);
#endif
    

    /*! Now configure the rdgen test context values
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                             lun_object_id,
                                             FBE_CLASS_ID_INVALID, FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             pattern,
                                             0,    /* Run forever */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             fbe_test_sep_io_get_threads(threads),    /* One thread for each SP. */
                                             lba_spec,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* min blocks */
                                             io_size_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Now simply run setup context supplied
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                    peer_options);

    /* Check if we are asked to generate random aborts
     */
    if (ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification, 
                                                 ran_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, 
                                                            FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
}
/******************************************
 * end big_bird_setup_test_contexts()
 ******************************************/

/*!***************************************************************************
 *  big_bird_setup_lun_test_contexts
 *****************************************************************************
 *
 * @brief   Run the rdgen test setup method passed against all luns for all
 *          raid groups specified.
 *
 * @param rg_config_p - The raid group configuration array under test.
 * @param context_p - Array of rdgen test contexts, one per LUN.
 * @param num_contexts - Total number of contexts in array.
 * @param rdgen_operation - Operation to setup for.
 * @param threads - Number of threads to kick off.
 * @param io_size_blocks - Max I/O size.
 * @param ran_abort_msecs - Number of seconds to abort after.
 * @param peer_options - If we should run dual sp I/O or not.
 * 
 * @return fbe_status_t
 *
 * @author
 *  8/22/2011 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t big_bird_setup_lun_test_contexts(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_api_rdgen_context_t *context_p,
                                              fbe_u32_t num_contexts,
                                              fbe_rdgen_operation_t rdgen_operation,
                                              fbe_u32_t threads,
                                              fbe_u32_t io_size_blocks,
                                              fbe_test_random_abort_msec_t ran_abort_msecs,
                                              fbe_api_rdgen_peer_options_t peer_options,
                                              fbe_bool_t b_stripe_align)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_api_rdgen_context_t     *current_context_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index = 0;
    fbe_object_id_t             lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                   total_luns_processed = 0;
    fbe_api_raid_group_get_info_t rg_info;

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    current_context_p = context_p;
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id,
                                                     &rg_object_id);
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* For this raid group iterate over all the luns
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            fbe_lun_number_t lun_number = current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

            /* Validate that we haven't exceeded the number of context available.
             */
            if (total_luns_processed >= num_contexts) 
            {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "setup context for all luns: rg_config_p: 0x%p luns processed: %d exceeds contexts: %d",
                           rg_config_p, total_luns_processed, num_contexts); 
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Get the lun object id from the lun number
             */
            MUT_ASSERT_INT_EQUAL(current_rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, 
                                 current_rg_config_p->raid_group_id);
            status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            big_bird_setup_test_contexts(current_context_p, rdgen_operation, lun_object_id,
                                         threads, io_size_blocks, ran_abort_msecs, peer_options);

            if (b_stripe_align)
            {
                status = fbe_api_rdgen_io_specification_set_blocks(&current_context_p->start_io.specification, 
                                                                   FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                                                   rg_info.sectors_per_stripe, rg_info.sectors_per_stripe);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_rdgen_io_specification_set_alignment_size(&current_context_p->start_io.specification, 
                                                                           (fbe_u32_t)rg_info.sectors_per_stripe);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            /* Increment the number of luns processed and the test context pointer
             */
            total_luns_processed++;
            current_context_p++;

        } /* end for all luns in a raid group */

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */

    /* Else return success
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end big_bird_setup_lun_test_contexts()
 ************************************************************/

/*!**************************************************************
 * big_bird_start_io()
 ****************************************************************
 * @brief
 *  Run I/O.
 *  Degrade the raid group.
 *  Allow I/O to continue degraded.
 *  Bring drive back and allow it to rebuild.
 *  Stop I/O.
 *
 * @param threads - number of threads to pass to rdgen.
 * @param io_size_blocks - Max I/O size in blocks for rdgen.
 * @param rdgen_operation - opcode to pass to rdgen.
 * @param peer_options - To send this to peer or not.
 *
 * @return None.
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_start_io(fbe_test_rg_configuration_t * const rg_config_p,
                       fbe_api_rdgen_context_t **context_p,
                       fbe_u32_t threads,
                       fbe_u32_t io_size_blocks,
                       fbe_rdgen_operation_t rdgen_operation,
                       fbe_test_random_abort_msec_t ran_abort_msecs,
                       fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_u32_t num_luns;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = big_bird_setup_lun_test_contexts(rg_config_p,
                                              *context_p,
                                              num_luns,
                                              rdgen_operation,
                                              threads,
                                              io_size_blocks, ran_abort_msecs, peer_options,
                                              FBE_FALSE /* don't stripe align */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(*context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end big_bird_start_io()
 ******************************************/
/*!**************************************************************
 * fbe_test_io_wait_for_io_count()
 ****************************************************************
 * @brief
 *  Confirms that for all context we attained at least a certain
 *  I/O count.
 *
 * @param context_p - Array of contexts.
 * @param num_tests - Number of entries in array.
 * 
 * @return fbe_status_t
 *
 * @note
 *  We assume the structure was previously initted via a
 *  call to fbe_api_rdgen_test_context_init().
 *
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  9/27/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_io_wait_for_io_count(fbe_api_rdgen_context_t *context_p,
                                   fbe_u32_t num_tests,
                                   fbe_u32_t io_count,
                                   fbe_u32_t timeout_msecs)
{
    fbe_u32_t test_index;
    fbe_status_t status;

    /* Iterate over all tests.
     * If we find one that has not started enough threads or has not started enough I/Os, then 
     * just wait for it to complete. 
     */
    for (test_index = 0; test_index < num_tests; test_index++)
    {
        fbe_api_rdgen_get_stats_t stats;
        fbe_rdgen_filter_t filter;
        fbe_u32_t total_ios;
        fbe_u32_t msecs = 0;
        fbe_api_rdgen_filter_init(&filter, 
                                  context_p[test_index].start_io.filter.filter_type, 
                                  context_p[test_index].start_io.filter.object_id, 
                                  context_p[test_index].start_io.filter.class_id, 
                                  context_p[test_index].start_io.filter.package_id,
                                  0    /* edge index */);
        status = fbe_api_rdgen_get_stats(&stats, &filter);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        total_ios = context_p[test_index].start_io.specification.threads * io_count;

        /* Make sure all our threads have started and on average have 
         * completed at least a few I/Os per thread. 
         */
        while ((stats.threads != context_p[test_index].start_io.specification.threads) ||
               ((stats.io_count + stats.error_count) < total_ios))
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "waiting for I/Os to start for obj: 0x%x threads: 0x%x ios: 0x%x\n",
                          context_p[test_index].start_io.filter.object_id,
                          stats.threads, stats.io_count);
            fbe_api_sleep(500);
            /* We always increment and never reset this to zero so that our 
             * total wait time is not longer than timeout_msecs. 
             */
            msecs += 500;
            if (msecs > timeout_msecs)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_INFO, "waiting for I/Os to start for obj: 0x%x threads: 0x%x ios: 0x%x\n",
                              context_p[test_index].start_io.filter.object_id,
                               stats.threads, stats.io_count);
                MUT_FAIL_MSG("exceeded wait time");
            }
            status = fbe_api_rdgen_get_stats(&stats, &filter);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
    return;
}
/**************************************
 * end fbe_test_io_wait_for_io_count()
 **************************************/

/*!**************************************************************
 * big_bird_degraded_shutdown_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.
 *
 * @param rg_config_p - Current configuration
 * @param raid_group_count - number of rgs under test.
 * @param luns_per_rg - Number of LUNs for each RG.
 * @param peer_options - How to run rdgen I/O local or with peer.            
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_degraded_shutdown_test(fbe_test_rg_configuration_t * const rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_u32_t luns_per_rg,
                                     fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_drives_to_remove;
    fbe_test_drive_removal_mode_t removal_mode = FBE_DRIVE_REMOVAL_MODE_RANDOM; /* Default to Random. */
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting peer options: %d==", __FUNCTION__, peer_options);

    /* Run I/O degraded.
     */
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, BIG_BIRD_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(big_bird_io_msec_short);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, BIG_BIRD_WAIT_IO_COUNT, BIG_BIRD_WAIT_MSEC);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        num_drives_to_remove = 3;
    }
    else if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        num_drives_to_remove = 1;
    }
    else if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Raid 10 goes sequential to insure a shutdown.
         */
        removal_mode = FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL;
        num_drives_to_remove = 2;
    }
    else
    {
        num_drives_to_remove = 2;
    }
    big_bird_remove_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);

		if(status != FBE_STATUS_OK){
			fbe_test_common_panic_both_sps();
		}

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }


    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors.
     */
    /* there is little chance that no outstanding IO is on the 2nd drive while it is removed 
     * so we might not have error counts. on the other hand, even there is io_err, it might be introduced by packet transport error, not block error
     */
    // MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
    // MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);


    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives ==", __FUNCTION__);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting drives successful. ==", __FUNCTION__);

    fbe_api_sleep(big_bird_io_msec_long);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, num_drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************
 * end big_bird_degraded_shutdown_test()
 ******************************************/

/*!**************************************************************
 * big_bird_write_log_test_basic()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.
 *
 * @param rg_config_p - Current configuration
 * @param raid_group_count - number of rgs under test.
 * @param luns_per_rg - Number of LUNs for each RG.
 * @param peer_options - How to run rdgen I/O local or with peer.            
 *
 * @return None.
 *
 * @author
 *  4/2011 - Created. Dan Cummins.
 *
 ****************************************************************/
void big_bird_write_log_test_basic(fbe_test_rg_configuration_t * const rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   fbe_u32_t luns_per_rg,
                                   fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t num_drives_to_remove;
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run I/O degraded.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, BIG_BIRD_MAX_IO_SIZE_BLOCKS, 
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                      FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(1000);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, BIG_BIRD_WAIT_IO_COUNT, BIG_BIRD_WAIT_MSEC);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        num_drives_to_remove = 2;
    }
    else
    {
        num_drives_to_remove = 1;
    }

    /* degrade the arrays
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s degrading arrays pull %d drives ==", __FUNCTION__, num_drives_to_remove);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0 /* msec wait between removals */,
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* allow IO to continue
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s contiue I/O on degraded arrays for ~5 seconds ==", __FUNCTION__);
    fbe_api_sleep(big_bird_io_msec_long + 5000);

    /* pull one more drive which will take the array offline
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s pull one more drive ==", __FUNCTION__);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, 1,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for arrays to fail ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
         if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s arrays have failed, halting I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  I/O halted ==", __FUNCTION__);

    /* Make sure there were errors.
     */
    /* there is little chance that no outstanding IO is on the 2nd drive while it is removed 
     * so we might not have error counts. on the other hand, even there is io_err, it might be introduced by packet transport error not block error
     */
    // MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
    // MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

    /* Put the last drive back in...
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s reinsert last pulled drive ==", __FUNCTION__);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, 1,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s last drive inserted ==", __FUNCTION__);

    fbe_api_sleep(big_bird_io_msec_long);
    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for arrays to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s insert remaining drives ==", __FUNCTION__);
    big_bird_insert_all_drives(rg_config_p, raid_group_count, num_drives_to_remove,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s drives inserted ==", __FUNCTION__);

    /* reinsert remaining drives
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, num_drives_to_remove + 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebuilds finished ==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************
 * end big_bird_write_log_test_basic()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_get_rg_num_nr_chunks()
 ****************************************************************
 * @brief
 *  This is a rebuild logging test.
 *  We pull out drives run a bit of I/O,
 *  and then re-insert the same drives.
 *
 * @param rg_config_p - Raid group config under test.
 * @param raid_group_count - Number of raid groups total in the list.
 * @param expected_num_nr_chunks - number of chunks we expect to
 *                                 be marked NR on each RG.
 * 
 *
 * @return None.
 *
 * @author
 *  3/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_get_rg_num_nr_chunks(fbe_test_rg_configuration_t * const rg_config_p,
                                            fbe_u32_t raid_group_count,
                                            fbe_u32_t expected_num_nr_chunks)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_api_raid_group_get_paged_info_t paged_info;
        fbe_object_id_t rg_object_id;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;
            fbe_u32_t mirror_index;
            fbe_api_raid_group_get_paged_info_t mirror_paged_info;
            fbe_zero_memory(&paged_info, sizeof(paged_info));

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            /* For a raid 10 the downstream object list has the IDs of the mirrors. 
             * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
             */
            for (mirror_index = 0; 
                  mirror_index < downstream_object_list.number_of_downstream_objects; 
                  mirror_index++)
            {
                status = fbe_api_raid_group_get_paged_bits(downstream_object_list.downstream_object_list[mirror_index], 
                                                           &mirror_paged_info, FBE_TRUE /* Get data from disk*/);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                paged_info.num_nr_chunks += mirror_paged_info.num_nr_chunks;
            }
        }
        else
        {
            status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, FBE_TRUE /* Get data from disk*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* The test ran two I/Os per LUN.  So we should have 2 * LUNS_PER_RG number of chunks 
         * marked. 
         */
        if (paged_info.num_nr_chunks != expected_num_nr_chunks)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "num nr chunks 0x%llx != 0x%x for rg id: %d rg object id: 0x%x",
                       (unsigned long long)paged_info.num_nr_chunks,
		       expected_num_nr_chunks,
		       current_rg_config_p->raid_group_id, rg_object_id);
            MUT_FAIL();
        }
        current_rg_config_p++;
    }
    return;
}
/*!**************************************************************
 * big_bird_simple_rebuild_logging_test()
 ****************************************************************
 * @brief
 *  This is a rebuild logging test.
 *  We pull out drives run a bit of I/O,
 *  and then re-insert the same drives.
 *
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  3/29/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_simple_rebuild_logging_test(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t luns_per_rg,
                                          fbe_element_size_t element_size,
                                          fbe_u32_t drives_to_remove,
                                          fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_block_count_t test_blocks;
    fbe_api_rdgen_send_one_io_params_t params;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    
    test_blocks = element_size * drives_to_remove;

    /* Zero out the data with all drives inserted.
     * We will write a pattern when the drive is removed. 
     * We will then check to make sure the pattern was read back. 
     */
    big_bird_set_sp_for_single_io(peer_options);
    fbe_api_rdgen_io_params_init(&params);
    params.object_id = FBE_OBJECT_ID_INVALID;
    params.class_id = FBE_CLASS_ID_LUN;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE;

    params.rdgen_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK;
    params.pattern = FBE_RDGEN_PATTERN_ZEROS;
    params.lba = 0;
    params.blocks = 1;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Use invalid lba to write to the last stripe.
     */
    params.lba = FBE_LBA_INVALID;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Run fixed I/O to mark 1 chunk as needing to be rebuilt. 
     * We write the first stripe of the unit. 
     */
    big_bird_set_sp_for_single_io(peer_options);
    params.lba = 0;
    params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    fbe_test_sep_util_get_rg_num_nr_chunks(rg_config_p, raid_group_count, luns_per_rg);


    /* Run fixed I/O to mark 1 more chunk as needing to be rebuilt (per lun).
     */
    /* Use invalid lba to write to the last stripe.
     */
    params.lba = FBE_LBA_INVALID;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    fbe_test_sep_util_get_rg_num_nr_chunks(rg_config_p, raid_group_count, luns_per_rg * 2);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);


    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);
    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Make sure the data was rebuilt.
     * We are reading back and checking for the pattern. 
     * Remember that since we wrote a pattern of zeros at the beginning then if 
     * no data was rebuilt we would get back zeros. 
     */
    params.lba = 0;
    params.rdgen_operation = FBE_RDGEN_OPERATION_READ_CHECK;
    big_bird_set_sp_for_single_io(peer_options);
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    status = fbe_api_rdgen_send_one_io(context_p, FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN, /* Issue I/O to every LUN. */
                                       FBE_PACKAGE_ID_SEP_0,
                                       FBE_RDGEN_OPERATION_READ_CHECK,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       0, /* lba */
                                       test_blocks /* blocks */,
                                       FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    /* Read back another chunk at the end of the LUN.
     */
    params.lba = FBE_LBA_INVALID;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end big_bird_simple_rebuild_logging_test()
 ******************************************/

/*!**************************************************************
 * fbe_test_sep_util_run_rdgen_for_pvd_ex()
 ****************************************************************
 * @brief
 *  Run rdgen across a range of blocks on the PVD.
 *
 * @param pvd_object_id
 * @param operation - Rdgen operation to start.
 * @param pattern - Rdgen pattern to use.
 * @param start_lba - Start of range to run rdgen operation.
 * @param blocks - Size of range to run operation on.
 *
 * @return None   
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_run_rdgen_for_pvd_ex(fbe_object_id_t pvd_object_id, 
                                            fbe_package_id_t package_id,
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t context;

    status = fbe_api_rdgen_test_context_init(&context, 
                                             pvd_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             package_id,
                                             operation,
                                             pattern,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             start_lba,    /* Start lba*/
                                             start_lba,    /* Min lba */
                                             start_lba + blocks - 1,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             0x800,    /* Min blocks */
                                             0x800    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* We would rather not panic the SP on a failure, but 
     * rather just fail the test. 
     */
    status = fbe_api_rdgen_io_specification_set_options(&context.start_io.specification,
                                                        FBE_RDGEN_OPTIONS_DO_NOT_PANIC_ON_DATA_MISCOMPARE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_test_context_run_all_tests(&context, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context.start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(context.start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context.start_io.statistics.io_count, 0);
}
/******************************************
 * end fbe_test_sep_util_run_rdgen_for_pvd_ex()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_run_rdgen_for_pvd()
 ****************************************************************
 * @brief
 *  Run rdgen across a range of blocks on the PVD.
 *
 * @param pvd_object_id
 * @param operation - Rdgen operation to start.
 * @param pattern - Rdgen pattern to use.
 * @param start_lba - Start of range to run rdgen operation.
 * @param blocks - Size of range to run operation on.
 *
 * @return None   
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_util_run_rdgen_for_pvd(fbe_object_id_t pvd_object_id, 
                                         fbe_rdgen_operation_t operation, 
                                         fbe_rdgen_pattern_t pattern,
                                         fbe_lba_t start_lba,
                                         fbe_block_count_t blocks)
{
    return;fbe_test_sep_util_run_rdgen_for_pvd_ex(pvd_object_id, FBE_PACKAGE_ID_SEP_0, operation, pattern, start_lba, blocks);
}
/******************************************
 * end fbe_test_sep_util_run_rdgen_for_pvd()
 ******************************************/
/*!**************************************************************
 * fbe_test_sep_util_run_rdgen_for_pvd_capacity()
 ****************************************************************
 * @brief
 *  Run a given rdgen operation across the entire capacity of the PVD.
 *
 * @param pvd_object_id
 * @param operation - Rdgen operation to start.
 * @param pattern - Rdgen pattern to use.
 *
 * @return None.   
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_sep_util_run_rdgen_for_pvd_capacity(fbe_object_id_t pvd_object_id, 
                                                  fbe_rdgen_operation_t operation, 
                                                  fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;
    fbe_api_provision_drive_info_t provision_drive_info;

    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (provision_drive_info.default_offset >= provision_drive_info.capacity)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "pvd: 0x%x Offset 0x%llx <= capacity 0x%llx",
                   pvd_object_id, (unsigned long long)provision_drive_info.default_offset, (unsigned long long)provision_drive_info.capacity);
        MUT_FAIL_MSG("There is no valid pvd capacity");
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "pvd: 0x%x Offset 0x%llx capacity 0x%llx",
                   pvd_object_id, (unsigned long long)provision_drive_info.default_offset, (unsigned long long)provision_drive_info.capacity);
    }
    fbe_test_sep_util_run_rdgen_for_pvd(pvd_object_id, operation, pattern,
                                        provision_drive_info.default_offset, /* start lba */ 
                                        provision_drive_info.capacity - provision_drive_info.default_offset /* blocks */);
}
/******************************************
 * end fbe_test_sep_util_run_rdgen_for_pvd_capacity()
 ******************************************/

/*!**************************************************************
 * big_bird_get_journal_lba_blocks()
 ****************************************************************
 * @brief
 *   Make sure the journal area has the zeroed pattern, as
 *   it would after having been rebuilt.
 *
 * @param rg_config_p -
 * @param raid_group_count -
 * @param failed_drives - Number of drives to check for rebuild.              
 *
 * @return None.
 *
 * @author
 *  6/25/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_get_journal_lba_blocks(fbe_test_rg_configuration_t * const rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_lba_t *lba_p,
                                     fbe_block_count_t *blocks_p)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    fbe_lba_t min_lba = FBE_LBA_INVALID;
    fbe_lba_t max_lba = 0;

    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        min_lba = FBE_MIN(min_lba, rg_info.write_log_start_pba + rg_info.physical_offset);
        max_lba = FBE_MAX(max_lba, rg_info.write_log_start_pba + rg_info.physical_offset + rg_info.write_log_physical_capacity - 1);
        current_rg_config_p++;
    }
    *lba_p = min_lba;
    *blocks_p = max_lba - min_lba + 1;
}
/******************************************
 * end big_bird_get_journal_lba_blocks()
 ******************************************/
/*!**************************************************************
 * big_bird_send_op_to_all_spares()
 ****************************************************************
 * @brief
 *  Send an operation to the entire capacity of all spares in the system.
 *   
 * @param operation - Rdgen operation to start.
 * @param pattern - Rdgen pattern to use.        
 *
 * @return None.
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_send_op_to_all_spares(fbe_rdgen_operation_t operation, fbe_rdgen_pattern_t pattern,
                                    fbe_lba_t lba, fbe_block_count_t blocks)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_test_block_size_t block_size; 
    fbe_drive_type_t drive_type_index;
    fbe_test_raid_group_disk_set_t      *drive_info_p = NULL;
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_test_discovered_drive_locations_t unused_pvd_locations;
    fbe_object_id_t pvd_object_id;

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);
    fbe_test_sep_util_get_all_unused_pvd_location(&drive_locations, &unused_pvd_locations);

    /* Send I/O to all possible spares to write out a pattern so we will ensure the spares get rebuilt.
     */
    for (block_size = 0; block_size <= FBE_TEST_BLOCK_SIZE_LAST; block_size++)
    {
        for (drive_type_index = 0; drive_type_index < FBE_DRIVE_TYPE_LAST; drive_type_index++)
        {
            for (index = 0; index < unused_pvd_locations.drive_counts[block_size][drive_type_index]; index++)
            {
                drive_info_p = &unused_pvd_locations.disk_set[block_size][drive_type_index][index];

                /* get the object id.of provision drive */
                status = fbe_api_provision_drive_get_obj_id_by_location(drive_info_p->bus,
                                                                        drive_info_p->enclosure,
                                                                        drive_info_p->slot,
                                                                        &pvd_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                fbe_test_sep_util_run_rdgen_for_pvd(pvd_object_id, operation, pattern, lba, blocks);
            }
        }
    }
}
/******************************************
 * end big_bird_send_op_to_all_spares()
 ******************************************/
/*!**************************************************************
 * big_bird_validate_journal_rebuild()
 ****************************************************************
 * @brief
 *   Make sure the journal area has the zeroed pattern, as
 *   it would after having been rebuilt.
 *
 * @param rg_config_p -
 * @param raid_group_count -
 * @param failed_drives - Number of drives to check for rebuild.              
 *
 * @return None.
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_validate_journal_rebuild(fbe_test_rg_configuration_t * const rg_config_p,
                                       fbe_u32_t raid_group_count, 
                                       fbe_u32_t failed_drives)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t index;
    fbe_u32_t disk_index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t pvd_object_id;

    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        for (disk_index = 0; disk_index < failed_drives; disk_index++)
        {
            fbe_u32_t position = current_rg_config_p->drives_removed_history[disk_index];
            status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[position].bus,
                                                                    current_rg_config_p->rg_disk_set[position].enclosure,
                                                                    current_rg_config_p->rg_disk_set[position].slot,
                                                                    &pvd_object_id);
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "Validate journal rg_id: 0x%x pos: %d pvd_id: 0x%x lba: 0x%llx bl: 0x%llx\n",
                       rg_object_id, position, pvd_object_id, (unsigned long long)rg_info.write_log_start_pba + rg_info.physical_offset,
                       (unsigned long long)rg_info.write_log_physical_capacity);
            /* validate that the journal has zeros since this is what the journal rebuild puts down.
             */
            fbe_test_sep_util_run_rdgen_for_pvd(pvd_object_id, FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_ZEROS, 
                                                rg_info.write_log_start_pba + rg_info.physical_offset,
                                                rg_info.write_log_physical_capacity);
        }
        current_rg_config_p++;
    }
}
/******************************************
 * end big_bird_validate_journal_rebuild()
 ******************************************/

/*!**************************************************************
 * big_bird_advanced_rebuild_test()
 ****************************************************************
 * @brief
 *  This test seeds the entire LUN with a known pattern.
 *  It then optionally lays down another pattern (b_write_degraded false).
 *  Then we pull drives and optionally lay down another pattern(b_write_degraded true).
 * 
 *  Then we check that all the patterns are there.
 *  Then kick off a continual read/compare.
 * 
 *  Then we reinsert the drives and wait for them to rebuild.
 * 
 *  Then we stop the continual read/compare.
 *  We kick off one final read/compare of all LUNs.
 *
 * @param rg_config_p - The already created config to test.
 * @param raid_group_count - Number of raid groups to test.
 * @param b_reinsert_same_drive - TRUE if we are doing a rebuild logging
 *                                test where the same drive is pulled/reinserted.
 *                                FALSE if we pull a drive and then
 *                                allow a permanent spare to swap in.
 * @param drives_to_remove - Number of drives per rg to remove.
 * @param peer_options - Options for running I/O on the peer.
 * @param background_pattern - Pattern to put down when the test starts.
 * @param pattern - Optional pattern to overlay over the background pattern.
 *
 * @return None.
 *
 * @author
 *  8/31/2011 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_advanced_rebuild_test(fbe_test_rg_configuration_t * const rg_config_p,
                                    fbe_u32_t raid_group_count,
                                    fbe_bool_t b_reinsert_same_drive,
                                    fbe_u32_t drives_to_remove,
                                    fbe_api_rdgen_peer_options_t peer_options,
                                    fbe_data_pattern_t background_pattern,
                                    fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting == reinsert_same_drive: %s", __FUNCTION__,
               (b_reinsert_same_drive) ? "YES" : "NO");

    /* Since we are typically just using one thread, we can't use load balance, so we will use random instead 
     * so that some traffic occurrs on both sides. 
     */
    if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE)
    {
        peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s write background pattern %d ==", __FUNCTION__, background_pattern);
    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS)
    {
        big_bird_write_zero_background_pattern();
    }
    else
    {
        big_bird_write_background_pattern();
    } 
    mut_printf(MUT_LOG_TEST_STATUS, "== %s write background pattern %d. Complete.==", __FUNCTION__, background_pattern);

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        BIG_BIRD_MAX_IO_SIZE_BLOCKS);

    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               b_reinsert_same_drive,
                               0,    /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Write the foreground regions over the background regions.
     */
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);

    /* Make sure the patterns are still there, both the background and the holes.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s read degraded pattern %d. ==", __FUNCTION__, background_pattern);
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_reinit(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s read degraded pattern %d. Complete. ==", __FUNCTION__, background_pattern);

    /* Change all contexts to continually read.
     */
    for (index = 0; index < num_luns; index++)
    {
        MUT_ASSERT_INT_EQUAL(read_context_p[index].start_io.specification.max_passes, 1);
        fbe_api_rdgen_io_specification_set_max_passes(&read_context_p[index].start_io.specification, 0);
    }

    /* Kick off the continual read, this runs all during the rebuild.
     */
    status = fbe_api_rdgen_start_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               b_reinsert_same_drive);
    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Stop our continual read stream.
     */
    status = fbe_api_rdgen_stop_tests(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    /* Can this fail if no I/O runs before we stop?  */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_reinit(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change all contexts to do one pass.
     * We need to to a single read/check since it is not guaranteed that all tests made it through one pass 
     * before we stopped. 
     */
    for (index = 0; index < num_luns; index++)
    {
        MUT_ASSERT_INT_EQUAL(read_context_p[index].start_io.specification.max_passes, 0);
        fbe_api_rdgen_io_specification_set_max_passes(&read_context_p[index].start_io.specification, 1);
    }

    /* Run our read/check I/O test.  This ensures we run the entire read/check after the rebuild is complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s read non-degraded pattern %d. ==", __FUNCTION__, background_pattern);
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s read non-degraded pattern %d.  Complete.==", __FUNCTION__, background_pattern);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end big_bird_advanced_rebuild_test()
 ******************************************/
/*!**************************************************************
 * big_bird_advanced_full_rebuild_test()
 ****************************************************************
 * @brief
 *  This is a test where we rebuild the entire drive.
 * 
 *  As part of this test we also validate the journal is rebuilt
 *  for parity raid groups only.
 *
 * @param rg_config_p - The already created config to test.
 * @param raid_group_count - Number of raid groups to test.
 * @param drives_to_remove - Number of drives per rg to remove.
 * @param peer_options - Options for running I/O on the peer.
 * @param background_pattern - Pattern to put down when the test starts.
 * @param pattern - Optional pattern to overlay over the background pattern.
 *
 * @return None.
 *
 * @author
 *  6/1/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_advanced_full_rebuild_test(fbe_test_rg_configuration_t * const rg_config_p,
                                         fbe_u32_t raid_group_count,
                                         fbe_u32_t drives_to_remove,
                                         fbe_api_rdgen_peer_options_t peer_options,
                                         fbe_data_pattern_t background_pattern,
                                         fbe_data_pattern_t pattern)
{ 
    /* We can't validate the contents of the journal log when rebuilding two drives. 
     * The reason is because if one drive rebuilds first then it will start rebuild logging to that drive, 
     * thereby changing the contents of the journal log for that drive. 
     */
    if ((rg_config_p->class_id == FBE_CLASS_ID_PARITY) && (drives_to_remove == 1))
    {
        fbe_lba_t lba;
        fbe_block_count_t blocks;
        big_bird_get_journal_lba_blocks(rg_config_p, raid_group_count, &lba, &blocks);
        /* First put down a pattern on the spare drives that will swap in. 
         * Later we will check that this pattern was changed by the rebuild. 
         */
        big_bird_send_op_to_all_spares(FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_RDGEN_PATTERN_LBA_PASS, lba, blocks);
        big_bird_send_op_to_all_spares(FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS, lba, blocks);
    }

    big_bird_advanced_rebuild_test(rg_config_p, raid_group_count,
                                   FBE_FALSE,    /* spare new drive */ drives_to_remove, peer_options,
                                   background_pattern, FBE_DATA_PATTERN_LBA_PASS);

   if ((rg_config_p->class_id == FBE_CLASS_ID_PARITY) && (drives_to_remove == 1))
    {
        /* Refresh the disks since a spare swapped in and there is a new drive in this raid group.
         */
        fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

        /* validate that the journal has zeros since this is what the journal rebuild puts down.
         * We originally put down a seeded pattern on this spare drive so it would only get changed if the journal rebuild 
         * rebuilt zeros. 
         */
        big_bird_validate_journal_rebuild(rg_config_p, raid_group_count, drives_to_remove);
    }
}
/**************************************
 * end big_bird_advanced_full_rebuild_test()
 **************************************/
/*!**************************************************************
 * big_bird_rebuild_read_only_test()
 ****************************************************************
 * @brief
 *  This test seeds the entire LUN with a known pattern,
 *  and then kicks off a read/compare that proceeds while we
 *  pull and insert drives.  
 *
 * @param rg_config_p - The already created config to test.
 * @param raid_group_count - Number of raid groups to test.
 * @param b_reinsert_same_drive - TRUE if we are doing a rebuild logging
 *                                test where the same drive is pulled/reinserted.
 *                                FALSE if we pull a drive and then
 *                                allow a permanent spare to swap in.
 * @param drives_to_remove - Number of drives per rg to remove.
 * @param peer_options - Options for running I/O on the peer.
 * @param background_pattern - Pattern to put down when the test starts.
 * @param pattern - Optional pattern to overlay over the background pattern.
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  8/31/2011 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_rebuild_read_only_test(fbe_test_rg_configuration_t * const rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_bool_t b_reinsert_same_drive,
                                     fbe_u32_t drives_to_remove,
                                     fbe_api_rdgen_peer_options_t peer_options,
                                     fbe_data_pattern_t background_pattern,
                                     fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t num_luns;
    fbe_u32_t background_pass_count = 0;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Since we are typically just using one thread, we can't use load balance, so we will use random instead 
     * so that some traffic occurrs on both sides. 
     */
    if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE)
    {
        peer_options = FBE_API_RDGEN_PEER_OPTIONS_RANDOM;
    }

    /* We use a zero background pattern, and then later we put random other I/Os into this.
     */
    if (background_pattern == FBE_DATA_PATTERN_ZEROS)
    {
        big_bird_write_zero_background_pattern();
    }
    else
    {
        big_bird_write_background_pattern();
    }

    fbe_test_setup_dual_region_contexts(rg_config_p, peer_options, background_pattern, pattern, 
                                        &read_context_p, &write_context_p, background_pass_count,
                                        BIG_BIRD_MAX_IO_SIZE_BLOCKS);

    /* We do the pattern writes now prior to degrading.
     * This gives us a different patterns across the entire range. 
     */
    fbe_test_run_foreground_region_write(rg_config_p, background_pattern, read_context_p, write_context_p);

    /* Make sure the patterns are still there.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_reinit(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change all contexts to continually read.
     */
    for (index = 0; index < num_luns; index++)
    {
        MUT_ASSERT_INT_EQUAL(read_context_p[index].start_io.specification.max_passes, 1);
        fbe_api_rdgen_io_specification_set_max_passes(&read_context_p[index].start_io.specification, 0);
    }

    /* Kick off the continual read, this runs all during the rest of the test while we remove and then reinsert drives.
     */
    status = fbe_api_rdgen_start_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               b_reinsert_same_drive,
                               0,    /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove, b_reinsert_same_drive);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Stop our continual read stream.
     */
    status = fbe_api_rdgen_stop_tests(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    /* Can this fail if no I/O runs before we stop?  */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_reinit(read_context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change all contexts to do one pass.
     * We need to to a single read/check since it is not guaranteed that all tests made it through one pass 
     * before we stopped. 
     */
    for (index = 0; index < num_luns; index++)
    {
        MUT_ASSERT_INT_EQUAL(read_context_p[index].start_io.specification.max_passes, 0);
        fbe_api_rdgen_io_specification_set_max_passes(&read_context_p[index].start_io.specification, 1);
    }

    /* Run our read/check I/O test.  This ensures we run the entire read/check after the rebuild is complete.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(read_context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_context_check_io_status(read_context_p, num_luns, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&read_context_p, num_luns);
    fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&write_context_p, num_luns);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end big_bird_rebuild_read_only_test()
 ******************************************/
/*!**************************************************************
 * big_bird_wait_for_rb_logging()
 ****************************************************************
 * @brief
 *  Wait for a single raid group to start rebuild logging.
 *
 * @param rg_config_p
 * @param num_rebuilds
 * @param target_wait_ms
 * 
 * @return fbe_status_t - FBE_STATUS_OK on success
 *                        FBE_STATUS_TIMEOUT if we take too long.
 *                        Other fbe status for failures.
 *
 * @author
 *  8/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t big_bird_wait_for_rb_logging(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t num_rebuilds,
                                          fbe_u32_t target_wait_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t rb_logging_count;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    /* We arbitrarily decide to wait 2 seconds in between displaying.
     */
    #define BIG_BIRD_DISPLAY_REBUILD_MSEC 2000

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_ms)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                             FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        rb_logging_count = 0;
        /* Physical width for a r10 is the number of actual drives. 
         * For other raid types physical width is the same as rg_info.width. 
         */
        for (index = 0; index < rg_info.physical_width; index++)
        {
            if (rg_info.b_rb_logging[index])
            {
                rb_logging_count++;
            }
        }
        if (rb_logging_count == num_rebuilds)
        {
            return FBE_STATUS_OK;
        }
        if (rb_logging_count > num_rebuilds)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s found %d rb logging positions, expected %d", 
                       __FUNCTION__, rb_logging_count, num_rebuilds);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % BIG_BIRD_DISPLAY_REBUILD_MSEC) == 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "=== %s waiting for rb logging rg: 0x%x ", __FUNCTION__, rg_object_id);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= target_wait_ms)
    {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
/******************************************
 * end big_bird_wait_for_rb_logging()
 ******************************************/

/*!**************************************************************
 * big_bird_wait_for_rgs_rb_logging()
 ****************************************************************
 * @brief
 *  Wait for all raid groups to start rebuild logging for some
 *  number of positions.
 *  
 * @param rg_config_p
 * @param raid_group_count
 * @param num_rebuilds          
 *
 * @return None.
 *
 * @author
 *  8/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_wait_for_rgs_rb_logging(fbe_test_rg_configuration_t * const rg_config_p,
                                      fbe_u32_t raid_group_count,
                                      fbe_u32_t num_rebuilds)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = NULL;
    fbe_u32_t index;

    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = big_bird_wait_for_rb_logging(current_rg_config_p, 
                                              num_rebuilds, FBE_TEST_WAIT_TIMEOUT_MS);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
}
/******************************************
 * end big_bird_wait_for_rgs_rb_logging()
 ******************************************/
/*!**************************************************************
 * big_bird_rebuild_logging_test()
 ****************************************************************
 * @brief
 *  This is a rebuild logging test.
 *  We pull out drives and then re-insert the same drives.
 *
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  3/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_rebuild_logging_test(fbe_test_rg_configuration_t * const rg_config_p,
                                   fbe_u32_t raid_group_count,
                                   fbe_u32_t luns_per_rg,
                                   fbe_element_size_t element_size,
                                   fbe_u32_t drives_to_remove,
                                   fbe_test_random_abort_msec_t ran_abort_msecs,
                                   fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Run I/O degraded.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting heavy I/O. ==", __FUNCTION__);

    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, BIG_BIRD_MAX_IO_SIZE_BLOCKS,
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                      ran_abort_msecs, peer_options);

    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % big_bird_io_msec_short) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, BIG_BIRD_WAIT_IO_COUNT, BIG_BIRD_WAIT_MSEC);

    /* Pull drives.
     */
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Sleep a random time from 1msec to 2 sec.
     * Half of the time we will not do a sleep here. 
     */
    if ((fbe_random() % 2) == 1)
    {
        fbe_api_sleep((fbe_random() % 2000) + 1);
    }
    /* This code tests that the rg get info code does the right things to return 
     * the rebuild logging status for these drive(s). 
     */
    big_bird_wait_for_rgs_rb_logging(rg_config_p, raid_group_count, drives_to_remove);

    /* Stop I/O.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    fbe_test_sep_io_check_status(context_p, num_contexts, ran_abort_msecs);

    /* Start I/O, but not as aggressive so rebuilds will make progress.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting light I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, BIG_BIRD_LIGHT_THREAD_COUNT, BIG_BIRD_SMALL_IO_SIZE_BLOCKS,
                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK, ran_abort_msecs, peer_options);

    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % big_bird_io_msec_short) + EMCPAL_MINIMUM_TIMEOUT_MSECS);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for all rebuilds to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Validate the status of I/Os.
     */
    fbe_test_sep_io_check_status(context_p, num_contexts, ran_abort_msecs);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end big_bird_rebuild_logging_test()
 ******************************************/
/*!**************************************************************
 * big_bird_degraded_full_rebuild_test()
 ****************************************************************
 * @brief
 *  Pull drives with I/O running.
 *  Cause a permanent spare to swap in.
 *  Run I/O during full rebuild.
 *
 * @param element_size - size in blocks of element.    
 * @param rdgen_operation - type of operation to run with rdgen.         
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_degraded_full_rebuild_test(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_u32_t raid_group_count,
                                    fbe_element_size_t element_size,
                                    fbe_rdgen_operation_t rdgen_operation,
                                    fbe_u32_t drives_to_remove,
                                    fbe_test_random_abort_msec_t ran_abort_msecs,
                                    fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Run I/O non-degraded.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting heavy I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, BIG_BIRD_MAX_IO_SIZE_BLOCKS, rdgen_operation,
                      ran_abort_msecs, peer_options);

    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % big_bird_io_msec_short) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, BIG_BIRD_WAIT_IO_COUNT, BIG_BIRD_WAIT_MSEC);

    /* Pull drives.
     */
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_FALSE, /* We are pulling and reinserting a different drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Sleep a random time from 1msec to 5 sec.
     */
    fbe_api_sleep((fbe_random() % big_bird_io_msec_long) + EMCPAL_MINIMUM_TIMEOUT_MSECS);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    fbe_test_sep_io_check_status(context_p, num_contexts, ran_abort_msecs);

    /* Start I/O, but not as aggressive so rebuilds will make progress.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting light I/O. ==", __FUNCTION__);
    big_bird_start_io(rg_config_p, &context_p, BIG_BIRD_LIGHT_THREAD_COUNT, BIG_BIRD_SMALL_IO_SIZE_BLOCKS,
                      rdgen_operation, ran_abort_msecs, peer_options);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_FALSE /* We are pulling and reinserting a different drive*/);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);


    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    fbe_test_sep_io_check_status(context_p, num_contexts, ran_abort_msecs);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end big_bird_degraded_full_rebuild_test()
 ******************************************/

/*!**************************************************************
 * big_bird_rebuild_test()
 ****************************************************************
 * @brief
 *  basic full rebuild test using a single background pattern.
 *  This is currently unused and replaced by the degraded read only test
 *  and the advanced rebuild test.
 *
 * @param element_size - size in blocks of element.   
 * @param rdgen_operation - type of operation to run with rdgen.             
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_rebuild_test(fbe_test_rg_configuration_t *rg_config_p,
                           fbe_u32_t raid_group_count,
                           fbe_element_size_t element_size,
                           fbe_rdgen_operation_t rdgen_operation,
                           fbe_u32_t drives_to_remove,
                           fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &big_bird_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_rdgen_pattern_t pattern;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Pull drives.
     */
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_FALSE, /* We are pulling and reinserting a different drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Next do a test where we write the entire raid group and read it all back after the 
     * rebuild has finished. 
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        big_bird_write_zero_background_pattern();
    }
    else
    {
        big_bird_write_background_pattern();
    }

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_FALSE /* We are pulling and reinserting a different drive*/);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
         if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY, BIG_BIRD_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Make sure the background pattern can be read still.
     */
    pattern = big_bird_get_pattern_for_operation(rdgen_operation);
    big_bird_read_background_pattern(pattern);
    /* Run I/O normal mode again to make sure there are no errors in normal mode I/O (like
     * shed stamp errors. 
     */
    big_bird_start_io(rg_config_p, &context_p, FBE_U32_MAX, BIG_BIRD_MAX_IO_SIZE_BLOCKS, 
                      rdgen_operation, FBE_TEST_RANDOM_ABORT_TIME_INVALID, peer_options);
 
    /* Run I/O briefly.
     * Sleep a random time from 1msec to 2 sec.
     */
    fbe_api_sleep((fbe_random() % 2000) + 1);

    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure there were no errors.
     */
    fbe_test_sep_io_check_status(context_p, num_contexts, FBE_TEST_RANDOM_ABORT_TIME_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/******************************************
 * end big_bird_rebuild_test()
 ******************************************/
/*!**************************************************************
 * big_bird_verify_during_degraded_test()
 ****************************************************************
 * @brief
 *  Pull drives.
 *  Kick off a verify.
 *  Validate verify is set on our chunks.
 *  Re-insert the drives.
 *  Validate that when the rebuild is done, the verify is cleared.
 *
 * @param element_size - size in blocks of element.   
 * @param rdgen_operation - type of operation to run with rdgen.             
 *
 * @return None.
 *
 * @author
 *  3/23/2012 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_verify_during_degraded_test(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t drives_to_remove)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Let system verify complete before we proceed. */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    fbe_test_sep_util_validate_no_rg_verifies(rg_config_p, raid_group_count);

    /* Mark as needing a verify.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate verify on all raid groups. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
         if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_ERROR);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate verify on all raid groups. (successful)==", __FUNCTION__);

    /* Pull drives.
     */
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_FALSE, /* We are pulling and reinserting a different drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);
    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_FALSE /* We are pulling and reinserting a different drive*/);

    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Make sure the verify was cleared.
     */
    fbe_test_sep_util_validate_no_rg_verifies(rg_config_p, raid_group_count);

    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
         if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    return;
}
/******************************************
 * end big_bird_verify_during_degraded_test()
 ******************************************/

/*!**************************************************************
 * big_bird_run_io_with_random_aborts()
 ****************************************************************
 * @brief
 *  This will run I/Os with random abort during rebuild logging
 *  and degraded state
 *
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  3/26/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/
void big_bird_run_io_with_random_aborts(fbe_test_rg_configuration_t * const rg_config_p,
                                        fbe_u32_t raid_group_count,
                                        fbe_u32_t luns_per_rg,
                                        fbe_element_size_t element_size,
                                        fbe_u32_t drives_to_remove,
                                        fbe_test_random_abort_msec_t ran_abort_msecs,
                                        fbe_api_rdgen_peer_options_t peer_options)
{

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Big-bird test with random aborts started for: %d msecs",
               __FUNCTION__, ran_abort_msecs);

    /* Run tests where we remove and reinsert the same drive with I/O running.
     */
    big_bird_rebuild_logging_test(rg_config_p, raid_group_count, luns_per_rg,
                                  element_size, 1    /* drive to remove */,
                                  ran_abort_msecs, peer_options);

    /* Run a test where we remove and insert drives with I/O.
     * The entire drive is rebuilt in this case.
     */
    big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                   element_size, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                   drives_to_remove, ran_abort_msecs,
                                   peer_options);
    big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                   element_size, FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                   drives_to_remove, ran_abort_msecs,
                                   peer_options);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Big-bird test with random aborts completed for: %d msecs",
               __FUNCTION__, ran_abort_msecs);

    return;
}
/**************************************
 * end big_bird_run_io_with_random_aborts()
 **************************************/

/*!**************************************************************
 * big_bird_run_tests_for_config()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param raid_group_count - Total number of raid groups.
 * @param luns_per_rg - Luns to create per rg.          
 * @param element_sizes - Number of element sizes.  
 *
 * @return none
 *
 * @author
 *  5/19/2010 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_run_tests_for_config(fbe_test_rg_configuration_t * rg_config_p, 
                                   fbe_u32_t raid_group_count, 
                                   fbe_u32_t luns_per_rg, 
                                   fbe_u32_t element_sizes)
{
    fbe_element_size_t element_size;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t max_drives_to_remove;
    fbe_u32_t drives_to_remove;
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_bool_t b_zero_cases = fbe_test_sep_util_get_cmd_option("-zero_only");
    fbe_data_pattern_t background_pattern = FBE_DATA_PATTERN_INVALID;

    if (rg_config_p->b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        max_drives_to_remove = 2;
    }
    else
    {
        max_drives_to_remove = 1;
    }

    /* We do not execute the degraded tests if we are only testing shutdown.
     */
    if (fbe_test_sep_util_get_cmd_option("-shutdown_only"))
    {
        max_drives_to_remove =  0;
    }
    for (drives_to_remove = 1; drives_to_remove <= max_drives_to_remove; drives_to_remove++)
    {
        big_bird_verify_during_degraded_test(rg_config_p, raid_group_count, drives_to_remove);
        if (!b_abort_cases)
        {
            if (!b_zero_cases)
            {
                /* Randomize equally between the background patterns of zero and lba/pass.
                 */
                if (fbe_random() % 2)
                {
                    background_pattern = FBE_DATA_PATTERN_LBA_PASS;
                }
                else
                {
                    background_pattern = FBE_DATA_PATTERN_ZEROS;
                }
                
                /* This tests that we can write degraded and read degraded. 
                 * It also tests that we preserve the contents of the data written before we were degraded. 
                 */
                big_bird_advanced_rebuild_test(rg_config_p, raid_group_count,
                                               FBE_TRUE,    /* reinsert same drive */ drives_to_remove, peer_options,
                                               background_pattern, FBE_DATA_PATTERN_LBA_PASS);

                /* This tests that we can successfully read degraded when no writes are performed degraded.
                 */
                big_bird_rebuild_read_only_test(rg_config_p, raid_group_count,
                                                FBE_TRUE,    /* reinsert same drive */ drives_to_remove, peer_options,
                                                background_pattern, FBE_DATA_PATTERN_LBA_PASS);

                /* Run a test where we remove the drive, run some I/O and reinsert the same drive.
                 */
                big_bird_simple_rebuild_logging_test(rg_config_p, raid_group_count, luns_per_rg,
                                                     element_size, drives_to_remove, peer_options);
    
                /* Run tests where we remove and reinsert the same drive with I/O running.
                 */
                big_bird_rebuild_logging_test(rg_config_p, raid_group_count, luns_per_rg,
                                              element_size, 1    /* drive to remove */,
                                              ran_abort_msecs, peer_options);
    
                /* Run a test where we remove and insert drives with I/O.
                 * The entire drive is rebuilt in this case.
                 */
                big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                               element_size, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                               drives_to_remove, ran_abort_msecs, peer_options);
            }
            /* Finally do some zeroing tests while degraded.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "=== Zero tests starting. ===");
            big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                           element_size, FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                           drives_to_remove, ran_abort_msecs, peer_options);
            mut_printf(MUT_LOG_TEST_STATUS, "=== Zero tests completed. ===");
        }
        if (test_level > 0)
        {
            big_bird_run_io_with_random_aborts(rg_config_p, raid_group_count,luns_per_rg,
                                               element_size, drives_to_remove, 
                                               FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC,
                                               peer_options);
        }
    }

    /*! Do shutdown and write log test.
     */
    if (!b_abort_cases && 
        !b_zero_cases)
    {
        /* Write the background pattern to seed this element size.
         */
        big_bird_write_background_pattern();
        /* Run a test where we shut off the raid group with I/O.
         */
        big_bird_degraded_shutdown_test(rg_config_p, raid_group_count, luns_per_rg, peer_options);

        /* For the dualsp test we run an additional test that runs I/O through both SPs.
         */
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            big_bird_degraded_shutdown_test(rg_config_p, raid_group_count, luns_per_rg, 
                                            FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE);
        }

        big_bird_write_background_pattern();
        big_bird_write_log_test_basic(rg_config_p, raid_group_count, luns_per_rg, peer_options);
    }

    for (drives_to_remove = 1; drives_to_remove <= max_drives_to_remove; drives_to_remove++)
    {
        big_bird_advanced_full_rebuild_test(rg_config_p, raid_group_count,
                                            drives_to_remove, peer_options,
                                            background_pattern, FBE_DATA_PATTERN_LBA_PASS);
    }
}
/******************************************
 * end big_bird_run_tests_for_config()
 ******************************************/

/*!**************************************************************
 * big_bird_normal_degraded_io_test()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param drive_to_remove_p - Num drives to remove. 
 *
 * @return none
 *
 * @author
 *  9/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_normal_degraded_io_test(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p)
{
    fbe_u32_t drives_to_remove = (fbe_u32_t)drive_to_remove_p;
    fbe_element_size_t element_size;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_bool_t b_zero_cases = fbe_test_sep_util_get_cmd_option("-zero_only");
    fbe_data_pattern_t background_pattern = FBE_DATA_PATTERN_INVALID;
    fbe_u32_t luns_per_rg = BIG_BIRD_TEST_LUNS_PER_RAID_GROUP;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    if (rg_config_p->b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();

    big_bird_verify_during_degraded_test(rg_config_p, raid_group_count, drives_to_remove);
    if (!b_abort_cases)
    {
        if (!b_zero_cases)
        {
            /* Randomize equally between the background patterns of zero and lba/pass.
             */
            if (fbe_random() % 2)
            {
                background_pattern = FBE_DATA_PATTERN_LBA_PASS;
            }
            else
            {
                background_pattern = FBE_DATA_PATTERN_ZEROS;
            }

            /* This tests that we can write degraded and read degraded. 
             * It also tests that we preserve the contents of the data written before we were degraded. 
             */
            big_bird_advanced_rebuild_test(rg_config_p, raid_group_count,
                                           FBE_TRUE,    /* reinsert same drive */ drives_to_remove, peer_options,
                                           background_pattern, FBE_DATA_PATTERN_LBA_PASS);

            /* This tests that we can successfully read degraded when no writes are performed degraded.
             */
            big_bird_rebuild_read_only_test(rg_config_p, raid_group_count,
                                            FBE_TRUE,    /* reinsert same drive */ drives_to_remove, peer_options,
                                            background_pattern, FBE_DATA_PATTERN_LBA_PASS);

            /* Run a test where we remove the drive, run some I/O and reinsert the same drive.
             */
            big_bird_simple_rebuild_logging_test(rg_config_p, raid_group_count, luns_per_rg,
                                                 element_size, drives_to_remove, peer_options);

            /* Run tests where we remove and reinsert the same drive with I/O running.
             */
            big_bird_rebuild_logging_test(rg_config_p, raid_group_count, luns_per_rg,
                                          element_size, 1    /* drive to remove */,
                                          ran_abort_msecs, peer_options);

            /* Run a test where we remove and insert drives with I/O.
             * The entire drive is rebuilt in this case.
             */
            big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                                element_size, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                drives_to_remove, ran_abort_msecs, peer_options);
        }
    }
    big_bird_advanced_full_rebuild_test(rg_config_p, raid_group_count,
                                        drives_to_remove, peer_options,
                                        background_pattern, FBE_DATA_PATTERN_LBA_PASS);

	if (fbe_test_sep_util_get_dualsp_test_mode()){
		fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);
		fbe_test_rg_configuration_t *current_rg_config_p = NULL;
		fbe_u32_t index;
		fbe_object_id_t rg_object_id;
	    fbe_transport_connection_target_t   target_invoked_on = FBE_TRANSPORT_SP_A;

		target_invoked_on = fbe_api_transport_get_target_server();

		fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
		fbe_api_base_config_disable_all_bg_services();
		fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
		fbe_api_base_config_disable_all_bg_services();		

		current_rg_config_p = rg_config_p;
		for ( index = 0; index < raid_group_count; index++) {
			if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
				current_rg_config_p++;
				continue;
			}
			fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
			fbe_test_sep_util_check_stripe_lock(rg_object_id);
			current_rg_config_p++;
		}

		fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_A);
		fbe_api_base_config_enable_all_bg_services();
		fbe_api_transport_set_target_server(FBE_TRANSPORT_SP_B);
		fbe_api_base_config_enable_all_bg_services();

		fbe_api_transport_set_target_server(target_invoked_on);
	}
}
/******************************************
 * end big_bird_normal_degraded_io_test()
 ******************************************/
/*!*******************************************************************
 * @def BIG_BIRD_ENCRYPTION_HALT_LBA
 *********************************************************************
 * @brief  Halt part way through the 2nd LUN.
 *
 *********************************************************************/
#define BIG_BIRD_ENCRYPTION_HALT_LBA FBE_RAID_DEFAULT_CHUNK_SIZE * (BIG_BIRD_CHUNKS_PER_LUN + (BIG_BIRD_CHUNKS_PER_LUN/2))
void big_bird_start_encryption(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, BIG_BIRD_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_ADD);
    
    // Add hook for encryption to complete and enable KMS
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, BIG_BIRD_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_WAIT);
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting Encryption. ==");
    //fbe_test_sep_util_start_rg_encryption(rg_config_p);

}

void big_bird_complete_encryption(fbe_test_rg_configuration_t *rg_config_p)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hooks so encryption can proceed. ==");

    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, BIG_BIRD_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_WAIT);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, BIG_BIRD_ENCRYPTION_HALT_LBA, FBE_TEST_HOOK_ACTION_DELETE);
    
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Encryption. ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE); /* KMS enabled */

}
/*!**************************************************************
 * big_bird_normal_degraded_encryption_test()
 ****************************************************************
 * @brief
 *  Run a degraded encryption test.
 *
 * @param rg_config_p - Config to create.
 * @param drive_to_remove_p - Num drives to remove. 
 *
 * @return none
 *
 * @author
 *  9/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_normal_degraded_encryption_test(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p)
{
    fbe_u32_t drives_to_remove = (fbe_u32_t)drive_to_remove_p;
    fbe_element_size_t element_size;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_data_pattern_t background_pattern = FBE_DATA_PATTERN_INVALID;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    if (rg_config_p->b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();
    fbe_test_rg_wait_for_zeroing(rg_config_p);

    //status = fbe_api_enable_system_encryption();
    //mut_printf(MUT_LOG_TEST_STATUS, "Encryption enable status %d", status);
    //MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Enabling of encryption failed\n");

    /* Randomize equally between the background patterns of zero and lba/pass.
     */
    if (fbe_random() % 2) {
        background_pattern = FBE_DATA_PATTERN_LBA_PASS;
    } else {
        background_pattern = FBE_DATA_PATTERN_ZEROS;
    }

    big_bird_start_encryption(rg_config_p);
    /* Run a test where we remove and insert drives with I/O.
     * The entire drive is rebuilt in this case.
     */
    big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                        element_size, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                        drives_to_remove, ran_abort_msecs, peer_options);
    big_bird_complete_encryption(rg_config_p);
#if 0
    big_bird_start_encryption(rg_config_p);
    big_bird_advanced_full_rebuild_test(rg_config_p, raid_group_count,
                                        drives_to_remove, peer_options,
                                        background_pattern, FBE_DATA_PATTERN_LBA_PASS);
    big_bird_complete_encryption(rg_config_p);
#endif
}
/******************************************
 * end big_bird_normal_degraded_encryption_test()
 ******************************************/
/*!**************************************************************
 * big_bird_zero_and_abort_io_test()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param drive_to_remove_p - Num drives to remove. 
 *
 * @return none
 *
 * @author
 *  9/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_zero_and_abort_io_test(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p)
{
    fbe_u32_t drives_to_remove = (fbe_u32_t)drive_to_remove_p;
    fbe_element_size_t element_size;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_bool_t b_zero_cases = fbe_test_sep_util_get_cmd_option("-zero_only");
    fbe_u32_t luns_per_rg = BIG_BIRD_TEST_LUNS_PER_RAID_GROUP;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    /* Run a test where we control background operations.
     */
    big_bird_bg_operation_control_test();

    /* We need to wait for initial verify complete so that raid group will not go into
     * quiesced mode to update verify checkpoint.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* Run a quiesce test.
     */
    big_bird_quiesce_test(rg_config_p, raid_group_count, 1);

    if (rg_config_p->b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();

    if (!b_abort_cases)
    {
        /* Finally do some zeroing tests while degraded.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "=== Zero tests starting. ===");
        big_bird_degraded_full_rebuild_test(rg_config_p, raid_group_count,
                                            element_size, FBE_RDGEN_OPERATION_ZERO_READ_CHECK, 
                                            drives_to_remove, ran_abort_msecs, peer_options);
        mut_printf(MUT_LOG_TEST_STATUS, "=== Zero tests completed. ===");
    }

    if (test_level > 0 && !b_zero_cases)
    {
        big_bird_run_io_with_random_aborts(rg_config_p, raid_group_count,luns_per_rg,
                                           element_size, drives_to_remove, 
                                           FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC,
                                           peer_options);
    }
}
/******************************************
 * end big_bird_zero_and_abort_io_test()
 ******************************************/
/*!**************************************************************
 * big_bird_run_shutdown_test()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param unused_context_p - 
 *
 * @return none
 *
 * @author
 *  9/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_run_shutdown_test(fbe_test_rg_configuration_t *rg_config_p, void * unused_context_p)
{
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_u32_t luns_per_rg = BIG_BIRD_TEST_LUNS_PER_RAID_GROUP;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();
    /* Run a test where we shut off the raid group with I/O.
     */
    big_bird_degraded_shutdown_test(rg_config_p, raid_group_count, luns_per_rg, peer_options);

    /* For the dualsp test we run an additional test that runs I/O through both SPs.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        big_bird_degraded_shutdown_test(rg_config_p, raid_group_count, luns_per_rg, 
                                        FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE);
    }

    big_bird_write_background_pattern();
    big_bird_write_log_test_basic(rg_config_p, raid_group_count, luns_per_rg, peer_options);
}
/******************************************
 * end big_bird_run_shutdown_test()
 ******************************************/
/*!**************************************************************
 * big_bird_run_tests()
 ****************************************************************
 * @brief
 *  We create a config and run the rebuild tests.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param luns_per_rg - The number of luns in each raid group.
 * @param chunks_per_lun - The number of chunks each lun should consume.
 * @param b_quiesce_test - True if we should run a quiesce test.               
 *
 * @return None.
 *
 * @author
 *  7/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t  raid_group_count = 0;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    /* Run a test where we control background operations.
     */
    big_bird_bg_operation_control_test();

    /* We do not execute the quiesce tests if we are only testing shutdown.
     */
    if (!fbe_test_sep_util_get_cmd_option("-shutdown_only") &&
        !fbe_test_sep_util_get_cmd_option("-abort_cases_only") &&
        !fbe_test_sep_util_get_cmd_option("-zero_only"))
    {
        fbe_u32_t iterations = BIG_BIRD_QUIESCE_TEST_ITERATIONS;

        /* For test level 0, we just run the quiesce test a default number of iterations 
         * (BIG_BIRD_QUIESCE_TEST_ITERATIONS). 
         * For other test levels up to 50 we will use the test level as the # of iterations. 
         */
        if ((test_level != 0) &&
            (test_level <= BIG_BIRD_QUIESCE_TEST_MAX_ITERATIONS))
        {
            /* We will allow a max number of iterations of 50.
             * Test level is our default number of iterations. 
             */
            iterations = test_level;
        }

        /* Run a quiesce test.
         */
        big_bird_quiesce_test(rg_config_p, raid_group_count, iterations);
    }

    /* Now that the config is created, run the main portion of the test.
     */
   // big_bird_normal_degraded_io_test(rg_config_p, 1 /* drives to remove*/);
    //big_bird_shutdown_test(rg_config_p, NULL);
    //big_bird_run_tests_for_config(rg_config_p, raid_group_count, luns_per_rg, 1);
    return;
}
/**************************************
 * end big_bird_run_tests()
 **************************************/
/*!**************************************************************
 * big_bird_single_degraded_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_single_degraded_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &big_bird_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &big_bird_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1,/* drives to remove */
                                                   big_bird_normal_degraded_io_test,
                                                   BIG_BIRD_TEST_LUNS_PER_RAID_GROUP,
                                                   BIG_BIRD_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end big_bird_test_single_degraded_test()
 ******************************************/

/*!**************************************************************
 * big_bird_single_degraded_zero_abort_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_single_degraded_zero_abort_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &big_bird_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &big_bird_raid_group_config_qual[0];

    }
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1, /* drives to remove */
                                                   big_bird_zero_and_abort_io_test,
                                                   BIG_BIRD_TEST_LUNS_PER_RAID_GROUP,
                                                   BIG_BIRD_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end big_bird_single_degraded_zero_abort_test()
 ******************************************/

/*!**************************************************************
 * big_bird_shutdown_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_shutdown_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &big_bird_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &big_bird_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, big_bird_run_shutdown_test,
                                                   BIG_BIRD_TEST_LUNS_PER_RAID_GROUP,
                                                   BIG_BIRD_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end big_bird_shutdown_test()
 ******************************************/

/*!**************************************************************
 * big_bird_single_degraded_encryption_test()
 ****************************************************************
 * @brief
 *  Run degraded I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_single_degraded_encryption_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &big_bird_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &big_bird_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1,/* drives to remove */
                                                   big_bird_normal_degraded_encryption_test,
                                                   BIG_BIRD_TEST_LUNS_PER_RAID_GROUP,
                                                   BIG_BIRD_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end big_bird_test_single_degraded_test()
 ******************************************/
/*!**************************************************************
 * big_bird_setup()
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
void big_bird_setup(void)
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
            rg_config_p = &big_bird_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &big_bird_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        elmo_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare
     * Set the spare trigger timer 1 second to swap in immediately.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();

    big_bird_set_io_seconds();
    
    return;
}
/**************************************
 * end big_bird_setup()
 **************************************/

/*!**************************************************************
 * big_bird_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the big_bird test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            fbe_test_sep_util_destroy_kms();
        }
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    fbe_test_sep_util_set_encryption_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end big_bird_cleanup()
 ******************************************/

void big_bird_dualsp_single_degraded_test(void)
{
    big_bird_single_degraded_test();
}
void big_bird_dualsp_single_degraded_zero_abort_test(void)
{
    big_bird_single_degraded_zero_abort_test();
}
void big_bird_dualsp_shutdown_test(void)
{
    big_bird_shutdown_test();
    return;
}

void big_bird_dualsp_single_degraded_encryption_test(void)
{
    big_bird_single_degraded_encryption_test();
}
/*!**************************************************************
 * big_bird_dualsp_setup()
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
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_dualsp_setup(void)
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
            rg_config_p = &big_bird_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &big_bird_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        sep_config_load_sep_and_neit_load_balance_both_sps();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms_both_sps(NULL);
        }
    }

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare
     * Set the spare trigger timer 1 second to swap in immediately.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();
    
    big_bird_set_io_seconds();
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    return;
}
/******************************************
 * end big_bird_dualsp_setup()
 ******************************************/
void big_bird_dualsp_encryption_setup(void)
{
    fbe_status_t status;
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    big_bird_dualsp_setup();
    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
void big_bird_encryption_setup(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    big_bird_setup();
}
/*!**************************************************************
 * big_bird_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the big_bird test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/22/2011 - Created. Rob Foley
 *
 ****************************************************************/

void big_bird_dualsp_cleanup(void)
{
    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        if (fbe_test_sep_util_get_encryption_test_mode()){
            fbe_test_sep_util_destroy_kms_both_sps();
        }
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    fbe_test_sep_util_set_encryption_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end big_bird_dualsp_cleanup()
 ******************************************/
/*************************
 * end file big_bird_test.c
 *************************/


