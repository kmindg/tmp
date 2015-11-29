/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file agent_jay_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains write_log flush protocol errors test.
 *  
 * @version
 *   4/23/2012 - Vamsi V. Created from Kermit test.
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
#include "sep_hook.h"
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
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * agent_jay_short_desc = "write_log flush protocol errors test.";
char * agent_jay_long_desc ="\
The agent_jay scenario tests parity rg's handling of write_log flush with protocol errors.\n\
\n\
-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and element sizes and additional error cases at -rtl 1.\n\
\n"\
                            "STEP 1: Bring up the initial topology.\n\
           - Configure all raid groups and luns \n\
           - Each raid group has one lun \n\
           - Each lun will have 3 chunks \n\
           - Each raid type will be tested with 520 block drives \n\
STEP 2: Write background pattern to target stripe.\n\
STEP 3: Degrade RG.\n\
STEP 4: Enable injection of protocol errors for write phase.\n\
STEP 5: Issue degraded IO to target stripe which causes RG shutdown due to protocol errors.\n\
STEP 6: Enable injection of protocol errors for Flush phase.\n\
STEP 7: Bring back RG from shutdown.\n\
STEP 8: Wait for Flushes to complete.\n\
STEP 9: Issue read on target stripe and validate data.\n\
STEP 10: Destroy all configured objects and disable error injection.\n\
Switches:\n\
    -abort_cases_only - Only test the abort/expiration cases.\n\
    -start_table - The table to begin with.\n\
    -start_index - The index number to begin with.\n\
    -total_cases - The number of test cases to run.\n\
    -repeat_case_count - The number of times to repeat each case.\n\
\
Description last updated: 07/19/2012.\n";

/*!*******************************************************************
 * @def AGENT_JAY_LUNS_PER_RAID_GROUP_QUAL
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define AGENT_JAY_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def AGENT_JAY_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define AGENT_JAY_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def AGENT_JAY_MAX_BACKEND_IO_SIZE
 *********************************************************************
 * @brief Max size we expect the I/O to get broken up after.
 *
 *********************************************************************/
#define AGENT_JAY_MAX_BACKEND_IO_SIZE 2048

/*!*******************************************************************
 * @def AGENT_JAY_BASE_OFFSET
 *********************************************************************
 * @brief All drives have this base offset, that we need to add on.
 *
 *********************************************************************/
#define AGENT_JAY_BASE_OFFSET 0x10000

/*!*******************************************************************
 * @def AGENT_JAY_ERR_COUNT_MATCH
 *********************************************************************
 * @brief Error count should match the size of the element.
 *
 *********************************************************************/
#define AGENT_JAY_ERR_COUNT_MATCH FBE_U32_MAX

/*!*******************************************************************
 * @def AGENT_JAY_ERR_COUNT_MATCH_ELSZ
 *********************************************************************
 * @brief Error count should match the size of the element.
 *
 *********************************************************************/
#define AGENT_JAY_ERR_COUNT_MATCH_ELSZ (FBE_U32_MAX - 1)

/*!*******************************************************************
 * @def AGENT_JAY_ERR_COUNT_NON_ZERO
 *********************************************************************
 * @brief The error count should not be zero but can match any non-zero count.
 *
 *********************************************************************/
#define AGENT_JAY_ERR_COUNT_NON_ZERO (FBE_U32_MAX - 2)

/*!*******************************************************************
 * @def AGENT_JAY_WAIT_MSEC
 *********************************************************************
 * @brief max number of millseconds we will wait.
 *        The general idea is that waits should be brief.
 *        We put in an overall wait time to make sure the test does not get stuck.
 *
 *********************************************************************/
#define AGENT_JAY_WAIT_MSEC 60000

/*!*******************************************************************
 * @def AGENT_JAY_MAX_RG_COUNT
 *********************************************************************
 * @brief Max number of raid groups we expect to test in parallel.
 *
 *********************************************************************/
#define AGENT_JAY_MAX_RG_COUNT 16

/*!*******************************************************************
 * @def AGENT_JAY_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define AGENT_JAY_RAID_TYPE_COUNT (4)

/*!*******************************************************************
 * @var agent_jay_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t agent_jay_raid_group_config_qual[] = 
{
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
        {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            2,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            3,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end agent_jay_raid_group_config_qual()
 **************************************/

/*!*******************************************************************
 * @var agent_jay_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t agent_jay_raid_group_config_extended[] = 
{
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            2,         0},
        {16,      0x22000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            3,         0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            4,         1},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            5,         1},
        {16,      0x22000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            6,         1},
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
        {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            2,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            3,          0},
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            4,          1},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            5,          1},
        {8,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            6,          1},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            7,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end agent_jay_raid_group_config_extended()
 **************************************/

/*!*************************************************
 * @enum fbe_agent_jay_raid_type_t
 ***************************************************
 * @brief Set of raid types we will test.
 *
 ***************************************************/
typedef enum fbe_agent_jay_raid_type_e
{
    AGENT_JAY_RAID_TYPE_RAID5 = 0,
    AGENT_JAY_RAID_TYPE_RAID3 = 1,
    AGENT_JAY_RAID_TYPE_RAID6_SINGLE = 2,
    AGENT_JAY_RAID_TYPE_RAID6_DOUBLE = 3,
    AGENT_JAY_RAID_TYPE_LAST,
}
fbe_agent_jay_raid_type_t;

/*!*************************************************
 * @enum fbe_agent_jay_error_type_t
 ***************************************************
 * @brief This defines classes of errors which we use
 *        to map to a physical error type depending on
 *        the protocol being used (scsi/sata).
 *
 ***************************************************/
typedef enum fbe_agent_jay_error_type_e
{
    AGENT_JAY_ERROR_TYPE_INVALID = 0,
    AGENT_JAY_ERROR_TYPE_RETRYABLE,
    AGENT_JAY_ERROR_TYPE_PORT_RETRYABLE,
    AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
    AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
    AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
    AGENT_JAY_ERROR_TYPE_INCOMPLETE_WRITE,
    AGENT_JAY_ERROR_TYPE_LAST,
}
fbe_agent_jay_error_type_t;

/*!*************************************************
 * @enum fbe_agent_jay_error_location_t
 ***************************************************
 * @brief This defines where we will inject the
 *        error on the raid group.
 *
 ***************************************************/
typedef enum fbe_agent_jay_error_location_e
{
    AGENT_JAY_ERROR_LOCATION_INVALID = 0,
    AGENT_JAY_ERROR_LOCATION_USER_DATA, /*!< Only inject on user area. */
    AGENT_JAY_ERROR_LOCATION_USER_DATA_CHUNK_1, /*!< Only inject on user area. */
    AGENT_JAY_ERROR_LOCATION_PAGED_METADATA, /*< Only inject on paged metadata. */
    AGENT_JAY_ERROR_LOCATION_RAID, /*< Inject on all raid protected areas both user and paged metadata. */
    AGENT_JAY_ERROR_LOCATION_WRITE_LOG,   /* Inject on write log area. */
    AGENT_JAY_ERROR_LOCATION_LAST,
}
fbe_agent_jay_error_location_t;

/*!*************************************************
 * @struct fbe_agent_jay_drive_error_definition_t
 ***************************************************
 * @brief Single drive's error definition.
 *
 ***************************************************/
typedef struct fbe_agent_jay_drive_error_definition_s
{
    fbe_agent_jay_error_type_t error_type;  /*!< Class of error (retryable/non/media/soft) */
    fbe_payload_block_operation_opcode_t command; /*!< Command to inject on. */
    fbe_u32_t times_to_insert_error;
    fbe_u32_t rg_position_to_inject;
    fbe_bool_t b_does_drive_die_on_error; /*! FBE_TRUE if the drive will die. */
    fbe_agent_jay_error_location_t location; /*!< Area to inject the error. */
    fbe_u8_t tag; /*!< this tag value is significant only when b_inc_for_all_drives is true. error counts are incremented for all the records having similar tag value. */
}
fbe_agent_jay_drive_error_definition_t;

/*!*************************************************
 * @enum fbe_agent_jay_error_type_t
 ***************************************************
 * @brief This defines classes of errors which we use
 *        to map to a physical error type depending on
 *        the protocol being used (scsi/sata).
 *
 ***************************************************/
typedef enum fbe_agent_jay_live_stripe_data_e
{
    AGENT_JAY_LIVE_STRIPE_DATA_INVALID = 0,
    AGENT_JAY_LIVE_STRIPE_DATA_OLD,
    AGENT_JAY_LIVE_STRIPE_DATA_NEW,
    AGENT_JAY_LIVE_STRIPE_DATA_MIXED,
    AGENT_JAY_LIVE_STRIPE_DATA_LAST,
}
fbe_agent_jay_live_stripe_data_t;

/*!*************************************************
 * @enum fbe_agent_jay_shutdown_error_t
 ***************************************************
 * @brief This defines classes of errors which we use
 *        to map to a physical error type depending on
 *        the protocol being used (scsi/sata).
 *
 ***************************************************/
typedef enum fbe_agent_jay_shutdown_error_e
{
    AGENT_JAY_SHUTDOWN_ERROR_NA = 0,
    AGENT_JAY_SHUTDOWN_ERROR_HDR_RD,
    AGENT_JAY_SHUTDOWN_ERROR_DATA_RD,
    AGENT_JAY_SHUTDOWN_ERROR_DATA_FLUSH,
    AGENT_JAY_SHUTDOWN_ERROR_HDR_INV,
    AGENT_JAY_SHUTDOWN_ERROR_DATA_LAST,
}
fbe_agent_jay_shutdown_error_t;

/*!*******************************************************************
 * @def AGENT_JAY_ERROR_CASE_MAX_ERRORS
 *********************************************************************
 * @brief Max number of errors per error case.
 *
 *********************************************************************/
#define AGENT_JAY_ERROR_CASE_MAX_ERRORS 10

/*!*******************************************************************
 * @def AGENT_JAY_MAX_DEGRADED_POSITIONS
 *********************************************************************
 * @brief Max number of positions to degrade before starting the test.
 *
 * @note  To support striped mirrors this is the max array with / 2
 *
 *********************************************************************/
#define AGENT_JAY_MAX_DEGRADED_POSITIONS 8

/*!*******************************************************************
 * @def AGENT_JAY_MAX_TEST_WIDTHS
 *********************************************************************
 * @brief Max number of widths we can specify to test.
 *
 *********************************************************************/
#define AGENT_JAY_MAX_TEST_WIDTHS 4

/*!*******************************************************************
 * @def AGENT_JAY_MAX_ERROR_CASES
 *********************************************************************
 * @brief Max number of possible cases to validate against.
 *
 *********************************************************************/
#define AGENT_JAY_MAX_ERROR_CASES 4

/*!*******************************************************************
 * @def AGENT_JAY_MAX_POSSIBLE_STATUS
 *********************************************************************
 * @brief Max number of possible different I/O status for rdgen request
 *
 *********************************************************************/
#define AGENT_JAY_MAX_POSSIBLE_STATUS  2
#define AGENT_JAY_INVALID_STATUS       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID

/*!*******************************************************************
 * @def AGENT_JAY_NUM_DIRECT_IO_ERRORS
 *********************************************************************
 * @brief Number of errors to insert into direct I/O execution (i.e.
 *        error that are inject but not into the raid library state
 *        machine).
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *
 *********************************************************************/
#define AGENT_JAY_NUM_DIRECT_IO_ERRORS                 1

/*!*******************************************************************
 * @def AGENT_JAY_NUM_READ_AND_SINGLE_VERIFY_ERRORS
 *********************************************************************
 * @brief Number of error to inject that accomplishes the following:
 *          o Inject error assuming that direct I/o encounters the first
 *          o Inject error assuming read state machine encounters the second
 *          o Inject error assuming that verify state machine encounters the third
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *
 *********************************************************************/
#define AGENT_JAY_NUM_READ_AND_SINGLE_VERIFY_ERRORS    (AGENT_JAY_NUM_DIRECT_IO_ERRORS + 2)

/*!*******************************************************************
 * @def AGENT_JAY_NUM_RETRYABLE_TO_FAIL_DRIVE
 *********************************************************************
 * @brief Number of retryable errors to cause a drive to fail.
 *
 * @note  This assumes that we are in the recovery verify state machine.
 * 
 *********************************************************************/
#define AGENT_JAY_NUM_RETRYABLE_TO_FAIL_DRIVE 10

/*!*************************************************
 * @typedef fbe_agent_jay_error_case_t
 ***************************************************
 * @brief Single test case with the error and
 *        the blocks to read for.
 *
 ***************************************************/
typedef struct fbe_agent_jay_error_case_s
{
    fbe_u8_t *description_p;            /*!< Description of test case. */
    fbe_u32_t line;                     /*!< Line number of test case. */
    fbe_u32_t stripe_num;               /*!< Morley stripe number. */
    fbe_lba_t stripe_offset;            /*!< Offset into Morley stripe. */
    fbe_block_count_t blocks;           /*!< Rdgen block count */
    fbe_rdgen_operation_t operation;    /*!< Rdgen operation to send. */
    fbe_bool_t b_set_rg_expiration_time; /*!< FBE_TRUE to set expiration time rg puts into packet. */
    fbe_bool_t b_inc_for_all_drives; /*!< TRUE */

    fbe_agent_jay_drive_error_definition_t errors[AGENT_JAY_ERROR_CASE_MAX_ERRORS];
    fbe_u32_t num_errors;
    fbe_agent_jay_drive_error_definition_t flush_errors[AGENT_JAY_ERROR_CASE_MAX_ERRORS];
    fbe_u32_t flush_num_errors;
    fbe_agent_jay_shutdown_error_t shutdown_error; 
    fbe_u32_t degraded_positions[AGENT_JAY_MAX_DEGRADED_POSITIONS];
    fbe_u32_t widths[AGENT_JAY_MAX_TEST_WIDTHS]; /*! Widths to test with. FBE_U32_MAX if invalid*/
    fbe_time_t expiration_msecs; /*!< msecs before I/O will expire. */
    fbe_time_t abort_msecs; /*!< msecs before I/O will get cancelled. */
    fbe_notification_type_t wait_for_state_mask; /*!< Wait for the rg to get to this state. */
    fbe_u32_t expected_io_error_count;
    fbe_payload_block_operation_status_t block_status[AGENT_JAY_MAX_POSSIBLE_STATUS];
    fbe_payload_block_operation_qualifier_t block_qualifier[AGENT_JAY_MAX_POSSIBLE_STATUS];

    /* For any of these counters, FBE_U32_MAX means that it is not used. 
     * FBE_U32_MAX - 1 means to use a multiple of the element size. 
     */
    fbe_u32_t retryable_error_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected retryable errors. */
    fbe_u32_t non_retryable_error_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected non-retryable errors. */
    fbe_u32_t shutdown_error_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected shutdown errors. */
    fbe_u32_t correctable_media_error_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected correctable media errors. */
    fbe_u32_t uncorrectable_media_error_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected uncorrectable media errors. */
    fbe_u32_t correctable_time_stamp[AGENT_JAY_MAX_ERROR_CASES];
    fbe_u32_t correctable_write_stamp[AGENT_JAY_MAX_ERROR_CASES];
    fbe_u32_t uncorrectable_time_stamp[AGENT_JAY_MAX_ERROR_CASES];
    fbe_u32_t uncorrectable_write_stamp[AGENT_JAY_MAX_ERROR_CASES];
    fbe_u32_t correctable_coherency[AGENT_JAY_MAX_ERROR_CASES];
    fbe_u32_t correctable_crc_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected correctable crc errors. */
    fbe_u32_t uncorrectable_crc_count[AGENT_JAY_MAX_ERROR_CASES]; /*!< Expected uncorrectable crc errors. */
    fbe_u32_t max_error_cases; /*!< Max number of possible counts above. */

    /* Expected data on live-stripe after Flush operation. */
    fbe_agent_jay_live_stripe_data_t expected_data;

    /* Live stripe status */
    fbe_u32_t live_io_err_cnt;
    fbe_payload_block_operation_status_t live_block_status[AGENT_JAY_MAX_POSSIBLE_STATUS];
    fbe_payload_block_operation_qualifier_t live_block_qualifier[AGENT_JAY_MAX_POSSIBLE_STATUS];

    /* Pre-blocks status */
    fbe_u32_t pre_io_err_cnt;
    fbe_payload_block_operation_status_t pre_block_status[AGENT_JAY_MAX_POSSIBLE_STATUS];
    fbe_payload_block_operation_qualifier_t pre_block_qualifier[AGENT_JAY_MAX_POSSIBLE_STATUS];

    /* Post-blocks status */
    fbe_u32_t post_io_err_cnt;
    fbe_payload_block_operation_status_t post_block_status[AGENT_JAY_MAX_POSSIBLE_STATUS];
    fbe_payload_block_operation_qualifier_t post_block_qualifier[AGENT_JAY_MAX_POSSIBLE_STATUS];
}
fbe_agent_jay_error_case_t;

/*!***************************************************************************
 * @typedef fbe_agent_jay_provision_drive_default_background_operations_t
 *****************************************************************************
 * @brief   Global setting of default background operations.
 *
 ***************************************************/
typedef struct fbe_agent_jay_provision_drive_default_background_operations_t
{
    fbe_bool_t  b_is_initialized;
    fbe_bool_t  b_is_background_zeroing_enabled;
    fbe_bool_t  b_is_sniff_verify_enabled;  
    /*! @note As more background operations are added add flag here.
     */
}
fbe_agent_jay_provision_drive_default_background_operations_t;

/*!*******************************************************************
 * @def AGENT_JAY_MAX_STRING_LENGTH
 *********************************************************************
 * @brief Max size of a string in a agent_jay structure.
 *
 *********************************************************************/
#define AGENT_JAY_MAX_STRING_LENGTH 500

static fbe_status_t fbe_sep_agent_jay_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                               fbe_object_id_t object_id,
                                                               fbe_rdgen_operation_t rdgen_operation,
                                                               fbe_u32_t num_ios);
static fbe_status_t agent_jay_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                 fbe_u32_t *position_p);
static fbe_bool_t agent_jay_get_and_check_degraded_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                            fbe_agent_jay_error_case_t *error_case_p,
                                                            fbe_u32_t  degraded_drive_index,
                                                            fbe_u32_t *degraded_position_p);
static fbe_status_t agent_jay_get_pdo_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                                fbe_u32_t drv_pos,
                                                fbe_object_id_t *pdo_object_id_p);
void agent_jay_setup(fbe_u32_t raid_type_index);
void agent_jay_set_rg_options(void);
void agent_jay_unset_rg_options(void);
fbe_bool_t agent_jay_is_rg_valid_for_error_case(fbe_agent_jay_error_case_t *error_case_p,
                                                fbe_test_rg_configuration_t *rg_config_p);
fbe_bool_t agent_jay_is_error_case_media_error(fbe_agent_jay_error_case_t *error_case_p,
                                               fbe_test_rg_configuration_t *rg_config_p);

/*!*******************************************************************
 * @var agent_jay_test_cases_single_degraded
 *********************************************************************
 * @brief These are single parity degraded only cases.
 *********************************************************************/
fbe_agent_jay_error_case_t agent_jay_test_cases_single_degraded[] =
{
    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media error on header read."
        "Reconstructs header and flushes new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- data pos 0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos 1 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

        { 
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media errors."
        "Abandon Flush. Live-stripe has old data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_DATA_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media errors."
        "Abandon Flush. Live-stripe has mix of old and new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_DATA_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_MIXED, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees retryable errors."
        "Flush is successful. Live-stripe has new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                1, /* raid group position to inject -- Diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_DATA_FLUSH, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on write to journal causes RG shutdown. Flush during RG activate sees retryable errors."
        "Header Reads are successful but Invalid. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- Diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_INV, /* Flush shutdown error */
        /* 1 degraded drive 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**********************************************
 * end agent_jay_test_cases_single_degraded *
 **********************************************/

/*!*******************************************************************
 * @var agent_jay_test_cases_single_degraded_extended
 *********************************************************************
 * @brief These are single parity degraded only cases.
 *********************************************************************/
fbe_agent_jay_error_case_t agent_jay_test_cases_single_degraded_extended[] =
{
    {
        "Degraded RG. Non-Retryable error on write to write_log causes RG shutdown. Flush during RG activate sees hard media errors."
        "Headers are reconstructed but data read fails with media error. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on write to write_log causes RG shutdown. Flush during RG activate sees soft media errors."
        "Invalid headers are detected. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                4, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                4, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },
    {
        "Degraded RG. Non-Retryable error on write to write_log causes RG shutdown. Flush during RG activate sees hard media errors."
        "Headers are reconstructed. Data read is successful but detects csum_of_csum error. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x2, /* stripe_offset */ 0x13, /* keep this odd and different from previous case to catch csum_of_csum */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },
    {

        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media error on header read."
        "Reconstructs header and flushes new data.", __LINE__,
        0x0, /* stripe_num */ 0x4, /* stripe_offset */ 0x93, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* test unaligned writes */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- data pos 0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on */
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos 1 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {FBE_U32_MAX}, /* Non-retryable errors. 1 for R5 and 2 for R6. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**********************************************
 * end agent_jay_test_cases_single_degraded_extended *
 **********************************************/

/*!*******************************************************************
 * @var agent_jay_test_cases_double_degraded
 *********************************************************************
 * @brief These are parity test cases we run during qual.
 *
 *********************************************************************/
fbe_agent_jay_error_case_t agent_jay_test_cases_double_degraded[] =
{
    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media error on header read."
        "Reconstructs header and flushes new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_TRUE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- data pos */
            FBE_FALSE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_INV, /* Flush shutdown error */
        /* 2 degraded drive at position 0 -- parity. 
         */
        { 0, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media errors."
        "Abandon Flush. Live-stripe has old data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x10 blocks will only test 468 and rcw */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- Data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media errors."
        "Abandon Flush. Live-stripe has a mix of old and new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_TRUE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            10, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_FALSE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_MIXED, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end agent_jay_test_cases_double_degraded
 **************************************/

/*!*******************************************************************
 * @var agent_jay_test_cases_double_degraded_extended
 *********************************************************************
 * @brief These are parity test cases we run during qual.
 *
 *********************************************************************/
fbe_agent_jay_error_case_t agent_jay_test_cases_double_degraded_extended[] =
{
    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees retryable errors."
        "Flush is successful. Live-stripe has new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_TRUE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            3, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_FALSE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_DATA_RD, /* Flush shutdown error */
        /* 1 degraded drive at position 0 -- parity. 
         */
        { 0, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on write to journal causes RG shutdown. Flush during RG activate sees retryable errors."
        "Header Reads are successful but Invalid. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            1, /* times to insert error. */
            0, /* raid group position to inject -- parity */
            FBE_TRUE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            3, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_FALSE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on write to write_log causes RG shutdown. Flush during RG activate sees retryable errors."
        "Header Reads are successful but data read fails with media error. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            1, /* times to insert error. */
            0, /* raid group position to inject -- parity */
            FBE_TRUE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
            10, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_FALSE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on write to write_log causes RG shutdown. Flush during RG activate sees soft media errors."
        "Invalid headers are detected. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                4, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { 1, FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on write to write_log causes RG shutdown. Flush during RG activate sees hard media errors."
        "Headers are reconstructed. Data read is successful but detects csum_of_csum error. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x2, /* stripe_offset */ 0x93, /* keep blk_cnt odd and different from previous case to catch csum_of_csum */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x93 will cover two blocks */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { 1, FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Degraded RG. Non-Retryable error on live-stripe write causes RG shutdown. Flush during RG activate sees media error on header read."
        "Reconstructs header and flushes new data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x17, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* test unaligned writes */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
            1, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
            FBE_TRUE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on */
            2, /* times to insert error. */
            FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- data pos */
            FBE_FALSE, /* Does drive die as a result of this error? */
            AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
            0, /* tag value only significant in case b_inc_for_all_drives is true*/
        },
        1, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_INV, /* Flush shutdown error */
        /* 2 degraded drive at position 0 -- parity. 
         */
        { 0, 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
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
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },


    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end agent_jay_test_cases_double_degraded_extended
 **************************************/

/*!*******************************************************************
 * @var agent_jay_test_cases_r6_single_degraded
 *********************************************************************
 * @brief These are single parity degraded cases for R6.
 *********************************************************************/
fbe_agent_jay_error_case_t agent_jay_test_cases_r6_single_degraded[] =
{
    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Flush during RG activate sees retryable and hard media errors. Header Reads are reconstructed. Flush is completed. Live-stripe has New data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                3, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- Data pos-0 */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                1, /* raid group position to inject -- Diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_DATA_FLUSH, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched one pos. Headers and data reads are successful; flush is performed. Live-stripe has New data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- parity */ /* This is also a degraded pos. So, flush does not read from here.*/
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_INV, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched diag parity pos. Header reads and data reads see media errors; flush is abandoned. Live-stripe has Old and invalidated data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        1, /* Number of I/O errors we expect. Invalidated sectors are entirely contained withing written area */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched diag parity pos but not row parity. Header reads and data reads see media errors; flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x11, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- raid group position to inject -- row parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_DATA_RD, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { 4, 6, 12, 16}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status - N/A for this lba */ 
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**********************************************
 * end agent_jay_test_cases_r6_single_degraded *
 **********************************************/

/*!*******************************************************************
 * @var agent_jay_test_cases_r6_single_degraded_extended
 *********************************************************************
 * @brief These are single parity degraded cases for R6.
 *********************************************************************/
fbe_agent_jay_error_case_t agent_jay_test_cases_r6_single_degraded_extended[] =
{

    {
        "Single Degraded R6. Non-Retryable errors on write to live-stripe causes RG shutdown. Live-stripe is inconsistent with time stamp errors."
        "Header reads and data reads see media errors; flush is abandoned. Live-stripe has mix of old/new data.", __LINE__,
        0x0, /* stripe_num */ 0x100, /* stripe_offset */ 0x200, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3, /* raid group position to inject -- raid group position to inject -- row parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 4, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { 6, 8, FBE_U32_MAX, FBE_U32_MAX }, /* Test width 6 for rcw and 8 for 468 */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {FBE_U32_MAX}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_MIXED, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status - N/A for this lba */ 
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    { /* Single degraded R6 data corruption. */
        "Single Degraded R6. Non-Retryable errors on write to live-stripe causes RG shutdown. Live-stripe is inconsistent with time/write stamp errors."
        "Header reads and data reads see media errors; flush is abandoned. Live-stripe has mix of old/new data.", __LINE__,
        0x0, /* stripe_num */ 0x93, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- raid group position to inject -- row parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- raid group position to inject -- data 1*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { 6, 8, FBE_U32_MAX, FBE_U32_MAX}, /* Test rcw with width 6 and 468 with 8 */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {FBE_U32_MAX}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_MIXED, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status - N/A for this lba */ 
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    { /* R6 Double degraded data corruption */
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched row parity pos. Headers reads and data reads see media errors; flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x11, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    { /* Single degraded R6 data corruption. MR3 wide width */
        "Single Degraded R6. Non-Retryable errors on write to live-stripe causes RG shutdown. Live-stripe is inconsistent with time/write stamp errors."
        "Header reads and data reads see media errors; flush is abandoned. Live-stripe has mix of old/new data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x300, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- raid group position to inject -- row parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 4, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        3, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { 8, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {3}, /* Non-retryable errors. */
        {FBE_U32_MAX}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_MIXED, /* expected data in live-stripe */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status - N/A for this lba */ 
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {/* VV: Check if timestamps are fixed. */
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched row parity pos but not diag parity. Header reads and data reads see media errors; flush is abandoned. Live-stripe has Invalidated data.", __LINE__,
        0x0, /* stripe_num */ 0x7, /* stripe_offset */ 0x11, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {/* Cause flush abandoned on multiple header read failures to send event to event log. */
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched row parity pos but not diag parity. Both header reads see media errors; flush is abandoned. Live-stripe has Invalidated data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x13, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    { /* MR3 Dead error BUG */ 
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write does not touch any pos. Header reads and data reads are successful; flush is completed. Live-stripe has New data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x100, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        /* 0x100 blocks will test mr3 for width 3/4, rcw for width 5/6 and 468 for larger widths */
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* diag parity pos */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- raid group position to inject -- diag parity*/
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                2, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- data pos */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_INV, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched 1 data pos. Headers reads and data reads see media errors; flush is abandoned. Live-stripe has NEW data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                0, /* raid group position to inject -- row parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at position not touched by the write. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_NEW, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status - N/A for this Lba*/
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe write touched no pos. Headers are reconstructed but data read fails with media error. Flush is abandoned. Live-stripe has Old data.", __LINE__,
        0x0, /* stripe_num */ 0x1, /* stripe_offset */ 0x10, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                1, /* raid group position to inject -- parity */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- data pos-0 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_NA, /* Flush shutdown error */
        /* 1 degraded drive at row parity. 
         */
        { 0, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },

    {
        "Single Degraded R6. Non-Retryable error on write to journal causes RG double degraded. Non-Retryable error on write to live-stripe causes RG shutdown."
        "Live stripe has new data in both parities and one data pos and has old data in one data pos. Headers are reconstructed but data read fails with media error."
        "Flush is abandoned. Live-stripe has Invalidated data.", __LINE__,
        0x0, /* stripe_num */ 0x0, /* stripe_offset */ 0x200, /* blocks */ FBE_RDGEN_OPERATION_WRITE_ONLY, /* opcode */ 
        FBE_FALSE, /* set rg expiration time? */
        FBE_FALSE, /* inc for all drives */
        /* Errors during Write phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2, /* raid group position to inject -- data pos 1 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,  /* opcode to inject on*/
                1, /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3, /* raid group position to inject -- data pos-3 */
                FBE_TRUE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_USER_DATA, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        /* Errors during Flush phase.
         */
        {
            {
                AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                10, /* times to insert error. */
                0, /* raid group position to inject -- parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,  /* opcode to inject on*/
                5, /* times to insert error. */
                1, /* raid group position to inject -- diag parity */
                FBE_FALSE, /* Does drive die as a result of this error? */
                AGENT_JAY_ERROR_LOCATION_WRITE_LOG, /* Location to inject. */
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
        },
        2, /* Num Errors */
        AGENT_JAY_SHUTDOWN_ERROR_HDR_RD, /* Flush shutdown error */
        /* 1 degraded drive at data pos. 
         */
        { FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX}, 
        { 6, 8, 16, FBE_U32_MAX}, /* Test all widths. */
        0, /* Expiration time. */ 0, /* No abort msecs. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        /*  
         * --> We see a shutdown error when the monitor fails the I/O. In this case the I/O completed first, and went
         * waiting.  Once the monitor runs it fails the I/O. 
         */
        {0}, /* Retryable errors */
        {2}, /* Non-retryable errors. */
        {1}, /* Shutdown errors. */
        {0}, /* Correctable media errors. */
        {0}, /* Uncorrectable media errors. */
        {0}, /* correctable time stamp errors. */
        {0}, /* correctable write stamp errors. */
        {0}, /* uncorrectable time stamp errors. */
        {0}, /* uncorrectable write stamp errors. */
        {0}, /* correctable_coherency. */ {0}, /* c_crc */ {0}, /* u_crc */    
        1, /* number of error cases */
        AGENT_JAY_LIVE_STRIPE_DATA_OLD, /* expected data in live-stripe */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Pre-blocks status -- N/A for this LBA */
        1, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},

        /* Post-blocks status */
        0, /* Number of I/O errors we expect. */
        { FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
        { FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, /* Error we expect. */ AGENT_JAY_INVALID_STATUS},
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**********************************************
 * end agent_jay_test_cases_r6_single_degraded_extended *
 **********************************************/

/*!*******************************************************************
 * @def AGENT_JAY_MAX_TABLES_PER_RAID_TYPE
 *********************************************************************
 * @brief Max number of error tables for each raid type.
 *
 *********************************************************************/
#define AGENT_JAY_MAX_TABLES_PER_RAID_TYPE 10

/*!*******************************************************************
 * @var agent_jay_qual_error_cases
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
fbe_agent_jay_error_case_t *agent_jay_qual_error_cases[AGENT_JAY_RAID_TYPE_COUNT][AGENT_JAY_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 5 */
    {
        &agent_jay_test_cases_single_degraded[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        //&agent_jay_test_cases_single_degraded[0], /* Disabled to meet 1000 sec limit */
        NULL /* Terminator */
    },
    /* Raid 6 single */
    {
        //&agent_jay_test_cases_single_degraded[0], /*TODO enable  */
        &agent_jay_test_cases_r6_single_degraded[0],
        NULL /* Terminator */
    },
    /* Raid 6 double*/
    {
        //&agent_jay_test_cases_r6_double_degraded[0],
        NULL /* Terminator */
    },
};

/*!*******************************************************************
 * @var agent_jay_extended_error_cases
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
fbe_agent_jay_error_case_t *agent_jay_extended_error_cases[AGENT_JAY_RAID_TYPE_COUNT][AGENT_JAY_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 5 */
    {
        &agent_jay_test_cases_single_degraded[0],
        &agent_jay_test_cases_single_degraded_extended[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        &agent_jay_test_cases_single_degraded[0],
        &agent_jay_test_cases_single_degraded_extended[0],
        NULL /* Terminator */
    },
    /* Raid 6 SINGLE degraded */
    {
        //&agent_jay_test_cases_single_degraded[0], /* TODO: Enable */
        //&agent_jay_test_cases_single_degraded_extended[0], /* TODO: Enable */
        &agent_jay_test_cases_r6_single_degraded[0],
        &agent_jay_test_cases_r6_single_degraded_extended[0],
        NULL /* Terminator */
    },
    /* Raid 6 DOUBLE degraded */
    {
        &agent_jay_test_cases_double_degraded[0],
        &agent_jay_test_cases_double_degraded_extended[0],
        NULL /* Terminator */
    },
};

/*!***************************************************************************
 * @var     agent_jay_provision_drive_default_operations
 *****************************************************************************
 * @brief   This simply records the default value for the provision drive
 *          background operations.
 *
 *********************************************************************/
fbe_agent_jay_provision_drive_default_background_operations_t agent_jay_provision_drive_default_operations = { 0};

/*!**************************************************************
 * agent_jay_send_seed_io()
 ****************************************************************
 * @brief
 *  Write background pattern to whole parity stripe.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/30/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t agent_jay_send_io(fbe_test_rg_configuration_t *rg_config_p,
                               fbe_api_rdgen_context_t *context_p,
                               fbe_rdgen_operation_t rdgen_operation,
                               fbe_time_t expiration_msecs,
                               fbe_time_t abort_msecs,
                               fbe_rdgen_pattern_t pattern,
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
    context_p->start_io.specification.region_list[0].sp_id = FBE_DATA_PATTERN_SP_ID_A; /* Vamsi: Rdgen can over-write this value based on SP its writing from */
    context_p->start_io.specification.region_list_length++;

    /* Set flag to use regions */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Send IO */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}

/*!**************************************************************
 * agent_jay_send_seed_io()
 ****************************************************************
 * @brief
 *  Write background pattern to whole parity stripe.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/30/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t agent_jay_send_seed_io(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_agent_jay_error_case_t *error_case_p,
                                    fbe_api_rdgen_context_t *context_p, fbe_rdgen_pattern_t pattern,
                                    fbe_lba_t lba, fbe_lba_t *seed_lba, fbe_block_count_t *seed_blk_cnt, 
                                    fbe_u32_t pass_count)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_block_count_t elsz;
    fbe_u32_t data_disks;
    fbe_lba_t stripe_start_lba;
    fbe_lba_t stripe_end_lba;
    fbe_block_count_t blk_cnt;
    fbe_u32_t num_stripes_per_parity = 8; /* FBE_RAID_ELEMENTS_PER_PARITY, FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH */

    /* Calculate num data disks */
    if ((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) || (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3))
    {
        data_disks = rg_config_p->width - 1; 
    }
    else if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        data_disks = rg_config_p->width - 2;
    }
    else
    {
        MUT_FAIL_MSG("Unexpected operation");
    }

    /* start_lba is start of parity stripe */
    elsz = fbe_test_sep_util_get_element_size(rg_config_p);
    stripe_start_lba = lba;    
    stripe_start_lba = stripe_start_lba - (stripe_start_lba % (elsz * data_disks * num_stripes_per_parity));

    /* Count blocks up to end of parity stripe */
    stripe_end_lba = (lba + error_case_p->blocks - 1);  
    stripe_end_lba = stripe_end_lba - (stripe_end_lba % (elsz * num_stripes_per_parity * (rg_config_p->width - data_disks ))); /* This is start of ending parity-stripe */ 
    blk_cnt = ((stripe_end_lba - stripe_start_lba) + (elsz * num_stripes_per_parity * data_disks)); 

    /* Get Lun object IDs */
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Initialize rdgen context */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  blk_cnt /* max blocks.  We override this later with regions.  */);

    /* status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification, peer_options); */

    /* Create one region in rdgen context for this IO.
     */
    context_p->start_io.specification.region_list_length = 1; 
    context_p->start_io.specification.region_list[0].lba = stripe_start_lba;
    context_p->start_io.specification.region_list[0].blocks = blk_cnt;
    context_p->start_io.specification.region_list[0].pattern = pattern;
    context_p->start_io.specification.region_list[0].pass_count = pass_count;
    context_p->start_io.specification.region_list[0].sp_id = FBE_DATA_PATTERN_SP_ID_A;

    /* Set flag to use regions */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_USE_REGIONS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Send IO */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* return lba range to which background IO was performed to */
    *seed_lba = stripe_start_lba;
    *seed_blk_cnt = blk_cnt;

    return status;
}
static fbe_status_t agent_jay_wait_for_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_agent_jay_error_case_t *error_case_p,
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
                                                         AGENT_JAY_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for drives that failed due to injected errors.
         */
        for ( drive_index = 0; drive_index < error_case_p->num_errors; drive_index++)
        {
            degraded_position = error_case_p->errors[drive_index].rg_position_to_inject;
            agent_jay_get_drive_position(rg_config_p, &degraded_position);
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, degraded_position);
        }

        /* Wait for drives that failed due to us degrading the raid group on purpose.
         */
        for ( drive_index = 0; drive_index < AGENT_JAY_MAX_DEGRADED_POSITIONS; drive_index++)
        {
            if (agent_jay_get_and_check_degraded_position(rg_config_p,
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
static fbe_status_t agent_jay_get_verify_report(fbe_test_rg_configuration_t *rg_config_p,
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
static fbe_status_t agent_jay_wait_for_verify(fbe_test_rg_configuration_t *rg_config_p,
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

fbe_bool_t agent_jay_does_count_match(fbe_u32_t expected_count,
                                      fbe_u32_t actual_count,
                                      fbe_u32_t element_size )
{
    if (expected_count == AGENT_JAY_ERR_COUNT_MATCH)
    {
        /* If FBE_U32_MAX then it is always a match.
         */
        return FBE_TRUE;
    }
    /* If the expected count is AGENT_JAY_ERR_COUNT_MATCH_ELSZ then we match the errors to element size.
     */
    if (expected_count == AGENT_JAY_ERR_COUNT_MATCH_ELSZ)
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
    else if (expected_count == AGENT_JAY_ERR_COUNT_NON_ZERO)
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

void agent_jay_validate_io_verify_report_counts(fbe_agent_jay_error_case_t * error_case_p, 
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
        if (!agent_jay_does_count_match(error_case_p->correctable_time_stamp[index],
                                        verify_report_p->totals.correctable_time_stamp,
                                        element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!agent_jay_does_count_match(error_case_p->correctable_write_stamp[index],
                                        verify_report_p->totals.correctable_write_stamp,
                                        element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!agent_jay_does_count_match(error_case_p->uncorrectable_time_stamp[index],
                                        verify_report_p->totals.uncorrectable_time_stamp,
                                        element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!agent_jay_does_count_match(error_case_p->uncorrectable_write_stamp[index],
                                        verify_report_p->totals.uncorrectable_write_stamp,
                                        element_size))
        {
            b_match = FBE_FALSE;
        }
        if (!agent_jay_does_count_match(error_case_p->correctable_coherency[index],
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
void agent_jay_validate_verify_report(fbe_agent_jay_error_case_t *error_case_p,
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
    agent_jay_validate_io_verify_report_counts(error_case_p, verify_report_p, element_size);
    return;
}

void agent_jay_cleanup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_agent_jay_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_lun_verify_report_t verify_report;
    fbe_u32_t element_size;

    /* Wait for verifies to complete.  This needs to be after restoring the raid group,
     * since if we insert an error that requires verify and then shutdown the rg, 
     * then we must bring the rg back before we can wait for the verify. 
     */
    agent_jay_wait_for_verify(rg_config_p, AGENT_JAY_WAIT_MSEC);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Now get the verify report for the (1) LUN associated with the raid group
     */
    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate Verify counts.
     */
    element_size = fbe_test_sep_util_get_element_size(rg_config_p);
    agent_jay_validate_verify_report(error_case_p, &verify_report, element_size);

    mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     AGENT_JAY_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

static fbe_status_t agent_jay_wait_for_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_agent_jay_error_case_t *error_case_p,
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
            agent_jay_cleanup_test_case(rg_config_p, error_case_p);
        }
        rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for verifies to finish. (successful)==", __FUNCTION__);
    return FBE_STATUS_OK;
}

static fbe_status_t agent_jay_clear_drive_error_counts(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_agent_jay_error_case_t *error_case_p,
                                                       fbe_u32_t raid_group_count, fbe_u32_t num_errors)
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

            /* Clear degraded write errros */
            for ( drive_index = 0; drive_index < num_errors; drive_index++)
            {
                fbe_u32_t position = error_case_p->errors[drive_index].rg_position_to_inject;
                agent_jay_get_drive_position(rg_config_p, &position);
                agent_jay_get_pdo_object_id(rg_config_p, position, &object_id);
                status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }

            /* Clear flush errros */
            for ( drive_index = 0; drive_index < num_errors; drive_index++)
            {
                fbe_u32_t position = error_case_p->flush_errors[drive_index].rg_position_to_inject;
                agent_jay_get_drive_position(rg_config_p, &position);
                agent_jay_get_pdo_object_id(rg_config_p, position, &object_id);
                status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }

            if (error_case_p->b_set_rg_expiration_time)
            {
                /* We changed the service time.  Set it back to some default value.
                 */
                status = fbe_api_physical_drive_get_service_time(object_id, &get_service_time);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_physical_drive_set_service_time(object_id, get_service_time.default_service_time_ms);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        rg_config_p++;
    }
    return FBE_STATUS_OK;
}

/*{
    fbe_physical_drive_information_t physical_drive_info = { 0 };
    fbe_status_t status = FBE_STATUS_OK;
   
    status = fbe_api_physical_drive_get_drive_information(record->object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (physical_drive_info.block_size == FBE_4K_BYTES_PER_BLOCK)
    {
        record->lba_start = record->lba_start / FBE_520_BLOCKS_PER_4K;
        record->lba_end = record->lba_end / FBE_520_BLOCKS_PER_4K;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s pdo block size is 0x%x pdo object: %d start: 0x%llx end: 0x%llx", 
                   __FUNCTION__, FBE_4K_BYTES_PER_BLOCK, record->object_id, record->lba_start, record->lba_end);
    }
    mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s pdo object: %d start: 0x%llx end: 0x%llx", 
                   __FUNCTION__, record->object_id, record->lba_start, record->lba_end);
}*/

static fbe_status_t agent_jay_inject_scsi_error (fbe_protocol_error_injection_error_record_t *record_in_p,
                                                 fbe_protocol_error_injection_record_handle_t *record_handle_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* convert 520 error to 4k if necessary*/
    status = fbe_test_neit_utils_convert_to_physical_drive_range(record_in_p->object_id,
                                                                 record_in_p->lba_start,
                                                                 record_in_p->lba_end,
                                                                 &record_in_p->lba_start,
                                                                 &record_in_p->lba_end);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

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
static fbe_status_t agent_jay_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
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
static fbe_bool_t agent_jay_get_and_check_degraded_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                            fbe_agent_jay_error_case_t *error_case_p,
                                                            fbe_u32_t  degraded_drive_index,
                                                            fbe_u32_t *degraded_position_p)
{
    fbe_bool_t  b_position_degraded = FBE_FALSE;
    fbe_u32_t   max_num_degraded_positions = 0;

    /* First get the degraded position value from the error table
     */
    MUT_ASSERT_TRUE(degraded_drive_index < AGENT_JAY_MAX_DEGRADED_POSITIONS);
    *degraded_position_p = error_case_p->degraded_positions[degraded_drive_index];

    /* The purpose of this method is to prevent a bad entry in the error case.
     */
    switch (raid_group_config_p->raid_type)
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
        agent_jay_get_drive_position(raid_group_config_p, degraded_position_p);
        b_position_degraded = FBE_TRUE;
    }
    return(b_position_degraded);
}
static fbe_status_t agent_jay_get_port_enclosure_slot(fbe_test_rg_configuration_t *raid_group_config_p,
                                                      fbe_u32_t drv_pos,
                                                      fbe_u32_t *port_number_p,
                                                      fbe_u32_t *encl_number_p,
                                                      fbe_u32_t *slot_number_p )
{
    fbe_u32_t position_to_inject = drv_pos;
    agent_jay_get_drive_position(raid_group_config_p, &position_to_inject);
    *port_number_p = raid_group_config_p->rg_disk_set[position_to_inject].bus;
    *encl_number_p = raid_group_config_p->rg_disk_set[position_to_inject].enclosure;
    *slot_number_p = raid_group_config_p->rg_disk_set[position_to_inject].slot;
    return FBE_STATUS_OK;
}
static fbe_status_t agent_jay_get_pdo_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                                fbe_u32_t drv_pos,
                                                fbe_object_id_t *pdo_object_id_p)
{
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    status = agent_jay_get_port_enclosure_slot(raid_group_config_p, drv_pos,
                                               &port_number, &encl_number, &slot_number);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(port_number, encl_number, slot_number, pdo_object_id_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(*pdo_object_id_p, FBE_OBJECT_ID_INVALID);
    return status;
}
fbe_u8_t agent_jay_get_scsi_command(fbe_payload_block_operation_opcode_t opcode)
{
    fbe_u8_t scsi_command;
    /* Map a block operation to a scsi command that we should inject on.
     */
    switch (opcode)
    {
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
        scsi_command = FBE_SCSI_READ_16;
        break;
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        scsi_command = FBE_SCSI_WRITE_16;
        break;
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
        scsi_command = FBE_SCSI_WRITE_SAME_16;
        break;
    default:
        MUT_FAIL_MSG("unknown opcode");
        break;
    };
    return scsi_command;
}
fbe_sata_command_t agent_jay_get_sata_command(fbe_payload_block_operation_opcode_t opcode)
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

static void agent_jay_get_scsi_error(fbe_protocol_error_injection_scsi_error_t *protocol_error_p,
                                     fbe_agent_jay_error_type_t error_type)
{
    /* Map an error enum into a specific error.
     */
    switch (error_type)
    {
    case AGENT_JAY_ERROR_TYPE_RETRYABLE:
        fbe_test_sep_util_set_scsi_error_from_error_type(protocol_error_p, FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE);
        break;
    case AGENT_JAY_ERROR_TYPE_PORT_RETRYABLE:
        fbe_test_sep_util_set_scsi_error_from_error_type(protocol_error_p, FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE);
        break;
    case AGENT_JAY_ERROR_TYPE_NON_RETRYABLE:
        fbe_test_sep_util_set_scsi_error_from_error_type(protocol_error_p, FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE);
        break;
    case AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR:
        fbe_test_sep_util_set_scsi_error_from_error_type(protocol_error_p, FBE_TEST_PROTOCOL_ERROR_TYPE_HARD_MEDIA_ERROR);
        break;
    case AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR:
        fbe_test_sep_util_set_scsi_error_from_error_type(protocol_error_p, FBE_TEST_PROTOCOL_ERROR_TYPE_SOFT_MEDIA_ERROR);
        break;
    case AGENT_JAY_ERROR_TYPE_INCOMPLETE_WRITE: /* Same as non-retryable */
        protocol_error_p->scsi_sense_key = FBE_SCSI_SENSE_KEY_HARDWARE_ERROR;
        protocol_error_p->scsi_additional_sense_code = FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
        protocol_error_p->scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
        protocol_error_p->port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
        break;
    default:
        MUT_FAIL_MSG("unknown error type");
        break;
    };
    return;
}
static void agent_jay_get_sata_error(fbe_protocol_error_injection_sata_error_t *sata_error_p,
                                     fbe_agent_jay_error_type_t error_type)
{
    fbe_port_request_status_t                   port_status;
    /* Map an error enum into a specific error.
     */
    switch (error_type)
    {
    case AGENT_JAY_ERROR_TYPE_RETRYABLE:
        port_status = FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT;
        break;
    case AGENT_JAY_ERROR_TYPE_PORT_RETRYABLE:
        port_status = FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
        break;
    case AGENT_JAY_ERROR_TYPE_NON_RETRYABLE:
        port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
        break;
    case AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR:
        port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
        break;
    case AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR:
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
static void agent_jay_fill_error_record(fbe_test_rg_configuration_t *raid_group_config_p,
                                        fbe_agent_jay_drive_error_definition_t * error_p,
                                        fbe_protocol_error_injection_error_record_t *record_p)
{
    record_p->num_of_times_to_insert = error_p->times_to_insert_error;

    /* Tag tells the injection service to group together records.  When one is injected, 
     * the count is incremented on all.  To make this unique per raid group we add on the rg id. 
     */
    record_p->tag = error_p->tag + raid_group_config_p->raid_group_id;
    /* If there is a port error, then inject it.
     */
    if (error_p->error_type == AGENT_JAY_ERROR_TYPE_PORT_RETRYABLE)
    {
        record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT;
    }
    else
    {
        record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    }
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
    agent_jay_get_scsi_command(error_p->command);
    agent_jay_get_scsi_error(&record_p->protocol_error_injection_error.protocol_error_injection_scsi_error,
                             error_p->error_type);
    if (error_p->error_type == AGENT_JAY_ERROR_TYPE_INCOMPLETE_WRITE)
    {
        record_p->skip_blks = 1; /* Vamsi: hard coded */
    }
    else if (error_p->error_type == AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR)
    {
        record_p->skip_blks = 0;
    }
    return;
}

static void agent_jay_disable_provision_drive_background_operations(fbe_u32_t bus,
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
    if (agent_jay_provision_drive_default_operations.b_is_initialized == FBE_FALSE)
    {
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_background_zeroing_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_background_zeroing_enabled == FBE_TRUE)
        {
            agent_jay_provision_drive_default_operations.b_is_background_zeroing_enabled = FBE_TRUE;
        }
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_background_sniff_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_background_sniff_enabled == FBE_TRUE)
        {
            agent_jay_provision_drive_default_operations.b_is_sniff_verify_enabled = FBE_TRUE;
        }
        FBE_ASSERT_AT_COMPILE_TIME((FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING | FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF) ==
                                   FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
        agent_jay_provision_drive_default_operations.b_is_initialized = FBE_TRUE;
    }
    else
    {
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_background_zeroing_enabled);
        MUT_ASSERT_INT_EQUAL(agent_jay_provision_drive_default_operations.b_is_background_zeroing_enabled, b_is_background_zeroing_enabled);
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_background_sniff_enabled);
        MUT_ASSERT_INT_EQUAL(agent_jay_provision_drive_default_operations.b_is_sniff_verify_enabled, b_is_background_sniff_enabled);
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

/*!**************************************************************
 * agent_jay_enable_shutdown_error_injection()
 ****************************************************************
 * @brief
 *  Inject scsi errors shuch that RG will go shutdown during 
 *  write_log flushes.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/10/2012 - Created. Vamsi V
 *
 ****************************************************************/
static void agent_jay_enable_shutdown_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_agent_jay_error_case_t *error_case_p,
                                                      fbe_agent_jay_drive_error_definition_t *error_p,
                                                      fbe_u32_t num_errors,
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

    /* Get info about the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for ( error_index = 0; error_index < num_errors; error_index++)
    {
        /* Initialize the error injection record.
         */
        fbe_test_neit_utils_init_error_injection_record(&record);

        status = agent_jay_get_port_enclosure_slot(rg_config_p, error_p->rg_position_to_inject,
                                                   &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Put the pdo object id into the record.
         */
        agent_jay_get_pdo_object_id(rg_config_p, error_p->rg_position_to_inject, &record.object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_case_p->shutdown_error == AGENT_JAY_SHUTDOWN_ERROR_HDR_RD)
        {
            record.lba_start  = (info.write_log_start_pba);
            record.lba_end    = (info.write_log_start_pba);
            record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
            agent_jay_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
        }
        else if (error_case_p->shutdown_error == AGENT_JAY_SHUTDOWN_ERROR_DATA_RD)
        {
            record.lba_start  = (info.write_log_start_pba + 1); // hardcoded to slot-0
            record.lba_end    = info.write_log_start_pba + info.element_size;  /* slot size is now element + 1 header block */
            record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
            agent_jay_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
        }
        else if (error_case_p->shutdown_error == AGENT_JAY_SHUTDOWN_ERROR_DATA_FLUSH)
        {
            /* For now we just inject the error over a chunk.
             */
            record.lba_start  = 0x0;
            record.lba_end    = FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
            record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
            agent_jay_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
        }
        else if (error_case_p->shutdown_error == AGENT_JAY_SHUTDOWN_ERROR_HDR_INV)
        {
            record.lba_start  = (info.write_log_start_pba);
            record.lba_end    = (info.write_log_start_pba);
            record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
            agent_jay_get_scsi_command(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Error index %d location %d is invalid",
                       error_index, error_p->location);
            MUT_FAIL_MSG("Error location is invalid");
        }

        if (error_case_p->b_set_rg_expiration_time)
        {
#define AGENT_JAY_PDO_SERVICE_TIME_MS 200
            status = fbe_api_physical_drive_set_service_time(record.object_id, AGENT_JAY_PDO_SERVICE_TIME_MS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* Setup scsi error for shutdown */
        record.num_of_times_to_insert = 1;
        record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
        record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_HARDWARE_ERROR;
        record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code = FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
        record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
        record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;

        /* Tag tells the injection service to group together records.  When one is injected, 
         * the count is incremented on all.  To make this unique per raid group we add on the rg id. 
         */
        record.tag = error_p->tag + rg_config_p->raid_group_id;

        record.lba_start += AGENT_JAY_BASE_OFFSET;
        record.lba_end += AGENT_JAY_BASE_OFFSET;
        if (error_case_p->b_inc_for_all_drives)
        {
            record.b_inc_error_on_all_pos  = FBE_TRUE;
        }

        status = agent_jay_inject_scsi_error(&record, &handle_array_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s rg: 0x%x pos: %d pdo object: %d (0x%x) b/e/s:%d/%d/%d slba:0x%llx elba:0x%llx", 
                   __FUNCTION__, rg_object_id, error_p->rg_position_to_inject, 
                   record.object_id, record.object_id, port_number, encl_number, slot_number,
                   (unsigned long long)record.lba_start, (unsigned long long)record.lba_end);

        /* Increment to next error */
        error_p++;
    }
    return;
}

static void agent_jay_enable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_agent_jay_error_case_t *error_case_p,
                                             fbe_agent_jay_drive_error_definition_t *error_p,
                                             fbe_u32_t num_errors,
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

    /* Get info about the raid group */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for ( error_index = 0; error_index < num_errors; error_index++)
    {
        /* Initialize the error injection record.
         */
        fbe_test_neit_utils_init_error_injection_record(&record);

        status = agent_jay_get_port_enclosure_slot(rg_config_p, error_p->rg_position_to_inject,
                                                   &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Put the pdo object id into the record.
         */
        agent_jay_get_pdo_object_id(rg_config_p, error_p->rg_position_to_inject, &record.object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_p->location == AGENT_JAY_ERROR_LOCATION_RAID)
        {
            /* Inject on both user and metadata.
             */
            record.lba_start  = 0x0;
            record.lba_end    = (info.raid_capacity / info.num_data_disk) - 1;
        }
        else if (error_p->location == AGENT_JAY_ERROR_LOCATION_USER_DATA)
        {
            /* For now we just inject the error over a chunk.
             */
            record.lba_start  = 0x0;
            record.lba_end    = FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
        }
        else if (error_p->location == AGENT_JAY_ERROR_LOCATION_USER_DATA_CHUNK_1)
        {
            /* For now we just inject the error over a chunk.
             * The chunk we inject on depends on the error index.
             */
            record.lba_start  = FBE_RAID_DEFAULT_CHUNK_SIZE;
            record.lba_end    = record.lba_start + FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
        }
        else if (error_p->location == AGENT_JAY_ERROR_LOCATION_PAGED_METADATA)
        {
            /* Inject from the start of the paged metadata to the end of the paged metadata.
             */
            record.lba_start  = info.paged_metadata_start_lba / info.num_data_disk;
            record.lba_end    = (info.raid_capacity / info.num_data_disk) - 1;
        }
        else if (error_p->location == AGENT_JAY_ERROR_LOCATION_WRITE_LOG)
        {
            /* Using slot-0 + slot-1
             * The IO used in this test may be splitted to 2 siots, each siots will use a slot.
             * But we could not control the slot allocation order and maybe only one siots will touch the drive on which this error injects.
             * So we have to inject errors on both slots to block write logging journal flush.
             */
            record.lba_start  = info.write_log_start_pba;
            record.lba_end    = record.lba_start + info.element_size*2;  /* slot size is now element + 1 header block */
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Error index %d location %d is invalid",
                       error_index, error_p->location);
            MUT_FAIL_MSG("Error location is invalid");
        }

        if (error_case_p->b_set_rg_expiration_time)
        {
#define AGENT_JAY_PDO_SERVICE_TIME_MS 200
            status = fbe_api_physical_drive_set_service_time(record.object_id, AGENT_JAY_PDO_SERVICE_TIME_MS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        agent_jay_fill_error_record(rg_config_p, error_p, &record);
        record.lba_start += AGENT_JAY_BASE_OFFSET;
        record.lba_end += AGENT_JAY_BASE_OFFSET;


        if (error_case_p->b_inc_for_all_drives)
        {
            record.b_inc_error_on_all_pos  = FBE_TRUE;
        }

        /* If the error is not lba specific, disable provision drive background services.*/
        if (error_p->error_type == AGENT_JAY_ERROR_TYPE_PORT_RETRYABLE)
        {
            MUT_ASSERT_INT_EQUAL(1, num_errors);
            agent_jay_disable_provision_drive_background_operations(port_number, encl_number, slot_number);
        }

        status = agent_jay_inject_scsi_error(&record, &handle_array_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s rg: 0x%x pos: %d pdo object: %d (0x%x) b/e/s:%d/%d/%d slba:0x%llx elba:0x%llx", 
                   __FUNCTION__, rg_object_id, error_p->rg_position_to_inject, 
                   record.object_id, record.object_id, port_number, encl_number, slot_number,
                   (unsigned long long)record.lba_start, (unsigned long long)record.lba_end);

        /* Increment to next error */
        error_p++;
    }
    return;
}
static void agent_jay_enable_provision_drive_background_operations(fbe_u32_t bus,
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

    MUT_ASSERT_INT_EQUAL(agent_jay_provision_drive_default_operations.b_is_initialized, FBE_TRUE);

    /* Enable those provision drive operations that were enabled by default.*/
    if (agent_jay_provision_drive_default_operations.b_is_background_zeroing_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    if (agent_jay_provision_drive_default_operations.b_is_sniff_verify_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
static void agent_jay_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_agent_jay_drive_error_definition_t *error_p,
                                              fbe_u32_t num_errors,
                                              fbe_protocol_error_injection_record_handle_t *handle_p)
{
    fbe_u32_t error_index;
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    for ( error_index = 0; error_index < num_errors; error_index++)
    {
        status = fbe_api_protocol_error_injection_remove_record(handle_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = agent_jay_get_port_enclosure_slot(rg_config_p, error_p->rg_position_to_inject,
                                                   &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_p->error_type == AGENT_JAY_ERROR_TYPE_PORT_RETRYABLE)
        {
            MUT_ASSERT_INT_EQUAL(1, num_errors);
            agent_jay_enable_provision_drive_background_operations(port_number, encl_number, slot_number);
        }

        /* Increment to next error */
        error_p++;
    }
    return;
}

/*!**************************************************************
 * agent_jay_verify_pvd()
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

void agent_jay_verify_pvd(fbe_test_raid_group_disk_set_t *drive_to_insert_p)
{
    fbe_status_t    status;

    /* Verify the PDO and LDO are in the READY state
     */
    status = fbe_test_pp_util_verify_pdo_state(drive_to_insert_p->bus, 
                                               drive_to_insert_p->enclosure,
                                               drive_to_insert_p->slot,
                                               FBE_LIFECYCLE_STATE_READY,
                                               AGENT_JAY_WAIT_MSEC);
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
 * end agent_jay_verify_pvd()
 ******************************************/
void agent_jay_enable_trace_flags(fbe_object_id_t object_id)

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
void agent_jay_disable_trace_flags(fbe_object_id_t object_id)

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

void agent_jay_reset_drive_state(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_agent_jay_drive_error_definition_t *error_p)
{
    fbe_status_t status;
    fbe_object_id_t pdo_object_id;
    fbe_object_id_t pvd_object_id;

    fbe_u32_t position = error_p->rg_position_to_inject;

    status = agent_jay_get_pdo_object_id(rg_config_p, position, &pdo_object_id);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    agent_jay_get_drive_position(rg_config_p, &position);
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


    agent_jay_verify_pvd(&rg_config_p->rg_disk_set[position]);
    return;
}

void agent_jay_reinsert_drives(fbe_test_rg_configuration_t *rg_config_p,
                               fbe_agent_jay_drive_error_definition_t *error_p,
                               fbe_u32_t num_errors)
{
    fbe_u32_t drive_index;
    for ( drive_index = 0; drive_index < num_errors; drive_index++)
    {
        agent_jay_reset_drive_state(rg_config_p, error_p);
        error_p++;
    }
}

void agent_jay_degrade_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_agent_jay_error_case_t *error_case_p)
{
    fbe_u32_t index;
    fbe_u32_t number_physical_objects;
    fbe_u32_t degraded_position;

    for ( index = 0; index < AGENT_JAY_MAX_DEGRADED_POSITIONS; index++)
    {
        if (agent_jay_get_and_check_degraded_position(rg_config_p,
                                                      error_case_p,
                                                      index,
                                                      &degraded_position) == FBE_TRUE)
        {
            fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
            sep_rebuild_utils_remove_drive_and_verify(rg_config_p, degraded_position, 
                                                      (number_physical_objects - 1 /* Pdo*/), 
                                                      &rg_config_p->drive_handle[degraded_position]);
            sep_rebuild_utils_verify_rb_logging(rg_config_p, degraded_position);
        }
    }
    return;
}

void agent_jay_restore_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_agent_jay_error_case_t *error_case_p, 
                                   fbe_object_id_t obj_id)
{
    fbe_u32_t index;
    fbe_u32_t degraded_position;
    fbe_u32_t number_physical_objects;

    for ( index = 0; index < AGENT_JAY_MAX_DEGRADED_POSITIONS; index++)
    {
        if (agent_jay_get_and_check_degraded_position(rg_config_p,
                                                      error_case_p,
                                                      index,
                                                      &degraded_position) == FBE_TRUE)
        {
            fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
            sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, degraded_position, 
                                                        (number_physical_objects + 1 /* Pdo*/), 
                                                        &rg_config_p->drive_handle[degraded_position]);
        }
    }
    return;
}

void agent_jay_validate_io_error_counts(fbe_raid_verify_error_counts_t *io_error_counts_p)
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

/* This function is created since even though the drive dies metadata service will
   return IO retryable status that can cause the RAID to increment the retryable counter
   and agent_jay will fail infrequently where retryable error will be one greater than expected
*/
fbe_bool_t agent_jay_does_retry_count_match(fbe_u32_t expected_count,
                                            fbe_u32_t actual_count,
                                            fbe_u32_t element_size )
{
    if (expected_count == AGENT_JAY_ERR_COUNT_MATCH)
    {
        /* If FBE_U32_MAX then it is always a match.
         */
        return FBE_TRUE;
    }
    /* If the expected count is AGENT_JAY_ERR_COUNT_MATCH_ELSZ then we match the errors to element size.
     */
    if (expected_count == AGENT_JAY_ERR_COUNT_MATCH_ELSZ)
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
    else if (expected_count == AGENT_JAY_ERR_COUNT_NON_ZERO)
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
    /* Othewise we match the expected with actual + 1
     */
    else if ((expected_count != actual_count) &&
             ((expected_count+1) != actual_count))
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}

void agent_jay_validate_io_status_for_error_case(fbe_agent_jay_error_case_t *error_case_p, 
                                                 fbe_api_rdgen_context_t *context_p)
{
    /* Handle more than one error case and return a status.
     */
    fbe_u32_t index;
    fbe_bool_t b_match = FBE_FALSE;

    for ( index = 0; index < AGENT_JAY_MAX_POSSIBLE_STATUS; index++)
    {
        if ((error_case_p->block_status[index] != AGENT_JAY_INVALID_STATUS) &&
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
        for ( index = 0; index < AGENT_JAY_MAX_POSSIBLE_STATUS; index++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "expected case %d - bl_status: 0x%x bl_qual: 0x%x",
                       index,
                       error_case_p->block_status[index],
                       error_case_p->block_qualifier[index]);
        }
        MUT_FAIL_MSG("no io status match found");
    }
}
void agent_jay_validate_io_error_counts_for_error_case(fbe_agent_jay_error_case_t * error_case_p, 
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
        if (!agent_jay_does_retry_count_match(error_case_p->retryable_error_count[index],
                                              io_error_counts_p->retryable_errors, element_size))
        {
            b_match = FBE_FALSE;
        }

        /* If either count mismatches we set the boolean.
         */
        if ((error_case_p->non_retryable_error_count[index] != FBE_U32_MAX) &&
            (error_case_p->non_retryable_error_count[index] != io_error_counts_p->non_retryable_errors))
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

void agent_jay_enable_trace_flags_for_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t drive_index;
    fbe_object_id_t object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_raid_group_disk_set_t *disk_p = NULL;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    agent_jay_enable_trace_flags(rg_object_id);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    agent_jay_enable_trace_flags(object_id);

    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_p->bus, 
                                                                disk_p->enclosure,
                                                                disk_p->slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        agent_jay_enable_trace_flags(object_id);
    }
    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_index, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        agent_jay_enable_trace_flags(object_id);
    }
    return;
}
void agent_jay_disable_trace_flags_for_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t drive_index;
    fbe_object_id_t object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_raid_group_disk_set_t *disk_p = NULL;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    agent_jay_enable_trace_flags(rg_object_id);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    agent_jay_enable_trace_flags(object_id);

    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_p->bus, 
                                                                disk_p->enclosure,
                                                                disk_p->slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        agent_jay_enable_trace_flags(object_id);
    }
    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_index, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        agent_jay_enable_trace_flags(object_id);
    }
    return;
}

void agent_jay_setup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                               fbe_agent_jay_error_case_t *error_case_p,
                               fbe_api_rdgen_context_t *context_p,
                               fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                               fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_raid_library_debug_flags_t current_debug_flags;
    fbe_raid_library_debug_flags_t default_debug_flags;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

    /* Degrade raid group if needed.
     */
    agent_jay_degrade_raid_groups(rg_config_p, error_case_p);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Clear the verify report.
     */
    status = fbe_api_lun_clear_verify_report(lun_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_raid_group_get_library_debug_flags(rg_object_id, &current_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_get_library_debug_flags(FBE_OBJECT_ID_INVALID, &default_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add the existing flags to the siots tracing flags.
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING | 
                                                         current_debug_flags | default_debug_flags));

    /*@!todo
     * Add the following library and raid group debug flags temporarily to debug the current sporadic 
     * agent_jay failures. The following debug flags need to remove at some time once all agent_jay tests are stable. 
     * 04/09/2012 
     */
    /* set library debug flags */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                        (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING | 
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING |
                                                         FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING|
                                                         current_debug_flags | default_debug_flags));
    /* set raid group debug flags */
    fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
                                                  FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING);


    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

void agent_jay_wait_for_state(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_agent_jay_error_case_t *error_case_p,
                              fbe_api_rdgen_context_t *context_p,
                              fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                              fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Wait for the raid group to get to the expected state.  The notification was
         * initialized above. 
         */
        fbe_test_common_util_wait_lifecycle_state_notification(ns_context_p, AGENT_JAY_WAIT_MSEC);
        fbe_test_common_util_unregister_lifecycle_state_notification(ns_context_p);
    }
}
void agent_jay_validate_io_op(fbe_api_rdgen_context_t *context_p, fbe_u32_t io_err_cnt, 
                              fbe_payload_block_operation_status_t block_status, 
                              fbe_payload_block_operation_qualifier_t block_qualifier)
{
    fbe_raid_verify_error_counts_t *io_error_counts_p = &context_p->start_io.statistics.verify_errors;

    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, io_err_cnt);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, block_status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, block_qualifier);

    if (context_p->start_io.statistics.error_count == 0)
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
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->retryable_errors, 0);
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->non_retryable_errors, 0);
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->shutdown_errors, 0);
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_media_count, 0);
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_media_count, 0);
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->c_crc_count, 0);
        MUT_ASSERT_INT_EQUAL(io_error_counts_p->u_crc_count, 0);
    }
}

/*!**************************************************************
 * agent_jay_retry_test_case()
 ****************************************************************
 * @brief
 *  Execution flow for each error case.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/30/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t agent_jay_retry_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_agent_jay_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[AGENT_JAY_MAX_RG_COUNT][AGENT_JAY_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[AGENT_JAY_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p;
    fbe_api_rdgen_context_t context[AGENT_JAY_MAX_RG_COUNT];
    fbe_object_id_t rg_object_id[AGENT_JAY_MAX_RG_COUNT];
    fbe_status_t io_status;
    fbe_lba_t lba[AGENT_JAY_MAX_RG_COUNT];
    fbe_lba_t stripe_start_lba[AGENT_JAY_MAX_RG_COUNT]; 
    fbe_block_count_t stripe_blk_cnt[AGENT_JAY_MAX_RG_COUNT];
    fbe_object_id_t lun_object_id;
    fbe_u32_t expected_pass;
    fbe_bool_t pre_check[AGENT_JAY_MAX_RG_COUNT];
    fbe_bool_t post_check[AGENT_JAY_MAX_RG_COUNT];
    fbe_block_count_t elsz;
    fbe_u32_t num_stripes_per_parity = 8; /* FBE_RAID_ELEMENTS_PER_PARITY, FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH */
    fbe_rdgen_operation_t rdgen_op;
    fbe_u32_t num_errors;


    /* Lookup RG IDs and save them locally */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== Running rg width 0x%x ==", current_rg_config_p->width);
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id[rg_index]);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            mut_printf(MUT_LOG_TEST_STATUS, "== Skipping rg width 0x%x ==", current_rg_config_p->width);
        }
        current_rg_config_p++;
    }

    /* From error_case calculate LBA which is based on RG width. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            elsz = fbe_test_sep_util_get_element_size(current_rg_config_p);
            if ((current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) || (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3))
            {
                lba[rg_index] = (((current_rg_config_p->width - 1) * elsz) * num_stripes_per_parity * error_case_p->stripe_num) + error_case_p->stripe_offset; 
            }
            else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
            {
                lba[rg_index] = (((current_rg_config_p->width - 2) * elsz) * num_stripes_per_parity * error_case_p->stripe_num) + error_case_p->stripe_offset;
            }
            else
            {
                MUT_FAIL_MSG("Unexpected operation");
            }
            mut_printf(MUT_LOG_TEST_STATUS, "== Running rg lba 0x%llx ==", lba[rg_index]);
        }
        current_rg_config_p++;
    }

    /* Step-1: Write background pattern to whole stripe that error case will do IO to. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Seed the parity stripe with zero pattern. */
            agent_jay_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index], FBE_RDGEN_PATTERN_LBA_PASS, 
                                   lba[rg_index], &stripe_start_lba[rg_index], &stripe_blk_cnt[rg_index], 
                                   error_case_p->line /*pass-count*/ );
        }
        current_rg_config_p++;
    }

    /* Step-1a:  */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Seed the write_log slot with zero pattern. Reason for this step is so that an intial write causes
             * all subsequents IOs to write_log slot sent to physical package so that PEI can inject erros.
             * Otherwise, PVD can complete IOs if ZOD.
             */
            (void) edgar_the_bug_zero_write_log_slot(rg_object_id[rg_index], 0); /* TODO change to slot-0 */
        }
        current_rg_config_p++;
    }

    /* Step-2: Setup test case (Degrade RG, clear Verify report, etc). */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            agent_jay_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                      &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Step-3: Setup protocol error records and enable injection (for Write SM) */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
            {
                num_errors = error_case_p->num_errors; 
            }
            else
            {
                num_errors = (error_case_p->num_errors - 1);
            }
            agent_jay_enable_error_injection(current_rg_config_p, error_case_p, &error_case_p->errors[0], 
                                             num_errors, &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }   
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step-4: Degraded Write (invokes write logging) */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Send I/O.  This is an asynchronous send.
             */
            io_status = agent_jay_send_io(current_rg_config_p, &context[rg_index], FBE_RDGEN_OPERATION_WRITE_ONLY,
                                          error_case_p->expiration_msecs, error_case_p->abort_msecs, 
                                          FBE_RDGEN_PATTERN_LBA_PASS, lba[rg_index], error_case_p->blocks, (error_case_p->line + 1) /*pass-count*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, io_status);
        }
        current_rg_config_p++;
    }

    /* Step-5: Wait for I/O to finish. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            fbe_api_rdgen_wait_for_ios(&context[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&context[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Step-6: Check for any RG state transitions based on inserted errors */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Wait for RG to go Shutdown */
            status = fbe_api_wait_for_object_lifecycle_state(rg_object_id[rg_index], FBE_LIFECYCLE_STATE_FAIL,
                                                             AGENT_JAY_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Step-7: Check status of degraded write operation. */ 
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            fbe_u32_t element_size;
            fbe_raid_verify_error_counts_t *io_error_counts_p = &context[rg_index].start_io.statistics.verify_errors;

            /* Validate the I/O status against all possible status
             */
            agent_jay_validate_io_status_for_error_case(error_case_p, &context[rg_index]); 

            /* Make sure the io error counts make sense
             */
            agent_jay_validate_io_error_counts(io_error_counts_p);

            /* Validate the verify counts attached to the io.
             */
            element_size = fbe_test_sep_util_get_element_size(current_rg_config_p);
            agent_jay_validate_io_error_counts_for_error_case(error_case_p, io_error_counts_p, element_size);
        }
        current_rg_config_p++;
    }

    /* Step-8: Remove protocol error records and disable injection */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
            {
                num_errors = error_case_p->num_errors; 
            }
            else
            {
                num_errors = (error_case_p->num_errors - 1);
            }
            agent_jay_disable_error_injection(current_rg_config_p, &error_case_p->errors[0], 
                                              num_errors, &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step-pre9: Add scheduler hook to pause eval Rebuild, we don't want rebuild getting involved */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_api_scheduler_add_debug_hook(rg_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Step-9: Add scheduler hook to pause Flush Start */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_api_scheduler_add_debug_hook(rg_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                      FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Step-10: Restore state of RG */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
            {
                num_errors = error_case_p->num_errors; 
            }
            else
            {
                num_errors = (error_case_p->num_errors - 1);
            }
            agent_jay_clear_drive_error_counts(current_rg_config_p, error_case_p, 1 /*num_rgs*/, num_errors);

            /* Reinsert drives that failed due to errors.
             */
            agent_jay_reinsert_drives(current_rg_config_p, &error_case_p->errors[0], num_errors);
        }
        current_rg_config_p++;
    }

    /* Step-pre11s: Wait for scheduler hook (Indicates eval rebuild logging condition started, but paused)
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_test_wait_for_debug_hook(rg_object_id[rg_index], 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Give the drives 10 extra seconds over the standard 5 seconds that eval RL waits
             * Normally if the drive isn't ready and we mark for rebuild, it's just an efficiency issue,
             * but here, the rebuild will fix or invalidate errors intermittently and we can't predict it,
             * causing intermittent failures of the test when reading back the test io in Raid 6.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "Waiting %d seconds for drives to come back before eval rebuild logging", 
                       10000/1000);
            fbe_api_sleep(10000);

            /* Now release the scheduler */
            status = fbe_api_scheduler_del_debug_hook(rg_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                                      FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
        current_rg_config_p++;
    }

    if (error_case_p->shutdown_error != AGENT_JAY_SHUTDOWN_ERROR_NA)
    {
        /* Step-11s: Wait for scheduler hook (Indicates Flush cond function ran but Flushes are paused) */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                status = fbe_test_wait_for_debug_hook(rg_object_id[rg_index], 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                     FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
                                                     SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            current_rg_config_p++;
        }

        /* Step-12s: Setup protocol error records for RG shutdown and enable injection (for Flush SM) */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
                {
                    num_errors = error_case_p->num_errors; 
                }
                else
                {
                    num_errors = (error_case_p->num_errors - 1);
                }
                agent_jay_enable_shutdown_error_injection(current_rg_config_p, error_case_p, &error_case_p->errors[0], 
                                                          num_errors, &error_handles[rg_index][0]);
            }
            current_rg_config_p++;
        }   
        status = fbe_api_protocol_error_injection_start(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Step-13s: Switch hook from pause of Flush Start to Flush Done */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                status = fbe_api_scheduler_add_debug_hook(rg_object_id[rg_index],
                                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                          FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE,
                                                          0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_scheduler_del_debug_hook(rg_object_id[rg_index],
                                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                          FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
                                                          0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            current_rg_config_p++;
        }

        /* Step-14s: Wait for RG shutdown during Flush SM */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                      rg_object_id[rg_index], FBE_LIFECYCLE_STATE_FAIL,
                                                                                      20000, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            }
            current_rg_config_p++;
        }

        /* Step-15s: Remove Flush Done hook */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                status = fbe_api_scheduler_del_debug_hook(rg_object_id[rg_index],
                                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                          FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE,
                                                          0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            current_rg_config_p++;
        }

        /* Step-16s: Remove protocol error records and disable injection */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
                {
                    num_errors = error_case_p->flush_num_errors; 
                }
                else
                {
                    num_errors = (error_case_p->flush_num_errors - 1);
                }
                agent_jay_disable_error_injection(current_rg_config_p, &error_case_p->errors[0], 
                                                  num_errors, &error_handles[rg_index][0]);
            }
            current_rg_config_p++;
        }
        status = fbe_api_protocol_error_injection_stop(); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Step-9s: Add scheduler hook to pause Flush Start */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                status = fbe_api_scheduler_add_debug_hook(rg_object_id[rg_index],
                                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                          FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
                                                          0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            current_rg_config_p++;
        }

        /* Step-10s: Restore state of RG */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
        {
            if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
            {
                if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
                {
                    num_errors = error_case_p->num_errors; 
                }
                else
                {
                    num_errors = (error_case_p->num_errors - 1);
                }
                agent_jay_clear_drive_error_counts(current_rg_config_p, error_case_p, 1 /*num_rgs*/, num_errors);

                /* Reinsert drives that failed due to errors.
                 */
                agent_jay_reinsert_drives(current_rg_config_p, &error_case_p->errors[0], num_errors);
            }
            current_rg_config_p++;
        }
    }

    /* Step-11: Wait for scheduler hook (Indicates Flush cond function ran but Flushes are paused) */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_test_wait_for_debug_hook(rg_object_id[rg_index], 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                 FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Step-12: Setup protocol error records and enable injection (for Flush SM) */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
            {
                num_errors = error_case_p->flush_num_errors; 
            }
            else
            {
                num_errors = (error_case_p->flush_num_errors - 1);
            }
            agent_jay_enable_error_injection(current_rg_config_p, error_case_p, &error_case_p->flush_errors[0], 
                                             num_errors, &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }   
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step-13: Switch hook from pause of Flush Start to Flush Done */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_api_scheduler_add_debug_hook(rg_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                      FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_scheduler_del_debug_hook(rg_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                      FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Step-14: Wait for Flush on slots to complete */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            //fbe_bool_t b_flushed = FALSE;
            status = fbe_test_wait_for_debug_hook(rg_object_id[rg_index], 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                 FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE, 0,0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            //status = edgar_the_bug_verify_write_log_flushed(rg_object_id[rg_index], FBE_FALSE, 0, &b_flushed); /* hardcoded to slot-0 */
            //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            //MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_flushed);

        }
        current_rg_config_p++;
    }

    /* Step-15: Remove Flush Done hook */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_api_scheduler_del_debug_hook(rg_object_id[rg_index],
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH,
                                                      FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Step-16: Remove protocol error records and disable injection */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
            {
                num_errors = error_case_p->flush_num_errors; 
            }
            else
            {
                num_errors = (error_case_p->flush_num_errors - 1);
            }
            agent_jay_disable_error_injection(current_rg_config_p, &error_case_p->flush_errors[0], 
                                              num_errors, &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step-17: Wait for Lun to go Ready */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             AGENT_JAY_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Step-18: Issue read to check for background pattern on the stripe LBAs before error_case LBA range */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Check if there are any blocks in the stripe before error_case lba region */
            pre_check[rg_index] = (lba[rg_index] > stripe_start_lba[rg_index]) ? FBE_TRUE : FBE_FALSE; 
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if ((agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) && (pre_check[rg_index]))
        {
            /* Send I/O.  This is an asynchronous send.
             */
            io_status = agent_jay_send_io(current_rg_config_p, &context[rg_index], FBE_RDGEN_OPERATION_READ_CHECK,
                                          error_case_p->expiration_msecs, error_case_p->abort_msecs, 
                                          FBE_RDGEN_PATTERN_LBA_PASS, stripe_start_lba[rg_index], 
                                          (lba[rg_index] - stripe_start_lba[rg_index]), error_case_p->line /*pass-count*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, io_status);
        }
        current_rg_config_p++;
    }

    /* Wait for I/O to finish. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if ((agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) && (pre_check[rg_index]))
        {
            fbe_api_rdgen_wait_for_ios(&context[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&context[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Check status of read IO */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if ((agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) && (pre_check[rg_index]))
        {
            agent_jay_validate_io_op(&context[rg_index], error_case_p->pre_io_err_cnt, 
                                     error_case_p->pre_block_status[0], error_case_p->pre_block_qualifier[0]);
        }
        current_rg_config_p++;
    }

    /* Step-19: Issue read to check on previous write operation (this validates the Flush operation) */
    if (error_case_p->expected_data == AGENT_JAY_LIVE_STRIPE_DATA_NEW)
    {
        rdgen_op = FBE_RDGEN_OPERATION_READ_CHECK; 
        expected_pass = (error_case_p->line + 1);
    }
    else if (error_case_p->expected_data == AGENT_JAY_LIVE_STRIPE_DATA_OLD)
    {
        rdgen_op = FBE_RDGEN_OPERATION_READ_CHECK;
        expected_pass = error_case_p->line;
    }
    else if (error_case_p->expected_data == AGENT_JAY_LIVE_STRIPE_DATA_MIXED)
    {
        rdgen_op = FBE_RDGEN_OPERATION_READ_ONLY;
        expected_pass = 0; /* N/A */
    }
    else
    {
        MUT_FAIL_MSG("Unexpected operation");
    }
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Send I/O.  This is an asynchronous send.
             */
            io_status = agent_jay_send_io(current_rg_config_p, &context[rg_index], rdgen_op,
                                          error_case_p->expiration_msecs, error_case_p->abort_msecs, 
                                          FBE_RDGEN_PATTERN_LBA_PASS, lba[rg_index], error_case_p->blocks, expected_pass);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, io_status);
        }
        current_rg_config_p++;
    }

    /* Wait for I/O to finish. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            fbe_api_rdgen_wait_for_ios(&context[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&context[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Check status of read IO */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            agent_jay_validate_io_op(&context[rg_index], error_case_p->live_io_err_cnt, 
                                     error_case_p->live_block_status[0], error_case_p->live_block_qualifier[0]);
        }
        current_rg_config_p++;
    }

    /* Step-20: Issue read to check for background pattern on the stripe LBAs after error_case LBA range */

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            /* Check if there are any blocks in the stripe after error_case lba region */
            post_check[rg_index] = ((stripe_start_lba[rg_index] + stripe_blk_cnt[rg_index]) > (lba[rg_index] + error_case_p->blocks)) ? FBE_TRUE : FBE_FALSE; 
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if ((agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) && (post_check[rg_index]))
        {
            /* Send I/O.  This is an asynchronous send.
             */
            io_status = agent_jay_send_io(current_rg_config_p, &context[rg_index], FBE_RDGEN_OPERATION_READ_CHECK,
                                          error_case_p->expiration_msecs, error_case_p->abort_msecs, FBE_RDGEN_PATTERN_LBA_PASS,
                                          (lba[rg_index] + error_case_p->blocks), /* start_lba which is end of error_case lba range */ 
                                          ((stripe_start_lba[rg_index] + stripe_blk_cnt[rg_index]) - (lba[rg_index] + error_case_p->blocks)), /* blk_cnt upto end of stripe */
                                          error_case_p->line /*pass-count*/); 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, io_status);
        }
        current_rg_config_p++;
    }

    /* Wait for I/O to finish. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if ((agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) && (post_check[rg_index]))
        {
            fbe_api_rdgen_wait_for_ios(&context[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&context[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Check status of read IO */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if ((agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) && (post_check[rg_index]))
        {
            agent_jay_validate_io_op(&context[rg_index], error_case_p->post_io_err_cnt, 
                                     error_case_p->post_block_status[0], error_case_p->post_block_qualifier[0]);
        }
        current_rg_config_p++;
    }

    /* Step-21: Restore drives that were pulled to setup degraded RG. */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            agent_jay_restore_raid_groups(current_rg_config_p, error_case_p, rg_object_id[rg_index]);
        }
        current_rg_config_p++;
    }

    /* Do this in a separate loop to allow all raid groups to rebuilds in parallel to save test time
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
            fbe_test_sep_util_wait_for_raid_group_to_rebuild(rg_object_id[rg_index]);
        }
        current_rg_config_p++;
    }

    /* Step-22: Check Verify report */
    current_rg_config_p = rg_config_p;
    if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
    {
        agent_jay_wait_for_verifies(current_rg_config_p, error_case_p, num_raid_groups);
    }

    /* Step-23: Check that flush didn't leave any valid headers in write log, now that
     *          verify is finished and thus any remaps of headers that had media errors are complete.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (agent_jay_is_rg_valid_for_error_case(error_case_p, current_rg_config_p))
        {
//            fbe_bool_t b_flushed = FBE_FALSE;
//            fbe_bool_t b_chk_remap = agent_jay_is_error_case_media_error(error_case_p, current_rg_config_p);

            /* Since below operation is time consuming, we will only randomly do the checks. */
            if ((fbe_random() % 10) == 0)
            {
                /* Check that the flush did not leave any valid slot headers on restored disks
                 */
                //status = edgar_the_bug_verify_write_log_flushed(rg_object_id[rg_index], b_chk_remap, FBE_PARITY_WRITE_LOG_INVALID_SLOT, &b_flushed); /* Verify all slots */
                //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                //MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_flushed);
            }
        }
        current_rg_config_p++;
    }

    return FBE_STATUS_OK;
}


fbe_u32_t agent_jay_get_table_case_count(fbe_agent_jay_error_case_t *error_case_p)
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
fbe_u32_t agent_jay_get_test_case_count(fbe_agent_jay_error_case_t *error_case_p[])
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
        test_case_count += agent_jay_get_table_case_count(&error_case_p[table_index][0]);
        table_index++;
    }
    return test_case_count;
}
fbe_u32_t agent_jay_get_num_error_positions(fbe_agent_jay_error_case_t *error_case_p)
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
fbe_bool_t agent_jay_is_width_valid_for_error_case(fbe_agent_jay_error_case_t *error_case_p,
                                                   fbe_u32_t width)
{
    fbe_u32_t index = 0;
    fbe_u32_t num_err_positions = agent_jay_get_num_error_positions(error_case_p);

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
fbe_u32_t agent_jay_error_case_num_valid_widths(fbe_agent_jay_error_case_t *error_case_p)
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

fbe_bool_t agent_jay_is_rg_valid_for_error_case(fbe_agent_jay_error_case_t *error_case_p,
                                                fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    if (!agent_jay_is_width_valid_for_error_case(error_case_p, rg_config_p->width))
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
fbe_bool_t agent_jay_is_error_case_media_error(fbe_agent_jay_error_case_t *error_case_p,
                                               fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t idx;
    fbe_u32_t num_errors;

    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        num_errors = error_case_p->flush_num_errors; 
    }
    else
    {
        num_errors = (error_case_p->flush_num_errors - 1);
    }

    for (idx = 0; idx < num_errors; idx++)
    {
        if ((error_case_p->flush_errors[idx].error_type == AGENT_JAY_ERROR_TYPE_HARD_MEDIA_ERROR) ||
            (error_case_p->flush_errors[idx].error_type == AGENT_JAY_ERROR_TYPE_SOFT_MEDIA_ERROR))
        {
            return FBE_TRUE; 
        }
    }

    return FBE_FALSE;
}
void agent_jay_run_test_case_for_all_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                                 fbe_agent_jay_error_case_t *error_case_p,
                                                 fbe_u32_t num_raid_groups,
                                                 fbe_u32_t overall_test_case_index,
                                                 fbe_u32_t total_test_case_count,
                                                 fbe_u32_t table_test_case_index,
                                                 fbe_u32_t table_test_case_count)
{
    fbe_status_t status;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();

    mut_printf(MUT_LOG_TEST_STATUS, "test_case: %s", error_case_p->description_p); 
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    mut_printf(MUT_LOG_TEST_STATUS, "starting table case: (%d of %d) overall: (%d of %d) line: %d for raid type %s (0x%x)", 
               table_test_case_index, table_test_case_count,
               overall_test_case_index, total_test_case_count, 
               error_case_p->line, raid_type_string_p, rg_config_p->raid_type);

    status = agent_jay_retry_test_case(rg_config_p, error_case_p);
    if (status != FBE_STATUS_OK)
    {
        MUT_FAIL_MSG("test case failed\n");
    }
    return;
}
/*!**************************************************************
 * agent_jay_retry_test()
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
 *
 ****************************************************************/

fbe_status_t agent_jay_retry_test(fbe_test_rg_configuration_t *rg_config_p,
                                  fbe_agent_jay_error_case_t *error_case_p[],
                                  fbe_u32_t num_raid_groups)
{
    fbe_u32_t test_index = 0;
    fbe_u32_t table_test_index;
    fbe_u32_t total_test_case_count = agent_jay_get_test_case_count(error_case_p);
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
    b_abort_expiration_only = fbe_test_sep_util_get_cmd_option("-abort_cases_only");mut_printf(MUT_LOG_TEST_STATUS, "agent_jay start_table: %d start_index: %d total_cases: %d abort_only: %d",start_table_index, start_test_index, max_case_count, b_abort_expiration_only);

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
        table_test_count = agent_jay_get_table_case_count(&error_case_p[table_index][0]);

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
                agent_jay_set_rg_options();
            }
            current_iteration = 0;
            repeat_count = 1;
            fbe_test_sep_util_get_cmd_option_int("-repeat_case_count", &repeat_count);
            while (current_iteration < repeat_count)
            {
                /* Run this test case for all raid groups.
                 */
                agent_jay_run_test_case_for_all_raid_groups(rg_config_p, 
                                                            &error_case_p[table_index][table_test_index], 
                                                            num_raid_groups,
                                                            test_index, total_test_case_count,
                                                            table_test_index,
                                                            table_test_count);
                current_iteration++;
                if (repeat_count > 1)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "agent_jay completed test case iteration [%d of %d]", current_iteration, repeat_count);
                }
            }
            agent_jay_unset_rg_options();
            time = fbe_get_time();
            msec_duration = (time - start_time);
            mut_printf(MUT_LOG_TEST_STATUS,
                       "table %d test %d took %lld seconds (%lld msec)",
                       table_index, table_test_index,
                       (long long)(msec_duration / FBE_TIME_MILLISECONDS_PER_SECOND),
                       (long long)msec_duration);
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

    /* in this test, the injected scsi error will have EOL set on some drives, which will trigger proactive copy on VD.
     * when proactive copy completes, internal SWAP_OUT job will be added to queue to swap out edges.
     * But when comes out of this function, test frame work will also add JOBs to destroy LUN and RG.
     * If RG destroy JOB goes ahead of SWAP_OUT JOB, the SWAP_OUT JOB will be failed since the VD has been destroyed.
     * So we simply add some delays here to avoid this case.
     */
    fbe_api_sleep(60000);

    return FBE_STATUS_OK;
}
/**************************************
 * end agent_jay_retry_test()
 **************************************/
void agent_jay_set_rg_options(void)
{
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.paged_metadata_io_expiration_time = 200;
    set_options.user_io_expiration_time = 200;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
void agent_jay_unset_rg_options(void)
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
 * agent_jay_test_rg_config()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/

void agent_jay_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_agent_jay_error_case_t  **error_cases_p = NULL;
    error_cases_p = (fbe_agent_jay_error_case_t **) context_p; 

    mut_printf(MUT_LOG_TEST_STATUS, "running agent_jay test");

    /*  Disable automatic sparing so that no spares swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Kermit test injects error purposefully so we do not want to run any kind of Monitor IOs to run during the test 
      * disable sniff and background zeroing to run at PVD level
      */
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();

    big_bird_write_background_pattern();

    /* Run test cases for this set of raid groups with this element size.
    */
    agent_jay_retry_test(rg_config_p, error_cases_p, raid_group_count);

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
 * end agent_jay_test_rg_config()
 ******************************************/

/*!**************************************************************
 * agent_jay_test()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/

void agent_jay_test(fbe_u32_t raid_type_index)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_u32_t                   extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_agent_jay_error_case_t  **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_status_t                status;
    fbe_raid_library_debug_flags_t default_debug_flags;

    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-6 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(AGENT_JAY_MAX_DEGRADED_POSITIONS == 8);

    /* Run the error cases for all raid types supported
     */
    if (extended_testing_level == 0)
    {
            /* Qual.
             */
            error_cases_p = agent_jay_qual_error_cases[raid_type_index];
            rg_config_p = &agent_jay_raid_group_config_qual[raid_type_index][0];
    }
    else
    {
        /* Extended. 
         */
        error_cases_p = agent_jay_extended_error_cases[raid_type_index];
        if (raid_type_index == AGENT_JAY_RAID_TYPE_RAID6_DOUBLE) {
            rg_config_p = &agent_jay_raid_group_config_extended[raid_type_index -1][0];
        } else {
            rg_config_p = &agent_jay_raid_group_config_extended[raid_type_index][0];
        }
    }
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", raid_type_string_p, rg_config_p->raid_type);
        return;
    }

    /* Since we're testing write log handling of errors, we don't want to deal with it skipping cases
     * that can be optimized around.  So we'll just turn off the skip logic for this test.
     * This needs to be prior to creating the RAID groups so the RAID groups all have this set. 
     */
    status = fbe_api_raid_group_get_library_debug_flags(FBE_OBJECT_ID_INVALID, &default_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_set_library_debug_flags(FBE_OBJECT_ID_INVALID, 
                                                        FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOG_SKIP | default_debug_flags);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (raid_type_index + 1 >= AGENT_JAY_RAID_TYPE_COUNT) {
        /* Now create the raid groups and run the tests
         */
        fbe_test_run_test_on_rg_config(rg_config_p, error_cases_p, 
                                       agent_jay_test_rg_config,
                                       AGENT_JAY_LUNS_PER_RAID_GROUP,
                                       AGENT_JAY_CHUNKS_PER_LUN);
    }
    else 
    {
        fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, error_cases_p, 
                                           agent_jay_test_rg_config,
                                           AGENT_JAY_LUNS_PER_RAID_GROUP,
                                           AGENT_JAY_CHUNKS_PER_LUN,
                                           FBE_FALSE);
    }    
    return;
}
/******************************************
 * end agent_jay_test()
 ******************************************/

/*!**************************************************************
 * agent_jay_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 0 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *
 ****************************************************************/
void agent_jay_setup(fbe_u32_t raid_type_index)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_array_t *array_p = NULL;

        if (extended_testing_level == 0)
        {
            /* Qual.
             */
            array_p = &agent_jay_raid_group_config_qual[0];
        }
        else
        {
            /* Extended. 
             */
            array_p = &agent_jay_raid_group_config_extended[0];
        }

        fbe_test_sep_util_init_rg_configuration_array(&array_p[raid_type_index][0]);
        fbe_test_rg_setup_random_block_sizes(&array_p[raid_type_index][0]);
        fbe_test_sep_util_randomize_rg_configuration_array(&array_p[raid_type_index][0]);

        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
        elmo_load_sep_and_neit();
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
 * end agent_jay_setup()
 ******************************************/

/*!**************************************************************
 * agent_jay_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the agent_jay test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *
 ****************************************************************/

void agent_jay_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_restore_sector_trace_level();

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end agent_jay_cleanup()
 ******************************************/

void agent_jay_test_r5()
{
    agent_jay_test(AGENT_JAY_RAID_TYPE_RAID5);
}

void agent_jay_test_r3() 
{
    agent_jay_test(AGENT_JAY_RAID_TYPE_RAID3);
}

void agent_jay_test_r6_single_degraded()
{
    agent_jay_test(AGENT_JAY_RAID_TYPE_RAID6_SINGLE);
}

void agent_jay_test_r6_double_degraded()
{
    agent_jay_test(AGENT_JAY_RAID_TYPE_RAID6_DOUBLE);
}

void agent_jay_setup_r5()
{
    agent_jay_setup(AGENT_JAY_RAID_TYPE_RAID5);
}

void agent_jay_setup_r3() 
{
    agent_jay_setup(AGENT_JAY_RAID_TYPE_RAID3);
}

void agent_jay_setup_r6()
{
    agent_jay_setup(AGENT_JAY_RAID_TYPE_RAID6_SINGLE);
}

/*************************
 * end file agent_jay_test.c
 *************************/
