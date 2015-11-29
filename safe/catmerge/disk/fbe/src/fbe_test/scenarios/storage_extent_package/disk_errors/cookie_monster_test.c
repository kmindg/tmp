/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cookie_monster_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an uncorrectable error test.
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
#include "sep_test_io.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_event_log_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * cookie_monster_short_desc = "raid uncorrectable error test (non-degraded and degraded)";
char * cookie_monster_long_desc ="\
The CookieMonster scenario tests uncorrectable errors with I/O. \n\
\n\
Each error table is first injected against non-degraded raid groups then    \n\
sufficient drives are removed from each raid group to cause the raid group  \n\
to become degraded.  Both drives with 520-bps and 512-bps are tested at all \n\
test levels.  There is only (1) LU bound per raid group.                    \n\
\n"\
"\
Based on the raid test level and raid type the following error injection    \n\
tables are used: \n\
    o Table  0 - Correctable Errors, Injects Errors Always, Assorted Error Types        \n\
    o Table  1 - Correctable Errors, Injects Errors 15 Times, Hard Error Types          \n\
    o Table  3 - Uncorrectable Errors, Injects Always, Hard Error Types   \n\
    o Table  5 - All types and combinations of correctable checksum errors              \n\
    o Table  6 - Tests all types and combinations of uncorrectable checksum errors \n\
    o Table 10 - (RAID-6 only) random error patterns (3 errors) \n\
    o Table 12 - (RAID-6 only) random error patterns (3 errors)  \n\
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
        Currently Table 6 is disabled for RAID-5 and RAID-3 raid types due to   \n\
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
#define COOKIE_MONSTER_TEST_LUNS_PER_RAID_GROUP 1

/* Element size of our LUNs.
 */
#define COOKIE_MONSTER_TEST_ELEMENT_SIZE 128 

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define COOKIE_MONSTER_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/* The number of blocks in a raid group bitmap chunk.
 */
#define COOKIE_MONSTER_TEST_RAID_GROUP_CHUNK_SIZE  FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH

/*!*******************************************************************
 * @def COOKIE_MONSTER_DEFAULT_IO_SECONDS_QUAL
 *********************************************************************
 * @brief Run I/O for a while before checking the error counts.
 *
 *********************************************************************/
#define COOKIE_MONSTER_DEFAULT_IO_SECONDS_QUAL 3

/*!*******************************************************************
 * @def COOKIE_MONSTER_DEFAULT_IO_SECONDS_EXTENDED
 *********************************************************************
 * @brief In extended mode we use a different time to wait before
 *        checking the error counts.
 *
 *********************************************************************/
#define COOKIE_MONSTER_DEFAULT_IO_SECONDS_EXTENDED 5

/*!*******************************************************************
 * @def COOKIE_MONSTER_MAX_IO_SECONDS
 *********************************************************************
 * @brief Max time to run I/O for.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MAX_IO_SECONDS 30

/*!*******************************************************************
 * @def COOKIE_MONSTER_MIN_ERROR_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of errors before
 *        continuing with the test.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MIN_ERROR_COUNT_TO_WAIT_FOR 10

/*!*******************************************************************
 * @def COOKIE_MONSTER_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define COOKIE_MONSTER_VERIFY_WAIT_SECONDS 30

/*!*******************************************************************
 * @def COOKIE_MONSTER_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MAX_IO_SIZE_BLOCKS (COOKIE_MONSTER_TEST_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @def COOKIE_MONSTER_MAX_CONFIGS
 *********************************************************************
 * @brief The number of configurations we will test in extended mode.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MAX_CONFIGS 20
#define COOKIE_MONSTER_MAX_CONFIGS_EXTENDED COOKIE_MONSTER_MAX_CONFIGS
/*!*******************************************************************
 * @def COOKIE_MONSTER_MAX_CONFIGS_QUAL
 *********************************************************************
 * @brief The number of configurations we will test in qual.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MAX_CONFIGS_QUAL 3

/*!*******************************************************************
 * @def COOKIE_MONSTER_MAX_TABLES
 *********************************************************************
 * @brief Max number of tables we can test for one raid group.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MAX_TABLES 10

/*!*******************************************************************
 * @def COOKIE_MONSTER_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with at a time.
 *
 *********************************************************************/
#define COOKIE_MONSTER_MAX_RAID_GROUP_COUNT 10

/*!*******************************************************************
 * @def COOKIE_MONSTER_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define COOKIE_MONSTER_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def COOKIE_MONSTER_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.
 *
 *********************************************************************/
#define COOKIE_MONSTER_FIRST_POSITION_TO_REMOVE 0

/*!*******************************************************************
 * @def COOKIE_MONSTER_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define COOKIE_MONSTER_SECOND_POSITION_TO_REMOVE 1

/*!*******************************************************************
 * @def COOKIE_MONSTER_QUIESCE_TEST_MAX_SLEEP_TIME
 *********************************************************************
 * @brief Max time we will sleep for in the quiesce test.
 *        Each iteration of the queisce test does a sleep for a random
 *        amount of time from 0..N seconds.
 *********************************************************************/
#define COOKIE_MONSTER_QUIESCE_TEST_MAX_SLEEP_TIME 2000

/*!*******************************************************************
 * @def COOKIE_MONSTER_QUIESCE_TEST_THREADS_PER_LUN
 *********************************************************************
 * @brief The number of threads per lun for the quiesce/unquiesce test.
 *
 *********************************************************************/
#define COOKIE_MONSTER_QUIESCE_TEST_THREADS_PER_LUN 10

/*!*******************************************************************
 * @def COOKIE_MONSTER_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define COOKIE_MONSTER_WAIT_MSEC 30000

/*!***************************************************************************
 * @def     COOKIE_MONSTER_DE50_RAID5_TABLE6
 *****************************************************************************
 * @brief   Currently DE50 is tracking an issue where any `invalidation' error
 *          will now result in a `Media Error' status.  Table 6 needs to be
 *          updated to expect both correctable and uncorrectable errors based
 *          on the error type (all invalidation error types will result in
 *          uncorrectable errors).
 *
 * @note    `COOKIE_MONSTER_DE50_RAID5_TABLE6' MUST BE THE LAST TABLE!!
 *
 * @todo    Fix this defect!
 *
 *****************************************************************************/
#define COOKIE_MONSTER_DE50_RAID5_TABLE6  (FBE_U32_MAX)   /*!< By setting this to the terminator the table is disabled. */

/*!*******************************************************************
 * @var COOKIE_MONSTER_MIRROR_LIBRARY_DEBUG_FLAGS
 *********************************************************************
 * @brief Default value of the raid library debug flags to set for mirror
 *
 *********************************************************************/
fbe_u32_t COOKIE_MONSTER_MIRROR_LIBRARY_DEBUG_FLAGS = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );

/*!*******************************************************************
 * @var cookie_monster_raid_groups_extended
 *********************************************************************
 * @brief Raid group config for extended level tests.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_logical_error_table_test_configuration_t cookie_monster_raid_groups_extended[COOKIE_MONSTER_MAX_CONFIGS] = 
#else
fbe_test_logical_error_table_test_configuration_t cookie_monster_raid_groups_extended[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
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
        { 3, 3, COOKIE_MONSTER_DE50_RAID5_TABLE6, COOKIE_MONSTER_DE50_RAID5_TABLE6, FBE_U32_MAX, /* Error tables. */ },
        { 0, 1, 0, 1, 0, /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        4,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        5,          0},
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        9,          1},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        10,         1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        11,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, 10, 12, 3, 6, 10, 3, 6, 10, FBE_U32_MAX    /* Error tables. */},
        { 0, 0, 0,  0,  1, 1, 1,  2, 2, 2,  0/* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        2,         0},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        3,         0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        6,         1},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        7,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 3, COOKIE_MONSTER_DE50_RAID5_TABLE6, COOKIE_MONSTER_DE50_RAID5_TABLE6,  FBE_U32_MAX  /* Error tables. */ },
        { 0, 1, 0, 1,  0, 0 /* Drives to power off */},
    },
   
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/

            {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
            {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          1},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          1},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 3, 6, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0,  0,  0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,    capacity    raid type,                  class,              block size, raid group id,  bandwidth */
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  520,            9,          1},
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  520,            10,         1},
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  520,            11,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, 3, 6, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 1, 1, 0, /* Drives to power off */},
    },
    {   
        { 
            /* width,    capacity    raid type,                  class,              block size, raid group id,  bandwidth */
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  520,           12,          1},
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  520,           13,          1},
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,  520,           14,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 13, 6, 13, 6, 13, 6, FBE_U32_MAX    /* Error tables. */},
        { 0, 0, 1, 1, 2, 2, 0/* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                   class,               block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        3,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        4,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        5,          0},
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        6,          1},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        7,          1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        9,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, 3, 6, FBE_U32_MAX    /* Error tables. */},
        { 0, 0, 1, 1, 0/* Drives to power off */},
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

/*!*******************************************************************
 * @var cookie_monster_raid_groups_qual
 *********************************************************************
 * @brief Raid group config for qual (level 0) testing.
 *
 *********************************************************************/
fbe_test_logical_error_table_test_configuration_t cookie_monster_raid_groups_qual[COOKIE_MONSTER_MAX_CONFIGS] = 
{
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/

            {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
            {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          1},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          1},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
            
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0,  0,  0, 0 /* Drives to power off */},
    },
    
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        3,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        4,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        5,          0},
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        9,          1},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        10,         1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        11,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, 10, 12, 3, 6, 10, 3, 6, 10, FBE_U32_MAX    /* Error tables. */},
        { 0, 0, 0,  0,  1, 1, 1,  2, 2, 2,  0/* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        3,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        5,          0},
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        9,          1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        11,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 3, COOKIE_MONSTER_DE50_RAID5_TABLE6, COOKIE_MONSTER_DE50_RAID5_TABLE6, FBE_U32_MAX, /* Error tables. */ },
        { 0, 1, 0, 1, 0, /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        4,          0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        10,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 3, COOKIE_MONSTER_DE50_RAID5_TABLE6, COOKIE_MONSTER_DE50_RAID5_TABLE6, FBE_U32_MAX, /* Error tables. */ },
        { 0, 1, 0, 1, 0, /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                   class,               block size, RAID-id.    bandwidth.*/
            {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR, 520,        8,          0},
            {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR, 520,        9,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, 3, 6, FBE_U32_MAX    /* Error tables. */},
        { 0, 0, 1, 1, 0/* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                   class,               block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        3,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        4,          0},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        5,          0},
            {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        6,          1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,        7,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 3, 6, 3, 6, FBE_U32_MAX    /* Error tables. */},
        { 0, 0, 1, 1, 0/* Drives to power off */},
    },
    
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

/*********************************************************************************
 * BEGIN Test, Setup, Cleanup.
 *********************************************************************************/
/*!**************************************************************
 * cookie_monster_test()
 ****************************************************************
 * @brief
 *  Run the uncorrectable error test using the abby cadabby test methods.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

void cookie_monster_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &cookie_monster_raid_groups_extended[0];
    }
    else
    {
        config_p = &cookie_monster_raid_groups_qual[0];
    }

    /* Run the test.
     */
    abby_cadabby_run_tests(config_p, FBE_TEST_RANDOM_ABORT_TIME_INVALID, raid_type);
	
    return;
}
/******************************************
 * end cookie_monster_test()
 ******************************************/
/*!**************************************************************
 * cookie_monster_setup()
 ****************************************************************
 * @brief
 *  Currently the setup occurs during the test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void cookie_monster_setup(fbe_raid_group_type_t raid_type)
{
    fbe_bool_t b_randomize_block_size = FBE_FALSE;  /*! @todo Add support to use random drive block size for all raid types */

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* We do not want event logs to flood the trace.
     */
    fbe_api_event_log_disable(FBE_PACKAGE_ID_SEP_0);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation()) {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0) {
            config_p = &cookie_monster_raid_groups_extended[0];
        } else {
            config_p = &cookie_monster_raid_groups_qual[0];
        }
        /*! @todo Add support to use random drive block size for all raid types.
         */
        
            b_randomize_block_size = FBE_TRUE;
        abby_cadabby_simulation_setup(config_p,
                                      b_randomize_block_size);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);
        elmo_load_sep_and_neit();
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* The following utility function does some setup for hardware. */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();

    /* This test will pull a drive out and insert a new drive
     * We configure this drive as a spare and we want this drive to swap in immediately. 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);
    
    return;
}
/**************************************
 * end cookie_monster_setup()
 **************************************/

/*!**************************************************************
 * cookie_monster_cleanup()
 ****************************************************************
 * @brief
 *  Currently the teardown occurs during the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void cookie_monster_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_restore_sector_trace_level();

    /* restore */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);
    
    if (fbe_test_util_is_simulation())
    {
    fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end cookie_monster_cleanup()
 ******************************************/

/*!**************************************************************
 * cookie_monster_dualsp_test()
 ****************************************************************
 * @brief
 *  Run an error test dual sp.
 *
 * @param  raid_type      
 *
 * @return None.   
 *
 * @author
 *  5/14/2012 - Created. Rob Foley
 *
 ****************************************************************/

void cookie_monster_dualsp_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &cookie_monster_raid_groups_extended[0];
    }
    else
    {
        config_p = &cookie_monster_raid_groups_qual[0];
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
 * end cookie_monster_dualsp_test()
 ******************************************/

/*!**************************************************************
 * cookie_monster_dualsp_setup()
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
 *  5/14/2012 - Created. Rob Foley
 *
 ****************************************************************/
void cookie_monster_dualsp_setup(fbe_raid_group_type_t raid_type)
{
    fbe_bool_t b_randomize_block_size = FBE_FALSE;  /*! @todo Add support to use random drive block size for all raid types */

    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* We do not want event logs to flood the trace.
     */
    fbe_api_event_log_disable(FBE_PACKAGE_ID_SEP_0);

    if (fbe_test_util_is_simulation()) { 
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0) {
            config_p = &cookie_monster_raid_groups_extended[0];
        } else {
            config_p = &cookie_monster_raid_groups_qual[0];
        }
        /*! @todo Add support to use random drive block size for all raid types.
         */
            b_randomize_block_size = FBE_TRUE;
        abby_cadabby_simulation_setup(config_p,
                                      b_randomize_block_size);

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

    fbe_test_disable_background_ops_system_drives();

    return;
}
/******************************************
 * end cookie_monster_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * cookie_monster_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/14/2012 - Created. Rob Foley
 *
 ****************************************************************/

void cookie_monster_dualsp_cleanup(void)
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
 * end cookie_monster_dualsp_cleanup()
 ******************************************/

/* Setup for single sp tests.
 */
void cookie_monster_raid0_setup(void)
{
    cookie_monster_setup(FBE_RAID_GROUP_TYPE_RAID0);
}
void cookie_monster_raid1_setup(void)
{
    cookie_monster_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void cookie_monster_raid10_setup(void)
{
    cookie_monster_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void cookie_monster_raid5_setup(void)
{
    cookie_monster_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void cookie_monster_raid3_setup(void)
{
    cookie_monster_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void cookie_monster_raid6_setup(void)
{
    cookie_monster_setup(FBE_RAID_GROUP_TYPE_RAID6);
}

/* Single sp tests. 
 */ 
void cookie_monster_raid0_test(void)
{
    cookie_monster_test(FBE_RAID_GROUP_TYPE_RAID0);
}
void cookie_monster_raid1_test(void)
{
    cookie_monster_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void cookie_monster_raid3_test(void)
{
    cookie_monster_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void cookie_monster_raid5_test(void)
{
    cookie_monster_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void cookie_monster_raid6_test(void)
{
    cookie_monster_test(FBE_RAID_GROUP_TYPE_RAID6);
}
void cookie_monster_raid10_test(void)
{
    cookie_monster_test(FBE_RAID_GROUP_TYPE_RAID10);
}

/* Setup for dual sp tests.
 */
void cookie_monster_dualsp_raid0_setup(void)
{
    cookie_monster_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID0);
}
void cookie_monster_dualsp_raid1_setup(void)
{
    cookie_monster_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void cookie_monster_dualsp_raid10_setup(void)
{
    cookie_monster_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void cookie_monster_dualsp_raid5_setup(void)
{
    cookie_monster_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void cookie_monster_dualsp_raid3_setup(void)
{
    cookie_monster_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void cookie_monster_dualsp_raid6_setup(void)
{
    cookie_monster_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID6);
}

/* Test functions for dual sp tests.
 */
void cookie_monster_dualsp_raid0_test(void)
{
    cookie_monster_dualsp_test(FBE_RAID_GROUP_TYPE_RAID0);
}
void cookie_monster_dualsp_raid1_test(void)
{
    cookie_monster_dualsp_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void cookie_monster_dualsp_raid3_test(void)
{
    cookie_monster_dualsp_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void cookie_monster_dualsp_raid5_test(void)
{
    cookie_monster_dualsp_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void cookie_monster_dualsp_raid6_test(void)
{
    cookie_monster_dualsp_test(FBE_RAID_GROUP_TYPE_RAID6);
}
void cookie_monster_dualsp_raid10_test(void)
{
    cookie_monster_dualsp_test(FBE_RAID_GROUP_TYPE_RAID10);
}
/*********************************************************************************
 * END Test, Setup, Cleanup.
 *********************************************************************************/
/*************************
 * end file cookie_monster_test.c
 *************************/


