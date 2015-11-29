/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file abby_cadabby_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a correctable error test.
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
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * abby_cadabby_short_desc = "raid correctable error test";
char * abby_cadabby_long_desc ="\
The AbbyCadabby scenario injects correctable errors with I/O.\n\
\n\
Depending on the redundancy and the raid test level a single drive for each \n\
raid group will be removed.  For raid test levels greater than 0 a single   \n\
drive will be removed for 3-way Mirror and RAID-6 raid types.               \n\
Both drives with 520-bps and 512-bps are tested at all test levels.         \n\
There is only (1) LU bound per raid group.                                  \n\
\n"\
"\
Based on the raid test level and raid type the following error injection    \n\
tables are used: \n\
    o Table  0 - Correctable Errors, Injects Errors Always, Assorted Error Types        \n\
    o Table  1 - Correctable Errors, Injects Errors 15 Times, Hard Error Types          \n\
    o Table  3 - (RAID-6 only) Uncorrectable Errors, Injects Always, Hard Error Types   \n\
    o Table  5 - All types and combinations of correctable checksum errors              \n\
    o Table  7 - (RAID-6 only) RAID 6 pre-determined checksum error patterns            \n\
    o Table  8 - (RAID-6 only) RAID 6 random checksum errors (all 1 and 2 error combinations) \n\
\n"\
"\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Create provisioned drives and attach edges to logical drives.\n\
        - Create virtual drives and attach edges to provisioned drives.\n\
        - Create raid groups.\n\
        - Attach raid groups provisioned drives.\n\
        - Create and attach LU(s) to each raid group.\n\
\n"\
"\
STEP 2: Write a background pattern to the entire extent of each LU.\n\
\n"\
"\
STEP 3: Run the error injection test.  The default number of threads for each       \n\
        I/O size is typically (1). \n\
        - Configure random aborts if enabled. \n\
        - Enable error injection. \n\
        - Run random write/read/compare with large maximum I/O size (61440 blocks). \n\
        - Run random write/read/compare with small maximum I/O size (128 blocks).   \n\
        - Let I/O run for (5) seconds. \n\
        - Validate that the type and minimum number of errors have been encountered. \n\
        - If drive removal is enabled \n\
            + Remove the specified number of drives from each raid group. \n\
            + Let I/O run for (3) seconds. \n\
        - Let I/O run for the additional time specified. \n\
        - Wait for up to (120) seconds for the minimum number of errors. \n\
            + If the type of errors or minimum number of errors is not encountered. \n\
              the test fails. \n\
        - Get the error injection statistics. \n\
        - Validate that the error counters are as expected. \n\
        - Disable error injection. \n\
        - Stop I/O. \n\
        - If any drives were removed:               \n\
            + Re-insert the removed drives.         \n\
            + Wait for all rebuilds to complete.    \n\
        - Validate the I/O status. \n\
        - Validate no unexpected events (i.e. coherency errors). \n\
        - Destroy the logical error injection service configuration. \n\
\n"\
"\
STEP 4: If the raid test level is greater than 0: \n\
        - Write a background pattern to the entire extent of each LU.\n\
        - Repeat STEP 3 with random aborts enabled. \n\
        - Initiate and wait for a verify of each raid group to complete. \n\
\n"\
"\
STEP 4: Verify that there are no unexpected trace errors in any of the \n\
        packages. \n\
\n"\
"\
STEP 8: Destroy raid groups and LUNs    \n\
\n"\
"\
Test Specific Switches:  \n\
        -fbe_io_seconds (sec) - Extends the default I/O time.\n\
        -degraded_only        - Only run test cases that contain degraded drives.\n\
\n"\
"\
Outstanding Issues: \n\
        Currently Table 5 is disabled for RAID-5 and RAID-3 raid types due to   \n\
        defect DE50.  Abby Cadabby should be updated to allow certain errors to \n\
        be uncorrectable based on error type (i.e. invalidation errors are      \n\
        uncorrectable). \n\
\n"\
"\
Description Last Updated:   \n\
        10/14/2011          \n\
\n";

/* The number of LUNs in our raid group.
 */
#define ABBY_CADABBY_TEST_LUNS_PER_RAID_GROUP 1

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define ABBY_CADABBY_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/*!*******************************************************************
 * @def ABBY_CADABBY_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ABBY_CADABBY_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def ABBY_CADABBY_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with at a time.
 *
 *********************************************************************/
#define ABBY_CADABBY_MAX_RAID_GROUP_COUNT 10

/*!*******************************************************************
 * @def ABBY_CADABBY_NUM_IO_CONTEXT
 *********************************************************************
 * @brief  Two I/O threads. lg and sm.
 *
 *********************************************************************/
#define ABBY_CADABBY_NUM_IO_CONTEXT 2

/*!*******************************************************************
 * @def ABBY_CADABBY_NUM_IO_CONTEXT_DUAL_SP
 *********************************************************************
 * @brief  Two sets of threads per lun.
 *
 *********************************************************************/
#define ABBY_CADABBY_NUM_IO_CONTEXT_DUAL_SP \
(ABBY_CADABBY_TEST_LUNS_PER_RAID_GROUP * ABBY_CADABBY_MAX_RAID_GROUP_COUNT * 2)

/*!*******************************************************************
 * @def ABBY_CADABBY_MAX_IO_SECONDS
 *********************************************************************
 * @brief Max time to run I/O for.
 *
 *********************************************************************/
#define ABBY_CADABBY_MAX_IO_SECONDS 120

/*!*******************************************************************
 * @def ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of errors before
 *        continuing with the test.
 *
 *********************************************************************/
#define ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR 10


/*!*******************************************************************
 * @def ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of rdgen ios before
 *        continuing with the test.
 *
 *********************************************************************/
#define ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR   (ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) 

/*!*******************************************************************
 * @def ABBY_CADABBY_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of errors before
 *        continuing with the test.
 *
 *********************************************************************/
#define ABBY_CADABBY_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR 2

/*!*******************************************************************
 * @def ABBY_CADABBY_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define ABBY_CADABBY_VERIFY_WAIT_SECONDS 120

/*!*******************************************************************
 * @def ABBY_CADABBY_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ABBY_CADABBY_MAX_IO_SIZE_BLOCKS (ABBY_CADABBY_TEST_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @def ABBY_CADABBY_MAX_CONFIGS
 *********************************************************************
 * @brief The number of configurations we will test in extended mode.
 *
 *********************************************************************/
#define ABBY_CADABBY_MAX_CONFIGS 10
#define ABBY_CADABBY_MAX_CONFIGS_EXTENDED ABBY_CADABBY_MAX_CONFIGS

/*!*******************************************************************
 * @def ABBY_CADABBY_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define ABBY_CADABBY_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @var ABBY_CADABBY_MIRROR_LIBRARY_DEBUG_FLAGS
 *********************************************************************
 * @brief Default value of the raid library debug flags to set for mirror
 *
 *********************************************************************/
fbe_u32_t ABBY_CADABBY_MIRROR_LIBRARY_DEBUG_FLAGS = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );

/*!*******************************************************************
 * @var abby_cadabby_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t abby_cadabby_test_contexts[ABBY_CADABBY_NUM_IO_CONTEXT_DUAL_SP];

/*!*******************************************************************
 * @var abby_cadabby_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 *********************************************************************/
fbe_test_logical_error_table_test_configuration_t abby_cadabby_raid_groups_extended[] = 
{
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/

            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
            {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            4,          0},
            {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            5,          0},
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
            {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
            {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            8,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        3,          0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        4,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        5,          0},
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        9,          1}, 
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        10,         1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        11,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, 5, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        9,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        10,         0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        11,         0},
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          1},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        4,         1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        5,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, 0, 1, 3, 7, 8,  FBE_U32_MAX    /* Error tables. */ },
        { 1, 1, 0, 0, 0, 0, 0,  0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        3,         0},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        4,         0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        7,         1},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        8,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, 5, FBE_U32_MAX  /* Error tables. */},
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {    /* width,    capacity    raid type,                  class,              block size, raid group id,  bandwidth */
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        1,             0},
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        2,             0},
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        3,             0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX  /* Error tables. */ },
        { 0, 0, 0 /* Drives to power off for each error table */ },
    },
    {
        {    /* width,    capacity    raid type,                  class,              block size, raid group id,  bandwidth */
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        4,             0},
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        5,             0},
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        6,             0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, 0, 1, FBE_U32_MAX  /* Error tables. */ },
        { 0, 0, 1, 1, 0 /* Drives to power off for each error table */ },
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

fbe_test_logical_error_table_test_configuration_t abby_cadabby_raid_groups_qual[] = 
{
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,520,        7,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0, FBE_U32_MAX, /* Error tables. */},
        {0, 0 /* Drives to power off for each error table */ },
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        3,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        1,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        4,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR, 520,        16,         0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0, FBE_U32_MAX, /* Error tables. */},
        {0, 0 /* Drives to power off for each error table */ },
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

/*!*******************************************************************
 * @var abby_cadabby_table_info
 *********************************************************************
 * @brief These are all the parameters we pass to the test.
 *
 *********************************************************************/
fbe_test_logical_error_table_info_t abby_cadabby_table_info[] = 
{
    /* number of   b_random_errors  table   raid-6   number of
     * errors                               only?    records 
     * injected 
     */
    {1,            FBE_FALSE,        0,     FBE_FALSE, 128},
    {1,            FBE_FALSE,        1,     FBE_FALSE, 128},

    /* Raid 6 tables.
     */
    {1,            FBE_FALSE,        7,     FBE_TRUE,  47},
    {2,            FBE_FALSE,        8,     FBE_TRUE,  63},
    {3,            FBE_TRUE,         10,    FBE_TRUE,  105},
    {3,            FBE_TRUE,         12,    FBE_TRUE,  120},

    /* Uncorrectable tables.
     */
    {2,            FBE_FALSE,        3,     FBE_FALSE, 128},
    {3,            FBE_FALSE,        13,    FBE_FALSE, 192},

    /* This table is completely random and has both correctable and 
     * uncorrectable errors. 
     */
    {1,            FBE_FALSE,        2,     FBE_FALSE, 128},
    {2,            FBE_TRUE,         4,     FBE_FALSE, 16},

    /* Table 5 injects coherency errors, so set number of errors to 2 to allow rdgen to continue on uncorrectable */
    {2,            FBE_FALSE,        5,     FBE_FALSE, 128},
    {3,            FBE_FALSE,        6,     FBE_FALSE, 70},

    /* Terminator.
     */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};

/*!**************************************************************
 * abby_cadabby_wait_for_raid_group_verify()
 ****************************************************************
 * @brief
 *  Wait for the raid group verify chunk count to goto zero.
 *
 * @param rg_object_id - Object id to wait on
 * @param wait_seconds - max seconds to wait
 * 
 * @return fbe_status_t FBE_STATUS_OK if flags match the expected value
 *                      FBE_STATUS_TIMEOUT if exceeded max wait time.
 *
 * @author
 *  2/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t abby_cadabby_wait_for_raid_group_verify(fbe_object_id_t rg_object_id,
                                                     fbe_u32_t wait_seconds)
{    
    return fbe_test_sep_util_wait_for_raid_group_verify(rg_object_id, wait_seconds);
}
/******************************************
 * end abby_cadabby_wait_for_raid_group_verify()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_wait_for_verifies()
 ****************************************************************
 * @brief
 *  Wait for all verifies to finish.
 *
 * @param rg_config_p - current configuration.
 * @param raid_group_count - total entries in above config_p array.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t abby_cadabby_wait_for_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t raid_group_count)
{
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    fbe_u32_t index; /* allow for lots of objects */
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);


    /* Loop over all raid groups, waiting for the raid group verify to finish on each.
     */
    for ( index = 0; index < rg_array_length; index++)
    {
        if (! fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* Wait for the verify on each mirrored pair.
             */
            for (index = 0; 
                  index < downstream_object_list.number_of_downstream_objects; 
                  index++)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for verifies to finish rg: %d (rgid: 0x%x)==", 
                           __FUNCTION__, rg_config_p->raid_group_id, downstream_object_list.downstream_object_list[index]);
                status = abby_cadabby_wait_for_raid_group_verify(downstream_object_list.downstream_object_list[index], 
                                                                 ABBY_CADABBY_VERIFY_WAIT_SECONDS);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for any verifies to finish rg: %d (rgid: 0x%x)==", 
                       __FUNCTION__, rg_config_p->raid_group_id, rg_object_id);
            status = abby_cadabby_wait_for_raid_group_verify(rg_object_id, ABBY_CADABBY_VERIFY_WAIT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        rg_config_p++;
    }
    return status;
}
/******************************************
 * end abby_cadabby_wait_for_verifies()
 ******************************************/
/*!**************************************************************
 * abby_cadabby_get_config_count()
 ****************************************************************
 * @brief
 *  Get the number of configs we will be working with.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of configs.               
 *
 * @return fbe_u32_t number of configs.   
 *
 * @author
 *  5/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t abby_cadabby_get_config_count(fbe_test_logical_error_table_test_configuration_t *config_p)
{
    fbe_u32_t config_count = 0;
    /* Loop until we find the table number or we hit the terminator.
     */
    while (config_p->raid_group_config[0].width != FBE_U32_MAX)
    {
        config_count++;
        config_p++;
    }
    return config_count;
}
/******************************************
 * end abby_cadabby_get_config_count()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param config_p - Current config.
 *
 * @return None.
 *
 * @author
 *  8/23/2010 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_load_physical_config(fbe_test_logical_error_table_test_configuration_t *config_p)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t current_num_520_drives = 0;
    fbe_u32_t current_num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;
    fbe_u32_t current_520_extra_drives = 0;
    fbe_u32_t current_4160_extra_drives = 0;

    fbe_u32_t config_count = abby_cadabby_get_config_count(config_p);

    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        /* First get the count of drives.
         */
        fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                           &current_num_520_drives,
                                                           &current_num_4160_drives);
        num_520_drives = FBE_MAX(num_520_drives, current_num_520_drives);
        num_4160_drives = FBE_MAX(num_4160_drives, current_num_4160_drives);

        /* consider extra disk count into account while create physical config
         */
        fbe_test_sep_util_get_rg_num_extra_drives(rg_config_p, raid_group_count,
                                              &current_520_extra_drives,
                                              &current_4160_extra_drives);

        num_520_extra_drives = FBE_MAX(num_520_extra_drives, current_520_extra_drives);
        num_4160_extra_drives = FBE_MAX(num_4160_extra_drives, current_4160_extra_drives);

        mut_printf(MUT_LOG_TEST_STATUS, "%s: %s %d 520 drives and %d 4160 drives", 
                   __FUNCTION__, raid_type_string_p, current_num_520_drives, current_num_4160_drives);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
        if (raid_group_count == 0)
        {
            continue;
        }
        fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, 
                                            num_520_drives, num_4160_drives);
    }

    num_520_drives += num_520_extra_drives;
    num_4160_drives += num_4160_extra_drives;
    
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
    return;
}
/******************************************
 * end abby_cadabby_load_physical_config()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_load_physical_config_raid_type()
 ****************************************************************
 * @brief
 *  Configure the physical config for a given raid type.
 *
 * @param config_p - Current config.
 * @param raid_type - Raid type to configure.
 *
 * @return None.
 *
 * @author
 *  5/7/2012 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_load_physical_config_raid_type(fbe_test_logical_error_table_test_configuration_t *config_p,
                                                 fbe_raid_group_type_t raid_type)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t current_num_520_drives = 0;
    fbe_u32_t current_num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;
    fbe_u32_t current_520_extra_drives = 0;
    fbe_u32_t current_4160_extra_drives = 0;

    fbe_u32_t config_count = abby_cadabby_get_config_count(config_p);

    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if ((raid_type != rg_config_p->raid_type) ||
            !fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
           continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        /* First get the count of drives.
         */
        fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                           &current_num_520_drives,
                                                           &current_num_4160_drives);
        num_520_drives = FBE_MAX(num_520_drives, current_num_520_drives);
        num_4160_drives = FBE_MAX(num_4160_drives, current_num_4160_drives);

        /* consider extra disk count into account while create physical config
         */
        fbe_test_sep_util_get_rg_num_extra_drives(rg_config_p, raid_group_count,
                                              &current_520_extra_drives,
                                              &current_4160_extra_drives);

        num_520_extra_drives = FBE_MAX(num_520_extra_drives, current_520_extra_drives);
        num_4160_extra_drives = FBE_MAX(num_4160_extra_drives, current_4160_extra_drives);

        mut_printf(MUT_LOG_TEST_STATUS, "%s: %s %d 520 drives and %d 4160 drives", 
                   __FUNCTION__, raid_type_string_p, current_num_520_drives, current_num_4160_drives);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];
        if ((raid_type != rg_config_p->raid_type) ||
            !fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            continue;
        }
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
        if (raid_group_count == 0)
        {
            continue;
        }
        fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, 
                                            num_520_drives, num_4160_drives);
    }

    num_520_drives += num_520_extra_drives;
    num_4160_drives += num_4160_extra_drives;
    
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);
    return;
}
/******************************************
 * end abby_cadabby_load_physical_config_raid_type()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_get_max_correctable_errors()
 ****************************************************************
 * @brief
 *  Get the max number of errors this raid type can correct.
 *
 * @param raid_type - type to check for.
 * @param width - Needed to determine how many surfaces to mirror
 * @param max_correctable_errors_p - The max number of adjacent
 *                                   errors that can be fixed.
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE otherwise
 *
 * @author
 *  2/11/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t abby_cadabby_get_max_correctable_errors(fbe_raid_group_type_t raid_group_type,
                                                     fbe_u32_t  width,
                                                     fbe_u32_t *max_correctable_errors_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (raid_group_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
            *max_correctable_errors_p = 0; 
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
            *max_correctable_errors_p = 1; 
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            *max_correctable_errors_p = 2; 
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            switch(width)
            {
                case 3:
                    *max_correctable_errors_p = 2;
                    break;
                case 2:
                    *max_correctable_errors_p = 1;
                    break;
                default:
                    *max_correctable_errors_p = 0; 
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
            }
            break;
        default:
            *max_correctable_errors_p = 0; 
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return status;
}
/******************************************
 * end abby_cadabby_get_max_correctable_errors()
 ******************************************/
/*!***************************************************************************
 * abby_cadabby_rg_get_error_totals_both_sps()
 *****************************************************************************
 * @brief
 *  For a given config find the smallest number of errors injected
 *  by the tool.
 *
 * @param rg_config_p - Configuration we are testing against.  
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/4/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t abby_cadabby_rg_get_error_totals_both_sps(fbe_object_id_t object_id,
                                                       fbe_api_logical_error_injection_get_object_stats_t *stats_p)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_object_stats_t stats_spb;

    status = fbe_api_logical_error_injection_get_object_stats(stats_p, object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_logical_error_injection_get_object_stats(&stats_spb, object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        stats_p->num_errors_injected += stats_spb.num_errors_injected;
        stats_p->num_read_media_errors_injected += stats_spb.num_read_media_errors_injected;
        stats_p->num_validations += stats_spb.num_validations;
        stats_p->num_write_verify_blocks_remapped += stats_spb.num_write_verify_blocks_remapped;

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    return status;
}
/**************************************
 * end abby_cadabby_rg_get_error_totals_both_sps
 **************************************/
/*!***************************************************************************
 * abby_cadabby_rg_get_error_totals()
 *****************************************************************************
 * @brief
 *  For a given config find the smallest number of errors injected
 *  by the tool.
 *
 * @param rg_config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above rg_config_p array.
 * @param min_validations_p - min number of validations across all
 *                            the raid groups under test.
 * @param min_count_p - min number of errors injected across all
 *                            the raid groups under test.
 * @param max_count_p - max number of errors injected across all
 *                            the raid groups under test.
 * @param min_validations_object_id_p - Address of object id to update with the
 *                      object that had the minimum number of error validations.
 * @param min_count_object_id_p - Address of object id to update with the object
 *                      that had the minimum number of errors injected.
 * @param max_count_object_id_p - Address of object id to update with the object
 *                      that had the maximum number of errors injected.
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/2/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t abby_cadabby_rg_get_error_totals(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_u64_t *min_validations_p,
                                              fbe_u64_t *min_count_p,
                                              fbe_u64_t *max_count_p,
                                              fbe_object_id_t *min_validations_object_id_p,
                                              fbe_object_id_t *min_count_object_id_p,
                                              fbe_object_id_t *max_count_object_id_p)
{
    fbe_object_id_t rg_object_id;
    fbe_status_t status;
    fbe_u64_t min_validations = *min_validations_p;
    fbe_u64_t min_count = *min_count_p;
    fbe_u64_t max_count = *max_count_p;
    fbe_api_logical_error_injection_get_object_stats_t stats;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* Raid-10s need to get the stats from each mirror. 
         */
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;

        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* Get the stats for each mirror.
         */
        for (index = 0; 
            index < downstream_object_list.number_of_downstream_objects; 
            index++)
        {
            status = abby_cadabby_rg_get_error_totals_both_sps(downstream_object_list.downstream_object_list[index], &stats);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if (stats.num_validations < min_validations)
            {
                min_validations = stats.num_validations;
                *min_validations_object_id_p = downstream_object_list.downstream_object_list[index];
            }
            if (stats.num_errors_injected < min_count)
            {
                min_count = stats.num_errors_injected;
                *min_count_object_id_p = downstream_object_list.downstream_object_list[index];
            }
            if (stats.num_errors_injected > max_count)
            {
                max_count = stats.num_errors_injected;
                *max_count_object_id_p = downstream_object_list.downstream_object_list[index];
            }
        }
    }
    else
    {
        /* Get the stats for this object.
         */
        status = abby_cadabby_rg_get_error_totals_both_sps(rg_object_id, &stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (stats.num_validations < min_validations)
        {
            min_validations = stats.num_validations;
            *min_validations_object_id_p = rg_object_id;
        }
        if (stats.num_errors_injected < min_count)
        {
            min_count = stats.num_errors_injected;
            *min_count_object_id_p = rg_object_id;
        }
        if (stats.num_errors_injected > max_count)
        {
            max_count = stats.num_errors_injected;
            *max_count_object_id_p = rg_object_id;
        }
    }
    /* Set the values being returned.
     */
    *min_validations_p = min_validations;
    *min_count_p = min_count;
    *max_count_p = max_count;

    return status;
}
/***********************************************
 * end abby_cadabby_rg_get_error_totals()
 ***********************************************/
/*!***************************************************************************
 * abby_cadabby_get_error_totals_for_config()
 *****************************************************************************
 * @brief
 *  For a given config find the smallest number of errors injected
 *  by the tool.
 *
 * @param rg_config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above rg_config_p array.
 * @param min_validations_p - min number of validations across all
 *                            the raid groups under test.
 * @param min_count_p - min number of errors injected across all
 *                            the raid groups under test.
 * @param max_count_p - max number of errors injected across all
 *                            the raid groups under test.
 * @param min_validations_object_id_p - Address of object id to update with the
 *                      object that had the minimum number of error validations.
 * @param min_count_object_id_p - Address of object id to update with the object
 *                      that had the minimum number of errors injected.
 * @param max_count_object_id_p - Address of object id to update with the object
 *                      that had the maximum number of errors injected.
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/7/2010 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t abby_cadabby_get_error_totals_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t raid_group_count,
                                                      fbe_u64_t *min_validations_p,
                                                      fbe_u64_t *min_count_p,
                                                      fbe_u64_t *max_count_p,
                                                      fbe_object_id_t *min_validations_object_id_p,
                                                      fbe_object_id_t *min_count_object_id_p,
                                                      fbe_object_id_t *max_count_object_id_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);

    *min_validations_p = FBE_U32_MAX;
    *min_count_p = FBE_U32_MAX; 
    *max_count_p = 0;
    *min_validations_object_id_p = FBE_OBJECT_ID_INVALID;
    *min_count_object_id_p = FBE_OBJECT_ID_INVALID;
    *max_count_object_id_p = FBE_OBJECT_ID_INVALID;

    /* Get stats for all raid groups.  Accumulate the max and min for values.
     */
    for (rg_index = 0; rg_index < rg_array_length; rg_index++)
    {
        if (! fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p++;
            continue;
        }
        status = abby_cadabby_rg_get_error_totals(rg_config_p, min_validations_p, min_count_p, max_count_p,
                                                  min_validations_object_id_p, min_count_object_id_p, max_count_object_id_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        rg_config_p++;
    }
    return status;
}
/***********************************************
 * end abby_cadabby_get_error_totals_for_config()
 ***********************************************/

/*!***************************************************************************
 *          abby_cadabby_wait_timed_out()
 *****************************************************************************
 *
 * @brief   We timed out waiting for sufficient errors to be injected and/or
 *          detected.
 *
 * @param   test_params_p - type of test to run.
 * @param   elapsed_msec - Elapsed time in milliseconds
 * @param   raid_type_max_correctable_errors - Maximum correctable errors for
 *                      this raid type
 * @param   stats_p - Pointer to logical error injection statistics
 * @param   rdgen_stats_p - Pointer to rdgen statistics
 * @param   min_object_validations - Minimum number of validations
 * @param   min_object_injected_errors - Minumum errors injected per object
 * @param   max_object_injected_errors - Maximum errors injected for object
 * @param   min_validations_object_id - Object id with minimum validations
 * @param   min_count_object_id - Object id with minimum errors injected
 * @param   max_count_object_id - Object id with maximum errors injected
 *
 * @return  fbe_status_t
 *
 * @author
 *  10/03/2011  Ron Proulx  - Created
 *
 ****************************************************************/

fbe_status_t abby_cadabby_wait_timed_out(fbe_test_logical_error_table_info_t *test_params_p,
                                         fbe_u32_t elapsed_msec,
                                         fbe_u32_t raid_type_max_correctable_errors,
                                         fbe_api_logical_error_injection_get_stats_t *stats_p,
                                         fbe_api_rdgen_get_stats_t *rdgen_stats_p,
                                         fbe_u64_t min_object_validations,
                                         fbe_u64_t min_object_injected_errors,
                                         fbe_u64_t max_object_injected_errors,
                                         fbe_object_id_t min_validations_object_id,
                                         fbe_object_id_t min_count_object_id,
                                         fbe_object_id_t max_count_object_id)
{
    fbe_status_t status = FBE_STATUS_TIMEOUT;

    /* Common status messages
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Did not find minimum number of errors: %d after: %d ms ",
               __FUNCTION__, ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR, elapsed_msec);
    mut_printf(MUT_LOG_TEST_STATUS, "%s rdgen io count: 0x%x rdgen error count: 0x%x",
               __FUNCTION__, rdgen_stats_p->io_count, rdgen_stats_p->error_count);
    mut_printf(MUT_LOG_TEST_STATUS, "%s found min errors: %llu for object id: 0x%x",
               __FUNCTION__, (unsigned long long)min_object_injected_errors, min_count_object_id);  
    mut_printf(MUT_LOG_TEST_STATUS, "%s found min validations: %llu for object id: 0x%x",
               __FUNCTION__, (unsigned long long)min_object_validations, min_validations_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "%s found max errors %llud for object id: 0x%x",
               __FUNCTION__, (unsigned long long)max_object_injected_errors, max_count_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "%s errors injected: %llud correctables: %llud uncorrectables: %llud",
               __FUNCTION__, (unsigned long long)stats_p->num_errors_injected,
	       (unsigned long long)stats_p->correctable_errors_detected,
	       (unsigned long long)stats_p->uncorrectable_errors_detected);

    /* Based on the type of errors expected, report the errors
     */
    if ((test_params_p->number_of_errors <= raid_type_max_correctable_errors) &&
        (test_params_p->b_random_errors == FBE_FALSE))
    {
        /* We injected errors that should be corectable for this raid type.
         * Simply check to see that we injected a reasonable number of errors.
         */
        if (rdgen_stats_p->io_count < ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of ios detected by rdgen: 0x%x was less than expected: 0x%x",
                       __FUNCTION__, rdgen_stats_p->io_count, ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR);
            /* Just blame the lowest object id
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (stats_p->num_errors_injected < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of errors injected: %llu was less than expected: %d",
                       __FUNCTION__,
		      (unsigned long long)stats_p->num_errors_injected,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR);
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (stats_p->correctable_errors_detected < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of correctable errors: %llu was less than expected: %d",
                       __FUNCTION__,
		      (unsigned long long)stats_p->correctable_errors_detected,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR);
            /*! @todo Should be the minium correctable errors object id.
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (min_object_injected_errors < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Min errors injected: %llu was less than expected: %d for rg obj: 0x%x",
                       __FUNCTION__,
		      (unsigned long long)min_object_injected_errors,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR,
		      min_count_object_id);
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (min_object_validations < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Min errors validations: %llu was less than expected: %d for rg obj: 0x%x",
                       __FUNCTION__, (unsigned long long)min_object_validations,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR,
		      min_validations_object_id);
            fbe_test_sep_util_raid_group_generate_error_trace(min_validations_object_id);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected validation error. total errors injected: %llu",
                       __FUNCTION__, (unsigned long long)stats_p->num_errors_injected);
            /* Else just blame the minimum number of errors injected.
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
    }
    else if ((test_params_p->number_of_errors > raid_type_max_correctable_errors) &&
             (test_params_p->b_random_errors == FBE_FALSE))
    {
        /* Should be uncorrectable, check to see that enough uncorrectable errors were
         * injected. 
         */
        if (rdgen_stats_p->io_count < ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of ios detected by rdgen: 0x%x was less than expected: 0x%x",
                       __FUNCTION__, rdgen_stats_p->io_count, ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR);
            /* Just blame the lowest object id
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (stats_p->num_errors_injected < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of errors injected: %llu was less than expected: %d",
                       __FUNCTION__,
		       (unsigned long long)stats_p->num_errors_injected,
		       ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR);
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (stats_p->uncorrectable_errors_detected < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of uncorrectable errors: %llu was less than expected: %d",
                       __FUNCTION__,
		      (unsigned long long)stats_p->uncorrectable_errors_detected,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR);
            /*! @todo Should be the minium uncorrectable errors object id.
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (rdgen_stats_p->error_count < ABBY_CADABBY_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of failed ios detected by rdgen: 0x%x was less than expected: 0x%x",
                       __FUNCTION__, rdgen_stats_p->error_count, ABBY_CADABBY_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR);
            /* Just blame the lowest object id
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (min_object_injected_errors < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Min errors injected: %llu was less than expected: %d for rg obj: 0x%x",
                       __FUNCTION__,
		      (unsigned long long)min_object_injected_errors,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR,
		      min_count_object_id);
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected validation error. total errors injected: %llu",
                       __FUNCTION__, (unsigned long long)stats_p->num_errors_injected);
            /* Else just blame the minimum number of errors injected.
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
    }
    else if (test_params_p->b_random_errors)
    {
        /* Should be uncorrectable, check to see that enough uncorrectable errors were
         * injected. 
         */
        if (rdgen_stats_p->io_count < ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of ios detected by rdgen: 0x%x was less than expected: 0x%x",
                       __FUNCTION__, rdgen_stats_p->io_count, ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR);
            /* Just blame the lowest object id
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (stats_p->num_errors_injected < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Number of errors injected: %llu was less than expected: %d",
                       __FUNCTION__,
		       (unsigned long long)stats_p->num_errors_injected,
		       ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR);
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (min_object_injected_errors < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Min errors injected: %llu was less than expected: %d for rg obj: 0x%x",
                       __FUNCTION__,
		      (unsigned long long)min_object_injected_errors,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR,
		      min_count_object_id);
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }
        else if (stats_p->num_validations < ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Min errors validations: %llu was less than expected: %d for rg obj: 0x%x",
                       __FUNCTION__,
		      (unsigned long long)min_object_validations,
		      ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR,
		      min_validations_object_id);
            fbe_test_sep_util_raid_group_generate_error_trace(min_validations_object_id);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected validation error. total errors injected: %llu",
                       __FUNCTION__, (unsigned long long)stats_p->num_errors_injected);
            /* Else just blame the minimum number of errors injected.
             */
            fbe_test_sep_util_raid_group_generate_error_trace(min_count_object_id);
        }

    } /* end based on type of errors injected */

    /* Now fail
     */
    MUT_FAIL_MSG("errors not found as expected prior to timeout");

    return status;
}
/***********************************************
 * end of abby_cadabby_wait_timed_out()
 ***********************************************/

/*!**************************************************************
 * abby_cadabby_wait_for_errors()
 ****************************************************************
 * @brief
 *  Wait until enough errors were injected.
 *
 * @param test_params_p - type of test to run.
 * @param config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above config_p array.
 * @param max_wait_seconds - # of seconds we will wait before assert.          
 *
 * @return fbe_status_t
 *
 * @author
 *  2/10/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t abby_cadabby_wait_for_errors(fbe_test_logical_error_table_info_t *test_params_p,
                                          fbe_test_rg_configuration_t *config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t max_wait_seconds,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = max_wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t raid_type_max_correctable_errors;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;
    fbe_u64_t min_object_validations;
    fbe_u64_t min_object_injected_errors;
    fbe_u64_t max_object_injected_errors;
    fbe_object_id_t min_validations_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t min_count_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t max_count_object_id = FBE_OBJECT_ID_INVALID;


    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID, 0 /* edge index */);
    
    status = abby_cadabby_get_max_correctable_errors(config_p->raid_type, 
                                                     config_p->width,
                                                     &raid_type_max_correctable_errors);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec <= target_wait_msec)
    {
        /* Get a snapshot of how many rdgen ios have been sent.
         * This only needs to get issued to the A side. 
         */
        status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Get a snapshot of what errors have been injected so far.
         */
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = abby_cadabby_get_error_totals_for_config(config_p, raid_group_count,
                                                          &min_object_validations, 
                                                          &min_object_injected_errors,
                                                          &max_object_injected_errors,
                                                          &min_validations_object_id,
                                                          &min_count_object_id,
                                                          &max_count_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        if ((test_params_p->number_of_errors <= raid_type_max_correctable_errors) &&
            (test_params_p->b_random_errors == FBE_FALSE))
        {
            /* We injected errors that should be correctable for this raid type.
             * Simply check to see that we injected a reasonable number of errors.
             */
            if ((rdgen_stats.io_count >= ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR) &&
                (stats.num_errors_injected >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (stats.correctable_errors_detected >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (min_object_injected_errors >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (min_object_validations >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s %d msec rdgen ios: 0x%x rdgen errors: 0x%x",
                           __FUNCTION__, elapsed_msec, rdgen_stats.io_count, rdgen_stats.error_count);
                mut_printf(MUT_LOG_TEST_STATUS, "found min errors: %llu and max errors %llu min validations: %llu",
                           (unsigned long long)min_object_injected_errors,
			   (unsigned long long)max_object_injected_errors,
			   (unsigned long long)min_object_validations);
                mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %llu correctables: %llu uncorrectables: %llu",
			   (unsigned long long)stats.num_errors_injected,
			   (unsigned long long)stats.correctable_errors_detected,
			   (unsigned long long)stats.uncorrectable_errors_detected);
                break;
            }
        }
        else if ((test_params_p->number_of_errors > raid_type_max_correctable_errors) &&
                 (test_params_p->b_random_errors == FBE_FALSE))
        {
            /* Should be uncorrectable, check to see that enough uncorrectable errors were
             * injected. 
             */
            if ((rdgen_stats.io_count >= ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR) &&
                (stats.num_errors_injected >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (stats.uncorrectable_errors_detected >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (rdgen_stats.error_count >= ABBY_CADABBY_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR) &&
                (min_object_injected_errors >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s %d msec rdgen ios: 0x%x rdgen errors: 0x%x",
                           __FUNCTION__, elapsed_msec, rdgen_stats.io_count, rdgen_stats.error_count);
                mut_printf(MUT_LOG_TEST_STATUS, "found min errors: %llu and max errors %llu min validations: %llu",
                           (unsigned long long)min_object_injected_errors,
			   (unsigned long long)max_object_injected_errors,
			   (unsigned long long)min_object_validations);
                mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %llu correctables: %llu uncorrectables: %llu",
			   (unsigned long long)stats.num_errors_injected,
			   (unsigned long long)stats.correctable_errors_detected,
			   (unsigned long long)stats.uncorrectable_errors_detected);
                break;
            }
        }
        else if (test_params_p->b_random_errors)
        {
            /* Should be uncorrectable, check to see that enough uncorrectable errors were
             * injected. 
             */
            if ((rdgen_stats.io_count >= ABBY_CADABBY_MIN_RDGEN_COUNT_TO_WAIT_FOR)      &&
                (stats.num_errors_injected >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (min_object_injected_errors >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (stats.num_validations >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s %d msec rdgen ios: 0x%x rdgen errors: 0x%x",
                           __FUNCTION__, elapsed_msec, rdgen_stats.io_count, rdgen_stats.error_count);
                mut_printf(MUT_LOG_TEST_STATUS, "found min errors: %llu and max errors %llu min validations: %llu",
			   (unsigned long long)min_object_injected_errors,
			   (unsigned long long)max_object_injected_errors,
			   (unsigned long long)min_object_validations);
                mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %llu correctables: %llu uncorrectables: %llu",
			   (unsigned long long)stats.num_errors_injected,
			   (unsigned long long)stats.correctable_errors_detected,
			   (unsigned long long)stats.uncorrectable_errors_detected);
                break;
            }
        }
        fbe_api_sleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure.
     */
    if(ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        /* it is possible that we will not get the errors if we have aborted the I/Os randomly
         */
        mut_printf(MUT_LOG_TEST_STATUS, "random aborts enabled : %d msecs", ran_abort_msecs);
        mut_printf(MUT_LOG_TEST_STATUS, "%s found min errors: %llu and max errors %llu min validations: %llu",
                   __FUNCTION__,
		   (unsigned long long)min_object_injected_errors,
		   (unsigned long long)max_object_injected_errors,
		   (unsigned long long)min_object_validations);
        mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %llu correctables: %llu uncorrectables: %llu",
		   (unsigned long long)stats.num_errors_injected,
		   (unsigned long long)stats.correctable_errors_detected,
		   (unsigned long long)stats.uncorrectable_errors_detected);
        mut_printf(MUT_LOG_TEST_STATUS, "rdgen_error_count: %d", rdgen_stats.error_count);
    }
    else if (elapsed_msec > target_wait_msec)
    {
        /* Invoke method that will give detailed information about the failure and
         * generate an `error trace' on the appropriate SP.
         */
        status = abby_cadabby_wait_timed_out(test_params_p,
                                             elapsed_msec,
                                             raid_type_max_correctable_errors,
                                             &stats,
                                             &rdgen_stats,
                                             min_object_validations,
                                             min_object_injected_errors,
                                             max_object_injected_errors,
                                             min_validations_object_id,
                                             min_count_object_id,
                                             max_count_object_id);
    }

    return status;
}
/**************************************
 * end abby_cadabby_wait_for_errors()
 **************************************/
/*!**************************************************************
 * abby_cadabby_enable_error_injection_for_raid_groups()
 ****************************************************************
 * @brief
 *  For a given config enable injection.
 *
 * @param rg_config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above rg_config_p array.
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/13/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t abby_cadabby_enable_error_injection_for_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);

    /* Enable all raid groups.
     */
    for(rg_index=0;rg_index<rg_array_length;rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(rg_config_p)){      
            rg_config_p++;
            continue;
        }
    
        /* First get the raid group object id.
         */
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Raid 10s need to enable on each of the mirrors.
         */
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_u32_t index;
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* Now enable injection for each mirror.
             */
            for (index = 0; 
                  index < downstream_object_list.number_of_downstream_objects; 
                  index++)
            {
               status = fbe_api_logical_error_injection_enable_object(downstream_object_list.downstream_object_list[index], 
                                                                      FBE_PACKAGE_ID_SEP_0);
               MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
        else
        {
            status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        
        rg_config_p++;
    }
    return status;
}
/******************************************
 * end abby_cadabby_enable_error_injection_for_raid_groups()
 ******************************************/
/*!**************************************************************
 *          abby_cadabby_set_debug_flags()
 ****************************************************************
 *
 * @brief   Set any debug flags
 *
 * @param   config_p - Multi raid group configuration pointer   
 * @param raid_group_count - total entries in above config_p array.        
 *
 * @return  None.
 *
 * @author
 * 05/03/2010   Ron Proulx - Created
 *
 ****************************************************************/
void abby_cadabby_set_debug_flags(fbe_test_rg_configuration_t *rg_config_p,
                                  fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags = 0;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);

    /* Populate the raid library debug flags to the value desired
     * (from the -debug_flags command line parameter)
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(0);

    /* For all raid groups in this configuration.
     */
    for (rg_index = 0; 
        (raid_library_debug_flags != 0) && (rg_index < rg_array_length); 
        rg_index++)
    {
        if (! fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p++;
            continue;
        }
        /* Set the debug flags for raid groups that we are interested in.
         */
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Set the raid library debug flags
         */
        status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                            raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_MEDIUM, "== %s set raid library debug flags to 0x%08x for rg_object_id: %d ==", 
                   __FUNCTION__, raid_library_debug_flags, rg_object_id);
        rg_config_p++;
    }
    
    return;
}
/**************************************
 * end abby_cadabby_set_debug_flags()
 **************************************/

/*!**************************************************************
 *          abby_cadabby_clear_debug_flags()
 ****************************************************************
 *
 * @brief   Clear any debug flags
 *
 * @param   config_p - Multi raid group configuration pointer     
 * @param raid_group_count - total entries in above config_p array.      
 *
 * @return  None.
 *
 * @author
 * 05/03/2010   Ron Proulx - Created
 *
 ****************************************************************/
void abby_cadabby_clear_debug_flags(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags = 0;
    fbe_u32_t                   rg_index;
    fbe_object_id_t             rg_object_id;
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);

    /* For all raid groups in this configuration.
     */
    for (rg_index = 0; 
          (rg_index < rg_array_length); 
          rg_index++)
    {
        if (! fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p++;
            continue;
        }

        /* Clearthe debug flags for raid groups that we are interested in.
         */
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Set the raid library debug flags to 0.
         */
        status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, raid_library_debug_flags);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        rg_config_p++;
    }
    
    return;
}
/**************************************
 * end abby_cadabby_clear_debug_flags()
 **************************************/
/*!**************************************************************
 * abby_cadabby_sum_error_counts()
 ****************************************************************
 * @brief
 *  Sum up all the error counts for our rdgen contexts.
 *
 * @param context_p - ptr to array of rdgen contexts.            
 *
 * @return none   
 *
 * @author
 *  5/17/2010 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_sum_error_counts(fbe_api_rdgen_context_t *context_p, fbe_u32_t num_contexts)
{
    fbe_u32_t index;

    for (index = 0; index < num_contexts; index++)
    {
        context_p[0].start_io.statistics.aborted_error_count += context_p[index].start_io.statistics.aborted_error_count; 
        context_p[0].start_io.statistics.error_count += context_p[index].start_io.statistics.error_count;
        context_p[0].start_io.statistics.inv_blocks_count += context_p[index].start_io.statistics.inv_blocks_count;
        context_p[0].start_io.statistics.io_count += context_p[index].start_io.statistics.io_count;
        context_p[0].start_io.statistics.io_failure_error_count += context_p[index].start_io.statistics.io_failure_error_count;
        context_p[0].start_io.statistics.media_error_count += context_p[index].start_io.statistics.media_error_count;
        context_p[0].start_io.statistics.pass_count += context_p[index].start_io.statistics.pass_count;
    }
    return;
}
/******************************************
 * end abby_cadabby_sum_error_counts()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_get_total_raid_group_count()
 ****************************************************************
 * @brief
 *  Determine how many total raid group objects we should
 *  be injecting errors on.  For raid 10s we simply
 *  count all the mirror objects.
 *
 * @param config_p - Configuration we are testing against.
 * @param raid_group_count - total entries in above config_p array.
 *
 * @return None.
 *
 * @author
 *  7/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t abby_cadabby_get_total_raid_group_count(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_u32_t raid_group_count)
{
    fbe_u32_t total_raid_groups = 0;
    fbe_u32_t index = 0;
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);

    /* Loop until we find the table number or we hit the terminator.
     */
    for (index=0; index<rg_array_length; index++)
    {
        if ( fbe_test_rg_config_is_enabled(rg_config_p))
        {
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                /* Add on all the mirrored pairs.
                 */
                total_raid_groups += rg_config_p->width / 2;
            }
            else
            {
                total_raid_groups++;
            }
        }
        rg_config_p++;
    }
    return total_raid_groups;
}
/******************************************
 * end abby_cadabby_get_total_raid_group_count()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_validate_event_log_errors()
 ****************************************************************
 * @brief
 *  Check that the event log errors that occurred are appropriate.
 *
 * @param None.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  10/25/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t abby_cadabby_validate_event_log_errors(void)
{
    fbe_u32_t count = 0;
    fbe_status_t status;

    /* We should not see any coherency errors.
     */
    status = fbe_test_utils_get_event_log_error_count(SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR, &count);
    MUT_ASSERT_INT_EQUAL(count, 0);
    return status;
}
/******************************************
 * end abby_cadabby_validate_event_log_errors()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_enable_injection()
 ****************************************************************
 * @brief
 *  Enable error injection on this configuration.
 *
 * @param test_params_p - type of test to run.
 * @param rg_config_p - Configuration we are testing against.
 * @param raid_group_count - total entries in above config_p array.
 *
 * @return None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
void abby_cadabby_enable_injection(fbe_test_logical_error_table_info_t *test_params_p,
                                   fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_api_logical_error_injection_get_stats_t stats;

    /* This includes striper and mirrors for raid 10s.
     */
    fbe_u32_t total_raid_group_count = abby_cadabby_get_total_raid_group_count(rg_config_p, raid_group_count);
    /* Clear the logs.
     */
    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    /* Next enable logical error injection with table_num.
     */
    status = fbe_api_logical_error_injection_load_table(test_params_p->table_number, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection for table %d==", __FUNCTION__, test_params_p->table_number);
    
    status = abby_cadabby_enable_error_injection_for_raid_groups(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, test_params_p->record_count);

    /* We use * 2 since we have 520 and 512.
     */
    MUT_ASSERT_INT_EQUAL(stats.num_objects, total_raid_group_count);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, total_raid_group_count);
    return;
}
/**************************************
 * end abby_cadabby_enable_injection()
 **************************************/
/*!**************************************************************
 * abby_cadabby_validate_io_status()
 ****************************************************************
 * @brief
 *  Validate the io status is correct for the given test config.
 *
 * @param test_params_p - type of test to run.
 * @param rg_config_p - Configuration we are testing against.
 * @param raid_group_count - total entries in above config_p array.
 * @param context_p - Pointer to array of rdgen contexts.
 * @param num_contexts - Number of valid rdgen contexts.
 * @param drives_to_power_off - Number of drives to turn off
 *                              while we are testing.
 * @param ran_abort_msecs - If not FBE_TEST_RANDOM_ABORT_TIME_INVALID,
 *                          then this is the number of seconds rdgen
 *                          should wait before rdgen aborts IO.
 *
 * @return None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
void abby_cadabby_validate_io_status(fbe_test_logical_error_table_info_t *test_params_p,
                                     fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_api_rdgen_context_t *context_p,
                                     fbe_u32_t num_contexts,
                                     fbe_u32_t drives_to_power_off,
                                     fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t status;
    fbe_u32_t raid_type_max_correctable_errors;

    status = abby_cadabby_get_max_correctable_errors(rg_config_p->raid_type, 
                                                     rg_config_p->width,
                                                     &raid_type_max_correctable_errors);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    abby_cadabby_sum_error_counts(context_p, num_contexts);
       
    /* We check for correctable or uncorrectable tables and make sure the errors
     * we received on the I/Os are correct. 
     * Note we do not do this for random tables since they could have received correctable 
     * or uncorrectable errors. 
     */
    if (((test_params_p->number_of_errors + drives_to_power_off) <= raid_type_max_correctable_errors) &&
        (test_params_p->b_random_errors == FBE_FALSE))
    {
        /* We injected errors that should be corectable for this raid type.
         * Make sure there were no errors.
         */
        if(ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            /* if we are injecting random aborts then we can expect error
             */
            if(drives_to_power_off == 0)
            {
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_failure_error_count, 0);
            }

            /* We can get media errors.  The media error count plus the abort 
             * error count should be all the errors.
             */
            mut_printf(MUT_LOG_TEST_STATUS, " %s: random abort results: media_error_count = %d; aborted_error_count = %d; error_count = %d ",
                       __FUNCTION__, 
                       context_p->start_io.statistics.media_error_count,                       
                       context_p->start_io.statistics.aborted_error_count,
                       context_p->start_io.statistics.error_count);
            
            MUT_ASSERT_TRUE((context_p->start_io.statistics.media_error_count + context_p->start_io.statistics.aborted_error_count) ==
                    context_p->start_io.statistics.error_count);


        }
        else
        {
            /* Else determine if we are running degraded or not.
             */
            if (drives_to_power_off == 0)
            {
                /*! @note Media errors are a special case.  Since SEP clients 
                 *        will get `success' status for requests that resulted
                 *        in media errors, rdgen treats media errors as `success'.
                 *        But since the block status is actually media error, we
                 *        we must determine if all the errors where media errors.
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_SUCCESS);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_failure_error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);


                if (context_p->start_io.statistics.error_count == 0)
                {
                    /* Ok, no media errors detected.
                     */
                    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
                }
                else
                {
                    /* Else the only errors expected are media errors.
                     */
                    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, context_p->start_io.statistics.media_error_count);
                    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
                }
            }
            else
            {
                /*! @todo temporarily since paged metadata errors transition us to activate, 
                *        ignore io failures during drive faulting.
                */
                if ((context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
                    (context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
                    /*! @todo temporarily since paged metadata errors transition us to activate, 
                    *        when drives are pulled. Ignore io failures during drive faulting.
                    */
                    (drives_to_power_off == 0))
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                            __FUNCTION__, context_p->start_io.status.block_status);
                    MUT_FAIL_MSG("block status from rdgen was unexpected");
                }
            }
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.aborted_error_count, 0);
        }
    }
    else if (((test_params_p->number_of_errors + drives_to_power_off) > raid_type_max_correctable_errors) &&
             (test_params_p->b_random_errors == FBE_FALSE))
    {
        /* We injected more than we should be able to correct.
         * Make sure there were errors.
         */
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.media_error_count, 0);
        /* The rdgen block status might be good or bad depending on what the last
         * completion value was.   We only expect success or media error. 
         */
        if ((context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
            (context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) &&
            /*! @todo temporarily since paged metadata errors transition us to activate, 
             *        when drives are pulled. Ignore io failures during drive faulting.
             */
            (drives_to_power_off == 0))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                       __FUNCTION__, context_p->start_io.status.block_status);
            MUT_FAIL_MSG("block status from rdgen was unexpected");
        }

        /* We do not expect aborted errors if we were not injecting aborts.
         */
        if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.aborted_error_count, 0);
        }
        /*! @todo temporarily since paged metadata errors transition us to activate, 
         *        ignore io failures during drive faulting.
         */
        if (drives_to_power_off == 0)
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_failure_error_count, 0);
        }
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/**************************************
 * end abby_cadabby_validate_io_status()
 **************************************/

/*!**************************************************************
 * abby_cadabby_disable_error_injection()
 ****************************************************************
 * @brief
 *  Disable error injection for a given class of object.
 *
 * @param rg_config_p - Configuration we are testing against. 
 * 
 * @return None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
void abby_cadabby_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    /* Disable error injection prior to disabling the class so that all error injection stops 
     * across all raid groups at once.  
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Raid 10s only enable the mirrors, so just disable the mirror class.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
    }
    else
    {
        status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/**************************************
 * end abby_cadabby_disable_error_injection()
 **************************************/

/*!**************************************************************
 * abby_cadabby_setup_test_contexts()
 ****************************************************************
 * @brief
 *  Initialize the rdgen test contexts.
 *
 * @param test_params_p - type of test to run.
 * @param rg_config_p - Configuration we are testing against.
 * @param context_p - Array of test contexts.
 * @param drives_to_power_off - Number of drives to turn off
 *                              while we are testing.
 * @param ran_abort_msecs - If not FBE_TEST_RANDOM_ABORT_TIME_INVALID,
 *                          then this is the number of seconds rdgen
 *                          should wait before rdgen aborts IO.
 *
 * @return None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
void abby_cadabby_setup_test_contexts(fbe_test_logical_error_table_info_t *test_params_p,
                                      fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_api_rdgen_context_t *context_p,
                                      fbe_u32_t *num_contexts_p,
                                      fbe_u32_t threads,
                                      fbe_test_random_abort_msec_t ran_abort_msecs,
                                      fbe_bool_t b_dualsp_mode)

{
    fbe_u32_t index;
    fbe_status_t status;
    fbe_bool_t b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    fbe_u32_t raid_group_count = fbe_test_get_rg_count(rg_config_p);

    if (b_dualsp_mode)
    {
        fbe_u32_t number_of_luns = raid_group_count * ABBY_CADABBY_TEST_LUNS_PER_RAID_GROUP;
        if ((number_of_luns * 2) > ABBY_CADABBY_NUM_IO_CONTEXT_DUAL_SP)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%d luns > %d contexts", number_of_luns, ABBY_CADABBY_NUM_IO_CONTEXT_DUAL_SP);
            MUT_FAIL();
        }
        status = fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(rg_config_p,
                                                                       context_p,
                                                                       number_of_luns,
                                                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                                       0, /* passes */
                                                                       threads,
                                                                       ABBY_CADABBY_MAX_IO_SIZE_BLOCKS,
                                                                       FBE_FALSE, /* Must be FALSE for caterpillar */
                                                                       FBE_TRUE, /* Run I/O increasing */
                                                                       b_ran_aborts_enabled, 
                                                                       b_dualsp_mode,
                                                                       fbe_test_sep_io_setup_caterpillar_rdgen_test_context);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(rg_config_p,
                                                                       &context_p[number_of_luns],
                                                                       number_of_luns,
                                                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                                       0, /* passes */
                                                                       threads,
                                                                       128, /* blocks max */
                                                                       FBE_FALSE, /* Must be FALSE for caterpillar */
                                                                       FBE_TRUE, /* Run I/O increasing */
                                                                       b_ran_aborts_enabled, 
                                                                       b_dualsp_mode,
                                                                       fbe_test_sep_io_setup_caterpillar_rdgen_test_context);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        *num_contexts_p = number_of_luns * 2;
    }
    else
    {
        fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           threads, /* threads */
                                           ABBY_CADABBY_MAX_IO_SIZE_BLOCKS,
                                           b_ran_aborts_enabled, 
                                           b_dualsp_mode);
        fbe_test_sep_io_setup_standard_rdgen_test_context(&context_p[1],
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           threads, /* threads */
                                           128, /* blocks max*/
                                           b_ran_aborts_enabled, 
                                           b_dualsp_mode);
        *num_contexts_p = ABBY_CADABBY_NUM_IO_CONTEXT;
    }

    
    /* Make sure that I/O runs until we stop it, even if -fbe_io_seconds was used 
    */
    for (index = 0; index < *num_contexts_p; index++)
    {
        fbe_api_rdgen_io_specification_set_num_ios(&(context_p[index].start_io.specification), 0,0,0);
    }

    if ((test_params_p->number_of_errors > 1) || (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
    {
        /* We are either injecting uncorrectables or we are non-redundant. 
         * Setup rdgen to continue on error. 
         */
        for (index = 0; index < *num_contexts_p; index++)
        {
            status = fbe_api_rdgen_io_specification_set_options(&context_p[index].start_io.specification,
                                                                FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        /* set the random abort time to invalid here as we will not be injecting random aborts in this case
          * This is needed because we will be checking this futher while analyzing the errors reported
          */
        for (index = 0; index < *num_contexts_p; index++)
        {
            status = fbe_api_rdgen_clear_random_aborts(&context_p[index].start_io.specification);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* If the caller asked for random aborts and we are not injecting them, just exit now, 
         * since we have nothing to do. 
         */
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    }
    else if (ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s random aborts inserted %d msecs", __FUNCTION__, ran_abort_msecs);

        /* else random aborts have been configured above
         */
    }
}
/**************************************
 * end abby_cadabby_setup_test_contexts()
 **************************************/
/*!**************************************************************
 * abby_cadabby_error_injection_test()
 ****************************************************************
 * @brief
 *  Run a test case for an error injection test.
 *
 * @param test_params_p - type of test to run.
 * @param rg_config_p - Configuration we are testing against.
 * @param raid_group_count - total entries in above config_p array.
 * @param drives_to_power_off - Number of drives to turn off
 *                              while we are testing.
 * @param ran_abort_msecs - If not FBE_TEST_RANDOM_ABORT_TIME_INVALID,
 *                          then this is the number of seconds rdgen
 *                          should wait before rdgen aborts IO.
 *
 * @return None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_error_injection_test(fbe_test_logical_error_table_info_t *test_params_p,
                                       fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_u32_t raid_group_count,
                                       fbe_u32_t drives_to_power_off,
                                       fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &abby_cadabby_test_contexts[0];
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t io_seconds;
    fbe_u32_t threads = fbe_test_sep_io_get_threads(2);
    fbe_bool_t b_ran_aborts_enabled;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;
    fbe_bool_t b_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_test_sep_io_dual_sp_mode_t dual_sp_mode;
    fbe_u32_t num_contexts;
    
    /* This includes striper and mirrors for raid 10s.
     */
    fbe_u32_t total_raid_group_count = abby_cadabby_get_total_raid_group_count(rg_config_p, raid_group_count);

    /* Initialize the filter
     */
    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID, 0 /* edge index */);

    /* Reset stats since we want to look at stats for just this test case.
     */
    status = fbe_api_rdgen_reset_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! If we support dual SP I/O, randomize between load balance and randomize.
     */
    if (b_dualsp_mode)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_rdgen_reset_stats();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        if (fbe_random() % 2)
        {
            dual_sp_mode = FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE;
        }
        else
        {
            dual_sp_mode = FBE_TEST_SEP_IO_DUAL_SP_MODE_RANDOM;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "Running I/O mode %s", 
                   (dual_sp_mode == FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE) ? "load balance" : "random");
    }
    status = fbe_test_sep_io_configure_dualsp_io_mode(b_dualsp_mode, dual_sp_mode);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the abort mode based on ran_abort_msecs
     */
    b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    status = fbe_test_sep_io_configure_abort_mode(b_ran_aborts_enabled, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the rdgen test contexts.
     */
    abby_cadabby_setup_test_contexts(test_params_p, rg_config_p, context_p, &num_contexts, threads, ran_abort_msecs, b_dualsp_mode);
    MUT_ASSERT_INT_EQUAL(context_p[0].b_initialized, 1);
    MUT_ASSERT_INT_EQUAL(context_p[num_contexts-1].b_initialized, 1);

    /* Enable injection on both SPs.
     */
    abby_cadabby_enable_injection(test_params_p, rg_config_p, raid_group_count);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        abby_cadabby_enable_injection(test_params_p, rg_config_p, raid_group_count);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Run our I/O test.
     * We allow the user to input a time to run I/O. 
     * By default io seconds is 0. 
     *  
     * By default we will run I/O until we see enough errors is encountered.
     */
    io_seconds = fbe_test_sep_util_get_io_seconds();
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Let /IO run for 5 seconds before any drive removal.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O before any drive removal for %d seconds ==", 
               __FUNCTION__, 5000/1000);
    fbe_api_sleep(5000);

    /* Validate that I/O was started successfully.
     */
    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_INVALID, context_p->status);
    MUT_ASSERT_INT_EQUAL(rdgen_stats.requests, num_contexts);

    /* Power off drives for this table.
     */
    if ((drives_to_power_off != 0) && (drives_to_power_off != FBE_U32_MAX))
    {
        big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_power_off, 
                                   FBE_FALSE, /* We do not plan on re-inserting this drive later. */ 
                                   3000, /* msecs between removals */
                                   FBE_DRIVE_REMOVAL_MODE_RANDOM);
        fbe_api_sleep(3000);
    }

    if (io_seconds > 0)
    {
        /* Let it run for a reasonable time before stopping it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O (%d threads) for %d seconds ==", __FUNCTION__, threads, io_seconds);
        fbe_api_sleep(io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O (%d threads)==", __FUNCTION__, threads);
    }

    /* Run until we have seen enough errors injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for errors  ==", __FUNCTION__);
    status = abby_cadabby_wait_for_errors(test_params_p, rg_config_p, raid_group_count,
                                          ABBY_CADABBY_MAX_IO_SECONDS, ran_abort_msecs); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure that we received enough errors.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, test_params_p->record_count);

    /* We use * 2 since we have 520 and 512.
     */
    MUT_ASSERT_INT_EQUAL(stats.num_objects, total_raid_group_count);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, total_raid_group_count);
    /*! check that errors occurred, and that errors were validated.
     */
    MUT_ASSERT_TRUE(stats.num_errors_injected >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR); 
    MUT_ASSERT_TRUE(stats.num_failed_validations == 0);
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_TRUE((stats.num_validations >= ABBY_CADABBY_MIN_ERROR_COUNT_TO_WAIT_FOR) );
                       
    }
    /* Stop logical error injection.  We do this before stopping I/O since if we allow
     * error injection to continue then it will take a while to stop I/O. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);

    abby_cadabby_disable_error_injection(rg_config_p);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        abby_cadabby_disable_error_injection(rg_config_p);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure we ran I/O on one or both SPs.
     */
    fbe_test_sep_io_validate_rdgen_ran();

     /* Power on drives for this table.
     */
    if ((drives_to_power_off != 0) && (drives_to_power_off != FBE_U32_MAX))
    {
        big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_power_off, 
                                   FBE_FALSE /* We do not plan on re-inserting this drive later. */ );
        /* Wait for the rebuilds to finish.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
        big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_power_off );
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
    }

    /* Make sure the io status is as expected.
     */
    abby_cadabby_validate_io_status(test_params_p, rg_config_p, raid_group_count, 
                                    context_p, num_contexts,
                                    drives_to_power_off, ran_abort_msecs);

    /* Validate the error log info.
     * This checks to make sure there are no coherency errors, but
     *   don't do it for table 5 since that injects coherency errors. 
     */
    if (test_params_p->table_number != 5)
    {
        abby_cadabby_validate_event_log_errors();
    }

    /* Destroy all the objects the logical error injection service is tracking. 
     * This will allow us to start fresh when we start the next test with 
     * number of errors and number of objects reported by lei.
     */
    status = fbe_api_logical_error_injection_destroy_objects();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_api_logical_error_injection_destroy_objects();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    return;
}
/******************************************
 * end abby_cadabby_error_injection_test()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_get_table_info()
 ****************************************************************
 * @brief
 *  Get the information on a given table number.
 *  We simply search the abby cadabby table info array for this
 *  table number and return a ptr to the table info.
 *
 * @param table_number - Table # to search for.
 * @param info_p - Ptr to info to return.
 *
 * @return FBE_STATUS_OK if we found it
 *         FBE_STATUS_GENERIC_FAILURE otherwise
 *
 * @author
 *  5/7/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t abby_cadabby_get_table_info(fbe_u32_t table_number,
                                         fbe_test_logical_error_table_info_t **info_p)
{
    fbe_u32_t table_index;
    /* Loop until we find the table number or we hit the terminator.
     */
    for (table_index = 0; (abby_cadabby_table_info[table_index].table_number != FBE_U32_MAX); table_index++)
    {
        if (abby_cadabby_table_info[table_index].table_number == table_number)
        {
            *info_p = &abby_cadabby_table_info[table_index];
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/**************************************
 * end abby_cadabby_get_table_info()
 **************************************/

/*!**************************************************************
 * abby_cadabby_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 0 I/O test.
 *
 * @param   rg_config_p
 * @param   context_p points to config on which test is currently running               
 *
 * @return None.   
 *
 * @author
 *  05/06/2011 - Created. Vamsi V
 *
 ****************************************************************/

void abby_cadabby_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t  raid_group_count = fbe_test_get_rg_count(rg_config_p);
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t test_index = 0;
    fbe_status_t status;
    fbe_bool_t b_only_degraded = fbe_test_sep_util_get_cmd_option("-degraded_only");
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_test_logical_error_table_test_configuration_t * curr_config_p = NULL;

    /* Context has pointer to config on which test is currently running
     */
    curr_config_p = (fbe_test_logical_error_table_test_configuration_t *) context_p;

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

    /* Loop over tables until we hit the terminator.
     */
    for (test_index = 0; (curr_config_p->error_table_num[test_index] != FBE_U32_MAX); test_index++)
    {
        fbe_test_logical_error_table_info_t *table_info_p = NULL;
    
        /* If we are only testing degraded mode, then skip this case.
         */
        if ((curr_config_p->num_drives_to_power_down[test_index] == 0) &&
            (b_only_degraded == FBE_TRUE))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== %s skipping test case %d for raid_type %s (-only_degraded is set)", 
                       __FUNCTION__, test_index, raid_type_string_p);
            continue;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== %s starting test %d for b_bandwidth %d raid_type: %s groups: %d", 
                   __FUNCTION__, 
                   curr_config_p->error_table_num[test_index],
                   curr_config_p->raid_group_config->b_bandwidth,
		   raid_type_string_p, raid_group_count);
    
        status = abby_cadabby_get_table_info(curr_config_p->error_table_num[test_index], &table_info_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    

        abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);

        /* Write the background pattern to seed this element size before starting the
         * test. 
         */
        big_bird_write_background_pattern();
        abby_cadabby_error_injection_test(table_info_p, rg_config_p, raid_group_count, 
                                             curr_config_p->num_drives_to_power_down[test_index], 
                                             FBE_TEST_RANDOM_ABORT_TIME_INVALID);

        if(test_level > 0)
        {
            mut_printf(MUT_LOG_TEST_STATUS,
		       " Test started with random aborts msecs : %d", 
                       FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC);

            abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);
            if ((table_info_p->number_of_errors > 1) || (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
            {
                /*! @todo currently we do not do abort testing in this case.  
                 * If the caller asked for abort testing then just return since we have nothing new 
                 * to test here.  This will shorten the test by avoiding injecting aborts for this case. 
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "%s skip abort test for table %d", __FUNCTION__, table_info_p->table_number);
                continue;
            }

            /* Write the background pattern to seed this element size before starting the
             * test. 
             */
            big_bird_write_background_pattern();
            abby_cadabby_error_injection_test(table_info_p, rg_config_p, raid_group_count, 
                                              curr_config_p->num_drives_to_power_down[test_index], 
                                              FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC);
        
            /* There was an issue where IO abort operation started and as a part of it 
             * it started verify operation.
             * This was not completed before we could start the new error table injection.
             * Verify was caught in between the error injection and drive removing timing window.
             * Hence, we're not ready to move on to the next test until all the verifies complete.
             * Hence moving the wait for verifies after each test.
             */
            abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);

            mut_printf(MUT_LOG_TEST_STATUS,
		       " Test with random aborts completed.");
        }
    }
    
    /* Make sure we did not get any trace errors.  We do this in between each
     * raid group test since it stops the test sooner. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/**************************************
 * end abby_cadabby_test_rg_config()
 **************************************/
/*!**************************************************************
 * abby_cadabby_run_tests()
 ****************************************************************
 * @brief
 *  Run error injection test to several LUNs.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_run_tests(fbe_test_logical_error_table_test_configuration_t *config_p, 
                            fbe_test_random_abort_msec_t ran_abort_msec,
                            fbe_raid_group_type_t raid_type)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_type_index = 0;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t config_count = abby_cadabby_get_config_count(config_p);
    fbe_raid_library_debug_flags_t current_debug_flags;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    status = fbe_api_raid_group_get_library_debug_flags(FBE_OBJECT_ID_INVALID, &current_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* always enable the FBE_RAID_LIBRARY_DEBUG_FLAG_INVALID_DATA_CHECKING */
    status = fbe_api_raid_group_set_library_debug_flags(FBE_OBJECT_ID_INVALID, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_INVALID_DATA_CHECKING | 
                                                         current_debug_flags));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p[0].raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", 
                       raid_type_string_p, rg_config_p[0].raid_type);
            continue;
        } 
        if (raid_type == rg_config_p[0].raid_type)
        {
            if (raid_type_index+1 >= config_count) {
                /* Now create the raid groups and run the tests
                 */
                fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, &config_p[raid_type_index], 
                                                               abby_cadabby_test_rg_config,
                                                               ABBY_CADABBY_TEST_LUNS_PER_RAID_GROUP,
                                                               ABBY_CADABBY_CHUNKS_PER_LUN); 
            }
            else {
                /* Now create the raid groups and run the tests
                 */
                fbe_test_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, &config_p[raid_type_index], 
                                                               abby_cadabby_test_rg_config,
                                                               ABBY_CADABBY_TEST_LUNS_PER_RAID_GROUP,
                                                               ABBY_CADABBY_CHUNKS_PER_LUN,
                                                               FBE_FALSE); 
            }
        }
    }
    return;
}
/**************************************
 * end abby_cadabby_run_tests()
 **************************************/

/*!***************************************************************************
 *          abby_cadabby_simulation_setup()
 *****************************************************************************
 *
 * @brief   Setup the simulation configuration.
 *
 * @param   logical_error_config_p - Pointer to array of logical error configs
 * @param   b_randomize_block_size - FBE_TRUE - Randomize the block size for
 *              each error config:
 *                  o 520
 *                  o 4K
 *                  o Mixed  
 *
 * @return  None.   
 *
 * @author
 *  3/24/2011 - Created. Rob Foley
 *
 *****************************************************************************/
void abby_cadabby_simulation_setup(fbe_test_logical_error_table_test_configuration_t *logical_error_config_p,
                                  fbe_bool_t b_randomize_block_size)
{
    fbe_u32_t test_index;
    fbe_u32_t config_count;
    /* Only load the physical config in simulation.
     */
    if (!fbe_test_util_is_simulation())
    {
        MUT_FAIL_MSG("Should only be called in simulation mode");
        return;
    }

    config_count = abby_cadabby_get_config_count(logical_error_config_p);
    for (test_index = 0; test_index < config_count; test_index++)
    {
        fbe_test_sep_util_init_rg_configuration_array(&logical_error_config_p[test_index].raid_group_config[0]);
        if (b_randomize_block_size == FBE_TRUE) {
            fbe_test_rg_setup_random_block_sizes(&logical_error_config_p[test_index].raid_group_config[0]);
            fbe_test_sep_util_randomize_rg_configuration_array(&logical_error_config_p[test_index].raid_group_config[0]);
        }

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(&logical_error_config_p[test_index].raid_group_config[0]);
    }
    return;
}
/**************************************
 * end abby_cadabby_simulation_setup()
 **************************************/

/*********************************************************************************
 * BEGIN Test, Setup, Cleanup.
 *********************************************************************************/
/*!**************************************************************
 * abby_cadabby_test()
 ****************************************************************
 * @brief
 *  Run error injection test to several LUNs.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &abby_cadabby_raid_groups_extended[0];
    }
    else
    {
        config_p = &abby_cadabby_raid_groups_qual[0];
    }

    /* Run the test.
     */
    abby_cadabby_run_tests(config_p, FBE_TEST_RANDOM_ABORT_TIME_INVALID, raid_type);

    return;
}
/******************************************
 * end abby_cadabby_test()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param raid_type          
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void abby_cadabby_setup(fbe_raid_group_type_t raid_type)
{
    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);
    /* We do not want event logs to flood the trace.
     */
    fbe_api_event_log_disable(FBE_PACKAGE_ID_SEP_0);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            config_p = &abby_cadabby_raid_groups_extended[0];
        }
        else
        {
            config_p = &abby_cadabby_raid_groups_qual[0];
        }
        abby_cadabby_simulation_setup(config_p,
                                      FBE_TRUE  /* Randomize the drive block size */);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);
        elmo_load_sep_and_neit();
    }

    /* This test will pull a drive out and insert a new drive
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);
    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    fbe_test_common_util_test_setup_init();
    
    return;
}
/**************************************
 * end abby_cadabby_setup()
 **************************************/


/*!**************************************************************
 * abby_cadabby_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the abby_cadabby test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_cleanup(void)
{
    //fbe_status_t status;
    //status = fbe_api_rdgen_stop_tests(abby_cadabby_test_contexts, ABBY_CADABBY_NUM_IO_CONTEXT);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_restore_sector_trace_level();
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end abby_cadabby_cleanup()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_dualsp_test()
 ****************************************************************
 * @brief
 *  Run an error test dual sp.
 *
 * @param  None             
 *
 * @return None.   
 *
 * @author
 *  6/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_dualsp_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &abby_cadabby_raid_groups_extended[0];
    }
    else
    {
        config_p = &abby_cadabby_raid_groups_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run the test.
     */
    abby_cadabby_run_tests(config_p, FBE_TEST_RANDOM_ABORT_TIME_INVALID, raid_type);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end abby_cadabby_dualsp_test()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for the test.
 *
 * @param raid_type - the type of rg to test with.           
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  6/28/2011 - Created. Rob Foley
 *
 ****************************************************************/
void abby_cadabby_dualsp_setup(fbe_raid_group_type_t raid_type)
{
    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);
    /* We do not want event logs to flood the trace.
     */
    fbe_api_event_log_disable(FBE_PACKAGE_ID_SEP_0);

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            config_p = &abby_cadabby_raid_groups_extended[0];
        }
        else
        {
            config_p = &abby_cadabby_raid_groups_qual[0];
        }

        abby_cadabby_simulation_setup(config_p,
                                      FBE_TRUE  /* Randomize the drive block size */);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /* This test will pull a drive out and insert a new drive
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */

    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/******************************************
 * end abby_cadabby_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * abby_cadabby_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void abby_cadabby_dualsp_cleanup(void)
{
    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

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
 * end abby_cadabby_dualsp_cleanup()
 ******************************************/

/* Setup for dual sp tests.
 */
void abby_cadabby_raid1_setup(void)
{
    abby_cadabby_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void abby_cadabby_raid10_setup(void)
{
    abby_cadabby_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void abby_cadabby_raid5_setup(void)
{
    abby_cadabby_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void abby_cadabby_raid3_setup(void)
{
    abby_cadabby_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void abby_cadabby_raid6_setup(void)
{
    abby_cadabby_setup(FBE_RAID_GROUP_TYPE_RAID6);
}
/* Test functions for single sp tests.
 */
void abby_cadabby_raid1_test(void)
{
    abby_cadabby_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void abby_cadabby_raid3_test(void)
{
    abby_cadabby_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void abby_cadabby_raid5_test(void)
{
    abby_cadabby_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void abby_cadabby_raid6_test(void)
{
    abby_cadabby_test(FBE_RAID_GROUP_TYPE_RAID6);
}
void abby_cadabby_raid10_test(void)
{
    abby_cadabby_test(FBE_RAID_GROUP_TYPE_RAID10);
}


/* Setup for dual sp tests.
 */
void abby_cadabby_dualsp_raid1_setup(void)
{
    abby_cadabby_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void abby_cadabby_dualsp_raid10_setup(void)
{
    abby_cadabby_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void abby_cadabby_dualsp_raid5_setup(void)
{
    abby_cadabby_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void abby_cadabby_dualsp_raid3_setup(void)
{
    abby_cadabby_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void abby_cadabby_dualsp_raid6_setup(void)
{
    abby_cadabby_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID6);
}

/* Test functions for dual sp tests.
 */
void abby_cadabby_dualsp_raid1_test(void)
{
    abby_cadabby_dualsp_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void abby_cadabby_dualsp_raid3_test(void)
{
    abby_cadabby_dualsp_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void abby_cadabby_dualsp_raid5_test(void)
{
    abby_cadabby_dualsp_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void abby_cadabby_dualsp_raid6_test(void)
{
    abby_cadabby_dualsp_test(FBE_RAID_GROUP_TYPE_RAID6);
}
void abby_cadabby_dualsp_raid10_test(void)
{
    abby_cadabby_dualsp_test(FBE_RAID_GROUP_TYPE_RAID10);
}

/*********************************************************************************
 * END Test, Setup, Cleanup.
 *********************************************************************************/
/*************************
 * end file abby_cadabby_test.c
 *************************/


