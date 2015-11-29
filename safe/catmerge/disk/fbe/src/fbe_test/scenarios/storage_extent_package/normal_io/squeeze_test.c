/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file squeeze_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains tests for corrupt crc.
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
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * squeeze_short_desc = "Validate raid's handling of lost data with and without `check checksum' set";
char * squeeze_long_desc ="\
The Squeeze scenario has (2) components:    \n\
    (1) Inject checksum errors into Read requests with and without the  \n\
        `check checksum' flag set.  \n\
    (2) Generate Write requests that contain different flavors of the `Corrupt  \n\
        CRC' data.  \n\
Validate that if any host data was lost (either thru raid verify or Corrupt CRC) \n\
that:   \n\
    o If the `check checksum' flag is cleared no error is reported and those    \n\
      blocks that were lost contain the expected `data lost' pattern.           \n\
    o If the `check checksum' flag is set the request fails with a status of    \n\
      `media error' and that all the data is transferred and that any `data lost'   \n\
      blocks are properly invalidated.  \n\
This test exercises all raid types.  Only non-degraded raid groups are tested.  \n\
Supported raid test levels are 0 and 1.  The primary differences between test   \n\
level 0 and test level 1 is the number of raid groups of each type that are used. \n\
\n"\
"\
STEP 1: configure all raid groups and LUNs.     \n\
        -Each raid group contains a single LUN  \n\
        -The configuration for both test levels contains at least (1) raid group \n\
         that use 512-bps drives for each raid group type.  \n\
\n"\
"\
STEP 2: For each set of raid groups (the raid groups in a set are always of the \n\
        same type). \n\
        -Write a background pattern \n\
\n"\
"\
STEP 3: Inject uncorrectable Read errors with the `check checksum' flag set.  For \n\
        the number of injection records perform the following.  \n\
        -Setup read error tables such that any read will result in uncorrectable    \n\
         errors (i.e. the associated lost blocks will be invalidated and will   \n\
         thus contain the `raid verify' invalidated pattern with bad CRC).      \n\
        -Issue a single Read request for each LUN and validate: \n\
            +Request fails with status `media error'    \n\
            +The number of invalidated blocks detected by rdgen is the number   \n\
             expected.  \n\
            +For (10) iterations run the following  \n\
                oUsing a random lba and size issue `Corrupt Data' write \n\
                oIssue Read to same range and (except for RAID-0) validate  \n\
                    >Read completes successfully    \n\
                    >No invalidations   \n\
        -Issue a verify for the raid group and confirm: \n\
            +Same number of errors are detected as the Read request \n\
\n"\
"\
STEP 4: For each set of raid groups \n\
        -Write a background pattern \n\
\n"\
"\
STEP 5: Inject CRC errors on Reads without the `check checksum' flag set.   \n\
        -Clear the verify report    \n\
        -Enable error injection \n\
        -For each of the error records execute the following    \n\
            +Issue single Read request that encounters errors   \n\
                oValidate that Read request completes successfully  \n\
                oValidate that the correct number of `bad CRC' blocks are   \n\
                 detected by rdgen  \n\
                oValidate that no invalidated blocks are detected by rdgen  \n\
            +Issue single Read request that doesn't encounter errors        \n\
                oValidate that Read completes successfully  \n\
                oValidate that no `bad CRC' blocks are detected by rdgen    \n\
                oValidate that no invalidated blocks are detected by rdgen  \n\
            +Wait for any background verify to complete \n\
\n"\
"\
STEP 6: For each set of raid groups \n\
        -Write a background pattern \n\
\n"\
"\
STEP 7: Populated random blocks of Write request with `Corrupt CRC' with    \n\
        `check checksum' set    \n\
        -Start (3) threads with Corrupt CRC/Read/Compare    \n\
            +rdgen will set the `corrupt CRC' payload flags \n\
            +rdgen will automatically validate that the blocks written with \n\
             `Corrupt CRC' are properly returned on the Read request        \n\
            +rdgen will validate that those blocks that were not corrupted  \n\
             contain the correct data   \n\
        -Start (1) thread with Write/Read/Compare (to validate that only the \n\
         blocks that should be affected by `Corrupt CRC' are invalidated).  \n\
            +rdgen will validate that those blocks that were not corrupted  \n\
             contain the correct data   \n\
        -Let I/O runs for a few seconds \n\
\n"\
"\
STEP 8: Destroy raid groups and LUNs    \n\
\n"\
"\
Test Specific Switches: \n\
        None            \n\
\n"\
"\
Outstanding Issues: \n\
        We are not testing other invalidated patterns that could occur in \n\
        Write data: \n\
            + `Data Lost - Invalidated'     \n\
            + `RAID Verify - Invalidated'   \n\
        We are not testing `bad memory':    \n\
            + Write data with bad CRC that is not an invalidated pattern    \n\
\n"\
"\
Description Last Updated:   \n\
        10/07/2011          \n\
\n";

/* The number of LUNs in our raid group.
 */
#define SQUEEZE_TEST_LUNS_PER_RAID_GROUP 1

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define SQUEEZE_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/* The number of blocks in a raid group bitmap chunk.
 */
#define SQUEEZE_TEST_RAID_GROUP_CHUNK_SIZE  FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH

/*!*******************************************************************
 * @def SQUEEZE_IO_SECONDS
 *********************************************************************
 * @brief Time to run I/O for.
 *
 *********************************************************************/
#define SQUEEZE_IO_SECONDS 5

/*!*******************************************************************
 * @def SQUEEZE_TEST_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define SQUEEZE_TEST_MAX_RAID_GROUP_COUNT 10

/*!*******************************************************************
 * @def SQUEEZE_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define SQUEEZE_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def SQUEEZE_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SQUEEZE_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def SQUEEZE_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SQUEEZE_LARGE_IO_SIZE_MAX_BLOCKS SQUEEZE_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def SQUEEZE_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SQUEEZE_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def SQUEEZE_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define SQUEEZE_VERIFY_WAIT_SECONDS 30

/*!*******************************************************************
 * @def SQUEEZE_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SQUEEZE_WAIT_SEC 60

/*!*******************************************************************
 * @def SQUEEZE_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SQUEEZE_WAIT_MSEC (1000 * SQUEEZE_WAIT_SEC)

/*!*******************************************************************
 * @def SQUEEZE_MAX_CONTEXT_PER_TEST
 *********************************************************************
 * @brief Maximum number of rdgen contexts used for any of the squeeze
 *        tests.
 *
 *********************************************************************/
#define SQUEEZE_MAX_CONTEXT_PER_TEST    3

/*!*******************************************************************
 * @def SQUEEZE_MAX_CONFIGS
 *********************************************************************
 * @brief The number of configurations we will test in extended mode.
 *
 *********************************************************************/
#define SQUEEZE_MAX_CONFIGS 14
#define SQUEEZE_MAX_CONFIGS_EXTENDED SQUEEZE_MAX_CONFIGS

/*!*******************************************************************
 * @def SQUEEZE_MAX_CONFIGS_QUAL
 *********************************************************************
 * @brief The number of configurations we will test in qual.
 *
 *********************************************************************/
#define SQUEEZE_MAX_CONFIGS_QUAL 1

/*!*******************************************************************
 * @var squeeze_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t squeeze_test_contexts[SQUEEZE_TEST_LUNS_PER_RAID_GROUP * SQUEEZE_MAX_CONFIGS];

/*!*******************************************************************
 * @def SQUEEZE_TEST_ELEMENT_SIZES
 *********************************************************************
 * @brief this is the number of element sizes to test.
 *
 *********************************************************************/
#define SQUEEZE_TEST_ELEMENT_SIZES 2

/*!*******************************************************************
 * @var squeeze_raid_type_enum
 *********************************************************************
 * @brief Enumeration of the raid types in the order they are tested
 *
 *********************************************************************/
typedef enum squeeze_raid_type_enum
{
    SQUEEZE_RAID0 = 0,
    SQUEEZE_RAID10,
    SQUEEZE_MIRROR,
    SQUEEZE_RAID5,
    SQUEEZE_RAID3,
    SQUEEZE_RAID6,
    SQUEEZE_MAX_RAID_TYPE
} squeeze_raid_type_enum_t;

fbe_test_logical_error_table_test_configuration_t squeeze_raid_groups_extended[] = 
{
    { /* Raid 0 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
            {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
            {1,       0x6000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          1},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          1},
            {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
            {1,       0x6000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 10 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {16,       0x2a000,    FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {6,        0x2a000,    FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
            {4,        0xe000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
            {16,       0x2a000,    FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            3,          1},
            {6,        0x2a000,    FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            4,          1},
            {4,        0xe000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            5,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 1 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,          0},
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 5 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          0},
            {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            2,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            3,          1},
            {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            4,          1},
            {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            5,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 3 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {9,       0x22000,    FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            1,          0},
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            2,          1},
            {9,       0x22000,    FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            3,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 6 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,          0},
            {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            2,          0},
            {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            3,          1},
            {6,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            4,          1},
            {16,      0x22000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            5,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

fbe_test_logical_error_table_test_configuration_t squeeze_raid_groups_qual[] = 
{
    { /* Raid 0 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 10 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {16,       0x1E000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 1 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 5 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 3 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    { /* Raid 6 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        { 0, 1, FBE_U32_MAX, /* Error tables. */ },
        { 0, 0, 0, 0 /* Drives to power off */},
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

/*!@todo Explain the RAID5 block operation injected errors as noted in the brief
 */

/*!*******************************************************************
 * @struct squeeze_invalid_on_error_error_case_t
 *********************************************************************
 * @brief This is a single test case for the squeeze_run_invalid_on_error_test() test.
 *        Note however that each of the lba_to_read and expected_num_errors_injected
 *           array members corresponds to a raid group configuration and one
 *           or more error injection records (beginning with the record contained
 *           in the same test case that holds the array). All error injection
 *           records are active at the same time; lba offsets prevent multiple
 *           record groups from being invoked at the same time.
 *           Invalid LBAs in the lba_to_read arrays prevent running a test case
 *           against a raid group which can't support it (not wide enough).
 *        Note that there are 5x2 items in these arrays, because the test_count
 *           is defined by the raid group count
 *        Note that expected_num_errors_injected is the result of counting:
 *           1 error for each read disk position with any error injected (host read)
 *           2 errors for each disk position with any error injected (verify stripe read and retry)
 *          We compare for error_count > expected because there are 1-4 extra errors injected at the
 *             block operation level, but not predictably. 
 *
 *********************************************************************/
typedef struct squeeze_invalid_on_error_error_case_s
{
    fbe_lba_t lba_to_read[SQUEEZE_TEST_ELEMENT_SIZES][SQUEEZE_TEST_MAX_RAID_GROUP_COUNT/2];
    fbe_u16_t expected_num_errors_injected[SQUEEZE_TEST_ELEMENT_SIZES][SQUEEZE_TEST_MAX_RAID_GROUP_COUNT/2];
    fbe_u32_t blocks_to_read;
    fbe_bool_t b_correctable;

    /*! Error to inject.
     */
    fbe_api_logical_error_injection_record_t record;
}
squeeze_invalid_on_error_error_case_t;

#define SQUEEZE_INVALID_ON_ERROR_TEST_COUNT 30
#define SQUEEZE_INVALID_ON_ERROR_TEST_COUNT_QUAL 2
squeeze_invalid_on_error_error_case_t squeeze_invalid_on_error_cases[SQUEEZE_MAX_CONFIGS][SQUEEZE_INVALID_ON_ERROR_TEST_COUNT] = 
{
    { /* Raid 0, */
        {
            {{0 /*w=5,*/, 0 /*w=3,*/, 0 /*w=16,*/, 0 /*w=1*/, FBE_LBA_INVALID}, /* lbas for z=0x80 */
                {0 /*w=5,*/, 0 /*w=3,*/, 0 /*w=16,*/, 0 /*w=1*/, FBE_LBA_INVALID}}, /* lbas for z=0x400 */ 
            {{1 /*w=5,*/, 1 /*w=3,*/, 1 /*w=16,*/, 1 /*w=1*/, 0 /*not used*/}, /* expected injected error count for z=0x400 */
                {1 /*w=5,*/, 1 /*w=3,*/, 1 /*w=16,*/, 1 /*w=1*/, 0 /*not used*/}}, /* expected injected error count for z=0x400 */ 
            1, /* blocks */
            FBE_FALSE, /* correctable */
            { 0x1,  /* pos_bitmap */
                0x10, /* width */
                0x0,  /* lba */
                0x1,  /* blocks */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,      /* error type */
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
                0x0,  /* error count */
                0x15, /* error limit */
                0x0,  /* skip count */
                0x15, /* skip limit */
                0x1,  /* error adjcency */
                0x0,  /* start bit */
                0x0,  /* number of bits */
                0x0,  /* erroneous bits */
                0x0,},/* crc detectable */
        },
        {
            {{0x301, 0x201, 0x881, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x481, 0x481, 0x481, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{1, 1, 1, 0, 0}, {1, 1, 1, 0, 0}}, /* 1 read */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x81, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x605, 0x405, 0x1105, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x905, 0x905, 0x905, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 1, 1, 0, 0}, {1, 1, 1, 0, 0}}, /* 1 read */
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x105, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x907, FBE_LBA_INVALID, 0x1987, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xd87, FBE_LBA_INVALID, 0xd87, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 0, 1, 0, 0}, {1, 0, 1, 0, 0}}, /* 1 read */
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x18, 0x10, 0x187, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xc03, FBE_LBA_INVALID, 0x2203, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x1203, FBE_LBA_INVALID, 0x1203, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 0, 1, 0, 0}, {1, 0, 1, 0, 0}}, /* 1 read */
            6, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x205, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xc80, 0x780, 0x2800, 0x280, FBE_LBA_INVALID}, {0x280, 0x280, 0x280, 0x280, FBE_LBA_INVALID}}, /* lba */
            {{1, 1, 1, 1, 0}, {1, 1, 1, 1, 0}}, /* 1 read */
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x280, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xf87, 0x987, 0x3087, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x707, 0x707, 0x707, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 1, 1, 0, 0}, {1, 1, 1, 0, 0}}, /* 1 read */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x307, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1282, 0xb82, 0x3902, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xb82, 0xb82, 0xb82, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 1, 1, 0, 0}, {1, 1, 1, 0, 0}}, /* 1 read */
            0x3, /* blocks */ FBE_FALSE, /* correctable */
            { 0xC, 0x10, 0x382, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1580, FBE_LBA_INVALID, 0x4180, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x2000, FBE_LBA_INVALID, 0x4c00, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 0, 1, 0, 0}, {1, 0, 1, 0, 0}}, /* 1 read */
            0x12, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x400, 0x12, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1881, FBE_LBA_INVALID, 0x4a01, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x2481, FBE_LBA_INVALID, 0x5081, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{1, 0, 1, 0, 0}, {1, 0, 1, 0, 0}}, /* 1 read */
            0x10, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x481, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1900, FBE_LBA_INVALID, 0x5000, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{2, 0, 2, 0, 0}, {2, 0, 2, 0, 0}}, /* 1 read */
            0xA1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x3, 0x10, 0x500, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1C02, FBE_LBA_INVALID, 0x5882, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{2, 0, 2, 0, 0}, {2, 0, 2, 0, 0}}, /* 1 read */
            0xA0, /* blocks */ FBE_FALSE, /* correctable */
            { 0x6, 0x10, 0x582, 0x7E, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1F03, FBE_LBA_INVALID, 0x6103, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{2, 0, 2, 0, 0}, {2, 0, 2, 0, 0}}, /* 1 read */
            0xA6, /* blocks */ FBE_FALSE, /* correctable */
            { 0xC, 0x10, 0x600, 0x80, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x2201, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{3, 0, 3, 0, 0}, {3, 0, 3, 0, 0}}, /* 1 read */
            0x100, /* blocks */ FBE_FALSE, /* correctable */
            { 0x19, 0x10, 0x680, 0x100, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },

    { /* Raid 10, Inject errors on each mirror pair to cause uncorrectables */
        {
            {{0, 0, 0, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, 0, 0, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{6, 6, 6, 0, 0}, {6, 6, 6, 0, 0}}, /* 1 read + 1 retry + 2 verify + 2 retry verify */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x403, 0x183, 0x103, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x83, 0x83, 0x83, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{6, 6, 6, 0, 0}, {6, 6, 6, 0, 0}}, /* 1 read + 1 retry + 2 verify + 2 retry verify */
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x83, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x83, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x83, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x83, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x803, 0x303, 0x203, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x103, 0x103, 0x103, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{6, 6, 6, 0, 0}, {6, 6, 6, 0, 0}}, /* 1 read + 1 retry + 2 verify + 2 retry verify */
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x103, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x103, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x103, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x103, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },

    { /* Raid 1, Inject errors on both pairs to cause uncorrectables */
        {
            {{0, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{6, 0, 0, 0, 0}, {6, 0, 0, 0, 0}}, /* 1 read + 1 retry + 2 verify + 2 retry verify */ 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x81, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x81, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{6, 0, 0, 0, 0}, {6, 0, 0, 0, 0}}, /* 1 read + 1 retry + 2 verify + 2 retry verify */ 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x81, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x81, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x103, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x103, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{6, 0, 0, 0, 0}, {6, 0, 0, 0, 0}}, /* 1 read + 1 retry + 2 verify + 2 retry verify */ 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x103, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x103, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x185, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x185, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 0, 0, 0}, {5, 0, 0, 0, 0}}, /* 1 read + 2 verify + 2 retry verify */ 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x185, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x185, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x271, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x271, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 0, 0, 0}, {5, 0, 0, 0, 0}}, /* 1 read + 2 verify + 2 retry verify */ 
            12, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x271, 0xC, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            12, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x271, 0xC, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x300, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x300, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 0, 0, 0}, {5, 0, 0, 0, 0}}, /* 1 read + 2 verify + 2 retry verify */ 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x300, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x300, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x381, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x381, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 0, 0, 0}, {5, 0, 0, 0, 0}}, /* 1 read + 2 verify + 2 retry verify */ 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x381, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x381, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x402, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x402, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 0, 0, 0}, {5, 0, 0, 0, 0}}, /* 1 read + 2 verify + 2 retry verify */ 
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x402, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x402, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x482, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x482, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 0, 0, 0}, {5, 0, 0, 0, 0}}, /* 1 read + 2 verify + 2 retry verify */ 
            21, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x482, 0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            21, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x482, 0x21, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },

    { /* Raid 5, Inject errors on two positions to cause uncorrectables */
        {
            {{0, FBE_LBA_INVALID, 0x580, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, FBE_LBA_INVALID, 0x2c00, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{5, 0, 5, 0, 0}, {5, 0, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x201, FBE_LBA_INVALID, 0xd01, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x81, FBE_LBA_INVALID, 0x2c81, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 5, 0, 0}, {5, 0, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x81, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x81, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x480, FBE_LBA_INVALID, 0x1500, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x500, FBE_LBA_INVALID, 0x3100, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 5, 0, 0}, {5, 0, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x100, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x100, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_TRUE, /* correctable */
            { 0x8, 0x10, 0x105, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x707, FBE_LBA_INVALID, 0x1d07, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x987, FBE_LBA_INVALID, 0x3587, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 5, 0, 0}, {5, 0, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x187, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x187, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x901, 0x401, 0x2481, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xa01, 0x201, 0x3601, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 5, 0, 0}, {5, 5, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x201, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x201, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xb81, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xe81, FBE_LBA_INVALID, 0x3a81, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 5, 0, 0}, {5, 5, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            0xa1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x281, 0xa1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0xa1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x281, 0xa1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xd80, FBE_LBA_INVALID, 0x3400, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xF00, FBE_LBA_INVALID, 0x3B00, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 0, 5, 0, 0}, {5, 0, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x300, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x300, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xF81, FBE_LBA_INVALID, 0x2481, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xF81, FBE_LBA_INVALID, 0x3B81, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 5, 0, 0}, {5, 5, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x381, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x381, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1181, 0x881, 0x4301, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 5, 0, 0}, {5, 5, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x401, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x401, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1383, 0x983, 0x4a83, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 5, 0, 0}, {5, 5, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x483, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x483, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x1500, FBE_LBA_INVALID, 0x5180, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x1500, FBE_LBA_INVALID, 0x6D00, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 5, 0, 0}, {5, 5, 5, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            0xa0, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x500, 0xa0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0xa0, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x500, 0xa0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },

        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },

    { /* Raid 3, inject errors on a two positions to cause uncorrectables. */
        {
            {{0, 0x200, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, 0x1000, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{5, 5, 0, 0, 0}, {5, 5, 0, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x205, 0x605, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x85, 0x1085, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 0, 0, 0}, {5, 5, 0, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x85, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x85, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x480, 0xa80, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x500, 0x1500, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 0, 0, 0}, {5, 5, 0, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x100, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x100, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x700, 0xf00, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x980, 0x1980, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 0, 0, 0}, {5, 5, 0, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x180, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x180, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x901, 0x1301, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xa01, 0x1a01, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{5, 5, 0, 0, 0}, {5, 5, 0, 0, 0}},  /* 1 read + 2 verify + 2 retry verify */
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x201, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x201, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },

    { /* Raid 6, inject errors on 3 positions to cause uncorrectables */
        {
            {{0, 0x100, 0x600, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, 0x800, 0x3000, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x185, 0x385, 0xd85, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x485, 0xc85, 0x3485, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x85, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x85, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x85, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x201, 0x501, 0x1401, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x101, 0x901, 0x3101, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x101, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x101, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x101, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x385, 0x785, 0x1b85, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x585, 0xd85, 0x3585, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x185, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x185, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            7, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x185, 0x7, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x400, 0x900, 0x2200, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x200, 0xa00, 0x3200, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x200, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x200, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x200, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x583, 0xb83, 0x2983, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x683, 0xe83, 0x3683, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x283, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x283, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x283, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x605, 0xd05, 0x3005, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x305, 0xb05, 0x3305, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x305, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x305, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x305, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x705, 0xf05, 0x3705, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x385, 0xb85, 0x3385, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x385, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x385, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x385, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x885, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{7, 7, 7, 0, 0}, {7, 7, 7, 0, 0}},  /* 1 read + 3 verify + 3 retry verify */ 
            0xa0, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x405, 0xa0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0xa0, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x405, 0xa0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0xa0, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x405, 0xa0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },

        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },
};


/*!*******************************************************************
 * @struct fbe_api_logical_error_injection_record_t
 *********************************************************************
 * @brief This is a single test case for the squeeze_run_read_do_not_check_crc_test() test.
 *        Note however that each of the lba_to_read and degraded_pos_bitmap
 *           array members corresponds to a raid group configuration and one
 *           or more error injection records (beginning with the record contained
 *           in the same test case that holds the array). All error injection
 *           records are active at the same time; lba offsets prevent multiple
 *           record groups from being invoked at the same time.
 *           Invalid LBAs in the lba_to_read arrays prevent running a test case
 *           against a raid group which can't support it (not wide enough).
 *        Note that there are 5x2 items in these arrays, because the test_count
 *           is defined by the raid group count
 *
 *********************************************************************/
typedef struct squeeze_do_not_check_crc_error_case_s
{
    fbe_lba_t lba_to_read[SQUEEZE_TEST_ELEMENT_SIZES][SQUEEZE_TEST_MAX_RAID_GROUP_COUNT/2];
    fbe_u16_t degraded_pos_bitmap[SQUEEZE_TEST_ELEMENT_SIZES][SQUEEZE_TEST_MAX_RAID_GROUP_COUNT/2];
    fbe_u32_t blocks_to_read;
    fbe_bool_t b_correctable;

    /*! Error to inject.
     */
    fbe_api_logical_error_injection_record_t record;
}
squeeze_do_not_check_crc_error_case_t;

#define SQUEEZE_READ_DO_NOT_CHECK_CRC_TEST_COUNT 10
#define SQUEEZE_READ_DO_NOT_CHECK_CRC_TEST_COUNT_QUAL 2
squeeze_do_not_check_crc_error_case_t squeeze_read_do_not_check_crc_cases[SQUEEZE_MAX_CONFIGS][SQUEEZE_INVALID_ON_ERROR_TEST_COUNT] = 
{
    { /* Raid 0 */
        {
            {{0 /*w=5*/, 0 /*w=3*/, 0 /*w=16*/, 0 /*w=1*/, FBE_LBA_INVALID},  /* z=0x80 */
             {0 /*w=5*/, 0 /*w=3*/, 0 /*w=16*/, 0/*w=1*/, FBE_LBA_INVALID}}, /* z=0x400 */ 
            {{0x0 /*w=5,*/, 0x0 /*w=3,*/, 0x0 /*w=16,*/, 0x0 /*w=1*/, 0x0 /*not used*/}, /* degraded pos bitmaps for z=0x80 */
                {0x0 /*w=5,*/, 0x0 /*w=3,*/, 0x0 /*w=16,*/, 0x0 /*w=1*/, 0x0 /*not used*/}}, /* degr pos bitmaps for z=0x400 */ 
            1, /* blocks */
            FBE_FALSE, /* correctable */
            { 0x1,  /* pos_bitmap */
                0x10, /* width */
                0x0,  /* lba */
                0x1,  /* blocks */
                FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,      /* error type */
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS, /* error mode */
                0x0,  /* error count */
                0x15, /* error limit */
                0x0,  /* skip count */
                0x15, /* skip limit */
                0x1,  /* error adjcency */
                0x0,  /* start bit */
                0x0,  /* number of bits */
                0x0,  /* erroneous bits */
                0x0,},/* crc detectable */
        },
        {
            {{0x301, 0x201, 0x881, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x481, 0x481, 0x481, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x81, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x605, 0x405, 0x1105, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x905, 0x905, 0x905, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x105, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x907, FBE_LBA_INVALID, 0x1987, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xd87, FBE_LBA_INVALID, 0xd87, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x187, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0xc02, FBE_LBA_INVALID, 0x2202, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x1202, FBE_LBA_INVALID, 0x1202, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            0x15, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x202, 0x15, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },
    { /* Raid 10 */
        {
            {{0, 0, 0, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, 0, 0, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{0x2, 0x2, 0x2, 0x0, 0x0}, {0x2, 0x2, 0x2, 0x0, 0x0}}, /* remove mirror drive of error drive 0x1 */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x403, 0x183, 0x103, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x83, 0x83, 0x83, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0x2, 0x2, 0x2, 0x0, 0x0}, {0x2, 0x2, 0x2, 0x0, 0x0}}, /* remove mirror drive of error drive 0x1 */
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x83, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        /* The next 3 cases are meaningless, since RAID 10 injects errors on the underlying mirror, which has only 2 drives
         */
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x100, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID,FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x180, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x200, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },
    { /* Raid 1 */
        {
            {{0, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{0x2, 0x0, 0x0, 0x0, 0x0}, {0x2, 0x0, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x83, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x83, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0x2, 0x0, 0x0, 0x0, 0x0}, {0x2, 0x0, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x83, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x105, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x105, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x0, 0x0, 0x0, 0x0}, {0x2, 0x0, 0x0, 0x0, 0x0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x105, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x180, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x180, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x0, 0x0, 0x0, 0x0}, {0x2, 0x0, 0x0, 0x0, 0x0}}, 
            0x12, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x180, 0x12, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x201, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x201, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x0, 0x0, 0x0, 0x0}, {0x2, 0x0, 0x0, 0x0, 0x0}}, 
            8, /* blocks */ FBE_FALSE, /* correctable */
            { 0x1, 0x10, 0x201, 0x8, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },
    { /* Raid 5 */
       {
            {{0x0, FBE_LBA_INVALID, 0x580, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{0x2, 0x0, 0x2, 0x0, 0x0}, {0x2, 0x0, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x303, FBE_LBA_INVALID, 0xe03, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x883, FBE_LBA_INVALID, 0x3483, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0x2, 0x0, 0x2, 0x0, 0x0}, {0x2, 0x0, 0x2, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x83, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x485, FBE_LBA_INVALID, 0x1505, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x505, FBE_LBA_INVALID, 0x3105, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x0, 0x2, 0x0, 0x0}, {0x2, 0x0, 0x2, 0x0, 0x0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x105, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x781, 0x381, 0x1d81, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xd81, 0x581, 0x3981, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x4, 0x4, 0x4, 0x0, 0x0}, {0x4, 0x4, 0x4, 0x0, 0x0}}, 
            0xf, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x181, 0xf, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x800, FBE_LBA_INVALID, 0x2380, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x200, FBE_LBA_INVALID, 0x2e00, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x0, 0x2, 0x0, 0x0}, {0x2, 0x0, 0x2, 0x0, 0x0}}, 
            0x12, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x200, 0x12, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },
    { /* Raid 3 */
       {
            {{0x0, 0x200, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0, 0x1000, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{0x2, 0x2, 0x0, 0x0, 0x0}, {0x2, 0x2, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x303, 0x703, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x883, 0x1883, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0x2, 0x2, 0x0, 0x0, 0x0}, {0x2, 0x2, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x83, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x485, 0xa85, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x505, 0x1505, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x2, 0x0, 0x0, 0x0}, {0x2, 0x2, 0x0, 0x0, 0x0}}, 
            3, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x105, 0x3, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x780, 0xf80 , FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0xd80, 0x1d80, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x4, 0x4, 0x0, 0x0, 0x0}, {0x4, 0x4, 0x0, 0x0, 0x0}}, 
            16, /* blocks */ FBE_FALSE, /* correctable */
            { 0x2, 0x10, 0x180, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x805, 0x1205, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x205, 0x1205, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x2, 0x2, 0x0, 0x0, 0x0}, {0x2, 0x2, 0x0, 0x0, 0x0}}, 
            0x13, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x205, 0x13, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },
    { /* Raid 6 */
       {
            {{FBE_LBA_INVALID, 0x80, 0x580, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, 0x400, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}},
            {{0x0, 0xc, 0xc, 0x0, 0x0}, {0x0, 0xc, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x0, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x187, 0x387, 0xd87, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x487, 0xc87, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0x3, 0x3, 0x3, 0x0, 0x0}, {0x3, 0x3, 0x0, 0x0, 0x0}}, 
            1, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x87, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, 0x505, 0x1405, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, 0x905, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0x6, 0x6, 0x0, 0x0}, {0x0, 0x6, 0x0, 0x0, 0x0}}, 
            2, /* blocks */ FBE_FALSE, /* correctable */
            { 0x8, 0x10, 0x105, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{0x380, 0x780, 0x1b80, FBE_LBA_INVALID, FBE_LBA_INVALID}, {0x580, 0xd80, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x9, 0x30, 0x30, 0x0, 0x0}, {0x9, 0x30, 0x30, 0x0, 0x0}}, 
            16, /* blocks */ FBE_FALSE, /* correctable */
            { 0x4, 0x10, 0x180, 0x10, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        {
            {{FBE_LBA_INVALID, 0x885, 0x2185, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, 0x605, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}}, /* lba */
            {{0x0, 0xc, 0xc, 0x0, 0x0}, {0x0, 0xc, 0x0, 0x0, 0x0}}, 
            5, /* blocks */ FBE_FALSE, /* correctable */
            { 0x10, 0x10, 0x205, 0x5, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
                0x0, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,},
        },
        { /* terminator*/
            {{FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}, {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_LBA_INVALID}} , /* lba */
            {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 
            0, /* blocks */ FBE_TRUE, /* correctable */
            { 0x0, 0x0, 0x0, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_INVALID, FBE_API_LOGICAL_ERROR_INJECTION_MODE_INVALID,
              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,}
        },
    },

};

fbe_u32_t squeeze_get_raid_type_index(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t index;
    switch (raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
            index = SQUEEZE_RAID0;
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            index = SQUEEZE_RAID10;
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            index = SQUEEZE_MIRROR;
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            index = SQUEEZE_RAID5;
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            index = SQUEEZE_RAID3;
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            index = SQUEEZE_RAID6;
            break;
        default:
            MUT_FAIL_MSG("unknown raid type");
            break;
    };
    return index;
}


/*!**************************************************************
 * squeeze_set_specific_drives_to_remove()
 ****************************************************************
 * @brief
 *  This function will populate the rg config table with the
 *  specific set of drives to be removed for the test.
 *
 * @param config_p - raid_group.               
 *
 * @return fbe_u32_t - number of drives to remove.  
 *
 ****************************************************************/
fbe_u32_t squeeze_set_specific_drives_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_u16_t degraded_pos_bitmap)
{
    fbe_u32_t bitmap_index;
    fbe_u32_t drives_index = 0;
    fbe_u32_t num_to_remove = 0;

    /* Loop through the bitmap, and set the drive if we find a bit set.
     */
    for (bitmap_index = 0; bitmap_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; bitmap_index++)
    {
        if ((degraded_pos_bitmap & (0x1 << bitmap_index)) != 0)
        {
            rg_config_p->specific_drives_to_remove[drives_index] = bitmap_index;
            drives_index++;
            num_to_remove++;
        }
    }
    return (num_to_remove);
}
/**************************************
 * end squeeze_set_specific_drives_to_remove()
 **************************************/

/*!**************************************************************
 * squeeze_get_raid_type_invalid_on_error_case_count()
 ****************************************************************
 * @brief
 *  Get the number of invalid on error cases for a given raid type
 *
 * @param config_p - raid_type_index               
 *
 * @return fbe_u32_t - error case count
 *
 ****************************************************************/
fbe_u32_t squeeze_get_raid_type_invalid_on_error_case_count(fbe_u32_t raid_type_index)
{
    fbe_u32_t count = 0;
    squeeze_invalid_on_error_error_case_t * error_cases_p = squeeze_invalid_on_error_cases[raid_type_index];
    while (error_cases_p->record.blocks != 0)
    {
        error_cases_p++;
        count++;
    }

    return count;
}
/**************************************
 * end squeeze_get_raid_type_invalid_on_error_case_count()
 **************************************/

/*!**************************************************************
 * squeeze_get_raid_type_crc_error_case_count()
 ****************************************************************
 * @brief
 *  Get the number of crc error cases for a given raid type
 *
 * @param config_p - raid_type_index               
 *
 * @return fbe_u32_t - error case count
 *
 ****************************************************************/
fbe_u32_t squeeze_get_raid_type_crc_error_case_count(fbe_u32_t raid_type_index)
{
    fbe_u32_t count = 0;
    squeeze_do_not_check_crc_error_case_t * error_cases_p = squeeze_read_do_not_check_crc_cases[raid_type_index];
    while (error_cases_p->record.blocks != 0)
    {
        error_cases_p++;
        count++;
    }

    return count;
}
/**************************************
 * end squeeze_get_raid_type_crc_error_case_count()
 **************************************/



/*!**************************************************************
 * squeeze_run_invalid_on_error_test()
 ****************************************************************
 * @brief
 * Cause uncorrectables and verify invalid blocks are returned
 *
 * @param context_p - context to execute.
 * @param element_index - Element index for this test loop.
 * 
 * @return None.   
 *
 * @author
 *  4/16/10 - Kevin Schleicher Created
 *
 ****************************************************************/

void squeeze_run_invalid_on_error_test(fbe_api_rdgen_context_t *context_p,
                                       fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = in_rg_config_p;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t hook_object_id;
    fbe_u32_t error_record_index;
    fbe_u32_t width_index;
    fbe_u32_t rg_index;
    fbe_u32_t  raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t test_count;
    fbe_u32_t raid_type_index = squeeze_get_raid_type_index(rg_config_p->raid_type);
    fbe_u32_t element_index;
    fbe_u64_t num_existing_errors_injected;
    fbe_u64_t num_new_errors_injected;
 
    if (test_level == 0)
    {
        test_count = SQUEEZE_INVALID_ON_ERROR_TEST_COUNT_QUAL;
    }
    else
    {
        test_count = squeeze_get_raid_type_invalid_on_error_case_count(raid_type_index);
    }

    /* Set all the verify hooks ahead of time, since errors on one rg add to counts on others
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index ++,
                                                     rg_config_p ++)
    {
        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {            
            continue;
        }

        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            hook_object_id = downstream_object_list.downstream_object_list[0];
        }
        else
        {
            hook_object_id = rg_object_id;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "%s adding debug hook for object:0x%x\n", 
                   __FUNCTION__, hook_object_id);

        status = fbe_api_scheduler_add_debug_hook(hook_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = abby_cadabby_wait_for_raid_group_verify(rg_object_id, SQUEEZE_VERIFY_WAIT_SECONDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    rg_config_p = in_rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index ++,
                                                     rg_config_p ++)
    {
        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {            
            continue;
        }
        element_index = rg_config_p->b_bandwidth;
        /* We do each width twice, once for low bandwidth, once for high bandwidth
         */
        width_index = rg_index % ((raid_group_count+1)/2);

        /* Now run the test for this test case.
         */
        fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                   &lun_object_id);
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);

        /* Clear the verify report.
         */
        status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Disable all the records.
         */
        status = fbe_api_logical_error_injection_disable_records(0, 128);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // We need to insert 3 records for raid 6 in order to inject an error
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6 && test_count == SQUEEZE_INVALID_ON_ERROR_TEST_COUNT_QUAL)
        {
            test_count++; 
        }

        /* Add our own records.
         */
        for (error_record_index = 0; error_record_index < test_count; error_record_index++)
        {
            status = fbe_api_logical_error_injection_create_record(&(squeeze_invalid_on_error_cases[raid_type_index][error_record_index].record));
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* Since we are purposefully injecting errors, only trace those sectors that 
         * result `critical' (i.e. for instance error injection mis-matches) errors.
         */
        fbe_test_sep_util_reduce_sector_trace_level();

        /* Enable injection on all raid groups.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);

        /* RAID10 can't have errors injected, they go into the mirror groups below the stripe
         */
        if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
        {
            status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
        }
        else
        {
            status = fbe_api_logical_error_injection_enable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* We should have no records, but enabled classes.
         */
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(stats.num_records, test_count);

        /* Run a single IO for each entry in our error record table.
         */
        for (error_record_index = 0; error_record_index < test_count; error_record_index++)
        {
            /* If the lba is 'invalid' then this the test is to be skipped.
             */
            if (squeeze_invalid_on_error_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index] != FBE_LBA_INVALID)
            {
                /* We only want the errors for this test case, but they accumulate in stats, so grab the pre-test value
                 */
                status = fbe_api_logical_error_injection_get_stats(&stats);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                num_existing_errors_injected = stats.num_errors_injected;

                mut_printf(MUT_LOG_TEST_STATUS, "== %s Sending IO LBA: 0x%llX, blocks 0x%x ==", __FUNCTION__,
                        (unsigned long long)squeeze_invalid_on_error_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                        squeeze_invalid_on_error_cases[raid_type_index][error_record_index].blocks_to_read);

                /* Send I/O for hitting the media error.
                 */
                status = fbe_api_rdgen_send_one_io(context_p,
                                                   lun_object_id,
                                                   FBE_CLASS_ID_INVALID,
                                                   FBE_PACKAGE_ID_SEP_0,
                                                   FBE_RDGEN_OPERATION_READ_ONLY,
                                                   FBE_RDGEN_LBA_SPEC_FIXED,
                                                   squeeze_invalid_on_error_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                                                   squeeze_invalid_on_error_cases[raid_type_index][error_record_index].blocks_to_read,
                                                   0, 0, 0,
                                                   FBE_API_RDGEN_PEER_OPTIONS_INVALID);

                /* Make sure there was an error. (since we injected an error for this lba)
                 */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);

                /* One block for every block sent should have been invalidated.
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, squeeze_invalid_on_error_cases[raid_type_index][error_record_index].blocks_to_read);

                status = fbe_api_logical_error_injection_get_stats(&stats);
                num_new_errors_injected = stats.num_errors_injected - num_existing_errors_injected;

                status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Make sure that we received enough errors.
                 */
                /* check that enough errors occurred, and that errors were validated
                 */
                MUT_ASSERT_TRUE(stats.num_failed_validations == 0);
                MUT_ASSERT_TRUE(num_new_errors_injected >= squeeze_invalid_on_error_cases[raid_type_index][error_record_index].expected_num_errors_injected[element_index][width_index]);
                MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
                MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.media_error_count, 0);

                /* We only expect media error. 
                 */
                if (context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                            __FUNCTION__, context_p->start_io.status.block_status);
                    MUT_FAIL_MSG("block status from rdgen was unexpected");
                }

                /* We do not expect failure or aborted errors.
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.aborted_error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_failure_error_count, 0);

                status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }

        /* Stop logical error injection.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
        status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Restore the sector trace level to it's default.
         */
        fbe_test_sep_util_restore_sector_trace_level();
    }

    /* Clear all the verify hooks at the end after all cases and configs have run
     */
    rg_config_p = in_rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index ++,
                                                     rg_config_p ++)
    {
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                          &rg_object_id);

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            hook_object_id = downstream_object_list.downstream_object_list[0];
        }
        else
        {
            hook_object_id = rg_object_id;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "%s removing debug hook for object:0x%x\n", 
                   __FUNCTION__, hook_object_id);

        status = fbe_api_scheduler_del_debug_hook(hook_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = abby_cadabby_wait_for_raid_group_verify(rg_object_id, SQUEEZE_VERIFY_WAIT_SECONDS);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
}
/*!**************************************************************
 * squeeze_run_read_do_not_check_crc_test()
 ****************************************************************
 * @brief
 * Cause crc errors, and set the do not check checksum flag 
 * and verify bad crc's are returned.
 *
 * @param context_p - context to execute.
 * @param element_index - Element index for this test loop.
 * 
 * @return None.   
 *
 * @author
 *  5/19/10 - Kevin Schleicher Created
 *
 ****************************************************************/
void squeeze_run_read_do_not_check_crc_test(fbe_api_rdgen_context_t *context_p,
                                            fbe_test_rg_configuration_t *in_rg_config_p)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t *rg_config_p = in_rg_config_p;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_u32_t error_record_index;
    fbe_u32_t width_index;
    fbe_u32_t rg_index;
    fbe_u32_t  raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t test_count;
    fbe_u32_t raid_type_index = squeeze_get_raid_type_index(rg_config_p->raid_type);
    fbe_u32_t element_index;
    fbe_u32_t num_drives_to_remove;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};

    if (test_level == 0)
    {
        test_count = SQUEEZE_READ_DO_NOT_CHECK_CRC_TEST_COUNT_QUAL;
    }
    else
    {
        test_count = squeeze_get_raid_type_crc_error_case_count(raid_type_index);
    }

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    for ( rg_index = 0; rg_index < raid_group_count; rg_index++,
                                                     rg_config_p++)
    {
        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {            
            continue;
        }

        element_index = rg_config_p->b_bandwidth;

        /* We do each width twice, once for low bandwidth, once for high bandwidth
         */
        width_index = rg_index % ((raid_group_count+1)/2);

        /* Now run the test for this test case.
         */
        fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                              &lun_object_id);
        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                     &rg_object_id);

        
        /* If we are a mirror we want to force I/Os to the primary side.  Incomplete write verify does this for us.
         */
        status = fbe_api_scheduler_add_debug_hook(rg_object_ids[rg_index],
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_raid_group_initiate_verify(rg_object_id, FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Clear the verify report.
         */
        status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Disable all the records.
         */
        status = fbe_api_logical_error_injection_disable_records(0, 128);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Add our own records.
         */
        for (error_record_index = 0; error_record_index < test_count; error_record_index++)
        {
            status = fbe_api_logical_error_injection_create_record(&(squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].record)); 
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* Since we purposefully injecting errors, only trace those sectors that 
         * result `critical' (i.e. for instance error injection mis-matches) errors.
         */
        fbe_test_sep_util_reduce_sector_trace_level();

        /* Enable injection on all raid groups.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);

        /* RAID10 can't have errors injected, they go into the mirror groups below the stripe
         */
        if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
        {
            status = fbe_api_logical_error_injection_enable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
        }
        else
        {
            status = fbe_api_logical_error_injection_enable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* We should have no records, but enabled classes.
         */
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(stats.num_records, test_count);

        /* Run a single IO for each entry in our error record table.
         */
        for (error_record_index = 0; error_record_index < test_count; error_record_index++)
        {
            /* If the lba is 'invalid' then this the test is to be skipped.
             */
            if (squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index] != FBE_LBA_INVALID)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "== %s Don't check crc: Sending IO LBA: 0x%llX, blocks 0x%x ==", __FUNCTION__,
                        (unsigned long long)squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                        squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read);

                /* Send I/O for hitting the crc error.
                 */
                status = fbe_api_rdgen_send_one_io(context_p,
                                                   lun_object_id,
                                                   FBE_CLASS_ID_INVALID,
                                                   FBE_PACKAGE_ID_SEP_0,
                                                   FBE_RDGEN_OPERATION_READ_ONLY,
                                                   FBE_RDGEN_LBA_SPEC_FIXED,
                                                   squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                                                   squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read,
                                                   FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC, 
                                                   0, 0,
                                                   FBE_API_RDGEN_PEER_OPTIONS_INVALID);

                /* Make sure no error.
                 */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

                /* One block for every block sent should have found a crc error, no
                 * invalidated blocks
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.bad_crc_blocks_count, squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 0);

                /* check that no errors occurred, and that errors were validated.
                 */
                status = fbe_api_logical_error_injection_get_stats(&stats);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                MUT_ASSERT_TRUE(stats.num_failed_validations == 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.media_error_count, 0);

                /* We only expect success.
                 */
                if (context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                            __FUNCTION__, context_p->start_io.status.block_status);
                    MUT_FAIL_MSG("block status from rdgen was unexpected");
                }

                /* We do not expect failure or aborted errors.
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.aborted_error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_failure_error_count, 0);

                mut_printf(MUT_LOG_TEST_STATUS, "== %s Don't check crc, miss error: Sending IO LBA: 0x%llX, blocks 0x%x ==", __FUNCTION__,
                        (unsigned long long)squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index]
                          + squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read,
                        squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read);

                /* Send I/O for missing the crc error.
                 */
                status = fbe_api_rdgen_send_one_io(context_p,
                                                   lun_object_id,
                                                   FBE_CLASS_ID_INVALID,
                                                   FBE_PACKAGE_ID_SEP_0,
                                                   FBE_RDGEN_OPERATION_READ_ONLY,
                                                   FBE_RDGEN_LBA_SPEC_FIXED,
                                                   squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index]
                                                     + squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read,
                                                   squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read,
                                                   FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC, 
                                                   0, 0,
                                                   FBE_API_RDGEN_PEER_OPTIONS_INVALID);

                /* Make sure no error was found.
                 */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

                /* No Blocks should have found a crc error.
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.bad_crc_blocks_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 0);

                /* Raid 0's will be uncorrectable; so test them below with other degraded arrays, without removing any drives.
                */
                if (raid_type_index != SQUEEZE_RAID0)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "== %s Check crc: Sending IO LBA: 0x%llX, blocks 0x%x ==", __FUNCTION__,
                            (unsigned long long)squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                            squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read);

                    /* Send I/O for hitting the crc error, but allow check crc's
                    */
                    status = fbe_api_rdgen_send_one_io(context_p,
                                                       lun_object_id,
                                                       FBE_CLASS_ID_INVALID,
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                                       FBE_RDGEN_LBA_SPEC_FIXED,
                                                       squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                                                       squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read,
                                                       0, 0, 0,
                                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
                    /* Make sure no error was found.
                    */
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

                    /* All CRC errors should have been fixed
                    */
                    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.bad_crc_blocks_count, 0);
                    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 0);
                }

                /* Now degrade the array and check that we get errors
                 */
                /* First remove drive(s) to degrade the RAID Group.
                 * We will reinsert when done to allow further testing with these configurations.
                 */
                if (raid_type_index != SQUEEZE_RAID0)  /* RAID 0 is already "degraded", i.e., non-redundant */
                {
                    num_drives_to_remove = squeeze_set_specific_drives_to_remove(rg_config_p,
                                             squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].degraded_pos_bitmap[element_index][width_index]);

                    big_bird_remove_all_drives(rg_config_p, 1, num_drives_to_remove,
                                               FBE_TRUE,    /* We are reinserting the same drive*/
                                               0,    /* do not wait between removals */
                                               FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
                }

                mut_printf(MUT_LOG_TEST_STATUS, "== %s Check crc degraded: Sending IO LBA: 0x%llX, blocks 0x%x ==", __FUNCTION__,
                        (unsigned long long)squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                        squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read);

                /* Send I/O for hitting the crc error, but allow check crc's
                */
                status = fbe_api_rdgen_send_one_io(context_p,
                                                   lun_object_id,
                                                   FBE_CLASS_ID_INVALID,
                                                   FBE_PACKAGE_ID_SEP_0,
                                                   FBE_RDGEN_OPERATION_READ_ONLY,
                                                   FBE_RDGEN_LBA_SPEC_FIXED,
                                                   squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].lba_to_read[element_index][width_index],
                                                   squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read,
                                                   0, 0, 0,
                                                   FBE_API_RDGEN_PEER_OPTIONS_INVALID);

                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);

                /* One block for every block sent should have been invalidated.
                */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 
                                     squeeze_read_do_not_check_crc_cases[raid_type_index][error_record_index].blocks_to_read);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.bad_crc_blocks_count, 0);


                /* Restore degraded array
                 */
                if (raid_type_index != SQUEEZE_RAID0)  /* RAID 0 is already "degraded", i.e., non-redundant */
                {
                    big_bird_insert_all_drives(rg_config_p, 1, num_drives_to_remove, 
                                               FBE_TRUE /* We are reinserting the same drive*/);
                    big_bird_wait_for_rebuilds(rg_config_p, 1, num_drives_to_remove);

                }
            }
        }

        status = fbe_api_scheduler_del_debug_hook(rg_object_ids[rg_index],
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Stop logical error injection.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
        if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
        {
            status = fbe_api_logical_error_injection_disable_class(rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
        }
        else
        {
            status = fbe_api_logical_error_injection_disable_class(FBE_CLASS_ID_MIRROR, FBE_PACKAGE_ID_SEP_0);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Restore the sector trace level to it's default.
         */
        fbe_test_sep_util_restore_sector_trace_level();
    }
}

/*!**************************************************************
 *          squeeze_setup_sequential_rdgen_context()
 ****************************************************************
 * @brief
 *  Setup the context for a rdgen operation.  This operation
 *  will sweep over an area with a given capacity using
 *  i/os of the same size.
 *
 * @param context_p - Context structure to setup.  
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param max_lba - largest lba to test with in blocks.
 * @param io_size_blocks - io size to fill with in blocks.             
 *
 * @return None.
 *
 ****************************************************************/
static void squeeze_setup_sequential_rdgen_context(  fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t capacity,
                                                    fbe_u32_t max_passes,
                                                    fbe_u32_t threads,
                                                    fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id, 
                                                class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                threads,
                                                FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                                io_size_blocks, /* Min blocks */
                                                io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

}
/*************************************************
 * end of squeeze_setup_sequential_rdgen_context()
 *************************************************

/*!**************************************************************
 *          squeeze_setup_random_rdgen_context()
 ****************************************************************
 * @brief
 *  Setup the context for a rdgen operation.  This operation
 *  will sweep over an area with a given capacity using
 *  i/os of the same size.
 *
 * @param context_p - Context structure to setup.  
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param max_lba - largest lba to test with in blocks.
 * @param io_size_blocks - io size to fill with in blocks.             
 *
 * @return None.
 *
 ****************************************************************/
static void squeeze_setup_random_rdgen_context(  fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t capacity,
                                                    fbe_u32_t max_passes,
                                                    fbe_u32_t threads,
                                                    fbe_u32_t io_size_blocks)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id, 
                                                class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                threads,
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

}
/*********************************************
 * end of squeeze_setup_random_rdgen_context()
 *********************************************
  
/*!***************************************************************************
 *          squeeze_run_corrupt_crc_on_config()
 *****************************************************************************
 *
 * @brief   Start rdgen threads to all luns executing the rdgen
 *          corrupt_crc/read/check opcode using the options specified:
 *              o Either set or clear the `check checksum' flag
 *              o Run degraded or not
 *              o Maximum blocks to use
 *
 * @param   rg_config_p - Pointer to array of raid groups to run against
 * @param   context_p - context to execute
 * @param   max_request_size - The maximum request size in blocks
 * @param   b_check_checksum - FBE_TRUE - Set the `check checksum' flag for
 *                                  all requests.
 *                             FBE_FALSE - Clear the `check checksum' flag
 *                                  for all requests.
 * @param   b_run_degraded - FBE_TRUE - Start non-degraded then run degraded
 * @param   b_wait_for_rebuild - FBE_TRUE - Another test will use these raid
 *                                  groups, therefore wait for rebuild.
 *                               FBE_FALSE - No other test will use these
 *                                  raid groups, therefore don't wait.
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  5/19/10 - Kevin Schleicher Created
 *
 *****************************************************************************/
fbe_status_t squeeze_run_corrupt_crc_on_config(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_api_rdgen_context_t *context_p,
                                               fbe_u32_t  max_request_size,
                                               fbe_bool_t b_check_checksum,
                                               fbe_bool_t b_run_degraded,
                                               fbe_bool_t b_wait_for_rebuild)
                                  
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           raid_group_count = fbe_test_get_rg_count(rg_config_p);
    fbe_u32_t           context_index = 0;
    fbe_u32_t           num_of_contexts_used = 0;
    fbe_u32_t           io_seconds = 0;
    fbe_rdgen_options_t rdgen_options = 0;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t  filter = {0};
    fbe_bool_t          b_is_non_optimal_rg = FBE_FALSE;
    fbe_u32_t           drives_to_remove = 1;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        drives_to_remove = 2;
    }

    /* Initialize the filter
     */
    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID, 0 /* edge index */);

    /* Due to issues with non-optimal raid groups determine if the raid group
     * is optimal or not.
     */
    if (rg_config_p->block_size != FBE_BE_BYTES_PER_BLOCK)
    {
        b_is_non_optimal_rg = FBE_TRUE;
    }

    /* We allow the user to input a time to run I/O.  By default io seconds 
     * is 0. 
     *  
     * By default we will run I/O until we see enough errors is encountered.
     */
    io_seconds = fbe_test_sep_util_get_io_seconds();
    
    /* Context 0: Setup (1) thread, sequential, fixed sized, corrupt crc/read/check.
     */
    MUT_ASSERT_TRUE(num_of_contexts_used < SQUEEZE_MAX_CONTEXT_PER_TEST);
    squeeze_setup_sequential_rdgen_context(&context_p[num_of_contexts_used],
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,      /* run forever*/ 
                                           1,      /* threads per lun */
                                           max_request_size);
    num_of_contexts_used++;

    /* Context 1: Setup (1) thread, random, random sized, corrupt crc/read/check.
     */
    MUT_ASSERT_TRUE(num_of_contexts_used < SQUEEZE_MAX_CONTEXT_PER_TEST);
    squeeze_setup_random_rdgen_context(&context_p[num_of_contexts_used],
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,      /* run forever*/ 
                                       1,      /* threads per lun */
                                       max_request_size);
    num_of_contexts_used++;

    /* Context 2: Setup (1) thread, random, random sized, write/read/check.
     */
    MUT_ASSERT_TRUE(num_of_contexts_used < SQUEEZE_MAX_CONTEXT_PER_TEST);
    squeeze_setup_random_rdgen_context(&context_p[num_of_contexts_used],
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,      /* run forever*/ 
                                       1,      /* threads per lun */
                                       max_request_size);
    num_of_contexts_used++;

    /* For the first context only set the rdgen opitions based on the 
     * parameters passed.  If b_check_checksum is false, do not check the 
     * checksum.
     */
    if (b_check_checksum == FBE_FALSE)
    {
        rdgen_options |= FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC;
    }

    /* For degraded parity raid groups we cannot validate the `invalidation reason'.
     * Therefore set the flag to ignore this if it is the case.
     */
    if ((b_run_degraded == FBE_TRUE)                   &&
        (rg_config_p->class_id == FBE_CLASS_ID_PARITY)    )
    {
        rdgen_options |= FBE_RDGEN_OPTIONS_DO_NOT_CHECK_INVALIDATION_REASON;
    }

    /* Now set the options for the corrupt crc/read/compare
     */
    for (context_index = 0; context_index < (num_of_contexts_used - 1); context_index++)
    {
        status = fbe_api_rdgen_io_specification_set_options(&context_p[context_index].start_io.specification,
                                                            rdgen_options);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, num_of_contexts_used); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate that I/O was started successfully.
     */
    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_INVALID, context_p->status);

    /* Let the I/O run for the required time
     */
    if (io_seconds > 0)
    {
        /* Let it run for a reasonable time before stopping it.
         */
    }
    else
    {
        /* Let I/O run for a few seconds */
        if (fbe_sep_test_util_get_raid_testing_extended_level() > 0)
        {
            io_seconds = 3;
        }
        else
        {
            /* Sleep for less in the 'qual' version.
             */
            io_seconds = 2;
        }
    }

    /* For very large I/Os, bump the io seconds
     */
    if ((max_request_size / io_seconds) > 2000)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Bump I/O time from: %d to %d seconds ==", 
                   __FUNCTION__, io_seconds, ((max_request_size / 2000) + 1));
        io_seconds = (max_request_size / 2000) + 1;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O for %d seconds ==", __FUNCTION__, io_seconds);
    fbe_api_sleep(io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND);

    /* If we are to run degraded degrade the raid group and run I/O for the
     * requested time.
     */
    if (b_run_degraded == FBE_TRUE)
    {
        /* Invoke method to degrade the raid group.
         */
        big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove, 
                                   FBE_TRUE, /* We plan on re-inserting same drive(s) later. */ 
                                   100, /* msecs between removals */
                                   FBE_DRIVE_REMOVAL_MODE_RANDOM);

        /* Let I/O run for the requested time
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s starting degraded I/O for %d seconds ==", __FUNCTION__, io_seconds);
        fbe_api_sleep(io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND);
    }

    /* Stop I/O. Validate status
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_of_contexts_used); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate the I/O status (based on whether `check checksum' was set or not)
     * The (n - 1) rdgen contexts are the context that generates errors or not.
     * Parity raid groups validate the data read from disk when they are degraded.
     */
    for (context_index = 0; context_index < (num_of_contexts_used - 1); context_index++)
    {
        /*! @note Sometimes it takes a long time for I/o to start, if the I/O
         *        count is is low and the I/O time is less than 5 seconds, punt.
         */
        if ((context_p[context_index].start_io.statistics.io_count < 1)                       &&
            (context_p[context_index].start_io.statistics.elapsed_msec < (io_seconds * 1000))    )
        {
            /* Skip the results for this test.
             */
            continue;
        }

        /* Validate that we executed at least (1) I/O per thread
         */
        MUT_ASSERT_TRUE(context_p[context_index].start_io.statistics.io_count >= 1);

        /* Determine what the expected status should be for the 
         * corrupt crc/read/compare contexts.
         */
        if (b_check_checksum == FBE_FALSE)
        {
            if ((b_run_degraded == FBE_FALSE)                  ||
                (rg_config_p->class_id != FBE_CLASS_ID_PARITY)    )
            {
                /* If we were not checking the checksums we don't expect errors.
                 */
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.media_error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.aborted_error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.io_failure_error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.invalid_request_err_count, 0);
                MUT_ASSERT_TRUE(context_p[context_index].start_io.statistics.inv_blocks_count > 0);
                MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.bad_crc_blocks_count, 0);
            }
            else
            {
                /* Else if we are not checking checksums and we are degraded and this
                 * is parity raid group, then we might get a status of `success'.
                 * Otherwise we expect every request (corrupt crc, read) to fail.
                 */
                if (((b_run_degraded == FBE_TRUE)                   ||
                     (rg_config_p->class_id == FBE_CLASS_ID_PARITY)    )                                                      &&
                    (context_p[context_index].start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)    )
                {
                    /* Just print a message that we detected a success status
                     */
                    mut_printf(MUT_LOG_TEST_STATUS, "== %s Found success status context: %d class_id: 0x%x ==", 
                               __FUNCTION__, context_index, rg_config_p->class_id);
                }
                else
                {
                    /* Else we expect failure status
                    */
                    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
                    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
                }
            }
        } /* end if `not checking checksums' */
        else
        {
            /* Else we are checking checksum and thus expect media errors.
             */
            MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
            MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
        } /* end else if check checksum is set */

        /* Since half the io_count is writes, we cannot validate the io_count against the error count
         */
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.media_error_count, context_p[context_index].start_io.statistics.error_count);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.aborted_error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.io_failure_error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.invalid_request_err_count, 0);
        MUT_ASSERT_TRUE(context_p[context_index].start_io.statistics.inv_blocks_count > 0);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.bad_crc_blocks_count, 0);

    } /* end for all corrupt crc/read/compare contexts */

    /* Now validate that no errors were detected in the write/read/compare
     * context.
     */
    context_index = (num_of_contexts_used - 1);

    /*! @note Sometimes it takes a long time for I/o to start, if the I/O
     *        count is is low and the I/O time is less than 5 seconds, punt.
     */
    if ((context_p[context_index].start_io.statistics.io_count >= 1)                       ||
        (context_p[context_index].start_io.statistics.elapsed_msec >= (io_seconds * 1000))    )
    {
        MUT_ASSERT_TRUE(context_p[context_index].start_io.statistics.io_count >= 1);
    }

   
    /* We expect success status.
     */
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.media_error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.aborted_error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.io_failure_error_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.inv_blocks_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.statistics.bad_crc_blocks_count, 0);

    /* Re-insert any removed drives*/
    if (b_run_degraded == FBE_TRUE)
    {
        /* Re-insert the removed drives
         */
        big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove, 
                                   FBE_TRUE /* We are reinserting the same drive(s) */);

        /* If requested wait for the rebuilds to complete
         */
        if (b_wait_for_rebuild == FBE_TRUE)
        {
            big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
        }
    }

    /* Always return the status
     */    
    return status;
}
/********************************************
 * end of squeeze_run_corrupt_crc_on_config()
 ********************************************

/*!**************************************************************
 * squeeze_run_corrupt_crc_test()
 ****************************************************************
 * @brief
 * Start rdgen threads to all luns executing the corrupt_crc/read/check
 * opcode.
 *
 * @param context_p - context to execute.
 * 
 * @return None.   
 *
 * @author
 *  5/19/10 - Kevin Schleicher Created
 *
 ****************************************************************/
void squeeze_run_corrupt_crc_test(fbe_api_rdgen_context_t *context_p,
                                  fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       small_io_size = SQUEEZE_SMALL_IO_SIZE_MAX_BLOCKS;
    fbe_u32_t       large_io_size = SQUEEZE_LARGE_IO_SIZE_MAX_BLOCKS;
    fbe_lba_t       min_lun_capacity;

    /* Determine the minimum lun capacity.
     */
    status = fbe_test_sep_util_get_minimum_lun_capacity_for_config(rg_config_p, &min_lun_capacity);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (small_io_size > (fbe_u32_t)(min_lun_capacity / 2))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Change small io size from: 0x%x to 1/2 min lun cap: 0x%llx ==", 
                   __FUNCTION__, small_io_size, (unsigned long long)min_lun_capacity);
        small_io_size = (fbe_u32_t)min_lun_capacity / 2;
    }
    if (large_io_size > (fbe_u32_t)(min_lun_capacity / 2))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Change large io size from: 0x%x to 1/2 min lun cap: 0x%llx ==", 
                   __FUNCTION__, large_io_size, (unsigned long long)min_lun_capacity);
        large_io_size = (fbe_u32_t)min_lun_capacity / 2;
    }

    /* Step 1: Run small I/O without `check checksum' flag set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Running small io: %d blocks w/o check checksum ==", 
               __FUNCTION__, small_io_size);
    status = squeeze_run_corrupt_crc_on_config(rg_config_p,
                                               context_p,
                                               small_io_size,
                                               FBE_FALSE,   /* Don't check the checksum */
                                               FBE_FALSE,   /* Don't run degraded */
                                               FBE_FALSE    /* Don't wait for rebuild */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Small io: %d blocks w/o check checksum: Complete ==", 
               __FUNCTION__, small_io_size);

    /* Step 2: Run small I/O with `check checksum' flag set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Running small io: %d blocks with check checksum ==", 
               __FUNCTION__, small_io_size);
    status = squeeze_run_corrupt_crc_on_config(rg_config_p,
                                               context_p,
                                               small_io_size,
                                               FBE_TRUE,    /* Check the checksum */
                                               FBE_FALSE,   /* Don't run degraded */
                                               FBE_FALSE    /* Don't wait for rebuild */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Small io: %d blocks with check checksum: Complete ==", 
               __FUNCTION__, small_io_size);

    /* Step 3: Run large I/O without `check checksum' flag set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Running large io: %d blocks w/o check checksum ==", 
               __FUNCTION__, large_io_size);
    status = squeeze_run_corrupt_crc_on_config(rg_config_p,
                                               context_p,
                                               large_io_size,
                                               FBE_FALSE,   /* Don't check the checksum */
                                               FBE_FALSE,   /* Don't run degraded */
                                               FBE_FALSE    /* Don't wait for rebuild */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Large io: %d blocks w/o check checksum: Complete ==", 
               __FUNCTION__, large_io_size);

    /* Step 4: Run large I/O with `check checksum' flag set.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Running large io: %d blocks with check checksum ==", 
               __FUNCTION__, large_io_size);
    status = squeeze_run_corrupt_crc_on_config(rg_config_p,
                                               context_p,
                                               large_io_size,
                                               FBE_TRUE,    /* Check the checksum */
                                               FBE_FALSE,   /* Don't run degraded */
                                               FBE_FALSE    /* Don't wait for rebuild */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Large io: %d blocks with check checksum: Complete ==", 
               __FUNCTION__, large_io_size);

    /* Only at test level 1 or greater will we run degraded testing
     */
    if ((fbe_sep_test_util_get_raid_testing_extended_level() > 0) &&
        (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)        )
    {
        /* Step 5: Run small I/O degraded without `check checksum' flag set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Running small io: %d blocks w/o check checksum degraded ==", 
                   __FUNCTION__, small_io_size);
        status = squeeze_run_corrupt_crc_on_config(rg_config_p,
                                                   context_p,
                                                   small_io_size,
                                                   FBE_FALSE,   /* Don't check the checksum */
                                                   FBE_TRUE,    /* Run degraded */
                                                   FBE_TRUE     /* Wait for rebuild */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Small io: %d blocks w/o check checksum degraded: Complete ==", 
                   __FUNCTION__, small_io_size);

        /* Step 6: Run large I/O degraded with `check checksum' flag set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Running large io: %d blocks with check checksum degraded ==", 
                   __FUNCTION__, large_io_size);
        status = squeeze_run_corrupt_crc_on_config(rg_config_p,
                                                   context_p,
                                                   large_io_size,
                                                   FBE_TRUE,    /* Check the checksum */
                                                   FBE_TRUE,    /* Run degraded */
                                                   FBE_FALSE    /* Don't wait for rebuild */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Large io: %d blocks with check checksum degraded: Complete ==", 
                   __FUNCTION__, large_io_size);

    } /* end if we are to test degraded */

    /* Return
     */
    return;
}
/***************************************
 * end of squeeze_run_corrupt_crc_test()
 ***************************************/

/*!**************************************************************
 * squeeze_run_corrupt_data_on_write_test()
 ****************************************************************
 * @brief
 *  Run a test where we will try to write corrupted blocks and
 *  make sure we get the appropriate status back.
 *
 * @param rg_config_p
 * @param rdgen_operation           
 *
 * @return none. 
 *
 * @author
 *  1/9/2013 - Created. Rob Foley
 *
 ****************************************************************/

void squeeze_run_corrupt_data_on_write_test(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_rdgen_operation_t rdgen_operation)
{
    fbe_api_rdgen_context_t *context_p = &squeeze_test_contexts[0];
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_object_id_t lun_object_id;
    fbe_u32_t raid_group_count = fbe_test_get_rg_count(rg_config_p);
    fbe_u32_t total_tests = 0;
    fbe_u32_t test_index;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting opcode: 0x%x ==", __FUNCTION__, rdgen_operation);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_rdgen_test_context_init( &context_p[total_tests], 
                                                      lun_object_id, 
                                                      FBE_CLASS_ID_INVALID,
                                                      FBE_PACKAGE_ID_SEP_0,
                                                      rdgen_operation,
                                                      FBE_RDGEN_PATTERN_CLEAR,
                                                      1,
                                                      0,    /* io count not used */
                                                      0,    /* time not used*/
                                                      1,
                                                      FBE_RDGEN_LBA_SPEC_RANDOM,
                                                      0,    /* Start lba*/
                                                      0,    /* Min lba */
                                                      FBE_LBA_INVALID,    /* max lba */
                                                      FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                      1,    /* Min blocks */
                                                      0x1000    /* Max blocks */ );
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            total_tests++;
        }
    }

    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, total_tests); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_NEIT, total_tests);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we get bad status values back for all the test contexts.
     */
    for (test_index = 0; test_index < total_tests; test_index++)
    {
        MUT_ASSERT_INT_EQUAL(context_p[test_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        MUT_ASSERT_INT_EQUAL(context_p[test_index].start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR);
        MUT_ASSERT_INT_EQUAL(context_p[test_index].start_io.statistics.error_count, 1);
        MUT_ASSERT_INT_EQUAL(context_p[test_index].start_io.statistics.io_failure_error_count, 1);

        status = fbe_api_rdgen_test_context_destroy(&context_p[test_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete opcode: 0x%x total_tests: 0x%x==", __FUNCTION__, rdgen_operation, total_tests);
}
/******************************************
 * end squeeze_run_corrupt_data_on_write_test()
 ******************************************/

/*!**************************************************************
 * squeeze_test_set()
 ****************************************************************
 * @brief
 *  The set of test functions to run on a raid group config
 *
 * @param                
 *
 * @return None.
 *
 * @author
 *  5/9/2011 - Created. Deanna Heng
 *
 ****************************************************************/
void squeeze_test_set(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_api_rdgen_context_t *rdgen_context_p = &squeeze_test_contexts[0];
    fbe_status_t status;

    /* Initialize the luns.
     */
    //big_bird_write_background_pattern();

    /* We are purposely sending blocks with bad checksums.
     * We don't want these to appear in the traces, 
     * since this significantly delays the test, so supress sector tracing. 
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    squeeze_run_corrupt_data_on_write_test(rg_config_p, FBE_RDGEN_OPERATION_WRITE_ONLY);
    squeeze_run_corrupt_data_on_write_test(rg_config_p, FBE_RDGEN_OPERATION_VERIFY_WRITE);
    fbe_test_sep_util_restore_sector_trace_level();

    /* Wait for zeroing and initial verify to finish. 
     * Otherwise we will see our tests hit unexpected results when the initial verify gets kicked off. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    squeeze_run_invalid_on_error_test(rdgen_context_p, rg_config_p);

    /* Re-initialize the luns.
     */
    big_bird_write_background_pattern();

    squeeze_run_read_do_not_check_crc_test(rdgen_context_p, rg_config_p);

    /* Re-initialize the luns.
     */
    big_bird_write_background_pattern();

    /* The next test should not get errors.  The prior tests are expected to have seen errors.
     */
    status = fbe_api_sector_trace_reset_counters();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);
    fbe_test_sep_util_enable_trace_limits(FBE_FALSE);

    /*! @note This test does not wait for rebuilds to complete.
     *        Therefore it should be last.
     */
    squeeze_run_corrupt_crc_test(rdgen_context_p, rg_config_p);

    /* DE654 tracks enabling this checking for Raid 6.
     */
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID6)
    {
    /* We do not expect any errors. */
        fbe_test_sep_util_validate_no_raid_errors();
    }

    /* Make sure we did not get any trace errors.  We do this in between each
     * raid group test since it stops the test sooner. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end squeeze_test_set()
 ******************************************/


void squeeze_run_tests(fbe_test_logical_error_table_test_configuration_t *config_p,
                       fbe_u32_t element_sizes,
                       fbe_raid_group_type_t raid_type)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t config_count = abby_cadabby_get_config_count(config_p);
    fbe_u32_t raid_type_index = 0;

    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];

        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type) ||
            (raid_type != rg_config_p->raid_type))
        {
            continue;
        }
        
        if (raid_type_index + 1 >= config_count) {
            fbe_test_run_test_on_rg_config(rg_config_p, 0, squeeze_test_set,
                                           SQUEEZE_TEST_LUNS_PER_RAID_GROUP, 
                                           SQUEEZE_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 0, squeeze_test_set,
                                           SQUEEZE_TEST_LUNS_PER_RAID_GROUP, 
                                           SQUEEZE_CHUNKS_PER_LUN,FBE_FALSE);
        }
    }
    return;
}

/*!**************************************************************
 * squeeze_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 3 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void squeeze_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t element_sizes;
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        element_sizes = SQUEEZE_TEST_ELEMENT_SIZES;
        config_p = &squeeze_raid_groups_extended[0];
    }
    else
    {
        element_sizes = SQUEEZE_TEST_ELEMENT_SIZES;
        config_p = &squeeze_raid_groups_qual[0];
    }

    /* Run the test.
     */
    squeeze_run_tests(config_p, element_sizes, raid_type);

    return;
}
/******************************************
 * end squeeze_test()
 ******************************************/

/*!**************************************************************
 * squeeze_setup()
 ****************************************************************
 * @brief
 *  Setup for the squeeze test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void squeeze_setup(fbe_raid_group_type_t raid_type)
{
    /* This test does error injection, turn off debug flags */
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
            config_p = &squeeze_raid_groups_extended[0];
        }
        else
        {
            config_p = &squeeze_raid_groups_qual[0];
        }
        abby_cadabby_simulation_setup(config_p,
                                      FBE_TRUE);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end squeeze_setup()
 **************************************/

/*!**************************************************************
 * squeeze_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the squeeze test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void squeeze_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end squeeze_cleanup()
 ******************************************/



void squeeze_raid0_setup(void)
{
    squeeze_setup(FBE_RAID_GROUP_TYPE_RAID0);
}
void squeeze_raid1_setup(void)
{
    squeeze_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void squeeze_raid10_setup(void)
{
    squeeze_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void squeeze_raid5_setup(void)
{
    squeeze_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void squeeze_raid3_setup(void)
{
    squeeze_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void squeeze_raid6_setup(void)
{
    squeeze_setup(FBE_RAID_GROUP_TYPE_RAID6);
}

void squeeze_raid0_test(void)
{
    squeeze_test(FBE_RAID_GROUP_TYPE_RAID0);
}
void squeeze_raid1_test(void)
{
    squeeze_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void squeeze_raid10_test(void)
{
    squeeze_test(FBE_RAID_GROUP_TYPE_RAID10);
}
void squeeze_raid3_test(void)
{
    squeeze_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void squeeze_raid5_test(void)
{
    squeeze_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void squeeze_raid6_test(void)
{
    squeeze_test(FBE_RAID_GROUP_TYPE_RAID6);
}

/*************************
 * end file squeeze_test.c
 *************************/


