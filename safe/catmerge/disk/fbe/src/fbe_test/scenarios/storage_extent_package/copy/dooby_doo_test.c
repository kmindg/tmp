/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * scooby_doo_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains proactive copy with I/O to non-degraded raid groups.
 *
 * HISTORY
 *  03/22/2012  Ron Proulx - Created.
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
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "sep_event.h"
#include "sep_rebuild_utils.h"
#include "sep_test_io.h"
#include "sep_test_background_ops.h"
#include "sep_test_region_io.h"
#include "sep_hook.h"

/*************************
 *   LOCAL FUNCTION DEFINITIONS
 *************************/

char * dooby_doo_short_desc = "Test Copy Operations with I/O on non-degraded raid groups.";
char * dooby_doo_long_desc = 
    "\n"
"This test verifies a Copy from the Source(Candidate) drive to the Destination(Spare) drive. \n"
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: Run a get-info test for the raid groups.\n\
        - The purpose of this test is to validate the functionality of the\n\
          get-info usurper.  This usurper is used when we create a raid group to\n\
          validate the parameters of raid group creation such as width, element size, etc.\n\
        - Issue get info usurper to the striper class and validate various\n\
          test cases including normal and error cases.\n\
\n\
STEP 2: configure all raid groups and LUNs.\n\
        - All raid groups have 3 LUNs each\n\
\n\
STEP 3: write a background pattern to all luns and then read it back and make sure it was written successfully\n\
\n\
STEP 4: Peform the standard I/O test sequence.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n\
STEP 5: Peform the standard I/O test sequence above with aborts.\n\
\n\
STEP 6: Peform the caterpillar I/O sequence.\n\
        - Caterpillar is multi-threaded sequential I/O that causes\n\
          stripe lock contention at the raid group.\n\
        - Perform write/read/compare for small I/os.\n\
        - Perform verify-write/read/compare for small I/Os.\n\
        - Perform verify-write/read/compare for large I/Os.\n\
        - Perform write/read/compare for large I/Os.\n\
\n"\
"STEP 7: Run the block operation usurper test.\n\
        - Issues single small I/Os for every LUN and every operation type.\n\
          - Operations tested on the LUN include:\n\
            - write, corrupt data, corrupt crc, verify, zero.\n\
          - Operations tested on the raid group include:\n\
            - verify, zero.\n\
\n\
STEP 8: Run the block operation usurper test (above) with large I/Os.\n\
\n"\
"STEP 9: Destroy raid groups and LUNs.\n\
\n\
Test Specific Switches:\n\
          -abort_cases_only - Only run the abort tests.\n\
\n\
Description last updated: 03/22/2012.\n";

/*!*******************************************************************
 * @def DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP  4

/*!*******************************************************************
 * @def DOOBY_DOO_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 * @note  This needs to be fairly large so that we test I/O with copy.
 *********************************************************************/
#define DOOBY_DOO_CHUNKS_PER_LUN            5

/*!*******************************************************************
 * @def DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS DOOBY_DOO_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def DOOBY_DOO_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define DOOBY_DOO_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def DOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types.
 *
 *********************************************************************/
#define DOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES 3

/*!*******************************************************************
 * @def DOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types
 *
 *********************************************************************/
#define DOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES     1


/*!*******************************************************************
 * @def DOOBY_DOO_WAIT_OBJECT_TIMEOUT_MS
 *********************************************************************
 * @brief Number of milliseconds we wait for an object.
 *        We make this relatively large since if it takes this long
 *        then we know something is very wrong.
 *
 *********************************************************************/
#define DOOBY_DOO_WAIT_OBJECT_TIMEOUT_MS 60000

/*!*******************************************************************
 * @def     DOOBY_DOO_WAIT_FOR_REBUILD_SECS
 *********************************************************************
 * @brief   How many seconds to wait for a single raid group to complete
 *          a rebuild.
 *
 *********************************************************************/
#define DOOBY_DOO_WAIT_FOR_REBUILD_SECS             60

/*!*******************************************************************
 * @def     DOOBY_DOO_GLITCH_DELAY_MS
 *********************************************************************
 * @brief   How many milli-seconds to wait for the `glitch delay'.  The
 *          glitch delay is how long raid waits for a drive to come back
 *          before declaring it gone and marking rl.
 *
 *********************************************************************/
#define DOOBY_DOO_GLITCH_DELAY_MS                   (10 * 1000)

/*!*******************************************************************
 * @var dooby_doo_threads
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t dooby_doo_threads = 5;

/*!*******************************************************************
 * @def DOOBY_DOO_MAX_RAID_GROUPS_PER_TEST
 *********************************************************************
 * @brief   Maximum number of raid groups under test at a time.
 *
 *********************************************************************/
#define DOOBY_DOO_MAX_RAID_GROUPS_PER_TEST 5

/*!*******************************************************************
 * @var dooby_doo_io_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t dooby_doo_io_contexts[DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP * DOOBY_DOO_MAX_RAID_GROUPS_PER_TEST];

/*!*******************************************************************
 * @var dooby_doo_io_msec_short
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `short' period of time.
 *
 *********************************************************************/
#define DOOBY_DOO_SHORT_IO_TIME_SECS  2
static fbe_u32_t dooby_doo_io_msec_short = (DOOBY_DOO_SHORT_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var dooby_doo_io_msec_long
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `long' period of time.
 *
 *********************************************************************/
#define DOOBY_DOO_LONG_IO_TIME_SECS  10
static fbe_u32_t dooby_doo_io_msec_long = (DOOBY_DOO_LONG_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var     dooby_doo_pause_params
 *********************************************************************
 * @brief This variable the debug hook locations required for any
 *        test that require a `pause'.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t dooby_doo_pause_params = { 0 };

/*!*******************************************************************
 * @var     dooby_doo_debug_hooks
 *********************************************************************
 * @brief This variable the debug hook locations required for any
 *        test that require a `pause'.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t dooby_doo_debug_hooks = { 0 };

/*!*******************************************************************
 * @var     dooby_doo_dest_array
 *********************************************************************
 * @brief   Arrays of destination positions to spare (for user copy).
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t dooby_doo_dest_array[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = { 0 };

/*!*******************************************************************
 * @var dooby_doo_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifndef __SAFE__
fbe_test_rg_configuration_array_t dooby_doo_raid_groups_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t dooby_doo_raid_groups_extended[] =
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */ 
{

    /* RAID-1 and RAID-10 configuraitons
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, raid group id,  bandwidth */
        {2,         0x20000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        0,              0},
        {3,         0x20000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        1,              1},
        {8,         0x20000,    FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,   520,        2,              0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Parity raid group type configurations.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {16,        0x20000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,        0,          0},
        {6,         0x20000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,        1,          1},
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,        2,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, },
    },
    /* RAID-0 configurations (should not be supported !).
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth. */
        {1,       0x20000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,              0},
        {3,       0x20000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,              1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var dooby_doo_raid_groups_qual
 *********************************************************************
 * @brief Test config for raid test level 0 (default test level).
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifndef __SAFE__
fbe_test_rg_configuration_array_t dooby_doo_raid_groups_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t dooby_doo_raid_groups_qual[] = 
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */ 
{
    /* All raid group configurations for qual
     */
    {
        /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
        {6,         0x20000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        0,          0},
        {5,         0x20000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        1,          1},
        {5,         0x20000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        2,          0},
        {2,         0x20000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR, 520,        3,          0},
        {4,         0x20000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,520,        4,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/************************ 
 * EXTERNAL FUNCTIONS
 ************************/
extern void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t  vd_object_id,
                                                                fbe_edge_index_t * source_edge_index_p,
                                                                fbe_edge_index_t * dest_edge_index_p);

/*!**************************************************************
 * dooby_doo_set_io_seconds()
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
static void dooby_doo_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();

    if (long_io_time_seconds >= DOOBY_DOO_LONG_IO_TIME_SECS)
    {
        dooby_doo_io_msec_long = (long_io_time_seconds * 1000);
    }
    else
    {
        dooby_doo_io_msec_long  = ((fbe_random() % DOOBY_DOO_LONG_IO_TIME_SECS) + 1) * 1000;
    }
    dooby_doo_io_msec_short = ((fbe_random() % DOOBY_DOO_SHORT_IO_TIME_SECS) + 1) * 1000;
    return;
}
/******************************************
 * end dooby_doo_set_io_seconds()
 ******************************************/

/*!***************************************************************************
 *          dooby_doo_set_source_drive_removed_hook()
 *****************************************************************************
 *
 * @brief   Set the requested debug hooks for the associated virtual drive
 *          objects.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 *
 * @return  fbe_status_t 
 *
 * @author
 *  07/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_set_source_drive_removed_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_u32_t position_to_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;

    /* Step 1: Locate the associated virtual drive and set the debug hook.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Set the requested debug hook.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id, 
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************
 * end dooby_doo_set_source_drive_removed_hook()
 *********************************************************/

/*!***************************************************************************
 *          dooby_doo_set_source_drive_glitched_hook()
 *****************************************************************************
 *
 * @brief   Set the requested debug hooks for the associated virtual drive
 *          objects.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 *
 * @return  fbe_status_t 
 *
 * @author
 *  07/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_set_source_drive_glitched_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_u32_t position_to_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;

    /* Step 1: Locate the associated virtual drive and set the debug hook.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Set the requested debug hook.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id, 
                                                SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************
 * end dooby_doo_set_source_drive_glitched_hook()
 *********************************************************/

/*!***************************************************************************
 *          dooby_doo_handle_source_drive_glitch_or_fail()
 *****************************************************************************
 *
 * @brief   This test `glitches' the source drive of a proactive copy by
 *          clearing EOL.  Unfortunately (due to scheduling) some source will
 *          take more than the `glitch delay' (5 seconds) for the virtual drive
 *          downstream edge to change from disabled to enabled.  Thus the
 *          virtual drive goes broken and the parent raid group see the edge
 *          goto broken.  But in the majority of cases the path state goes
 *          to enabled in less than the `glitch' delay.  This routine handles
 *          either case on a per raid group basis.
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The position in the raid group(s) to copy
 * @param   rdgen_operation - If this is not a read only and the virtual drive
 *              went broken, then the parent raid group will be marked NR.
 *
 * @return  status
 *
 * @author
 *  03/04/2014  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_handle_source_drive_glitch_or_fail(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_scheduler_debug_hook_t      hook;

    /* Step 1: Wait for the `glitch delay' period.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait `glitch delay' ==", __FUNCTION__);
    fbe_api_sleep(DOOBY_DOO_GLITCH_DELAY_MS);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait `glitch delay' - complete ==", __FUNCTION__);

    /* Walk thru all the raid groups and determine of the source drive `glitched'
     * or the virtual drive went broken.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Step 2: Determine if the virtual drive went broken or not.
         */
        status = fbe_test_get_debug_hook_active(vd_object_id, 
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE,
                                                0, 0,
                                                &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (hook.counter > 0) {
            /* Step 3: Virtual drive went broken.  Remove the broken hook and
             *         validate that no chunks were marked NR in the parent
             *         raid group.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s vd obj: 0x%x went broken validate rg obj: 0x%x is degraded ==", 
                       __FUNCTION__, vd_object_id, rg_object_id);

            /* Wait for the parent raid group to go degraded
             */
            status = fbe_test_sep_util_wait_for_degraded_bitmask(rg_object_id, (1 << position_to_copy), 
                                                                 FBE_TEST_WAIT_TIMEOUT_MS);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Remove the virtual drive failed hook.
             */
            status = fbe_test_del_debug_hook_active(vd_object_id,
                                                    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                                    FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                                    0, 0, 
                                                    SCHEDULER_CHECK_STATES, 
                                                    SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Wait for the eval mark NR hook in the virtual drive.
             */
            status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                         FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                         SCHEDULER_CHECK_STATES,
                                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                                         0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Validate that no chunks are marked on the source and that there
             * are chunks marked on the destination.
             */
            fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, source_edge_index);
            fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

            /* Remove the virtual drive eval mark NR hook.
             */
            status = fbe_test_del_debug_hook_active(vd_object_id,
                                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE, 
                                                    0, 0, 
                                                    SCHEDULER_CHECK_STATES, 
                                                    SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Wait for the parent raid group to hit the eval mark NR done hook.
             */
            fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook(current_rg_config_p, 1);

            /* If the I/O was only reads then validate that the parent didn't mark NR.
             * If the I/O was writes then the parent will be marked and we cannot validate.
             */
            if ((rdgen_operation == FBE_RDGEN_OPERATION_READ_ONLY)  ||
                (rdgen_operation == FBE_RDGEN_OPERATION_READ_CHECK)    ) {
                fbe_test_sep_rebuild_validate_raid_group_not_marked_nr(current_rg_config_p, 1);
            }

            /* Remove the eval mark NR done hook.
             */
            fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook(current_rg_config_p, 1);
        } else {
            /* Step 3: Virtual drive never went broken (it `glitched).  Wait
             *         for the eval mark NR done hook in the virtual drive.
             *         Validate that no chunks are marked for the source drive
             *         and that there are chunks marked for the destination drive.
             *         Remove all the hooks.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s vd obj: 0x%x glitched validate rg obj: 0x%x is not degraded ==", 
                       __FUNCTION__, vd_object_id, rg_object_id);

            /* Wait for the eval mark NR hook in the virtual drive.
             */
            status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR, 
                                                         FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE,
                                                         SCHEDULER_CHECK_STATES,
                                                         SCHEDULER_DEBUG_ACTION_PAUSE,
                                                         0, 0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Validate parent raid group is not degraded.
             */
            status = fbe_test_sep_validate_raid_groups_are_not_degraded(current_rg_config_p, 1);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Validate that there are no chunks marked in the parent.
             */
            fbe_test_sep_rebuild_validate_raid_group_not_marked_nr(current_rg_config_p, 1);

            /* Validate that no chunks are marked on the source and that there
             * are chunks marked on the destination.
             */
            fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, source_edge_index);
            fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

            /* Remove the virtual drive eval mark NR hook.
             */
            status = fbe_test_del_debug_hook_active(vd_object_id,
                                                    SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                    FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE, 
                                                    0, 0, 
                                                    SCHEDULER_CHECK_STATES, 
                                                    SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Remove the virtual drive failed hook.
             */
            status = fbe_test_del_debug_hook_active(vd_object_id,
                                                    SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                                    FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                                    0, 0, 
                                                    SCHEDULER_CHECK_STATES, 
                                                    SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Remove the eval mark NR done hook.
             */
            fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook(current_rg_config_p, 1);

        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return status
     */
    return status;
}
/***************************************************** 
 * end dooby_doo_handle_source_drive_glitch_or_fail()
 ****************************************************/

/*!***************************************************************************
 *          dooby_doo_validate_copy_doesnt_run_on_raid0()
 *****************************************************************************
 *
 * @brief   Validate that proactive copy doesn't start on RAID-0 type raid
 *          groups.
 *
 * @param   rg_config_p - config to run test against. 
 *
 * @return  status
 *
 * @note    This test only runs against the first raid group in the array
 *          There is not point in running against RAID-0 of different widths.
 *
 * @author
 *  10/23/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_validate_copy_doesnt_run_on_raid0(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       position_to_copy = 0;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_event_found = FBE_FALSE;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_u32_t                       retry_count = 0;

    /* Step 1: Set EOL and validate that proactive copy doesn't run
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Attempt proactive copy for rg obj: 0x%x pos: %d ==", 
               __FUNCTION__, rg_object_id, position_to_copy);

    /* Get the provision object to set debug hook, etc. 
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position_to_copy].bus,
                                                            rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                            rg_config_p->rg_disk_set[position_to_copy].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Here we are triggering proactive sparing by setting end of life state 
     * for the given drive.
     */
    status = fbe_api_provision_drive_set_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s EOL set on pvd obj: 0x%x (%d_%d_%d) ==", 
               __FUNCTION__, pvd_object_id,
               rg_config_p->rg_disk_set[position_to_copy].bus,
               rg_config_p->rg_disk_set[position_to_copy].enclosure,
               rg_config_p->rg_disk_set[position_to_copy].slot);

    /* Get the provision drive info and validate EOL is set.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while ((provision_drive_info.end_of_life_state != FBE_TRUE) &&
           (retry_count < 100)                                      )
    {
        fbe_api_sleep(200);
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        retry_count++;
    }
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, provision_drive_info.end_of_life_state);
    retry_count = 0;

    /* Validate that the proper event was generated.
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         30000 /* Wait up to 30 seconds for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Now clear EOL.
     */
    status = fbe_api_provision_drive_clear_eol_state(pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the provision drive info and validate EOL is clear
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while ((provision_drive_info.end_of_life_state != FBE_FALSE) &&
           (retry_count < 100)                                      )
    {
        fbe_api_sleep(200);
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        retry_count++;
    }
    MUT_ASSERT_INT_EQUAL(FBE_FALSE, provision_drive_info.end_of_life_state);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Attempt proactive copy - failed as expected==", 
               __FUNCTION__);

    /* Step 2: Attempt a user copy. First wait for source drive to be Ready
     *         since clearing EOL causes the provision drive to enter the
     *         activate state.
     */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          vd_object_id, 
                                                                          FBE_LIFECYCLE_STATE_READY, 
                                                                          DOOBY_DOO_WAIT_OBJECT_TIMEOUT_MS, 
                                                                          FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Attempt user copy for rg id: %d pos: %d ==", 
               __FUNCTION__, rg_config_p->raid_group_id, position_to_copy);
    status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_NON_REDUNDANT_RAID_GROUP);

    /* Validate that the proper event was generated.
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         30000 /* Wait up to 30 seconds for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Attempt user copy - failed as expected==", 
               __FUNCTION__);

    /* Return the execution status.
     */
    return status;
}
/**************************************************************
 * end dooby_doo_validate_copy_doesnt_run_on_raid0()
 *************************************************************/

/*!***************************************************************************
 *          dooby_doo_start_io()
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
 * @param       rdgen_pattern - The expected background pattern
 * @param       threads - How many I/O threads to run
 * @param       io_size_blocks - The maximum I/O size to use  
 * @param       ran_abort_msecs - If enabled the abort mode     
 * @param       requested_dualsp_mode - If dualsp mode is enable the load
 *                  balancing mode (balanced, peer only, etc)
 *
 * @return      status
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_start_io
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_start_io(fbe_rdgen_operation_t rdgen_operation,
                                       fbe_rdgen_pattern_t rdgen_pattern,
                                       fbe_u32_t threads,
                                       fbe_u32_t io_size_blocks,
                                       fbe_test_random_abort_msec_t ran_abort_msecs,
                                       fbe_test_sep_io_dual_sp_mode_t requested_dualsp_mode)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &dooby_doo_io_contexts[0];
    fbe_bool_t                  b_ran_aborts_enabled;
    fbe_bool_t                  b_dualsp_mode;

    mut_printf(MUT_LOG_TEST_STATUS, 
           "== %s Start I/O ==", 
           __FUNCTION__);

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
                                           b_dualsp_mode  /* If dualsp mode  */);

    /* Set the data pattern
     */
    status = fbe_api_rdgen_io_specification_set_pattern(&context_p->start_io.specification, rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

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

    /* Run our I/O test.  Since we are running against all LUs,
     * there is only (1) rdgen context.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return status;
}
/******************************************
 * end dooby_doo_start_io()
 ******************************************/

/*!**************************************************************
 * dooby_doo_stop_io()
 ****************************************************************
 * @brief   Stop the previously started I/O and validate no error
 *
 * @param None.               
 *
 * @return  status
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_start_io
 *
 ****************************************************************/
static fbe_status_t dooby_doo_stop_io(void)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = &dooby_doo_io_contexts[0];

    /* Run our I/O test.  Since we are running against all LUs,
     * there is only (1) rdgen context.
     */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_rdgen_context_check_io_status(context_p, 1, FBE_STATUS_OK, 0,    /* err count*/
                                                   FBE_TRUE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************
 * end dooby_doo_stop_io()
 ******************************************/

/*!**************************************************************
 * dooby_doo_clear_all_hooks()
 ****************************************************************
 * @brief   Clear any local hooks (so that any additional iterations
 *          will not attempt to set `unhit' hooks).
 *
 * @param None.               
 *
 * @return  status
 *
 * @author
 *  07/24/2010  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t dooby_doo_clear_all_hooks(void)
{
    /* First re-zero the hooks.
     */
    fbe_zero_memory(&dooby_doo_pause_params, sizeof(fbe_sep_package_load_params_t));
    fbe_zero_memory(&dooby_doo_debug_hooks, sizeof(fbe_sep_package_load_params_t));

    return FBE_STATUS_OK;
}
/******************************************
 * end dooby_doo_clear_all_hooks()
 ******************************************/

/*!***************************************************************************
 *          dooby_doo_populate_destination_drives()
 *****************************************************************************
 *
 * @brief   First disable `automatic' sparing.  Then determine which free drives 
 *          will be used for each of the raid groups under test and then add 
 *          them to the `extra' information for each of the raid groups under 
 *          test.  
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   dest_array_pp - Address of pointer to array of destination drive
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_populate_destination_drives(fbe_test_rg_configuration_t *rg_config_p,   
                                                           fbe_u32_t raid_group_count,
                                                           fbe_test_raid_group_disk_set_t **dest_array_pp)           
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       rg_index;
    fbe_u32_t       dest_index = 0;

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Refresh the `extra' disk information.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /* Step 2: Walk thru all the raid groups and add the `extra' drive to
     *         the array of destination drives.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Validate that there is at least (1) extra disk.
         */
        MUT_ASSERT_TRUE(current_rg_config_p->num_of_extra_drives > 0);

        /* Add (1) `extra' drive per raid group to the `destination' array.
         */
        dooby_doo_dest_array[dest_index] = current_rg_config_p->extra_disk_set[0];

        /* Goto next raid group.
         */
        current_rg_config_p++;
        dest_index++;
    }

    /* Update the address passed with the array of destination drives.
     */
    *dest_array_pp = &dooby_doo_dest_array[0];

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end dooby_doo_populate_destination_drives()
 ***************************************************************/

/*!***************************************************************************
 *          dooby_doo_wait_for_rebuild_completion()
 *****************************************************************************
 *
 * @brief   Wait for the required number of positions to complete
 *          rebuild.
 *
 * @param   rg_config_p - Pointer to raid group configuraiton to wait for
 * @param   raid_group_count - Count of raid groups wait for rebuild
 * @param   position_to_rebuild - Position to wait for rebuild for
 *
 * @return  None.
 *
 * @author
 *  01/25/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void dooby_doo_wait_for_rebuild_completion(fbe_test_rg_configuration_t * const rg_config_p,
                                                  fbe_u32_t raid_group_count,
                                                  fbe_u32_t position_to_rebuild)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_u32_t                               rg_index;
    fbe_object_id_t                         rg_object_id;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;

    /* Get the current target server
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* For all enabled raid groups.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* There is no need to wait if the rg_config is not enabled
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {        
            current_rg_config_p++;    
            continue;
        }

        /* Get the riad group object id
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Wait for the rebuild to complete
         */
        status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_rebuild,
                                                       DOOBY_DOO_WAIT_FOR_REBUILD_SECS);           
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(other_sp);
            status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_rebuild,
                                                           DOOBY_DOO_WAIT_FOR_REBUILD_SECS);           
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_sim_transport_set_target_server(this_sp);
        }

        /* Goto the next raid group*/
        current_rg_config_p++;

    } /* end for all raid groups that need to be rebuilt */

    return;
}
/*********************************************
 * end dooby_doo_wait_for_rebuild_completion()
 **********************************************/

/*!**************************************************************
 *          dooby_doo_test_proactive_copy_without_io()
 ****************************************************************
 *
 * @brief   Run I/O validataion after proactive copy.
 *
 * @param   rg_config_p - config to run test against. 
 *
 * @return  status
 *
 * @author
 *  03/23/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t dooby_doo_test_proactive_copy_without_io(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_u32_t       raid_group_count = 0;
    fbe_u32_t       position_to_spare;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Phase 1: Perform the following (without proactive copy running):
     *              1a) Write a background pattern
     *              1b) Initiate proactive copy
     *              1c) Wait for proactive copy to complete (swap-out also)
     *              1d) Read back the background pattern
     *              1e) Verify no errors in the user space
     */

    /* Step 1a) Write a fixed background pattern.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_WRITE_FIXED_PATTERN,
                                      DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_FALSE,                            /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Step 1b) Initiate a proactive copy
     */
    position_to_spare = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Forcing drives into proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_spare,
                                                              FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                              NULL /* The system selects the destination drives*/);
                                                              
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__); 
    
    /* Step 1c) Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_spare,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Step 1d) Read back the fixed pattern
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_READ_FIXED_PATTERN,
                                      DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_FALSE,                            /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Step 1e) Verify no error in user space
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_VERIFY_DATA,
                                      DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_FALSE,                            /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Return the execution status.
     */
    return status;
}
/************************************************
 * end dooby_doo_test_proactive_copy_without_io()
 ************************************************/

/*!***************************************************************************
 *          dooby_doo_test_bind_during_copy()
 *****************************************************************************
 *
 * @brief   This test first unbinds one of the luns, then starts I/O and then
 *          binds the original lun.
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The position in the raid group(s) to copy
 * @param   b_run_abort_io - Determines if we will randomly inject aborts or not
 * @param   rdgen_operation - The rdgen I/O operation.  For instance w/r/c.
 *
 * @return  status
 *
 * @note    Tests performed:
 *          o Bind a lun during a copy operations with I/O
 *              + Unbind one of the luns
 *              + Start I/O
 *              + Set `pause' to stop copy operation when virtual drive enters mirror mode
 *              + Start copy operation
 *              + Set `pause' to stop copy operation when copy is 50% complete
 *              + Set a debug hook when virtual drive enters `Failed' state
 *              + Re-start copy operation
 *              + Immediately pull and re-insert source drive
 *              + Validate that the virtual drive goes to failed
 *              + Validate that the virtual drive remains in mirror mode
 *              + Remove failed state debug hook
 *              + Validate virtual drive hits 50% copied hook
 *              + Remove 50% complete hook and validate copy completes
 *
 * @author
 *  01/15/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_test_bind_during_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t raid_group_count,
                                                    fbe_u32_t position_to_copy,
                                                    fbe_bool_t b_run_abort_io,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_rdgen_pattern_t rdgen_pattern)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_random_abort_msec_t            ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_spare_swap_command_t                copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    fbe_test_raid_group_disk_set_t         *dest_array_p = NULL;
    fbe_test_sep_background_copy_state_t    current_copy_state;
    fbe_test_sep_background_copy_state_t    copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t    validation_event;
    fbe_u32_t                               percentage_rebuilt_before_pause = 5;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_u32_t                               rg_index;
    fbe_u32_t                               lun_index;
    fbe_u32_t                               lun_index_to_reinsert;
    fbe_u32_t                               num_of_luns_with_io;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;
    fbe_object_id_t                         lun_object_id;

    /* Determine the abort mode.
     */
    if (b_run_abort_io == FBE_TRUE)
    {
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC;
    }

    /* Step 1: Remove all the luns
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing %d luns from raid groups ==",
               __FUNCTION__,  current_rg_config_p->number_of_luns_per_rg);
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* We assume there is at (3) luns
         */
        MUT_ASSERT_TRUE(current_rg_config_p->number_of_luns_per_rg > 2);

        /* Unbind all the luns 
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            logical_unit_configuration_p = &current_rg_config_p->logical_unit_configuration_list[lun_index];
    
            /* Destroy a logical unit. */
            status = fbe_test_sep_util_destroy_logical_unit(logical_unit_configuration_p->lun_number);
            MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical Unit failed.");
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing luns - complete ==",
               __FUNCTION__);

    /* Step 2: Bind all except the last lun
     */
    current_rg_config_p = rg_config_p;
    num_of_luns_with_io = (current_rg_config_p->number_of_luns_per_rg - 1);
    lun_index_to_reinsert = (current_rg_config_p->number_of_luns_per_rg - 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-bind %d luns per raid group==",
               __FUNCTION__,  num_of_luns_with_io);
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Re-bind all the luns 
         */
        logical_unit_configuration_p = &current_rg_config_p->logical_unit_configuration_list[0];
    
        /* Create a RAID group with user provided configuration. */
        status = fbe_test_sep_util_create_logical_unit_configuration(logical_unit_configuration_p,
                                                                     num_of_luns_with_io);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical Unit failed.");

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Binding of all (minus 1) luns - complete==",
               __FUNCTION__);

    /* Step 3: Wait for all the luns to become ready
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for %d luns per raid group to become ready==",
               __FUNCTION__,  (current_rg_config_p->number_of_luns_per_rg - 1));
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Wait for all luns to be ready for I/O
         */
        for (lun_index = 0; lun_index < num_of_luns_with_io; lun_index++)
        {
            /* All luns show be ready to test.
             */
            logical_unit_configuration_p = &current_rg_config_p->logical_unit_configuration_list[lun_index];
            status = fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
                
            /* verify the lun get to ready state in reasonable time. */
            status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                  lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                                  DOOBY_DOO_WAIT_OBJECT_TIMEOUT_MS,
                                                                                  FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* verify the lun connect to BVD in reasonable time. */
            status = fbe_test_sep_util_wait_for_LUN_edge_on_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                    lun_object_id,
                                                                    60000);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x connects to BVD",
                       __FUNCTION__, lun_object_id);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for luns ready - complete==",
               __FUNCTION__);

    /* Step 3: Start I/O (random)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, dooby_doo_threads, DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS);
    status = dooby_doo_start_io(rdgen_operation,
                                rdgen_pattern,
                                dooby_doo_threads,
                                DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                ran_abort_msecs,
                                FBE_TEST_SEP_IO_DUAL_SP_MODE_RANDOM);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Step 5: Re-bind the last lun
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-bind last lun per raid group==",
               __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Re-bind the last lun (we don't need to wait for it to become ready)
         */
        logical_unit_configuration_p = &current_rg_config_p->logical_unit_configuration_list[lun_index_to_reinsert];
    
        /* Create a RAID group with user provided configuration. */
        status = fbe_test_sep_util_create_logical_unit_configuration(logical_unit_configuration_p,
                                                                     1);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical Unit failed.");

        /* Goto the next raid group.
         */
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-bind of last lun - complete==",
               __FUNCTION__);

    /* Allow I/O to continue for a short period
     */
    fbe_api_sleep(dooby_doo_io_msec_short);

    /* Populate the destination array
     */
    status = dooby_doo_populate_destination_drives(rg_config_p, raid_group_count, &dest_array_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4: Set `pause' to stop copy operation when percentage rebuild it hit.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause, /* pause at 10% rebuilt */
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               dest_array_p /* Specify the destination drives */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 5: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &dooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 6: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               dest_array_p /* Specify the destination drives */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 7: Remove 5% complete hook and validate copy completes.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               dest_array_p /* Specify the destination drives */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__); 

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(dooby_doo_io_msec_long);

    /* Step 8: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = dooby_doo_stop_io();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - Successful ==", __FUNCTION__);

    /* Return the execution status.
     */
    return status;
}
/***************************************************
 * end dooby_doo_test_bind_during_copy()
 ***************************************************/

/*!***************************************************************************
 *          dooby_doo_test_glitch_source_drive()
 *****************************************************************************
 *
 * @brief   This test `glitches' the source drive of a proactive copy by
 *          clearing EOL.  It validates the following:
 *          o Source Drive Fails.
 *              + Quiesce I/O
 *              + Fail outstanding I/Os with failed/retryable status
 *              + Virtual drive goes to failed state
 *              + Parent raid groups marks rebuild logging for that position,
 *                becomes degraded
 *          o Virtual drive will wait up to five minutes for source drive to 
 *            return before swapping-out the source position.
 *              + If the source drive returns within 5 minutes:
 *                  - Virtual drive becomes Ready
 *                  - Quiesce I/O
 *                  - Enable copy operation
 *                  - Unquiesce I/O
 *                  - Resume copy operation
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The position in the raid group(s) to copy
 * @param   b_run_abort_io - Determines if we will randomly inject aborts or not
 * @param   rdgen_operation - The type of rdgen opertion to run
 * @param   rdgen_pattern - The rdgen pattern to read or write
 *
 * @return  status
 *
 * @note    Tests performed:
 *          o Remove then immediately re-insert source drive:
 *              + Start I/O
 *              + Set `pause' to stop copy operation when virtual drive enters mirror mode
 *              + Start copy operation
 *              + Set `pause' to stop copy operation when copy is 50% complete
 *              + Set a debug hook when virtual drive enters `Failed' state
 *              + Re-start copy operation
 *              + Immediately pull and re-insert source drive
 *              + Validate that the virtual drive goes to failed
 *              + Validate that the virtual drive remains in mirror mode
 *              + Remove failed state debug hook
 *              + Validate virtual drive hits 50% copied hook
 *              + Remove 50% complete hook and validate copy completes
 *
 * @author
 *  11/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_test_glitch_source_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_u32_t raid_group_count,
                                                       fbe_u32_t position_to_copy,
                                                       fbe_bool_t b_run_abort_io,
                                                       fbe_rdgen_operation_t rdgen_operation,
                                                       fbe_rdgen_pattern_t rdgen_pattern)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t       percentage_rebuilt_before_pause = 5;

    /* Determine the abort mode.
     */
    if (b_run_abort_io == FBE_TRUE)
    {
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC;
    }

    /*! @note Remove then immediately re-insert source drive.
     */

    /* Step 1: Start Read Only I/O (so that we don't mark NR)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, dooby_doo_threads, DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS);
    status = dooby_doo_start_io(rdgen_operation,
                                rdgen_pattern,
                                dooby_doo_threads,
                                DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                ran_abort_msecs,
                                FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(dooby_doo_io_msec_short);

    /* Step 2: Set `pause' to stop copy operation when percentage rebuild it hit.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause, /* pause at 5% rebuilt */
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 3: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &dooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 4: Set a hook in the parent raid groups so that we can confirm that
     *         no chunks have been marked NR.
     */
    fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 5: There are two possibilities:
     *          1. The source drive comes back before the `glitch delay'
     *                  OR
     *          2. It takes more than the `glitch delay' to come back
     *
     *         Set debug hooks in the virtual drives so that either 
     *         scenario is handled.
     */
    status = dooby_doo_set_source_drive_glitched_hook(rg_config_p, raid_group_count, position_to_copy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = dooby_doo_set_source_drive_removed_hook(rg_config_p, raid_group_count, position_to_copy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Invoke method to `glitch' the source pvd.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s `glitch' source pvds ==", 
               __FUNCTION__);
    status = fbe_test_sep_background_pause_glitch_drives(rg_config_p, raid_group_count, position_to_copy,
                                     FBE_TRUE,  /* Glitch the source drive */
                                     FBE_FALSE  /* Don't glitch the destination drive*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Due to scheduling some drives will take longer than the 
     *         `glitch period' before they become enabled.  Thus in some cases
     *         the virtual drive will go broken (i.e. will not `glitch').
     *         dooby_doo_handle_source_drive_glitch_or_fail() determines and 
     *         handles either case.
     */
    status = dooby_doo_handle_source_drive_glitch_or_fail(rg_config_p, raid_group_count, position_to_copy,
                                                          rdgen_operation /* If fail determines if parent will be marked NR or not */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 9: Remove 5% complete hook and validate copy completes.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__); 

    /* Step 10: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = dooby_doo_stop_io();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - Successful ==", __FUNCTION__);

    /* Return the execution status.
     */
    return status;
}
/***************************************************
 * end dooby_doo_test_glitch_source_drive()
 ***************************************************/

/*!***************************************************************************
 *          dooby_doo_test_remove_reinsert_source_drive()
 *****************************************************************************
 *
 * @brief   This test removes and re-inserts the source drive during a copy 
 *          operation. It validates the following:
 *          o Source Drive Fails.
 *              + Quiesce I/O
 *              + Fail outstanding I/Os with failed/retryable status
 *              + Virtual drive goes to failed state
 *              + Parent raid groups marks rebuild logging for that position,
 *                becomes degraded
 *          o Virtual drive will wait up to five minutes for source drive to 
 *            return before swapping-out the source position.
 *              + If the source drive returns within 5 minutes:
 *                  - No chunks will be marked NR in the parent raid group
 *                  - Virtual drive becomes Ready
 *                  - Quiesce I/O
 *                  - Enable copy operation
 *                  - Unquiesce I/O
 *                  - Resume copy operation
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The position in the raid group(s) to copy
 * @param   b_run_abort_io - Determines if we will randomly inject aborts or not
 * @param   rdgen_operation - The type of rdgne operation w/r/c, etc
 * @param   rdgen_pattern - The rdgen pattern expected 
 *
 * @return  status
 *
 * @note    Tests performed:
 *          o Remove then immediately re-insert source drive:
 *              + Start I/O
 *              + Set `pause' to stop copy operation when virtual drive enters mirror mode
 *              + Start copy operation
 *              + Set `pause' to stop copy operation when copy is 50% complete
 *              + Set a debug hook when virtual drive enters `Failed' state
 *              + Re-start copy operation
 *              + Immediately pull and re-insert source drive
 *              + Validate that the virtual drive goes to failed
 *              + Validate that the virtual drive remains in mirror mode
 *              + Remove failed state debug hook
 *              + Validate virtual drive hits 50% copied hook
 *              + Remove 50% complete hook and validate copy completes
 *
 * @author
 *  07/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_test_remove_reinsert_source_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_copy,
                                                                fbe_bool_t b_run_abort_io,
                                                                fbe_rdgen_operation_t rdgen_operation,
                                                                fbe_rdgen_pattern_t rdgen_pattern)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t       percentage_rebuilt_before_pause = 5;

    /* Determine the abort mode.
     */
    if (b_run_abort_io == FBE_TRUE)
    {
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC;
    }

    /*! @note Remove then immediately re-insert source drive.
     */

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting Read Only I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, dooby_doo_threads, DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS);
    status = dooby_doo_start_io(rdgen_operation,
                                rdgen_pattern,
                                dooby_doo_threads,
                                DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                ran_abort_msecs,
                                FBE_TEST_SEP_IO_DUAL_SP_MODE_PEER_ONLY);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(dooby_doo_io_msec_short);

    /* Step 2: Set `pause' to stop copy operation when virtual drive enters 
     *         mirror mode.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 3: Start copy operation.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_START;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate metadata rebuild start - successful. ==", __FUNCTION__); 

    /* Step 4: Set `pause' when 5% copied
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 5% rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when metadata rebuild starts - successfully. ==", __FUNCTION__);

    /* Step 5: Set a debug hook when virtual drive enters `Failed' state
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &dooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &dooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 7: Set a rebuild mark NR done hook in parent
     */
    fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 8: Immediately pull and re-insert source drive
     */
    big_bird_remove_drives(rg_config_p, raid_group_count,
                           FBE_FALSE,   /* Don't transition pvd to fail */
                           FBE_TRUE,    /* Pull and re-insert same drive */
                           FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    big_bird_insert_drives(rg_config_p, raid_group_count,
                           FBE_FALSE    /* Don't transition pvd to activate */);

    /* Step 9: Validate that the virtual drive goes to failed.  Then 
     *         remove the debug hook.
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &dooby_doo_debug_hooks,
                                                                        FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Wait for parent raid group mark NR
     */
    fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 11: Validate that no chunks were mark NR in the parent raid group.
     */
    fbe_test_sep_rebuild_validate_raid_group_not_marked_nr(rg_config_p, raid_group_count);

    /* Step 12: Remove the eval mark NR done hook.
     */
    fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 13: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 14: Validate that none of the raid groups have any positions marked
     *         `rebuild logging'.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that no raid groups are rb logging ==", __FUNCTION__);
    status = sep_rebuild_utils_verify_no_raid_groups_are_rb_logging(rg_config_p, raid_group_count, position_to_copy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that no raid groups are rb logging - successful==", __FUNCTION__);

    /* Step 15: Remove 50% complete hook and validate copy completes.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__); 

    /* Step 16: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = dooby_doo_stop_io();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - Successful ==", __FUNCTION__);

    /* Return the execution status.
     */
    return status;
}
/***************************************************
 * end dooby_doo_test_remove_reinsert_source_drive()
 ***************************************************/

/*!***************************************************************************
 *          dooby_doo_test_remove_source_drive()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a copy operation.  It 
 *          validate the following:
 *          o Source Drive Fails.
 *              + Quiesce I/O
 *              + Fail outstanding I/Os with failed/retryable status
 *              + Virtual drive goes to failed state
 *              + Parent raid groups marks rebuild logging for that position,
 *                becomes degraded
 *          o Virtual drive will wait up to five minutes for source drive to 
 *            return before swapping-out the source position.
 *              + If the 5 minute timer expires without the source drive returning:
 *                  - Swap-out source drive/edge
 *                  - Enter pass-thru mode using destination drive
 *                  - Give copy checkpoint to parent raid group to mark NR
 *                  - Virtual drive becomes Ready (in pass-thru)
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The position in the raid group(s) to copy
 * @param   b_run_abort_io - Determines if we will randomly inject aborts or not
 * @param   rdgen_operation - The rdgen I/O operation.  For instance w/r/c.
 * @param   rdgen_pattern - The rdgen pattern to read or write
 *
 * @return  status
 *
 * @note    Tests performed:
 *          o Remove the source drive:
 *              + Start I/O
 *              + Set `pause' to stop copy operation when virtual drive enters mirror mode
 *              + Start copy operation
 *              + Set `pause' to stop copy operation when copy is 50% complete
 *              + Set a debug hook when virtual drive enters `Failed' state
 *              + Re-start copy operation
 *              + Immediately pull the source drive
 *              + Validate that the virtual drive goes to failed
 *              + Validate that the virtual drive goes to pass-thru mode
 *              + Validate that the destination drive is now the primary
 *              + Validate that the virtual drive checkpoint is ~50%
 *              + Re-insert the source drives so that EOL can be cleared
 *              + Cleanup the copy operation (remove all debug hooks)
 *              + Validate that parent raid for position is ~50% rebuilt
 *              + Validate that the virtual drive doesn't request a spare
 *
 * @author
 *  07/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_test_remove_source_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_u32_t raid_group_count,
                                                       fbe_u32_t position_to_copy,
                                                       fbe_bool_t b_run_abort_io,
                                                       fbe_rdgen_operation_t rdgen_operation,
                                                       fbe_rdgen_pattern_t rdgen_pattern)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t       percentage_rebuilt_before_pause = 5;

    /* Determine the abort mode.
     */
    if (b_run_abort_io == FBE_TRUE)
    {
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC;
    }

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, dooby_doo_threads, DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS);
    status = dooby_doo_start_io(rdgen_operation,
                                rdgen_pattern,
                                dooby_doo_threads,
                                DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                ran_abort_msecs,
                                FBE_TEST_SEP_IO_DUAL_SP_MODE_RANDOM);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(dooby_doo_io_msec_short);

    /* Step 2: Set `pause' to stop copy operation when virtual drive enters 
     *         mirror mode.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 3: Start copy operation.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_MODE_SET_TO_MIRROR;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate mirror mode - successful. ==", __FUNCTION__); 

    /* Step 4: Set `pause' to stop copy operation when copy is 5% complete
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 5% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 5: Set a debug hook when virtual drive enters `Failed' state
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &dooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Pull the source drive
     */
    big_bird_remove_drives(rg_config_p, raid_group_count,
                           FBE_FALSE,   /* Don't transition pvd to fail */
                           FBE_TRUE,    /* Pull and re-insert same drive */
                           FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);


    /* Step 7: Validate that virtual drive is marked rebuild logging.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that pos: %d is marked rb logging ==", __FUNCTION__, position_to_copy);
    status = sep_rebuild_utils_verify_raid_groups_rb_logging(rg_config_p, raid_group_count, position_to_copy);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that raid groups are rb logging - successful==", __FUNCTION__);

    /* Step 8: Validate that the virtual drive goes to failed.  Then 
     *         remove the debug hook.
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &dooby_doo_debug_hooks,
                                                                        FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Set the `need replacement drive' timer to (5) seconds then sleep for
     *          (10) seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);
    fbe_api_sleep(10000);

    /* Step 10: Validate that the virtual drive goes to pass-thru without
     *          completing the copy operation.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__); 

    /* Step 13: Stop I/O and validate no errors.
     */
    fbe_api_sleep(10000);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = dooby_doo_stop_io();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - Successful ==", __FUNCTION__);

    /* Step 14: Remove any `local' debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing `local' debug hooks ==", __FUNCTION__);
    status = dooby_doo_clear_all_hooks();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing `local' debug hooks - Successful==", __FUNCTION__);

    /* Restore the spare swap-in timer back to default of 5 minutes.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Return the execution status.
     */
    return status;
}
/******************************************
 * end dooby_doo_test_remove_source_drive()
 ******************************************/

/*!***************************************************************************
 *          dooby_doo_test_paco_r()
 *****************************************************************************
 *
 * @brief   This test validates the PACO-R feature:
 *          o EOL is posted to the copy position
 *              + Validate that the media error thresholds are disabled for
 *                all positions in the raid group EXCEPT the paoc position
 *          o Proactive Copy completes.  Validate that the media error 
 *            thresholds are restored to the default value for all positions 
 *            in the raid group.
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_copy - The position in the raid group(s) to copy
 * @param   b_run_abort_io - Determines if we will randomly inject aborts or not
 * @param   rdgen_operation - The type of rdgen opertion to run
 * @param   rdgen_pattern - The rdgen pattern to read or write
 *
 * @return  status
 *
 * @note    Tests performed:
 *          o Remove then immediately re-insert source drive:
 *              + Start I/O
 *              + Set hook (on both SPs) when media error thresholds are disabled
 *              + Set `pause' to stop copy operation when virtual drive enters mirror mode
 *              + Start copy operation
 *              + Validate that increase thresholds hook is hit
 *              + Validate that extended media error handling (EMEH) thresholds are
 *                disabled for ALL raid group positions
 *              + Set hook when thresholds are restored
 *              + Re-start copy operation
 *              + Validate copy operation is complete
 *              + Validate that the virtual drive remains in mirror mode
 *              + Remove failed state debug hook
 *              + Validate virtual drive hits 50% copied hook
 *              + Remove 50% complete hook and validate copy completes
 *
 * @author
 *  05/12/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t dooby_doo_test_paco_r(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t position_to_copy,
                                          fbe_bool_t b_run_abort_io,
                                          fbe_rdgen_operation_t rdgen_operation,
                                          fbe_rdgen_pattern_t rdgen_pattern)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;

    /* Determine the abort mode.
     */
    if (b_run_abort_io == FBE_TRUE)
    {
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC;
    }

    /* Step 1: Start Read Only I/O (so that we don't mark NR)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, dooby_doo_threads, DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS);
    status = dooby_doo_start_io(rdgen_operation,
                                rdgen_pattern,
                                dooby_doo_threads,
                                DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                ran_abort_msecs,
                                FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(dooby_doo_io_msec_short);

    /* Step 2: Enable the Extended Media Error Handling.
     */
    status = fbe_test_sep_raid_group_class_set_emeh_values(
                                  FBE_RAID_EMEH_MODE_ENABLED_NORMAL,    /*! @note Enable EMEH at the raid group class*/
                                                           FBE_FALSE,   /*! @note Do not change thresholds */
                                                           0,           /*! @note No change to defaault threshold increase */
                                                           FBE_FALSE    /*! Don't persist the changes */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Set a hook when proactive copy starts and EMEH is initiated.  Also
     *         set a hook with the proactive copy start is `done' (i.e. both active
     *         and passive SPs have completed the EMEH proactive copy start).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set EMEH paco start hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_START,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_PAUSE,
                                                      FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set EMEH paco start done hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_DONE,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4: Set a hook to stop the virutal drive copy operaiton once the paged
     *         metadata rebuild starts.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 5: Start copy operation.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_START;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate metadata rebuild start - successful. ==", __FUNCTION__); 

    /* Step 6: Wait for all the raid groups to hit the EMEH paco start hook. Then remove
     *         this hook and wait for the proactive copy start `done' hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH paco start hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_START,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_PAUSE,
                                                      FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH paco start hook - complete ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_START,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_PAUSE,
                                                      FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Wait for the EMEH proactive copy start `done' logged on both SPs.
     *         Then remove the hook and validate that thresholds have been disabled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH paco done hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_DONE,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH paco start done hook - complete ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_PACO, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_PACO_DONE,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Validate that the thresholds have been disabled for each drive position
     *         for each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that EMEH thresholds have been disabled ==", __FUNCTION__);
    status = fbe_test_sep_raid_group_validate_emeh_values(rg_config_p, raid_group_count, position_to_copy,
                                                          FBE_FALSE,    /* We do not expect the default values*/
                                                          FBE_TRUE,     /* We expect that the thresholds have been disabled */
                                                          FBE_FALSE     /* We do not expect the values to be increased */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Set the hooks for the EMEH restore values (occurs after copy complete job
     *         is finished).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set EMEH restore defaults start hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_START,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set EMEH restore defaults done hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_DONE,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Add a hook when the virtual drive has initiated copy complete job.
     *          Confirmation is disabled so the job service isn't blocked.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Initiate copy complete - successful. ==", __FUNCTION__); 

    /* Step 12: Wait for and clear the initiate copy complete job.
     *          Confirmation is disabled so the job service isn't blocked.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_MODE_SET_TO_PASS_THRU;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for copy complete - successful. ==", __FUNCTION__); 

    /* Step 13: Validate that EMEH setting have been restore to the default values.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH restore defaults start hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_START,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH restore defaults start hook - complete ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_START,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 14: Wait for the EMEH restore defaults `done' logged on both SPs.
     *          Then remove the hook and validate that EMEH settings have been restored
     *          to the default values on BOTH SPs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH restore defaults done hook ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_DONE,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for EMEH restore defaults done hook - complete ==", __FUNCTION__);
    status = fbe_test_rg_config_use_rg_hooks_both_sps(rg_config_p, raid_group_count,
                                                      1, /*! @note We always use first mirrored pair on RAID-10 */ 
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_EMEH_RESTORE, 
                                                      FBE_RAID_GROUP_SUBSTATE_EMEH_RESTORE_DONE,
                                                      0, 0,
                                                      SCHEDULER_CHECK_STATES, 
                                                      SCHEDULER_DEBUG_ACTION_LOG,
                                                      FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 15: Validate that the thresholds have been restored for each drive position
     *         for each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that EMEH thresholds have been restored ==", __FUNCTION__);
    status = fbe_test_sep_raid_group_validate_emeh_values(rg_config_p, raid_group_count, position_to_copy,
                                                          FBE_TRUE,    /* We do expect the default values*/
                                                          FBE_FALSE,   /* We do not expect that the thresholds have been disabled */
                                                          FBE_FALSE    /* We don't expect the values to be increased */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 16: Cleanup the copy operation
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &dooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__); 

    /* Step 17: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = dooby_doo_stop_io();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - Successful ==", __FUNCTION__);

    /* Step 18: Restore the EMEH values back to the default (either enabled or disabled)
     */
    status = fbe_test_sep_raid_group_class_set_emeh_values(FBE_RAID_EMEH_MODE_DEFAULT,  /*! @note Restore EMEH to defaults*/
                                                           FBE_FALSE,                   /*! @note Ignored */
                                                           0,                           /*! @note Ignored */
                                                           FBE_TRUE                     /*! Persist the changes */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Return the execution status.
     */
    return status;
}
/***************************************************
 * end dooby_doo_test_paco_r()
 ***************************************************/

/*!**************************************************************
 * dooby_doo_advanced_copy_test()
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
static void dooby_doo_advanced_copy_test(fbe_test_rg_configuration_t * const rg_config_p,
                                    fbe_u32_t raid_group_count,
                                    fbe_api_rdgen_peer_options_t peer_options,
                                    fbe_data_pattern_t background_pattern,
                                    fbe_data_pattern_t pattern)
{
    fbe_api_rdgen_context_t *read_context_p = NULL;
    fbe_api_rdgen_context_t *write_context_p = NULL;
    fbe_status_t status;
    fbe_u32_t   index;
    fbe_u32_t   num_luns;
    fbe_u32_t   background_pass_count = 0;
    fbe_u32_t   position_to_spare;
    fbe_u32_t   pause_after_raid_group_percent = 10;    /* Pause the virtual drive copy after 10% copied */

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting == ", __FUNCTION__);

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
                                        DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS);

    /* Step 1b) Initiate a proactive copy
     */
    position_to_spare = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start proactive copy and pause after: %d pct done ==", __FUNCTION__, pause_after_raid_group_percent); 
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                      raid_group_count,
                                                      position_to_spare,
                                                      FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                      NULL /* No destination for proactive copy*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__); 

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

    /* Wait for the copy operation to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete (after restarting). ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                               raid_group_count,
                                                               position_to_spare,
                                                               FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                               FBE_TRUE, /* Wait for swap out*/
                                                               FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete (after restarting). ==", __FUNCTION__);

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
 * end dooby_doo_advanced_copy_test()
 ******************************************/

/*!**************************************************************
 *          dooby_doo_test_rg_config()
 ****************************************************************
 *
 * @brief   Run I/O validataion after proactive copy.  The run I/O
 *          with proactive copy operation in progress.
 *
 * @param   rg_config_p - config to run test against. 
 * @param   context_p - Pointer to rdgen context              
 *
 * @return None.   
 *
 * @author
 *  03/23/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
void dooby_doo_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_bool_t      b_run_abort_io = FBE_FALSE;
    fbe_bool_t      b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_bool_t      b_abort_cases_only = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_rdgen_pattern_t rdgen_pattern;
    fbe_u32_t       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t       position_to_copy = 0;

    /* This test uses `big_bird' to remove drives so set the copy position to
     * next position to remove (this can only be done for source drive removal).
     */
    position_to_copy = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Initialize our `short' and `long' I/O times
     */
    dooby_doo_set_io_seconds();

    /* Determine if we should run abort cases or not.
     */
    if (test_level > 0)
    {
        b_run_abort_io = FBE_TRUE;
    }

    /*! @note Current requirement is that proactive copy (one of the methods
     *        used to trigger a copy operation) does not get executed for
     *        non-redundant raid groups.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        /* Validate that proactive copy doesn't run on RAID-0.
         */
        status = dooby_doo_validate_copy_doesnt_run_on_raid0(rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! @note Copy operations (of any type) are not supported for RAID-0 raid groups.
         *        Therefore skip these tests if the raid group type is RAID-0.  
         */
        /* Print a message and return.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Skipping test with copy_type: %d.  Copy operations not supported on RAID-0 ==",
                   __FUNCTION__, copy_type);
        return;
    }

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);

    /*! @note All the spare drives will come from the unbound pvds.  a.k.a.
     *        the `automatic' spare pool.  Therefore enable automatic
     *        sparing (which should be enabled by default) will select the
     *        next available drive.
     */
    /*! @note Automatic sparing is the default fbe_api_control_automatic_hot_spare(FBE_TRUE); */

    /* Write a zero pattern so that the first tests can perform a read check
     */
    rdgen_pattern = FBE_RDGEN_PATTERN_ZEROS;
    big_bird_write_zero_background_pattern();

    /* Test 1: Run the `remove and re-insert the source drive' test
     */
    status = dooby_doo_test_remove_reinsert_source_drive(rg_config_p, raid_group_count, position_to_copy,
                                                         b_run_abort_io,
                                                         FBE_RDGEN_OPERATION_READ_CHECK,
                                                         rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Randomize between the background patterns of zero and lba/pass.
     */
    if (fbe_random() % 3)
    {
        rdgen_pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    }
    else
    {
        rdgen_pattern = FBE_RDGEN_PATTERN_ZEROS;
    }

    /* Test 2: `Glitch' the source drive.  Validates that if the source
     *          pvd edge temporarily goes disabled, that the copy proceeds.
     */
    status = dooby_doo_test_glitch_source_drive(rg_config_p, raid_group_count, position_to_copy,
                                                b_run_abort_io,
                                                FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 3: Run the `remove source' test.
     */
    status = dooby_doo_test_remove_source_drive(rg_config_p, raid_group_count, position_to_copy,
                                                b_run_abort_io,
                                                FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Reduce the spare swap-in timer from 5 minutes (300 seconds) to (1) second.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    /* Test 4:  Bind a lun while there is a copy operation in progress
     */
    status = dooby_doo_test_bind_during_copy(rg_config_p, raid_group_count, position_to_copy,
                                             b_run_abort_io,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 5: Perform the following (without proactive copy running):
     *              1a) Write a background pattern
     *              1b) Initiate proactive copy
     *              1c) Wait for proactive copy to complete (swap-out also)
     *              1d) Read back the background pattern
     *              1e) Verify no errors in the user space
     */
    if (!b_abort_cases_only)
    {
        status = dooby_doo_test_proactive_copy_without_io(rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Test 6: Validate PACO-R feature: Thresholds are disabled during
     *         proactive copy, thresholds are restored to default values
     *         when proactive copy completes.
     */
    status = dooby_doo_test_paco_r(rg_config_p, raid_group_count, position_to_copy,
                                   b_run_abort_io,
                                   FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                   rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @todo This tests that we preserve the contents of the data written 
     *        before we start the copy operation.
     *
     *  @todo This test is currently not functional.
     */
    //dooby_doo_advanced_copy_test(rg_config_p, raid_group_count,
    //                             FBE_API_RDGEN_PEER_OPTIONS_INVALID,
    //                             background_pattern, FBE_DATA_PATTERN_LBA_PASS);

    /* Test 7: Enable proactive copy during I/O.
     */
    status = fbe_test_sep_io_configure_background_operations(FBE_TRUE, /* Enable background ops */
                                                             FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_PROACTIVE_COPY /* Start proactive copy */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run caterpillar I/Os without aborts
     */
    if (!b_abort_cases_only)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
                                          DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                         /* Don't inject aborts. */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_TRUE,                         /* Inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);


    /*! @note Block operation testing is done in other scenarios.
     */

    /* Test 8: Enable user copy during I/O.
     */
    copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    status = fbe_test_sep_io_configure_background_operations(FBE_TRUE, /* Enable background ops */
                                                             FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_USER_COPY /* Start user copy */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run standard with prefill I/O without aborts
     */
    if (!b_abort_cases_only)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                         /* Don't inject aborts. */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);


    }

    /* Run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      DOOBY_DOO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      DOOBY_DOO_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_TRUE,                             /* Inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Disable copy during I/O.
     */
    status = fbe_test_sep_io_configure_background_operations(FBE_FALSE, /* Disable background ops */
                                                             FBE_TEST_SEP_IO_BACKGROUND_OP_ENABLE_INVALID /* Disable all */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the spare swap-in timer back to default of 5 minutes.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    /* Set the sector trace stop on error off.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    return;
}
/******************************************
 * end dooby_doo_test_rg_config()
 ******************************************/

/*!**************************************************************
 * dooby_doo_test()
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
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void dooby_doo_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        /* Extended 
         */
        raid_group_types = DOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        /* Qual 
         */
        raid_group_types = DOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES;
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Loop over the number of configurations we have.
     */
    for (raid_group_type_index = 0; raid_group_type_index < raid_group_types; raid_group_type_index++)
    {
        if (test_level > 0)
        {
            /* Extended
             */
            rg_config_p = &dooby_doo_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            /* Qual
             */
            rg_config_p = &dooby_doo_raid_groups_qual[raid_group_type_index][0];
        }

        /* Since this doesn't test degraded disk not additional disk are 
         * required.  All the copy disk will come from the automatic spare pool.
         */

        if (raid_group_type_index + 1 >= raid_group_types) {
            fbe_test_run_test_on_rg_config(rg_config_p, NULL, dooby_doo_test_rg_config,
                                           DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP,
                                           DOOBY_DOO_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, dooby_doo_test_rg_config,
                                           DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP,
                                           DOOBY_DOO_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }

    } /* for all raid group types */

    return;
}
/******************************************
 * end dooby_doo_test()
 ******************************************/

/*!**************************************************************
 * dooby_doo_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void dooby_doo_dualsp_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        raid_group_types = DOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        raid_group_types = DOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES;
    }


    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Loop over the number of configurations we have.
     */
    for (raid_group_type_index = 0; raid_group_type_index < raid_group_types; raid_group_type_index++)
    {
        if (test_level > 0)
        {
            rg_config_p = &dooby_doo_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            rg_config_p = &dooby_doo_raid_groups_qual[raid_group_type_index][0];
        }

        /* Since this doesn't test degraded disk not additional disk are 
         * required.  All the copy disk will come from the automatic spare pool.
         */
        if (raid_group_type_index + 1 >= raid_group_types) {
            fbe_test_run_test_on_rg_config(rg_config_p, NULL, dooby_doo_test_rg_config,
                                           DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP,
                                           DOOBY_DOO_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, dooby_doo_test_rg_config,
                                           DOOBY_DOO_TEST_LUNS_PER_RAID_GROUP,
                                           DOOBY_DOO_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }

    } /* for all raid group types */

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end dooby_doo_dualsp_test()
 ******************************************/

/*!**************************************************************
 * dooby_doo_setup()
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
void dooby_doo_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_array_t *array_p = NULL;
    fbe_u32_t rg_index, raid_type_count;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        if (test_level > 0)
        {
            /* Extended.
             */
            array_p = &dooby_doo_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &dooby_doo_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            fbe_test_sep_util_populate_rg_num_extra_drives(&array_p[rg_index][0]);
        }

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
        sep_config_load_sep_and_neit();
    }
    
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end dooby_doo_setup()
 **************************************/

/*!**************************************************************
 * dooby_doo_dualsp_setup()
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
void dooby_doo_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_array_t *array_p = NULL;
    fbe_u32_t rg_index, raid_type_count;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        if (test_level > 0)
        {
            /* Extended.
             */
            array_p = &dooby_doo_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &dooby_doo_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            fbe_test_sep_util_populate_rg_num_extra_drives(&array_p[rg_index][0]);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /*! Determine and load the physical config and populate all the 
         *  raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

    }

    return;
}
/**************************************
 * end dooby_doo_dualsp_setup()
 **************************************/

/*!**************************************************************
 * dooby_doo_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the grover test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void dooby_doo_cleanup(void)
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
 * end dooby_doo_cleanup()
 ******************************************/

/*!**************************************************************
 * dooby_doo_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the grover test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void dooby_doo_dualsp_cleanup(void)
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
 * end dooby_doo_dualsp_cleanup()
 ******************************************/

/*************************
 * end file dooby_doo_test.c
 *************************/


