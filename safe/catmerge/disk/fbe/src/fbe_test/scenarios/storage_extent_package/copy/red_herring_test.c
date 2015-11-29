/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file red_herring_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a correctable error test with copy operations.
 *
 * @version
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
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_background_ops.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"

/*! @todo Should not need the following
 */
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * red_herring_short_desc = "Raid correctable error test with copy operation.";
char * red_herring_long_desc ="\
The Red Herring scenario injects correctable errors with I/O and (1) copy operation.\n\
\n\
Depending on the redundancy and the raid test level a single drive for each \n\
raid group will be removed.  For raid test levels greater than 0 a single   \n\
drive will be removed for 3-way Mirror and RAID-6 raid types.               \n\
Only 520-bps drives are tested.         \n\
There is (3) LUs bound per raid group.                                  \n\
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
STEP 0: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Create extra provisioned drives for copy operation.\n\
        - Create provisioned drives and attach edges to logical drives.\n\
        - Create virtual drives and attach edges to provisioned drives.\n\
        - Create raid groups.\n\
        - Attach raid groups provisioned drives.\n\
        - Prevent the `reserved' provisioned drive from being spared.\n\
        - Create and attach LU(s) to each raid group.\n\
\n"\
"\
STEP 1: Configure the abort, dualsp, and drive replacement settings.\n\
        - Aborts are enabled if the test level is greater than 0.\n\
        - Copy position is currently hard coded to 0.\n\
        - The virtual drive associated with copy position is considered\n\
          as a raid group `under test'.  Thus the same error table that is\n\
          being run against the `parent' raid group(s) will be used to inject\n\
          errors on the virtual drive that is `mirror' mode.  Currently\n\
          errors will be injected to both positions (0 and 1) of the virtual\n\
          drive.\n\
        - The applicable error table is loaded.\n\
        - Reduce the spare swap-in time from default to (1) second.\n\
\n"\
"\
STEP 2: Start the I/O.  One `large' and one `small' random w/r/c thread.\n\
        - Let the I/O run for a short period before starting copy operation.\n\
\n"\
"\
STEP 3: Start (1) User Proactive Copy to position 0.\n\
        - All copy operations must be started before the associated raid group\n\
          is degraded.  Copy operations will not be started on a degraded raid\n\
          group.\n\
\n"\
"\
STEP 4: Enable error injection on both SPs.\n\
        - Currently the test will inject errors (based on the table) to both\n\
          the copy source and destination positions.\n\
\n"\
"\
STEP 5: Let I/O run for a short period.\n\
        - Let I/O and copy operation run for a short period before any\n\
          drives are removed.\n\
\n"\
"\
STEP 6: Based on the test configuration and raid type, remove drives.\n\
        - If enabled in the test configuration, remove the requested number\n\
          of drives from each raid group.  Each test only uses raid groups\n\
          of the exact same type (i.e. 2-way and 3-way mirrors are not tested\n\
          at the same time).\n\
        - Drives may be removed while the copy operation is in progress.  This\n\
          will pause the copy operation until the drives are replaced and \n\
          rebuilt.\n\
\n"\
"\
STEP 7: Replace removed drives and wait for rebuilds.\n\
        - If drives were removed replace the drive and wait for all rebuilds\n\
          to complete (this is required since thee copy operation will be \n\
          paused while the raid group is degraded).\n\
\n"\
"\
STEP 8: Wait for the copy operation to complete.\n\
        - Wait for the user proactive copy operation to complete (source drive\n\
          is swapped-out. \n\
        - Wait for any required cleanup to complete.\n\
\n"\
"\
STEP 9: Run until sufficient errors are detected.\n\
        - Let the I/O run until either sufficient errors are detected or\n\
          the wait for errors timeout occurs (currently 120 seconds).\n\
\n"\
"\
STEP 10: Stop error injection.\n\
        - Error injection (and validation) is stopped prior to stopping I/O\n\
          since stopping I/O may result in aborts being generated.\n\
\n"\
"\
STEP 11: Stop the I/O\n\
        - Stop the rdgen I/O.\n\
\n"\
"\
STEP 12: Validate I/O status.\n\
        - Based on the test configuration validate the expected I/O status.\n\
          For instance if only correctable errors are injected there should \n\
          be no I/O errors.\n\
        - Validate that there are no unexpected event messages.\n\
\n"\
"\
STEP 13: Cleanup.\n\
        - Perform any necessary cleanup such as destroying the logical\n\
          error injection objects etc.\n\
\n"\
"\
STEP 14: Destroy raid groups and LUNs    \n\
\n"\
"\
Test Specific Switches:  \n\
        -fbe_io_seconds (sec) - Extends the default I/O time.\n\
        -degraded_only        - Only run test cases that contain degraded drives.\n\
\n"\
"\
Outstanding Issues: \n\
        Currently `extended' testing is disabled.  When the -rtl is greater than 0  \n\
        `qual' testing is executed.  Extended testing is disabled because specific   \n\
        error injection tables must be created to support the copy operation.       \n\
        Namely when errors are injected into a virtual drive during a copy          \n\
        operation the mirror library may invalidate the destination position.  The  \n\
        invalidation is executed at the `chunk' level.  Chunks are much larger than \n\
        the `regions' used in all the other error injection tables.  Thus tables    \n\
        that are based on chunk size must be used.                                  \n\
\n"\
"\
Description Last Updated:   \n\
        05/10/2012          \n\
\n";


/*!*******************************************************************
 * @def RED_HERRING_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY
 *********************************************************************
 * @brief Number of `extra' drives to reserve for copy operation.
 *
 *********************************************************************/
#define RED_HERRING_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY    1

/*!*******************************************************************
 * @def RED_HERRING_INDEX_RESERVED_FOR_COPY
 *********************************************************************
 * @brief Position reserved for copy operation
 *
 *********************************************************************/
#define RED_HERRING_INDEX_RESERVED_FOR_COPY              0

/*!*******************************************************************
 * @def RED_HERRING_INVALID_DISK_POSITION
 *********************************************************************
 *  @brief Invalid disk position.  Used when searching for a disk 
 *         position and no disk is found that meets the criteria.
 *
 *********************************************************************/
#define RED_HERRING_INVALID_DISK_POSITION ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def RED_HERRING_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief The number of LUs in each raid group.
 *
 *********************************************************************/
#define RED_HERRING_LUNS_PER_RAID_GROUP                 3

/*!*******************************************************************
 * @def RED_HERRING_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define RED_HERRING_CHUNKS_PER_LUN                      3

/*!*******************************************************************
 * @def RED_HERRING_NUM_IO_THREADS
 *********************************************************************
 * @brief   Number of I/O threads for each I/O size.
 *
 *********************************************************************/
#define RED_HERRING_NUM_IO_THREADS                      2

/*!*******************************************************************
 * @def RED_HERRING_NUM_IO_CONTEXT
 *********************************************************************
 * @brief  Two I/O context: (1) large and (1) small.
 *
 *********************************************************************/
#define RED_HERRING_NUM_IO_CONTEXT                      2

/*!*******************************************************************
 * @def RED_HERRING_MAX_IO_SECONDS
 *********************************************************************
 * @brief Max time to run I/O for after the copy operation is complete.
 *
 *********************************************************************/
#define RED_HERRING_MAX_IO_SECONDS                      120

/*!*******************************************************************
 * @def RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of errors before
 *        continuing with the test.
 *
 *********************************************************************/
#define RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR         10


/*!*******************************************************************
 * @def RED_HERRIN_MIN_VALIDATION_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of validations before
 *        continuing with the test.
 *
 *********************************************************************/
/* Geng: following cases has a contribution to VD's error injection counters
 * case 1: IO originated from VD copy (rebuild)
 * case 2: IO sent to a mirroring mode VD from RG
 * buf the case below will not update the min_validations number
 * case 1: VD have to copy a chunk from the source drive to dst drive
 *         a read media error was injected to the source drive,
 *         VD have no way except writting a invalidated pattern with reason XORLIB_SECTOR_INVALID_REASON_COPY_INVALIDATED
 *         xor library will not be called on the path
 *         and min_validations will not be updated
 * the user copy initiated in this testcase will not update min_validations,
 * and further more the randomized IO generated by rdgen could not guarantee leading to a nonzero min_object_validations
 * thus min_object_validations might be zero sometimes and a time out error was found in red_herring_wait_for_errors
 *
 * 2 way to avoid that
 *
 * way_1: set RED_HERRIN_MIN_VALIDATION_COUNT_TO_WAIT_FOR to zero (prefer this one)
 * way_2: don't get error on vd_object
 */
#define RED_HERRIN_MIN_VALIDATION_COUNT_TO_WAIT_FOR      0

/*!*******************************************************************
 * @def RED_HERRING_MIN_RDGEN_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of rdgen ios before
 *        continuing with the test.
 *
 *********************************************************************/
#define RED_HERRING_MIN_RDGEN_COUNT_TO_WAIT_FOR   (RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) 

/*!*******************************************************************
 * @def RED_HERRING_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR
 *********************************************************************
 * @brief We wait for at least this number of errors before
 *        continuing with the test.
 *
 *********************************************************************/
#define RED_HERRING_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR 2

/*!*******************************************************************
 * @def RED_HERRING_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define RED_HERRING_VERIFY_WAIT_SECONDS 120

/*!*******************************************************************
 * @def RED_HERRING_DEFAULT_RAID_GROUP_OFFSET
 *********************************************************************
 * @brief This is the default raid group offset
 *
 *********************************************************************/
#define RED_HERRING_DEFAULT_RAID_GROUP_OFFSET   0x10000ULL

/*!*******************************************************************
 * @def RED_HERRING_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max small I/O size.
 *
 *********************************************************************/
#define RED_HERRING_SMALL_IO_SIZE_BLOCKS        (128)

/*!*******************************************************************
 * @def RED_HERRING_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define RED_HERRING_MAX_IO_SIZE_BLOCKS          (RED_HERRING_TEST_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @def RED_HERRING_MAX_CONFIGS
 *********************************************************************
 * @brief The number of configurations we will test in extended mode.
 *
 *********************************************************************/
#define RED_HERRING_MAX_CONFIGS 10
#define RED_HERRING_MAX_CONFIGS_EXTENDED RED_HERRING_MAX_CONFIGS

/*!*******************************************************************
 * @def RED_HERRING_MAX_CONFIGS_QUAL
 *********************************************************************
 * @brief The number of configurations we will test in qual.
 *
 *********************************************************************/
#define RED_HERRING_MAX_CONFIGS_QUAL 3

/*!*******************************************************************
 * @def RED_HERRING_MAX_TABLES
 *********************************************************************
 * @brief Max number of tables we can test for one raid group.
 *
 *********************************************************************/
#define RED_HERRING_MAX_TABLES 10

/*!*******************************************************************
 * @def RED_HERRING_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with at a time.
 *
 *********************************************************************/
#define RED_HERRING_MAX_RAID_GROUP_COUNT 10

/*!*******************************************************************
 * @def RED_HERRING_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define RED_HERRING_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def RED_HERRING_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.
 *
 *********************************************************************/
#define RED_HERRING_FIRST_POSITION_TO_REMOVE 0

/*!*******************************************************************
 * @def RED_HERRING_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define RED_HERRING_SECOND_POSITION_TO_REMOVE 1

/*!*******************************************************************
 * @def RED_HERRING_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define RED_HERRING_WAIT_MSEC 30000

/*!***************************************************************************
 * @def     RED_HERRING_DE50_RAID5_TABLE5
 *****************************************************************************
 * @brief   Currently DE50 is tracking an issue where any `invalidation' error
 *          will now result in a `Media Error' status.  Table 5 needs to be
 *          updated to expect both correctable and uncorrectable errors based
 *          on the error type (all invalidation error types will result in
 *          uncorrectable errors).
 *
 * @note    `RED_HERRING_DE50_RAID5_TABLE5' MUST BE THE LAST TABLE!!
 *
 * @todo    Fix this defect!
 *
 *****************************************************************************/
#define RED_HERRING_DE50_RAID5_TABLE5  (FBE_U32_MAX)   /*!< By setting this to the terminator the table is disabled. */

/*!*******************************************************************
 * @var     red_herring_pause_params
 *********************************************************************
 * @brief This variable the debug hook locations required for any
 *        test that require a `pause'.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t red_herring_pause_params = { 0 };

/*!*******************************************************************
 * @var red_herring_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t red_herring_test_contexts[RED_HERRING_NUM_IO_CONTEXT];

/*!*******************************************************************
 * @var red_herring_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_logical_error_table_test_configuration_t red_herring_raid_groups_extended[RED_HERRING_MAX_CONFIGS] = 
#else
fbe_test_logical_error_table_test_configuration_t red_herring_raid_groups_extended[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/

            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,    0,          0},
            {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,    1,          1},
            {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,    2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        0,          0},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        1,          1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, RED_HERRING_DE50_RAID5_TABLE5, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        0,          0},
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        1,          1},
            {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX    /* Error tables. */ },
        { 0, 0, 0/* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        0,         0},
            {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        1,         1},
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        2,         1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, RED_HERRING_DE50_RAID5_TABLE5, FBE_U32_MAX  /* Error tables. */},
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {    /* width,    capacity    raid type,                  class,              block size, raid group id,  bandwidth */
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        0,             0},
            {3,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        1,             0},
            {2,         0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,520,        2,             0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX  /* Error tables. */ },
        { 0, 0, 0 /* Drives to power off for each error table */ },
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_logical_error_table_test_configuration_t red_herring_raid_groups_qual[RED_HERRING_MAX_CONFIGS] = 
#else
fbe_test_logical_error_table_test_configuration_t red_herring_raid_groups_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,520,        0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0, FBE_U32_MAX, /* Error tables. */},
        {0, 0 /* Drives to power off for each error table */ },
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,        1,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY, 520,        2,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,        3,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 /* Drives to power off */},
    },
    {
        {
            /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
            {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR, 520,        4,         0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {0, FBE_U32_MAX, /* Error tables. */},
        {0, 0 /* Drives to power off for each error table */ },
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};


/*!*******************************************************************
 * @var red_herring_table_info
 *********************************************************************
 * @brief These are all the parameters we pass to the test.
 *
 *********************************************************************/
fbe_test_logical_error_table_info_t red_herring_table_info[] = 
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
    {1,            FBE_FALSE,        5,     FBE_FALSE, 128},
    {3,            FBE_FALSE,        6,     FBE_FALSE, 70},

    /* Terminator.
     */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};


/*!***************************************************************************
 *          red_herring_get_source_destination_edge_index()
 *****************************************************************************
 *
 * @brief   Based on the virtual drive configuration mode determine the 
 *          `source' (where data is being read from) and the destination (where 
 *          data is being written to) edge index of the virtual drive.
 *
 * @param   vd_object_id - object id of virtual drive performing copy
 * @param   source_edge_index_p - Address of source edge index to populate
 * @param   dest_edge_index_p - Address of destination edge index to populate
 *
 * @return  status
 *
 * @author  
 *  03/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t red_herring_get_source_destination_edge_index(fbe_object_id_t vd_object_id,
                                                                  fbe_edge_index_t *source_edge_index_p,
                                                                  fbe_edge_index_t *dest_edge_index_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;

    /* Initialize our local variables.
     */
    *source_edge_index_p = FBE_EDGE_INDEX_INVALID;
    *dest_edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)       )
    {
        *source_edge_index_p = 0;
        *dest_edge_index_p = 1;
    }
    else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) ||
             (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)       )
    { 
        *source_edge_index_p = 1;
        *dest_edge_index_p = 0;
    }
    else
    {
        /* Else unsupported configuration mode.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: get source and dest for vd obj: 0x%x mode: %d invalid.", 
                   __FUNCTION__, vd_object_id, configuration_mode);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/*********************************************************************
 * end red_herring_get_source_destination_edge_index()
 *********************************************************************/

/*!***************************************************************************
 *          red_herring_generate_error_records()
 *****************************************************************************
 *
 * @brief   Generate the error injection records (but do not enabled them).
 *
 * @param   test_params_p - type of test to run.
 * @param   rg_config_p - Configuration we are testing against.
 * @param   raid_group_count - total entries in above config_p array.
 * @param   copy_position - The raid group position (must be the same for
 *              all raid groups under test) that is being copied.  For this
 *              position we need to inject the error below the the virtual
 *              drive.
 *
 * @return  None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 *****************************************************************************/
static void red_herring_generate_error_records(fbe_test_logical_error_table_info_t *test_params_p,
                                               fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_u32_t raid_group_count,
                                               fbe_u32_t copy_position)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Next load logical error injection with table_num.
     */
    status = fbe_api_logical_error_injection_load_table(test_params_p->table_number, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}
/******************************************
 * end red_herring_generate_error_records()
 ******************************************/

/*!***************************************************************************
 *          red_herring_enable_error_injection_for_raid_groups()
 *****************************************************************************
 * @brief
 *  For a given config enable injection.
 *
 * @param rg_config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above rg_config_p array
 * @param copy_position - Position in raid group that is executing copy operation.
 * @param b_inject_on_destination - FBE_TRUE - When injecting errors to the 
 *          virtual drive, inject on copy destination (default is to inject to the
 *          copy source).
 *                                  FBE_FALSE - Inject virtual drive errors on
 *          the copy source.
 *
 * @return fbe_status_t 
 *
 * @author
 *  05/02/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t red_herring_enable_error_injection_for_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                                       fbe_u32_t raid_group_count,
                                                                       fbe_u32_t copy_position,
                                                                       fbe_bool_t b_inject_on_destination)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           rg_index = 0;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t     vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t           rg_array_length = fbe_test_get_rg_array_length(rg_config_p);
    fbe_edge_index_t    source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t    dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_u32_t           copy_position_bitmask = 0;
    fbe_u32_t           vd_injection_bitmask = 0;
    fbe_u32_t           edge_hook_enable_bitmask = FBE_U32_MAX;
    fbe_api_get_block_edge_info_t block_edge_info;
    fbe_lba_t           virtual_drive_offset = 0;        

    /* Don't enable the edge hook for the copy position.
     */
    copy_position_bitmask = (1 << copy_position);

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
                /* Don't inject on the copy position.
                 */
                fbe_api_logical_error_injection_generate_edge_hook_bitmask(copy_position_bitmask,
                                                                           &edge_hook_enable_bitmask);
                status = fbe_api_logical_error_injection_modify_object(downstream_object_list.downstream_object_list[index],
                                                                       edge_hook_enable_bitmask,
                                                                       0, /* Don't modify the injection lba */
                                                                       FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_logical_error_injection_enable_object(downstream_object_list.downstream_object_list[index], 
                                                                      FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* If it is the correct raid groups position fetch the vd object id.
                 * Then enable error injection for it.
                 */
                if (index == copy_position)
                {
                    /* Get and validate virtual drive object id.
                     */
                    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(downstream_object_list.downstream_object_list[index], 
                                                                                      copy_position, 
                                                                                      &vd_object_id);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

                    /* Get the source and destination edges for the copy operation.
                     * Inject on BOTH positions if enabled.
                     */
                    red_herring_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
                    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
                    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);
                    vd_injection_bitmask = (1 << source_edge_index);
                    if (b_inject_on_destination == FBE_TRUE)
                    {
                        vd_injection_bitmask |= (1 << dest_edge_index);
                    }

                    /* Get the raid group offset for the virtual drive position.
                     */
                    status = fbe_api_get_block_edge_info(downstream_object_list.downstream_object_list[index], copy_position, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    virtual_drive_offset = block_edge_info.offset;
                    status = fbe_api_logical_error_injection_modify_object(vd_object_id,
                                                                           vd_injection_bitmask,
                                                                           virtual_drive_offset,
                                                                           FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    status = fbe_api_logical_error_injection_enable_object(vd_object_id, 
                                                                          FBE_PACKAGE_ID_SEP_0);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }

            }
        }
        else
        {
            /* Don't inject on the copy position.
             */
            fbe_api_logical_error_injection_generate_edge_hook_bitmask(copy_position_bitmask,
                                                                       &edge_hook_enable_bitmask);
            status = fbe_api_logical_error_injection_modify_object(rg_object_id,
                                                                   edge_hook_enable_bitmask,
                                                                   0, /* Don't modify the injection lba */
                                                                   FBE_PACKAGE_ID_SEP_0);
            status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Get and validate virtual drive object id.
             */
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 
                                                                               copy_position, 
                                                                               &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Get the source and destination edges for the copy operation.
             * Inject on BOTH positions if enabled.
             */
            red_herring_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);
            vd_injection_bitmask = (1 << source_edge_index);
            if (b_inject_on_destination == FBE_TRUE)
            {
                vd_injection_bitmask |= (1 << dest_edge_index);
            }

            /* Get the raid group offset for the virtual drive position.
             */
            status = fbe_api_get_block_edge_info(rg_object_id, copy_position, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            virtual_drive_offset = block_edge_info.offset;
            status = fbe_api_logical_error_injection_modify_object(vd_object_id,
                                                                   vd_injection_bitmask,
                                                                   virtual_drive_offset,
                                                                   FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_logical_error_injection_enable_object(vd_object_id, 
                                                                   FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        
        /* Goto next */
        rg_config_p++;
    }
    return status;
}
/**********************************************************
 * end red_herring_enable_error_injection_for_raid_groups()
 **********************************************************/

/*!**************************************************************
 * red_herring_get_total_raid_group_count()
 ****************************************************************
 * @brief
 *  Determine how many total raid group objects we should
 *  be injecting errors on.  For raid 10s we simply
 *  count all the mirror objects.
 *
 * @param config_p - Configuration we are testing against.
 * @param raid_group_count - total entries in above config_p array.
 * @param num_of_copy_positions - Number of position that are being copied.
 *
 * @return  Count of the number of raid groups with error injection enabled.
 *
 * @author
 *  7/27/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_u32_t red_herring_get_total_raid_group_count(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_u32_t num_of_copy_positions)
{
    fbe_u32_t   total_raid_groups_with_error_injection_enabled = 0;
    fbe_u32_t   enabled_raid_groups = 0;
    fbe_u32_t   num_vds_with_error_injection_enabled = 0;
    fbe_u32_t index = 0;
    fbe_u32_t rg_array_length = fbe_test_get_rg_array_length(rg_config_p);

    /* Loop until we find the table number or we hit the terminator.
     */
    for (index=0; index<rg_array_length; index++)
    {
        if ( fbe_test_rg_config_is_enabled(rg_config_p))
        {
            enabled_raid_groups++;
            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                /* Add on all the mirrored pairs.
                 */
                total_raid_groups_with_error_injection_enabled += rg_config_p->width / 2;
            }
            else
            {
                total_raid_groups_with_error_injection_enabled++;
            }
        }
        rg_config_p++;
    }

    /* Add the virtual drive raid groups.
     */
    num_vds_with_error_injection_enabled = (enabled_raid_groups * num_of_copy_positions);
    total_raid_groups_with_error_injection_enabled += num_vds_with_error_injection_enabled;
    return total_raid_groups_with_error_injection_enabled;
}
/**********************************************
 * end red_herring_get_total_raid_group_count()
 **********************************************/

/*!***************************************************************************
 *          red_herring_enable_injection()
 *****************************************************************************
 *
 * @brief   Enable error injection on this configuration.  For any errors
 *          injected to the `copy position' we must use `protocol' error
 *          injection. 
 *
 * @param   test_params_p - type of test to run.
 * @param   rg_config_p - Configuration we are testing against.
 * @param   raid_group_count - total entries in above config_p array.
 * @param   copy_position - The raid group position (must be the same for
 *              all raid groups under test) that is being copied.  For this
 *              position we need to inject the error below the the virtual
 *              drive.
 * @param   b_inject_on_destination - FBE_TRUE - Inject the errors on the
 *              destination of the copy operation (default is to inject on the
 *              source).
 *                                    FBE_FALSE - Inject errors on the source
 *              of the copy operation.
 *
 * @return  None.
 *
 * @author
 *  6/30/2011 - Created. Rob Foley
 *
 *****************************************************************************/
static void red_herring_enable_injection(fbe_test_logical_error_table_info_t *test_params_p,
                                         fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_u32_t raid_group_count,
                                         fbe_u32_t copy_position,
                                         fbe_bool_t b_inject_on_destination)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t       num_raid_groups_with_error_injection_enabled = 0;

    /* Populate the error injection records (but do not start error injection).
     */
    red_herring_generate_error_records(test_params_p,
                                       rg_config_p,
                                       raid_group_count,
                                       copy_position);

    /* This includes striper and mirrors for raid 10s and virtual drives.
     */
    num_raid_groups_with_error_injection_enabled = red_herring_get_total_raid_group_count(rg_config_p, 
                                                                                          raid_group_count,
                                                                                          1 /* Only copy (1) position*/);

    /* Clear the logs.
     */
    fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);

    /* Enable injection on all raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection for table %d==", __FUNCTION__, test_params_p->table_number);
    status = red_herring_enable_error_injection_for_raid_groups(rg_config_p, raid_group_count, copy_position, b_inject_on_destination);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled classes.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, test_params_p->record_count);

    /* We are injecting to each virtual drive (1 per raid group also)
     */
    MUT_ASSERT_INT_EQUAL(stats.num_objects, num_raid_groups_with_error_injection_enabled);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, num_raid_groups_with_error_injection_enabled);
    return;
}
/************************************
 * end red_herring_enable_injection()
 ************************************/

/*!**************************************************************
 * red_herring_disable_error_injection()
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
static void red_herring_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Disable error injection prior to disabling the class so that all error injection stops 
     * across all raid groups at once.  
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Disable the virtual drive class. */
    status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_VIRTUAL_DRIVE, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Raid 10s only enable the mirrors, so just disable the mirror class. */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
    }
    else
    {
        status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/*******************************************
 * end red_herring_disable_error_injection()
 *******************************************/

/*!***************************************************************************
 *          red_herring_rg_get_error_totals()
 *****************************************************************************
 *
 * @brief   For the raid group find the minimum number of errors injected and
 *          validated to the copy position.
 *
 * @param rg_config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above rg_config_p array.
 * @param copy_position - The raid group position that is executing the copy request.
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
static fbe_status_t red_herring_rg_get_error_totals(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_u32_t copy_position,
                                              fbe_u64_t *min_validations_p,
                                              fbe_u64_t *min_count_p,
                                              fbe_u64_t *max_count_p,
                                              fbe_object_id_t *min_validations_object_id_p,
                                              fbe_object_id_t *min_count_object_id_p,
                                              fbe_object_id_t *max_count_object_id_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u64_t       min_validations = *min_validations_p;
    fbe_u64_t       min_count = *min_count_p;
    fbe_u64_t       max_count = *max_count_p;
    fbe_api_logical_error_injection_get_object_stats_t stats;

    /* Get the parent raid group.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* For RAID-10 get the downstream mirror raid groups.
     */
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
            /*! @note Do NOT USE abby_cadabby_rg_get_error_totals_both_sps.
             *        error injection only runs on (1) SP since the copy errors
             *        MUST be injected on the same SP as raid group errors.
             */
            status = fbe_api_logical_error_injection_get_object_stats(&stats, downstream_object_list.downstream_object_list[index]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /*! @note The minimum validations come from the virtual drive that 
             *        is executing the copy operation.
             */
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

            /* If it is the correct raid group position fetch the vd object id.
             */
            if (index == copy_position)
            {
                /* Get and validate virtual drive object id.
                 */
                status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(downstream_object_list.downstream_object_list[index], 
                                                                                   copy_position, 
                                                                                   &vd_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
                status = fbe_api_logical_error_injection_get_object_stats(&stats, vd_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /*! @note The minimum validations come from the virtual drive that 
                 *        is executing the copy operation.
                 */
                if (stats.num_validations < min_validations)
                {
                    min_validations = stats.num_validations;
                    *min_validations_object_id_p = vd_object_id;
                }
                if (stats.num_errors_injected < min_count)
                {
                    min_count = stats.num_errors_injected;
                    *min_count_object_id_p = vd_object_id;
                }
                if (stats.num_errors_injected > max_count)
                {
                    max_count = stats.num_errors_injected;
                    *max_count_object_id_p = vd_object_id;
                }
            } /* end if this is the copy position */
        }
    }
    else
    {
        /* Get the stats for this object.
         */
        status = fbe_api_logical_error_injection_get_object_stats(&stats, rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! @note The minimum validations come from the virtual drive that 
         *        is executing the copy operation.
         */
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

        /* Get and validate virtual drive object id.
         */
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, 
                                                                           copy_position, 
                                                                           &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
        status = fbe_api_logical_error_injection_get_object_stats(&stats, vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! @note The minimum validations come from the virtual drive that 
         *        is executing the copy operation.
         */
        if (stats.num_validations < min_validations)
        {
            min_validations = stats.num_validations;
            *min_validations_object_id_p = vd_object_id;
        }
        if (stats.num_errors_injected < min_count)
        {
            min_count = stats.num_errors_injected;
            *min_count_object_id_p = vd_object_id;
        }
        if (stats.num_errors_injected > max_count)
        {
            max_count = stats.num_errors_injected;
            *max_count_object_id_p = vd_object_id;
        }
    }

    /* Validate that we located the copy position.
     */
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Set the values being returned.
     */
    *min_validations_p = min_validations;
    *min_count_p = min_count;
    *max_count_p = max_count;

    return status;
}
/***********************************************
 * end red_herring_rg_get_error_totals()
 ***********************************************/

/*!***************************************************************************
 * red_herring_get_error_totals_for_config()
 *****************************************************************************
 * @brief
 *  For a given config find the smallest number of errors injected
 *  by the tool.
 *
 * @param rg_config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above rg_config_p array.
 * @param copy_position - The raid group position that is executing the copy request.
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
static fbe_status_t red_herring_get_error_totals_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_u32_t copy_position,
                                                            fbe_u64_t *min_validations_p,
                                                            fbe_u64_t *min_count_p,
                                                            fbe_u64_t *max_count_p,
                                                            fbe_object_id_t *min_validations_object_id_p,
                                                            fbe_object_id_t *min_count_object_id_p,
                                                            fbe_object_id_t *max_count_object_id_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t rg_index = 0;
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
        status = red_herring_rg_get_error_totals(rg_config_p, copy_position,
                                                 min_validations_p, min_count_p, max_count_p,
                                                 min_validations_object_id_p, min_count_object_id_p, max_count_object_id_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        rg_config_p++;
    }
    return status;
}
/***********************************************
 * end red_herring_get_error_totals_for_config()
 ***********************************************/

/*!**************************************************************
 * red_herring_wait_for_errors()
 ****************************************************************
 * @brief
 *  Wait until enough errors were injected.
 *
 * @param test_params_p - type of test to run.
 * @param config_p - Configuration we are testing against.  
 * @param raid_group_count - total entries in above config_p array.
 * @param copy_position - The raid group position that is executing the copy request.
 * @param max_wait_seconds - # of seconds we will wait before assert.          
 *
 * @return fbe_status_t
 *
 * @author
 *  2/10/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t red_herring_wait_for_errors(fbe_test_logical_error_table_info_t *test_params_p,
                                                fbe_test_rg_configuration_t *config_p,
                                                fbe_u32_t raid_group_count,
                                                fbe_u32_t copy_position,
                                                fbe_u32_t max_wait_seconds,
                                                fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = max_wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t raid_type_max_correctable_errors;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter = {0};
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

        status = red_herring_get_error_totals_for_config(config_p, raid_group_count, copy_position,
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
            if ((rdgen_stats.io_count >= RED_HERRING_MIN_RDGEN_COUNT_TO_WAIT_FOR) &&
                (stats.num_errors_injected >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (stats.correctable_errors_detected >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (min_object_injected_errors >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (min_object_validations >= RED_HERRIN_MIN_VALIDATION_COUNT_TO_WAIT_FOR))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s %d msec rdgen ios: 0x%x rdgen errors: 0x%x",
                           __FUNCTION__, elapsed_msec, rdgen_stats.io_count, rdgen_stats.error_count);
                mut_printf(MUT_LOG_TEST_STATUS, "found min errors: %lld and max errors %lld min validations: %lld",
                           (long long)min_object_injected_errors, (long long)max_object_injected_errors, (long long)min_object_validations);
                mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %lld correctables: %lld uncorrectables: %lld",
                           (long long)stats.num_errors_injected, (long long)stats.correctable_errors_detected, (long long)stats.uncorrectable_errors_detected);
                break;
            }
        }
        else if ((test_params_p->number_of_errors > raid_type_max_correctable_errors) &&
                 (test_params_p->b_random_errors == FBE_FALSE))
        {
            /* Should be uncorrectable, check to see that enough uncorrectable errors were
             * injected. 
             */
            if ((rdgen_stats.io_count >= RED_HERRING_MIN_RDGEN_COUNT_TO_WAIT_FOR) &&
                (stats.num_errors_injected >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (stats.uncorrectable_errors_detected >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (rdgen_stats.error_count >= RED_HERRING_MIN_RDGEN_FAILED_COUNT_TO_WAIT_FOR) &&
                (min_object_injected_errors >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s %d msec rdgen ios: 0x%x rdgen errors: 0x%x",
                           __FUNCTION__, elapsed_msec, rdgen_stats.io_count, rdgen_stats.error_count);
                mut_printf(MUT_LOG_TEST_STATUS, "found min errors: %lld and max errors %lld min validations: %lld",
                           (long long)min_object_injected_errors, (long long)max_object_injected_errors, (long long)min_object_validations);
                mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %lld correctables: %lld uncorrectables: %lld",
                           (long long)stats.num_errors_injected, (long long)stats.correctable_errors_detected, (long long)stats.uncorrectable_errors_detected);
                break;
            }
        }
        else if (test_params_p->b_random_errors)
        {
            /* Should be uncorrectable, check to see that enough uncorrectable errors were
             * injected. 
             */
            if ((rdgen_stats.io_count >= RED_HERRING_MIN_RDGEN_COUNT_TO_WAIT_FOR)      &&
                (stats.num_errors_injected >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (min_object_injected_errors >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) &&
                (stats.num_validations >= RED_HERRIN_MIN_VALIDATION_COUNT_TO_WAIT_FOR))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s %d msec rdgen ios: 0x%x rdgen errors: 0x%x",
                           __FUNCTION__, elapsed_msec, rdgen_stats.io_count, rdgen_stats.error_count);
                mut_printf(MUT_LOG_TEST_STATUS, "found min errors: %lld and max errors %lld min validations: %lld",
                           (long long)min_object_injected_errors, (long long)max_object_injected_errors, (long long)min_object_validations);
                mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %lld correctables: %lld uncorrectables: %lld",
                           (long long)stats.num_errors_injected, (long long)stats.correctable_errors_detected, (long long)stats.uncorrectable_errors_detected);
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
        mut_printf(MUT_LOG_TEST_STATUS, "%s found min errors: %lld and max errors %lld min validations: %lld",
                   __FUNCTION__, (long long)min_object_injected_errors, (long long)max_object_injected_errors, (long long)min_object_validations);
        mut_printf(MUT_LOG_TEST_STATUS, "errors injected: %lld correctables: %lld uncorrectables: %lld",
                   (long long)stats.num_errors_injected, (long long)stats.correctable_errors_detected, (long long)stats.uncorrectable_errors_detected);
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
 * end red_herring_wait_for_errors()
 **************************************/

/*!***************************************************************************
 *          red_herring_error_injection_test()
 *****************************************************************************
 *
 * @brief   This method is structure similarly to 
 *          `abby_cadabby_error_injection_test()', but there are (2) differences:
 *              1. This test starts a copy operation prior to starting the
 *                 error injection.
 *              2. This test will translate the `logical' error injection for
 *                 `copy position' into a protocol error injection.
 *          The flow of this test is as follows:
 *              1. Configure the abort, dualsp, and drive replacement settings.
 *              2. Start the I/O and wait a short period.
 *              3. Start the copy operation.
 *              4. Enable error injection.
 *              5. Let I/O and copy proceed before drive removal
 *              6. If enabled, remove the drives
 *              7. If enabled, replace any removed drives
 *              8. Wait for copy operation to complete
 *              9. Wait for sufficient errors to be detected
 *             10. Disable error injection
 *             11. Stop I/O.
 *             12. Validate the I/O status
 *             13. Cleanup
 *
 * @param   test_params_p - Error table information for this instance of the
 * @param   rg_config_p - Configuration we are testing against.
 * @param   raid_group_count - total entries in above config_p array.
 * @param   drives_to_power_off - Number of drives to turn off while we are testing.
 * @param   ran_abort_msecs - If not FBE_TEST_RANDOM_ABORT_TIME_INVALID, then this 
 *              is the number of seconds rdgen should wait before rdgen aborts IO.
 *
 * @return  None.
 *
 * @author
 *  05/01/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void red_herring_error_injection_test(fbe_test_logical_error_table_info_t *test_params_p,
                                             fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t raid_group_count,
                                             fbe_u32_t drives_to_power_off,
                                             fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &red_herring_test_contexts[0];
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_u32_t                   io_seconds;
    fbe_u32_t                   num_contexts = 0;
    fbe_u32_t                   threads = fbe_test_sep_io_get_threads(RED_HERRING_NUM_IO_THREADS);
    fbe_u32_t                   total_threads = 0;
    fbe_bool_t                  b_ran_aborts_enabled;
    fbe_api_rdgen_get_stats_t   rdgen_stats;
    fbe_rdgen_filter_t          filter = {0};
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    fbe_u32_t                   position_to_copy;
    fbe_bool_t                  b_inject_on_copy_destination = FBE_TRUE; /*! @note Currently we inject on BOTH the source and destination. */
    fbe_u32_t                   total_objects_with_error_injection_enabled = 0;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t                   percentage_rebuilt_before_pause = 22;
    fbe_bool_t                  b_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_test_sep_io_dual_sp_mode_t dualsp_io_mode = FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID;
    fbe_sim_transport_connection_target_t original_sp = fbe_api_sim_transport_get_target_server();
    fbe_sim_transport_connection_target_t active_sp = original_sp;
    fbe_sim_transport_connection_target_t passive_sp = original_sp;

    /* This test only works if the I/O and all error injection occur on the same SP.
     */
    if (b_dualsp_mode) {
        /* Must use the CMI Active SP.
         */
        fbe_test_sep_get_active_target_id(&active_sp);
        passive_sp = fbe_test_sep_get_peer_target_id(active_sp);
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /*! @note Since the virtual drive is injecting error below the raid group we
     *        we do NOT want the raid group to shoot drives when it gets multi-bit
     *        CRC errors that were injected by the downstream virtual drive.
     */
    fbe_test_sep_util_set_rg_library_debug_flags(rg_config_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_SEND_CRC_ERR_TO_PDO);

    /* This includes striper and mirrors for raid 10s and virtual drives
     * that are in copy.
     */
    total_objects_with_error_injection_enabled = red_herring_get_total_raid_group_count(rg_config_p, 
                                                                                        raid_group_count,
                                                                                        1 /* Only (1) position will execute copy*/);

    /* Set the copy position to the `next position to remove'.
     */
    position_to_copy = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Initialize the filter
     */
    fbe_api_rdgen_filter_init(&filter, 
                              FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_LUN, 
                              FBE_PACKAGE_ID_SEP_0,
                              0 /* edge index */);

    /* Step 1:  Configure the abort, dualsp, and drive replacement settings.
     */
    status = fbe_api_rdgen_reset_stats();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Setup the abort mode based on ran_abort_msecs
     */
    b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    status = fbe_test_sep_io_configure_abort_mode(b_ran_aborts_enabled, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We run random write/read/compare with (2) I/O sizes:
     *      o Large I/O with (# threads / 2)
     *      o Small I/O with (# threads / 2) ==> total threads
     *  is the the number of threads allocated.
     */
    fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       threads, /* threads */
                                       RED_HERRING_MAX_IO_SIZE_BLOCKS,
                                       b_ran_aborts_enabled, 
                                       FBE_FALSE /* Peer I/O not supported */);
    total_threads += threads;
    num_contexts++;
    fbe_test_sep_io_setup_standard_rdgen_test_context(&context_p[RED_HERRING_NUM_IO_CONTEXT - 1],
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       threads, /* threads */
                                       RED_HERRING_SMALL_IO_SIZE_BLOCKS,
                                       b_ran_aborts_enabled, 
                                       FBE_FALSE /* Peer I/O not supported */);
    total_threads += threads;
    num_contexts++;
    MUT_ASSERT_TRUE(num_contexts <= RED_HERRING_NUM_IO_CONTEXT);

    /*! @note We MUST send all I/O on the same SP as the error injection for copy.
     */
    if (b_dualsp_mode)
    {
        dualsp_io_mode = FBE_TEST_SEP_IO_DUAL_SP_MODE_LOCAL_ONLY;
        mut_printf(MUT_LOG_TEST_STATUS, "Running I/O mode %d",
                   dualsp_io_mode);
    }
    status = fbe_test_sep_io_configure_dualsp_io_mode(b_dualsp_mode, dualsp_io_mode);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure that I/O runs until we stop it, even if -fbe_io_seconds was used 
     */
    fbe_api_rdgen_io_specification_set_num_ios(&(context_p->start_io.specification),
                                               0,0,0);
    fbe_api_rdgen_io_specification_set_num_ios(&(context_p[RED_HERRING_NUM_IO_CONTEXT - 1].start_io.specification),
                                               0,0,0);                                                       

    if ((test_params_p->number_of_errors > 1) || (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0))
    {
        /* We are either injecting uncorrectables or we are non-redundant. 
         * Setup rdgen to continue on error. 
         */
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                            FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p[1].start_io.specification,
                                                            FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        /* set the random abort time to invalid here as we will not be injecting random aborts in this case
          * This is needed because we will be checking this futher while analyzing the errors reported
          */
        status = fbe_api_rdgen_clear_random_aborts(&context_p->start_io.specification);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_clear_random_aborts(&context_p[RED_HERRING_NUM_IO_CONTEXT - 1].start_io.specification);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /*! @todo currently we do not do abort testing in this case.  
         * If the caller asked for abort testing then just return since we have nothing new 
         * to test here.  This will shorten the test by avoiding injecting aborts for this case. 
         */
        if (ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s skip abort test for table %d", __FUNCTION__, test_params_p->table_number);
            fbe_api_rdgen_test_context_destroy(context_p);
            fbe_api_rdgen_test_context_destroy(&context_p[1]);
            return;
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

    /* Reduce the spare swap-in timer from 5 minutes (300 seconds) to
     * (1) second.
     */
	fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    /* Step 2:  Run our I/O test.
     *          We allow the user to input a time to run I/O. 
     *          By default io seconds is 0. 
     *  
     * By default we will run I/O until we see enough errors is encountered.
     */
    io_seconds = fbe_test_sep_util_get_io_seconds();
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, RED_HERRING_NUM_IO_CONTEXT );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Let /IO run for (1) second before starting the copy operation
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s running I/O for %d seconds before starting copy operation ==", 
               __FUNCTION__, 5000/5000);
    fbe_api_sleep(5000);

    /* Validate that I/O was started successfully.
     */
    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_INVALID, context_p->status);
    MUT_ASSERT_INT_EQUAL(rdgen_stats.requests, RED_HERRING_NUM_IO_CONTEXT);

    /* Step 3:  Enable injection on both SPs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable table: %d ==", __FUNCTION__, test_params_p->table_number);
    red_herring_enable_injection(test_params_p, rg_config_p, raid_group_count, position_to_copy, b_inject_on_copy_destination);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection enabled ==", __FUNCTION__);

    /* Step 4: Start a user proactive copy request and pause at 10% complete.
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
                                                               &red_herring_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 10% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully.", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 5:  Let I/O run and then validate sufficient errors detected.
     */
    if (io_seconds > 0)
    {
        /* Let it run for a reasonable time before stopping it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s letting I/O (%d threads) run for %d seconds ==", __FUNCTION__, total_threads, io_seconds);
        fbe_api_sleep(io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s running I/O (%d threads) for %d seconds ==", __FUNCTION__, total_threads, io_seconds);
    }

    /* Step 6: Disable automatic swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_disable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Power off drives for this table.
     */
    if ((drives_to_power_off != 0) && (drives_to_power_off != FBE_U32_MAX))
    {
        /* Refresh how many extra drives are required.
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing random drives. ==", __FUNCTION__);
        big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_power_off, 
                                   FBE_FALSE, /* We do not plan on re-inserting this drive later. */ 
                                   remove_delay, /* msecs between removals */
                                   FBE_DRIVE_REMOVAL_MODE_RANDOM);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing drives - complete ==", __FUNCTION__);
    }

    /* Step 6: Enable automatic swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_enable_spare_select_unconsumed();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8:  Let drives rebuild.
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

    /* Step 9: Run until we have seen enough errors injected.
     *         We need to do this here before we allow the swap out to occur. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting: %d seconds for errors  ==", __FUNCTION__, RED_HERRING_MAX_IO_SECONDS);
    status = red_herring_wait_for_errors(test_params_p, rg_config_p, raid_group_count, position_to_copy,
                                         RED_HERRING_MAX_IO_SECONDS, ran_abort_msecs); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Validate virtual drive hits 50% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &red_herring_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 50% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 11: Wait for the proactive copy to complete.  The raid group must
     *         not be degraded if the copy operation is to make progress.
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &red_herring_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__);

    /* Make sure that we received enough errors.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, test_params_p->record_count);

    /* Total objects that have error injection enabled
     */
    MUT_ASSERT_INT_EQUAL(stats.num_objects, total_objects_with_error_injection_enabled);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, total_objects_with_error_injection_enabled);
    /*! check that errors occurred, and that errors were validated.
     */
    MUT_ASSERT_TRUE(stats.num_errors_injected >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR); 
    MUT_ASSERT_TRUE(stats.num_failed_validations == 0);
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_TRUE((stats.num_validations >= RED_HERRING_MIN_ERROR_COUNT_TO_WAIT_FOR) );
                       
    }

    /* Step 12:  Stop logical error injection.  We do this before stopping I/O 
     *          since if we allow error injection to continue then it will take 
     *          a while to stop I/O. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    red_herring_disable_error_injection(rg_config_p);

    /* Step 13:  Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, RED_HERRING_NUM_IO_CONTEXT);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Step 14:  Make sure the io status is as expected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate I/O status ==", __FUNCTION__);
    abby_cadabby_validate_io_status(test_params_p, rg_config_p, raid_group_count, 
                                    context_p, num_contexts,
                                    drives_to_power_off, ran_abort_msecs);

    /* Validate the error log info.
     */
    abby_cadabby_validate_event_log_errors();
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate I/O status - complete ==", __FUNCTION__);

    /* Step 15:  Cleanup.  Destroy all the objects the logical error injection 
     *           service is tracking.  This will allow us to start fresh when 
     *           we start the next test with number of errors and number of 
     *           objects reported by lei.
     */
    status = fbe_api_logical_error_injection_destroy_objects();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* populate the required extra disk count in rg config */
    fbe_test_rg_populate_extra_drives(rg_config_p, RED_HERRING_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

    /* Restore the default spare trigger time
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Re-enable shooting the PDO by clearing the `don't shoot pdo' flag.
     */
    fbe_test_sep_util_set_rg_library_debug_flags(rg_config_p, 0);

    /* Set the SP back to original 
     */
    if (b_dualsp_mode) {
        /* Set SP back to orignal
         */
        if (original_sp != active_sp) {
            status = fbe_api_sim_transport_set_target_server(original_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }

    return;
}
/******************************************
 * end red_herring_error_injection_test()
 ******************************************/

/*!**************************************************************
 *          red_herring_run_test_on_rg_config()
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
 *  05/01/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static void red_herring_run_test_on_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t  raid_group_count = fbe_test_get_rg_count(rg_config_p);
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t test_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_only_degraded = fbe_test_sep_util_get_cmd_option("-degraded_only");
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_test_logical_error_table_test_configuration_t * curr_config_p = NULL;
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;

    /*! @note Automatic sparing is enabled
     */
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    /* Context has pointer to config on which test is currently running
     */
    curr_config_p = (fbe_test_logical_error_table_test_configuration_t *) context_p;
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

    /* Loop over tables until we hit the terminator.
     */
    for (test_index = 0; (curr_config_p->error_table_num[test_index] != FBE_U32_MAX); test_index++)
    {
        fbe_test_logical_error_table_info_t *table_info_p = NULL;
    
        fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
        
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
                   curr_config_p->raid_group_config->b_bandwidth, raid_type_string_p, raid_group_count);
    
        status = abby_cadabby_get_table_info(curr_config_p->error_table_num[test_index], &table_info_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);

        /* Determine if we should inject aborts or not.
         */
        ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
        if (test_level > 0) {
            if ((fbe_random() % 2) == 0) {
                ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC;
            }
        }

        /* Write the background pattern to seed this element size before starting the
         * test. 
         */
        big_bird_write_background_pattern();
        red_herring_error_injection_test(table_info_p, rg_config_p, raid_group_count, 
                                         curr_config_p->num_drives_to_power_down[test_index], 
                                         ran_abort_msecs);
    }

    /* Make sure we did not get any trace errors.  We do this in between each
     * raid group test since it stops the test sooner. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end red_herring_run_test_on_rg_config()
 ******************************************/

/*!**************************************************************
 * red_herring_run_test_on_rg_config_with_extra_disk()
 ****************************************************************
 * @brief
 *  This function is wrapper of fbe_test_run_test_on_rg_config(). 
 *  This function populates extra disk count in rg config and then start 
 *  the test.
 * 
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 *
 * @return  None.
 *
 * @author
 *  06/01/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void red_herring_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{

    if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }


    /* run the test */
    fbe_test_run_test_on_rg_config(rg_config_p, context_p, test_fn,
                                   luns_per_rg,
                                   chunks_per_lun);

    return;
}
/********************************************************
 * end red_herring_run_test_on_rg_config_with_extra_disk()
 ********************************************************/
/*!**************************************************************
 * red_herring_run_tests()
 ****************************************************************
 * @brief
 *  Run error injection test to several LUNs.
 *
 * @param   config_p - Pointer to array of array configurations to run 
 * @param   ran_abort_msec - Determines is abort is enabled
 * @param   raid_type - The raid type to execute for this iteration      
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/
static void red_herring_run_tests(fbe_test_logical_error_table_test_configuration_t *config_p, 
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
    

    /* For each set of raid groups (grouped by raid type), run the
     * red_herring: copy with error injection, test.
     */
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

        /*! @note Run only (1) group of raid groups for the raid type specified.
         */
        if (raid_type == rg_config_p[0].raid_type)
        {
            /* Now create the raid groups and run the tests
            */
            red_herring_run_test_on_rg_config_with_extra_disk(rg_config_p, &config_p[raid_type_index], 
                                       red_herring_run_test_on_rg_config,
                                       RED_HERRING_LUNS_PER_RAID_GROUP,
                                       RED_HERRING_CHUNKS_PER_LUN); 
        }
    }
    return;
}
/**************************************
 * end red_herring_run_tests()
 **************************************/

/*********************************************************************************
 * BEGIN Test, Setup, Cleanup.
 *********************************************************************************/
/*!**************************************************************
 * red_herring_test()
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

static void red_herring_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &red_herring_raid_groups_extended[0];
    }
    else
    {
        config_p = &red_herring_raid_groups_qual[0];
    }

    /* Run the test.
     */
    red_herring_run_tests(config_p, FBE_TEST_RANDOM_ABORT_TIME_INVALID, raid_type);

    return;
}
/******************************************
 * end red_herring_test()
 ******************************************/
void red_herring_raid1_test(void)
{
    red_herring_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void red_herring_raid3_test(void)
{
    red_herring_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void red_herring_raid5_test(void)
{
    red_herring_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void red_herring_raid6_test(void)
{
    red_herring_test(FBE_RAID_GROUP_TYPE_RAID6);
}
void red_herring_raid10_test(void)
{
    red_herring_test(FBE_RAID_GROUP_TYPE_RAID10);
}


/*!**************************************************************
 * red_herring_simulation_setup()
 ****************************************************************
 * @brief
 *  Setup the simulation configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/24/2011 - Created. Rob Foley
 *
 ****************************************************************/
static void red_herring_simulation_setup(fbe_test_logical_error_table_test_configuration_t *config_p_qual,
                                         fbe_test_logical_error_table_test_configuration_t *config_p_extended)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;
    fbe_u32_t test_index;
    fbe_u32_t config_count;

    /* Only load the physical config in simulation.
     */
    if (!fbe_test_util_is_simulation())
    {
        MUT_FAIL_MSG("Should only be called in simulation mode");
        return;
    }

    if (test_level == 0)
    {
        /* Qual.
         */
        config_p = config_p_qual;
    }
    else
    {
        /* Extended. 
         */
        config_p = config_p_extended;
    }

    config_count = abby_cadabby_get_config_count(config_p);
    for (test_index = 0; test_index < config_count; test_index++)
    {
        /* Randomize block size and any other randomized value.
         */
        fbe_test_sep_util_init_rg_configuration_array(&config_p[test_index].raid_group_config[0]);
        fbe_test_rg_setup_random_block_sizes(&config_p[test_index].raid_group_config[0]);
        fbe_test_sep_util_randomize_rg_configuration_array(&config_p[test_index].raid_group_config[0]);

        /* Initialize the number of extra drive required by each rg.
         */
        fbe_test_rg_populate_extra_drives(&config_p[test_index].raid_group_config[0], RED_HERRING_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);
    }
    return;
}
/**************************************
 * end red_herring_simulation_setup()
 **************************************/

/*!**************************************************************
 * red_herring_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param   raid_type              
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
static void red_herring_setup(fbe_raid_group_type_t raid_type)
{
    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

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
            config_p = &red_herring_raid_groups_extended[0];
        }
        else
        {
            config_p = &red_herring_raid_groups_qual[0];
        }
        red_herring_simulation_setup(&red_herring_raid_groups_qual[0],
                                     &red_herring_raid_groups_extended[0]);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);
        elmo_load_sep_and_neit();
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    fbe_test_common_util_test_setup_init();
    
    return;
}
/**************************************
 * end red_herring_setup()
 **************************************/

/* Setup for single sp tests.
 */
void red_herring_raid1_setup(void)
{
    red_herring_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void red_herring_raid10_setup(void)
{
    red_herring_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void red_herring_raid5_setup(void)
{
    red_herring_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void red_herring_raid3_setup(void)
{
    red_herring_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void red_herring_raid6_setup(void)
{
    red_herring_setup(FBE_RAID_GROUP_TYPE_RAID6);
}

/*!**************************************************************
 * red_herring_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the red_herring test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void red_herring_cleanup(void)
{
    //fbe_status_t status;
    //status = fbe_api_rdgen_stop_tests(red_herring_test_contexts, RED_HERRING_NUM_IO_CONTEXT);

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
 * end red_herring_cleanup()
 ******************************************/

/*!**************************************************************
 * red_herring_dualsp_test()
 ****************************************************************
 * @brief
 *  Run an error test dual sp.
 *
 * @param   raid_type          
 *
 * @return None.   
 *
 * @author
 *  6/28/2011 - Created. Rob Foley
 *
 ****************************************************************/
static void red_herring_dualsp_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &red_herring_raid_groups_extended[0];
    }
    else
    {
        config_p = &red_herring_raid_groups_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run the test.
     */
    red_herring_run_tests(config_p, FBE_TEST_RANDOM_ABORT_TIME_INVALID, raid_type);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end red_herring_dualsp_test()
 ******************************************/
void red_herring_dualsp_raid1_test(void)
{
    red_herring_dualsp_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void red_herring_dualsp_raid3_test(void)
{
    red_herring_dualsp_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void red_herring_dualsp_raid5_test(void)
{
    red_herring_dualsp_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void red_herring_dualsp_raid6_test(void)
{
    red_herring_dualsp_test(FBE_RAID_GROUP_TYPE_RAID6);
}
void red_herring_dualsp_raid10_test(void)
{
    red_herring_dualsp_test(FBE_RAID_GROUP_TYPE_RAID10);
}

/*!**************************************************************
 * red_herring_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for the test.
 *
 * @param   raid_type             
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  6/28/2011 - Created. Rob Foley
 *
 ****************************************************************/
static void red_herring_dualsp_setup(fbe_raid_group_type_t raid_type)
{
    /* error injected during the test */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            config_p = &red_herring_raid_groups_extended[0];
        }
        else
        {
            config_p = &red_herring_raid_groups_qual[0];
        }

        red_herring_simulation_setup(&red_herring_raid_groups_qual[0],
                                     &red_herring_raid_groups_extended[0]);

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
 * end red_herring_dualsp_setup()
 ******************************************/

/* Setup for dual sp tests.
 */
void red_herring_dualsp_raid1_setup(void)
{
    red_herring_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void red_herring_dualsp_raid10_setup(void)
{
    red_herring_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void red_herring_dualsp_raid5_setup(void)
{
    red_herring_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void red_herring_dualsp_raid3_setup(void)
{
    red_herring_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void red_herring_dualsp_raid6_setup(void)
{
    red_herring_dualsp_setup(FBE_RAID_GROUP_TYPE_RAID6);
}

/*!**************************************************************
 * red_herring_dualsp_cleanup()
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

void red_herring_dualsp_cleanup(void)
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
 * end red_herring_dualsp_cleanup()
 ******************************************/
/*********************************************************************************
 * END Test, Setup, Cleanup.
 *********************************************************************************/
/*************************
 * end file red_herring_test.c
 *************************/


