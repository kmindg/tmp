/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file kermit_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a physical error retry test.
 *
 * @version
 *   9/10/2009 - Created. Rob Foley
 *   5/11/2011 - Changes to run on Hardware. Vamsi V
 *  02/28/2012 - Ron Proulx - Remove non-optimal (a.k.a 512-bps) raid groups
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "sep_rebuild_utils.h"                              //  .h shared between Dora and other NR/rebuild tests

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * kermit_short_desc = "raid physical error retrys test.";
char * kermit_long_desc ="\
The Kermit scenario tests raid group handling of non-logical and non-media errors.\n\
\n\
-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n"\
"STEP 1: Bring up the initial topology.\n\
           - Configure all raid groups and luns \n\
           - Each raid group has one lun \n\
           - Each lun will have 3 chunks \n\
           - Each raid type will be tested with 512 and 520 block drives \n\
STEP 2: Enable injection of appropriate error using an edge tap.\n\
STEP 3: Start rdgen to issue some number of I/Os that will encounter errors.\n\
STEP 4: Validate that the status is as expected on the I/O.\n\
STEP 5: Validate that the error was injected and retried the appropriate number of times (possibly 0 retries).\n\
STEP 6: Destroy all configured objects and disable error injection.\n\
Switches:\n\
    -abort_cases_only - Only test the abort/expiration cases.\n\
    -start_table - The table to begin with.\n\
    -start_index - The index number to begin with.\n\
    -total_cases - The number of test cases to run.\n\
    -repeat_case_count - The number of times to repeat each case.\n\
\
Description last updated: 10/13/2011.\n";

/*!*******************************************************************
 * @def KERMIT_LUNS_PER_RAID_GROUP_QUAL
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define KERMIT_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def KERMIT_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define KERMIT_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def KERMIT_MAX_BACKEND_IO_SIZE
 *********************************************************************
 * @brief Max size we expect the I/O to get broken up after.
 *
 *********************************************************************/
#define KERMIT_MAX_BACKEND_IO_SIZE 2048

/*!*******************************************************************
 * @def KERMIT_BASE_OFFSET
 *********************************************************************
 * @brief All drives have this base offset, that we need to add on.
 *
 *********************************************************************/
#define KERMIT_BASE_OFFSET 0x10000

/*!*******************************************************************
 * @def KERMIT_ERR_COUNT_MATCH
 *********************************************************************
 * @brief Error count should match the size of the element.
 *
 *********************************************************************/
#define KERMIT_ERR_COUNT_MATCH FBE_U32_MAX

/*!*******************************************************************
 * @def KERMIT_ERR_COUNT_MATCH_ELSZ
 *********************************************************************
 * @brief Error count should match the size of the element.
 *
 *********************************************************************/
#define KERMIT_ERR_COUNT_MATCH_ELSZ (FBE_U32_MAX - 1)

/*!*******************************************************************
 * @def KERMIT_ERR_COUNT_NON_ZERO
 *********************************************************************
 * @brief The error count should not be zero but can match any non-zero count.
 *
 *********************************************************************/
#define KERMIT_ERR_COUNT_NON_ZERO (FBE_U32_MAX - 2)

/*!*******************************************************************
 * @def KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE
 *********************************************************************
 * @brief When verifying IO error counts,  this flag will indicate it 
 *    should use a range for FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, 
 *    instead of exact number.
 *
 *********************************************************************/
#define KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE (FBE_U32_MAX - 3)


/*!*******************************************************************
 * @def KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE
 *********************************************************************
 * @brief When verifying IO error counts,  this flag will indicate it 
 *    should use a range for KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2, 
 *    instead of exact number.  This should be used when error case
 *    injects retryable errors into 2 drives in same RG, expecting
 *    both drives to fail.
 *
 *********************************************************************/
#define KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE (FBE_U32_MAX - 4)

/*!*******************************************************************
 * @def KERMIT_WAIT_MSEC
 *********************************************************************
 * @brief max number of millseconds we will wait.
 *        The general idea is that waits should be brief.
 *        We put in an overall wait time to make sure the test does not get stuck.
 *
 *********************************************************************/
#define KERMIT_WAIT_MSEC 60000

/*!*******************************************************************
 * @def KERMIT_MAX_RG_COUNT
 *********************************************************************
 * @brief Max number of raid groups we expect to test in parallel.
 *
 *********************************************************************/
#define KERMIT_MAX_RG_COUNT 16
 
/*!*******************************************************************
 * @def KERMIT_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define KERMIT_RAID_TYPE_COUNT 6

/*!*******************************************************************
 * @var kermit_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t kermit_raid_group_config_qual[] = 
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {16,      0x22000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            2,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 3 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 6 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            2,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 0 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
        {3,       0x9000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 10 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           0,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end kermit_raid_group_config_qual()
 **************************************/

/*!*******************************************************************
 * @var kermit_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t kermit_raid_group_config_extended[] = 
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,         0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            2,         1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {16,      0x22000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            2,         0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            3,         1},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            4,         1},
        {16,      0x22000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            5,         1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 3 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,         0},
        {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            2,         1},
        {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            3,         1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 6 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            2,          0},
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            3,          1},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            4,          1},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            5,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 0 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
        {3,       0x9000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          1},
        {3,       0x9000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          1},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 10 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          1},
        {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
        {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end kermit_raid_group_config_extended()
 **************************************/

/*!*************************************************
 * @enum fbe_kermit_raid_type_t
 ***************************************************
 * @brief Set of raid types we will test.
 *
 ***************************************************/
typedef enum fbe_kermit_raid_type_e
{
    KERMIT_RAID_TYPE_RAID1 = 0,
    KERMIT_RAID_TYPE_RAID5 = 1,
    KERMIT_RAID_TYPE_RAID3 = 2,
    KERMIT_RAID_TYPE_RAID6 = 3,
    KERMIT_RAID_TYPE_RAID0 = 4,
    KERMIT_RAID_TYPE_RAID10 = 5,
    KERMIT_RAID_TYPE_LAST,
}
fbe_kermit_raid_type_t;

/*!*************************************************
 * @enum fbe_kermit_error_location_t
 ***************************************************
 * @brief This defines where we will inject the
 *        error on the raid group.
 *
 ***************************************************/
typedef enum fbe_kermit_error_location_e
{
    KERMIT_ERROR_LOCATION_INVALID = 0,
    KERMIT_ERROR_LOCATION_USER_DATA, /*!< Only inject on user area. */
    KERMIT_ERROR_LOCATION_USER_DATA_CHUNK_1, /*!< Only inject on user area. */
    KERMIT_ERROR_LOCATION_PAGED_METADATA, /*< Only inject on paged metadata. */
    KERMIT_ERROR_LOCATION_RAID, /*< Inject on all raid protected areas both user and paged metadata. */
    KERMIT_ERROR_LOCATION_WRITE_LOG,   /* Inject on write log area. */
    KERMIT_ERROR_LOCATION_LAST,
}
fbe_kermit_error_location_t;

/*!*************************************************
 * @struct fbe_kermit_drive_error_definition_t
 ***************************************************
 * @brief Single drive's error definition.
 *
 ***************************************************/
typedef struct fbe_kermit_drive_error_definition_s
{
    fbe_test_protocol_error_type_t error_type;  /*!< Class of error (retryable/non/media/soft) */
    fbe_payload_block_operation_opcode_t command; /*!< Command to inject on. */
    fbe_u32_t times_to_insert_error;
    fbe_u32_t rg_position_to_inject;
    fbe_bool_t b_does_drive_die_on_error; /*! FBE_TRUE if the drive will die. */
    fbe_kermit_error_location_t location; /*!< Area to inject the error. */
    fbe_u8_t tag; /*!< this tag value is significant only when b_inc_for_all_drives is true. error counts are incremented for all the records having similar tag value. */
}
fbe_kermit_drive_error_definition_t;

/*!*******************************************************************
 * @def KERMIT_ERROR_CASE_MAX_ERRORS
 *********************************************************************
 * @brief Max number of errors per error case.
 *
 *********************************************************************/
#define KERMIT_ERROR_CASE_MAX_ERRORS 10

/*!*******************************************************************
 * @def KERMIT_MAX_DEGRADED_POSITIONS
 *********************************************************************
 * @brief Max number of positions to degrade before starting the test.
 *
 * @note  To support striped mirrors this is the max array with / 2
 *
 *********************************************************************/
#define KERMIT_MAX_DEGRADED_POSITIONS 8

/*!*******************************************************************
 * @def KERMIT_MAX_TEST_WIDTHS
 *********************************************************************
 * @brief Max number of widths we can specify to test.
 *
 *********************************************************************/
#define KERMIT_MAX_TEST_WIDTHS 4

/*!*******************************************************************
 * @def KERMIT_MAX_ERROR_CASES
 *********************************************************************
 * @brief Max number of possible cases to validate against.
 *
 *********************************************************************/
#define KERMIT_MAX_ERROR_CASES 4

/*!*******************************************************************
 * @def KERMIT_MAX_POSSIBLE_STATUS
 *********************************************************************
 * @brief Max number of possible different I/O status for rdgen request
 *
 *********************************************************************/
#define KERMIT_MAX_POSSIBLE_STATUS  2
#define KERMIT_INVALID_STATUS       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID

/*!*******************************************************************
 * @def KERMIT_NUM_DIRECT_IO_ERRORS
 *********************************************************************
 * @brief Number of errors to insert into direct I/O execution (i.e.
 *        error that are inject but not into the raid library state
 *        machine).
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *
 *********************************************************************/
#define KERMIT_NUM_DIRECT_IO_ERRORS                 1

/*!*******************************************************************
 * @def KERMIT_NUM_READ_AND_SINGLE_VERIFY_ERRORS
 *********************************************************************
 * @brief Number of error to inject that accomplishes the following:
 *          o Inject error assuming that direct I/o encounters the first
 *          o Inject error assuming read state machine encounters the second
 *          o Inject error assuming that verify state machine encounters the third
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *
 *********************************************************************/
#define KERMIT_NUM_READ_AND_SINGLE_VERIFY_ERRORS    (KERMIT_NUM_DIRECT_IO_ERRORS + 2)

/*!*******************************************************************
 * @def KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE
 *********************************************************************
 * @brief Number of retryable errors to cause a drive to fail.
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *        This number utimately translates to the number of errors DIEH 
 *        detects before issuing a drive reset, which then causes PDO 
 *        edge to go disabled.  Drive does not actually fail.
 * 
 *********************************************************************/
#define KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE

/*!*******************************************************************
 * @def KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2
 *********************************************************************
 * @brief Number of accumulated retryable errors to cause two
 *        drives in same RG to fail.
 * 
 *********************************************************************/
#define KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2 (KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE*2)

/*!*************************************************
 * @typedef fbe_kermit_error_case_t
 ***************************************************
 * @brief Single test case with the error and
 *        the blocks to read for.
 *
 ***************************************************/
typedef struct fbe_kermit_error_case_s
{
    fbe_u8_t *description_p;            /*!< Description of test case. */
    fbe_u32_t line;                     /*!< Line number of test case. */
    fbe_lba_t lba;                      /*!< Rdgen lba. */
    fbe_block_count_t blocks;           /*!< Rdgen block count */
    fbe_rdgen_operation_t operation;    /*!< Rdgen operation to send. */
    fbe_bool_t b_set_rg_expiration_time; /*!< FBE_TRUE to set expiration time rg puts into packet. */
    fbe_bool_t b_element_size_multiplier; /*!< TRUE to multiply blocks by element size */
    fbe_bool_t b_inc_for_all_drives; /*!< TRUE */

    fbe_kermit_drive_error_definition_t errors[KERMIT_ERROR_CASE_MAX_ERRORS];
    fbe_u32_t num_errors;
    fbe_u32_t degraded_positions[KERMIT_MAX_DEGRADED_POSITIONS];
    fbe_u32_t widths[KERMIT_MAX_TEST_WIDTHS]; /*! Widths to test with. FBE_U32_MAX if invalid*/
    fbe_time_t expiration_msecs; /*!< msecs before I/O will expire. */
    fbe_time_t abort_msecs; /*!< msecs before I/O will get cancelled. */
    fbe_notification_type_t wait_for_state_mask; /*!< Wait for the rg to get to this state. */
    fbe_u32_t expected_io_error_count;
    fbe_payload_block_operation_status_t block_status[KERMIT_MAX_POSSIBLE_STATUS];
    fbe_payload_block_operation_qualifier_t block_qualifier[KERMIT_MAX_POSSIBLE_STATUS];

    /* For any of these counters, FBE_U32_MAX means that it is not used. 
     * FBE_U32_MAX - 1 means to use a multiple of the element size. 
     */
    fbe_u32_t retryable_error_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected retryable errors. */
    fbe_u32_t non_retryable_error_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected non-retryable errors. */
    fbe_u32_t shutdown_error_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected shutdown errors. */
    fbe_u32_t correctable_media_error_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected correctable media errors. */
    fbe_u32_t uncorrectable_media_error_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected uncorrectable media errors. */
    fbe_u32_t correctable_time_stamp[KERMIT_MAX_ERROR_CASES];
    fbe_u32_t correctable_write_stamp[KERMIT_MAX_ERROR_CASES];
    fbe_u32_t uncorrectable_time_stamp[KERMIT_MAX_ERROR_CASES];
    fbe_u32_t uncorrectable_write_stamp[KERMIT_MAX_ERROR_CASES];
    fbe_u32_t correctable_coherency[KERMIT_MAX_ERROR_CASES];
    fbe_u32_t correctable_crc_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected correctable crc errors. */
    fbe_u32_t uncorrectable_crc_count[KERMIT_MAX_ERROR_CASES]; /*!< Expected uncorrectable crc errors. */
    fbe_u32_t max_error_cases; /*!< Max number of possible counts above. */
}
fbe_kermit_error_case_t;

/*!***************************************************************************
 * @typedef fbe_kermit_provision_drive_default_background_operations_t
 *****************************************************************************
 * @brief   Global setting of default background operations.
 *
 ***************************************************/
typedef struct fbe_kermit_provision_drive_default_background_operations_t
{
    fbe_bool_t  b_is_initialized;
    fbe_bool_t  b_is_background_zeroing_enabled;
    fbe_bool_t  b_is_sniff_verify_enabled;  
    /*! @note As more background operations are added add flag here.
     */
}
fbe_kermit_provision_drive_default_background_operations_t;

/*!*******************************************************************
 * @def KERMIT_MAX_STRING_LENGTH
 *********************************************************************
 * @brief Max size of a string in a kermit structure.
 *
 *********************************************************************/
#define KERMIT_MAX_STRING_LENGTH 500

static fbe_status_t fbe_sep_kermit_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                          fbe_object_id_t object_id,
                                                          fbe_rdgen_operation_t rdgen_operation,
                                                          fbe_u32_t num_ios);
static fbe_status_t kermit_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                              fbe_u32_t *position_p);
static fbe_bool_t kermit_get_and_check_degraded_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                         fbe_kermit_error_case_t *error_case_p,
                                                         fbe_u32_t  degraded_drive_index,
                                                         fbe_u32_t *degraded_position_p);
static fbe_status_t kermit_get_pdo_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                             fbe_kermit_error_case_t *error_case_p,
                                             fbe_object_id_t *pdo_object_id_p,
                                             fbe_u32_t error_number);
void kermit_setup(void);
void kermit_set_rg_options(void);
void kermit_unset_rg_options(void);
fbe_bool_t kermit_is_rg_valid_for_error_case(fbe_kermit_error_case_t *error_case_p,
                                             fbe_test_rg_configuration_t *rg_config_p);

/*!*******************************************************************
 * @var kermit_test_cases_raid_0_qual
 *********************************************************************
 * @brief Brief table of raid 0 test cases for qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid_0_qual[] =
{
    {
        "20 Retryable errors on read. The packet expiration causes the pdo to fail and RG to shutdown.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            20, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We either see a single non-retryable or we get aborted due to shutdown. 
         */
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */            
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */            
        1, /* One error case. */
    },

    {
        /* Inject a media error to cause us to mark verify on paged.
         * Inject a write error on paged so that the update of the paged fails, 
         * and causes the raid group to fail. 
         */
        "1 Media error causes update of paged. "
        "5 Retryable errors on write of metadata causes pdo to fail due to expiration.", __LINE__,
        1, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* Inject a write error on paged when we are marking verify.
             * Note the first and second error cases use the same drive.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* User data error causes us to want to update the paged metadata.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                KERMIT_NUM_READ_AND_SINGLE_VERIFY_ERRORS,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */

        /* Either the monitor catches us on the terminator queue and marks us failed because we are shutting down, 
         * or we complete before they catch us. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST},
        /* The timers we use are not exact so we do not check the retries.
         * We see zero shutdown errors since the error counts do not go off to the MDS. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        {0,0}, /* Non-retryable errors. */
        {KERMIT_ERR_COUNT_MATCH,0}, /* Shutdown error on failed metadata write */
        {0,0}, /* Correctable media errors. */
        {0,0}, /* Uncorrectable media errors. */
        {0,0}, /* correctable time stamp errors. */
        {0,0}, /* correctable write stamp errors. */
        {0,0}, /* uncorrectable time stamp errors. */
        {0,0}, /* uncorrectable write stamp errors. */
        {0,0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors above */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */            
        1, /* One error case. */
    },
    {
        "1 Retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
            1, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 16, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
            2, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_TRUE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on read. The drive will get shot on the 14th error.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, much more than needed. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        /* Non-retryable errors.
         * There are two possible cases.
         *    1) Before we can retry to a failed device and we see the non-retryable error.
         *    2) Request completes from drive and before we can retry the monitor has seen the edge state change.
         */
        {1, 0}, /* Non-retryable errors.*/
        {1, 1}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* error cases. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid_0_qual
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_raid_0_extended
 *********************************************************************
 * @brief These are additional test cases that we do not run during
 *        the standard qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid_0_extended[] =
{

    /* Qual test has the following test cases:
     *   20 retryable errors on read causing packet expiration and rg shutdown.
     *   1 media error causing update of paged, write of paged expires and rg shuts down.
     *     1 retryable error on read. 
     */
    {
        "2 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors above */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "3 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            3, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors above. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on read. The I/O gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            20, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 300, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */            
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */            
        1, /* error cases */
    },
    {
        "20 Retryable errors on read. The drive will get shot on the 14th error.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, much more than needed. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        /* Non-retryable errors.
         *  There are two possible cases.
         *    1) Before we can retry to a failed device and we see the non-retryable error.
         *    2) Request completes from drive and before we can retry the monitor has seen the edge state change.
         */
        {1, 0}, /* Non-retryable errors.*/
        {1, 1}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* error cases. */
    },
    /* Qual test has: 
     *   1 retryable error on write. 
     */ 
    {
        "2 Retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
            2, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "100 Retryable error on write.  Eventually the drive gets shot.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
            100, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        /* Non-retryable errors.
         *  There are two possible cases.
         *    We cannot get a shutdown error since R0 never marks the request waiting when
         *    it receives a failure.
         *    1) Before we can retry to a failed device, the request is aborted by
         *    monitor.
         *    2) Request completes from drive and raid returns error.
         *  
         *    We cannot get a shutdown error since R0 never marks the request waiting when
         *    it receives a failure.
         */
        {1, 0}, /* Non-retryable errors */
        {1, 1}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* error cases. */
    },
    {
        "100 Retryable errors on write.  Packet expiration causes PDO to fail after 200ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
            100, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX,FBE_U32_MAX}, /* Retryable errors */
        /* In this case the raid group realizes the fruts is timed out and causes rebuild logging to start.
         * We do not actually see the drive go away.
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        1, /* error cases */
    },
    {
        "5 Retryable errors on Write. Packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            10,/* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* status */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* qual */FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */            
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */            
        1, /* error cases */
    },
    {
        "2 Retryable errors on Zero.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
            2, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "100 Retryable error on Zero.  Eventually the drive gets shot.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
            100, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        /* Non-retryable errors.
         *  There are two possible cases.
         *    1) We retry to a failed device and we see the non-retryable error                                 .
         *    2) Request completes from drive and before we can retry the monitor has seen the edge state change.
         */
        {1, 0}, /* Non-retryable errors */
        {1, 1}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* error cases. */
    },
    {
        "100 Retryable errors on Zero.  Packet expiration causes PDO to fail after 200ms.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
            100, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_FALSE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* We may or may not see the drive go away due to multiple retry errors.
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1, 1, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        2, /* error cases */
    },
    {
        "5 Retryable errors on Zero. The I/O gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,  /* opcode to inject on*/
            10,/* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 300, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* status */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* qual */FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */            
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */            
        1, /* error cases */
    },
    /* Qual test has: Non-retryable error on read.
     */
    {
        "2 Non-retryable errors on read.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 5, 16, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on Write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
            2, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_TRUE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Non-retryable errors on Write.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
                2, /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
                2, /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on Zero.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
            2, /* times to insert error. */
            0,    /* raid group position to inject.*/
            FBE_TRUE,    /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Non-retryable errors on Zero.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
                2, /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
                2, /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid_0_extended
 **************************************/
/*!*******************************************************************
 * @var kermit_test_cases_parity_qual
 *********************************************************************
 * @brief These are parity test cases we run during qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_parity_qual[] =
{
    {
        /* Inject retryables, and non-retryables at the same time. 
         */
        "5 Retryable errors two positions, and a non-retryable on another.", __LINE__,
        0, /* lba */ 3, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                2,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 6, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We see zero non-retryable errors since the error counts do not go off to the MDS. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Retryable error on write.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            3, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on write.  The packet expiration causes the pdo to fail and rg to go degraded.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* We might see the non-retryable error if we retry before the monitor has a chance
         * to quiesce.  If we quiesce before we retry we don't see the error. 
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0,0}, /* c_crc */ {0,0}, /* u_crc */    
        2, /* One error case. */
    },
    {
        "Non-retryable error on write.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Non-retryable error and 5 retryable errors on write.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                2,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_parity_qual
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_single_parity_degraded
 *********************************************************************
 * @brief These are single parity degraded only cases.
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_single_parity_degraded[] =
{
    {
        "Degraded Raid Group. Parity is dead.  2 other drives have 3 Retryable errors on read." , __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                3,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                3,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
         /* 1 degraded position 
          */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
   {
        "Degraded Raid Group. Read to dead drive. 1 Non-Retryable error on degraded-read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* 1 degraded position. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
    },

    {
        "Degraded Raid Group. Read to dead drive. 1 Retryable error on degraded-read." , __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. Read to active drive. 5 Retryable error on read." , __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    {
        "Degraded Raid Group. Read to dead drive. 14 Retryable errors on degraded read. Packet expires and another pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* 1 degraded position. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries. 
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
    },

    {
        "Degraded Raid Group. Read to dead drive. 14 Retryable errors on degraded read. Drive gets shot on 14th error, raid group shuts down briefly.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* We may or may not see the non-retryable errors depending on the timing.
         * 1) Either the retry occurs and the errors are seen 
         * 2) Or the monitor marks the iots for abort before the retry occurs with the edge disabled. 
         * 3) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. Read to dead drive. 1 soft media error on degraded-read." , __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_SOFT_MEDIA_ERROR,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. Read to dead drive. 1 hard media error on degraded-read." , __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {1}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {1}, /* u_crc */
        1, /* One error case. */
    },

    {
        "Degraded Raid Group. Small Write to dead position. 2 Retryable errors on Parity.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. Small Write to dead position. 14 Retryable errors on Parity. Packet expires and another pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {FBE_U32_MAX}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. Small Write to dead position. 14 Retryable errors on Parity. Drive gets shot on 14th error, raid group shuts down briefly.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0,0}, /* Correctable media errors. */
        {0,0}, /* Uncorrectable media errors. */
        {0,0}, /* correctable time stamp errors. */
        {0,0}, /* correctable write stamp errors. */
        {0,0}, /* uncorrectable time stamp errors. */
        {0,0}, /* uncorrectable write stamp errors. */
        {0,0}, /* correctable_coherency. */ {0,0}, /* c_crc */ {0,0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. Small Write to dead position. 1 Non-Retryable errors on Parity.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            1, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. MR3 Write. 2 Retryable errors on Parity.", __LINE__,
        0, /* lba */ 256, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. MR3 Write. 14 Retryable errors on Parity. Packet expires and another pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 256, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {FBE_U32_MAX}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. MR3 Write. 14 Retryable errors on Parity. Drive gets shot on 14th error, raid group shuts down briefly.", __LINE__,
        0, /* lba */ 256, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

	{
        "Degraded Raid Group. MR3 Write. 1 Non-Retryable errors on Parity.", __LINE__,
        0, /* lba */ 256, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            1, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**********************************************
 * end kermit_test_cases_degraded_parity_qual *
 **********************************************/

/*!*******************************************************************
 * @var kermit_test_cases_degraded_r6_qual
 *********************************************************************
 * @brief These are parity test cases we run during qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_degraded_r6_qual[] =
{
    {
        "Degraded, 1 Retryable error on read of row parity.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Degraded, 1 Retryable error on read of diag parity.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Data position Degraded, 3 Retryable errors on write to row parity.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            3, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Data position Degraded, 3 Retryable errors on write to diag parity.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            3, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Data position Degraded, 5 Retryable errors on write to row+diag parity. The packet expiration causes the pdos to fail and rg to go shutdown.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* In this case the raid group realizes the fruts is timed out and causes rebuild logging to start.
         * We do not actually see the drive go away.
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* One error case. */
    },
    {
        "Data position Degraded, Non-retryable error on write to row parity.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Data position Degraded, 1 Non-retryable error (row) and 5 retryable errors (diag) on write to parity.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Row parity Degraded, 1 Non-retryable error on write to data position and 5 retryable errors on write to diag parity.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Diag parity Degraded, 1 Non-retryable error on write to data position and 5 retryable errors on write to row parity.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
     {
        "Row+Diag parity Degraded, 1 Non-retryable error on write to data position.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { 0, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* One degraded position. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_degraded_r6_qual
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_parity_extended
 *********************************************************************
 * @brief These are R5/R3 test cases where errors occur during
 *        recovery or normal verify.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_parity_extended[] =
{
    /* Qual test has: 
     *   1 Retryable error on read. 
     */
    {
        "1 Retryable port error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /*! @note Num Errors must be 1 */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* Only one error occurs since we use redundancy to fix the error.
         */
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on read (correctable).", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* Only one error occurs since we use redundancy to fix the error.
         */
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* Degraded Raid Group with the drive we are reading from being dead.
     * This forces us to the degraded read state machine                   .
     */
    {
        "2 Retryable errors on read.  Degraded Raid Group.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* 1 degraded position
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* Qual test has: 
     *    1 Retryable error on write.
     */
    {
        "2 Retryable errors on write.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on write.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on write.  Eventually the drive gets shot.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, much more than needed to fail drive. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* If our retry runs before the raid group quiesces then we see the non-retryable error, 
         * otherwise the quiesce occurs and we don't see the drive die. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },
    {
        "1 Retryable error on pre-read for a 468 write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on pre-read for a 468 write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on pre-read for a 468 write.  Packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 6, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */    
        1, /* One error case. */
    },
    {
        "5 Retryable errors on pre-read for a 468 write.  Write packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* status */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* qual */FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "20 Retryable errors on pre-read for a 468 write.  Eventually drive gets shot.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, much more than needed to fail drive. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* If our retry runs before the raid group quiesces then we see the non-retryable error, 
         * otherwise the quiesce occurs and we don't see the drive die. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },
    {
        "2 retryable errors on 2 different drives for a pre-read for a 468 write.", __LINE__,
        /* test with offset lba of 0x40 */
        0x40, /* lba */ 0x80, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {4}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    /* MR3 pre-read test cases have been removed since, MR3's can only occur on aligned full stripe writes.
     * In other words, MR3 should never have to do a pre-read.
     */

    /* RCW pre-read test cases 
     */
    {
        "1 retryable error for a pre-read for a RCW write.", __LINE__,
        0, /* lba */ 0x80, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 4, FBE_U32_MAX}, /* Test widths for which this I/O is an RCW w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 retryable errors for a pre-read for a RCW write.", __LINE__,
        /* test with offset of 0x10 with write on one drive*/
        0x10, /* lba */ 0x70, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 4, FBE_U32_MAX}, /* Test widths for which this I/O is an RCW w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 retryable errors on two positions for a pre-read for a RCW write.", __LINE__,
        /* test with offset of 0x10 with write on two drives */
        0x10, /* lba */ 0x90, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 4, FBE_U32_MAX}, /* Test widths for which this I/O is an RCW w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {10}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 retryable errors on two position for a pre-read for a RCW write.  "
        "One of the packets expires and the drive dies.", __LINE__,
        /* test with offset of 2 positions => 0x100 blocks  */
        0x100, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 6, FBE_U32_MAX}, /* Test widths for which this I/O is an RCW w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 retryable errors on two positions for a pre-read for a RCW write."
        "The packets expire and the rg shuts down", __LINE__,
        0x40, /* lba */ 0x80, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is an RCW w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {FBE_U32_MAX}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 retryable errors on two positions and 1 non-retryable on 1 position for a pre-read for a RCW write."
        "max position is dead other retries", __LINE__,
        0xf, /* lba */ 0x206, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            }
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We see zero non-retryable errors since the error counts do not go off to the MDS. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    /* Qual test has: 
     *     5 Retryable errors on write.  The packet expiration causes the pdo to fail and rg to go degraded.
     */
    {
        "5 Retryable errors on write.  Write packet gets aborted after 200 ms."
        "Since it is a write, we let the retries finish.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            5,/* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Depending on the timing we will either see success or aborted. 
         * Success if we are aborted during the retries, but let the retries finish. 
         * Aborted if we get aborted before the write even has a chance to get issued. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Qual we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "1 Retryable error on zero.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 4, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "3 Retryable errors on zero.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            3, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 4, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0, 0}, /* Retryable errors */
        {1, 2}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* Two error case. */
    },
    /* Qual test has: 
     *    Non-retryable error on write. 
     */
    {
        "1 Non-retryable error and 5 retryable errors on write.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {       /* retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject. (parity drive) */
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                    /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                2,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* We see the non-retryable error for every retryable error also since we handle 
         * the retryable before the non-retryable. 
         */
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on zero.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, 4, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_parity_extended
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_parity_verify_raid_3_normal
 *********************************************************************
 * @brief These are R3 test cases where errors occur during
 *        recovery or normal verify.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_parity_verify_raid_3_normal[] =
{
    {
        "5 Retryable errors on zero.  Packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* no expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* error cases. */
    },
    /* We are doing a zero of the stripe and the abort causes a coherency error.
     */
    {
        "5 Retryable errors on zero.  Zero packet gets aborted after 200 ms."
        "Since it is a zero, we let the retries finish.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Depending on the timing we will either see success or aborted. 
         * Success if we are aborted during the retries, but let the retries finish. 
         * Aborted if we get aborted before the write even has a chance to get issued. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Qual we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    {
        "14 Retryable errors on one position and one retryable on another position for read.", __LINE__,
        /* The second error is needed to cause verify to retry the errors.
         */
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                /* Retryable error */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE, 
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                /* Retryable error */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE, 
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* One for read. Two in verify. One retry error on another position in verify
         * causes us to see the first error again. 
         */
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on 2 positions on read.  Packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * One retryable in context of read and either 1 or 2  per position (2 or 4 total)
         * in the context of verify. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on two positions on read.  Read packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    /* Since the drive was shot, verify retries and the media error goes away.
     */
    {
        "1 media error on a read sends us to recovery verify."
        "In rvr we get 14 retryable errors and drive is shot and we continue degraded.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                KERMIT_NUM_READ_AND_SINGLE_VERIFY_ERRORS, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* It is possible for us to get back a retryable error, but before we can retry it, the 
         * drive has gone away and the monitor has asked us to quiesce.  When we get the continue we 
         * continue degraded and do not see the dead error. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },

    {
        "1 Media error and 1 retryable error on read. "
        "When we get to verify we also detect 1 media error and 1 Retryable error on read.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* 1 retryable in read. 
         * In rvr we try to recover and run into a media error and see the 2nd retryable 
         * error.
         */
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* Note that Media errors only get logged when we call into xor. 
     * In this case because we retry errors, we never get to xor with the media error. 
     */
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr 5 recoverable errors occur and packet expires after 300ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr 5 recoverable errors occur and packet gets aborted after 300ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No Expiration time. */ 300, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error and 1 retryable error on pre-read for a 468 write."
        "When we get to verify we detect 1 media error and 1 Retryable error on read.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 8, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error on pre-read for a RCW write. "
        "When we get to verify we detect 2 Retryable errors on read. ", __LINE__,
        /* write to two drives with pre read errors on both drives */
        0x10, /* lba */ 0x90, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is an MR3 w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error on read. When we get to verify we detect 1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_parity_verify_raid_3_normal
 **************************************/
/*!*******************************************************************
 * @var kermit_test_cases_parity_verify_raid_5_normal
 *********************************************************************
 * @brief These are R5 test cases where errors occur during
 *        recovery or normal verify.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_parity_verify_raid_5_normal[] =
{
    {
        "5 Retryable errors on zero.  Packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* no expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* error cases. */
    },
    /* We are doing a zero of the stripe and the abort causes a coherency error.
     */
    {
        "5 Retryable errors on zero.  Zero packet gets aborted after 200 ms."
        "Since it is a zero, we let the retries finish.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Depending on the timing we will either see success or aborted. 
         * Success if we are aborted during the retries, but let the retries finish. 
         * Aborted if we get aborted before the write even has a chance to get issued. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Qual we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    {
        "14 Retryable errors on one position and one retryable on another position for read.", __LINE__,
        /* The second error is needed to cause verify to retry the errors.
         */
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                /* Retryable error */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE, 
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                /* Retryable error */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE, 
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* One for read. Two in verify. One retry error on another position in verify
         * causes us to see the first error again. 
         */
        {3}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on 2 positions on read.  Packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * One retryable in context of read and either 1 or 2  per position (2 or 4 total)
         * in the context of verify. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on two positions on read.  Read packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    /* Since the drive was shot, verify retries and the media error goes away.
     */
    {
        "1 media error on a read sends us to recovery verify."
        "In rvr we get 14 retryable errors and drive is shot and we continue degraded.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                KERMIT_NUM_READ_AND_SINGLE_VERIFY_ERRORS, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* It is possible for us to get back a retryable error, but before we can retry it, the 
         * drive has gone away and the monitor has asked us to quiesce.  When we get the continue we 
         * continue degraded and do not see the dead error. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },

    {
        "1 Media error and 1 retryable error on read. "
        "When we get to verify we also detect 1 media error and 1 Retryable error on read.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* 1 retryable in read. 
         * In rvr we try to recover and run into a media error and see the 2nd retryable 
         * error.
         */
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* Note that Media errors only get logged when we call into xor. 
     * In this case because we retry errors, we never get to xor with the media error. 
     */
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr 5 recoverable errors occur and packet expires after 300ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr 5 recoverable errors occur and packet gets aborted after 300ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No Expiration time. */ 300, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error and 1 retryable error on pre-read for a 468 write."
        "When we get to verify we detect 1 media error and 1 Retryable error on read.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 16, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error on pre-read for a RCW write. "
        "When we get to verify we detect 2 Retryable errors on read. ", __LINE__,
        /* RCW with offset and write to only one drive */
        0x10, /* lba */ 0x10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is an MR3 w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error on read. When we get to verify we detect 1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_parity_verify_raid_5_normal
 **************************************/
/*!*******************************************************************
 * @var kermit_test_cases_parity_verify_raid_6_normal
 *********************************************************************
 * @brief These are R6 test cases where errors occur during
 *        recovery or normal verify.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_parity_verify_raid_6_normal[] =
{
    {
        /* We inject a media error to cause us to want to update paged metadata.
         * Then we cause the paged metadata update to also hit an error. 
         * After that we also inject metadata errors on a different drive so that 
         * the verify during activate hits some errors. 
         */
        "5 Retryable errors on metadata write.  Packet expires causing pdo to fail after 200ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error this is on the third drive from the end since
             * parity has rotated.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We see zero non-retryable errors since the error counts do not go off to the MDS.
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {0}, /* Non-retryable errors. */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "5 Retryable errors on zero.  Zero packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 4, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* error cases. */
    },
    /* We are doing a zero of the stripe and the abort causes time stamp errors.
     */
    {
        "5 Retryable errors on zero.  Zero packet gets aborted after 200 ms."
        "Since it is a zero, we let the retries finish.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */

        /* We only do width == 4 since the block count above and the elsz_multiplier == TRUE ensures 
         * that we have a zero for the entire stripe.  Widths beyond 4 will not be a full stripe, 
         * and will end up going down the write path. 
         */
        { 4, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Depending on the timing we will either see success or aborted. 
         * Success if we are aborted during the retries, but let the retries finish. 
         * Aborted if we get aborted before the write even has a chance to get issued. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Qual we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1 /* One error case. */
    },
    {
        "14 Retryable errors on one position and one retryable on another position for read (raid6).", __LINE__,
        /* The second error is needed to cause verify to retry the errors.
         */
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE, 
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* One for read. One in verify.  Then we use redundancy to reconstruct.
         */
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on 2 positions on read.  Since only 2 errors, raid 6 reconstructs.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2, 0}, /* Retryable errors */
        {0, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on 3 positions on read.  "
        "Read packet expires and pdos fail after 200 ms, causing the RG to fail.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         *  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         * --> Otherwise the monitor runs first and marks the iots aborted for shutdown. When the I/O finishes it sees 
         * we are aborted and bails out (no shutdown error). 
         */  
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0, 0}, /* Non-retryable errors. */
        {1, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "14 Retryable errors on 3 positions on read.  Read packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    /*! @todo causing interaction between verify and rebuild.
     */
#if 0
    /* Since the drive was shot, verify continues degraded.
     */
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr we get 14 retryable errors on two positions and drive is shot.", __LINE__,
        0, /* lba */ 128, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
            {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
#endif
    {
        "1 retryable on read forces us to rvr."
        "In rvr we get 14 retryables on 3 positions, which forces raid 6 rg shutdown.", __LINE__,
        0, /* lba */ 128, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* We may or may not see the non-retryable errors depending on the timing.
         * 1) Either the retry occurs and the errors are seen 
         * 2) Or the monitor marks the iots for abort before the retry occurs with the edge disabled. 
         * 3) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error and 1 retryable error on read. "
        "When we get to verify we also detect 1 media error and use redundancy to reconstruct.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* 1 retryable in read. 
         * In rvr we try to recover and only see the media error.
         */
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr 5 recoverable errors occur and packet expires and pdos fail after 300ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 media error on a read sends us to recovery verify. "
        "In rvr 5 recoverable errors occur and packet gets aborted after 300ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,  /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No Expiration time. */ 300, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error and 1 retryable error on pre-read for a 468 write."
        "When we get to verify we detect 1 media error and 1 Retryable error on read.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 8, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error and 1 retryable error on pre-read for a 468 write."
        "When we get to verify we detect 1 media error and 1 Retryable error on read.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 8, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error on pre-reads for a RCW write. "
        "When we get to verify we detect 2 Retryable errors on read. ", __LINE__,
        0x10, /* lba */ 0x80, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is an MR3 w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on read. Degraded Raid Group. Packet expires and another pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* 1 degraded position. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */    
        1, /* One error case. */
    },
    {
        "2 Non-retryable errors and 5 retryable errors on write.", __LINE__,
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        0, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                    /* retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                7,    /* times to insert error. */
                0,    /* raid group position to inject (primary position)*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                    /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                    /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* We see the non-retryable error for every retryable error also since we handle 
         * the retryable before the non-retryable. 
         */
        {KERMIT_ERR_COUNT_NON_ZERO}, /* Retryable errors */
        {KERMIT_ERR_COUNT_NON_ZERO}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 retryable errors on two positions for a pre-read for a RCW write."
        "The packets expire and the rg continues degraded", __LINE__,
        /* offset of 2 drives (0x100 blocks)  */
        0x100, /* lba */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 6, FBE_U32_MAX}, /* Test widths for which this I/O is an MR3 w/ pre-read. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {FBE_U32_MAX}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Media error on read. When we get to verify we detect 1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_parity_verify_raid_6_normal
 **************************************/
/*!*******************************************************************
 * @var kermit_test_cases_parity_verify_shared_normal
 *********************************************************************
 * @brief These are parity (R5/R3/R6)test cases where errors occur during
 *        recovery or normal verify.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_parity_verify_shared_normal[] =
{
    {
        "1 Media error on pre-read for a 468 write."
        "When we get to verify we detect 1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                4, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, 16, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths for which this I/O is a 468. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {1}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_parity_verify_shared_normal
 **************************************/
/*!*******************************************************************
 * @var kermit_test_cases_raid5_failure
 *********************************************************************
 * @brief These are raid 5 and raid 3 test cases where the raid group
 *        goes shutdown.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid5_failure[] =
{
    {
        "14 Retryable errors on two positions for read. The drives will get shot on the 14th error. ", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* We may or may not see the non-retryable errors depending on the timing.
         * 1) Either the retry occurs and the errors are seen 
         * 2) Or the monitor marks the iots for abort before the retry occurs with the edge disabled. 
         * 3) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE}, /* Retryable errors */
        /* This can be 0, 1 or two depending on timing.
         * The retry could either not happen if the monitor is quiescing or the retry 
         * could see the bad edge or the retry could see that the drive finished the reset and is ready. 
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */ 
        {0, 1}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        2, /* # error cases. */
    },
    /* Degraded Raid Group with the drive we are reading from being dead.
     * This forces us to the degraded read state machine. 
     * The drive will get shot on the 14th error Raid group shuts down briefly, but comes 
     * back alive after PDO resets. 
     */
    {
        "14 Retryable errors on read.  Degraded raid group, raid group shuts down.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* We may or may not see the non-retryable errors depending on the timing.
         * 1) Either the retry occurs and the errors are seen 
         * 2) Or the monitor marks the iots for abort before the retry occurs with the edge disabled. 
         * 3) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0, 0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },

    /*! @todo need to add. 
     * 14 Retryable errors on Write.
     *    Degraded Raid Group with the drive we are writing to being dead.
     * The drive will get shot on the 14th error Raid group shuts down briefly, but comes 
     * back alive after PDO resets. 
     */
    {
        "Non-retryable error on read.  Raid group already degraded.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
         /* 1 degraded position 
          */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        /* --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         * --> Otherwise the monitor runs first and marks the iots aborted for shutdown. When the I/O finishes it sees 
         * we are aborted and bails out (no shutdown error). 
         */
        {1, 1}, /* Non-retryable errors */
        {0, 1}, /* Shutdown error. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* Two error cases. */
    },
    /*! @todo need to add 
     *  Non-retryable error on Write.  Raid group already degraded.
     */
    {
        "2 Non-retryable errors on read.   Raid group shuts down.", __LINE__,
        0, /* lba */ 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH-1, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Non-retryable errors on Write.  Raid group shuts down.", __LINE__,
        /* Write on two drives with errors on pre reads on both drives */ 
        0x40, /* lba */ 0x80, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH-1, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* Just one error case.*/
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid5_failure
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_mirror_qual
 *********************************************************************
 * @brief These are mirror test cases we run during qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_mirror_qual[] =
{
    /* 1 Retryable error on read.  Degraded Raid Group. 
     *    Degraded Raid Group with the drive we are reading from being dead.
     *    This forces us to the degraded read state machine.
     */
    {
        "9 retryable on one area of the I/O, uncorrectable media errors on the other area of the I/O."
        "eventually the I/O is aborted.  Returned status should be aborted.",
        __LINE__,
        /* We use a large I/O size so the I/O is broken up. The first piece of the I/O gets a retryable error. The
         * second piece of the I/O gets media errors. 
         */
        0, /* lba */ KERMIT_MAX_BACKEND_IO_SIZE * 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_TRUE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                7,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            }, 
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                7,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            }, 
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                9,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA_CHUNK_1,    /* Location to inject. */
                1,    /* tag value only significant in case b_inc_for_all_drives is true*/
            }, 
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                9,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA_CHUNK_1,    /* Location to inject. */
                2,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        4, /* Num Errors */
        /* no degraded position  */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  700, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* qualifer we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_ERR_COUNT_NON_ZERO, KERMIT_ERR_COUNT_NON_ZERO}, /* Retryable errors */
        {0, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {3, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* Two error cases. */
    },
    {
        /* Inject a media error to cause verify to get marked.
         * Then we cause the paged metadata update hit enough errors to cause the packet to expire and PDO to fail. 
         */
        "5 Retryable errors on write metadata.  Write packet expires and pdo fails after 100 ms.", __LINE__,
        1, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error to cause us to mark verify.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                2,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error causes PDO to expire packet and fail drive.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         *  
         * Metadata does not get the error bitmask so we do not see any non-retryable errors. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {1, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        /* Inject a media error to cause verify to get marked.
         * Then we cause the paged metadata update to hit enough errors to cause the packet to expire and PDO to fail. 
         * After that we also inject metadata errors on a different drive so that the verify during activate hits some 
         * errors. 
         */
        "5 Retryable errors on write metadata.  "
        "Write packets expire and pdos fail after 100 ms causing rg shutdown.", __LINE__,
        1, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error to cause us to mark verify.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                6,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error causes PDO to expire packet and fail.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error causes PDO to expire packet and fail.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Either the monitor catches us on the terminator queue and marks us failed because we are shutting down, 
         * or we complete before they catch us. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE},
        /* The timers we use are not exact so we do not check the retries.
         *  
         * Metadata does not get the error bitmask so we do not see any non-retryable errors. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {1, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Retryable errors on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on write.  Write packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We either see the retryable error or not. 
         */
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "Non-retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_mirror_qual
 **************************************/


/*!*******************************************************************
 * @var kermit_test_cases_degraded_mirror_qual
 *********************************************************************
 * @brief These are degraded mirror test cases we run during qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_degraded_mirror_qual[] =
{
    /* 1 Retryable error on read.  Degraded Raid Group. 
     *    Degraded Raid Group with the drive we are reading from being dead.
     *    This forces us to the degraded read state machine.
     */
    {
		
        "Degraded on second drive, 9 retryable on one area of the I/O, uncorrectable media errors on the other area of the I/O."
        "eventually the I/O is aborted.  Returned status should be aborted.",
        __LINE__,
        /* We use a large I/O size so the I/O is broken up. The first piece of the I/O gets a retryable error. The
         * second piece of the I/O gets media errors. 
         */
        0, /* lba */ KERMIT_MAX_BACKEND_IO_SIZE * 2, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                4,    /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            }, 
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                4,    /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                1,    /* tag value only significant in case b_inc_for_all_drives is true*/
            }, 
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                9,    /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA_CHUNK_1,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            }, 
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                9,    /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA_CHUNK_1,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        4, /* Num Errors */
        /* degraded position  */
        {FBE_RAID_MAX_DISK_ARRAY_WIDTH- 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  700, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* qualifer we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_ERR_COUNT_NON_ZERO, KERMIT_ERR_COUNT_NON_ZERO}, /* Retryable errors */
        {0, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {4, 4}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /*two error cases*/
    },
    {
        /* Inject a media error to cause verify to get marked.
         * Then we cause the paged metadata update hit enough errors to cause the packet to expire and PDO to fail. 
         */
        "Degraded on first drive, 5 Retryable errors on write metadata.  Write packet expires and pdo fails after 100 ms.", __LINE__,
        1, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error to cause us to mark verify.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                6,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error causes PDO to expire packet and fail drive.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         *  
         * Metadata does not get the error bitmask so we do not see any non-retryable errors. 
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {1, 1, 1}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        1, /* error cases. */
	},
    {
        /* Inject a media error to cause verify to get marked.
         * Then we cause the paged metadata update to hit enough errors to cause the packet to expire and PDO to fail. 
         * After that we also inject metadata errors on a different drive so that the verify during activate hits some 
         * errors. 
         */
        "5 Retryable errors on write metadata.  "
        "Write packets expire and pdos fail after 100 ms causing rg shutdown.", __LINE__,
        1, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error to cause us to mark verify.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                6,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error causes PDO to expire packet and fail.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error causes PDO to expire packet and fail.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
         0, /* Number of I/O errors we expect. */
        /* Either the monitor catches us on the terminator queue and marks us failed because we are shutting down, 
         * or we complete before they catch us. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE},
        /* The timers we use are not exact so we do not check the retries.
         *  
         * Metadata does not get the error bitmask so we do not see any non-retryable errors. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {1}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "Degraded on seconds drive, 1 Retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            }
        },
        1, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Degraded on first drive, 2 Retryable errors on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, 
    },
	{
        "Degraded on second drive, 5 Retryable errors on write.  Write packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We either see the retryable error or not. 
         */
        {KERMIT_ERR_COUNT_NON_ZERO, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "Degraded on first drive, Non-retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_degraded_mirror_qual
 **************************************/


/*!*******************************************************************
 * @var kermit_test_cases_mirror_extended
 *********************************************************************
 * @brief These are mirror test cases we run during extended testing.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_mirror_extended[] =
{
    /* Qual test has: 
     * 1 error on writing paged metadata. 
     * 1 retryable on read. 
     */
    {
        "2 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2,    /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* Only one error occurs since we use redundancy to fix the error.
         */
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* 1 Retryable error on read.  Degraded Raid Group. 
     *    Degraded Raid Group with the drive we are reading from being dead.
     *    This forces us to the degraded read state machine.
     */
    {
        "1 Retryable error on read.  Degraded Raid Group."
        " Degraded Raid Group with the drive we are reading from being dead.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            1, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* 1 degraded position 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* Degraded Raid Group with the drive we are reading from being dead.
     * This forces us to the degraded read state machine                   .
     */
    {
        "2 Retryable errors on read.  Degraded Raid Group.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* 1 degraded position 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on read to 2 drive R1. Degraded Raid Group. Drives die and raid group shuts down.", __LINE__,
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        /* 1 degraded position. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* We may or may not see the non-retryable errors depending on the timing.
         * 1) Either the retry occurs and the errors are seen 
         * 2) Or the monitor marks the iots for abort before the retry occurs with the edge disabled. 
         * 3) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, 
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        1, /* Three error case. */
    },
    {
        "14 Retryable errors on read to 3 drive r1. Degraded Raid Group. Drives die and raid group shuts down.", __LINE__,
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* 1 degraded position. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* We may or may not see the non-retryable errors depending on the timing.
         * 1) Either the retry occurs and the errors are seen 
         * 2) Or the monitor marks the iots for abort before the retry occurs with the edge disabled. 
         * 3) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {KERMIT_ERR_COUNT_MATCH}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0, 0, 0, 0}, /* c_crc */ {0, 0, 0, 0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "14 Retryable errors on read. Degraded Raid Group. Read packet expires and PDO fails after 200 ms.", __LINE__,
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* 1 degraded position. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        200, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The number of retryable errors we see depends on the timing.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {1, 0, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        3, /* Three error case. */
    },
    /* Qual test has 2 retryable on write.
     */
    {
        "5 Retryable errors on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on write.  Eventually the drive gets shot.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, twice what we need to shoot drive. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* If our retry runs before the raid group quiesces then we see the non-retryable error, 
         * otherwise the quiesce occurs and we don't see the drive die. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {1 ,0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* Two error case. */
    },
    {
        "5 Retryable errors on 2 drives for write.  "
        "Write packets expire and pdos fail after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* 1) Either the monitor marks the iots for abort before the retry occurs with the edge disabled
         * 2) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* In this case the raid group realizes the fruts is timed out and causes rebuild logging to start.
         * We do not actually see the drive go away.
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 1, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        2, /* error cases. */
    },
    {
        "5 Retryable errors on read.  Read packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 10, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on write.  Write packet gets aborted after 200 ms."
        "Since it is a write, we let the retries finish.", __LINE__,
        0, /* lba */ 10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Depending on the timing we will either see success or aborted. 
         * Success if we are aborted during the retries, but let the retries finish. 
         * Aborted if we get aborted before the write even has a chance to get issued. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Qual we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Retryable errors on zero.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on zero.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on zero.  Eventually the drive gets shot.", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, Twice what we need to shoot drive. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* If our retry runs before the raid group quiesces then we see the non-retryable error, 
         * otherwise the quiesce occurs and we don't see the drive die. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },
    {
        "5 Retryable errors on zero.  Zero packet gets aborted after 200 ms."
        "Since it is a zero, we let the retries finish.", __LINE__,
        0, /* lba */ 1024, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        /* Depending on the timing we will either see success or aborted. 
         * Success if we are aborted during the retries, but let the retries finish. 
         * Aborted if we get aborted before the write even has a chance to get issued. 
         */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Qual we expect. */ FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_WR_NOT_STARTED_CLIENT_ABORTED},
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /*! @todo add zero tests. 
     */
    {
        "Non-retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_TRUE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                1,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                1,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0, 0},/* Retryable errors */
        {2, 1}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },
    {
        "Non-retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Non-retryable error and 5 retryable errors on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                    /* retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject (primary position)*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                    /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* We see the non-retryable error for every retryable error also since we handle 
         * the retryable before the non-retryable. 
         */
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on zero.", __LINE__,
        0, /* lba */ 1024, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Non-retryable error and 5 retryable errors on zero.", __LINE__,
        0, /* lba */ 1024, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                    /* retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject. (parity drive) */
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
                1,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* We see the non-retryable error for every retryable error also since we handle 
         * the retryable before the non-retryable. 
         */
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    /* Since the drive was shot, verify retries and the media error goes away.
     */
    {
        "1 media error on a read sends us to recovery verify."
        "In rvr we get 14 retryable errors and drive is shot and we continue degraded.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_TRUE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                5,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                5,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        4, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* It is possible for us to get back a retryable error, but before we can retry it, the 
         * drive has gone away and the monitor has asked us to quiesce.  When we get the continue we 
         * continue degraded and do not see the dead error. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {1, 1}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_mirror_extended
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_mirror_degraded_extended
 *********************************************************************
 * @brief These are mirror test cases we run during extended testing.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_mirror_degraded_extended[] =
{
    /* Qual test has: 
     * 1 error on writing paged metadata. 
     * 1 retryable on read. 
     */
    {
        "Degraded on third drive, 2 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },

			{
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                2,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {1}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Degraded on second position,5 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on both positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2,    /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect.(0 since even if one have error, and one is degraded, the 3rd should be there to save the day */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* Qual test has 2 retryable on write.
     */
    {
        "5 Retryable errors on write. Degraded Raid group (3 Way mirror only)", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on write. Degraded RG 2 disk only, PACO-R keeps drive online no drive failure", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, twice what we need to shoot drive. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        1, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE},
        /* If our retry runs before the raid group quiesces then we see the non-retryable error, 
         * otherwise the quiesce occurs and we don't see the drive die. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE}, /* Retryable errors */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* one error case. */
    },
    {
        "5 Retryable errors on 1 drives for write with other one degraded"
        "Write packets expire and pdos fail after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH -2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* 1) Either the monitor marks the iots for abort before the retry occurs with the edge disabled
         * 2) Or the monitor sees one drive die and starts quiescing and io waits before doing retry. 
         *    In this case the monitor sees the I/O is waiting and completes the I/O (shutdown case).
         */
        { KERMIT_ERR_COUNT_NON_ZERO, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* In this case the raid group realizes the fruts is timed out and causes rebuild logging to start.
         * We do not actually see the drive go away.
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0, 1, 0}, /* Shutdown errors. */
        {0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0}, /* uncorrectable write stamp errors. */
        {0, 0, 0}, /* correctable_coherency. */ {0, 0, 0}, /* c_crc */ {0, 0, 0}, /* u_crc */
        2, /* error cases. */
    },
	
    {
        "5 Retryable errors on read.  Position 2 degraded, Read packet gets aborted after 200 ms.", __LINE__,
        0, /* lba */ 10, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                10,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {KERMIT_ERR_COUNT_NON_ZERO}, /* Retryable errors */
        {0, 0, 0, 0}, /* Non-retryable errors. */
        {0, 0, 0, 0}, /* Shutdown errors. */
        {0, 0, 0, 0}, /* Correctable media errors. */
        {0, 0, 0, 0}, /* Uncorrectable media errors. */
        {0, 0, 0, 0}, /* correctable time stamp errors. */
        {0, 0, 0, 0}, /* correctable write stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable time stamp errors. */
        {0, 0, 0, 0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on write on a degraded RG.  Write packet gets aborted after 200 ms."
        "Since it is a write, we let the retries finish.", __LINE__,
        0, /* lba */ 10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors. */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Retryable errors on zero. Degraded at position 1", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on zero. Degraded", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "20 Retryable errors on zero. Degraded RG, Eventually the drive gets shot but RG should stay alive", __LINE__,
        0, /* lba */ 64, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE * 2, /* times to insert error, Twice what we need to shoot drive. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* If our retry runs before the raid group quiesces then we see the non-retryable error, 
         * otherwise the quiesce occurs and we don't see the drive die. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },
    {
        "5 Retryable errors on zero on degraded RG.  Zero packet gets aborted after 200 ms."
        "Since it is a zero, we let the retries finish.", __LINE__,
        0, /* lba */ 1024, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,  /* opcode to inject on*/
            5, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
        0, /* no expiration time. */  200, /* abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so the number of retries might vary.
         */
        {5}, /* Retryable errors */
        {0}, /* Non-retryable errors. */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Non-retryable error on read. Degraded RG(2 drive only) RG fails", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_TRUE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,    /* Location to inject. */
                1,    /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        { 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    {
        "Non-retryable error on write. Degraded", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Non-retryable error and 5 retryable errors on write. Degraded at positon 2", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                    /* retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH -1,    /* raid group position to inject (primary position)*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                    /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* We see the non-retryable error for every retryable error also since we handle 
         * the retryable before the non-retryable. 
         */
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
	
    {
        "Non-retryable error on zero. Degraded at position 0 and RG dies", __LINE__,
        0, /* lba */ 1024, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME, /* opcode to inject on */
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "1 Non-retryable error and 5 retryable errors on zero. Degraded", __LINE__,
        0, /* lba */ 1024, /* blocks */ FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                    /* retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH -1,    /* raid group position to inject. (parity drive) */
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                /* non-retryable */
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH -1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* We see the non-retryable error for every retryable error also since we handle 
         * the retryable before the non-retryable. 
         */
        {5}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },

    /* Since the drive was shot, verify retries and the media error goes away.
     */
    {
        "1 media error on a read sends us to recovery verify."
        "In rvr we get 14 retryable errors and drive is shot and we continue degraded.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        3, /* Num Errors */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
		{ 3, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* It is possible for us to get back a retryable error, but before we can retry it, the 
         * drive has gone away and the monitor has asked us to quiesce.  When we get the continue we 
         * continue degraded and do not see the dead error. 
         */
        {KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE, KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE}, /* Retryable errors */
        {1, 0}, /* Non-retryable errors */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* One error case. */
    },

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_mirror_degraded_extended
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_raid10_qual
 *********************************************************************
 * @brief These are raid10 test cases we run during qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid10_qual[] =
{
    {
        /* Inject a media error to cause us to mark needs verify.  The
         * mark needs verify takes errors and the drive fails. 
         */
        "5 Retryable errors on write of metadata.  Write packet expires and PDO fails after 100 ms (2 + 2 only)", __LINE__,
        0, /* lba */ 2, /* blocks(positions) */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data and metadata error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Inject read error on both positions.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                2,    /* times to insert error. */
                0,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 4, FBE_U32_MAX, FBE_U32_MAX}, /* Only test 2 + 2 striped mirrors */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, KERMIT_INVALID_STATUS },
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, KERMIT_INVALID_STATUS },
        {FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {1}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "2 Retryable errors on read (2 + 2 only)", __LINE__,
        0, /* lba */ 2, /* blocks(width) */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on all positions since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        4, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 4, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /*  Only test 2 + 2 striped mirrors */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors (1 for each mirror pair) */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                0, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                1, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
                2, /* times to insert error. */
                1, /* raid group position to inject.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                1, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "2 Retryable errors on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "5 Retryable errors on write.  Write packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            5, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "1 Non-retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_TRUE, /*inc for all drives */
       {
           {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
            1, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            1, /* tag value only significant in case b_inc_for_all_drives is true*/
           },
           {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
            1, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            1, /* tag value only significant in case b_inc_for_all_drives is true*/
           },
       },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0, 0}, /* Retryable errors */
        {1, 2}, /* Non-retryable errors (due to mirror read optimization it could be either value)*/
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /* Two error cases. */
    },
    {
        "1 Non-retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            0, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {1}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid10_qual
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_degraded_raid10_qual
 *********************************************************************
 * @brief These are degraded raid10 test cases we run during qual.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_degraded_raid10_qual[] =
{
    {
        "Degraded at position 1, 5 Retryable errors on write of metadata.  Write packet expires and PDO fails after 100 ms (2 + 2 only)", __LINE__,
        0, /* lba */ 2, /* blocks(positions) */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data and metadata error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Inject read error on both positions.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX },
        { 4, FBE_U32_MAX, FBE_U32_MAX}, /* Only test 2 + 2 striped mirrors */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, KERMIT_INVALID_STATUS },
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, KERMIT_INVALID_STATUS },
        {KERMIT_ERR_COUNT_NON_ZERO, KERMIT_ERR_COUNT_NON_ZERO}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {2, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    {
        "Degraded on position 0, 2 Retryable errors on read (2 + 2 only)", __LINE__,
        0, /* lba */ 2, /* blocks(width) */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_TRUE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        /* Inject on all positions but he degraded one since it is not deterministic which drive 
         * the mirror will read from. 
         */
        {
			
			{
					FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
					FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
					1,    /* times to insert error. */
					1,    /* raid group position to inject.*/
					FBE_FALSE,    /* Does drive die as a result of this error? */
					KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
					0, /* tag value only significant in case b_inc_for_all_drives is true*/
			},
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                2,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
			},
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on*/
                1,    /* times to insert error. */
                3,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        3, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { 4, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /*  Only test 2 + 2 striped mirrors */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors (1 for each mirror pair) */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Degraded at position 0, 2 Retryable errors on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Degraded at position 0, 2 Retryable errors on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {2}, /* Retryable errors */
        {0}, /* Non-retryable errors */
        {0}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    {
        "Degraded at position 0, 5 Retryable errors on write.  Write packet expires and pdo fails after 200 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {KERMIT_ERR_COUNT_NON_ZERO}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, KERMIT_ERR_COUNT_MATCH}, /* Non-retryable errors. */
        {2, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        2, /*two error cases*/
    },
    {
        "Degraded at position 0, 1 Non-retryable error on read.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_TRUE, /*inc for all drives */
       {
           {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, /* opcode to inject on */
            1, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_FALSE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            1, /* tag value only significant in case b_inc_for_all_drives is true*/
           }
           
       },
        1, /* Num Errors */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0,}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {2}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /*one error*/
    },
    {
        "Degraded at position 0, 1 Non-retryable error on write.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, /* opcode to inject on */
            2, /* times to insert error. */
            1, /* raid group position to inject.*/
            FBE_TRUE, /* Does drive die as a result of this error? */
            KERMIT_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        { 0 , FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* No expiration time. */  0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors */
        {2}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */
        1, /* One error case. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_degraded_raid10_qual
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_raid10_extended
 *********************************************************************
 * @brief These are RAID-10 test cases we run during extended testing.
 *
 * @note  These tests are striped mirror specific.  That is they we only
 *        work on a striped mirror.  For extended testing we run RAID-10,
 *        RAID-0 and RAID-1 tests.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid10_extended[] =
{
    /* So far this is empty since the qual test covers everything.
     */
    /*! @todo What about abort cases?
     */

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid10_extended
 **************************************/

/*!*******************************************************************
 * @var kermit_test_cases_raid5_only
 *********************************************************************
 * @brief These are raid 5 only cases.
 *        We need these specific cases since the location of
 *        these errors is specific to the raid type.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid5_only[] =
{
    {
        /* We inject a media error to cause an update of paged metadata.
         * Then we cause the paged metadata update to also expire and pdo to fail. 
         */
        "5 Retryable errors on write of metadata.  Write packet expires and pdo fails after 100 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Metadata error this is on the third drive from the end since
             * parity has rotated.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        0, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         * We see zero non-retryable errors since the error counts do not go off to the MDS. 
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        {0, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {1, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid5_only
 **************************************/
/*!*******************************************************************
 * @var kermit_test_cases_raid3_only
 *********************************************************************
 * @brief These are raid 5 only cases.
 *        We need these specific cases since the location of
 *        these errors is specific to the raid type.
 *
 *********************************************************************/
fbe_kermit_error_case_t kermit_test_cases_raid3_only[] =
{
    /*! @todo what about case where multiple pdos fail?
     */
    /*! @todo what about case where we are degraded and the mark NR fails?
     */
    {
        /* We inject a media error to cause us to need to mark verify.
         * Then we cause the paged metadata update to take retryable errors and 
         * this eventually causes the pdo to fail due to expiration. 
         */
        "5 Retryable errors on write metadata.  Write packet expires and pdo fails after 100 ms.", __LINE__,
        0, /* lba */ 1, /* blocks */ FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /* opcode */ 
        FBE_TRUE, /* set rg expiration time? */ 
        FBE_FALSE, /* elsz multiplier */
        FBE_FALSE, /* inc for all drives */
        {
            /* User data error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
            /* Metadata error this is on the third drive from the end since
             * parity has rotated.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,   /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                KERMIT_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 5, FBE_U32_MAX, FBE_U32_MAX}, /* Test widths. */
        100, /* expiration time in msecs. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ KERMIT_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ KERMIT_INVALID_STATUS},
        /* The timers we use are not exact so we do not check the retries.
         */
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Retryable errors */
        /* There are two possible cases:
         * 1) either the rg sees the drive go away before we attempt another retry 
         * 2) we retry when the VD is not in ready and we get a non-retryable. 
         */
        {KERMIT_ERR_COUNT_MATCH, 0}, /* Non-retryable errors. */
        {0, 0}, /* Shutdown errors. */
        {0, 0}, /* Correctable media errors. */
        {0, 0}, /* Uncorrectable media errors. */
        {0, 0}, /* correctable time stamp errors. */
        {0, 0}, /* correctable write stamp errors. */
        {0, 0}, /* uncorrectable time stamp errors. */
        {0, 0}, /* uncorrectable write stamp errors. */
        {0, 0}, /* correctable_coherency. */ {0, 0}, /* c_crc */ {0, 0}, /* u_crc */
        1, /* error cases. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end kermit_test_cases_raid3_only
 **************************************/
/*!*******************************************************************
 * @def KERMIT_MAX_TABLES_PER_RAID_TYPE
 *********************************************************************
 * @brief Max number of error tables for each raid type.
 *
 *********************************************************************/
#define KERMIT_MAX_TABLES_PER_RAID_TYPE 10

/*!*******************************************************************
 * @var kermit_qual_error_cases
 *********************************************************************
 * @brief This is the set of test cases we will execute for each
 *        different raid type.
 *         This is a limited test set to test either very quick cases
 *         or test one of each type (retryable, non-retryable, etc.)
 * 
 *        Some raid types have more than one set of test case arrays
 *        to execute.
 * 
 *        Some test case arrays are common across raid types.
 *
 *********************************************************************/
fbe_kermit_error_case_t *kermit_qual_error_cases[KERMIT_RAID_TYPE_COUNT][KERMIT_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 1 */
    {
        &kermit_test_cases_mirror_qual[0],
		&kermit_test_cases_degraded_mirror_qual[0],
        NULL /* Terminator */
    },
    /* Raid 5 */
    {
        &kermit_test_cases_parity_qual[0],
        &kermit_test_cases_raid5_only[0],
        &kermit_test_cases_single_parity_degraded[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        &kermit_test_cases_parity_qual[0],
        &kermit_test_cases_single_parity_degraded[0],
        NULL /* Terminator */
    },
    /* Raid 6 */
    {
        &kermit_test_cases_parity_qual[0], 
        &kermit_test_cases_degraded_r6_qual[0],
        NULL /* Terminator */
    },
    /* Raid 0 */
    {
        &kermit_test_cases_raid_0_qual[0],
        NULL /* Terminator */
    },
    /* Raid 10 - Striped mirror specific errors*/
    {
        &kermit_test_cases_raid10_qual[0],
		&kermit_test_cases_degraded_raid10_qual[0],
        NULL /* Terminator */
    },
};

/*!*******************************************************************
 * @var kermit_extended_error_cases
 *********************************************************************
 * @brief This is the set of test cases we will execute for each
 *        different raid type.  These are extended tests that are designed
 *        to be more exhaustive and test the majority of cases.
 * 
 *        Some raid types have more than one set of test case arrays to execute.
 * 
 *        Some test case arrays are common across raid Non-retryable error on write.types.
 *
 *********************************************************************/
fbe_kermit_error_case_t *kermit_extended_error_cases[KERMIT_RAID_TYPE_COUNT][KERMIT_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 1 */
    {
        &kermit_test_cases_mirror_qual[0],
        &kermit_test_cases_mirror_extended[0],
		&kermit_test_cases_degraded_mirror_qual[0],
		&kermit_test_cases_mirror_degraded_extended[0],
        NULL /* Terminator */
    },
    /* Raid 5 */
    {
        &kermit_test_cases_parity_qual[0],
		&kermit_test_cases_raid5_only[0],
		&kermit_test_cases_parity_extended[0],
        &kermit_test_cases_parity_verify_raid_5_normal[0],
        &kermit_test_cases_parity_verify_shared_normal[0],
		&kermit_test_cases_raid5_failure[0],
        &kermit_test_cases_single_parity_degraded[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        &kermit_test_cases_parity_qual[0],
        &kermit_test_cases_raid3_only[0],
        &kermit_test_cases_parity_extended[0],
        &kermit_test_cases_parity_verify_raid_3_normal[0],
        &kermit_test_cases_parity_verify_shared_normal[0],
        &kermit_test_cases_raid5_failure[0],
        &kermit_test_cases_single_parity_degraded[0],
        NULL /* Terminator */
    },
    /* Raid 6 */
    {
        &kermit_test_cases_parity_qual[0],
        &kermit_test_cases_degraded_r6_qual[0],
        &kermit_test_cases_parity_extended[0],
        &kermit_test_cases_parity_verify_raid_6_normal[0], 
        &kermit_test_cases_parity_verify_shared_normal[0], 
        NULL /* Terminator */
    },
    /* Raid 0 */
    {
        &kermit_test_cases_raid_0_qual[0],
		&kermit_test_cases_raid_0_extended[0],
        NULL /* Terminator */
    },
    /* Raid 10 - RAID-10 specific tests, striper and mirror tests */
    {
        &kermit_test_cases_raid10_qual[0],
        &kermit_test_cases_raid10_extended[0],
		&kermit_test_cases_degraded_raid10_qual[0],
        NULL /* Terminator */
    },
};

/*!***************************************************************************
 * @var     kermit_provision_drive_default_operations
 *****************************************************************************
 * @brief   This simply records the default value for the provision drive
 *          background operations.
 *
 *********************************************************************/
fbe_kermit_provision_drive_default_background_operations_t kermit_provision_drive_default_operations = { 0 };

fbe_status_t kermit_send_io(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_kermit_error_case_t *error_case_p,
                            fbe_api_rdgen_context_t *context_p,
                            fbe_rdgen_operation_t rdgen_operation,
                            fbe_time_t expiration_msecs,
                            fbe_time_t abort_msecs)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_block_count_t blocks;
    fbe_block_count_t elsz;
    fbe_api_rdgen_send_one_io_params_t params;

    elsz = fbe_test_sep_util_get_element_size(rg_config_p);

    if (error_case_p->b_element_size_multiplier)
    {
        blocks = error_case_p->blocks * elsz;
    }
    else
    {
        blocks = error_case_p->blocks;
    }
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = abort_msecs;
    params.msecs_to_expiration = expiration_msecs;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;

    params.rdgen_operation = rdgen_operation;
    params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    params.lba = error_case_p->lba;
    params.blocks = blocks;
    params.b_async = FBE_TRUE;
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}

fbe_status_t kermit_send_seed_io(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_kermit_error_case_t *error_case_p,
                                 fbe_api_rdgen_context_t *context_p,
                                 fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_block_count_t blocks;
    fbe_block_count_t elsz;

    elsz = fbe_test_sep_util_get_element_size(rg_config_p);

    if (error_case_p->b_element_size_multiplier)
    {
        blocks = error_case_p->blocks * elsz;
    }
    else
    {
        blocks = error_case_p->blocks;
    }
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    status = fbe_api_rdgen_send_one_io(context_p,
                                       lun_object_id,
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_SEP_0,
                                       rdgen_operation,
                                       FBE_RDGEN_LBA_SPEC_FIXED,
                                       0,
                                       elsz * rg_config_p->width, /* Clean up at least a full stripe. (this is a bit more than a stripe)*/
                                       FBE_RDGEN_OPTIONS_INVALID,
                                       0, 0,
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    return status;
}
static fbe_status_t kermit_wait_for_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_kermit_error_case_t *error_case_p,
                                             fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t index;
    /* Wait for the rebuilds to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    for ( index = 0; index < raid_group_count; index++)
    {

        fbe_object_id_t object_id;
        fbe_u32_t drive_index;
        fbe_u32_t degraded_position;

        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {
            rg_config_p++;
            continue;
        }

        fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &object_id);

        /* Make sure object is ready.
         */
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY,
                                                         KERMIT_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for drives that failed due to injected errors.
         */
        for ( drive_index = 0; drive_index < error_case_p->num_errors; drive_index++)
        {
            degraded_position = error_case_p->errors[drive_index].rg_position_to_inject;
            kermit_get_drive_position(rg_config_p, &degraded_position);
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, degraded_position);
        }

        /* Wait for drives that failed due to us degrading the raid group on purpose.
         */
        for ( drive_index = 0; drive_index < KERMIT_MAX_DEGRADED_POSITIONS; drive_index++)
        {
            if (kermit_get_and_check_degraded_position(rg_config_p,
                                                       error_case_p,
                                                       drive_index,
                                                       &degraded_position) == FBE_TRUE)
            {
                sep_rebuild_utils_wait_for_rb_comp(rg_config_p, degraded_position);
            }
        }
        sep_rebuild_utils_check_bits(object_id);
        rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
    return FBE_STATUS_OK;
}
static fbe_status_t kermit_get_verify_report(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_object_id_t lun_object_id,
                                             fbe_lun_verify_report_t *verify_report_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! @todo This method is only temporary unitl the striper object knows
     *        how to retrieve and coelsce the verify reports form the underlying
     *        mirror raid groups.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
    }
    else
    {
        status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, verify_report_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return status;
}
static fbe_status_t kermit_wait_for_verify(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_kermit_error_case_t *error_case_p,
                                           fbe_u32_t max_wait_msecs)
{
    fbe_status_t status;
    fbe_object_id_t                 rg_object_id;
    fbe_api_raid_group_get_info_t       raid_group_info;
    fbe_u32_t wait_msecs = 0;
    fbe_u32_t                       num_raid_groups_to_verify = 1;
    fbe_u32_t                       raid_group_index;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;

    /* Get the raid group object id
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* For RAID-10 we will inject the error into the associated mirror
     * raid group.  Therefore don't get the raid group info from the striper.
     */
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        /* Else we need to raid for all the mirror raid groups under the striper
         * to complete the abort verify.  Thus the number of raid groups to verify
         * is the downstream object count.
         */
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
        num_raid_groups_to_verify = downstream_object_list.number_of_downstream_objects;
    }

    /* For all raid groups that need to be verified
     */
    for ( raid_group_index = 0; raid_group_index < num_raid_groups_to_verify; raid_group_index++)
    {
        /* For RAID-10 we will inject the error into the associated mirror
         * raid group.  Therefore get the raid group information for the 
         * mirror raid group.
         */
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            rg_object_id = downstream_object_list.downstream_object_list[raid_group_index];
            status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* note that we only care that the verify is done, not that 
         * the chunk count is zero.  when we move away from chunk counts we 
         * will need this to check that verify is complete by looking at the 
         * checkpoint or some other info. 
         */
        while ((raid_group_info.ro_verify_checkpoint != FBE_LBA_INVALID) ||
               (raid_group_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
               (raid_group_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
               (raid_group_info.b_is_event_q_empty == FBE_FALSE))               
        {
            if (wait_msecs > max_wait_msecs)
            {
                 mut_printf(MUT_LOG_TEST_STATUS, "%s: rg: 0x%x ro chkpt: 0x%llx  rw chkpt: 0x%llx  err chkpt: 0x%llx", 
                           __FUNCTION__, rg_object_id, (unsigned long long)raid_group_info.ro_verify_checkpoint,
                           (unsigned long long)raid_group_info.rw_verify_checkpoint, (unsigned long long)raid_group_info.error_verify_checkpoint);
                MUT_FAIL_MSG("failed waiting for verify to finish");
            }
            fbe_api_sleep(200);
            wait_msecs += 200;
            status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

    }    /* end for all required raid groups */

    return FBE_STATUS_OK;
}
static fbe_status_t kermit_wait_for_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_kermit_error_case_t *error_case_p,
                                             fbe_u32_t raid_group_count)
{
    fbe_u32_t index;
    /* Wait for the verifies to finish 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for verifies to finish. ==", __FUNCTION__);
    for ( index = 0; index < raid_group_count; index++)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
            /* Wait for verifies to complete.
             */
            kermit_wait_for_verify(rg_config_p, error_case_p, KERMIT_WAIT_MSEC);
        }
        rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for verifies to finish. (successful)==", __FUNCTION__);
    return FBE_STATUS_OK;
}
static fbe_status_t kermit_wait_for_degraded(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_kermit_error_case_t *error_case_p,
                                             fbe_u32_t max_wait_msecs)
{
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_u32_t drive_index;
    fbe_u32_t degraded_position;
    fbe_api_raid_group_get_info_t       raid_group_info;
    fbe_u32_t wait_msecs = 0;
    /* Wait for drives that failed due to injected errors.
     */
    for ( drive_index = 0; drive_index < KERMIT_MAX_DEGRADED_POSITIONS; drive_index++)
    {
        if (kermit_get_and_check_degraded_position(rg_config_p,
                                                   error_case_p,
                                                   drive_index,
                                                   &degraded_position) == FBE_TRUE)
        {
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_raid_group_get_info(object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            while (raid_group_info.rebuild_checkpoint[degraded_position] == FBE_LBA_INVALID)
            {
                if (wait_msecs > max_wait_msecs)
                {
                    MUT_FAIL_MSG("failed waiting for raid groups to go degraded");
                }
                fbe_api_sleep(200);
                wait_msecs += 200;
                status = fbe_api_raid_group_get_info(object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
    }
    return FBE_STATUS_OK;
}
static fbe_status_t kermit_clear_drive_error_counts(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_kermit_error_case_t *error_case_p,
                                                    fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_physical_drive_control_get_service_time_t get_service_time;

    for ( index = 0; index < raid_group_count; index++)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
            fbe_object_id_t object_id;
            fbe_u32_t drive_index;
            for ( drive_index = 0; drive_index < error_case_p->num_errors; drive_index++)
            {
                fbe_u32_t position = error_case_p->errors[drive_index].rg_position_to_inject;
                kermit_get_drive_position(rg_config_p, &position);
                kermit_get_pdo_object_id(rg_config_p, error_case_p, &object_id, drive_index);
                status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if (error_case_p->b_set_rg_expiration_time)
                {
                    /* We changed the service time.  Set it back to default value.
                     */
                    status = fbe_api_physical_drive_get_service_time(object_id, &get_service_time);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    status = fbe_api_physical_drive_set_service_time(object_id, get_service_time.default_service_time_ms);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                }
            }
        }
        rg_config_p++;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t kermit_inject_scsi_error (fbe_protocol_error_injection_error_record_t *record_in_p,
                                              fbe_protocol_error_injection_record_handle_t *record_handle_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    
	/*set the error using NEIT package. */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: starting error injection SK:%d, ASC:%d.", 
               __FUNCTION__, 
               record_in_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key, 
               record_in_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code);

	status = fbe_api_protocol_error_injection_add_record(record_in_p, record_handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
static fbe_status_t kermit_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                              fbe_u32_t *position_p)
{
    if ((*position_p != FBE_U32_MAX)                &&
        (*position_p >= raid_group_config_p->width)    )
    {
        /* User selected beyond the raid group width.
         * We will inject at a relative position from the end of the raid group. 
         * So FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1 is the last drive, 
         *    FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2 is the second to last drive, etc. 
         */
        *position_p = (raid_group_config_p->width - 1) - (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1 - *position_p);
    }
    if ((*position_p != FBE_U32_MAX) && (*position_p >= raid_group_config_p->width))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: position %d > width %d", __FUNCTION__, *position_p, raid_group_config_p->width);
        MUT_FAIL_MSG("position is unexpected");
    }
    return FBE_STATUS_OK;
}
static fbe_bool_t kermit_get_and_check_degraded_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                         fbe_kermit_error_case_t *error_case_p,
                                                         fbe_u32_t  degraded_drive_index,
                                                         fbe_u32_t *degraded_position_p)
{
    fbe_bool_t  b_position_degraded = FBE_FALSE;
    fbe_u32_t   max_num_degraded_positions = 0;

    /* First get the degraded position value from the error table
     */
    MUT_ASSERT_TRUE(degraded_drive_index < KERMIT_MAX_DEGRADED_POSITIONS);
    *degraded_position_p = error_case_p->degraded_positions[degraded_drive_index];

    /* The purpose of this method is to prevent a bad entry in the error case.
     */
    switch(raid_group_config_p->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID1:
            max_num_degraded_positions = raid_group_config_p->width - 1;
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            max_num_degraded_positions = raid_group_config_p->width / 2;
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
        case FBE_RAID_GROUP_TYPE_RAID5:
            max_num_degraded_positions = 1;
            break;   
        case FBE_RAID_GROUP_TYPE_RAID0:
            max_num_degraded_positions = 0;
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            max_num_degraded_positions = 2;
            break; 
        default:
            break;
    }
    if ((degraded_drive_index < max_num_degraded_positions)                     &&
        (error_case_p->degraded_positions[degraded_drive_index] != FBE_U32_MAX)    )
    {
        /* Now scale the position to the width of the raid group
         */
        kermit_get_drive_position(raid_group_config_p, degraded_position_p);
        b_position_degraded = FBE_TRUE;
    }
    return(b_position_degraded);
}
static fbe_status_t kermit_get_port_enclosure_slot(fbe_test_rg_configuration_t *raid_group_config_p,
                                                   fbe_kermit_error_case_t *error_case_p,
                                                   fbe_u32_t error_number,
                                                   fbe_u32_t *port_number_p,
                                                   fbe_u32_t *encl_number_p,
                                                   fbe_u32_t *slot_number_p )
{
    fbe_u32_t position_to_inject = error_case_p->errors[error_number].rg_position_to_inject;
    kermit_get_drive_position(raid_group_config_p, &position_to_inject);
    *port_number_p = raid_group_config_p->rg_disk_set[position_to_inject].bus;
    *encl_number_p = raid_group_config_p->rg_disk_set[position_to_inject].enclosure;
    *slot_number_p = raid_group_config_p->rg_disk_set[position_to_inject].slot;
    return FBE_STATUS_OK;
}
static fbe_status_t kermit_get_pdo_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                             fbe_kermit_error_case_t *error_case_p,
                                             fbe_object_id_t *pdo_object_id_p,
                                             fbe_u32_t error_number)
{
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    status = kermit_get_port_enclosure_slot(raid_group_config_p, error_case_p, error_number, /* Error case. */
                                            &port_number, &encl_number, &slot_number);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(port_number, encl_number, slot_number, pdo_object_id_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(*pdo_object_id_p, FBE_OBJECT_ID_INVALID);
    return status;
}

fbe_sata_command_t kermit_get_sata_command(fbe_payload_block_operation_opcode_t opcode)
{
    fbe_sata_command_t sata_command;
    /* Map a block operation to a scsi command that we should inject on.
     */
    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            sata_command = FBE_SATA_READ_FPDMA_QUEUED;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            sata_command = FBE_SATA_WRITE_FPDMA_QUEUED;
            break;
            /* There is no sata write same, it turns into a write at pdo.
             */
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
            sata_command = FBE_SATA_WRITE_FPDMA_QUEUED;
            break;
        default:
            MUT_FAIL_MSG("unknown opcode");
            break;
    };
    return sata_command;
}

static void kermit_get_sata_error(fbe_protocol_error_injection_sata_error_t *sata_error_p,
                                  fbe_test_protocol_error_type_t error_type)
{
    fbe_port_request_status_t                   port_status;
    /* Map an error enum into a specific error.
     */
    switch (error_type)
    {
        case FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE:
            port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE:
            port_status = FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE:
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR:
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        case FBE_TEST_PROTOCOL_ERROR_TYPE_SOFT_MEDIA_ERROR:
            port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
            break;
        default:
            MUT_FAIL_MSG("unknown error type");
            break;
    };
    //sata_error_p->response_buffer[0] = 0;
    sata_error_p->port_status = port_status;
    return;
}
static void kermit_fill_error_record(fbe_test_rg_configuration_t *raid_group_config_p,
                                     fbe_kermit_error_case_t *error_case_p,
                                     fbe_u32_t drive_index,
                                     fbe_protocol_error_injection_error_record_t *record_p)
{
    record_p->num_of_times_to_insert = error_case_p->errors[drive_index].times_to_insert_error;

    /* Tag tells the injection service to group together records.  When one is injected, 
     * the count is incremented on all.  To make this unique per raid group we add on the rg id. 
     */
    record_p->tag = error_case_p->errors[drive_index].tag + raid_group_config_p->raid_group_id;
    /* If there is a port error, then inject it.
     */
    if (error_case_p->errors[drive_index].error_type == FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE)
    {
        record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT;
        record_p->protocol_error_injection_error.protocol_error_injection_port_error.scsi_command[0] =
            fbe_test_sep_util_get_scsi_command(error_case_p->errors[drive_index].command);
        record_p->protocol_error_injection_error.protocol_error_injection_port_error.port_status = 
            FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
    }
    else
    {
        record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
        record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
        fbe_test_sep_util_get_scsi_command(error_case_p->errors[drive_index].command);
        fbe_test_sep_util_set_scsi_error_from_error_type(&record_p->protocol_error_injection_error.protocol_error_injection_scsi_error,
                                                         error_case_p->errors[drive_index].error_type);
    }
#if 0
        if (raid_group_config_p->block_size == 512)
        {
            record_p->protocol_error_injection_error_type = FBE_TEST_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS;
            record_p->protocol_error_injection_error.protocol_error_injection_sata_error.sata_command = 
                kermit_get_sata_command(error_case_p->errors[drive_index].command);
            kermit_get_sata_error(&record_p->protocol_error_injection_error.protocol_error_injection_sata_error,
                                  error_case_p->errors[drive_index].error_type);
        }
#endif
    return;
}

static void kermit_disable_provision_drive_background_operations(fbe_u32_t bus,
                                                                 fbe_u32_t enclosure,
                                                                 fbe_u32_t slot)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_bool_t      b_is_background_zeroing_enabled = FBE_TRUE;
    fbe_bool_t      b_is_background_sniff_enabled = FBE_TRUE;

    status = fbe_api_provision_drive_get_obj_id_by_location(bus, 
                                                            enclosure,
                                                            slot,
                                                            &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* First determine the setting of provision drive background service operations.*/
    if (kermit_provision_drive_default_operations.b_is_initialized == FBE_FALSE)
    {
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_background_zeroing_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_background_zeroing_enabled == FBE_TRUE)
        {
            kermit_provision_drive_default_operations.b_is_background_zeroing_enabled = FBE_TRUE;
        }
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_background_sniff_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_background_sniff_enabled == FBE_TRUE)
        {
            kermit_provision_drive_default_operations.b_is_sniff_verify_enabled = FBE_TRUE;
        }
        FBE_ASSERT_AT_COMPILE_TIME((FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING | FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF) ==
                                        FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
        kermit_provision_drive_default_operations.b_is_initialized = FBE_TRUE;
    }
    else
    {
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_background_zeroing_enabled);
        MUT_ASSERT_INT_EQUAL(kermit_provision_drive_default_operations.b_is_background_zeroing_enabled, b_is_background_zeroing_enabled);
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_background_sniff_enabled);
        MUT_ASSERT_INT_EQUAL(kermit_provision_drive_default_operations.b_is_sniff_verify_enabled, b_is_background_sniff_enabled);
    }

    /* Now disable all the provision drive background services (setting the op disables that operation).*/
    if ((b_is_background_zeroing_enabled == FBE_TRUE) ||
        (b_is_background_sniff_enabled == FBE_TRUE)      )
    {
        status = fbe_api_base_config_disable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Since the background operation could have been in progress, we sleep
         * to let any in-progress background operations complete (so that they
         * don't encounter the error).
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s - pvd object: 0x%x (%d) b/e/s:%d/%d/%d sleeping %d seconds", 
                   __FUNCTION__, object_id, object_id, bus, enclosure, slot, (3000 / 1000));
        fbe_api_sleep(3000);
    }

    return;
}

static void kermit_enable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_kermit_error_case_t *error_case_p,
                                          fbe_protocol_error_injection_record_handle_t *handle_array_p)
{
    fbe_protocol_error_injection_error_record_t     record;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;
    fbe_u32_t error_index;
    fbe_api_raid_group_get_info_t info;
    fbe_object_id_t rg_object_id;


    /* For RAID-10 we will inject the error into the associated mirror
     * raid group.  Therefore don't get the raid group info from the striper.
     */
    if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)
    {
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    for ( error_index = 0; error_index < error_case_p->num_errors; error_index++)
    {
        /* For RAID-10 we will inject the error into the associated mirror
         * raid group.  Therefore get the info based on the drive position in
         * the striper raid group and then translate to the associated position
         * in the mirror raid group.
         */
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_u32_t in_position_to_inject = error_case_p->errors[error_index].rg_position_to_inject;
            fbe_u32_t out_position_to_inject;

            sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(rg_config_p,
                                                           in_position_to_inject, 
                                                           &rg_object_id, 
                                                           &out_position_to_inject);

            status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
            //error_case_p->errors[error_index].rg_position_to_inject = out_position_to_inject;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Initialize the error injection record.
         */
        fbe_test_neit_utils_init_error_injection_record(&record);

        status = kermit_get_port_enclosure_slot(rg_config_p, error_case_p, error_index,/* Error case. */
                                                &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Put the pdo object id into the record.
         */
        kermit_get_pdo_object_id(rg_config_p, error_case_p, &record.object_id, error_index);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_case_p->errors[error_index].location == KERMIT_ERROR_LOCATION_RAID)
        {
            /* Inject on both user and metadata.
             */
            record.lba_start  = 0x0;
            record.lba_end    = (info.raid_capacity / info.num_data_disk) - 1;
        }
        else if (error_case_p->errors[error_index].location == KERMIT_ERROR_LOCATION_USER_DATA)
        {
            /* For now we just inject the error over a chunk.
             */
            record.lba_start  = 0x0;
            record.lba_end    = FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
        }
        else if (error_case_p->errors[error_index].location == KERMIT_ERROR_LOCATION_USER_DATA_CHUNK_1)
        {
            /* For now we just inject the error over a chunk.
             * The chunk we inject on depends on the error index.
             */
            record.lba_start  = FBE_RAID_DEFAULT_CHUNK_SIZE;
            record.lba_end    = record.lba_start + FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
        }
        else if (error_case_p->errors[error_index].location == KERMIT_ERROR_LOCATION_PAGED_METADATA)
        {
            /* Inject from the start of the paged metadata to the end of the paged metadata.
             */
            record.lba_start  = info.paged_metadata_start_lba / info.num_data_disk;
            record.lba_end    = (info.raid_capacity / info.num_data_disk) - 1;
        }
        else if (error_case_p->errors[error_index].location == KERMIT_ERROR_LOCATION_WRITE_LOG)
        {
            /* Inject just for the signature.
             */
            record.lba_start  = info.write_log_start_pba;
            record.lba_end    = info.write_log_start_pba + info.write_log_physical_capacity - 1;
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Error index %d location %d is invalid",
                       error_index, error_case_p->errors[error_index].location);
            MUT_FAIL_MSG("Error location is invalid");
        }

        if (error_case_p->b_set_rg_expiration_time)
        {
            #define KERMIT_PDO_SERVICE_TIME_MS 200
            status = fbe_api_physical_drive_set_service_time(record.object_id, KERMIT_PDO_SERVICE_TIME_MS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        kermit_fill_error_record(rg_config_p, error_case_p, error_index, &record);
        record.lba_start += KERMIT_BASE_OFFSET;
        record.lba_end += KERMIT_BASE_OFFSET;
        if (error_case_p->b_inc_for_all_drives)
        {
            record.b_inc_error_on_all_pos  = FBE_TRUE;
        }

        /* If the error is not lba specific, disable provision drive background services.*/
        if (error_case_p->errors[error_index].error_type == FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE)
        {
            MUT_ASSERT_INT_EQUAL(1, error_case_p->num_errors);
            kermit_disable_provision_drive_background_operations(port_number, encl_number, slot_number);
        }

        status = kermit_inject_scsi_error(&record, &handle_array_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s rg: 0x%x pos: %d pdo object: %d (0x%x) b/e/s:%d/%d/%d slba:0x%llx elba:0x%llx", 
                   __FUNCTION__, rg_object_id, error_case_p->errors[error_index].rg_position_to_inject, 
                   record.object_id, record.object_id, port_number, encl_number, slot_number,
                   (unsigned long long)record.lba_start,
		   (unsigned long long)record.lba_end);
    }
    return;
}
static void kermit_enable_provision_drive_background_operations(fbe_u32_t bus,
                                                                fbe_u32_t enclosure,
                                                                fbe_u32_t slot)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;

    status = fbe_api_provision_drive_get_obj_id_by_location(bus, 
                                                            enclosure,
                                                            slot,
                                                            &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_INT_EQUAL(kermit_provision_drive_default_operations.b_is_initialized, FBE_TRUE);

    /* Enable those provision drive operations that were enabled by default.*/
    if (kermit_provision_drive_default_operations.b_is_background_zeroing_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    if (kermit_provision_drive_default_operations.b_is_sniff_verify_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
static void kermit_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_kermit_error_case_t *error_case_p,
                                           fbe_protocol_error_injection_record_handle_t *handle_p)
{
    fbe_u32_t error_index;
    fbe_object_id_t pdo_object_id;
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    /* First remove all the records then remove the object */
    for ( error_index = 0; error_index < error_case_p->num_errors; error_index++)
    {
        status = fbe_api_protocol_error_injection_remove_record(handle_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = kermit_get_port_enclosure_slot(rg_config_p, error_case_p, error_index,/* Error case. */
                                                &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_case_p->errors[error_index].error_type == FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE)
        {
            MUT_ASSERT_INT_EQUAL(1, error_case_p->num_errors);
            kermit_enable_provision_drive_background_operations(port_number, encl_number, slot_number);
        }
    }

    status = kermit_get_pdo_object_id(rg_config_p, error_case_p, &pdo_object_id, 1    /* error number */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_remove_object(pdo_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*!**************************************************************
 * kermit_verify_pvd()
 ****************************************************************
 * @brief
 *  Reattach the pvd to the pdo.  We also validate the pdo and pdo
 *  are in the appropriate states.
 *
 * @param disk_to_insert_p - the drive info to insert for.
 * @param pvd_object_id - Need this to do the attach.               
 *
 * @return None. 
 * 
 ****************************************************************/

void kermit_verify_pvd(fbe_test_raid_group_disk_set_t *drive_to_insert_p)
{
    fbe_status_t    status;
        
    /* Verify the PDO and LDO are in the READY state
     */
    status = fbe_test_pp_util_verify_pdo_state(drive_to_insert_p->bus, 
                                               drive_to_insert_p->enclosure,
                                               drive_to_insert_p->slot,
                                               FBE_LIFECYCLE_STATE_READY,
                                               KERMIT_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

#if 0
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus, 
                                                            drive_to_insert_p->enclosure,
                                                            drive_to_insert_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* wait until the PVD object transition to READY state
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s wait for pvd: 0x%x to bus: %d encl: %d slot: %d to become ready", 
               __FUNCTION__, pvd_id, 
               drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
#endif
    return;
}
/******************************************
 * end kermit_verify_pvd()
 ******************************************/
void kermit_enable_trace_flags(fbe_object_id_t object_id)

{
    fbe_status_t status;
    fbe_api_trace_level_control_t control;
    control.fbe_id = object_id;
    control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
    control.trace_flag = FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE;
    status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);
    mut_printf(MUT_LOG_TEST_STATUS, "%s enabled lifecyclee trace flags for object: %d", 
               __FUNCTION__, object_id);
    return;
}
void kermit_disable_trace_flags(fbe_object_id_t object_id)

{
    fbe_status_t status;
    fbe_api_trace_level_control_t control;
    control.fbe_id = object_id;
    control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
    control.trace_flag = 0;
    status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);
    mut_printf(MUT_LOG_TEST_STATUS, "%s disabled lifecycle trace flags for object: %d", 
               __FUNCTION__, object_id);

    return;
}

void kermit_reset_drive_state(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_kermit_error_case_t *error_case_p, 
                              fbe_u32_t drive_index)
{
    fbe_status_t status;
    fbe_object_id_t pdo_object_id;
    fbe_object_id_t pvd_object_id;

    fbe_u32_t position = error_case_p->errors[drive_index].rg_position_to_inject;


    status = kermit_get_pdo_object_id(rg_config_p, error_case_p, &pdo_object_id, drive_index /* error number */);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    kermit_get_drive_position(rg_config_p, &position);
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                            rg_config_p->rg_disk_set[position].enclosure, 
                                                            rg_config_p->rg_disk_set[position].slot, 
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Pull drive: %d_%d_%d ", __FUNCTION__,rg_config_p->rg_disk_set[position].bus,rg_config_p->rg_disk_set[position].enclosure,
                                                                             rg_config_p->rg_disk_set[position].slot );
    status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position].bus,
                                         rg_config_p->rg_disk_set[position].enclosure,
                                         rg_config_p->rg_disk_set[position].slot,                                   
                                         &rg_config_p->drive_handle[position]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for PDO: 0x%x to be destroyed", __FUNCTION__, pdo_object_id);
    status = fbe_api_wait_till_object_is_destroyed(pdo_object_id,FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Re-insert drive: %d_%d_%d ", __FUNCTION__,rg_config_p->rg_disk_set[position].bus,rg_config_p->rg_disk_set[position].enclosure,
                                                                             rg_config_p->rg_disk_set[position].slot );
    status = fbe_test_pp_util_reinsert_drive(rg_config_p->rg_disk_set[position].bus,
                                             rg_config_p->rg_disk_set[position].enclosure,
                                             rg_config_p->rg_disk_set[position].slot,                                   
                                             rg_config_p->drive_handle[position]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear drive fault */
    status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    kermit_verify_pvd(&rg_config_p->rg_disk_set[position]);
    return;
}

void kermit_reinsert_drives(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_kermit_error_case_t *error_case_p)
{
    fbe_u32_t drive_index;
    for ( drive_index = 0; drive_index < error_case_p->num_errors; drive_index++)
    {
        kermit_reset_drive_state(rg_config_p, error_case_p, drive_index);
    }
}

void kermit_degrade_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_kermit_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t pdo_object_id;

    for ( index = 0; index < KERMIT_MAX_DEGRADED_POSITIONS; index++)
    {
        fbe_u32_t degraded_position;

        if (kermit_get_and_check_degraded_position(rg_config_p,
                                                   error_case_p,
                                                   index,
                                                   &degraded_position) == FBE_TRUE)
        {
            fbe_test_raid_group_disk_set_t *disk_set_p = &rg_config_p->rg_disk_set[degraded_position];
            /* Transition pdo to failed.  Wait for it to go failed.
             */
            status = fbe_api_get_physical_drive_object_id_by_location(disk_set_p->bus,
                                                                      disk_set_p->enclosure,
                                                                      disk_set_p->slot,
                                                                      &pdo_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);
            mut_printf(MUT_LOG_TEST_STATUS, "%s degrade position %d pdo %d 0x%x",
                       __FUNCTION__, degraded_position, pdo_object_id, pdo_object_id);

            status = fbe_api_physical_drive_update_link_fault(pdo_object_id, FBE_TRUE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                             KERMIT_WAIT_MSEC, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    return;
}
void kermit_restore_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_kermit_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t pdo_object_id;
    fbe_api_terminator_device_handle_t drive_handle;

    for ( index = 0; index < KERMIT_MAX_DEGRADED_POSITIONS; index++)
    {
        fbe_u32_t degraded_position;

        if (kermit_get_and_check_degraded_position(rg_config_p,
                                                   error_case_p,
                                                   index,
                                                   &degraded_position) == FBE_TRUE)
        {
            fbe_test_raid_group_disk_set_t *disk_set_p = &rg_config_p->rg_disk_set[degraded_position];
            /* Transition pdo to activate.  Wait for it to go ready.
             */
            status = fbe_api_get_physical_drive_object_id_by_location(disk_set_p->bus,
                                                                      disk_set_p->enclosure,
                                                                      disk_set_p->slot,
                                                                      &pdo_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(pdo_object_id, FBE_OBJECT_ID_INVALID);
  
            mut_printf(MUT_LOG_TEST_STATUS, "%s Pull drive: %d_%d_%d ", __FUNCTION__,disk_set_p->bus,disk_set_p->enclosure,disk_set_p->slot);
            status = fbe_test_pp_util_pull_drive(disk_set_p->bus,
                                                 disk_set_p->enclosure,
                                                 disk_set_p->slot,                                  
                                                 &drive_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "%s Wait for PDO: 0x%x to be destroyed", __FUNCTION__, pdo_object_id);
            status = fbe_api_wait_till_object_is_destroyed(pdo_object_id,FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            mut_printf(MUT_LOG_TEST_STATUS, "%s re-insert drive: %d_%d_%d ", __FUNCTION__,disk_set_p->bus,disk_set_p->enclosure,disk_set_p->slot);      
            status = fbe_test_pp_util_reinsert_drive(disk_set_p->bus,
                                                     disk_set_p->enclosure,
                                                     disk_set_p->slot,                          
                                                     drive_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            kermit_verify_pvd(disk_set_p);
        }
    }
    return;
}
void kermit_validate_io_error_counts(fbe_raid_verify_error_counts_t *io_error_counts_p)
{
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_coh_count, 0);
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_coh_count, 0);

    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_crc_multi_count, 0);
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_crc_multi_count, 0);

    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_crc_single_count, 0);
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_crc_single_count, 0);

    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_soft_media_count, 0);

    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_ss_count, 0);
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_ss_count, 0);

    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_ts_count, 0);
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_ts_count, 0);

    MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_ws_count, 0);
    MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_ws_count, 0);
    return;
}
fbe_bool_t kermit_does_count_match(fbe_u32_t expected_count,
                                   fbe_u32_t actual_count,
                                   fbe_u32_t element_size )
{
    if (expected_count == KERMIT_ERR_COUNT_MATCH)
    {
        /* If FBE_U32_MAX then it is always a match.
         */
        return FBE_TRUE;
    }
    /* If the expected count is KERMIT_ERR_COUNT_MATCH_ELSZ then we match the errors to element size.
     */
    if (expected_count == KERMIT_ERR_COUNT_MATCH_ELSZ)
    {
        if (actual_count != element_size)
        {
            return FBE_FALSE;
        }
        else
        {
            return FBE_TRUE;
        }
    }
    else if (expected_count == KERMIT_ERR_COUNT_NON_ZERO)
    {
        /* We expect a non-zero count.
         */
        if (actual_count == 0)
        {
            return FBE_FALSE;
        }
        else
        {
            return FBE_TRUE;
        }
    }
    /* Othewise we just match expected to actual.
     */
    else if (expected_count != actual_count)
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}

/* This function is created since even though the drive dies metadata service will
   return IO retryable status that can cause the RAID to increment the retryable counter
   and kermit will fail infrequently where retryable error will be one greater than expected.
   Also, the actual errors are based on DIEH algorithm, which has many factors such as
   fading and burst reduction.   It's very difficult to predict the exact number of errors. 
   In this case, a small range will be used to determine if error counts match.
*/
fbe_bool_t kermit_does_retry_count_match(fbe_u32_t expected_count,
                                   fbe_u32_t actual_count,
                                   fbe_u32_t element_size )
{
    if (expected_count == KERMIT_ERR_COUNT_MATCH)
    {
        /* If FBE_U32_MAX then it is always a match.
         */
        return FBE_TRUE;
    }
    /* If the expected count is KERMIT_ERR_COUNT_MATCH_ELSZ then we match the errors to element size.
     */
    if (expected_count == KERMIT_ERR_COUNT_MATCH_ELSZ)
    {
        if (actual_count != element_size)
        {
            return FBE_FALSE;
        }
        else
        {
            return FBE_TRUE;
        }
    }
    else if (expected_count == KERMIT_ERR_COUNT_NON_ZERO)
    {
        /* We expect a non-zero count.
         */
        if (actual_count == 0)
        {
            return FBE_FALSE;
        }
        else
        {
            return FBE_TRUE;
        }
    }
    /* Handle range check if special flag is set for expect value.  This handles the cases
       where number of DIEH retry errors can't be predicted.
     */
    else if (expected_count == KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE)
    {
        if ((actual_count >= KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE - 1) &&  
            (actual_count <= KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE + 1))
        {
            return FBE_TRUE;
        }
        else
        {
            return FBE_FALSE;
        }
    }
    /* Handle range check if special flag is set for expect value.  This handles the cases
       where number of DIEH retry errors can't be predicted.
     */
    else if (expected_count == KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE)
    {
        if ((actual_count >= (KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2 - 2)) &&  
            (actual_count <= (KERMIT_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2 + 2)))
        {
            return FBE_TRUE;
        }
        else
        {
            return FBE_FALSE;
        }
    }    
    /* Test if we match the expected with actual + 1 for metadata service error
     */
    else if ((expected_count != actual_count) &&
             ((expected_count+1) != actual_count))
    {
        return FBE_FALSE;
    }


    return FBE_TRUE;
}

void kermit_validate_io_verify_report_counts(fbe_kermit_error_case_t * error_case_p, 
                                             fbe_lun_verify_report_t *verify_report_p,
                                             fbe_u32_t element_size)
{
    /* Handle more than one error case and return a status.
     */
    fbe_u32_t index;
    fbe_bool_t b_match;
    for ( index = 0; index < error_case_p->max_error_cases; index++)
    {
        b_match = FBE_TRUE;

        /* If either count mismatches we set the boolean.
         */
        if (!kermit_does_count_match(error_case_p->correctable_time_stamp[index],
                                     verify_report_p->totals.correctable_time_stamp,
                                     element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!kermit_does_count_match(error_case_p->correctable_write_stamp[index],
                                     verify_report_p->totals.correctable_write_stamp,
                                     element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!kermit_does_count_match(error_case_p->uncorrectable_time_stamp[index],
                                     verify_report_p->totals.uncorrectable_time_stamp,
                                     element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!kermit_does_count_match(error_case_p->uncorrectable_write_stamp[index],
                                     verify_report_p->totals.uncorrectable_write_stamp,
                                     element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!kermit_does_count_match(error_case_p->correctable_coherency[index],
                                     verify_report_p->totals.correctable_coherency,
                                     element_size))
        {
            b_match = FBE_FALSE;
        }

        /* Both counts matched.
         */
        if (b_match)
        {
            break;
        }
    }
    /* Fail immediately if none of the above matched.
     */
    if (!b_match)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "no io vr match c_ts: 0x%x u_ts: 0x%x c_ws: 0x%x u_ws: 0x%x c_coh: 0x%x",
                   verify_report_p->totals.correctable_time_stamp, verify_report_p->totals.uncorrectable_time_stamp,
                   verify_report_p->totals.correctable_write_stamp, verify_report_p->totals.uncorrectable_write_stamp,
                   verify_report_p->totals.correctable_coherency);
        MUT_FAIL_MSG("no error match found");
    }
}
void kermit_validate_verify_report(fbe_kermit_error_case_t *error_case_p,
                                   fbe_lun_verify_report_t *verify_report_p,
                                   fbe_u32_t element_size)
{
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.correctable_lba_stamp, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.correctable_media, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.correctable_multi_bit_crc, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.correctable_shed_stamp, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.correctable_single_bit_crc, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.correctable_soft_media, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.uncorrectable_coherency, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.uncorrectable_lba_stamp, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.uncorrectable_media, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.uncorrectable_multi_bit_crc, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.uncorrectable_shed_stamp, 0);
    MUT_ASSERT_INT_EQUAL(verify_report_p->totals.uncorrectable_single_bit_crc, 0);

    /* Validate specific counts for certain errors.
     */
    kermit_validate_io_verify_report_counts(error_case_p, verify_report_p, element_size);
    return;
}
void kermit_validate_io_status_for_error_case(fbe_kermit_error_case_t *error_case_p, 
                                              fbe_api_rdgen_context_t *context_p)
{
    /* Handle more than one error case and return a status.
     */
    fbe_u32_t index;
    fbe_bool_t b_match = FBE_FALSE;

    for ( index = 0; index < KERMIT_MAX_POSSIBLE_STATUS; index++)
    {
        if ((error_case_p->block_status[index] != KERMIT_INVALID_STATUS) &&
            (error_case_p->block_status[index] == context_p->start_io.status.block_status) &&
            (error_case_p->block_qualifier[index] == context_p->start_io.status.block_qualifier) )
        {
            /* It may be that we have two possible status values, one with erorr and the other 
             * without error.  So in this case since we only have one error count, we need to 
             * allow the error count for the errored case. 
             * In this case the error count in the error case is 0, and the exception here is to check 
             * for the error. 
             */
            if ((error_case_p->expected_io_error_count == 0) && 
                (error_case_p->block_status[index] != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
            {
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, error_case_p->expected_io_error_count);
            }
            b_match = FBE_TRUE;
            break;
        }
    }

    /* Fail immediately if none of the above matched.
     */
    if (!b_match)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "no status match - bl_status: 0x%x bl_qual: 0x%x err_count: 0x%x",
                   context_p->start_io.status.block_status,
                   context_p->start_io.status.block_qualifier,
                   context_p->start_io.statistics.error_count);
        for ( index = 0; index < KERMIT_MAX_POSSIBLE_STATUS; index++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "expected case %d - bl_status: 0x%x bl_qual: 0x%x",
                       index,
                       error_case_p->block_status[index],
                       error_case_p->block_qualifier[index]);
        }
        MUT_FAIL_MSG("no io status match found");
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "status match - bl_status: 0x%x bl_qual: 0x%x err_count: 0x%x",
                   context_p->start_io.status.block_status,
                   context_p->start_io.status.block_qualifier,
                   context_p->start_io.statistics.error_count);
    }
}
void kermit_validate_io_error_counts_for_error_case(fbe_kermit_error_case_t * error_case_p, 
                                                    fbe_raid_verify_error_counts_t * io_error_counts_p,
                                                    fbe_u32_t element_size)
{
    /* Handle more than one error case and return a status.
     */
    fbe_u32_t index;
    fbe_bool_t b_match;
    for ( index = 0; index < error_case_p->max_error_cases; index++)
    {
        b_match = FBE_TRUE;
        /* If either count mismatches we set the boolean.
         */
        /* Note retryable error uses a different function for match. Look
           at the function for the explanation
        */
        if (!kermit_does_retry_count_match(error_case_p->retryable_error_count[index],
                                    io_error_counts_p->retryable_errors, element_size))
        {
            b_match = FBE_FALSE;
        }

        /* If either count mismatches we set the boolean.
         */
        if (!kermit_does_retry_count_match(error_case_p->non_retryable_error_count[index],
                                           io_error_counts_p->non_retryable_errors, element_size))
        {
            b_match = FBE_FALSE;
        }

        if ((error_case_p->shutdown_error_count[index] != FBE_U32_MAX) &&
            (error_case_p->shutdown_error_count[index] != io_error_counts_p->shutdown_errors))
        {
            b_match = FBE_FALSE;
        }

        if ((error_case_p->correctable_media_error_count[index] != FBE_U32_MAX) &&
            (error_case_p->correctable_media_error_count[index] != io_error_counts_p->c_media_count))
        {
            b_match = FBE_FALSE;
        }

        if ((error_case_p->uncorrectable_media_error_count[index] != FBE_U32_MAX) &&
            (error_case_p->uncorrectable_media_error_count[index] != io_error_counts_p->u_media_count))
        {
            b_match = FBE_FALSE;
        }

        if ((error_case_p->correctable_crc_count[index] != FBE_U32_MAX) &&
            (error_case_p->correctable_crc_count[index] != io_error_counts_p->c_crc_count))
        {
            b_match = FBE_FALSE;
        }

        if ((error_case_p->uncorrectable_crc_count[index] != FBE_U32_MAX) &&
            (error_case_p->uncorrectable_crc_count[index] != io_error_counts_p->u_crc_count))
        {
            b_match = FBE_FALSE;
        }

        /* Both counts matched.
         */
        if (b_match)
        {
            break;
        }
    }
    /* Fail immediately if none of the above matched.
     */
    if (!b_match)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "no io err match rtry: 0x%x nretry: 0x%x shtdn: 0x%x c_md: 0x%x u_md: 0x%x",
                   io_error_counts_p->retryable_errors, io_error_counts_p->non_retryable_errors,
                   io_error_counts_p->shutdown_errors,
                   io_error_counts_p->c_media_count, io_error_counts_p->u_media_count);
        MUT_FAIL_MSG("no error match found");
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "io err match found rtry: 0x%x nretry: 0x%x shtdn: 0x%x c_md: 0x%x u_md: 0x%x",
                   io_error_counts_p->retryable_errors, io_error_counts_p->non_retryable_errors,
                   io_error_counts_p->shutdown_errors, io_error_counts_p->c_media_count, io_error_counts_p->u_media_count);
    }
}
fbe_status_t kermit_validate_error_counts(fbe_kermit_error_case_t *error_case_p,
                                          fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_api_rdgen_context_t *context_p,
                                           fbe_lun_verify_report_t *verify_report_p)
{
    fbe_raid_verify_error_counts_t *io_error_counts_p = &context_p->start_io.statistics.verify_errors;
    fbe_u32_t element_size;

    /* Validate the I/O status for this error case.  For the majority of 
     * cases there is exactly (1) possible status
     */
    if (0 && error_case_p->block_status[1] == KERMIT_INVALID_STATUS)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, error_case_p->expected_io_error_count);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, error_case_p->block_status[0]);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, error_case_p->block_qualifier[0]);
    }
    else
    {
        /* Else we need to check against all possible status
         */
        kermit_validate_io_status_for_error_case(error_case_p, context_p); 
    }

    /* Make sure the io error counts make sense
     */
    kermit_validate_io_error_counts(io_error_counts_p);

    element_size = fbe_test_sep_util_get_element_size(rg_config_p);

    /* Validate the verify counts attached to the io.  To make debugging easier 
     * we will simply handle the case with 1 set of error cases first. 
     */
    kermit_validate_io_error_counts_for_error_case(error_case_p, io_error_counts_p, element_size);
    kermit_validate_verify_report(error_case_p, verify_report_p, element_size);

    return FBE_STATUS_OK;
}
void kermit_enable_trace_flags_for_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t drive_index;
    fbe_object_id_t object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_raid_group_disk_set_t *disk_p = NULL;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kermit_enable_trace_flags(rg_object_id);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kermit_enable_trace_flags(object_id);

    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_p->bus, 
                                                                disk_p->enclosure,
                                                                disk_p->slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        kermit_enable_trace_flags(object_id);
    }
    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_index, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        kermit_enable_trace_flags(object_id);
    }
    return;
}
void kermit_disable_trace_flags_for_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t drive_index;
    fbe_object_id_t object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_raid_group_disk_set_t *disk_p = NULL;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kermit_enable_trace_flags(rg_object_id);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    kermit_enable_trace_flags(object_id);

    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_p->bus, 
                                                                disk_p->enclosure,
                                                                disk_p->slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        kermit_enable_trace_flags(object_id);
    }
    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_index, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        kermit_enable_trace_flags(object_id);
    }
    return;
}

void kermit_setup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_kermit_error_case_t *error_case_p,
                            fbe_api_rdgen_context_t *context_p,
                            fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                            fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_rdgen_operation_t seed_operation;
    fbe_raid_library_debug_flags_t current_debug_flags;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    /* Seed the area with the pattern we expect for read or with a different pattern than
     * the one we are writing/zeroing. 
     */
    if ((error_case_p->operation == FBE_RDGEN_OPERATION_READ_CHECK) ||
        (error_case_p->operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK))
    {
        seed_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    }
    else if (error_case_p->operation == FBE_RDGEN_OPERATION_WRITE_READ_CHECK)
    {
        seed_operation = FBE_RDGEN_OPERATION_ZERO_ONLY;
    }
    else
    {
        MUT_FAIL_MSG("Unexpected operation");
    }
    kermit_send_seed_io(rg_config_p, error_case_p, context_p, seed_operation);

    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Setup the notification before we inject the error since the 
         * notification will happen as soon as we inject the error. 
         */
        fbe_test_common_util_register_lifecycle_state_notification(ns_context_p, FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                   FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP,
                                                                   rg_object_id,
                                                                   error_case_p->wait_for_state_mask);
    }

    /* Degrade raid group if needed.
     */
    kermit_degrade_raid_groups(rg_config_p, error_case_p);

    /* Wait for the raid group to know it is degraded.
     */
    kermit_wait_for_degraded(rg_config_p, error_case_p, KERMIT_WAIT_MSEC);

    /* Start injecting errors.
     */
    kermit_enable_error_injection(rg_config_p, error_case_p, error_handles_p);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Clear the verify report.
     */
    status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, &current_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add the existing flags to the siots tracing flags.
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING | 
                                                         current_debug_flags));

    /*@!todo
     * Add the following library and raid group debug flags temporarily to debug the current sporadic 
     * kermit failures. The following debug flags need to remove at some time once all kermit tests are stable. 
     * 04/09/2012 
     */
    /* set library debug flags */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING|
                                                         current_debug_flags));
    /* set raid group debug flags */
    fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                               FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING);
    

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

void kermit_disable_injection(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_kermit_error_case_t *error_case_p,
                              fbe_api_rdgen_context_t *context_p,
                              fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                              fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    fbe_status_t status;

    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Disable error injection since it might prevent the object from coming ready.
         */
        kermit_disable_error_injection(rg_config_p, error_case_p, error_handles_p);

        if (error_case_p->wait_for_state_mask == FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE)
        {
            /* We are going to activate temporarily, wait for the object to come back ready, 
             * since we want any errors to continue to get injected until the object brings 
             * itself back to ready. 
             */
            fbe_object_id_t rg_object_id;
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             KERMIT_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    else
    {
        /* Disable error injection.
         */
        kermit_disable_error_injection(rg_config_p, error_case_p, error_handles_p);
    }

    kermit_clear_drive_error_counts(rg_config_p, error_case_p, 1);
}
void kermit_wait_for_state(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_kermit_error_case_t *error_case_p,
                              fbe_api_rdgen_context_t *context_p,
                              fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                              fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Wait for the raid group to get to the expected state.  The notification was
         * initialized above. 
         */
        fbe_test_common_util_wait_lifecycle_state_notification(ns_context_p, KERMIT_WAIT_MSEC);
        fbe_test_common_util_unregister_lifecycle_state_notification(ns_context_p);
    }
}
void kermit_cleanup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_kermit_error_case_t *error_case_p,
                              fbe_api_rdgen_context_t *context_p,
                              fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                              fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_lun_verify_report_t verify_report;

    /* Wait for verifies to complete.  This needs to be after restoring the raid group,
     * since if we insert an error that requires verify and then shutdown the rg, 
     * then we must bring the rg back before we can wait for the verify. 
     */
    kermit_wait_for_verify(rg_config_p, error_case_p, KERMIT_WAIT_MSEC);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Now get the verify report for the (1) LUN associated with the raid group
     */
    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate the counts.
     */
    status = kermit_validate_error_counts(error_case_p, rg_config_p, context_p, &verify_report);
    if (status != FBE_STATUS_OK)
    {
        MUT_FAIL_MSG("no match found for error counts\n");
    }

#if 0 /* We can enable this to see PVD lifecycle tracing */
    kermit_disable_trace_flags(rg_config_p->first_object_id + rg_config_p->width);
    kermit_disable_trace_flags(rg_config_p->first_object_id);
    kermit_disable_trace_flags(rg_config_p->first_object_id + (rg_config_p->width * 2));
    kermit_disable_trace_flags(rg_config_p->first_object_id + (rg_config_p->width * 2) + 1);
#endif
    mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     KERMIT_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
fbe_status_t kermit_retry_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_kermit_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[KERMIT_MAX_RG_COUNT][KERMIT_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[KERMIT_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[KERMIT_MAX_RG_COUNT];
    fbe_status_t io_status;
    fbe_object_id_t rg_object_ids[KERMIT_MAX_RG_COUNT];
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (kermit_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            kermit_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                   &ns_context[rg_index], &error_handles[rg_index][0]);
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
                current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                /* Set mirror perfered position to disable mirror read optimization */
                status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_ids[rg_index], 1);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (kermit_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Send I/O.  This is an asynchronous send.
             */
            io_status = kermit_send_io(current_rg_config_p, error_case_p, &context[rg_index], error_case_p->operation,
                                       error_case_p->expiration_msecs, error_case_p->abort_msecs);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, io_status);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (kermit_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Wait for I/O to finish.
             */
            fbe_api_rdgen_wait_for_ios(&context[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&context[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (kermit_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            kermit_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                  &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (kermit_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            kermit_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                     &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            kermit_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            kermit_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (kermit_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            kermit_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                     &ns_context[rg_index], &error_handles[rg_index][0]);
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1 || 
                current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                /* Set mirror perfered position to enable mirror read optimization */
                status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_ids[rg_index], 0);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        current_rg_config_p++;
    }
    return FBE_STATUS_OK;
}
fbe_u32_t kermit_get_table_case_count(fbe_kermit_error_case_t *error_case_p)
{
    fbe_u32_t test_case_index = 0;

    /* Loop over all the cases in this table until we hit a terminator.
     */
    while (error_case_p[test_case_index].blocks != 0)
    {
        test_case_index++;
    }
    return test_case_index;
}
fbe_u32_t kermit_get_test_case_count(fbe_kermit_error_case_t *error_case_p[])
{
    fbe_u32_t test_case_count = 0;
    fbe_u32_t table_index = 0;
    fbe_u32_t test_case_index;

    /* Loop over all the tables until we hit a terminator.
     */
    while (error_case_p[table_index] != NULL)
    {
        test_case_index = 0;
        /* Add the count for this table.
         */
        test_case_count += kermit_get_table_case_count(&error_case_p[table_index][0]);
        table_index++;
    }
    return test_case_count;
}
fbe_u32_t kermit_get_num_error_positions(fbe_kermit_error_case_t *error_case_p)
{
    fbe_u32_t index;
    fbe_u32_t err_count = 0;
    fbe_u32_t bitmask = 0;

    /* Count the number of unique positions in the error array.
     */
    for ( index = 0; index < error_case_p->num_errors; index++)
    {
        fbe_u32_t pos_bitmask = (1 << error_case_p->errors[index].rg_position_to_inject);
        /* Only count it if it is not already set in the bitmask.
         */
        if ((bitmask & pos_bitmask) == 0)
        {
            err_count++;
            bitmask |= (1 << error_case_p->errors[index].rg_position_to_inject);
        }
    }
    return err_count;
}
fbe_bool_t kermit_is_width_valid_for_error_case(fbe_kermit_error_case_t *error_case_p,
                                                fbe_u32_t width)
{
    fbe_u32_t index = 0;
    fbe_u32_t num_err_positions = kermit_get_num_error_positions(error_case_p);

    /* Too many errors for this width.
     */
    if (num_err_positions > width)
    {
        return FBE_FALSE;
    }
    /* If we do not have widths, just return it is valid.
     */
    if (error_case_p->widths[index] == FBE_U32_MAX)
    {
        return FBE_TRUE;
    }
    /* Otherwise we need to search for this width in the array.
     */
    while (error_case_p->widths[index] != FBE_U32_MAX)
    {
        if (error_case_p->widths[index] == width)
        {
            return FBE_TRUE;
        }
        index++;
    }
    return FBE_FALSE;
}
fbe_u32_t kermit_error_case_num_valid_widths(fbe_kermit_error_case_t *error_case_p)
{
    fbe_u32_t index = 0;

    /* If we do not have widths, just return FBE_U32_MAX to say all widths are valid.
     */
    if (error_case_p->widths[index] == FBE_U32_MAX)
    {
        return FBE_U32_MAX;
    }
    /* Otherwise we need to search for widths in the array.
     */
    while (error_case_p->widths[index] != FBE_U32_MAX)
    {
        index++;
    }
    return index;
}

fbe_bool_t kermit_is_rg_valid_for_error_case(fbe_kermit_error_case_t *error_case_p,
                                             fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    if (!kermit_is_width_valid_for_error_case(error_case_p, rg_config_p->width))
    {
        b_status = FBE_FALSE;
    }
    if (!fbe_test_rg_config_is_enabled(rg_config_p))
    {
        b_status = FBE_FALSE;
    }

    /* Only execute bandwidth on level 2 or greater.
     */
    if ((rg_config_p->b_bandwidth) && (extended_testing_level <= 1))
    {
       b_status = FBE_FALSE; 
    }
    return b_status;
}
void kermit_run_test_case_for_all_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_kermit_error_case_t *error_case_p,
                                              fbe_u32_t num_raid_groups,
                                              fbe_u32_t overall_test_case_index,
                                              fbe_u32_t total_test_case_count,
                                              fbe_u32_t table_test_case_index,
                                              fbe_u32_t table_test_case_count)
{
    fbe_status_t status;
    const fbe_char_t *raid_type_string_p = NULL;

    mut_printf(MUT_LOG_TEST_STATUS, "test_case: %s", error_case_p->description_p); 
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    mut_printf(MUT_LOG_TEST_STATUS, "starting table case: (%d of %d) overall: (%d of %d) line: %d for raid type %s (0x%x)", 
               table_test_case_index, table_test_case_count,
               overall_test_case_index, total_test_case_count, 
               error_case_p->line, raid_type_string_p, rg_config_p->raid_type);

    status = kermit_retry_test_case(rg_config_p, error_case_p);
    if (status != FBE_STATUS_OK)
    {
        MUT_FAIL_MSG("test case failed\n");
    }

    /* Only execute this case if this raid group has enough drives.
     */
    if (kermit_get_num_error_positions(error_case_p) <= rg_config_p->width)
    {
        kermit_wait_for_rebuilds(&rg_config_p[0], error_case_p, num_raid_groups);
        kermit_wait_for_verifies(&rg_config_p[0], error_case_p, num_raid_groups);
        kermit_clear_drive_error_counts(&rg_config_p[0], error_case_p, num_raid_groups);
    }
    return;
}
/*!**************************************************************
 * kermit_retry_test()
 ****************************************************************
 * @brief
 *  Run a retry test for all test cases passed in,
 *  and for all raid groups that are part of the input rg_config_p.
 *  
 * @param rg_config_p - Raid group table.
 * @param error_case_p - Table of test cases.
 * @param num_raid_groups - Number of raid groups.
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t kermit_retry_test(fbe_test_rg_configuration_t *rg_config_p,
                               fbe_kermit_error_case_t *error_case_p[],
                               fbe_u32_t num_raid_groups)
{
    fbe_u32_t test_index = 0;
    fbe_u32_t table_test_index;
    fbe_u32_t total_test_case_count = kermit_get_test_case_count(error_case_p);
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t start_table_index = 0;
    fbe_u32_t table_index = 0;
    fbe_u32_t start_test_index = 0;
    fbe_u32_t table_test_count;
    fbe_u32_t max_case_count = FBE_U32_MAX;
    fbe_u32_t test_case_executed_count = 0;
    fbe_u32_t repeat_count = 1;
    fbe_u32_t current_iteration = 0;
    fbe_bool_t b_abort_expiration_only;
    /* The user can optionally choose a table index or test_index to start at.
     * By default we continue from this point to the end. 
     */
    fbe_test_sep_util_get_cmd_option_int("-start_table", &start_table_index);
    fbe_test_sep_util_get_cmd_option_int("-start_index", &start_test_index);
    fbe_test_sep_util_get_cmd_option_int("-total_cases", &max_case_count);
    b_abort_expiration_only = fbe_test_sep_util_get_cmd_option("-abort_cases_only");mut_printf(MUT_LOG_TEST_STATUS, "kermit start_table: %d start_index: %d total_cases: %d abort_only: %d",start_table_index, start_test_index, max_case_count, b_abort_expiration_only);

    fbe_test_sep_util_get_raid_type_string(rg_config_p[0].raid_type, &raid_type_string_p);

    /* Loop over all the tests for this raid type.
     */
    while (error_case_p[table_index] != NULL)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Starting table %d for raid type %s", 
                   __FUNCTION__, table_index, raid_type_string_p);

        /* If we have not yet reached the start index, get the next table.
         */
        if (table_index < start_table_index)
        {
            table_index++;
            continue;
        }
        table_test_index = 0;
        table_test_count = kermit_get_table_case_count(&error_case_p[table_index][0]);

        /* Loop until we hit a terminator.
         */ 
        while (error_case_p[table_index][table_test_index].blocks != 0)
        {
            fbe_time_t start_time = fbe_get_time();
            fbe_time_t time;
            fbe_time_t msec_duration;

            
            if (start_test_index == error_case_p[table_index][table_test_index].line)
            {
                /* Found a match.  We allow the start_test_index to also match
                 *                 a line number in a test case. 
                 */
                start_test_index = 0;
            }
            else if (table_test_index < start_test_index)
            {
                /* If we have not yet reached the start index, get the next test case.
                 */ 
                table_test_index++;
                test_index++;
                continue;
            }
            if (b_abort_expiration_only && 
                (error_case_p[table_index][table_test_index].b_set_rg_expiration_time == FBE_FALSE) &&
                ((error_case_p[table_index][table_test_index].block_status[0] != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
                 (error_case_p[table_index][table_test_index].abort_msecs != 0)))
            {
                table_test_index++;
                test_index++;
                continue;
            }
            /* For certain cases we need to set the expiration time the raid groups put into their packets.
             */
            if (error_case_p[table_index][table_test_index].b_set_rg_expiration_time)
            {
                kermit_set_rg_options();
            }
            current_iteration = 0;
            repeat_count = 1;
            fbe_test_sep_util_get_cmd_option_int("-repeat_case_count", &repeat_count);
            while (current_iteration < repeat_count)
            {
                /* Run this test case for all raid groups.
                 */
                kermit_run_test_case_for_all_raid_groups(rg_config_p, 
                                                         &error_case_p[table_index][table_test_index], 
                                                         num_raid_groups,
                                                         test_index, total_test_case_count,
                                                         table_test_index,
                                                         table_test_count);
                current_iteration++;
                if (repeat_count > 1)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "kermit completed test case iteration [%d of %d]", current_iteration, repeat_count);
                }
            }
            kermit_unset_rg_options();
            time = fbe_get_time();
            msec_duration = (time - start_time);
            mut_printf(MUT_LOG_TEST_STATUS,
                       "table %d test %d took %llu seconds (%llu msec)",
                       table_index, table_test_index,
                       (unsigned long long)(msec_duration / FBE_TIME_MILLISECONDS_PER_SECOND),
                       (unsigned long long)msec_duration);
            table_test_index++;
            test_index++;
            test_case_executed_count++;

            /* If the user chose a limit on the max test cases and we are at the limit, just return. 
             */  
            if (test_case_executed_count >= max_case_count)
            {
                return FBE_STATUS_OK;
            }
        }    /* End loop over all tests in this table. */

        /* After the first test case reset start_test_index, so for all the remaining tables we will 
         * start at the beginning. 
         */
        start_test_index = 0;
        table_index++;
    }    /* End loop over all tables. */
    return FBE_STATUS_OK;
}
/**************************************
 * end kermit_retry_test()
 **************************************/
void kermit_set_rg_options(void)
{
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.paged_metadata_io_expiration_time = 200;
    set_options.user_io_expiration_time = 200;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
void kermit_unset_rg_options(void)
{
    /*! @todo better would be to get the options initially and save them.  
     * Then restore them here. 
     */
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.paged_metadata_io_expiration_time = 10000;
    set_options.user_io_expiration_time = 45000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}

/*!**************************************************************
 * kermit_test_rg_config()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void kermit_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_kermit_error_case_t    **error_cases_p = NULL;
    error_cases_p = (fbe_kermit_error_case_t **) context_p; 

    mut_printf(MUT_LOG_TEST_STATUS, "running kermit test");

    /*  Disable automatic sparing so that no spares swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for zeroing and initial verify to finish. 
     * Otherwise we will see our tests hit unexpected results when the initial verify gets kicked off. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* Kermit test injects error purposefully so we do not want to run any kind of Monitor IOs to run during the test 
      * disable sniff and background zeroing to run at PVD level
      */
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();
    
    big_bird_write_background_pattern();
    
    /* Run test cases for this set of raid groups with this element size.
    */
    kermit_retry_test(rg_config_p, error_cases_p, raid_group_count);
    
    /*  Enable automatic sparing at the end of the test */
    status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Make sure we did not get any trace errors.  We do this in between each
     * raid group test since it stops the test sooner. 
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end kermit_test_rg_config()
 ******************************************/

/*!**************************************************************
 * kermit_test()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void kermit_test(fbe_kermit_raid_type_t raid_type)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_u32_t                   extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_kermit_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-10 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(KERMIT_MAX_DEGRADED_POSITIONS == (FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2));

    if (extended_testing_level == 0)
    {
        /* Qual.
         */
        error_cases_p = kermit_qual_error_cases[raid_type];
        rg_config_p = &kermit_raid_group_config_qual[raid_type][0];
    }
    else
    {
        /* Extended. 
         */
        error_cases_p = kermit_extended_error_cases[raid_type];
        rg_config_p = &kermit_raid_group_config_extended[raid_type][0];
    }
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", raid_type_string_p, rg_config_p->raid_type);
        return;
    }

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config(rg_config_p, error_cases_p, 
                                   kermit_test_rg_config,
                                   KERMIT_LUNS_PER_RAID_GROUP,
                                   KERMIT_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end kermit_test()
 ******************************************/
void kermit_raid0_test(void)
{
    kermit_test(KERMIT_RAID_TYPE_RAID0);
}
void kermit_raid1_test(void)
{
    kermit_test(KERMIT_RAID_TYPE_RAID1);
}
void kermit_raid3_test(void)
{
    kermit_test(KERMIT_RAID_TYPE_RAID3);
}
void kermit_raid5_test(void)
{
    kermit_test(KERMIT_RAID_TYPE_RAID5);
}
void kermit_raid6_test(void)
{
    kermit_test(KERMIT_RAID_TYPE_RAID6);
}
void kermit_raid10_test(void)
{
    kermit_test(KERMIT_RAID_TYPE_RAID10);
}
/*!**************************************************************
 * kermit_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 0 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void kermit_setup(void)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;

        if (extended_testing_level == 0)
        {
            /* Qual.
             */
            array_p = &kermit_raid_group_config_qual[0];
        }
        else
        {
            /* Extended. 
             */
            array_p = &kermit_raid_group_config_extended[0];
        }
        for (test_index = 0; test_index < KERMIT_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
        }
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
        elmo_load_esp_sep_and_neit();
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************
 * end kermit_setup()
 ******************************************/

/*!**************************************************************
 * kermit_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the kermit test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void kermit_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_restore_sector_trace_level();
    
    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_esp_neit_sep_physical();
    }
    return;
}
/******************************************
 * end kermit_cleanup()
 ******************************************/
/*************************
 * end file kermit_test.c
 *************************/
