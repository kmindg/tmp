/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file headless_horseman_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a degraded errors during rekey test.
 *
 * @version
 *   11/21/2013 - Created. Rob Foley
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
#include "fbe/fbe_api_encryption_interface.h"
#include "sep_rebuild_utils.h"                              //  .h shared between Dora and other NR/rebuild tests
#include "fbe_test.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * headless_horseman_short_desc = "Degraded and error test during Encryption Rekey";
char * headless_horseman_long_desc ="\
The Headless Horseman scenario tests handling of errors during Encryption Rekey..\n\
\n\
Switches:\n\
    -abort_cases_only - Only test the abort/expiration cases.\n\
    -start_table - The table to begin with.\n\
    -start_index - The index number to begin with.\n\
    -total_cases - The number of test cases to run.\n\
    -repeat_case_count - The number of times to repeat each case.\n\
\
Description last updated: 11/21/2013.\n";

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP_QUAL
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP 2

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_CHUNKS_PER_LUN 6

enum {
    /* Blocks RAID allocates to scan the paged.
     * We need our RG to be twice as big as this so we can pause encryption at some amount larger than the blob, so we 
     * can see a scan that is larger than a blob. 
     */  
    HEADLESS_BLOCKS_PER_BLOB = 16,
    HEADLESS_HORSEMAN_CHUNKS_PER_LUN_LARGE = (128 * HEADLESS_BLOCKS_PER_BLOB * 2),
    /* Convert to blocks and scale up by data disks (3).
     */
    HEADLESS_HORSEMAN_BLOCKS_PER_LARGE_RG = HEADLESS_HORSEMAN_CHUNKS_PER_LUN_LARGE * 0x800 * 2, // was 0x201000,

    /* Time to wait before allowing vault to encrypt
     */
    HEADLESS_HORSEMAN_VAULT_ENCRYPT_WAIT_TIME_MS = 10000,

    HEADLESS_EXTRA_CHUNKS = 0,
    
    HEADLESS_BLOCKS_PER_LARGE_R10_RG = ((HEADLESS_HORSEMAN_CHUNKS_PER_LUN_LARGE + HEADLESS_EXTRA_CHUNKS) * 0x800) + FBE_TEST_EXTRA_CAPACITY_FOR_METADATA,

};
/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_TEST_ELEMENT_SIZE
 *********************************************************************
 * @brief Default element size
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_TEST_ELEMENT_SIZE 128

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_BACKEND_IO_SIZE
 *********************************************************************
 * @brief Max size we expect the I/O to get broken up after.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_BACKEND_IO_SIZE 2048

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_BASE_OFFSET
 *********************************************************************
 * @brief All drives have this base offset, that we need to add on.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_BASE_OFFSET 0x10000

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_ERR_COUNT_MATCH
 *********************************************************************
 * @brief Error count should match the size of the element.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_ERR_COUNT_MATCH FBE_U32_MAX

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_ERR_COUNT_MATCH_ELSZ
 *********************************************************************
 * @brief Error count should match the size of the element.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_ERR_COUNT_MATCH_ELSZ (FBE_U32_MAX - 1)

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_ERR_COUNT_NON_ZERO
 *********************************************************************
 * @brief The error count should not be zero but can match any non-zero count.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_ERR_COUNT_NON_ZERO (FBE_U32_MAX - 2)

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE
 *********************************************************************
 * @brief When verifying IO error counts,  this flag will indicate it 
 *    should use a range for FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE, 
 *    instead of exact number.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_RANGE (FBE_U32_MAX - 3)


/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE
 *********************************************************************
 * @brief When verifying IO error counts,  this flag will indicate it 
 *    should use a range for HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2, 
 *    instead of exact number.  This should be used when error case
 *    injects retryable errors into 2 drives in same RG, expecting
 *    both drives to fail.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2_RANGE (FBE_U32_MAX - 4)

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_WAIT_MSEC
 *********************************************************************
 * @brief max number of millseconds we will wait.
 *        The general idea is that waits should be brief.
 *        We put in an overall wait time to make sure the test does not get stuck.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_WAIT_MSEC 360000

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_RG_COUNT
 *********************************************************************
 * @brief Max number of raid groups we expect to test in parallel.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_RG_COUNT 16
 
/*!*************************************************
 * @enum fbe_headless_horseman_raid_type_t
 ***************************************************
 * @brief Set of raid types we will test.
 *
 * @note  The raid type value must match the order in
 *        the error case and raid group configuration
 *        tables.
 ***************************************************/
typedef enum fbe_headless_horseman_raid_type_e
{
    HEADLESS_HORSEMAN_RAID_TYPE_RAID0 = 0,
    HEADLESS_HORSEMAN_RAID_TYPE_RAID1 = 1,
    HEADLESS_HORSEMAN_RAID_TYPE_RAID5 = 2,
    HEADLESS_HORSEMAN_RAID_TYPE_RAID3 = 3,
    HEADLESS_HORSEMAN_RAID_TYPE_RAID6 = 4,
    HEADLESS_HORSEMAN_RAID_TYPE_RAID10 = 5,
    HEADLESS_HORSEMAN_RAID_TYPE_LAST,
}
fbe_headless_horseman_raid_type_t;

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_RAID_TYPE_COUNT   (HEADLESS_HORSEMAN_RAID_TYPE_LAST)

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_BLOCK_COUNT
 *********************************************************************
 * @brief Total count of blocks to check/set with rdgen.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_BLOCK_COUNT 0x10000

/*!*******************************************************************
 * @var headless_horseman_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t headless_horseman_raid_group_config_qual[] = 
{
    /* Raid 0 configurations.
     */
    {                                                  /*! @note DO NOT SET BANDWIDTH THE TEST WILL FAIL !-----v*/
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,         0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,         0},
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            2,         0},
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
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            2,         0},
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
    /* Raid 10 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           0,          0},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           1,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,    520,           2,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end headless_horseman_raid_group_config_qual()
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_raid_group_config_vault
 *********************************************************************
 * @brief   This is the array config set for testing the vault.
 *
 * @note    Currently only the vault LUN will be encrypted (because it
 *          contains user data).
 *
 * @note    The capacity of the LUN is huge.
 *
 *********************************************************************/
fbe_test_rg_configuration_t headless_horseman_raid_group_config_vault[] = 
{
    /*! @note Although all fields must be valid only the system id is used to populate the test config+
     *                                                                                                |
     *                                                                                                v    */
    /*width capacity    raid type                   class               block size  system id                               bandwidth.*/
    {5,     0xF004800,  FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY, 520,       FBE_PRIVATE_SPACE_LAYOUT_RG_ID_VAULT,   0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var headless_horseman_raid_group_config_large
 *********************************************************************
 * @brief   This is the array config set for testing with a large capacity rg.
 * 
 *********************************************************************/
fbe_test_rg_configuration_array_t headless_horseman_raid_group_config_large[] = 
{
    {
        /*width capacity                                        raid type                   class    block size system id
          bandwidth. */
        {3,     HEADLESS_HORSEMAN_BLOCKS_PER_LARGE_RG,  FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY, 520,       0,   0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var headless_horseman_raid_group_config_r10_large
 *********************************************************************
 * @brief   This is the array config set for testing with a large capacity rg.
 * 
 *********************************************************************/
fbe_test_rg_configuration_array_t headless_horseman_raid_group_config_r10_large[] = 
{
    {
        /*width capacity                                        raid type                   class    block size system id
          bandwidth. */
        {4,     HEADLESS_BLOCKS_PER_LARGE_R10_RG * 2,  FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520,       0,   0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*!*************************************************
 * @enum fbe_headless_horseman_error_location_t
 ***************************************************
 * @brief This defines where we will inject the
 *        error on the raid group.
 *
 ***************************************************/
typedef enum fbe_headless_horseman_error_location_e
{
    HEADLESS_HORSEMAN_ERROR_LOCATION_INVALID = 0,
    HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA, /*!< Only inject on user area. */
    HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA_CHUNK_1, /*!< Only inject on user area. */
    HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA, /*< Only inject on paged metadata. */
    HEADLESS_HORSEMAN_ERROR_LOCATION_RAID, /*< Inject on all raid protected areas both user and paged metadata. */
    HEADLESS_HORSEMAN_ERROR_LOCATION_WRITE_LOG,   /* Inject on write log area. */
    HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA, /* Inject an error in the provision drive metadata*/
    HEADLESS_HORSEMAN_ERROR_LOCATION_LAST,
}
fbe_headless_horseman_error_location_t;

/*!*************************************************
 * @struct fbe_headless_horseman_drive_error_definition_t
 ***************************************************
 * @brief Single drive's error definition.
 *
 ***************************************************/
typedef struct fbe_headless_horseman_drive_error_definition_s
{
    fbe_test_protocol_error_type_t error_type;  /*!< Class of error (retryable/non/media/soft) */
    fbe_payload_block_operation_opcode_t command; /*!< Command to inject on. */
    fbe_u32_t times_to_insert_error;
    fbe_u32_t rg_position_to_inject;
    fbe_bool_t b_does_drive_die_on_error; /*! FBE_TRUE if the drive will die. */
    fbe_headless_horseman_error_location_t location; /*!< Area to inject the error. */
    fbe_u8_t tag; /*!< this tag value is significant only when b_inc_for_all_drives is true. error counts are incremented for all the records having similar tag value. */
}
fbe_headless_horseman_drive_error_definition_t;

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS
 *********************************************************************
 * @brief Max number of errors per error case.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS 10

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS
 *********************************************************************
 * @brief Max number of positions to degrade before starting the test.
 *
 * @note  To support striped mirrors this is the max array with / 2
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS 8

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_TEST_WIDTHS
 *********************************************************************
 * @brief Max number of widths we can specify to test.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_TEST_WIDTHS 4

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_ERROR_CASES
 *********************************************************************
 * @brief Max number of possible cases to validate against.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_ERROR_CASES 4

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_POSSIBLE_STATUS
 *********************************************************************
 * @brief Max number of possible different I/O status for rdgen request
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_POSSIBLE_STATUS  2
#define HEADLESS_HORSEMAN_INVALID_STATUS       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_NUM_DIRECT_IO_ERRORS
 *********************************************************************
 * @brief Number of errors to insert into direct I/O execution (i.e.
 *        error that are inject but not into the raid library state
 *        machine).
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_NUM_DIRECT_IO_ERRORS                 1

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_NUM_READ_AND_SINGLE_VERIFY_ERRORS
 *********************************************************************
 * @brief Number of error to inject that accomplishes the following:
 *          o Inject error assuming that direct I/o encounters the first
 *          o Inject error assuming read state machine encounters the second
 *          o Inject error assuming that verify state machine encounters the third
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_NUM_READ_AND_SINGLE_VERIFY_ERRORS    (HEADLESS_HORSEMAN_NUM_DIRECT_IO_ERRORS + 2)

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE
 *********************************************************************
 * @brief Number of retryable errors to cause a drive to fail.
 *
 * @note  This assumes that we are in the recovery verify state machine.
 *        This number utimately translates to the number of errors DIEH 
 *        detects before issuing a drive reset, which then causes PDO 
 *        edge to go disabled.  Drive does not actually fail.
 * 
 *********************************************************************/
#define HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE FBE_TEST_NUM_RETRYABLE_TO_FAIL_DRIVE

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2
 *********************************************************************
 * @brief Number of accumulated retryable errors to cause two
 *        drives in same RG to fail.
 * 
 *********************************************************************/
#define HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE_TIMES_2 (HEADLESS_HORSEMAN_NUM_RETRYABLE_TO_FAIL_DRIVE*2)


typedef fbe_status_t (*headless_test_function_t)(fbe_test_rg_configuration_t *rg_config_p,
                                         void *error_case_p);
/*!*************************************************
 * @typedef fbe_headless_horseman_error_case_t
 ***************************************************
 * @brief Single test case with the error and
 *        the blocks to read for.
 *
 ***************************************************/
typedef struct fbe_headless_horseman_error_case_s
{
    fbe_u8_t *description_p;            /*!< Description of test case. */
    fbe_u32_t line;                     /*!< Line number of test case. */
    fbe_bool_t b_inc_for_all_drives; /*!< TRUE */
    headless_test_function_t function;

    fbe_chunk_count_t error_chunk_offset; /*! Number of chunks to offset the error insertion. */
    fbe_headless_horseman_drive_error_definition_t errors[HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_u32_t degraded_positions[HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS];
    fbe_u32_t widths[HEADLESS_HORSEMAN_MAX_TEST_WIDTHS]; /*! Widths to test with. FBE_U32_MAX if invalid*/
    fbe_notification_type_t wait_for_state_mask; /*!< Wait for the rg to get to this state. */
    fbe_u32_t substate; /*!< Substate to wait for. */
}
fbe_headless_horseman_error_case_t;

/*!***************************************************************************
 * @typedef fbe_headless_horseman_provision_drive_default_background_operations_t
 *****************************************************************************
 * @brief   Global setting of default background operations.
 *
 ***************************************************/
typedef struct fbe_headless_horseman_provision_drive_default_background_operations_s
{
    fbe_bool_t  b_is_initialized;
    fbe_bool_t  b_is_background_zeroing_enabled;
    fbe_bool_t  b_is_sniff_verify_enabled;  
    /*! @note As more background operations are added add flag here.
     */
}
fbe_headless_horseman_provision_drive_default_background_operations_t;

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_STRING_LENGTH
 *********************************************************************
 * @brief Max size of a string in a headless_horseman structure.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_STRING_LENGTH 500

static fbe_status_t headless_horseman_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                              fbe_u32_t *position_p);
static fbe_bool_t headless_horseman_get_and_check_degraded_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                         fbe_headless_horseman_error_case_t *error_case_p,
                                                         fbe_u32_t  degraded_drive_index,
                                                         fbe_u32_t *degraded_position_p);
static fbe_status_t headless_horseman_get_pdo_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
                                             fbe_object_id_t *pdo_object_id_p,
                                             fbe_u32_t error_number);
static fbe_status_t headless_horseman_get_pvd_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
                                             fbe_object_id_t *pvd_object_id_p,
                                             fbe_u32_t error_number);
void headless_horseman_setup(void);
fbe_bool_t headless_horseman_is_rg_valid_for_error_case(fbe_headless_horseman_error_case_t *error_case_p,
                                             fbe_test_rg_configuration_t *rg_config_p);
static fbe_status_t headless_horseman_raid0_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                void *case_p);
static fbe_status_t headless_horseman_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                void * case_p);
static fbe_status_t headless_horseman_provision_drive_paged_error_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                     void *case_p);
static fbe_status_t headless_horseman_raid0_paged_error_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                            void *case_p);
static fbe_status_t headless_horseman_paged_error_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                            void *case_p);
static fbe_status_t headless_horseman_shutdown_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                         void *case_p);
static fbe_status_t headless_horseman_incomplete_write_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                 void *case_p);
static fbe_status_t headless_horseman_dualsp_incomplete_write_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                        void *case_p);
static fbe_status_t headless_horseman_raid0_dualsp_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                          void *case_p);
static fbe_status_t headless_horseman_dualsp_iw_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                          void *case_p);
static fbe_status_t headless_horseman_paged_vault_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                            void *case_p);
static fbe_status_t headless_horseman_dualsp_paged_iw_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                void *case_p);

static fbe_status_t headless_horseman_dualsp_pvd_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                           void *case_p);
static fbe_status_t headless_horseman_dualsp_reboot_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                              void *case_p);
static fbe_status_t headless_horseman_large_rg_reboot_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                void *case_p);
static fbe_status_t headless_horseman_dualsp_reboot_both_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                   void *case_p);
static fbe_status_t headless_horseman_dualsp_degraded_iw_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                   void *case_p);
static fbe_status_t headless_horseman_dualsp_peer_lost_rekey_error_part1(fbe_test_rg_configuration_t *rg_config_p,
                                                                         void *case_p);
static fbe_status_t headless_horseman_dualsp_peer_lost_rekey_error_part2(fbe_test_rg_configuration_t *rg_config_p,
                                                                         void *case_p);
/*!*******************************************************************
 * @var headless_horseman_test_cases
 *********************************************************************
 * @brief These are the standard test cases
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_test_cases[] =
{
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Error on rekey paged write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_paged_error_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on read.
         */
        "non-retryable Error on rekey read", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on read.
         */
        "non-retryable Error on rekey write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on read.
         */
        "non-retryable errors on rekey read causes RG shutdown", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_shutdown_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Three retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 2,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                FBE_RAID_MAX_DISK_ARRAY_WIDTH - 3,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject retryable on write to provision drive paged metadata
         */
        "retryable Error on rekey paged write to provision drive retry succeeds", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_provision_drive_paged_error_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One retryable error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
    },
    {
        /* Inject non-retryable on write-verify to provision drive paged metadata
         */
        "non-retryable Error on rekey paged write to provision drive enter Activate and retry", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_provision_drive_paged_error_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* wait for pvd to enter activate  */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_test_cases
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_raid0_test_cases
 *********************************************************************
 * @brief Brief table of raid 0 test cases.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_raid0_test_cases[] =
{
    {
        "non-retryable Error on rekey paged write causes shutdown\n"
        "reinserting the drive results in page being reconstructed", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_raid0_paged_error_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Insert non-retryable errors until drive exists ready.
             * (without setting DRIVE_FAULT)
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        "non-retryable Error on rekey read", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_raid0_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        "non-retryable Error on rekey write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_raid0_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on read chunk
         */
        "non-retryable Errors on rekey read chunk 3", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_raid0_test_case,
        3, /* Chunks to offset error insertion. */
        {
            /* Five non-retryable errors
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes shutdown write last chunk", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_raid0_test_case,
        HEADLESS_HORSEMAN_CHUNKS_PER_LUN - 1, /* Chunks to offset error insertion. */
        {
            /* Five non-retryable errors
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/******************************************
 * end headless_horseman_raid0_test_cases
 ******************************************/

/*!*******************************************************************
 * @var headless_horseman_mirror_test_cases
 *********************************************************************
 * @brief These are the standard test cases
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_mirror_test_cases[] =
{
    {
        "non-retryable Error on rekey paged write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_paged_error_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
    },
    {
        "non-retryable Error on rekey read", __LINE__,
        FBE_TRUE, /* inc for all drives */
        headless_horseman_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },

    {
        "non-retryable Error on rekey write", __LINE__,
        FBE_TRUE, /* inc for all drives */
        headless_horseman_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        "non-retryable errors on rekey read causes RG shutdown", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_shutdown_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Three retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, 4, FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (2 drive RAID-1)\n"
        "chunk 0", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, 4, FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (2 drive RAID-1)\n"
        "chunk 6", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        5, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, 4, FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (2 drive RAID-1)\n"
        "last chunk", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        HEADLESS_HORSEMAN_CHUNKS_PER_LUN - 1, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 2, 4, FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (3 drive RAID-1)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                2,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { 3, FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_mirror_test_cases
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_incomplete_write_parity_test_cases
 *********************************************************************
 * @brief These are the incomplete write tests.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_incomplete_write_parity_test_cases[] =
{
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (chunk 0)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },

    {
        /* Inject non-retryable on write.  Third chunk.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (chunk 2)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        2, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.  Last chunk.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (last chunk)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        HEADLESS_HORSEMAN_CHUNKS_PER_LUN - 1, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_incomplete_write_parity_test_cases
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_raid6_test_cases
 *********************************************************************
 * @brief These are the incomplete write tests.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_raid6_test_cases[] =
{
    {
        /* Inject non-retryable on write.
         */
        "non-retryable errors on rekey write cause rg shutdown with incomplete write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_incomplete_write_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                2,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_raid6_test_cases
 **************************************/
/*!*******************************************************************
 * @var headless_horseman_test_cases_parity_only
 *********************************************************************
 * @brief These are the standard test cases
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_test_cases_parity_only[] =
{
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Error on rekey journal write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_paged_error_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_WRITE_LOG,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_test_cases_parity_only
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_raid0_test_cases
 *********************************************************************
 * @brief Brief table of raid 0 dualsp test cases.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_raid0_dualsp_test_cases[] =
{
   {
        /* Inject incomplete write on paged.
         */
        "rekey paged write for RG is incomplete and SP fails.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_paged_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Dropped errors so that we can crash this SP with the I/O outstanding.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
#if 0
    {
        "non-retryable Error on rekey read", __LINE__,
        FBE_TRUE, /* inc for all drives */
        headless_horseman_raid0_dualsp_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
#endif
    {
        "non-retryable Error on rekey write", __LINE__,
        FBE_TRUE, /* inc for all drives */
        headless_horseman_raid0_dualsp_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/***********************************************
 * end headless_horseman_raid0_dualsp_test_cases
 ***********************************************/

/*!*******************************************************************
 * @var headless_horseman_mirror_dualsp_test_cases
 *********************************************************************
 * @brief These are the standard test cases
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_mirror_dualsp_test_cases[] =
{
    {
        /* Inject non-retryable on write-verify to provision drive paged metadata
         */
        "non-retryable Error on rekey paged write to provision drive and SP is crashed.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_pvd_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
    },
    {
        /* Inject incomplete write on paged.
         */
        "rekey paged write for RG is incomplete and SP fails.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_paged_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Dropped errors so that we can crash this SP with the I/O outstanding.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (2 drive RAID-1)\n"
        "chunk 0", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (2 drive RAID-1)\n"
        "chunk 6", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        5, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write (2 drive RAID-1)\n"
        "last chunk", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        HEADLESS_HORSEMAN_CHUNKS_PER_LUN - 1, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_mirror_dualsp_test_cases
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_parity_dualsp_test_cases
 *********************************************************************
 * @brief These are the incomplete write tests.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_parity_dualsp_test_cases[] =
{
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write.  Peer takes over degraded.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_degraded_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write-verify to provision drive paged metadata
         */
        "non-retryable Error on rekey paged write to provision drive and SP is crashed.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_pvd_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
    },
    {
        /* Inject incomplete write on paged.
         */
        "rekey paged write for RG is incomplete and SP fails.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_paged_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Dropped errors so that we can crash this SP with the I/O outstanding.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (chunk 0)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },

    {
        /* Inject non-retryable on write.  Third chunk.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (chunk 2)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        2, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.  Last chunk.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (last chunk)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        HEADLESS_HORSEMAN_CHUNKS_PER_LUN - 1, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_parity_dualsp_test_cases
 **************************************/
/*!*******************************************************************
 * @var headless_horseman_raid6_dualsp_test_cases
 *********************************************************************
 * @brief These are the incomplete write tests.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_raid6_dualsp_test_cases[] =
{
    {
        /* Inject non-retryable on write-verify to provision drive paged metadata
         */
        "non-retryable Error on rekey paged write to provision drive and SP is crashed.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_pvd_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                1,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
    },
    {
        /* Inject incomplete write on paged.
         */
        "rekey paged write for RG is incomplete and SP fails.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_paged_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Dropped errors so that we can crash this SP with the I/O outstanding.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (chunk 0)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },

    {
        /* Inject non-retryable on write.  Third chunk.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (chunk 2)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        2, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    {
        /* Inject non-retryable on write.  Last chunk.
         */
        "non-retryable Errors on rekey write causes incomplete write and RG shutdown (last chunk)", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_iw_test_case,
        HEADLESS_HORSEMAN_CHUNKS_PER_LUN - 1, /* Chunks to offset error insertion. */
        {
            /* Dropped errors so we can reboot the SP.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_DROP,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                1,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_raid6_dualsp_test_cases
 **************************************/

/*!*******************************************************************
 * @var headless_horseman_vault_test_cases
 *********************************************************************
 * @brief These are the vault related test cases
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_vault_test_cases[] =
{
    {
        /* Inject non-retryable on write.
         */
        "non-retryable Error on rekey paged write", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_paged_vault_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Two retryable errors.
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_vault_test_cases
 **************************************/
/*!*******************************************************************
 * @var headless_horseman_dualsp_reboot_test_cases
 *********************************************************************
 * @brief These are test cases where we reboot one SP during encryption.
 *        We bring the SP back when encryption is completing
 *        and make sure it syncronizes properly.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_dualsp_reboot_test_cases[] =
{
    {
        "Reboot SP during encryption, bring it back when encryption is completing just before setting rekey complete..", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_reboot_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
        FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_SET_REKEY_COMPLETE, /* Substate to wait for. */
    },
    {
        "Reboot SP during encryption, bring it back when encryption is completing just before setting ENCRYPTED.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_reboot_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
        FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING, /* Substate to wait for. */
    },
    {
        "Reboot SP during encryption, bring it back when encryption is completing at remove old keys phase.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_reboot_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
        FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY, /* Substate to wait for. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_dualsp_reboot
 **************************************/
/*!*******************************************************************
 * @var headless_horseman_dualsp_reboot_rekey_error_test_cases
 *********************************************************************
 * @brief These are test cases where we reboot one SP during encryption.
 *        We bring the SP back when encryption is completing and inject
 *        and error.  In some cases data will be lost.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_dualsp_reboot_rekey_error_test_cases[] =
{
    {
        /* Delay rekey write and shutdown active SP.
         */
        "Delay rekey write and shutdown active SP", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_peer_lost_rekey_error_part1,
        1, /* Chunks to offset error insertion. */
        {
            /* Insert many retryable errors to delay I/O
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                100,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_FALSE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
        FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAUSED,
    },
    {
        /* Inject dead error during incomplete write invalidate
         */
        "Inject dead error during IW invalidate after peer lost", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_peer_lost_rekey_error_part2,
        1, /* Chunks to offset error insertion. */
        {
            /* One non-retryable error (drive dies)
             */
            {
                FBE_TEST_PROTOCOL_ERROR_TYPE_NON_RETRYABLE,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,    /* opcode to inject on */
                5,    /* times to insert error. */
                0,    /* raid group position to inject.*/
                FBE_TRUE,    /* Does drive die as a result of this error? */
                HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA,
                0, /* tag value only significant in case b_inc_for_all_drives is true*/
            },
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************************************
 * end headless_horseman_dualsp_reboot_rekey_error_test_cases()
 **************************************************************/

/*!*******************************************************************
 * @var headless_horseman_dualsp_r10_test_cases
 *********************************************************************
 * @brief These are test cases where we reboot one SP during encryption.
 *        We bring the SP back when encryption is completing
 *        and make sure it syncronizes properly.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_dualsp_r10_test_cases[] =
{
    {
        "Reboot both SPs during encryption.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_dualsp_reboot_both_test_case,
        0, /* Chunks to offset error insertion. */
        {
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* wait for pvd to enter activate  */
        FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_SET_REKEY_COMPLETE, /* Substate to wait for. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};
/**************************************
 * end headless_horseman_vault_test_cases
 **************************************/
/*!*******************************************************************
 * @var headless_horseman_large_rg_dualsp_test_cases
 *********************************************************************
 * @brief Reboot tests cases.
 *
 *********************************************************************/
fbe_headless_horseman_error_case_t headless_horseman_large_rg_dualsp_test_cases[] =
{
    {
        /* Inject incomplete write on paged.
         */
        "Test to make sure checkpoint is persisted.", __LINE__,
        FBE_FALSE, /* inc for all drives */
        headless_horseman_large_rg_reboot_test_case,
        0, /* Chunks to offset error insertion. */
/* No errors */
        {
            /* Terminator */
            { FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, 0, 0, 0 },
        },
        { FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX }, /* No degraded positions. */
        { FBE_U32_MAX}, /* Test widths. */
        FBE_NOTIFICATION_TYPE_INVALID, /* Wait for rg to get to this lifecycle state. */
    },
    /* terminator
     */
    {
        0,0,0,0,0,0,0,
    },
};

/*!*******************************************************************
 * @def HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE
 *********************************************************************
 * @brief Max number of error tables for each raid type.
 *
 *********************************************************************/
#define HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE 10

/*!*******************************************************************
 * @var headless_horseman_qual_error_cases
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
fbe_headless_horseman_error_case_t *headless_horseman_qual_error_cases[HEADLESS_HORSEMAN_RAID_TYPE_COUNT][HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 0 */
    {
        &headless_horseman_raid0_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 1 */
    {
        &headless_horseman_mirror_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 5 */
    {
        &headless_horseman_test_cases[0],
        &headless_horseman_test_cases_parity_only[0],
        &headless_horseman_incomplete_write_parity_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        &headless_horseman_test_cases[0],
        &headless_horseman_test_cases_parity_only[0],
        &headless_horseman_incomplete_write_parity_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 6 */
    {
        &headless_horseman_test_cases[0],
        &headless_horseman_test_cases_parity_only[0],
        &headless_horseman_raid6_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 10 - Striped mirror specific errors*/
    {
        &headless_horseman_mirror_test_cases[0],
        NULL /* Terminator */
    },
};

/*!*******************************************************************
 * @var headless_horseman_vault_error_cases
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
fbe_headless_horseman_error_case_t *headless_horseman_vault_error_cases[HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 3 */
    &headless_horseman_vault_test_cases[0],
    NULL /* Terminator */
};

/*!*******************************************************************
 * @var headless_horseman_dualsp_error_cases
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
fbe_headless_horseman_error_case_t *headless_horseman_dualsp_error_cases[HEADLESS_HORSEMAN_RAID_TYPE_COUNT][HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 0 */
    {
        &headless_horseman_raid0_dualsp_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 1 */
    {
        &headless_horseman_mirror_dualsp_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 5 */
    {
        &headless_horseman_parity_dualsp_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        &headless_horseman_parity_dualsp_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 6 */
    {
        &headless_horseman_raid6_dualsp_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 10 - Striped mirror specific errors*/
    {
        &headless_horseman_mirror_dualsp_test_cases[0],
        NULL /* Terminator */
    },
};

/*!*******************************************************************
 * @var headless_horseman_dualsp_reboot_error_cases
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
fbe_headless_horseman_error_case_t *headless_horseman_dualsp_reboot_error_cases[HEADLESS_HORSEMAN_RAID_TYPE_COUNT][HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE] =
{
    /* Raid 0 */
    {
        &headless_horseman_dualsp_reboot_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 1 */
    {
        &headless_horseman_dualsp_reboot_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 5 */
    {
        &headless_horseman_parity_dualsp_test_cases[0],
        /*! @todo Re-enable this test case when completely debugged.*/
        //&headless_horseman_dualsp_reboot_rekey_error_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 3 */
    {
        &headless_horseman_dualsp_reboot_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 6 */
    {
        &headless_horseman_dualsp_reboot_test_cases[0],
        NULL /* Terminator */
    },
    /* Raid 10 - Striped mirror specific errors*/
    {
        &headless_horseman_dualsp_reboot_test_cases[0],
        NULL /* Terminator */
    },
};
/*!*******************************************************************
 * @var headless_horseman_dualsp_large_rg_error_cases
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
fbe_headless_horseman_error_case_t *headless_horseman_dualsp_large_rg_error_cases[HEADLESS_HORSEMAN_RAID_TYPE_COUNT][HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE] =
{
    {
        /* Raid 5 */
        &headless_horseman_large_rg_dualsp_test_cases[0],
        NULL /* Terminator */
    },
};

/*!*******************************************************************
 * @var headless_horseman_dualsp_large_r10_error_cases
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
fbe_headless_horseman_error_case_t *headless_horseman_dualsp_large_r10_error_cases[HEADLESS_HORSEMAN_RAID_TYPE_COUNT][HEADLESS_HORSEMAN_MAX_TABLES_PER_RAID_TYPE] =
{
    {
        &headless_horseman_dualsp_r10_test_cases[0],
        NULL /* Terminator */
    },
};

/*!***************************************************************************
 * @var     headless_horseman_provision_drive_default_operations
 *****************************************************************************
 * @brief   This simply records the default value for the provision drive
 *          background operations.
 *
 *********************************************************************/
fbe_headless_horseman_provision_drive_default_background_operations_t headless_horseman_provision_drive_default_operations = { 0 };


/*!**************************************************************
 * headless_horseman_get_error_count()
 ****************************************************************
 * @brief
 *  Get the number of raid_groups in the array.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
fbe_u32_t headless_horseman_get_error_count(fbe_headless_horseman_error_case_t *error_case_p)
{
    fbe_u32_t num_cases = 0;
    fbe_headless_horseman_drive_error_definition_t *errors_p = &error_case_p->errors[0];
    /* Loop until we find the table number or we hit the terminator.
     */
    while (errors_p->error_type != FBE_TEST_PROTOCOL_ERROR_TYPE_INVALID)
    {
        num_cases++;
        errors_p++;
    }
    return num_cases;
}
/**************************************
 * end headless_horseman_get_error_count()
 **************************************/
fbe_bool_t headless_horseman_is_write_error_case(fbe_headless_horseman_error_case_t *error_case_p,
                                                fbe_u32_t error_index)
{
    fbe_u32_t error_count;

    error_count = headless_horseman_get_error_count(error_case_p);
    MUT_ASSERT_TRUE_MSG(error_index < error_count, "invalid error_index");
    if (error_case_p->errors[error_index].command == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
fbe_status_t headless_horseman_send_seed_io(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_headless_horseman_error_case_t *error_case_p,
                                            fbe_api_rdgen_context_t *context_p,
                                            fbe_rdgen_operation_t rdgen_operation,
                                            fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_block_count_t elsz;
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_api_raid_group_get_info_t rg_info;
    elsz = fbe_test_sep_util_get_element_size(rg_config_p);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    
    params.rdgen_operation = rdgen_operation;
    params.pattern = pattern;
    if (error_case_p) {
        params.lba = 0 + (error_case_p->error_chunk_offset * FBE_RAID_DEFAULT_CHUNK_SIZE * rg_info.num_data_disk);
    } else {
        params.lba = 0 + (FBE_RAID_DEFAULT_CHUNK_SIZE * rg_info.num_data_disk);
    }
    params.blocks = FBE_RAID_DEFAULT_CHUNK_SIZE * rg_info.num_data_disk; /* Clean up at least a full ch stripe. */

    if (pattern == FBE_RDGEN_PATTERN_INVALIDATED) {
        params.options |= FBE_RDGEN_OPTIONS_DO_NOT_CHECK_INVALIDATION_REASON;
        params.options |= FBE_RDGEN_OPTIONS_PERFORMANCE; /* Don't lock. */
        params.options |= FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC; /* No errors on reads. */
        params.options |= FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR; /* No errors on reads. */

        /* To read just one position we read each element on that position.
         */
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
            params.blocks = rg_info.element_size;
            params.lba_spec = FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING;
            /* We want to check the first position, so just inc by a full stripe.
             */
            params.inc_lba = rg_info.sectors_per_stripe;
            params.end_lba = (error_case_p->error_chunk_offset + 1) * FBE_RAID_DEFAULT_CHUNK_SIZE * rg_info.num_data_disk;
        }
    }
    params.b_async = FBE_FALSE; /* sync/wait*/
    mut_printf(MUT_LOG_TEST_STATUS, "send rdgen op 0x%x (object 0x%x) lba 0x%llx bl 0x%llx",
               rdgen_operation, lun_object_id, params.lba, params.blocks);
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);

    if (pattern != FBE_RDGEN_PATTERN_INVALIDATED) {
        /* We do not expect any errors for non invalidated patterns.
         */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else {
        /* For invalidated patterns we might see media errors.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "errs: %d hme: %d io_err: %d abort_err: %d",
                   context_p->start_io.statistics.error_count,
                   context_p->start_io.statistics.media_error_count, 
                   context_p->start_io.statistics.io_failure_error_count,
                   context_p->start_io.statistics.aborted_error_count);
        if (context_p->start_io.statistics.error_count != context_p->start_io.statistics.media_error_count) {
            MUT_FAIL_MSG("Unexpected errors encountered");
        }
    }
    return status;
}

fbe_status_t headless_horseman_send_pin_io(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_headless_horseman_error_case_t *error_case_p,
                                           fbe_api_rdgen_context_t *context_p,
                                           fbe_rdgen_operation_t rdgen_operation,
                                           fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_block_count_t elsz;
    fbe_api_rdgen_send_one_io_params_t params;
    fbe_api_raid_group_get_info_t rg_info;
    elsz = fbe_test_sep_util_get_element_size(rg_config_p);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    fbe_api_rdgen_io_params_init(&params);
    params.object_id = lun_object_id;
    params.class_id = FBE_CLASS_ID_INVALID;
    params.package_id = FBE_PACKAGE_ID_SEP_0;
    params.msecs_to_abort = 0;
    params.msecs_to_expiration = 0;
    params.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    
    params.rdgen_operation = rdgen_operation;
    params.pattern = pattern;
    params.lba = 0 + error_case_p->error_chunk_offset * FBE_RAID_DEFAULT_CHUNK_SIZE * rg_info.num_data_disk;
    params.blocks = FBE_RAID_DEFAULT_CHUNK_SIZE * rg_info.num_data_disk; /* Clean up at least a full ch stripe. */

    params.b_async = FBE_TRUE; /* Don't wait, we want the pin to be outstanding. */
    mut_printf(MUT_LOG_TEST_STATUS, "send rdgen op 0x%x (object 0x%x) lba 0x%llx bl 0x%llx",
               rdgen_operation, lun_object_id, params.lba, params.blocks);
    status = fbe_api_rdgen_send_one_io_params(context_p, &params);

    return status;
}
static fbe_status_t headless_horseman_wait_for_pin_io(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t timeout_msec)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;
    fbe_u32_t total_time_ms = 0;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    fbe_api_rdgen_filter_init(&filter, FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID, 0 /* edge index */);

    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    while (rdgen_stats.io_count < num_raid_groups) {
        if (total_time_ms > timeout_msec) {
            MUT_FAIL_MSG("pins not competed in expected time");
        }

        fbe_api_sleep(200);
        total_time_ms += 200;
        if ((total_time_ms % 1000) == 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "wait for all rg pins to complete pins: %d num_raid_groups: %d",
                       rdgen_stats.io_count, num_raid_groups);
        }
        status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_wait_for_rebuilds(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
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
                                                         HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for drives that failed due to injected errors.
         */
        for ( drive_index = 0; drive_index < headless_horseman_get_error_count(error_case_p); drive_index++)
        {
            degraded_position = error_case_p->errors[drive_index].rg_position_to_inject;
            headless_horseman_get_drive_position(rg_config_p, &degraded_position);
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, degraded_position);
        }

        /* Wait for drives that failed due to us degrading the raid group on purpose.
         */
        for ( drive_index = 0; drive_index < HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS; drive_index++)
        {
            if (headless_horseman_get_and_check_degraded_position(rg_config_p,
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
static fbe_status_t headless_horseman_get_verify_report(fbe_test_rg_configuration_t *rg_config_p,
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
static fbe_status_t headless_horseman_wait_for_verify(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_headless_horseman_error_case_t *error_case_p,
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
static fbe_status_t headless_horseman_wait_for_verifies(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
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
            headless_horseman_wait_for_verify(rg_config_p, error_case_p, HEADLESS_HORSEMAN_WAIT_MSEC);
        }
        rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for verifies to finish. (successful)==", __FUNCTION__);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_wait_for_degraded(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
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
    for ( drive_index = 0; drive_index < HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS; drive_index++)
    {
        if (headless_horseman_get_and_check_degraded_position(rg_config_p,
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
static fbe_status_t headless_horseman_clear_drive_error_counts(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_headless_horseman_error_case_t *error_case_p,
                                                    fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t index;
    for ( index = 0; index < raid_group_count; index++)
    {
        if (fbe_test_rg_config_is_enabled(rg_config_p))
        {
            fbe_object_id_t object_id;
            fbe_u32_t drive_index;
            for ( drive_index = 0; drive_index < headless_horseman_get_error_count(error_case_p); drive_index++)
            {
                fbe_u32_t position = error_case_p->errors[drive_index].rg_position_to_inject;
                headless_horseman_get_drive_position(rg_config_p, &position);
                headless_horseman_get_pdo_object_id(rg_config_p, error_case_p, &object_id, drive_index);
                status = fbe_api_physical_drive_clear_error_counts(object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
        }
        rg_config_p++;
    }
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_inject_scsi_error (fbe_protocol_error_injection_error_record_t *record_in_p,
                                              fbe_protocol_error_injection_record_handle_t *record_handle_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    
    /* convert 520 error to 4k if necessary*/
    status = fbe_test_neit_utils_convert_to_physical_drive_range(record_in_p->object_id,
                                                                 record_in_p->lba_start,
                                                                 record_in_p->lba_end,
                                                                 &record_in_p->lba_start,
                                                                 &record_in_p->lba_end);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*set the error using NEIT package. */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: obj: 0x%x start: 0x%llx end: 0x%llx SK:%d ASC:%d.", 
               __FUNCTION__, record_in_p->object_id, record_in_p->lba_start, record_in_p->lba_end,
               record_in_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key, 
               record_in_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code);

    status = fbe_api_protocol_error_injection_add_record(record_in_p, record_handle_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}
static fbe_status_t headless_horseman_get_drive_position(fbe_test_rg_configuration_t *raid_group_config_p,
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
static fbe_bool_t headless_horseman_get_and_check_degraded_position(fbe_test_rg_configuration_t *raid_group_config_p,
                                                         fbe_headless_horseman_error_case_t *error_case_p,
                                                         fbe_u32_t  degraded_drive_index,
                                                         fbe_u32_t *degraded_position_p)
{
    fbe_bool_t  b_position_degraded = FBE_FALSE;
    fbe_u32_t   max_num_degraded_positions = 0;

    /* First get the degraded position value from the error table
     */
    MUT_ASSERT_TRUE(degraded_drive_index < HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS);
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
        headless_horseman_get_drive_position(raid_group_config_p, degraded_position_p);
        b_position_degraded = FBE_TRUE;
    }
    return(b_position_degraded);
}
static fbe_status_t headless_horseman_get_port_enclosure_slot(fbe_test_rg_configuration_t *raid_group_config_p,
                                                   fbe_headless_horseman_error_case_t *error_case_p,
                                                   fbe_u32_t error_number,
                                                   fbe_u32_t *port_number_p,
                                                   fbe_u32_t *encl_number_p,
                                                   fbe_u32_t *slot_number_p )
{
    fbe_u32_t position_to_inject = error_case_p->errors[error_number].rg_position_to_inject;
    headless_horseman_get_drive_position(raid_group_config_p, &position_to_inject);
    *port_number_p = raid_group_config_p->rg_disk_set[position_to_inject].bus;
    *encl_number_p = raid_group_config_p->rg_disk_set[position_to_inject].enclosure;
    *slot_number_p = raid_group_config_p->rg_disk_set[position_to_inject].slot;
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_get_pdo_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
                                             fbe_object_id_t *pdo_object_id_p,
                                             fbe_u32_t error_number)
{
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    status = headless_horseman_get_port_enclosure_slot(raid_group_config_p, error_case_p, error_number, /* Error case. */
                                            &port_number, &encl_number, &slot_number);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_physical_drive_object_id_by_location(port_number, encl_number, slot_number, pdo_object_id_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(*pdo_object_id_p, FBE_OBJECT_ID_INVALID);
    return status;
}
static fbe_status_t headless_horseman_get_pvd_object_id(fbe_test_rg_configuration_t *raid_group_config_p,
                                             fbe_headless_horseman_error_case_t *error_case_p,
                                             fbe_object_id_t *pvd_object_id_p,
                                             fbe_u32_t error_number)
{
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    status = headless_horseman_get_port_enclosure_slot(raid_group_config_p, error_case_p, error_number, /* Error case. */
                                            &port_number, &encl_number, &slot_number);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(port_number, encl_number, slot_number, pvd_object_id_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(*pvd_object_id_p, FBE_OBJECT_ID_INVALID);
    return status;
}
fbe_sata_command_t headless_horseman_get_sata_command(fbe_payload_block_operation_opcode_t opcode)
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

static void headless_horseman_get_sata_error(fbe_protocol_error_injection_sata_error_t *sata_error_p,
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
static void headless_horseman_fill_error_record(fbe_test_rg_configuration_t *raid_group_config_p,
                                     fbe_headless_horseman_error_case_t *error_case_p,
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
    }
    else
    {
        record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    }
    if (error_case_p->errors[drive_index].error_type == FBE_TEST_PROTOCOL_ERROR_TYPE_DROP) {
        record_p->b_drop = FBE_TRUE;
    }
    record_p->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = 
    fbe_test_sep_util_get_scsi_command(error_case_p->errors[drive_index].command);
    fbe_test_sep_util_set_scsi_error_from_error_type(&record_p->protocol_error_injection_error.protocol_error_injection_scsi_error,
                                                     error_case_p->errors[drive_index].error_type);
#if 0
        if (raid_group_config_p->block_size == 512)
        {
            record_p->protocol_error_injection_error_type = FBE_TEST_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS;
            record_p->protocol_error_injection_error.protocol_error_injection_sata_error.sata_command = 
                headless_horseman_get_sata_command(error_case_p->errors[drive_index].command);
            headless_horseman_get_sata_error(&record_p->protocol_error_injection_error.protocol_error_injection_sata_error,
                                  error_case_p->errors[drive_index].error_type);
        }
#endif
    return;
}

static void headless_horseman_disable_provision_drive_background_operations(fbe_u32_t bus,
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
    if (headless_horseman_provision_drive_default_operations.b_is_initialized == FBE_FALSE)
    {
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_background_zeroing_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_background_zeroing_enabled == FBE_TRUE)
        {
            headless_horseman_provision_drive_default_operations.b_is_background_zeroing_enabled = FBE_TRUE;
        }
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_background_sniff_enabled);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_is_background_sniff_enabled == FBE_TRUE)
        {
            headless_horseman_provision_drive_default_operations.b_is_sniff_verify_enabled = FBE_TRUE;
        }
        FBE_ASSERT_AT_COMPILE_TIME((FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING | FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF) ==
                                        FBE_PROVISION_DRIVE_BACKGROUND_OP_ALL);
        headless_horseman_provision_drive_default_operations.b_is_initialized = FBE_TRUE;
    }
    else
    {
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING, &b_is_background_zeroing_enabled);
        MUT_ASSERT_INT_EQUAL(headless_horseman_provision_drive_default_operations.b_is_background_zeroing_enabled, b_is_background_zeroing_enabled);
        status = fbe_api_base_config_is_background_operation_enabled(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_is_background_sniff_enabled);
        MUT_ASSERT_INT_EQUAL(headless_horseman_provision_drive_default_operations.b_is_sniff_verify_enabled, b_is_background_sniff_enabled);
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

static void headless_horseman_enable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_headless_horseman_error_case_t *error_case_p,
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
    fbe_private_space_layout_region_t region;
    fbe_lba_t rg_offset = HEADLESS_HORSEMAN_BASE_OFFSET;

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

    for ( error_index = 0; error_index < headless_horseman_get_error_count(error_case_p); error_index++)
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
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Get the region and raid group information prior to metadata shrink.
         */
        if (rg_object_id < FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST) {
            
            status = fbe_private_space_layout_get_region_by_object_id(rg_object_id, &region);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            rg_offset = region.starting_block_address;
        }

        /* Initialize the error injection record.
         */
        fbe_test_neit_utils_init_error_injection_record(&record);

        status = headless_horseman_get_port_enclosure_slot(rg_config_p, error_case_p, error_index,/* Error case. */
                                                &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Put the pdo object id into the record.
         */
        headless_horseman_get_pdo_object_id(rg_config_p, error_case_p, &record.object_id, error_index);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_case_p->errors[error_index].location == HEADLESS_HORSEMAN_ERROR_LOCATION_RAID)
        {
            /* Inject on both user and metadata.
             */
            record.lba_start  = 0x0;
            record.lba_end    = (info.raid_capacity / info.num_data_disk) - 1;
        }
        else if (error_case_p->errors[error_index].location == HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA)
        {
            /* For now we just inject the error over a chunk.
             */
            record.lba_start  = (error_case_p->error_chunk_offset * FBE_RAID_DEFAULT_CHUNK_SIZE);
            record.lba_end    = (error_case_p->error_chunk_offset * FBE_RAID_DEFAULT_CHUNK_SIZE) + FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
        }
        else if (error_case_p->errors[error_index].location == HEADLESS_HORSEMAN_ERROR_LOCATION_USER_DATA_CHUNK_1)
        {
            /* For now we just inject the error over a chunk.
             * The chunk we inject on depends on the error index.
             */
            record.lba_start  = FBE_RAID_DEFAULT_CHUNK_SIZE;
            record.lba_end    = record.lba_start + FBE_RAID_DEFAULT_CHUNK_SIZE - 1;
        }
        else if (error_case_p->errors[error_index].location == HEADLESS_HORSEMAN_ERROR_LOCATION_PAGED_METADATA)
        {
            /* Inject from the start of the paged metadata to the end of the paged metadata.
             */
            record.lba_start  = info.paged_metadata_start_lba / info.num_data_disk;
            record.lba_end    = (info.raid_capacity / info.num_data_disk) - 1;
        }
        else if (error_case_p->errors[error_index].location == HEADLESS_HORSEMAN_ERROR_LOCATION_WRITE_LOG)
        {
            /* Inject just for the signature.
             */
            record.lba_start  = info.write_log_start_pba;
            record.lba_end    = info.write_log_start_pba + info.write_log_physical_capacity - 1;
        }
        else if (error_case_p->errors[error_index].location == HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA)
        {
            fbe_object_id_t pvd_object_id;
            fbe_api_provision_drive_info_t  provision_drive_info;

            headless_horseman_get_pvd_object_id(rg_config_p, error_case_p, &pvd_object_id, error_index);

            /* Just inject to (1) block of the paged.
             */
            status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            record.lba_start  = provision_drive_info.paged_metadata_lba;
            record.lba_end    = provision_drive_info.paged_metadata_lba + provision_drive_info.chunk_size - 1;
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "Error index %d location %d is invalid",
                       error_index, error_case_p->errors[error_index].location);
            MUT_FAIL_MSG("Error location is invalid");
        }

        headless_horseman_fill_error_record(rg_config_p, error_case_p, error_index, &record);

        if (error_case_p->errors[error_index].location != HEADLESS_HORSEMAN_ERROR_LOCATION_PROVISION_DRIVE_METADATA) 
        {
            if (error_case_p->errors[error_index].command != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) 
            {
                record.skip_blks = 1; /* Skip 1 block to cause incomplete write/read. */
            }
            record.lba_start += rg_offset;
            record.lba_end += rg_offset;
        }
        if (error_case_p->b_inc_for_all_drives) 
        {
            record.b_inc_error_on_all_pos  = FBE_TRUE;
        }

        /* If the error is not lba specific, disable provision drive background services.*/
        if (error_case_p->errors[error_index].error_type == FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE)
        {
            MUT_ASSERT_INT_EQUAL(1, headless_horseman_get_error_count(error_case_p));
            headless_horseman_disable_provision_drive_background_operations(port_number, encl_number, slot_number);
        }

        status = headless_horseman_inject_scsi_error(&record, &handle_array_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "%s rg: 0x%x pos: %d pdo object: %d (0x%x) b/e/s:%d/%d/%d slba:0x%llx elba:0x%llx", 
                   __FUNCTION__, rg_object_id, error_case_p->errors[error_index].rg_position_to_inject, 
                   record.object_id, record.object_id, port_number, encl_number, slot_number,
                   (unsigned long long)record.lba_start,
           (unsigned long long)record.lba_end);
    }
    return;
}
static void headless_horseman_enable_provision_drive_background_operations(fbe_u32_t bus,
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

    MUT_ASSERT_INT_EQUAL(headless_horseman_provision_drive_default_operations.b_is_initialized, FBE_TRUE);

    /* Enable those provision drive operations that were enabled by default.*/
    if (headless_horseman_provision_drive_default_operations.b_is_background_zeroing_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    if (headless_horseman_provision_drive_default_operations.b_is_sniff_verify_enabled == FBE_TRUE)
    {
        status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
static void headless_horseman_disable_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                           fbe_headless_horseman_error_case_t *error_case_p,
                                           fbe_protocol_error_injection_record_handle_t *handle_p)
{
    fbe_u32_t error_index;
    fbe_object_id_t pdo_object_id;
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;

    /* First remove all the records then remove the object */
    for ( error_index = 0; error_index < headless_horseman_get_error_count(error_case_p); error_index++)
    {
        status = fbe_api_protocol_error_injection_remove_record(handle_p[error_index]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = headless_horseman_get_port_enclosure_slot(rg_config_p, error_case_p, error_index,/* Error case. */
                                                &port_number, &encl_number, &slot_number);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (error_case_p->errors[error_index].error_type == FBE_TEST_PROTOCOL_ERROR_TYPE_PORT_RETRYABLE)
        {
            MUT_ASSERT_INT_EQUAL(1, headless_horseman_get_error_count(error_case_p));
            headless_horseman_enable_provision_drive_background_operations(port_number, encl_number, slot_number);
        }
    }

    status = headless_horseman_get_pdo_object_id(rg_config_p, error_case_p, &pdo_object_id, 1    /* error number */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_remove_object(pdo_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*!**************************************************************
 * headless_horseman_verify_pvd()
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

void headless_horseman_verify_pvd(fbe_test_raid_group_disk_set_t *drive_to_insert_p)
{
    fbe_status_t    status;
        
    /* Verify the PDO and LDO are in the READY state
     */
    status = fbe_test_pp_util_verify_pdo_state(drive_to_insert_p->bus, 
                                               drive_to_insert_p->enclosure,
                                               drive_to_insert_p->slot,
                                               FBE_LIFECYCLE_STATE_READY,
                                               HEADLESS_HORSEMAN_WAIT_MSEC);
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
 * end headless_horseman_verify_pvd()
 ******************************************/
void headless_horseman_enable_trace_flags(fbe_object_id_t object_id)

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
void headless_horseman_disable_trace_flags(fbe_object_id_t object_id)

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

void headless_horseman_reset_drive_state(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_headless_horseman_error_case_t *error_case_p, 
                              fbe_u32_t drive_index)
{
    fbe_status_t status;
    fbe_object_id_t pdo_object_id;
    fbe_object_id_t pvd_object_id;

    fbe_u32_t position = error_case_p->errors[drive_index].rg_position_to_inject;


    status = headless_horseman_get_pdo_object_id(rg_config_p, error_case_p, &pdo_object_id, drive_index /* error number */);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    headless_horseman_get_drive_position(rg_config_p, &position);
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


    headless_horseman_verify_pvd(&rg_config_p->rg_disk_set[position]);
    return;
}

void headless_horseman_reinsert_drives(fbe_test_rg_configuration_t *rg_config_p,
                            fbe_headless_horseman_error_case_t *error_case_p)
{
    fbe_u32_t drive_index;
    for ( drive_index = 0; drive_index < headless_horseman_get_error_count(error_case_p); drive_index++)
    {
        headless_horseman_reset_drive_state(rg_config_p, error_case_p, drive_index);
    }
}

void headless_horseman_degrade_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_headless_horseman_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t pdo_object_id;

    for ( index = 0; index < HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS; index++)
    {
        fbe_u32_t degraded_position;

        if (headless_horseman_get_and_check_degraded_position(rg_config_p,
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
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    return;
}
void headless_horseman_restore_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_headless_horseman_error_case_t *error_case_p)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_object_id_t pdo_object_id;
    fbe_api_terminator_device_handle_t drive_handle;

    for ( index = 0; index < HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS; index++)
    {
        fbe_u32_t degraded_position;

        if (headless_horseman_get_and_check_degraded_position(rg_config_p,
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

            headless_horseman_verify_pvd(disk_set_p);
        }
    }
    return;
}

void headless_horseman_validate_journal_flags(fbe_test_rg_configuration_t *rg_config_p, 
                                              fbe_parity_write_log_flags_t flags)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_parity_get_write_log_info_t write_log_info;
    fbe_api_raid_group_get_info_t rg_info;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (rg_config_p->class_id == FBE_CLASS_ID_PARITY &&
        rg_info.rekey_checkpoint != FBE_LBA_INVALID)
    {
        /* Get the slot info from the raid group
         */
        status = fbe_api_raid_group_get_write_log_info(rg_object_id,
                                                       &write_log_info, 
                                                       FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        /* If the journal supports 4K drives then validate that the 4K flag is set.
         */
        if (((rg_info.element_size == FBE_RAID_SECTORS_PER_ELEMENT)                   &&
             (write_log_info.slot_size == FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM_4K)    )       ||
            ((rg_info.element_size == FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH)              &&
             (write_log_info.slot_size == FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH_4K)     )    ) {
            flags |= FBE_PARITY_WRITE_LOG_FLAGS_4K_SLOT;
        }

        MUT_ASSERT_INT_EQUAL(write_log_info.flags, flags);
        mut_printf(MUT_LOG_TEST_STATUS, "Validate write log flags 0x%x=0x%x for rg 0x%x", 
                   flags, write_log_info.flags, rg_object_id);
    }
}

void headless_horseman_enable_trace_flags_for_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t drive_index;
    fbe_object_id_t object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_raid_group_disk_set_t *disk_p = NULL;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    headless_horseman_enable_trace_flags(rg_object_id);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    headless_horseman_enable_trace_flags(object_id);

    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_p->bus, 
                                                                disk_p->enclosure,
                                                                disk_p->slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        headless_horseman_enable_trace_flags(object_id);
    }
    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_index, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        headless_horseman_enable_trace_flags(object_id);
    }
    return;
}
void headless_horseman_disable_trace_flags_for_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t drive_index;
    fbe_object_id_t object_id;
    fbe_object_id_t rg_object_id;
    fbe_test_raid_group_disk_set_t *disk_p = NULL;

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    headless_horseman_enable_trace_flags(rg_object_id);
    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    headless_horseman_enable_trace_flags(object_id);

    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_api_provision_drive_get_obj_id_by_location(disk_p->bus, 
                                                                disk_p->enclosure,
                                                                disk_p->slot,
                                                                &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        headless_horseman_enable_trace_flags(object_id);
    }
    for (drive_index = 0; drive_index < rg_config_p->width; drive_index++)
    {
        disk_p = &rg_config_p->rg_disk_set[drive_index];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_index, &object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        headless_horseman_enable_trace_flags(object_id);
    }
    return;
}
void headless_horseman_provision_drive_setup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_headless_horseman_error_case_t *error_case_p,
                                       fbe_api_rdgen_context_t *context_p,
                                       fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                                       fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    fbe_object_id_t pvd_object_id;

    headless_horseman_get_pvd_object_id(rg_config_p, error_case_p, &pvd_object_id,
                                        0 /* There is only (1) error record*/);

    /* Setup the notification before we inject the error since the 
     * notification will happen as soon as we inject the error. 
     */
    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Setup the notification before we inject the error since the 
         * notification will happen as soon as we inject the error. 
         */
        fbe_test_common_util_register_lifecycle_state_notification(ns_context_p, FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                   FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                                   pvd_object_id,
                                                                   error_case_p->wait_for_state_mask);
    }

    /* Start injecting errors.
     */
    headless_horseman_enable_error_injection(rg_config_p, error_case_p, error_handles_p);
    return;
}
void headless_horseman_wait_for_error_injection(fbe_test_rg_configuration_t *rg_config_p,
                                                fbe_headless_horseman_error_case_t *error_case_p,
                                                fbe_api_rdgen_context_t *context_p,
                                                fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                                                fbe_protocol_error_injection_record_handle_t *error_handles_p,
                                                fbe_u32_t timeout_msec)
{
    fbe_status_t status;
    fbe_u32_t error_index;
    fbe_u32_t total_time_ms = 0;
    fbe_protocol_error_injection_error_record_t record;

    for ( error_index = 0; error_index < headless_horseman_get_error_count(error_case_p); error_index++) {
        status = fbe_api_protocol_error_injection_get_record(error_handles_p[error_index], &record);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        while (record.times_inserted < 1) {
            if (total_time_ms > timeout_msec){
                MUT_FAIL_MSG("error not seen in expected time");
            }
            
            fbe_api_sleep(200);
            total_time_ms += 200;
            if ((total_time_ms % 1000) == 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "wait for error to be injected pos: %d rg: %d",
                           error_case_p->errors[error_index].rg_position_to_inject,
                           rg_config_p->raid_group_id);
            }
            status = fbe_api_protocol_error_injection_get_record(error_handles_p[error_index], &record);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }
}
void headless_horseman_setup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_headless_horseman_error_case_t *error_case_p,
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
    seed_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;

    headless_horseman_send_seed_io(rg_config_p, error_case_p, context_p, seed_operation, FBE_RDGEN_PATTERN_LBA_PASS);
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
    headless_horseman_degrade_raid_groups(rg_config_p, error_case_p);

    /* Wait for the raid group to know it is degraded.
     */
    headless_horseman_wait_for_degraded(rg_config_p, error_case_p, HEADLESS_HORSEMAN_WAIT_MSEC);

    /* Start injecting errors.
     */
    headless_horseman_enable_error_injection(rg_config_p, error_case_p, error_handles_p);

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
    //status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
    //                                                    (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
    //                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING | 
    //                                                     current_debug_flags));
    /* set library debug flags */
    //status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
    //                                                    (FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
    //                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING | 
    //                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING |
    //                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING|
    //                                                     current_debug_flags));
    /* set raid group debug flags */
    //fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING |
    //                                                           FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING);
    

   // MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

void headless_horseman_disable_injection(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_headless_horseman_error_case_t *error_case_p,
                              fbe_api_rdgen_context_t *context_p,
                              fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                              fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    fbe_status_t status;

    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Disable error injection since it might prevent the object from coming ready.
         */
        headless_horseman_disable_error_injection(rg_config_p, error_case_p, error_handles_p);

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
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    else
    {
        /* Disable error injection.
         */
        headless_horseman_disable_error_injection(rg_config_p, error_case_p, error_handles_p);
    }

    headless_horseman_clear_drive_error_counts(rg_config_p, error_case_p, 1);
}
void headless_horseman_wait_for_state(fbe_test_rg_configuration_t *rg_config_p,
                              fbe_headless_horseman_error_case_t *error_case_p,
                              fbe_api_rdgen_context_t *context_p,
                              fbe_test_common_util_lifecycle_state_ns_context_t *ns_context_p,
                              fbe_protocol_error_injection_record_handle_t *error_handles_p)
{
    if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID)
    {
        /* Wait for the raid group to get to the expected state.  The notification was
         * initialized above. 
         */
        fbe_test_common_util_wait_lifecycle_state_notification(ns_context_p, HEADLESS_HORSEMAN_WAIT_MSEC);
        fbe_test_common_util_unregister_lifecycle_state_notification(ns_context_p);
    }
}
void headless_horseman_cleanup_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                         fbe_headless_horseman_error_case_t *error_case_p,
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
    headless_horseman_wait_for_verify(rg_config_p, error_case_p, HEADLESS_HORSEMAN_WAIT_MSEC);

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

    /* Now get the verify report for the (1) LUN associated with the raid group
     */
    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

#if 0 /* We can enable this to see PVD lifecycle tracing */
    headless_horseman_disable_trace_flags(rg_config_p->first_object_id + rg_config_p->width);
    headless_horseman_disable_trace_flags(rg_config_p->first_object_id);
    headless_horseman_disable_trace_flags(rg_config_p->first_object_id + (rg_config_p->width * 2));
    headless_horseman_disable_trace_flags(rg_config_p->first_object_id + (rg_config_p->width * 2) + 1);
#endif
    mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
void headless_set_rdgen_options(void)
{
    fbe_rdgen_svc_options_t set_options;
    fbe_status_t status;

    set_options = FBE_RDGEN_SVC_OPTIONS_HOLD_UNPIN_FLUSH;
    status = fbe_api_rdgen_set_svc_options(&set_options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
void headless_unset_rdgen_options(void)
{
    fbe_rdgen_svc_options_t set_options;
    fbe_status_t status;

    set_options = FBE_RDGEN_SVC_OPTIONS_INVALID;
    status  = fbe_api_rdgen_set_svc_options(&set_options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}

static fbe_status_t headless_horseman_raid0_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                      void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    
    headless_set_rdgen_options();

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    /* Setup the test */
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Start error injection. */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Enable background zeroing.");
    fbe_test_sep_drive_enable_background_zeroing_for_all_pvds();

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for raid groups to fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for raid groups to get to desired state.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Pause encryption while we bring drives back.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Restoring drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    /* Drives should be back.  Wait for us to get to ready.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* For errors injected on reads the data on disk is valid.  For errors
     * injected on writes the data was invalidated.
     */
    if (headless_horseman_is_write_error_case(error_case_p, 0)) {
        mut_printf(MUT_LOG_TEST_STATUS, "Check for invalidate pattern.");
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
                headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index],
                                               FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_INVALIDATED);
            }
        }
        current_rg_config_p++;
    }

    /* If it was as write flush the correct data and release the pin
     */
    if (headless_horseman_is_write_error_case(error_case_p, 0)) {
        mut_printf(MUT_LOG_TEST_STATUS, "Cause flush to occur.");
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
                headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index],
                                           FBE_RDGEN_OPERATION_UNLOCK_RESTART, FBE_RDGEN_PATTERN_LBA_PASS);
            }
            current_rg_config_p++;
        }
    }

    /* Validate the correct pattern exists.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Validate pattern on stripe.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    /* Unpause encryption
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Unpause Encryption.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    headless_unset_rdgen_options();
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for eval rb logging to run.  Once it does, allow it to run freely. 
     * Encryption will not run since we are degraded. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            big_bird_wait_for_rb_logging(current_rg_config_p, 1, FBE_TEST_WAIT_TIMEOUT_MS);
        }
        current_rg_config_p++;
    }

    /* Now check the background pattern while degraded.
     */
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Pause encryption while we bring drives back.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            //headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
            //                                  &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }
    /* Unpause encryption
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_provision_drive_paged_error_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                     void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t pvd_object_id;

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_provision_drive_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If the error will cause the paged re-init to fail set the hook.
             */
            if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID) {
                /* Add the hook to wait for the reconstruct start */
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                           FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            } else { 
                /* Else if the retry will succeed add that hook also */
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                                0, /* always position 0 */
                                                SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                                FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS,
                                                0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                                FBE_TEST_HOOK_ACTION_ADD);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for encryption usurper to be logged.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If the reconstruct will cause the pvd to enter activate wait. */
            if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID) {
                /* Wait for the pvd to be faulted then clear drive fault and wait for activate*/
                headless_horseman_get_pvd_object_id(current_rg_config_p,error_case_p,&pvd_object_id, 0);
                status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                                      pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                                      HEADLESS_HORSEMAN_WAIT_MSEC,
                                                                                      FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_provision_drive_clear_drive_fault(pvd_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
            } else {
                /* Else If the retry will succeed wait for that hook also */
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_WAIT);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
        }
        current_rg_config_p++;
    }

    /* Depending on the error type wait for reconstruct or usurper success.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID) {
                /* Wait for start and success */
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                           FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                           FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                           FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                           FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                           FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            } else {
                /* Else this is a retryable error clear the success hook. */
                status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                           0, /* always position 0 */
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            /* Remove the log hook */
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        } /* end if valid for this raid group type */
        current_rg_config_p++;
    }

    /* Now check the background pattern while degraded.
     */
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_raid0_paged_error_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                  void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for RG to go out of ready.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Re-insert the failed drive and clear DRIVE_FAULT.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    /* Wait for activate to be entered.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Wait for the paged to get fixed.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* The drive is still failed, wait for the LUNs to come back online.  Make sure we are rebuild logging.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Now check the background pattern during encryption.
     */
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_paged_error_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                            void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for RG to go out of ready.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Wait for activate to be entered.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Wait for the paged to get fixed.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* The drive is still failed, wait for the LUNs to come back online.  Make sure we are rebuild logging.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            big_bird_wait_for_rb_logging(current_rg_config_p, 1, FBE_TEST_WAIT_TIMEOUT_MS);
        }
        current_rg_config_p++;
    }

    /* Now check the background pattern while degraded.
     */
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
void vault_set_rg_options(void)
{
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = HEADLESS_HORSEMAN_VAULT_ENCRYPT_WAIT_TIME_MS;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}

void vault_set_rg_blob_options(void)
{
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = 1000;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    /* This number of blocks is enough memory for a blob and two slots.
     * We calculate this by sizeof(metadata_paged_blob_t) + 16 * sizeof(metadata_paged_slot_t) 
     * and then round up to the next number of blocks. 
     */
    set_options.encryption_blob_blocks = 0x1b;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
void vault_unset_rg_options(void)
{
    /*! @todo better would be to get the options initially and save them.  
     * Then restore them here. 
     */
    fbe_api_raid_group_class_set_options_t set_options;
    fbe_api_raid_group_class_init_options(&set_options);
    set_options.encrypt_vault_wait_time = 60000;
    set_options.paged_metadata_io_expiration_time = 30000;
    set_options.user_io_expiration_time = 30000;
    fbe_api_raid_group_class_set_options(&set_options, FBE_CLASS_ID_PARITY);
    return;
}
static fbe_status_t headless_horseman_paged_vault_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                            void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_database_lun_info_t lun_info;
    fbe_lba_t end_lba;

    vault_set_rg_options();
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  0, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    lun_info.lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VAULT_LUN;
    status = fbe_api_database_get_lun_info(&lun_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    end_lba = lun_info.capacity - HEADLESS_HORSEMAN_BLOCK_COUNT;
    /* Validate that all the data is now zeros*/
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                  end_lba, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_LBA_PASS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for RG to go out of ready.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Wait for activate to be entered.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Wait for the paged to get fixed.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* The drive is still failed, wait for the LUNs to come back online.  Make sure we are rebuild logging.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            big_bird_wait_for_rb_logging(current_rg_config_p, 1, FBE_TEST_WAIT_TIMEOUT_MS);
        }
        current_rg_config_p++;
    }

    /* Now check the background pattern while degraded.
     */
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for rebuilds ==");
    big_bird_wait_for_rebuilds(rg_config_p, num_raid_groups, 1);
    mut_printf(MUT_LOG_TEST_STATUS, " == Make sure encryption proceeds.");

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for Encryption rekey to finish. ==");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  0, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_rg_run_sync_io(rg_config_p,
                                  FBE_RDGEN_OPERATION_READ_CHECK,
                                  end_lba, HEADLESS_HORSEMAN_BLOCK_COUNT, /* lba, block range to check.*/
                                  FBE_RDGEN_PATTERN_ZEROS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    vault_unset_rg_options();
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_shutdown_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                         void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for eval rb logging to run.  Once it does, allow it to run freely. 
     * Encryption will not run since we are degraded. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    /* Wait for raid groups to fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for raid groups to get to desired state.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Pause encryption while we bring drives back.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Restoring drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }
    /* Drives should be back.  Wait for us to get to ready.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    /* Unpause encryption
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_incomplete_write_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                 void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];

    headless_set_rdgen_options();

    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for eval rb logging to run.  Once it does, allow it to run freely. 
     * Encryption will not run since we are degraded. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                           FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    /* Wait for raid groups to fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for raid groups to get to desired state.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_state(current_rg_config_p, error_case_p, &context[rg_index],
                                             &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    /* Pause encryption while we bring drives back.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Restoring drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }
    /* Drives should be back.  Wait for us to get to ready.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }
    /* First validate that the invalid pattern was written to the stripe.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check for invalidate pattern.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_INVALIDATED);
        }
        current_rg_config_p++;
    }

    /* Next cause the flush to occur.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Cause flush to occur.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index],
                                           FBE_RDGEN_OPERATION_UNLOCK_RESTART, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }
    /* Validate the correct pattern exists.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Validate pattern on stripe.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    /* Unpause encryption
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Unpause Encryption.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_cleanup_test_case(current_rg_config_p, error_case_p, &context[rg_index], 
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    headless_unset_rdgen_options();
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_raid0_dualsp_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                             void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_api_rdgen_context_t *pin_context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    headless_set_rdgen_options();

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    pin_context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context_p[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            /* Pin the data on the passsive side as well so that we will be able to flush it.
             */
            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            headless_horseman_send_pin_io(current_rg_config_p, error_case_p, &pin_context_p[rg_index],
                                          FBE_RDGEN_OPERATION_READ_AND_BUFFER, FBE_RDGEN_PATTERN_LBA_PASS);
            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for pins on the passive side.");
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    headless_horseman_wait_for_pin_io(rg_config_p, FBE_TEST_WAIT_TIMEOUT_MS);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for errors to be injected.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_error_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                       &ns_context[rg_index], &error_handles[rg_index][0],
                                                       FBE_TEST_WAIT_TIMEOUT_MS /* Max time to wait */);
        }
        current_rg_config_p++;
    }
    /* Add the hook to catch the invalidate. 
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Re-insert drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Restoring drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    /* Make sure we are still ready and that we quiesced.
     * By checking quiesce we ensure that we will not miss the invalidate, 
     * since here we check the quiesce is done and below we check that quiesce finished.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    /* Since the flush occurs during quiesce, we wait for quiesce to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for invalidate to occur");
    fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                    120, /* wait time */
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                    FBE_TRUE    /* Yes, wait for cleared */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* For errors injected on reads the data on disk is valid.  For errors
     * injected on writes the data was invalidated.
     */
    if (headless_horseman_is_write_error_case(error_case_p, 0)) {
        mut_printf(MUT_LOG_TEST_STATUS, "Check for invalidate pattern.");
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
                headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                               FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_INVALIDATED);
            }
        }
        current_rg_config_p++;
    }

    /* If it was as write flush the correct data and release the pin
     */
    if (headless_horseman_is_write_error_case(error_case_p, 0)) {
        mut_printf(MUT_LOG_TEST_STATUS, "Cause flush to occur.");
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
                headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_UNLOCK_RESTART, FBE_RDGEN_PATTERN_LBA_PASS);
            }
            current_rg_config_p++;
        }
    }

    /* Wait for flsuh */
    if (headless_horseman_is_write_error_case(error_case_p, 0)) {
        mut_printf(MUT_LOG_TEST_STATUS, "Wait for pin I/O to complete.");
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
            if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
                fbe_api_rdgen_wait_for_ios(&pin_context_p[rg_index], FBE_PACKAGE_ID_SEP_0, 1);
                status = fbe_api_rdgen_test_context_destroy(&pin_context_p[rg_index]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            current_rg_config_p++;
        }
    }

    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, NULL, NULL, FBE_FALSE);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUNs to be ready on both SPs");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate the correct pattern exists.  Since SP A wrote it we wait for SP A to be back up to read it.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Validate pattern.");  

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    headless_unset_rdgen_options();
    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    fbe_api_free_memory(context_p);
    fbe_api_free_memory(pin_context_p);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_dualsp_iw_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                          void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_api_rdgen_context_t *pin_context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    headless_set_rdgen_options();
    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    pin_context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context_p[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            /* Pin the data on the passsive side as well so that we will be able to flush it.
             */
            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            headless_horseman_send_pin_io(current_rg_config_p, error_case_p, &pin_context_p[rg_index],
                                          FBE_RDGEN_OPERATION_READ_AND_BUFFER, FBE_RDGEN_PATTERN_LBA_PASS);
            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for pins on the passive side.");
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    headless_horseman_wait_for_pin_io(rg_config_p, FBE_TEST_WAIT_TIMEOUT_MS);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for errors to be injected.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_error_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                       &ns_context[rg_index], &error_handles[rg_index][0],
                                                       FBE_TEST_WAIT_TIMEOUT_MS /* Max time to wait */);
        }
        current_rg_config_p++;
    }
    /* Add the hook to catch the invalidate. 
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we are still ready and that we quiesced.
     * By checking quiesce we ensure that we will not miss the invalidate, 
     * since here we check the quiesce is done and below we check that quiesce finished.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    /* Since the flush occurs during quiesce, we wait for quiesce to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for invalidate to occur");
    fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                    120, /* wait time */
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                    FBE_TRUE    /* Yes, wait for cleared */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* First validate that the invalid pattern was written to the stripe.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check for invalidate pattern.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_INVALIDATED);
        }
        current_rg_config_p++;
    }

    /* Next cause the flush to occur.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Allow flush to occur");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_UNLOCK_RESTART, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for pin I/O to complete.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            fbe_api_rdgen_wait_for_ios(&pin_context_p[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&pin_context_p[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    //fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, NULL, NULL, FBE_FALSE);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUNs to be ready on both SPs");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    // we should really wait for rekey here.
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Validate the correct pattern exists.  Since SP A wrote it we wait for SP A to be back up to read it.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Validate pattern.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    headless_unset_rdgen_options();
    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    fbe_api_free_memory(context_p);
    fbe_api_free_memory(pin_context_p);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_dualsp_degraded_iw_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                   void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_api_rdgen_context_t *pin_context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    headless_set_rdgen_options();
    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    pin_context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);

    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context_p[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
            /* Pin the data on the passsive side as well so that we will be able to flush it.
             */
            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            headless_horseman_send_pin_io(current_rg_config_p, error_case_p, &pin_context_p[rg_index],
                                          FBE_RDGEN_OPERATION_READ_AND_BUFFER, FBE_RDGEN_PATTERN_LBA_PASS);
            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for pins on the passive side.");
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    headless_horseman_wait_for_pin_io(rg_config_p, FBE_TEST_WAIT_TIMEOUT_MS);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for errors to be injected.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_error_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                       &ns_context[rg_index], &error_handles[rg_index][0],
                                                       FBE_TEST_WAIT_TIMEOUT_MS /* Max time to wait */);
        }
        current_rg_config_p++;
    }
    /* Add the hook to catch the invalidate. 
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we are still ready and that we quiesced.
     * By checking quiesce we ensure that we will not miss the invalidate, 
     * since here we check the quiesce is done and below we check that quiesce finished.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_QUIESCE,
                                           FBE_RAID_GROUP_SUBSTATE_QUIESCE_DONE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for encryption iw check. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Degrade raid groups");
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    big_bird_remove_all_drives(rg_config_p, num_raid_groups, 1, /* Drive count */
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    mut_printf(MUT_LOG_TEST_STATUS, "== Remove hook so iw check can proceed.. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Since the flush occurs during quiesce, we wait for quiesce to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for invalidate to occur");
    fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                    120, /* wait time */
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                    FBE_TRUE    /* Yes, wait for cleared */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    big_bird_insert_all_drives(rg_config_p, num_raid_groups, 1, /* Number of drives to insert */
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* First validate that the invalid pattern was written to the stripe.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Check for invalidate pattern.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_INVALIDATED);
        }
        current_rg_config_p++;
    }

    /* Next cause the flush to occur.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Allow flush to occur");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_UNLOCK_RESTART, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for pin I/O to complete.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            fbe_api_rdgen_wait_for_ios(&pin_context_p[rg_index], FBE_PACKAGE_ID_SEP_0, 1);

            status = fbe_api_rdgen_test_context_destroy(&pin_context_p[rg_index]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    //fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, NULL, NULL, FBE_FALSE);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUNs to be ready on both SPs");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    // we should really wait for rekey here.
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Validate the correct pattern exists.  Since SP A wrote it we wait for SP A to be back up to read it.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Validate pattern.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_send_seed_io(current_rg_config_p, error_case_p, &context_p[rg_index],
                                           FBE_RDGEN_OPERATION_READ_CHECK, FBE_RDGEN_PATTERN_LBA_PASS);
        }
        current_rg_config_p++;
    }

    headless_unset_rdgen_options();
    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, num_raid_groups, 1 /* drives to remove*/ );
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    fbe_api_free_memory(context_p);
    fbe_api_free_memory(pin_context_p);
    return FBE_STATUS_OK;
}
static fbe_status_t headless_horseman_dualsp_pvd_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                           void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    mut_printf(MUT_LOG_TEST_STATUS, " == Add error records. == ");
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_provision_drive_setup_test_case(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                              &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Start injecting errors ==");
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for errors to be injected.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_error_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                       &ns_context[rg_index], &error_handles[rg_index][0],
                                                       FBE_TEST_WAIT_TIMEOUT_MS /* Max time to wait */);
        }
        current_rg_config_p++;
    }
    /* Add the hook to catch the metadata reconstruct. 
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                            FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_INITIATE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                            FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we are still ready.
     * Make sure the peer kicked off the flush of paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "==  Check objects are ready and peer flush of paged kicked off. ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                            FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_INITIATE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                            FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_INITIATE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Check that the pvd paged write finished. ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                            FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_pvd_hooks(current_rg_config_p,
                                            0, /* always position 0 */
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                            FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                            FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    //fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, NULL, NULL, FBE_FALSE);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUNs to be ready on both SPs");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // we should wait for encryption to finish here.
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    fbe_api_free_memory(context_p);
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_dualsp_paged_iw_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    /* Seed all LUNs.
     */
    big_bird_write_background_pattern();
    mut_printf(MUT_LOG_TEST_STATUS, " == Add error records. == ");
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context_p[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Start injecting errors ==");
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for errors to be injected.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_error_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                       &ns_context[rg_index], &error_handles[rg_index][0],
                                                       FBE_TEST_WAIT_TIMEOUT_MS /* Max time to wait */);
        }
        current_rg_config_p++;
    }
    /* Add the hook to catch the metadata reconstruct. 
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PEER_FLUSH_PAGED,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        }
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we are still ready.
     * Make sure the peer kicked off the flush of paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "==  Check objects are ready and peer flush of paged kicked off. ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PEER_FLUSH_PAGED,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PEER_FLUSH_PAGED,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Check that the paged write finished. ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           1, /* one ds object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                           FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    /* Since the flush occurs during quiesce, we wait for quiesce to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for paged reconstruct and quiesce to finish.");
    fbe_test_sep_util_wait_for_rg_base_config_flags(rg_config_p, raid_group_count,
                                                    120, /* wait time */
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, 
                                                    FBE_TRUE    /* Yes, wait for cleared */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption rekey to finish.");
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    //fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);
    //fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, NULL, NULL, FBE_FALSE);

    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for LUNs to be ready on both SPs");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    // We should really wait for encryption here.
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);

    fbe_api_free_memory(context_p);
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_large_rg_reboot_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                void *case_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_sep_package_load_params_t sep_params;
    fbe_neit_package_load_params_t neit_params;
    fbe_api_raid_group_get_info_t rg_info;

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_lba_t first_chkpt;

    vault_set_rg_blob_options();

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    mut_printf(MUT_LOG_TEST_STATUS, "== Change peer update interval so the checkpoint is not sent to peer");
    fbe_test_sep_util_set_update_peer_checkpoint_interval(3000);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);

    first_chkpt = 0x800 * (((FBE_BE_BYTES_PER_BLOCK / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE) * HEADLESS_BLOCKS_PER_BLOB) + 10);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for first pause at 0x%llx==", first_chkpt);

    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, first_chkpt, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for first hook for first pause at 0x%llx==", first_chkpt);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, first_chkpt, FBE_TEST_HOOK_ACTION_WAIT_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on peer so encryption does not advance. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Setup to boot active again.
     */
    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on startup to detect that the iw check occurred. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        fbe_object_id_t rg_object_id;
        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        sep_params.scheduler_hooks[rg_index * 2].object_id = rg_object_id;
        sep_params.scheduler_hooks[rg_index * 2].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION;
        sep_params.scheduler_hooks[rg_index * 2].monitor_substate = FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START;
        sep_params.scheduler_hooks[rg_index * 2].check_type = SCHEDULER_CHECK_STATES;
        sep_params.scheduler_hooks[rg_index * 2].action = SCHEDULER_DEBUG_ACTION_PAUSE;
        sep_params.scheduler_hooks[rg_index * 2].val1 = NULL;
        sep_params.scheduler_hooks[rg_index * 2].val2 = NULL;
        current_rg_config_p++;
    }

   /* Crash the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for IW check to start on survivor. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Crash survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(peer_sp);
    fbe_test_sp_sim_stop_single_sp(peer_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Now we will bring up the original SP.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_large[0], active_sp, &sep_params, &neit_params, FBE_TRUE);

    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    vault_set_rg_blob_options();

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RGs to start IW check.  Validate checkpoint.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_info(rg_object_ids[rg_index], &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== RAID Group %d object 0x%x encryption checkpoint is at: 0x%llx",
                   current_rg_config_p->raid_group_id, rg_object_ids[rg_index], rg_info.rekey_checkpoint);
        /* We never persisted the chkpt, so we start from 0.
         */
        if (rg_info.rekey_checkpoint != 0) {
            MUT_FAIL_MSG("Failed to find expected checkpoint");
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RGs to perform IW check.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        fbe_scheduler_debug_hook_t hook;

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        status = fbe_api_raid_group_get_info(rg_object_ids[rg_index], &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== After IW check RAID Group %d object 0x%x encryption checkpoint is at: 0x%llx",
                   current_rg_config_p->raid_group_id, rg_object_ids[rg_index], rg_info.rekey_checkpoint);

        /* The checkpoint should be advanced to where we left off.
         */
        if (rg_info.rekey_checkpoint != first_chkpt) {
            MUT_FAIL_MSG("Failed to find expected checkpoint");
        }
        status = fbe_test_get_debug_hook(rg_object_ids[rg_index],
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                         FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0, 0, 
                                         &hook);
        /* We expect two advances of checkpoint.
         */
        if (hook.counter != 2) {
            mut_printf(MUT_LOG_TEST_STATUS, "Hook counter is 0x%llx not 2", hook.counter);
            MUT_FAIL_MSG("Hook hit unexpectedly");
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Allow IW check to proceed.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_fill_load_params(&sep_params);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_large[0], peer_sp, &sep_params, &neit_params, FBE_FALSE);

    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    /* Make sure we are still ready.
     * Make sure the peer kicked off the flush of paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "==  Check objects are ready  ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        fbe_object_id_t lun_object_id;
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        headless_horseman_validate_journal_flags(current_rg_config_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED);

        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "== Restore peer chkpt interval ==");
    fbe_test_sep_util_set_update_peer_checkpoint_interval(30);

    vault_unset_rg_options();
    fbe_api_free_memory(context_p);
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_dualsp_peer_lost_rekey_error_part1(fbe_test_rg_configuration_t *rg_config_p,
                                                                         void *case_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_lba_t first_chkpt;

    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);
    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    /* Second step is to disable load balance.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Now we will bring up the original SP.
     */
    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, NULL, NULL, FBE_FALSE);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_kms(NULL);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Shutdown peer
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(peer_sp);
    fbe_test_sp_sim_stop_single_sp(peer_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Now we will bring up the peer SP.
     */
    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], peer_sp, NULL, NULL, FBE_FALSE);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_kms(NULL);
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }

    /* Seed all LUNs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Write Background Pattern on %s == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    big_bird_write_background_pattern();

    /* Add ICW hooks on peer.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on peer SP so Incomplete Write Check does not advance");
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    first_chkpt = 0x800;
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for first pause at 0x%llx==", first_chkpt);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, first_chkpt, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for first hook for first pause at 0x%llx==", first_chkpt);
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, first_chkpt, FBE_TEST_HOOK_ACTION_WAIT_CURRENT);

    /* Setup the error injection on the Active SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Setup error injection on active");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_setup_test_case(current_rg_config_p, error_case_p, &context_p[rg_index],
                                              &ns_context[rg_index], &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Unpause encryption
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Unpause Encryption.");
    fbe_test_sep_use_pause_rekey_hooks(rg_config_p, first_chkpt, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for delay errors to be injected on Active.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_wait_for_error_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                       &ns_context[rg_index], &error_handles[rg_index][0],
                                                       FBE_TEST_WAIT_TIMEOUT_MS /* Max time to wait */);
        }
        current_rg_config_p++;
    }

   /* Crash the active SP.
    */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Update active SP*/
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_free_memory(context_p);
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_dualsp_peer_lost_rekey_error_part2(fbe_test_rg_configuration_t *rg_config_p,
                                                                         void *case_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = NULL;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_api_raid_group_get_info_t rg_info;
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_test_common_util_lifecycle_state_ns_context_t ns_context[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_protocol_error_injection_record_handle_t error_handles[HEADLESS_HORSEMAN_MAX_RG_COUNT][HEADLESS_HORSEMAN_ERROR_CASE_MAX_ERRORS];
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_lba_t first_chkpt;
    fbe_scheduler_debug_hook_t hook;
    fbe_rdgen_sp_id_t originating_sp = FBE_DATA_PATTERN_SP_ID_INVALID;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    context_p = (fbe_api_rdgen_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_context_t) * HEADLESS_HORSEMAN_MAX_RG_COUNT);

    /* Set the rdgen originating SP.
     */
    originating_sp = (peer_sp == FBE_SIM_SP_A) ? FBE_DATA_PATTERN_SP_ID_A : FBE_DATA_PATTERN_SP_ID_B;

    first_chkpt = 0x800;
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for IW check to start on surviving SP. ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Setup the error injection on the Active SP
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Setup error injection on surviving SP");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            if (error_case_p->wait_for_state_mask != FBE_NOTIFICATION_TYPE_INVALID) {
                fbe_object_id_t rg_object_id;

                status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

                /* Setup the notification before we inject the error since the 
                 * notification will happen as soon as we inject the error. 
                 */
                fbe_test_common_util_register_lifecycle_state_notification(&ns_context[rg_index], FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                           FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP,
                                                                           rg_object_id,
                                                                           error_case_p->wait_for_state_mask);
            }

            /* Start injecting errors.
             */
            headless_horseman_enable_error_injection(current_rg_config_p, error_case_p, &error_handles[rg_index][0]);
        }
        current_rg_config_p++;
    }
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RGs to start IW check on surviving SP.  Validate checkpoint.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        status = fbe_api_raid_group_get_info(rg_object_ids[rg_index], &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== RAID Group %d object 0x%x encryption checkpoint is at: 0x%llx",
                   current_rg_config_p->raid_group_id, rg_object_ids[rg_index], rg_info.rekey_checkpoint);
        /* We never persisted the chkpt, so we start from 0.
         */
        if (rg_info.rekey_checkpoint != 0) {
            MUT_FAIL_MSG("Failed to find expected checkpoint");
        }

        /* Add hooks for invalidate, move checkpoint and done.
         */
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_INVALIDATE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_UNQUIESCE,
                                       FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove IW start hook.
         */
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Wait for errors to get injected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for errors to be injected.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            fbe_status_t status;
            fbe_u32_t error_index;
            fbe_u32_t total_time_ms = 0;
            fbe_protocol_error_injection_error_record_t record;

            for ( error_index = 0; error_index < headless_horseman_get_error_count(error_case_p); error_index++) {
                status = fbe_api_protocol_error_injection_get_record(error_handles[rg_index][error_index], &record);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                while (record.times_inserted < 1) {
                    if (total_time_ms > 3000){
                        mut_printf(MUT_LOG_TEST_STATUS, "== No error injected for rg_index: %d rg id: %d rg obj: 0x%x - skip",
                                   rg_index, current_rg_config_p->raid_group_id, rg_object_ids[rg_index]);
                        break;
                    }
            
                    fbe_api_sleep(100);
                    total_time_ms += 100;
                    status = fbe_api_protocol_error_injection_get_record(error_handles[rg_index][error_index], &record);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                } /* end while error not found */
            } /* end for all errors */
        } /* end if error case is valid*/

        current_rg_config_p++;
    }

    /* Check if there are any raid groups that hit the IW invalidate
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Check for any Invalidate IW Chunk");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        status = fbe_test_get_debug_hook(rg_object_ids[rg_index],
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                         FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_INVALIDATE,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0, 0, 
                                         &hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If  the invalidation occurred log it.
         */
        if (hook.counter > 0) {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "== Incomplete Write Invalidation on rg index: %d rg id: %d rg obj: 0x%x",
                           rg_index, current_rg_config_p->raid_group_id, rg_object_ids[rg_index]);
        }
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_INVALIDATE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Allow IW check to proceed.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    /* Validate that all raid groups completed unquiesce.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for RGs to complete Unquiesce");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_UNQUIESCE,
                                       FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       1,    /* one ds object */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_UNQUIESCE,
                                       FBE_RAID_GROUP_SUBSTATE_UNQUIESCE_DONE,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        current_rg_config_p++;
    }

    /* Now we will bring up the original SP.
     */
    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], peer_sp, NULL, NULL, FBE_FALSE);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_config_load_kms(NULL);

    /* Now check the background pattern while degraded.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Tell rdgen which SP the I/O originated on.
     */
    handy_manny_test_setup_fill_rdgen_test_context(context_p,
                                                   FBE_OBJECT_ID_INVALID,
                                                   FBE_CLASS_ID_LUN,
                                                   FBE_RDGEN_OPERATION_READ_CHECK,
                                                   FBE_LBA_INVALID, /* use max capacity */
                                                   HEADLESS_HORSEMAN_TEST_ELEMENT_SIZE,
                                                   FBE_RDGEN_PATTERN_LBA_PASS,
                                                   1, /* passes */
                                                   0 /* I/O count not used */);
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CHECK_ORIGINATING_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_rdgen_io_specification_set_originating_sp(&context_p->start_io.specification, originating_sp);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    /*! @note Due to the lost encryption write we have lost data.
     *        So do not validate the rdgen status BUT we will detect a miscompare.
     *        (Shouldn't give back incorrect data).
     */
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    //MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "Restoring drives");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            headless_horseman_disable_injection(current_rg_config_p, error_case_p, &context_p[rg_index],
                                                &ns_context[rg_index], &error_handles[rg_index][0]);
            fbe_api_sleep(2000);
            /* Reinsert drives that failed due to errors.
             */
            headless_horseman_reinsert_drives(current_rg_config_p, error_case_p);

            /* Reinsert drives we failed prior to starting the test.
             */
            headless_horseman_restore_raid_groups(current_rg_config_p, error_case_p);
        }
        current_rg_config_p++;
    }

    /* Make sure we are still ready.
     * Make sure the peer kicked off the flush of paged. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "==  Check objects are ready  ==");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        
        fbe_object_id_t lun_object_id;
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        headless_horseman_validate_journal_flags(current_rg_config_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED);

        current_rg_config_p++;
    }

    /* Wait for encryption to complete (after rebuild completes).
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    /* Now check the background pattern after the rekey (after rebuild) is complete
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    handy_manny_test_setup_fill_rdgen_test_context(context_p,
                                                   FBE_OBJECT_ID_INVALID,
                                                   FBE_CLASS_ID_LUN,
                                                   FBE_RDGEN_OPERATION_READ_CHECK,
                                                   FBE_LBA_INVALID, /* use max capacity */
                                                   HEADLESS_HORSEMAN_TEST_ELEMENT_SIZE,
                                                   FBE_RDGEN_PATTERN_LBA_PASS,
                                                   1, /* passes */
                                                   0 /* I/O count not used */);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    /*! @note Due to the lost encryption write we have lost data.
     *        So do not validate the rdgen status BUT we will detect a miscompare.
     *        (Shouldn't give back incorrect data).
     */
    //MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    //MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    fbe_api_free_memory(context_p);
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_dualsp_reboot_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                              void *case_p)
{
    fbe_headless_horseman_error_case_t *error_case_p = (fbe_headless_horseman_error_case_t *)case_p;
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_sep_package_load_params_t sep_params;
    fbe_neit_package_load_params_t neit_params;
    fbe_u32_t num_lower_objects = FBE_U32_MAX; // By default use top objects only.

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    if ((error_case_p->substate == FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_SET_REKEY_COMPLETE) || 
        (error_case_p->substate == FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY)) {
        num_lower_objects = 1;
    }

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Setup hooks so we stop encryption at right point to reboot active.
     * The passive has a hook so when it becomes active, it does not advance past the point 
     * of finishing encryption we want to be in when the peer comes up. 
     */
    big_bird_write_background_pattern();
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_api_sim_transport_set_target_server(active_sp);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If we have a special substate, remove the hooks added above since it will conflict with the one we will add.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (error_case_p->substate == FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING){
        fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
    }
    
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption starting hook.");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           FBE_U32_MAX, /* only top object */
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Add this hook to pause the peer when it takes over and starts to complete encryption.
             */
            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           num_lower_objects,
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           error_case_p->substate,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_ADD_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait until we reach the correct point and then bring the peer back.
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t lun_object_id;
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(lun_object_id, FBE_OBJECT_ID_INVALID);

            mut_printf(MUT_LOG_TEST_STATUS, "%s waiting for lun object to come ready object id: 0x%x", __FUNCTION__, lun_object_id);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             HEADLESS_HORSEMAN_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            mut_printf(MUT_LOG_TEST_STATUS, "== Wait for newly active SP to start removing old keys.");
            status = fbe_test_use_rg_hooks(current_rg_config_p,
                                           num_lower_objects,
                                           SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                           error_case_p->substate,
                                           0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                           FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);
#if 0
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (headless_horseman_is_rg_valid_for_error_case(error_case_p, current_rg_config_p)) {
            
            fbe_object_id_t rg_object_id;
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            sep_params.scheduler_hooks[rg_index].object_id = rg_object_id;
            sep_params.scheduler_hooks[rg_index].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION;
            sep_params.scheduler_hooks[rg_index].monitor_substate = FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETE_REMOVE_OLD_KEY;
            sep_params.scheduler_hooks[rg_index].check_type = SCHEDULER_CHECK_STATES;
            sep_params.scheduler_hooks[rg_index].action = SCHEDULER_DEBUG_ACTION_PAUSE;
            sep_params.scheduler_hooks[rg_index].val1 = NULL;
            sep_params.scheduler_hooks[rg_index].val2 = NULL;
        }
        current_rg_config_p++;
    }
#endif
    /* Now we will bring up the original SP.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_qual[0], active_sp, &sep_params, &neit_params, FBE_FALSE);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    /* If we have a special substate, remove the hooks added above since it will conflict with the one we will add.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (error_case_p->substate == FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING){
        fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== delete hooks so newly active SP continues with encryption. ");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t lun_index;
        fbe_object_id_t lun_object_id;
        for ( lun_index = 0; lun_index < HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP; lun_index++) 
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       num_lower_objects,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       error_case_p->substate,
                                       0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_DELETE_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        current_rg_config_p++;
    }
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on rebooted SP for encryption complete ==");
    fbe_test_sep_encryption_completing_hooks(rg_config_p, FBE_TEST_HOOK_ACTION_ADD_CURRENT);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    // we should really wait for rekey here.
    fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}

static fbe_status_t headless_horseman_dualsp_reboot_both_test_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                   void *case_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_ids[HEADLESS_HORSEMAN_MAX_RG_COUNT];
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_sep_package_load_params_t sep_params;
    fbe_neit_package_load_params_t neit_params;
    fbe_u16_t data_disks;
    fbe_lba_t capacity;
    fbe_object_id_t rg_object_id;

    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);
    fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Setup hooks so we stop encryption at right point to reboot active.
     * The passive has a hook so when it becomes active, it does not advance past the point 
     * of finishing encryption we want to be in when the peer comes up. 
     */
    big_bird_write_background_pattern();

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption chkpt hook.");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {

        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* Just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START, 0x2000, 0,
                                       SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_sim_transport_set_target_server(peer_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* Just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START, 0x2000, 0,
                                       SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_ADD_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }


    mut_printf(MUT_LOG_TEST_STATUS, "Starting Encryption rekey.");
    status = fbe_test_sep_start_encryption_or_rekey(rg_config_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for Encryption chkpt hook.");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        status = fbe_test_use_rg_hooks(current_rg_config_p,
                                       FBE_U32_MAX, /* Just top level */
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                       FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START, 0x2000, 0,
                                       SCHEDULER_CHECK_VALS_EQUAL, SCHEDULER_DEBUG_ACTION_PAUSE,
                                       FBE_TEST_HOOK_ACTION_WAIT_CURRENT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
        status = fbe_test_sep_util_get_raid_class_info(current_rg_config_p->raid_type,
                                                       current_rg_config_p->class_id,
                                                       current_rg_config_p->width,
                                                       1,    /* not used yet. */
                                                       &data_disks,
                                                       &capacity);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p->capacity = HEADLESS_BLOCKS_PER_LARGE_R10_RG * data_disks;
        current_rg_config_p++;
    }

    /* Setup to boot active again.
     */
    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks on startup to detect that paged verify started. == ");
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < num_raid_groups; rg_index++) {

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        sep_params.scheduler_hooks[rg_index].object_id = rg_object_id;
        sep_params.scheduler_hooks[rg_index].monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION;
        sep_params.scheduler_hooks[rg_index].monitor_substate = FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_COMPLETING;
        sep_params.scheduler_hooks[rg_index].check_type = SCHEDULER_CHECK_STATES;
        sep_params.scheduler_hooks[rg_index].action = SCHEDULER_DEBUG_ACTION_LOG;
        sep_params.scheduler_hooks[rg_index].val1 = NULL;
        sep_params.scheduler_hooks[rg_index].val2 = NULL;
        current_rg_config_p++;
    }

   /* Reboot the active SP.
    */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Just work with the survivor.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(peer_sp);
    fbe_test_sp_sim_stop_single_sp(peer_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Now we will bring up the original SP.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_r10_large[0], active_sp, &sep_params, &neit_params, FBE_TRUE);
    
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);

    vault_set_rg_blob_options();

    /* Bring up passive SP.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", peer_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp_for_config_array(&headless_horseman_raid_group_config_r10_large[0], peer_sp, &sep_params, &neit_params, FBE_FALSE);

    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_load_kms(NULL);
    vault_set_rg_blob_options();

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* The raid group is very large, let's not wait for it to finish before checking the pattern. */
    // fbe_test_encryption_wait_rg_rekey(rg_config_p, FBE_TRUE);

    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Check entire LUN after encrytion rekey.");
    big_bird_read_background_pattern(FBE_RDGEN_PATTERN_LBA_PASS);
    return FBE_STATUS_OK;
}
fbe_u32_t headless_horseman_get_table_case_count(fbe_headless_horseman_error_case_t *error_case_p)
{
    fbe_u32_t test_case_index = 0;

    /* Loop over all the cases in this table until we hit a terminator.
     */
    while (error_case_p[test_case_index].description_p != NULL)
    {
        test_case_index++;
    }
    return test_case_index;
}
fbe_u32_t headless_horseman_get_test_case_count(fbe_headless_horseman_error_case_t *error_case_p[])
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
        test_case_count += headless_horseman_get_table_case_count(&error_case_p[table_index][0]);
        table_index++;
    }
    return test_case_count;
}
fbe_u32_t headless_horseman_get_num_error_positions(fbe_headless_horseman_error_case_t *error_case_p)
{
    fbe_u32_t index;
    fbe_u32_t err_count = 0;
    fbe_u32_t bitmask = 0;

    /* Count the number of unique positions in the error array.
     */
    for ( index = 0; index < headless_horseman_get_error_count(error_case_p); index++)
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
fbe_bool_t headless_horseman_is_width_valid_for_error_case(fbe_headless_horseman_error_case_t *error_case_p,
                                                fbe_u32_t width)
{
    fbe_u32_t index = 0;
    fbe_u32_t num_err_positions = headless_horseman_get_num_error_positions(error_case_p);

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
fbe_u32_t headless_horseman_error_case_num_valid_widths(fbe_headless_horseman_error_case_t *error_case_p)
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

fbe_bool_t headless_horseman_is_rg_valid_for_error_case(fbe_headless_horseman_error_case_t *error_case_p,
                                             fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    if (!headless_horseman_is_width_valid_for_error_case(error_case_p, rg_config_p->width))
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
void headless_horseman_run_test_case_for_all_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_headless_horseman_error_case_t *error_case_p,
                                              fbe_u32_t num_raid_groups,
                                              fbe_u32_t overall_test_case_index,
                                              fbe_u32_t total_test_case_count,
                                              fbe_u32_t table_test_case_index,
                                              fbe_u32_t table_test_case_count)
{
    fbe_status_t status;
    const fbe_char_t *raid_type_string_p = NULL;
    mut_printf(MUT_LOG_TEST_STATUS, "\n\n");
    mut_printf(MUT_LOG_TEST_STATUS, "test_case: %s", error_case_p->description_p); 
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    mut_printf(MUT_LOG_TEST_STATUS, "starting table case: (%u of %u) overall: (%u of %u) line: %u for raid type %s (0x%x)", 
               table_test_case_index, table_test_case_count,
               overall_test_case_index, total_test_case_count , 
               error_case_p->line, raid_type_string_p, rg_config_p->raid_type);

    status = (error_case_p->function)(rg_config_p, error_case_p);
    if (status != FBE_STATUS_OK)
    {
        MUT_FAIL_MSG("test case failed\n");
    }

    /* Only execute this case if this raid group has enough drives.
     */
    if ((headless_horseman_get_num_error_positions(error_case_p) <= rg_config_p->width) &&
        (error_case_p->substate != FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAUSED)              )
    {
        headless_horseman_wait_for_rebuilds(&rg_config_p[0], error_case_p, num_raid_groups);
        headless_horseman_wait_for_verifies(&rg_config_p[0], error_case_p, num_raid_groups);
        headless_horseman_clear_drive_error_counts(&rg_config_p[0], error_case_p, num_raid_groups);
    }
    return;
}
/*!**************************************************************
 * headless_horseman_retry_test()
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

fbe_status_t headless_horseman_retry_test(fbe_test_rg_configuration_t *rg_config_p,
                               fbe_headless_horseman_error_case_t *error_case_p[],
                               fbe_u32_t num_raid_groups)
{
    fbe_u32_t test_index = 0;
    fbe_u32_t table_test_index;
    fbe_u32_t total_test_case_count = headless_horseman_get_test_case_count(error_case_p);
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
    b_abort_expiration_only = fbe_test_sep_util_get_cmd_option("-abort_cases_only");mut_printf(MUT_LOG_TEST_STATUS, "headless_horseman start_table: %d start_index: %d total_cases: %d abort_only: %d",start_table_index, start_test_index, max_case_count, b_abort_expiration_only);

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
        table_test_count = headless_horseman_get_table_case_count(&error_case_p[table_index][0]);

        /* Loop until we hit a terminator.
         */ 
        while (error_case_p[table_index][table_test_index].description_p != NULL)
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
            current_iteration = 0;
            repeat_count = 1;
            fbe_test_sep_util_get_cmd_option_int("-repeat_case_count", &repeat_count);
            while (current_iteration < repeat_count)
            {
                /* Run this test case for all raid groups.
                 */
                headless_horseman_run_test_case_for_all_raid_groups(rg_config_p, 
                                                         &error_case_p[table_index][table_test_index], 
                                                         num_raid_groups,
                                                         test_index, total_test_case_count,
                                                         table_test_index,
                                                         table_test_count);
                current_iteration++;
                if (repeat_count > 1)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "headless_horseman completed test case iteration [%d of %d]", current_iteration, repeat_count);
                }
            }
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
 * end headless_horseman_retry_test()
 **************************************/

/*!**************************************************************
 * headless_horseman_test_rg_config()
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

void headless_horseman_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_u32_t                   rg_index;

    mut_printf(MUT_LOG_TEST_STATUS, "running headless_horseman test");
    error_cases_p = (fbe_headless_horseman_error_case_t **) context_p;

    /*  Disable automatic sparing so that no spares swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for zeroing and initial verify to finish. 
     * Otherwise we will see our tests hit unexpected results when the initial verify gets kicked off. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Waiting for zeroing to complete...");
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        /* Skip raid groups that are not enabled.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)){
            current_rg_config_p++;
            continue;
        }
        fbe_test_zero_wait_for_rg_zeroing_complete(current_rg_config_p);

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Zeroing of all bound drives complete.");
    if (!fbe_test_sep_util_get_no_initial_verify()) {
        fbe_test_sep_util_wait_for_initial_verify(rg_config_p);
    }

    /* headless_horseman test injects error purposefully so we do not want to run any kind of Monitor IOs to run during the test 
      * disable sniff and enable background zeroing to run at PVD level
      * Note: for unmap, it is necessary to allow bgz to run since the bgz checkpoint gets moved back to 0
      * when encryption is enabled. 
      */
    fbe_test_sep_drive_enable_background_zeroing_for_all_pvds();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();

    /* If the RG is too large we will not write to it.
     */
    if (rg_config_p->capacity < HEADLESS_HORSEMAN_BLOCKS_PER_LARGE_RG) {
        big_bird_write_background_pattern();
    }
    
    /* Run test cases for this set of raid groups with this element size.
    */
    headless_horseman_retry_test(rg_config_p, error_cases_p, raid_group_count);
    
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
 * end headless_horseman_test_rg_config()
 ******************************************/

/*!**************************************************************
 * headless_horseman_test()
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

void headless_horseman_test(fbe_headless_horseman_raid_type_t raid_type)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_u32_t                   extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-10 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS == (FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2));

    if (extended_testing_level == 0)
    {
        /* Qual.
         */
        error_cases_p = headless_horseman_qual_error_cases[raid_type];
        rg_config_p = &headless_horseman_raid_group_config_qual[raid_type][0];
    }
    else
    {
        /* Extended. 
         */
        error_cases_p = headless_horseman_qual_error_cases[raid_type];
        rg_config_p = &headless_horseman_raid_group_config_qual[raid_type][0];
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
                                   headless_horseman_test_rg_config,
                                   HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP,
                                   HEADLESS_HORSEMAN_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end headless_horseman_test()
 ******************************************/

/*!**************************************************************
 * headless_horseman_vault_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a vault error test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_vault_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    error_cases_p = (fbe_headless_horseman_error_case_t **) context_p; 

    mut_printf(MUT_LOG_TEST_STATUS, "running headless_horseman test");

    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /*  Disable automatic sparing so that no spares swap-in.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for zeroing and initial verify to finish. 
     * Otherwise we will see our tests hit unexpected results when the initial verify gets kicked off. 
     */
    //fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* headless_horseman test injects error purposefully so we do not want to run any kind of Monitor IOs to run during the test 
      * disable sniff and enable background zeroing to run at PVD level
      * Note: for unmap, it is necessary to allow bgz to run since the bgz checkpoint gets moved back to 0
      * when encryption is enabled. 
      */
    fbe_test_sep_drive_enable_background_zeroing_for_all_pvds();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();
    
    //big_bird_write_background_pattern();

    /* After RG is created, enable encryption */
    // Add hook for encryption completion and enable KMS 
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks for encryption completion and enable KMS started ==");
    status = fbe_test_sep_encryption_add_hook_and_enable_kms(rg_config_p, fbe_test_sep_util_get_dualsp_test_mode());
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== Add hooks to wait for encryption and enable KMS status: 0x%x. ==", status);
    
    rg_config_p->logical_unit_configuration_list[0].lun_number = FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VAULT;
    /* Run test cases for this set of raid groups with this element size.
    */
    headless_horseman_retry_test(rg_config_p, error_cases_p, raid_group_count);
    
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
 * end headless_horseman_vault_test_rg_config()
 ******************************************/
/*!**************************************************************
 * headless_horseman_vault_test()
 ****************************************************************
 * @brief
 *  Run a vault encryption error test.
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  1/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_vault_test(void)
{
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_test_create_raid_group_params_t params;
    fbe_status_t                    status;
    
    error_cases_p = headless_horseman_vault_error_cases;
    rg_config_p = &headless_horseman_raid_group_config_vault[0];

    /* Populate the raid groups test information
     */
    status = fbe_test_populate_system_rg_config(rg_config_p,
                                                1, /* LUNs per RG. */
                                                3 /* Chunks. */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    fbe_test_create_raid_group_params_init_for_time_save(&params, error_cases_p, headless_horseman_vault_test_rg_config,
                                                         1, /* 1 lun per rg, the vault lun. */
                                                         HEADLESS_HORSEMAN_CHUNKS_PER_LUN,
                                                         FBE_FALSE);
    params.b_encrypted = FBE_FALSE;
    params.b_skip_create = FBE_TRUE;
    fbe_test_run_test_on_rg_config_params(rg_config_p, &params);
    return;
}
/******************************************
 * end headless_horseman_vault_test()
 ******************************************/

void headless_horseman_raid0_test(void)
{
    headless_horseman_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID0);
}
void headless_horseman_raid1_test(void)
{
    headless_horseman_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID1);
    }
void headless_horseman_raid3_test(void)
{
    headless_horseman_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID3);
}
void headless_horseman_raid5_test(void)
{
    headless_horseman_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID5);
}
void headless_horseman_raid6_test(void)
{
    headless_horseman_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID6);
}
void headless_horseman_raid10_test(void)
{
    headless_horseman_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID10);
}
/*!**************************************************************
 * headless_horseman_setup()
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
void headless_horseman_setup(void)
{
    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;
        array_p = &headless_horseman_raid_group_config_qual[0];

        for (test_index = 0; test_index < HEADLESS_HORSEMAN_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
            fbe_test_rg_setup_random_block_sizes(&array_p[test_index][0]);
            fbe_test_sep_util_randomize_rg_configuration_array(&array_p[test_index][0]);
        }
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

    fbe_test_disable_background_ops_system_drives();

    /* load KMS */
    sep_config_load_kms(NULL);

    return;
}
/******************************************
 * end headless_horseman_setup()
 ******************************************/

/*!**************************************************************
 * headless_horseman_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the headless_horseman test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    fbe_test_sep_util_restore_sector_trace_level();
    
    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end headless_horseman_cleanup()
 ******************************************/

/*!**************************************************************
 * headless_horseman_dualsp_test()
 ****************************************************************
 * @brief
 *  Run an I/O error retry test.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *  1/9/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_dualsp_test(fbe_headless_horseman_raid_type_t raid_type)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-10 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS == (FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2));

    error_cases_p = headless_horseman_dualsp_error_cases[raid_type];
    rg_config_p = &headless_horseman_raid_group_config_qual[raid_type][0];

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", raid_type_string_p, rg_config_p->raid_type);
        return;
    }

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config(rg_config_p, error_cases_p, 
                                   headless_horseman_test_rg_config,
                                   HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP,
                                   HEADLESS_HORSEMAN_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end headless_horseman_dualsp_test()
 ******************************************/

void headless_horseman_dualsp_raid0_test(void)
{
    headless_horseman_dualsp_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID0);
}
void headless_horseman_dualsp_raid1_test(void)
{
    headless_horseman_dualsp_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID1);
}
void headless_horseman_dualsp_raid3_test(void)
{
    headless_horseman_dualsp_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID3);
}
void headless_horseman_dualsp_raid5_test(void)
{
    headless_horseman_dualsp_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID5);
}
void headless_horseman_dualsp_raid6_test(void)
{
    headless_horseman_dualsp_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID6);
}
void headless_horseman_dualsp_raid10_test(void)
{
    headless_horseman_dualsp_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID10);
}


/*!**************************************************************
 * headless_horseman_dualsp_reboot_test()
 ****************************************************************
 * @brief
 *  Reboot one SP while the other is completing encryption.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *  4/9/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_dualsp_reboot_test(fbe_headless_horseman_raid_type_t raid_type)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-10 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS == (FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2));

    error_cases_p = headless_horseman_dualsp_reboot_error_cases[raid_type];
    rg_config_p = &headless_horseman_raid_group_config_qual[raid_type][0];

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", raid_type_string_p, rg_config_p->raid_type);
        return;
    }

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config(rg_config_p, error_cases_p, 
                                   headless_horseman_test_rg_config,
                                   HEADLESS_HORSEMAN_LUNS_PER_RAID_GROUP,
                                   HEADLESS_HORSEMAN_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end headless_horseman_dualsp_reboot_test()
 ******************************************/

void headless_horseman_reboot_dualsp_raid0_test(void)
{
    headless_horseman_dualsp_reboot_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID0);
}
void headless_horseman_reboot_dualsp_raid1_test(void)
{
    headless_horseman_dualsp_reboot_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID1);
}
void headless_horseman_reboot_dualsp_raid3_test(void)
{
    headless_horseman_dualsp_reboot_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID3);
}
void headless_horseman_reboot_dualsp_raid5_test(void)
{
    headless_horseman_dualsp_reboot_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID5);
}
void headless_horseman_reboot_dualsp_raid6_test(void)
{
    headless_horseman_dualsp_reboot_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID6);
}
void headless_horseman_reboot_dualsp_raid10_test(void)
{
    headless_horseman_dualsp_reboot_test(HEADLESS_HORSEMAN_RAID_TYPE_RAID10);
}

/*!**************************************************************
 * headless_horseman_dualsp_large_rg_reboot_test()
 ****************************************************************
 * @brief
 *  Run encryption test when we use a large RG and
 *  reboot the peer mid encryption.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *  4/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_dualsp_large_rg_reboot_test(void)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-10 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS == (FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2));

    error_cases_p = headless_horseman_dualsp_large_rg_error_cases[0];
    rg_config_p = &headless_horseman_raid_group_config_large[0][0];

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", raid_type_string_p, rg_config_p->raid_type);
        return;
    }

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config(rg_config_p, error_cases_p, 
                                   headless_horseman_test_rg_config,
                                   1, /* One LUN per RG. */
                                   HEADLESS_HORSEMAN_CHUNKS_PER_LUN_LARGE);
    return;
}
/******************************************
 * end headless_horseman_dualsp_large_rg_reboot_test()
 ******************************************/

/*!**************************************************************
 * headless_horseman_dualsp_large_rg_setup()
 ****************************************************************
 * @brief
 *  Setup large capacity raid groups for this test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  4/24/2014 - Created. Rob Foley
 *
 ****************************************************************/
void headless_horseman_dualsp_large_rg_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);
        array_p = &headless_horseman_raid_group_config_large[0];
        fbe_test_sep_util_init_rg_configuration_array(&array_p[0][0]);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_disable_background_ops_system_drives();

    sep_config_load_kms_both_sps(NULL);
    fbe_test_sep_util_set_no_initial_verify(FBE_TRUE);

    return;
}
/**************************************
 * end headless_horseman_dualsp_large_rg_setup()
 **************************************/

/*!**************************************************************
 * headless_horseman_dualsp_large_r10_reboot_test()
 ****************************************************************
 * @brief
 *  Run encryption test when we use a large RG and reboot both SPs mid encryption.
 *
 * @param raid_type      
 *
 * @return None.   
 *
 * @author
 *  5/30/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_dualsp_large_r10_reboot_test(void)
{
    const fbe_char_t            *raid_type_string_p = NULL;
    fbe_headless_horseman_error_case_t    **error_cases_p = NULL;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    /*! @note We want to use a fixed value for the maximum number of degraded
     *        positions, but this assume  a maximum array width.  This is the
     *        worst case for RAID-10 raid groups.
     */
    FBE_ASSERT_AT_COMPILE_TIME(HEADLESS_HORSEMAN_MAX_DEGRADED_POSITIONS == (FBE_RAID_MAX_DISK_ARRAY_WIDTH / 2));

    error_cases_p = headless_horseman_dualsp_large_r10_error_cases[0];
    rg_config_p = &headless_horseman_raid_group_config_r10_large[0][0];

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled", raid_type_string_p, rg_config_p->raid_type);
        return;
    }

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config(rg_config_p, error_cases_p, 
                                   headless_horseman_test_rg_config,
                                   1, /* One LUN per RG. */
                                   HEADLESS_HORSEMAN_CHUNKS_PER_LUN_LARGE);
    return;
}
/******************************************
 * end headless_horseman_dualsp_large_r10_reboot_test()
 ******************************************/
/*!**************************************************************
 * headless_horseman_dualsp_large_r10_rg_setup()
 ****************************************************************
 * @brief
 *  Setup large capacity raid groups for this test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/30/2014 - Created. Rob Foley
 *
 ****************************************************************/
void headless_horseman_dualsp_large_r10_rg_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_test_set_use_fbe_sim_psl(FBE_TRUE);
        array_p = &headless_horseman_raid_group_config_r10_large[0];
        fbe_test_sep_util_init_rg_configuration_array(&array_p[0][0]);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_disable_background_ops_system_drives();

    sep_config_load_kms_both_sps(NULL);
    fbe_test_sep_util_set_no_initial_verify(FBE_TRUE);

    return;
}
/**************************************
 * end headless_horseman_dualsp_large_r10_rg_setup()
 **************************************/
/*!**************************************************************
 * headless_horseman_dualsp_setup()
 ****************************************************************
 * @brief
 *  dualsp encryption error test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1/9/2014 - Created. Rob Foley
 *
 ****************************************************************/
void headless_horseman_dualsp_setup(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Initialize encryption.  Ensures that the test is in sync with 
     * the raid groups which have no key yet. 
     */
    fbe_test_encryption_init();
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_array_t *array_p = NULL;
        fbe_u32_t test_index;
        array_p = &headless_horseman_raid_group_config_qual[0];
        for (test_index = 0; test_index < HEADLESS_HORSEMAN_RAID_TYPE_COUNT; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
        }
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Disable load balancing so that our read and pin will occur on the same SP that we are running the encryption. 
     * This is needed so that rdgen can do the locking on the same SP where the read and pin is occuring.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_database_set_load_balance(FBE_FALSE /* Disabled */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_disable_background_ops_system_drives();

    sep_config_load_kms_both_sps(NULL);

    return;
}
/**************************************
 * end headless_horseman_dualsp_setup()
 **************************************/
/*!**************************************************************
 * headless_horseman_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the headless_horseman test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  1/9/2014 - Created. Rob Foley
 *
 ****************************************************************/

void headless_horseman_dualsp_cleanup(void)
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
        fbe_test_sep_util_destroy_kms_both_sps();

        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end headless_horseman_dualsp_cleanup()
 ******************************************/
/*************************
 * end file headless_horseman_test.c
 *************************/
