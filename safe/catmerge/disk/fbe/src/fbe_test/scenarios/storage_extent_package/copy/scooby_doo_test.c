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
 *  This file contains proactive copy with I/O to degraded and non-degraded
 *  raid groups.
 *
 * HISTORY
 *  03/09/2012  Ron Proulx - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "sep_tests.h"
#include "sep_test_background_ops.h"
#include "sep_rebuild_utils.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_notification_lib.h"
#include "fbe_test_configurations.h"
#include "sep_test_background_ops.h"
#include "sep_event.h"
#include "sep_hook.h"

char * scooby_doo_short_desc = "Test Copy Operations with I/O, degrade raid group, validate copy completes after rebuild.";
char * scooby_doo_long_desc = 
    "\n"
    "\n"
    "This test verifies a Copy from the Source(Candidate) drive to the Destination(Spare) drive. \n"  
    "It is divided in two tasks based on how it initiate the Proactive Copy operation:\n\n"
    "\t 1) User driven Proactive Copy operation(fbe_cli):-\n"
    "\t\t - This scenario will initiate a Proactive Copy operation for specific drive using fbe_cli command (PS).\n"
    "\t\t - It will send the control packet to Config Service to start the Proactive Copy operation.\n"
    "\t\t - It will verify that Drive Sparing Service receives Proactive Spare Swap-In request using fbe_cli command.\n"
    "\t\t - It will verify that Drive Sparing Service has found appropriate Proactive Spare for the swap-in.\n"
    "\t\t - It will verify that VD object starts actual Proactive copy operation.\n"
    "\t\t - It will verify that it updates checkpoint regularly on Proactive Spare drive for every chunk it copies.\n"
    "\t\t - It will also verify that Proactive Copy operation is performed in LBA order (Decision might change in future).\n"
    "\t\t - It will verify that at the end of Proactive Copy completion VD object sends notification to Drive Swa service.\n" 
    "\t\t - It will verify that at the end of proactive copy Drive Swap service will transition to Hot Sparing by removing\n"
    "\t\t   edge between VD object and PVD object for Proactive Candidate drive.\n\n"
    "\t 2) Specific I/O error triggers Proactive Copy operation:-\n"
    "\t\t - This scenario will insert the specific I/O errors during write to specific drives using NEIT tool.\n"
    "\t\t - After insertion of errors it will verify that EOL attribute is propogated till RAID object.\n"
    "\t\t - It will verify that RAID object has requested for Proactive Sparing operation to Drive Sparing Service.\n"
    "\t\t - This scenario will do the all other verification as user driven Proactive Copy operation does.\n"
    "\n"
    "Dependencies:\n\n"
    "\t - Drive Sparing service must provide the policy to select the Proactive Spare.\n"
    "\t - Proactive Copy operation must be serialized across the SPs\n."
    "\t - VD object gets Private Data Library interface to update the metadata (checkpoint) information.\n"
    "\t - Drive Spare service must provide the handling to update the required configuration database based on requested operation.\n"
    "\n"
    "Starting Config:\n"
    "\n"
    "\t [PP] armada board\n"
    "\t [PP] SAS PMC port\n"
    "\t [PP] viper enclosure\n"
    "\t [PP] 5 SAS drives\n"
    "\t [PP] 5 logical drives\n"
    "\t [SEP] 5 provisioned drives\n"
    "\t [SEP] 5 virtual drives\n"
    "\t [SEP] raid group (raid 0), (raid 1), (raid 5), (raid 6), (raid 1/0) (Note: Different set of drives will be used for different RAID group)\n"
    "\t [SEP] 3 luns\n"
    "\n"
    "Test Sequence:\n\n"
    "1) User driven Proactive Copy operation(fbe_cli):-\n"
    "\n"
    "\tSTEP 1: Bring up the initial topology.\n"
    "\t\t - Create and verify the initial physical package config.\n"
    "\t\t - Create provisioned drives and attach edges to logical drives.\n"
    "\t\t - Create virtual drives and attach edges to provisioned drives.\n"
    "\t\t - Create different RAID group with different RAID type on different set of drives and attach edge with virtual drive (RAID 0, RAID 5 etc).\n"
    "\t\t - RAID group attributes:  width: 5 drives  capacity: 0x6000 blocks.\n"
    "\t\t - Create and attach 3 LUNs to the raid 5 object.\n"
    "\t\t - Each LUN will have capacity of 0x2000 blocks and element size of 128.\n"
    "\n"
    "\tSTEP 2: Start the Proactive Copy operation using fbe api or fbe_cli.\n"
    "\t\t - Issue the command through fbe api/fbe_cli to start the Proactive Copy operation.\n"
    "\t\t - Verify that Drive Spare Service has received request for the Proactive Spare swap-in.\n"
    "\n"
    "\tSTEP 3: Make sure that configured Proactive Spare is available for swap-in.\n"
    "\t\t - Verify that Drive Swap Service selects appropriate Proactive Spare and create edge between VD object and Proactive Spare PVD object.\n"
    "\t\t - Verifies that newly created edge with Proactive Spare drive has PS (Proactive Spare) flag set.\n"
    "\n"
    "\tSTEP 4: Verify the progress of the actual Proactive Copy operation.\n"
    "\t\t - Verify that VD object starts actual Proactive Copy based on LBA order.\n"
    "\t\t - Verifies that VD object updates checkpoint metadata using PDL (Private Data Library) regularly on Proactive Spare drive.\n"
    "\t\t - Verifies that VD object sends notification to Drive Swap Service when Proactive Copy operation completes.\n"
    "\n"
    "\tSTEP 5: Verify the transition to Hot Sparing gets done at the end of Proactive Copy operation.\n"
    "\t\t - Verifies that Drive Swap Service transition to Hot Sparing by removing edge with Proactive Candidate drive.\n"
    "\t\t - Verifies that Drive Swap Service updates required configuration database.\n"
    "\n"
    "2) Specific I/O error triggers Proactive Copy operation:-\n"
    "\n"
    "\tSTEP 1: Bring up the initial topology.\n"
    "\t\t - Create and verify the initial physical package config.\n"
    "\t\t - Create provisioned drives and attach edges to logical drives.\n"
    "\t\t - Create virtual drives and attach edges to provisioned drives.\n"
    "\t\t - Create different RAID group with different RAID type on different set of drives and attach edge with virtual drive (RAID 0, RAID 5 etc).\n"
    "\t\t - RAID group attributes:  width: 5 drives  capacity: 0x6000 blocks.\n"
    "\t\t - Create and attach 3 LUNs to the raid 5 object.\n"
    "\t\t - Each LUN will have capacity of 0x2000 blocks and element size of 128.\n"
    "\n"
    "\tSTEP 2: Start the Proactive Copy operation by inserting errors using NEIT tool.\n"
    "\t\t - Issue the NEIT commands to insert the errors which triggers EOL for the specificf drive.\n"
    "\t\t - Verify that EOL attribute is propogated till RAID object.\n"
    "\t\t - Verify that RAID object in response sends request to Drive Spare Service to start Proactive Spare swap-in operation.\n"
    "\n"
    "\tSTEP 3: Make sure that configured Proactive Spare is available for swap-in.\n"
    "\t\t - Verify that Drive Swap Service selects appropriate Proactive Spare and create edge between VD object and Proactive Spare PVD object.\n"
    "\t\t - Verifies that newly created edge with Proactive Spare drive has PS (Proactive Spare) flag set.\n"
    "\n"
    "\tSTEP 4: Verify the progress of the actual Proactive Copy operation.\n"
    "\t\t - Verify that VD object starts actual Proactive Copy based on LBA order.\n"
    "\t\t - Verifies that VD object updates checkpoint metadata using PDL (Private Data Library) regularly on Proactive Spare drive.\n"
    "\t\t - Verifies that VD object sends notification to Drive Swap Service when Proactive Copy operation completes.\n"
    "\n"
    "\tSTEP 5: Verify the transition to Hot Sparing gets done at the end of Proactive Copy operation.\n"
    "\t\t - Verifies that Drive Swap Service transition to Hot Sparing by removing edge with Proactive Candidate drive.\n"
    "\t\t - Verifies that Drive Swap Service updates required configuration database.\n";


/*! @def SCOOBY_DOO_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY
 *
 *  @brief Number of `extra' drives to reserve for copy operation.
 */
#define SCOOBY_DOO_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY    2

/*! @def SCOOBY_DOO_INDEX_RESERVED_FOR_COPY
 *
 *  @brief Position reserved for copy operation
 */
#define SCOOBY_DOO_INDEX_RESERVED_FOR_COPY              0

/*! @def SCOOBY_DOO_INVALID_DISK_POSITION
 *
 *  @brief Invalid disk position.  Used when searching for a disk position and no disk is 
 *         found that meets the criteria.
 */
#define SCOOBY_DOO_INVALID_DISK_POSITION ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def SCOOBY_DOO_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SCOOBY_DOO_WAIT_SEC 60

/*!*******************************************************************
 * @def BIG_BIRD_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SCOOBY_DOO_WAIT_MSEC (1000 * SCOOBY_DOO_WAIT_SEC)

/*!*******************************************************************
 * @def     SCOOBY_DOO_FIXED_PATTERN
 *********************************************************************
 * @brief   rdgen fixed pattern to use
 *
 * @todo    Currently the only fixed pattern that rdgen supports is
 *          zeros.
 *
 *********************************************************************/
#define SCOOBY_DOO_FIXED_PATTERN     FBE_RDGEN_PATTERN_ZEROS

/*!*******************************************************************
 * @def SCOOBY_DOO_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief The number of LUNs in our raid group.
 *
 *********************************************************************/
#define SCOOBY_DOO_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def SCOOBY_DOO_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 * @note Since we want copy to run for some time set this fairly large
 *
 *********************************************************************/
#define SCOOBY_DOO_CHUNKS_PER_LUN      (5)

/*!*******************************************************************
 * @def SCOOBY_DOO_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS
 *********************************************************************
 * @brief Capacity of VD is based on (3) LUNs and (33) chunks per lun
 *
 * @note Since we want copy to run for some time set this fairly large
 *
 *********************************************************************/
#define SCOOBY_DOO_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0x40000)

/*!*******************************************************************
 * @def SCOOBY_DOO_NUM_DRIVES_TO_FORCE_SHUTDOWN
 *********************************************************************
 * @brief Number of drives to remove to force a shutdown
 *
 *********************************************************************/
#define SCOOBY_DOO_NUM_DRIVES_TO_FORCE_SHUTDOWN    (3)

/*!*******************************************************************
 * @def SCOOBY_DOO_RDGEN_WAIT_SECS
 *********************************************************************
 * @brief Delay to run I/O before starting next phase.
 *
 *********************************************************************/
#define SCOOBY_DOO_RDGEN_WAIT_SECS          5

/*!*******************************************************************
 * @def SCOOBY_DOO_DRIVE_WAIT_MSEC
 *********************************************************************
 * @brief Delay to run I/O before starting next phase.
 *
 *********************************************************************/
#define SCOOBY_DOO_DRIVE_WAIT_MSEC          60000

/*!*******************************************************************
 * @def SCOOBY_DOO_TEST_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of our raid groups.
 *
 *********************************************************************/
#define SCOOBY_DOO_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def SCOOBY_DOO_MAX_RAID_GROUPS_PER_TEST
 *********************************************************************
 * @brief   Maximum number of raid groups under test at a time.
 *
 *********************************************************************/
#define SCOOBY_DOO_MAX_RAID_GROUPS_PER_TEST 5

/*!*******************************************************************
 * @def SCOOBY_DOO_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define SCOOBY_DOO_MAX_WIDTH 16

/*!*******************************************************************
 * @def SCOOBY_DOO_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Number of blocks in small io
 *
 *********************************************************************/
#define SCOOBY_DOO_SMALL_IO_SIZE_BLOCKS 1024  

/*!*******************************************************************
 * @def SCOOBY_DOO_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define SCOOBY_DOO_MAX_IO_SIZE_BLOCKS (SCOOBY_DOO_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @var scooby_doo_io_msec_short
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `short' period of time.
 *
 *********************************************************************/
#define SCOOBY_DOO_SHORT_IO_TIME_SECS  5
static fbe_u32_t scooby_doo_io_msec_short = (SCOOBY_DOO_SHORT_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var scooby_doo_io_msec_long
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `long' period of time.
 *
 *********************************************************************/
#define SCOOBY_DOO_LONG_IO_TIME_SECS  30
static fbe_u32_t scooby_doo_io_msec_long = (SCOOBY_DOO_LONG_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @def SCOOBY_DOO_MIRROR_ERROR_INJECTION_MULTIPLIER
 *********************************************************************
 * @brief Number of times a single error will be reported due to:
 *          1. First attempt sees error injection
 *          2. Second attempt during verify
 *          3. Third attempt is verify with mining mode
 *
 *********************************************************************/
#define SCOOBY_DOO_MIRROR_ERROR_INJECTION_MULTIPLIER 3 

/*!*******************************************************************
 * @def SCOOBY_DOO_PARITY_ERROR_INJECTION_MULTIPLIER
 *********************************************************************
 * @brief Number of times a single error will be reported due to:
 *          1. First attempt sees error injection
 *          2. Second attempt during verify
 *
 *********************************************************************/
#define SCOOBY_DOO_PARITY_ERROR_INJECTION_MULTIPLIER 2 

/*!*******************************************************************
 * @var scooby_doo_b_disable_shutdown
 *********************************************************************
 * @brief This variable controls of shutdown tests are enabled for 
 *        certain or all raid groups.
 *
 *********************************************************************/
static fbe_bool_t scooby_doo_b_disable_shutdown = FBE_TRUE;

/*!***************************************************************************
 * @var     scooby_doo_b_are_non_simple_tests_disabled_due_to_DE572
 *****************************************************************************
 * @brief   Determines if only simple tests are enabled to to the fact that 
 *          other tests are incomplete.
 *
 * @todo    Fix the issues preventing other tests to execute.
 *
 *****************************************************************************/
static fbe_bool_t scooby_doo_b_are_non_simple_tests_disabled_due_to_DE572 = FBE_TRUE; /*! Currently advanced testing is disabled */

/*!*******************************************************************
 * @var scooby_doo_threads
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t scooby_doo_threads = 5;

/*!*******************************************************************
 * @var scooby_doo_light_threads
 *********************************************************************
 * @brief Number of threads we will run for light I/O.
 *
 *********************************************************************/
fbe_u32_t scooby_doo_light_threads = 2;

/*!*******************************************************************
 * @var scooby_doo_b_debug_enable
 *********************************************************************
 * @brief Determines if debug should be enabled or not
 *
 *********************************************************************/
fbe_bool_t scooby_doo_b_debug_enable = FBE_FALSE;

/*!*******************************************************************
 * @var scooby_doo_library_debug_flags
 *********************************************************************
 * @brief Default value of the raid library debug flags to set
 *
 *********************************************************************/
fbe_u32_t scooby_doo_library_debug_flags = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                            FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                            FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING  |
                                            FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );

/*!*******************************************************************
 * @var scooby_doo_object_debug_flags
 *********************************************************************
 * @brief Default value of the raid group object debug flags to set
 *
 *********************************************************************/
fbe_u32_t scooby_doo_object_debug_flags = (FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING);

/*!*******************************************************************
 * @var scooby_doo_enable_state_trace
 *********************************************************************
 * @brief Default value of enabling state trace
 *
 *********************************************************************/
fbe_bool_t scooby_doo_enable_state_trace = FBE_TRUE;

/*!*******************************************************************
 * @var     scooby_doo_dest_array
 *********************************************************************
 * @brief   Array of destination positions to spare (for user copy).
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t scooby_doo_dest_array[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = { 0 };

/*!*******************************************************************
 * @var     scooby_doo_dest_handle_array
 *********************************************************************
 * @brief   Arrays of destination drive handles (local SP)
 *
 *********************************************************************/
static fbe_api_terminator_device_handle_t scooby_doo_dest_handle_array[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = { 0 };

/*!*******************************************************************
 * @var     scooby_doo_dest_handle_array
 *********************************************************************
 * @brief   Arrays of destination drive handles (local SP)
 *
 *********************************************************************/
static fbe_api_terminator_device_handle_t scooby_doo_dest_peer_handle_array[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = { 0 };

/*!*******************************************************************
 * @var scooby_doo_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t scooby_doo_test_contexts[SCOOBY_DOO_LUNS_PER_RAID_GROUP * SCOOBY_DOO_MAX_RAID_GROUPS_PER_TEST];

/*!*******************************************************************
 * @var     scooby_doo_pause_params
 *********************************************************************
 * @brief This variable the debug hook locations required for any
 *        test that require a `pause'.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t scooby_doo_pause_params = { 0 };

/*!*******************************************************************
 * @var     scooby_doo_debug_hooks
 *********************************************************************
 * @brief This variable the debug hook locations required for any
 *        test that require a `pause'.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t scooby_doo_debug_hooks = { 0 };

/*!*******************************************************************
 * @var     scooby_doo_protocol_error_injection_records
 *********************************************************************
 * @brief   Array of protocol error injection records (one per raid group)
 *
 *********************************************************************/
static fbe_protocol_error_injection_error_record_t  scooby_doo_protocol_error_injection_records[FBE_TEST_MAX_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @var     scooby_doo_protocol_error_injection_record_handles_p
 *********************************************************************
 * @brief   Array of protocol error injection record handles (one per raid group)
 *          pointer.
 *
 *********************************************************************/
static fbe_protocol_error_injection_record_handle_t scooby_doo_protocol_error_injection_record_handles_p[FBE_TEST_MAX_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @struct scooby_doo_lifecycle_state_ns_context_t
 *********************************************************************
 * @brief This is the notification context for lifecycle_state
 *
 *********************************************************************/
typedef struct scooby_doo_lifecycle_state_ns_context_s
{
    fbe_semaphore_t         sem;
    fbe_spinlock_t          lock;
    fbe_lifecycle_state_t   expected_lifecycle_state;
    fbe_bool_t              b_expected_state_match;
    fbe_lifecycle_state_t   unexpected_lifecycle_state;
    fbe_bool_t              b_unexpected_state_match;
    fbe_time_t              wait_start_time;
    fbe_u32_t               timeout_in_ms;
    fbe_topology_object_type_t object_type;
    fbe_notification_registration_id_t registration_id;
    /* add to here, if more fields to match */
}scooby_doo_lifecycle_state_ns_context_t;

/*!*******************************************************************
 * @var scooby_doo_ns_context
 *********************************************************************
 * @brief This is the notification context for lifecycle_state
 *
 *********************************************************************/
static scooby_doo_lifecycle_state_ns_context_t scooby_doo_ns_context = { 0 };

static fbe_status_t scooby_doo_test_pause_set_debug_hook_for_rgs(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position,
                                                               fbe_sep_package_load_params_t *hook_array_p,
                                                               fbe_u32_t monitor_state,
                                                               fbe_u32_t monitor_substate,
                                                               fbe_u64_t val1,
                                                               fbe_u64_t val2,
                                                               fbe_u32_t check_type,
                                                               fbe_u32_t action,
                                                               fbe_bool_t b_is_reboot_test);

static fbe_status_t scooby_doo_test_pause_wait_and_remove_debug_hook_for_rgs(fbe_test_rg_configuration_t *rg_config_p,
                                                                             fbe_u32_t raid_group_count,
                                                                             fbe_sep_package_load_params_t *sep_params_p,
                                                                             fbe_bool_t b_is_reboot_test);

/*!*******************************************************************
 * @def SCOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types.
 *
 *********************************************************************/
#define SCOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES 3

/*!*******************************************************************
 * @def SCOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types
 *
 *********************************************************************/
#define SCOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES     1

/*!*******************************************************************
 * @var scooby_doo_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t scooby_doo_raid_groups_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t scooby_doo_raid_groups_extended[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
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
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,        0,          0},
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
 * @var scooby_doo_raid_groups_qual
 *********************************************************************
 * @brief Test config for raid test level 0 (default test level).
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t scooby_doo_raid_groups_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t scooby_doo_raid_groups_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
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


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * scooby_doo_set_trace_level()
 ****************************************************************
 * @brief
 *  Cleanup the scooby_doo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/09/2012  Ron Proulx  - Created
 *
 ****************************************************************/
static void scooby_doo_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

/*!**************************************************************
 * scooby_doo_set_io_seconds()
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
void scooby_doo_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();

    if (long_io_time_seconds >= SCOOBY_DOO_LONG_IO_TIME_SECS)
    {
        scooby_doo_io_msec_long = (long_io_time_seconds * 1000);
    }
    else
    {
        scooby_doo_io_msec_long  = ((fbe_random() % SCOOBY_DOO_LONG_IO_TIME_SECS) + 1) * 1000;
    }
    scooby_doo_io_msec_short = ((fbe_random() % SCOOBY_DOO_SHORT_IO_TIME_SECS) + 1) * 1000;
    return;
}
/******************************************
 * end scooby_doo_set_io_seconds()
 ******************************************/

/*!**************************************************************
 * scooby_doo_clear_all_hooks()
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
static fbe_status_t scooby_doo_clear_all_hooks(void)
{
    /* First re-zero the hooks.
     */
    fbe_zero_memory(&scooby_doo_pause_params, sizeof(fbe_sep_package_load_params_t));
    fbe_zero_memory(&scooby_doo_debug_hooks, sizeof(fbe_sep_package_load_params_t));

    return FBE_STATUS_OK;
}
/******************************************
 * end scooby_doo_clear_all_hooks()
 ******************************************/

/*!**************************************************************
 * scooby_doo_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_write_background_pattern
 *
 ****************************************************************/
void scooby_doo_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing background pattern ==", 
               __FUNCTION__);

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            SCOOBY_DOO_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            SCOOBY_DOO_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end scooby_doo_write_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_write_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Seed all the LUNs with a fixed (i.e. doesn't vary) pattern.
 *
 * @param   None               
 *
 * @return  None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void scooby_doo_write_fixed_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing fixed pattern ==", 
               __FUNCTION__);

    /*!  Write a fixed pattern pattern to the LUNs.
     *
     *   @todo Currently zeros is the only fixed pattern available.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             SCOOBY_DOO_FIXED_PATTERN,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             SCOOBY_DOO_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If peer I/O is enabled - send I/O through peer
     */
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp .set I/O to go thru peer", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             SCOOBY_DOO_FIXED_PATTERN,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             SCOOBY_DOO_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If peer I/O is enabled - send I/O through peer
     */
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp .set I/O to go thru peer", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end scooby_doo_write_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_read_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Read all lUNs and validate fixed pattern
 *
 * @param   None               
 *
 * @return  None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void scooby_doo_read_fixed_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating fixed pattern ==", 
               __FUNCTION__);
    
    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             SCOOBY_DOO_FIXED_PATTERN,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             SCOOBY_DOO_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* If peer I/O is enabled - send I/O through peer
     */
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp .set I/O to go thru peer", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);    

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end scooby_doo_read_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_write_zero_background_pattern()
 *****************************************************************************
 *
 * @brief   Seed all the LUNs with a zero pattern.
 *
 * @param   None               
 *
 * @return  None.
 *
 * @author
 *  03/08/2011  Ruomu Gao  - Copied from big_bird_test.c
 *
 *****************************************************************************/
void scooby_doo_write_zero_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing zero pattern ==", 
               __FUNCTION__);

    /*!  Write a zero background pattern to the LUNs.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_ZERO_ONLY,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             SCOOBY_DOO_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             SCOOBY_DOO_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end scooby_doo_write_fixed_pattern()
 ******************************************/

/*!**************************************************************
 * scooby_doo_read_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_read_background_pattern
 *
 ****************************************************************/
void scooby_doo_read_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern ==", 
               __FUNCTION__);
    
    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            SCOOBY_DOO_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end scooby_doo_read_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_is_shutdown_allowed_for_raid_group()
 *****************************************************************************
 *
 * @brief   Simply determines if we all the specified raid group to be shutdown
 *          or not.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 *
 * @return  bool - FBE_TRUE - Allow raid group to be shutdown
 *                 FBE_FALSE - Not allowed to shutdown this raid group
 *
 * @author
 *  09/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_bool_t scooby_doo_is_shutdown_allowed_for_raid_group(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_bool_t  b_allow_shutdown = FBE_TRUE;

    /* Currently al
     */
    if (scooby_doo_b_disable_shutdown == FBE_TRUE)
    {
        b_allow_shutdown = FBE_FALSE;
    }

    return(b_allow_shutdown);
}
/*****************************************************
 * end scooby_doo_is_shutdown_allowed_for_raid_group()
 *****************************************************/

/*!***************************************************************************
 *          scooby_doo_validate_raid_group_degraded()
 *****************************************************************************
 *
 * @brief   Simply determines if we all the specified raid group to be shutdown
 *          or not.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 *
 * @return  bool - FBE_TRUE - Allow raid group to be shutdown
 *                 FBE_FALSE - Not allowed to shutdown this raid group
 *
 * @author
 *  09/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_validate_raid_group_degraded(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_raid_position_bitmask_t expected_degraded_bitmask)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id;

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_wait_for_degraded_bitmask(rg_object_id, 
                                                             expected_degraded_bitmask, 
                                                             30000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


        current_rg_config_p++;
    }

    return FBE_STATUS_OK;

}
/*****************************************************
 * end scooby_doo_validate_raid_group_degraded()
 *****************************************************/

/*!**************************************************************
 * scooby_doo_run_test_on_rg_config_with_extra_disk_with_time_save()
 ****************************************************************
 * @brief
 *  This function is wrapper of fbe_test_run_test_on_rg_config(). 
 *  This function populates extra disk count in rg config and then start 
 *  the test. It destroys RG config depending on the parameter passed to it.
 * 
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 * @param destroy_rg_config - FBE_TRUE: Does not destory the rg config and saves test execution time. FBE_FALSE: Destroys rg config.
 *
 * @return  None.
 *
 * @author
 *  02/21/2013 - Created. Preethi Poddatur
 *
 ****************************************************************/
static void scooby_doo_run_test_on_rg_config_with_extra_disk_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun,
                                    fbe_bool_t destroy_rg_config)
{

   if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    /* populate the required extra disk count in rg config */
    fbe_test_rg_populate_extra_drives(rg_config_p, SCOOBY_DOO_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

    if (destroy_rg_config == FBE_TRUE) {
       /* run the test */
       fbe_test_run_test_on_rg_config(rg_config_p, context_p, test_fn,
                                      luns_per_rg,
                                      chunks_per_lun);
    }
    else {
       /* run the test */
       fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, context_p, test_fn,
                                      luns_per_rg,
                                      chunks_per_lun,
                                      FBE_FALSE);
    }

    return;

}
/********************************************************
 * end scooby_doo_run_test_on_rg_config_with_extra_disk_with_time_save()
 ********************************************************/


/*!**************************************************************
 * scooby_doo_run_test_on_rg_config_with_extra_disk()
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
static void scooby_doo_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{
    scooby_doo_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, context_p, test_fn,
                                       luns_per_rg,
                                       chunks_per_lun,
                                       FBE_TRUE);
    return;
}
/********************************************************
 * end scooby_doo_run_test_on_rg_config_with_extra_disk()
 ********************************************************/


/*!***************************************************************************
 *          scooby_doo_wait_for_raid_group_to_fail()
 *****************************************************************************
 *
 * @brief   Wait for the specified raid group to enter the FAIL state.
 *
 * @param   rg_config_p - Pointer to raid group to wait for
 *
 * @return  None
 *
 * @author
 *  09/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void scooby_doo_wait_for_raid_group_to_fail(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_status_t    status;
    fbe_object_id_t rg_object_id;

    /* Skip this RG if it is not configured for this test */
    if (!fbe_test_rg_config_is_enabled(rg_config_p))
    {
        return;
    }

    /* Get the object id associated with the raid group
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* If allowed to shutdown, wait for the raid group to enter FAIL within
     * 20 seconds.
     */
    if (scooby_doo_is_shutdown_allowed_for_raid_group(rg_config_p))
    {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/******************************************
 * end scooby_doo_wait_for_raid_group_to_fail()
 ******************************************/

/*!**************************************************************
 * scooby_doo_configure_extra_drives_as_hot_spares()
 ****************************************************************
 * @brief
 *  Insert all drives that were removed (but don't include the
 *  sparing drives)
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_to_insert - Number of drives we are removing.
 * @param b_reinsert_same_drive 
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void scooby_doo_configure_extra_drives_as_hot_spares(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t                       index;
    fbe_test_rg_configuration_t     *current_rg_config_p = rg_config_p;
    fbe_test_raid_group_disk_set_t  *drive_to_spare_p = NULL;
    fbe_u32_t                       drive_number;
    fbe_u32_t                       num_removed_drives = 0;
    fbe_u32_t                       num_spared_drives = 0;
    fbe_u32_t                       spared_position;
    fbe_u32_t                       spared_index = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t                       index_to_spare = SCOOBY_DOO_INVALID_DISK_POSITION;

    /*  Disable the recovery queue to delay spare swap-in for any RG at this moment untill all are available
     */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* For every raid group, configure extra drives as hot-spares based on the number of drives removed. */
    for (index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the number of removed drives in the RG. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);

        /* This requires the test to privode enough extra drives for hotspare.
         * Subtrack any drives that are in the `spared' array.
         */
        num_spared_drives = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);
        MUT_ASSERT_CONDITION(num_spared_drives, <=,  current_rg_config_p->num_of_extra_drives);
        MUT_ASSERT_CONDITION(num_spared_drives, <=,  num_removed_drives);

        /* Configure automatic spares from the RG extra disk set. */
        spared_position = SCOOBY_DOO_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {    
            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == SCOOBY_DOO_INVALID_DISK_POSITION)    )
            {
                /* Skip over any spared drives */
                spared_position = fbe_test_sep_util_get_needing_spare_position_from_index(current_rg_config_p, drive_number);
                spared_index = drive_number;
                continue;
            }

            /* Get an extra drive to configure as hot spare. */
            index_to_spare = drive_number;
            drive_to_spare_p = &current_rg_config_p->extra_disk_set[index_to_spare];

            /* Validate that we haven't gone beyond the available spares
             */
            if ((drive_to_spare_p->bus == 0)       &&
                (drive_to_spare_p->enclosure == 0) &&
                (drive_to_spare_p->slot == 0)         )
            {
                /* Exceeded available sspares */
                status = FBE_STATUS_GENERIC_FAILURE;
                MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_STATUS_OK, status, "Insufficient extra drives");
            }

            /* Configure this drive as a hot-spare.*/
            mut_printf(MUT_LOG_TEST_STATUS, "== %s sparing rg %d (%d_%d_%d). ==", 
                       __FUNCTION__, index,  
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Wait for the PVD to be ready.  We need to wait at a minimum for the object to be created,
             * but the PVD is not spareable until it is ready. 
             */
            status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                          drive_to_spare_p->enclosure, 
                                                          drive_to_spare_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          SCOOBY_DOO_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Also wait for the drive to become ready on the peer, if dual-SP test.
             */
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_sim_transport_connection_target_t   local_sp;
                fbe_sim_transport_connection_target_t   peer_sp;
        
                /*  Get the ID of the local SP. */
                local_sp = fbe_api_sim_transport_get_target_server();
        
                /*  Get the ID of the peer SP. */
                if (FBE_SIM_SP_A == local_sp)
                {
                    peer_sp = FBE_SIM_SP_B;
                }
                else
                {
                    peer_sp = FBE_SIM_SP_A;
                }
                fbe_api_sim_transport_set_target_server(peer_sp);
                status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                              drive_to_spare_p->enclosure, 
                                                              drive_to_spare_p->slot,
                                                              FBE_LIFECYCLE_STATE_READY,
                                                              SCOOBY_DOO_DRIVE_WAIT_MSEC);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                fbe_api_sim_transport_set_target_server(local_sp);
            }

            /* Configure extra disk as a hot spare so that it will become part of the raid group permanently.
             */
            status = fbe_test_sep_util_configure_drive_as_hot_spare(drive_to_spare_p->bus, 
                                                                    drive_to_spare_p->enclosure, 
                                                                    drive_to_spare_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        }
        current_rg_config_p++;
   }

    /*  enable the recovery queue so that hot spare swap in occur concurrently at best suitable place
     */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);


   return;
}
/*************************************************************
 * end scooby_doo_configure_extra_drives_as_hot_spares()
 *************************************************************/

/*!**************************************************************
 * scooby_doo_wait_extra_drives_swap_in()
 ****************************************************************
 * @brief
 *  Insert all drives that were removed (but don't include the
 *  sparing drives)
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void scooby_doo_wait_extra_drives_swap_in(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t                       index;
    fbe_u32_t                       num_removed_drives = 0;
    fbe_u32_t                       num_spared_drives = 0;
    fbe_test_raid_group_disk_set_t *unused_drive_to_insert_p;
    fbe_test_rg_configuration_t     *current_rg_config_p = NULL;
    fbe_u32_t                       position_to_insert;
    fbe_u32_t                       drive_number;
    fbe_u32_t                       spared_position;
    fbe_u32_t                       spared_index = SCOOBY_DOO_INVALID_DISK_POSITION;
    #define MAX_RAID_GROUPS 20
    #define MAX_POSITIONS_TO_REINSERT 2
    fbe_u32_t saved_position_to_reinsert[MAX_RAID_GROUPS][MAX_POSITIONS_TO_REINSERT];
    fbe_u32_t saved_num_removed_drives[MAX_RAID_GROUPS];

    if (raid_group_count > MAX_RAID_GROUPS)
    {
        MUT_FAIL_MSG("unexpected number of raid groups");
    }
    /* For every raid group ensure spare drives have swapped in for failed drives. */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it. */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* This requires the test to privode enough extra drives for hotspare.
         * Subtrack any drives that are in the `spared' array.
         */
        num_spared_drives = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);

        /* Get the number of removed drives. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);
        if ((num_removed_drives - num_spared_drives) > MAX_POSITIONS_TO_REINSERT)
        {
            MUT_FAIL_MSG("unexpected number of positions to reinsert");
        }
        saved_num_removed_drives[index] = num_removed_drives;

        /* Confirm hot-spares swapped-in for the removed drives in the RG. */
        spared_position = SCOOBY_DOO_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {
            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == SCOOBY_DOO_INVALID_DISK_POSITION)    )
            {
                /* Skip over any spared drives */
                spared_position = fbe_test_sep_util_get_needing_spare_position_from_index(current_rg_config_p, drive_number);
                saved_position_to_reinsert[index][drive_number] = spared_position;
                spared_index = drive_number;
                continue;
            }
    
            /* Get the next position to insert a new drive. */
            position_to_insert = fbe_test_sep_util_get_next_position_to_insert(current_rg_config_p);
            saved_position_to_reinsert[index][drive_number] = position_to_insert;

            /*! @note Do not remove from the removed array since that will keep track
             *        of the drives that we need ot re-insert.
             */
            //fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert); 

            /* Add this position to the spare array (for rebuild) */
            fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_insert);

            /* Get a pointer to the position in the RG disk set of the removed drive. */
            unused_drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];
        
            if (drive_number < current_rg_config_p->num_of_extra_drives)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "== %s wait permanent swap-in for rg index %d, position %d. ==", 
                           __FUNCTION__, index, position_to_insert);
            
                /* Wait for an extra drive to be permanent swap-in for the failed position. */
                fbe_test_sep_drive_wait_permanent_spare_swap_in(current_rg_config_p, position_to_insert);
            
                mut_printf(MUT_LOG_TEST_STATUS, "== %s swap in complete for rg index %d, position %d. ==", 
                           __FUNCTION__, index, position_to_insert);
            }
        }

        current_rg_config_p++;
    }

    /* No need to validate spared drives*/
    if (num_spared_drives != 0)
    {
        return;
    }

    /* For every raid group ensure spare drives have swapped in for failed drives. */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it. */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* This requires the test to privode enough extra drives for hotspare.
         * Subtrack any drives that are in the `spared' array.
         */
        num_spared_drives = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);

        /* Get the number of removed drives. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);
        if ((num_removed_drives - num_spared_drives) > MAX_POSITIONS_TO_REINSERT)
        {
            MUT_FAIL_MSG("unexpected number of positions to reinsert");
        }

        /* Confirm hot-spares swapped-in for the removed drives in the RG. */
        spared_position = SCOOBY_DOO_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < saved_num_removed_drives[index]; drive_number++)
        {
            /* Get the next position to insert a new drive. */
            position_to_insert = saved_position_to_reinsert[index][drive_number];

            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == SCOOBY_DOO_INVALID_DISK_POSITION)    )
            {
                /* Skip over any spared drives */
                spared_position = position_to_insert;
                continue;
            }

            /* Get a pointer to the position in the RG disk set of the removed drive. */
            unused_drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];

            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting unused drive rg index %d (%d_%d_%d). ==", 
                       __FUNCTION__, index, 
                       unused_drive_to_insert_p->bus, unused_drive_to_insert_p->enclosure, unused_drive_to_insert_p->slot);

            /* Insert removed drive back. */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_reinsert_drive(unused_drive_to_insert_p->bus, 
                                                         unused_drive_to_insert_p->enclosure, 
                                                         unused_drive_to_insert_p->slot,
                                                         current_rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_sim_transport_connection_target_t   local_sp;
                    fbe_sim_transport_connection_target_t   peer_sp;

                    /*  Get the ID of the local SP. */
                    local_sp = fbe_api_sim_transport_get_target_server();

                    /*  Get the ID of the peer SP. */
                    if (FBE_SIM_SP_A == local_sp)
                    {
                        peer_sp = FBE_SIM_SP_B;
                    }
                    else
                    {
                        peer_sp = FBE_SIM_SP_A;
                    }

                    /*  Set the target server to the peer and reinsert the drive there. */
                    fbe_api_sim_transport_set_target_server(peer_sp);
                    status = fbe_test_pp_util_reinsert_drive(unused_drive_to_insert_p->bus, 
                                                             unused_drive_to_insert_p->enclosure, 
                                                             unused_drive_to_insert_p->slot,
                                                             current_rg_config_p->peer_drive_handle[position_to_insert]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /*  Set the target server back to the local SP. */
                    fbe_api_sim_transport_set_target_server(local_sp);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(unused_drive_to_insert_p->bus, 
                                                            unused_drive_to_insert_p->enclosure, 
                                                            unused_drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_sim_transport_connection_target_t   local_sp;
                    fbe_sim_transport_connection_target_t   peer_sp;

                    /*  Get the ID of the local SP. */
                    local_sp = fbe_api_sim_transport_get_target_server();

                    /*  Get the ID of the peer SP. */
                    if (FBE_SIM_SP_A == local_sp)
                    {
                        peer_sp = FBE_SIM_SP_B;
                    }
                    else
                    {
                        peer_sp = FBE_SIM_SP_A;
                    }

                    /*  Set the target server to the peer and reinsert the drive there. */
                    fbe_api_sim_transport_set_target_server(peer_sp);
                    status = fbe_test_pp_util_reinsert_drive_hw(unused_drive_to_insert_p->bus, 
                                                                unused_drive_to_insert_p->enclosure, 
                                                                unused_drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /*  Set the target server back to the local SP. */
                    fbe_api_sim_transport_set_target_server(local_sp);
                }
            }
        
            /* Wait for the unused pvd object to be in ready state. */
            status = fbe_test_sep_util_wait_for_pvd_state(unused_drive_to_insert_p->bus, 
                                                          unused_drive_to_insert_p->enclosure, 
                                                          unused_drive_to_insert_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          SCOOBY_DOO_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    return;
}
/*************************************************************
 * scooby_doo_wait_extra_drives_swap_in()
 *************************************************************/

/*!**************************************************************
 *          scooby_doo_get_next_position_to_insert()
 ****************************************************************
 *
 * @brief   Get the next (non-copy) position to insert
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param b_transition_pvd_fail - FBE_TRUE - Force PVD to fail 
 * @param b_pull_drive - FBE_TRUE - Pull and re-insert same drive
 *                       FBE_FALSE - Replace drive 
 * @param removal_mode - random or sequential
 *
 * @return status - Typically FBE_STATUS_OK,  Othersie we failed to
 *          locate a position to remove
 *
 * @author
 *  09/18/2012  Ron Proulx  - Updated.
 *
 ****************************************************************/
static fbe_status_t scooby_doo_get_next_position_to_insert(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t copy_position,
                                                           fbe_test_drive_removal_mode_t removal_mode,
                                                           fbe_u32_t *next_position_to_insert_p)
{
    fbe_status_t    status = FBE_STATUS_FAILED;
    fbe_u32_t       insert_index = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t       position_to_insert = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t       can_be_inserted[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t       removed_position;

    /* Initialize the position to insert to invalid.
     */
    *next_position_to_insert_p = SCOOBY_DOO_INVALID_DISK_POSITION;

    /* Validate the copy_position.
     */
    if ((copy_position != FBE_RAID_MAX_DISK_ARRAY_WIDTH) &&
        (copy_position >= rg_config_p->width)               )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: copy pos: %d is greater or equal width: %d", 
                   __FUNCTION__, copy_position, rg_config_p->width);

        /* There are no drives available to be removed.
        */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Generate the can be inserted array
     * First initialize all the positions to being present (but mark the
     * copy position as inserted).
     */
    for (insert_index = 0; insert_index < rg_config_p->width; insert_index++)
    {
        if (insert_index == copy_position)
        {
            can_be_inserted[insert_index] = SCOOBY_DOO_INVALID_DISK_POSITION;
        }
        else
        {
            can_be_inserted[insert_index] = insert_index;
        }
    }

    /* Check which insertion mode is requested.
     */
    switch (removal_mode)
    {
        case FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL:
        case FBE_DRIVE_REMOVAL_MODE_RANDOM:
            /* Loop through the 'can be inserted' array to find the first position that can be inserted.
             */
            insert_index = 0;
            while (insert_index != rg_config_p->width)
            {
                removed_position = rg_config_p->drives_removed_array[insert_index];
                if ((removed_position < rg_config_p->width)                                   &&
                    (can_be_inserted[removed_position] != FBE_TEST_SEP_UTIL_INVALID_POSITION)    )
                {
                    /* Found a position to be insert.
                     * Set position_to_insert to the position to be removed.
                     */
                    position_to_insert = removed_position;
                    break;
                }
                insert_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to insert.
             */
            MUT_ASSERT_TRUE(insert_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_insert != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_SPECIFIC:
            /* Loop through the 'can be inserted' array to find the first position that can be removed.
             */
            for (insert_index = 0; insert_index < rg_config_p->width; insert_index++)
            {                
                /* Found a position to be inserted
                 * Set position_to_insert to the position to be removed.
                 */
                removed_position = rg_config_p->specific_drives_to_remove[insert_index];                    
                if ((removed_position < rg_config_p->width)                                   &&
                    (removed_position == rg_config_p->drives_removed_array[insert_index])     &&
                    (can_be_inserted[removed_position] != FBE_TEST_SEP_UTIL_INVALID_POSITION)    )
                {   
                    /* Found a position to be insert.
                     * Set position_to_insert to the position to be removed.
                     */
                    position_to_insert = removed_position;
                    break;                 
                }

            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(insert_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_insert != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        default:
            /* Not a valid algorithm.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Invalid removal algorithm %d\n", 
                       __FUNCTION__, removal_mode);
            MUT_ASSERT_TRUE(FBE_FALSE);
            break;
    }

    MUT_ASSERT_TRUE(position_to_insert < rg_config_p->width);
    *next_position_to_insert_p = position_to_insert;

    return FBE_STATUS_OK;
}
/**********************************************
 * end scooby_doo_get_next_position_to_insert()
 **********************************************/

/*!**************************************************************
 * scooby_doo_insert_drives()
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
 *  09/18/2012  Ron Proulx  - Updated.
 *
 ****************************************************************/
static void scooby_doo_insert_drives(fbe_test_rg_configuration_t *rg_config_p, 
                                     fbe_u32_t raid_group_count,
                                     fbe_u32_t copy_position,
                                     fbe_bool_t b_transition_pvd,
                                     fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_u32_t                       index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t                    status;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       position_to_insert;
    fbe_u32_t                       drive_insert_index;
    fbe_u32_t                       num_drives_to_insert_for_optimal_rg = 1;

    /* For every raid group insert one drive.
     */
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Determine how many drives to remove based on raid type.
         */
        switch(current_rg_config_p->raid_type)
        {
            case FBE_RAID_GROUP_TYPE_RAID1:
                /*! @note To be consistent with `big_bird_wait_for_rebuilds'
                 *        and the purpose of this test isn't to validate
                 *        rebuild but copy, we always remove exactly (1) drive.
                 */
                //num_drives_to_insert_for_optimal_rg = current_rg_config_p->width - 1;
                num_drives_to_insert_for_optimal_rg = 1;
                break;

            case FBE_RAID_GROUP_TYPE_RAID10:
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
                /* 1 parity drive*/
                num_drives_to_insert_for_optimal_rg = 1;
                break;
   
            case FBE_RAID_GROUP_TYPE_RAID6:
                /*! @note To be consistent with `big_bird_wait_for_rebuilds'
                 *        and the purpose of this test isn't to validate
                 *        rebuild but copy, we always remove exactly (1) drive.
                 */
                //num_drives_to_insert_for_optimal_rg = 2;
                num_drives_to_insert_for_optimal_rg = 1;
                break;

            default:
                /* Type not supported.*/
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: rg id: %d type: %d not supported", 
                   __FUNCTION__, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return;
        }

        /* For all the number of drives to remove.
         */
        for (drive_insert_index = 0; drive_insert_index < num_drives_to_insert_for_optimal_rg; drive_insert_index++)
        {
            /* Skip copy position
             */
            status = scooby_doo_get_next_position_to_insert(current_rg_config_p,
                                                            copy_position,
                                                            removal_mode,
                                                            &position_to_insert);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
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
                    if (fbe_test_sep_util_get_dualsp_test_mode())
                    {
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                        status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                                 drive_to_insert_p->enclosure, 
                                                                 drive_to_insert_p->slot,
                                                                 current_rg_config_p->peer_drive_handle[position_to_insert]);
                        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                    }
                }
                else
                {
                    status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                         drive_to_insert_p->enclosure, 
                                                         drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
                    if (fbe_test_sep_util_get_dualsp_test_mode())
                    {
                        /* Set the target server to SPB and insert the drive there. */
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                        status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                                    drive_to_insert_p->enclosure, 
                                                                    drive_to_insert_p->slot);
                        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
                        /* Set the target server back to SPA. */
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                    }
                }
            }
        }

        /* Goto next raid group */
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end scooby_doo_insert_drives()
 ******************************************/

/*!**************************************************************
 *          scooby_doo_start_io()
 ****************************************************************
 * @brief   Start the requested random I/O using the number of 
 *          threads and maximum block size provided.
 *          
 * @param   rdgen_operation - The rdgen operation (w/r/c etc)               
 * @param   rdgen_pattern - The rdgen pattern (LBA, zeros)
 * @param   threads - Number of I/O threads
 * @param   io_size_blocks - Maximum I/O size
 * @param   ran_abort_msecs - Abort time or FBE_TEST_RANDOM_ABORT_TIME_INVALID
 * 
 * @return  None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from scooby_doo_start_io
 *
 ****************************************************************/
static void scooby_doo_start_io(fbe_rdgen_operation_t rdgen_operation,
                                fbe_rdgen_pattern_t rdgen_pattern,
                                fbe_u32_t threads,
                                fbe_u32_t io_size_blocks,
                                fbe_test_random_abort_msec_t ran_abort_msecs)
{

    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;
    fbe_bool_t                  b_ran_aborts_enabled;  

    mut_printf(MUT_LOG_TEST_STATUS, 
           "== %s Start I/O ==", 
           __FUNCTION__);

    /* Setup the abort mode to false
     */
    b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    status = fbe_test_sep_io_configure_abort_mode(b_ran_aborts_enabled, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
         fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           rdgen_operation,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           fbe_test_sep_io_get_threads(threads), /* threads */
                                           io_size_blocks,
                                           b_ran_aborts_enabled, /* Inject aborts if requested*/
                                           FBE_TRUE  /* Peer I/O  */);
    }
    else
    {
        fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           rdgen_operation,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           fbe_test_sep_io_get_threads(threads), /* threads */
                                           io_size_blocks,
                                           b_ran_aborts_enabled, /* Inject aborts if requested*/
                                           FBE_FALSE  /* Peer I/O not supported */);
    }

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

    /* Set the data pattern
     */
    status = fbe_api_rdgen_io_specification_set_pattern(&context_p->start_io.specification, rdgen_pattern);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end scooby_doo_start_io()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_check_if_all_threads_started()
 *****************************************************************************
 *
 * @brief      Check all threads has i/o count > 1 
 *
 * @param      object_info_p 
 * @param      thread_info_p
 *
 * @return     FBE_TRUE if i/o count > 0 for all threads
 *             otherwise FBE_FALSE
 *
 * @author
 *  09/20/2011  Maya Dagon  - Created
 *
 ****************************************************************/
static fbe_bool_t scooby_doo_check_if_all_threads_started(fbe_api_rdgen_get_object_info_t  *object_info_p,                          
                                                  fbe_api_rdgen_get_thread_info_t *thread_info_p)
{

    fbe_u32_t thread_index;
    fbe_u32_t object_index;
    fbe_api_rdgen_thread_info_t *current_thread_info_p = NULL;
    fbe_api_rdgen_object_info_t *current_object_info_p = NULL;
    fbe_u32_t found_threads = 0;

    for (object_index = 0; object_index < object_info_p->num_objects; object_index++)
    {
        current_object_info_p = &object_info_p->object_info_array[object_index];

        /* If we have a fldb, display it if there are outstanding threads.
         */
        if ( current_object_info_p->active_ts_count != 0 )
        {        
            found_threads = 0;

            /* Loop over all the threads and stop when we have found the number we were 
             * looking for. 
             */
            for ( thread_index = 0; 
                  ((thread_index < thread_info_p->num_threads) && (found_threads < current_object_info_p->active_ts_count)); 
                  thread_index++)
            {
                current_thread_info_p = &thread_info_p->thread_info_array[thread_index];
                if (current_thread_info_p->object_handle == current_object_info_p->object_handle)
                {
                    if(current_thread_info_p->io_count < 1)
                    {
                        return FBE_FALSE;
                    }
                    found_threads++;
                }
            }
        }
    }
    return FBE_TRUE;
}
/******************************************
 * end scooby_doo_check_if_all_threads_started()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_wait_for_rdgen_start()
 *****************************************************************************
 *
 * @brief  Wait for all threads to start i/o to luns
 *
 * @param  wait_seconds - max wait time in seconds               
 *
 * @return None.
 *
 * @author
 *  09/20/2011  Maya Dagon  - Created
 *
 ****************************************************************/
static void scooby_doo_wait_for_rdgen_start(fbe_u32_t wait_seconds)
{
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter = {0};
    fbe_api_rdgen_get_object_info_t *object_info_p = NULL;
    fbe_api_rdgen_get_request_info_t *request_info_p = NULL;
    fbe_api_rdgen_get_thread_info_t *thread_info_p = NULL;


    /* Get the stats, then allocate memory.
     */
    fbe_api_rdgen_filter_init(&filter, 
                              FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_LUN, 
                              FBE_PACKAGE_ID_SEP_0,
                              0 /* edge index */);
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if ((stats.objects > 0) && (stats.requests > 0))
    {
        /* Allocate memory to get objects, requests and threads.
         */
        object_info_p = (fbe_api_rdgen_get_object_info_t*)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_object_info_t) * stats.objects);
        MUT_ASSERT_TRUE(object_info_p != NULL);
        
        request_info_p = (fbe_api_rdgen_get_request_info_t*)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_request_info_t) * stats.requests);
        if(request_info_p == NULL)
        {
            fbe_api_free_memory(object_info_p);
            MUT_ASSERT_TRUE(request_info_p != NULL);
        }
        

        thread_info_p = (fbe_api_rdgen_get_thread_info_t*)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_thread_info_t) * stats.threads);
        if (thread_info_p == NULL)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            MUT_ASSERT_TRUE(thread_info_p != NULL);
        }

        /* Fetch info on objects, requests, and threads.
         */
        status = fbe_api_rdgen_get_object_info(object_info_p, stats.objects);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            fbe_api_free_memory(thread_info_p);
            MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK,"cannot get object info\n");
        }

        status = fbe_api_rdgen_get_request_info(request_info_p, stats.requests);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            fbe_api_free_memory(thread_info_p);
            MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK, "cannot get request info \n");
        }
        status = fbe_api_rdgen_get_thread_info(thread_info_p, stats.threads);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            fbe_api_free_memory(thread_info_p);
            MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK,"cannot get thread info\n");
        }
    

        /* Keep looping until we hit the target wait milliseconds.
         */
        do
        {
            /* refresh thread info
             */
            status = fbe_api_rdgen_get_thread_info(thread_info_p, stats.threads);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_free_memory(object_info_p);
                fbe_api_free_memory(request_info_p);
                fbe_api_free_memory(thread_info_p);
                MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK,"cannot get thread info\n");
            }

            if (scooby_doo_check_if_all_threads_started(object_info_p,thread_info_p))
            {
                fbe_api_free_memory(object_info_p);
                fbe_api_free_memory(request_info_p);
                fbe_api_free_memory(thread_info_p);
                return ;
            }

            fbe_api_sleep(500);
            elapsed_msec += 500;
        }while (elapsed_msec < target_wait_msec);
    }

    fbe_api_free_memory(object_info_p);
    fbe_api_free_memory(request_info_p);
    fbe_api_free_memory(thread_info_p);
    /* fail the test 
    */
    MUT_ASSERT_INT_EQUAL_MSG(1,0,"not all rdgen threads started\n");
}
/******************************************
 * end scooby_doo_wait_for_rdgen_start()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_start_fixed_io()
 *****************************************************************************
 *
 * @brief   Run a fixed I/O pattern.  The reason the pattern is fixed is the
 *          fact that the raid group will be shutdown during I/O.  Since some
 *          I/O will be dropped we cannot use a sequence tag in the data since
 *          we want to be able to validate not media errors when the raid group
 *          are brought back.    
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static void scooby_doo_start_fixed_io(fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, 
           "== %s Start fixed I/O ==", 
           __FUNCTION__);
    
    /*! @todo Currently the only fixed pattern that rdgen supports is
     *        zeros.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             SCOOBY_DOO_FIXED_PATTERN,
                                             0,    /* Run forever */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             fbe_test_sep_io_get_threads(scooby_doo_threads),    /* threads */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* Number of stripes to write. */
                                             SCOOBY_DOO_MAX_IO_SIZE_BLOCKS    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* If peer I/O is enabled - send I/O through peer
     */
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp .set I/O to go thru peer", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    if (ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s random aborts inserted %d msecs", __FUNCTION__, ran_abort_msecs);
        status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification, ran_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end scooby_doo_start_fixed_io()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_wait_for_rebuild_start()
 *****************************************************************************
 *
 * @brief   Wait for the specified raid group to `start' rebuilding.  Then
 *          perform any needed cleanup.
 *
 * @param   rg_config_p - Pointer to raid group configurations
 * @param   raid_group_count - Number of raid group configurations
 * @param   rg_object_id_to_wait_for - Raid group object id to wait for rebuild
 *                          to start
 *
 * @return  None.
 *
 * @author
 *  01/12/201   Ron Proulx - Created
 *
 *****************************************************************************/
static void scooby_doo_wait_for_rebuild_start(fbe_test_rg_configuration_t * const rg_config_p,
                                         const fbe_u32_t raid_group_count,
                                         const fbe_object_id_t rg_object_id_to_wait_for)
{
    fbe_u32_t       position_to_rebuild;
    fbe_u32_t       num_positions_to_rebuild;
    fbe_u32_t       rg_index, index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;

    /* First wait for the specified raid group to start the rebuild
     */
    fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(rg_object_id_to_wait_for);
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(rg_object_id_to_wait_for);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Since we are not wait for the rebuild to complete remove from spare array
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* We use the `drives_needing_spare' array to determine which positions
         * need to be rebuilt.  We assume that there is at least (1) position
         * that needs to be rebuilt.
         */
        num_positions_to_rebuild = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);
        MUT_ASSERT_TRUE(num_positions_to_rebuild > 0);
        for (index = 0; index < num_positions_to_rebuild; index++)
        {
            /* If this is the index reserved for copy operations skip it.*/
            if (index == SCOOBY_DOO_INDEX_RESERVED_FOR_COPY)
            {
                continue;
            }

            /* Get the next position to rebuild
             */
            position_to_rebuild = fbe_test_sep_util_get_needing_spare_position_from_index(current_rg_config_p, index);

            /* If this is the position */

            /* Remove the position from the `needing spare' array
             */
            fbe_test_sep_util_delete_needing_spare_position(current_rg_config_p, position_to_rebuild);

        } /* end for all positions that need to be rebuilt */

        current_rg_config_p++;

    } /* end for all raid groups */

    return;
}
/******************************************
 * end scooby_doo_wait_for_rebuild_start()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_wait_for_rebuild_completion()
 *****************************************************************************
 *
 * @brief   Wait for the required number of positions to complete
 *          rebuild.
 *
 * @param   rg_config_p - Pointer to raid group configuraiton to wait for
 * @param   b_wait_for_all_degraded_positions_to_rebuild - FBE_TRUE - Wait for
 *                  all positions to be fully rebuilt
 *                                                       - FBE_FALSE - Only wait
 *                  for (1) position to rebuild
 * @return  None.
 *
 * @author
 * 03/12/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void scooby_doo_wait_for_rebuild_completion(fbe_test_rg_configuration_t * const rg_config_p,
                                                   fbe_bool_t b_wait_for_all_degraded_positions_to_rebuild)
{
    fbe_object_id_t rg_object_id;
    fbe_u32_t       position_to_rebuild;
    fbe_u32_t       num_positions_to_rebuild;
    fbe_u32_t       index;

    /* There is no need to wait if the rg_config is not enabled
    */
    if (!fbe_test_rg_config_is_enabled(rg_config_p))
    {            
        return;
    }

    /* Get the riad group object id
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* We use the `drives_needing_spare' array to determine which positions
     * need to be rebuilt.  We assume that there is at least (1) position
     * that needs to be rebuilt.
     */
    num_positions_to_rebuild = fbe_test_sep_util_get_num_needing_spare(rg_config_p);
    MUT_ASSERT_TRUE(num_positions_to_rebuild > 0);

    /* Walk all the `needing spare' positions, wait for them to rebuild then
     * remove them from the `needing spare' array.
     */
    for (index = 0; index < num_positions_to_rebuild; index++)
    {
        fbe_status_t status;

        /* If this is the index reserved for copy operations skip it.*/
        if (index == SCOOBY_DOO_INDEX_RESERVED_FOR_COPY)
        {
            continue;
        }

        /* Get the next position to rebuild
         */
        position_to_rebuild = fbe_test_sep_util_get_needing_spare_position_from_index(rg_config_p, index);

        /* If we need to wait for all positions or this is the position we
         * need to wait for.
         */
        if (b_wait_for_all_degraded_positions_to_rebuild == FBE_TRUE)
        {
            /* Wait for the rebuild to complete
             */
            status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_rebuild,
                                                  FBE_TEST_SEP_UTIL_RB_WAIT_SEC);           
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_rebuild,
                                                  FBE_TEST_SEP_UTIL_RB_WAIT_SEC);           
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }

        /* Remove the position from the `needing spare' array
         */
        fbe_test_sep_util_delete_needing_spare_position(rg_config_p, position_to_rebuild);

    } /* end for all positions that need to be rebuilt */

    return;
}
/*********************************************
 * end scooby_doo_wait_for_rebuild_completion()
 **********************************************/

/*!**************************************************************
 *          scooby_doo_refresh_extra_disk_info()
 ****************************************************************
 * @brief
 *  Refresh all the extra drive slot information for a raid group
 *  configuration array.  This info can change over time
 *  as permanent spares get swapped in. This routine takes the drive 
 *  capacity into account while select extra drive for rg from unused
 *  drive pool.
 *
 * @note
 *  Right now this routine select extra drive from unused drive pool only
 *  if it's capacity match with rg drive capacity. The reason is spare drive
 *  selection select drive based on removed drive capacity and we don't know 
 *  yet which drive is going to remove from the raid group next. Hence 
 *  while creating the rg, test does select drive for same capacity and while
 *  sparing, it will use extra drive for same capacity to make sure that when
 *  we run the test on hardware, there should not be any drive selection problem.
 *
 * @todo
 *  When there will be change in spare selection to select drive based on minimum rg
 *  drive capacity, we need to refer minimum drive capacity from rg while select extra drive.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None.
 *
 * @author
 *  05/13/2011 - Created. Amit Dhaduk
 *  06/18/2011 - Modified. Amit Dhaduk
 *  
 *
 ****************************************************************/
static void scooby_doo_refresh_extra_disk_info(fbe_test_rg_configuration_t *rg_config_p, 
                                               fbe_u32_t raid_group_count)
{

    fbe_u32_t                               rg_index;
    fbe_u32_t                               index;
    fbe_u32_t                               removed_drive_index;
    fbe_u32_t                               drive_type;
    fbe_u32_t                               extra_drive_index;
    fbe_u32_t                               unused_drive_index;
    fbe_lba_t                               rg_drive_capacity;
    fbe_lba_t                               max_rg_drive_capacity;
    fbe_lba_t                               unused_drive_capacity;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_block_size_t                   block_size;
    fbe_bool_t                              skip_removed_drive;
        

    /* find the all available drive info */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* find the all available unused pvd info */
    fbe_test_sep_util_get_all_unused_pvd_location(&drive_locations, &unused_pvd_locations);

    /* Just loop over all our raid groups, refreshing extra disk info with available unused pvd .
    */    
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        drive_type = rg_config_p[rg_index].drive_type;

        /* find out the block size */
        if(rg_config_p[rg_index].block_size == 520)
        {
            block_size = FBE_TEST_BLOCK_SIZE_520;
        }
        else if(rg_config_p[rg_index].block_size == 512)
        {
            block_size = FBE_TEST_BLOCK_SIZE_512;
        }
        else
        {
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, " refresh extra disk info - Unsupported block size\n");
        }

        max_rg_drive_capacity = 0;

        /* get maximum drive capacity from raid group
         */
        for(index = 0 ; index < rg_config_p[rg_index].width; index++)
        {
            skip_removed_drive = FBE_FALSE;

            /* skip the drive if it is removed
             */
            for(removed_drive_index =0; removed_drive_index <  rg_config_p[rg_index].num_removed; removed_drive_index++)
            {
                if(index == rg_config_p[rg_index].drives_removed_array[removed_drive_index])
                {
                    skip_removed_drive = FBE_TRUE;
                    break;
                }
            }

            if (skip_removed_drive == FBE_TRUE)
            {
                continue;
            }
        
            /* find out the drive capacity */
            fbe_test_sep_util_get_drive_capacity(&rg_config_p[rg_index].rg_disk_set[index], &rg_drive_capacity);

            /* mut_printf(MUT_LOG_TEST_STATUS, "== refresh extra disk  -rg disk (%d_%d_%d), cap 0x%x . ==", 
                    rg_config_p[rg_index].rg_disk_set[index].bus, rg_config_p[rg_index].rg_disk_set[index].enclosure, rg_config_p[rg_index].rg_disk_set[index].slot, rg_drive_capacity );
            */

            if (max_rg_drive_capacity < rg_drive_capacity)
            {
                max_rg_drive_capacity = rg_drive_capacity;
            }
        }


        extra_drive_index = 0;

        /* We assume one of the `extra' drives has been consumed for the copy
         * operation.  So we compare against: (`number_of_extra_drives' - 1).
         */
        rg_config_p[rg_index].num_of_extra_drives -= 1;

        /* If required, scan all the unused drive and pick up the drive which 
         * is not yet selected and match the capacity. 
         */
        if (rg_config_p[rg_index].num_of_extra_drives < 1)
        {
            /* Just print an informational message.
             */
            mut_printf(MUT_LOG_TEST_STATUS,  "refresh extra: num of extra drives: %d rg_index: %d raid id: %d type: %d width: %d",
                       rg_config_p[rg_index].num_of_extra_drives, rg_index, rg_config_p[rg_index].raid_group_id,
                       rg_config_p[rg_index].raid_type, rg_config_p[rg_index].width);
            continue;
        }

            
        /* Validate the number of extra drives.
         */
        for(unused_drive_index =0; unused_drive_index <unused_pvd_locations.drive_counts[block_size][drive_type]; unused_drive_index++)
        {
            /* check if drive is not yet selected 
             */
            if (unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot != FBE_INVALID_SLOT_NUM)
            {
    
                fbe_test_sep_util_get_drive_capacity(&unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index], 
                                                    &unused_drive_capacity);
    
                /* check if the drive capacity match with rg max capacity 
                 */
                if(max_rg_drive_capacity == unused_drive_capacity)
                {
                    /* fill out the extra disk set information
                    */
                    rg_config_p[rg_index].extra_disk_set[extra_drive_index].bus = 
                            unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].bus ;
                    rg_config_p[rg_index].extra_disk_set[extra_drive_index].enclosure = 
                            unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].enclosure;
                    rg_config_p[rg_index].extra_disk_set[extra_drive_index].slot = 
                            unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot ;
    
                    /*
                    mut_printf(MUT_LOG_TEST_STATUS,  "filling extra disk pvd(%d_%d_%d) block size %d, drive type %d, rg drive capacity 0x%x, unused drive cnt %d, un dri cap 0x%x, rg index %d\n",
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].bus,
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].enclosure,
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].slot,
                        block_size, drive_type, rg_drive_capacity, unused_pvd_locations.drive_counts[block_size][drive_type] ,
                        unused_drive_capacity, rg_index); 
                    */
    
    
                    /* invalid the drive slot information for selected drive so that next time it will skip from selection
                    */
                    unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot = FBE_INVALID_SLOT_NUM;
    
                    /* check if all extra drive selected for this rg
                     */
                    extra_drive_index++;
                    if (extra_drive_index == rg_config_p[rg_index].num_of_extra_drives)
                    {
                        break;
                    }
                } /* end if((max_rg_drive_capacity == unused_drive_capacity) */
    
            } /* end if (unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot!= FBE_INVALID_SLOT_NUM) */
    
        } /* end for(unused_drive_index =0; unused_drive_index <unused_pvd_locations.drive_counts[block_size][drive_type] ; unused_drive_index++) */

        /* Validate that sufficient `extra' drives were located.
         */
        if (extra_drive_index != rg_config_p[rg_index].num_of_extra_drives)
        {

            /* We had make sure that have enough drives during setup. Let's fail the test and see
             * why could not get the suitable extra drive
             */
            mut_printf(MUT_LOG_TEST_STATUS,  "Did not find unused pvd, block size %d, drive type %d, max rg drive capacity 0x%llx, unused drive cnt %d, rg index %d\n",
                block_size, drive_type, max_rg_drive_capacity, unused_pvd_locations.drive_counts[block_size][drive_type], rg_index );

            /* fail the test to figure out that why test did not able to find out the unused pvd */
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Did not find unused pvd while refresh extra disk info table\n");
        }

    } /* end for ( rg_index = 0; rg_index < raid_group_count; rg_index++) */

    return;
}
/******************************************
 * end scooby_doo_refresh_extra_disk_info()
 ******************************************/

/*!**************************************************************
 *          scooby_doo_get_next_position_to_remove()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param b_transition_pvd_fail - FBE_TRUE - Force PVD to fail 
 * @param b_pull_drive - FBE_TRUE - Pull and re-insert same drive
 *                       FBE_FALSE - Replace drive 
 * @param removal_mode - random or sequential
 *
 * @return status - Typically FBE_STATUS_OK,  Othersie we failed to
 *          locate a position to remove
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *  05/23/2011 - Modified. Amit Dhaduk
 *
 ****************************************************************/
static fbe_status_t scooby_doo_get_next_position_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t copy_position,
                                                           fbe_test_drive_removal_mode_t removal_mode,
                                                           fbe_u32_t *next_position_to_remove_p)
{
    fbe_status_t    status = FBE_STATUS_FAILED;
    fbe_u32_t       remove_index = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t       position_to_remove = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t       can_be_removed[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t       random_position = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t       removed_position;

    /* Initialize the position to remove to invalid.
     */
    *next_position_to_remove_p = SCOOBY_DOO_INVALID_DISK_POSITION;

    /* Check if there any position that haven't been removed.
     */
    if ((rg_config_p->width - rg_config_p->num_removed) == 0)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: width: %d minus num removed: %d is 0", 
                   __FUNCTION__, rg_config_p->width, rg_config_p->num_removed);

        /* There are no drives available to be removed.
        */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the copy_position.
     */
    if ((copy_position != FBE_RAID_MAX_DISK_ARRAY_WIDTH) &&
        (copy_position >= rg_config_p->width)               )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: copy pos: %d is greater or equal width: %d", 
                   __FUNCTION__, copy_position, rg_config_p->width);

        /* There are no drives available to be removed.
        */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /*! @note Currently we prevent the removal of the copy position.  Therefore
     *        validate that it isn't removed.
     */
    for (remove_index = 0; remove_index < rg_config_p->num_removed; remove_index++)
    {

        /* Sanity check that the removed position is present in the array.
         */
        MUT_ASSERT_TRUE(remove_index < rg_config_p->width)
        MUT_ASSERT_TRUE(rg_config_p->drives_removed_array[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION);

        removed_position = rg_config_p->drives_removed_array[remove_index];
        MUT_ASSERT_TRUE(removed_position < rg_config_p->width)

        /* Validate tha this isn't the copy positin.
         */
        MUT_ASSERT_INT_NOT_EQUAL(copy_position, removed_position);
    }

    /* Generate the can be removed array
     * First initialize all the positions to being present (but mark the
     * copy position as removed).
     */
    for (remove_index = 0; remove_index < rg_config_p->width; remove_index++)
    {
        if (remove_index == copy_position)
        {
            can_be_removed[remove_index] = SCOOBY_DOO_INVALID_DISK_POSITION;
        }
        else
        {
            can_be_removed[remove_index] = remove_index;
        }
    }

    /* For each position found on the drives removed array, set that position to
     * removed in our local array.
     */
    for (remove_index = 0; remove_index < rg_config_p->num_removed; remove_index++)
    {
        fbe_u32_t   removed_position;

        /* Sanity check that the removed position is present in the array.
         */
        MUT_ASSERT_TRUE(remove_index < rg_config_p->width)
        MUT_ASSERT_TRUE(rg_config_p->drives_removed_array[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION);

        removed_position = rg_config_p->drives_removed_array[remove_index];
        MUT_ASSERT_TRUE(removed_position < rg_config_p->width)

        can_be_removed[removed_position] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }

    /* Check which removal mode is requested.
     */
    switch (removal_mode)
    {
        case FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL:
            /* Loop through the 'can be removed' array to find the first position that can be removed.
             */
            remove_index = 0;
            while (remove_index != rg_config_p->width)
            {
                if (can_be_removed[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
                {
                    /* Found a position to be removed.
                     * Set position_to_remove to the position to be removed.
                     */
                    position_to_remove = can_be_removed[remove_index];
                    break;
                }
                remove_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_RANDOM:
            /* Pick a random number from the number of drives available to remove.
             * The number of drives available should be nonzero.
             */
            MUT_ASSERT_TRUE((rg_config_p->width - rg_config_p->num_removed) != 0);
            random_position = fbe_random() % (rg_config_p->width - rg_config_p->num_removed);

            /* Loop through until and skip 'random_position' number of available drives to
             * return a random drive.
             */
            remove_index = 0;
            while (remove_index != rg_config_p->width)
            {
                if (can_be_removed[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
                {
                    if (random_position == 0)
                    {
                        /* Found a position to be removed.
                         * Set position_to_remove to the position to be removed.
                         */
                        position_to_remove = can_be_removed[remove_index];
                        break;
                    }
                    else
                    {
                        random_position--;
                    }
                }
                remove_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_SPECIFIC:
            /* Loop through the 'can be removed' array to find the first position that can be removed.
             */
            remove_index = 0;
            for ( remove_index = 0; remove_index < rg_config_p->width; remove_index++)
            {                
                /* Found a position to be removed.
                 * Set position_to_remove to the position to be removed.
                 */
                position_to_remove = rg_config_p->specific_drives_to_remove[remove_index];                    
                if(position_to_remove == rg_config_p->drives_removed_array[remove_index])
                {                    
                    continue;
                }

               break;                               
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        default:
            /* Not a valid algorithm.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Invalid removal algorithm %d\n", 
                       __FUNCTION__, removal_mode);
            MUT_ASSERT_TRUE(FBE_FALSE);
            break;
    }

    MUT_ASSERT_TRUE(position_to_remove < rg_config_p->width);
    *next_position_to_remove_p = position_to_remove;

    return FBE_STATUS_OK;
}
/**********************************************
 * end scooby_doo_get_next_position_to_remove()
 **********************************************/

/*!**************************************************************
 * scooby_doo_remove_drives()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param b_shutdown_rg - FBE_TRUE - Pull sufficient drives to shutdown raid group
 *                       FBE_FALSE - Degrade raid group
 * @param b_transition_pvd_fail - FBE_TRUE - Force PVD to fail 
 * @param b_reinsert_same_drive - IGNORED, Always re-insert same drive
 * @param removal_mode - random or sequential
 *
 * @return None.
 *
 * @author
 *  09/18/2012  Ron Proulx  - Updated.
 *
 ****************************************************************/
static void scooby_doo_remove_drives(fbe_test_rg_configuration_t *rg_config_p, 
                                     fbe_u32_t raid_group_count,
                                     fbe_u32_t copy_position,
                                     fbe_bool_t b_shutdown_rg,
                                     fbe_bool_t b_transition_pvd_fail,
                                     fbe_bool_t b_reinsert_same_drive,
                                     fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t       position_to_remove;
    fbe_u32_t       drive_remove_index;
    fbe_u32_t       num_drives_to_remove_to_degrade_rg;
    fbe_u32_t       num_drives_to_remove_to_shutdown_rg;
    fbe_u32_t       num_drives_to_remove = 1;

    FBE_UNREFERENCED_PARAMETER(b_reinsert_same_drive);

    /* For every raid group remove one drive.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Determine how many drives to remove based on raid type.
         */
        switch(current_rg_config_p->raid_type)
        {
            case FBE_RAID_GROUP_TYPE_RAID1:
                /*! @note To be consistent with `big_bird_wait_for_rebuilds'
                 *        and the purpose of this test isn't to validate
                 *        rebuild but copy, we always remove exactly (1) drive.
                 */
                //num_drives_to_remove_to_degrade_rg = current_rg_config_p->width - 1;
                num_drives_to_remove_to_degrade_rg = 1;
                num_drives_to_remove_to_shutdown_rg = current_rg_config_p->width;
                break;

            case FBE_RAID_GROUP_TYPE_RAID10:
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
                /* 1 parity drive*/
                num_drives_to_remove_to_degrade_rg = 1;
                num_drives_to_remove_to_shutdown_rg = num_drives_to_remove_to_degrade_rg + 1;
                break;
   
            case FBE_RAID_GROUP_TYPE_RAID6:
                /*! @note To be consistent with `big_bird_wait_for_rebuilds'
                 *        and the purpose of this test isn't to validate
                 *        rebuild but copy, we always remove exactly (1) drive.
                 */
                // num_drives_to_remove_to_degrade_rg = 2;
                num_drives_to_remove_to_degrade_rg = 1;
                num_drives_to_remove_to_shutdown_rg = 2 + 1;
                break;

            default:
                /* Type not supported.*/
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: rg id: %d type: %d not supported", 
                   __FUNCTION__, current_rg_config_p->raid_group_id, current_rg_config_p->raid_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return;
        }

        /* For all the number of drives to remove.
         */
        num_drives_to_remove = (b_shutdown_rg == FBE_TRUE) ? num_drives_to_remove_to_shutdown_rg : num_drives_to_remove_to_degrade_rg;
        for (drive_remove_index = 0; drive_remove_index < num_drives_to_remove; drive_remove_index++)
        {
            /* Skip copy position
             */
            status = scooby_doo_get_next_position_to_remove(current_rg_config_p,
                                                            copy_position,
                                                            removal_mode,
                                                            &position_to_remove);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
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
                    status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                         drive_to_remove_p->enclosure, 
                                                         drive_to_remove_p->slot,
                                                         &current_rg_config_p->drive_handle[position_to_remove]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    if (fbe_test_sep_util_get_dualsp_test_mode())
                    {
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                        status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                             drive_to_remove_p->enclosure, 
                                                             drive_to_remove_p->slot,
                                                             &current_rg_config_p->peer_drive_handle[position_to_remove]);
                        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                    }
                }
                else
                {
                    status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                         drive_to_remove_p->enclosure, 
                                                         drive_to_remove_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
                    if (fbe_test_sep_util_get_dualsp_test_mode())
                    {
                        /* Set the target server to SPB and remove the drive there. */
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                        status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                                drive_to_remove_p->enclosure, 
                                                                drive_to_remove_p->slot);
                        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
                        /* Set the target server back to SPA. */
                        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                    }
                }
            }

        } /* For the number of drives to remove to degrade raid group*/

        /* Goto next raid group*/
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

        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              SCOOBY_DOO_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end scooby_doo_remove_drives()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_remove_all_drives()
 *****************************************************************************
 *
 * @brief   For all raid groups, remove a the number of drives specified but
 *          skip the copy position.
 *
 * @param   rg_config_p - Our configuration.
 * @param   raid_group_count - Number of rgs in config.
 * @param   copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param   b_shutdown_rg - FBE_TRUE - Shutdown raid group
 *                          FBE_FALSE - Degraded raid group
 * @param   b_reinsert_same_drive - FBE_TRUE - Original drive wil be re-inserted
 * @param   removal_mode - random or sequential
 *
 * @return None.
 *
 * @author
 *  09/18/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static void scooby_doo_remove_all_drives(fbe_test_rg_configuration_t *const rg_config_p,
                                  fbe_u32_t raid_group_count,
                                  fbe_u32_t copy_position,
                                  fbe_bool_t b_shutdown_rg,
                                  fbe_bool_t b_reinsert_same_drive,
                                  fbe_test_drive_removal_mode_t removal_mode)
{
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);


    if (b_reinsert_same_drive == FBE_FALSE)
    {
        /* Refresh the locations of the all extra drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        scooby_doo_refresh_extra_disk_info(rg_config_p, raid_group_count);
    }

    /* Remove the required number of drives
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s rg count: %d copy pos: %d shutdown: %d ==", 
               __FUNCTION__, raid_group_count, copy_position, b_shutdown_rg);
    scooby_doo_remove_drives(rg_config_p, raid_group_count, copy_position,
                             b_shutdown_rg,
                             FBE_FALSE,    /* don't fail pvd */
                             b_reinsert_same_drive,
                             removal_mode);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive(s) - Completed. ==", __FUNCTION__);

    return;
}
/******************************************
 * end scooby_doo_remove_all_drives()
 ******************************************/

/*!**************************************************************
 * scooby_doo_remove_copy_position()
 ****************************************************************
 * @brief
 *  remove drives, one per raid group.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 * @param copy_position - The position to remove
 * @param b_transition_pvd - True to not use terminator to reinsert.
 *
 * @return None.
 *
 * @author
 *  09/19/2012  Ron Proulx  - Updated.
 *
 ****************************************************************/
static void scooby_doo_remove_copy_position(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t raid_group_count,
                                            fbe_u32_t copy_position,
                                            fbe_bool_t b_transition_pvd_fail)
{
    fbe_u32_t rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_status_t status;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_remove = copy_position;
    fbe_bool_t  b_pull_drive = FBE_TRUE;

    /* For every raid group remove one drive.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
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
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
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
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
            else
            {
                status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    /* Set the target server to SPB and remove the drive there. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure, 
                                                            drive_to_remove_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
        }
        current_rg_config_p++;
    }

    return;
}
/******************************************
 * end scooby_doo_remove_copy_position()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_remove_copy_destination_drives()
 *****************************************************************************
 *
 * @brief   remove drives, one per raid group.
 * 
 * @param   rg_config_p - raid group config array.
 * @param   raid_group_count
 * @param   copy_position - The position to remove
 * @param   dest_array_p - Pointer to array of drive locations to remove
 *
 * @return None.
 *
 * @author
 *  03/07/2013  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static void scooby_doo_remove_copy_destination_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t raid_group_count,
                                                      fbe_u32_t copy_position,
                                                      fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status;
    fbe_u32_t       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_test_raid_group_disk_set_t *drive_to_remove_array_p = dest_array_p;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       position_to_remove = copy_position + 1;
    fbe_bool_t      b_pull_drive = FBE_TRUE;

    /* For every raid group remove one drive.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the location of the drive to remove*/
        drive_to_remove_p = drive_to_remove_array_p++;

        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Removing drive raid group: %d position: 0x%x (%d_%d_%d)", 
                   __FUNCTION__, rg_index, position_to_remove, 
                   drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot);

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
                                                     &scooby_doo_dest_handle_array[rg_index]);
            }
            else
            {
                status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                           drive_to_remove_p->enclosure, 
                                                           drive_to_remove_p->slot);
            }
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                if (1 || b_pull_drive)
                {
                    status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                         drive_to_remove_p->enclosure, 
                                                         drive_to_remove_p->slot,
                                                         &scooby_doo_dest_peer_handle_array[rg_index]);
                }
                else
                {
                    status = fbe_test_pp_util_remove_sas_drive(drive_to_remove_p->bus, 
                                                               drive_to_remove_p->enclosure, 
                                                               drive_to_remove_p->slot);
                }
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }
        else
        {
            status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                 drive_to_remove_p->enclosure, 
                                                 drive_to_remove_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                /* Set the target server to SPB and remove the drive there. */
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                        drive_to_remove_p->enclosure, 
                                                        drive_to_remove_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Set the target server back to SPA. */
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }
        
        /* Goto to next raid group
         */
        current_rg_config_p++;
    }

    return;
}
/*************************************************
 * end scooby_doo_remove_copy_destination_drives()
 *************************************************/

/*!**************************************************************
 * scooby_doo_insert_all_drives()
 ****************************************************************
 * @brief
 *  Remove all the drives for this test.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param copy_position - Copy position
 * @param b_is_rg_shutdown - Has the raid group been shutdown?
 * @param b_reinsert_same_drive - FBE_TRUE - Same drives being re-inserted
 * @param removal_mode - Re-insertion mode
 *
 * @return None.
 *
 * @author
 *  09/18/2012  Ron Proulx  - Updated.
 *
 ****************************************************************/
static void scooby_doo_insert_all_drives(fbe_test_rg_configuration_t *rg_config_p, 
                                         fbe_u32_t raid_group_count, 
                                         fbe_u32_t copy_position,
                                         fbe_bool_t b_is_rg_shutdown,
                                         fbe_bool_t b_reinsert_same_drive,
                                         fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_u32_t num_to_insert = 0;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;

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

    /* If we are inserting different drive use the `extra_disk_set'.
     */
    if (b_reinsert_same_drive == FBE_FALSE)
    {
        /* Populate the number of extra drive to allow rebuild.
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Refresh the location of the all drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

        /* Refresh the locations of the all extra drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

        /* to insert new drive, configure extra drive as spare 
         */
         fbe_test_sep_drive_configure_extra_drives_as_hot_spares(rg_config_p, raid_group_count);

        /* make sure that hs swap in complete 
         */        
         fbe_test_sep_drive_wait_extra_drives_swap_in(rg_config_p, raid_group_count);


         /* set target server to SPA (test expects SPA will be left as target server and previous call may change that) 
          */
         fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    else
    {

        /* insert the same drive back 
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s rg count: %d copy pos: %d is shutdown: %d ==", 
                   __FUNCTION__, raid_group_count, copy_position, b_is_rg_shutdown);
        scooby_doo_insert_drives(rg_config_p, 
                                 raid_group_count,
                                 copy_position,
                                 FBE_FALSE,         /* don't fail pvd */
                                 removal_mode);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s insert drives - Complete. ==", __FUNCTION__);
    }

    return;
}
/******************************************
 * end scooby_doo_insert_all_drives()
 ******************************************/

/*!**************************************************************
 * scooby_doo_insert_copy_position()
 ****************************************************************
 * @brief
 *  insert drives, one per raid group.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 * @param copy_position - The position to insert
 * @param b_transition_pvd - True to not use terminator to reinsert.
 *
 * @return None.
 *
 * @author
 *  09/19/2012  Ron Proulx  - Updated.
 *
 ****************************************************************/
static void scooby_doo_insert_copy_position(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t raid_group_count,
                                            fbe_u32_t copy_position,
                                            fbe_bool_t b_transition_pvd)
{
    fbe_u32_t index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t status;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_insert = copy_position;

    /* For every raid group insert one drive.
     */
    for ( index = 0; index < raid_group_count; index++)
    {

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
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
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                             drive_to_insert_p->enclosure, 
                                                             drive_to_insert_p->slot,
                                                             current_rg_config_p->peer_drive_handle[position_to_insert]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    /* Set the target server to SPB and insert the drive there. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                                drive_to_insert_p->enclosure, 
                                                                drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end scooby_doo_insert_copy_position()
 ******************************************/

/*!**************************************************************
 *          scooby_doo_set_debug_flags()
 ****************************************************************
 *
 * @brief   Set any debug flags for all the raid groups specified
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed            
 *
 * @return  None.
 *
 * @author
 * 03/16/2010   Ron Proulx - Created
 *
 ****************************************************************/
void scooby_doo_set_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                           fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;
    
    /* If debug is not enabled, simply return
     */
    if (scooby_doo_b_debug_enable == FBE_FALSE)
    {
        return;
    }
    
    /* Populate the raid group debug flags to the value desired.
     * (there can only be 1 set of debug flag override)
     */
    raid_group_debug_flags = scooby_doo_object_debug_flags;
        
    /* Populate the raid library debug flags to the value desired.
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(scooby_doo_library_debug_flags);
    
    /* For all the raid groups specified set the raid library debug flags
     * to the desired value.
     */
    for (rg_index_to_change = 0; rg_index_to_change < raid_group_count; rg_index_to_change++)
    {
        fbe_object_id_t                 rg_object_id;
        fbe_api_trace_level_control_t   control;
    
    /* Set the debug flags for raid groups that we are interested in.
     * Currently is the is the the 3-way non-optimal raid group.
     */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    
     /* Set the raid group debug flags
      */
     status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, 
                                                       raid_group_debug_flags);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     mut_printf(MUT_LOG_TEST_STATUS, "== %s set raid group debug flags to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_group_debug_flags, rg_object_id);
   
     /* Set the raid library debug flags
      */
     status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                         raid_library_debug_flags);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     mut_printf(MUT_LOG_TEST_STATUS, "== %s set raid library debug flags to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_library_debug_flags, rg_object_id);

     /* Set the state trace flags for this object (if enabled)
      */
     if (scooby_doo_enable_state_trace == FBE_TRUE)
     {
         control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
         control.fbe_id = rg_object_id;
         control.trace_level = FBE_TRACE_LEVEL_INVALID;
         control.trace_flag = FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE;
         status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);
         mut_printf(MUT_LOG_TEST_STATUS, "%s enabled lifecyclee trace flags for object: %d", 
                    __FUNCTION__, rg_object_id);
     }

        /* Goto to next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups specified */

    return;
}
/******************************************
 * end scooby_doo_set_debug_flags()
 ******************************************/

/*!**************************************************************
 *          scooby_doo_clear_debug_flags()
 ****************************************************************
 *
 * @brief   Clear any debug flags for all the raid groups specified
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed           
 *
 * @return  None.
 *
 * @author
 * 03/16/2010   Ron Proulx - Created
 *
 ****************************************************************/
void scooby_doo_clear_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                             fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;

    /* If debug is not enabled, simply return
     */
    if (scooby_doo_b_debug_enable == FBE_FALSE)
    {
        return;
    }

    /* For all the raid groups specified clear the raid library debug flags
     */
    for (rg_index_to_change = 0; rg_index_to_change < raid_group_count; rg_index_to_change++)
    {
        fbe_object_id_t                 rg_object_id;
    fbe_api_trace_level_control_t   control;

    /* Clear the debug flags for raid groups that we are interested in.
     */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Get the current value of the raid group debug flags.
     */
    status = fbe_api_raid_group_get_group_debug_flags(rg_object_id,
                                                      &raid_group_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear the raid group debug flags
     */
    status = fbe_api_raid_group_set_group_debug_flags(rg_object_id,
                                                      0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s raid group debug flags changed from: 0x%08x to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_group_debug_flags, 0, rg_object_id);

    /* Get the current value of the raid library debug flags.
     */
    status = fbe_api_raid_group_get_library_debug_flags(rg_object_id,
                                                        &raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear the raid library debug flags
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id,
                                                        0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s raid library debug flags changed from: 0x%08x to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_library_debug_flags, 0, rg_object_id);

    /* Clear the state trace (if enabled)
     */
    if (scooby_doo_enable_state_trace == FBE_TRUE)
    {
        control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
        control.fbe_id = rg_object_id;
        control.trace_level = FBE_TRACE_LEVEL_INVALID;
        control.trace_flag = 0;
        status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);
        mut_printf(MUT_LOG_TEST_STATUS, "%s disabled lifecycle trace flags for object: %d", 
                   __FUNCTION__, rg_object_id);
    }

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups */

    return;
}
/******************************************
 * end scooby_doo_clear_debug_flags()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_populate_destination_drives()
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
static fbe_status_t scooby_doo_populate_destination_drives(fbe_test_rg_configuration_t *rg_config_p,   
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
        scooby_doo_dest_array[dest_index] = current_rg_config_p->extra_disk_set[0];

        /* Goto next raid group.
         */
        current_rg_config_p++;
        dest_index++;
    }

    /* Update the address passed with the array of destination drives.
     */
    *dest_array_pp = &scooby_doo_dest_array[0];

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end scooby_doo_populate_destination_drives()
 ***************************************************************/

/*!***************************************************************************
 *          scooby_doo_cleanup_destination_drives()
 *****************************************************************************
 *
 * @brief   First disable `automatic' sparing.  Then determine which free drives 
 *          will be used for e
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   dest_array_p - Pointer to array of drive locations removed
 * @param   b_are_drives_removed - FBE_TRUE - The drives are removed and thus
 *              must be re-inserted before removing them from the array.
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/07/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_cleanup_destination_drives(fbe_test_rg_configuration_t *rg_config_p,   
                                                          fbe_u32_t raid_group_count,
                                                          fbe_test_raid_group_disk_set_t *dest_array_p,
                                                          fbe_bool_t b_are_drives_removed)         
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t        *current_rg_config_p = rg_config_p;
    fbe_u32_t                           rg_index;
    fbe_test_raid_group_disk_set_t     *dest_drive_p = NULL;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_api_terminator_device_handle_t  peer_drive_handle;
    fbe_object_id_t                     pvd_object_id = FBE_OBJECT_ID_INVALID;

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


    /* We removed (1) destination drive per raid group.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t wait_attempts = 0;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Step 1: Validate the destination drive (currently we don't allow
         *         system drives so checking against 0_0_0 is Ok)
         */
        dest_drive_p = &dest_array_p[rg_index];
        if ((dest_drive_p->bus == 0)       &&
            (dest_drive_p->enclosure == 0) &&
            (dest_drive_p->slot == 0)         )
        {
            /* Report the failure
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "==scooby doo cleanup destination drives: rg index: %d drive: %d_%d_%d not valid !!==",
                       rg_index, dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }

        /* Step 2: If the drives are removed re-insert them
         */
        if (b_are_drives_removed == FBE_TRUE)
        {
            /* Re-insert (and verify) all the removed drives
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "==scooby doo cleanup destination drives: rg index: %d re-insert drive: %d_%d_%d ==",
                       rg_index, dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot);

            /* inserting the same drive back */
            if (fbe_test_util_is_simulation())
            {
                /* If the handle is invalid report the error
                 */
                drive_handle = scooby_doo_dest_handle_array[rg_index];
                if (drive_handle == NULL)
                {
                    /* Fail
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, 
                               "==scooby doo cleanup destination drives: rg index: %d invalid drive handle==",
                               rg_index);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
                }

                status = fbe_test_pp_util_reinsert_drive(dest_drive_p->bus, 
                                                         dest_drive_p->enclosure, 
                                                         dest_drive_p->slot,
                                                         drive_handle);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    /* If the handle is invalid report the error
                     */
                    peer_drive_handle = scooby_doo_dest_peer_handle_array[rg_index];
                    if (peer_drive_handle == NULL)
                    {
                        /* Fail
                         */
                        status = FBE_STATUS_GENERIC_FAILURE;
                        mut_printf(MUT_LOG_TEST_STATUS, 
                               "==scooby doo cleanup destination drives: rg index: %d invalid peer drive handle==",
                               rg_index);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        return status;
                    }

                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive(dest_drive_p->bus, 
                                                             dest_drive_p->enclosure, 
                                                             dest_drive_p->slot,
                                                             peer_drive_handle);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(dest_drive_p->bus, 
                                                            dest_drive_p->enclosure, 
                                                            dest_drive_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    /* Set the target server to SPB and insert the drive there. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive_hw(dest_drive_p->bus, 
                                                                dest_drive_p->enclosure, 
                                                                dest_drive_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }

            /* Wait for object to be valid.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rg index:%d  drive: %d_%d_%d to become ready ==", 
                       __FUNCTION__, rg_index, 
                       dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot);
            status = FBE_STATUS_GENERIC_FAILURE;
            while ((status != FBE_STATUS_OK) &&
                   (wait_attempts < 20)         )
            {
                /* Wait until we can get pvd object id.
                 */
                status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_p->bus,
                                                                        dest_drive_p->enclosure,
                                                                        dest_drive_p->slot,
                                                                        &pvd_object_id);
                if (status != FBE_STATUS_OK)
                {
                    fbe_api_sleep(500);
                }
            }
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for %d_%d_%d to become ready - complete ==", 
                       __FUNCTION__,
                       dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot);

        } /* end if drives are removed */

        /* Step 3: Clear (i.e. mark as free) the destination array entry.
         */
        dest_drive_p->bus = 0;
        dest_drive_p->enclosure = 0;
        dest_drive_p->slot = 0;

    } /* end for all raid groups under test*/

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end scooby_doo_cleanup_destination_drives()
 ***************************************************************/

/*!***************************************************************************
 *          scooby_doo_proactive_shutdown_step1()
 *****************************************************************************
 *
 * @brief   This test populates all raid groups (LUNs) with a fixed pattern.  
 *          Then with fixed I/O running it initiates a proactive copy for each 
 *          raid group.  It then `pulls' sufficient drives to force the raid
 *          group to go shutdown.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 *
 * @return  None.
 *
 * @author
 *  03/14/2012   Ron Proulx - Created
 *
 *****************************************************************************/
static void scooby_doo_proactive_shutdown_step1(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   position_to_spare;

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    scooby_doo_write_fixed_pattern();

    /* Now start fixed I/O
     */
    scooby_doo_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Set debug flags.
     */
    scooby_doo_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    scooby_doo_wait_for_rdgen_start(SCOOBY_DOO_RDGEN_WAIT_SECS);

    /* Force (1) drive for each raid group to enter proactive copy
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

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* No pull the first drive in each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing sufficient drives to shutdown raid group ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 FBE_TRUE, /* Shutdown raid groups */
                                 FBE_TRUE, /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        scooby_doo_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }
    
    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors. we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }
    
    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all  drives. ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 FBE_TRUE,  /* Raid groups are shutdown */
                                 FBE_TRUE,  /* Re-insert removed drives*/
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
 
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);
    
    /* Make sure the fixed pattern can be read still.
     */
    scooby_doo_read_fixed_pattern();

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        scooby_doo_wait_for_rebuild_completion(current_rg_config_p,
                                               FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);

    /* Restart fixed I/O
     */
    scooby_doo_start_fixed_io(ran_abort_msecs);

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_spare,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_FALSE, /* Don't wait for swap out*/
                                                                          FBE_TRUE   /* Skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Wait for the proactive copy to cleanup
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy cleanup. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_copy_operation_cleanup(rg_config_p, 
                                                                raid_group_count,
                                                                position_to_spare,
                                                                FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                FBE_FALSE /* Cleanup has not been performed */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy cleanup. ==", __FUNCTION__);

    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    scooby_doo_clear_debug_flags();
    */

    return;
}
/******************************************
 * end scooby_doo_proactive_shutdown_step1()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_proactive_shutdown_step2()
 *****************************************************************************
 *
 * @brief   This test validates that pulling drives during a differential 
 *          rebuild doesn't cause error (rebuild request that hit dead positions
 *          would cause a deadlock with the monitor at one point in time).
 *          First we validate that rebuilds are active then we pull all drives
 *          to force the raid groups to be shutdown.  We then validate that
 *          the rebuilds complete without error.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group    
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void scooby_doo_proactive_shutdown_step2(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t luns_per_rg,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   position_to_spare;

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    scooby_doo_write_fixed_pattern();

    /* Now start fixed I/O
     */
    scooby_doo_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Set debug flags.
     */
    scooby_doo_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    scooby_doo_wait_for_rdgen_start(SCOOBY_DOO_RDGEN_WAIT_SECS);

    /* Force (1) drive for each raid group to enter proactive copy
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

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* No pull the first drive in each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing sufficient drives to shutdown raid group ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 FBE_TRUE, /* Shutdown raid groups */
                                 FBE_TRUE, /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        scooby_doo_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors. we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all  drives. ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 FBE_TRUE,  /* Raid groups are shutdown */
                                 FBE_TRUE,  /* Re-insert removed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {

        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Make sure the fixed pattern can be read still.
     */
    scooby_doo_read_fixed_pattern();

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        scooby_doo_wait_for_rebuild_completion(current_rg_config_p, 
                                               FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);

    /* Restart fixed I/O
     */
    scooby_doo_start_fixed_io(ran_abort_msecs);

    /* Wait for the proactive copy to complete
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

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    scooby_doo_clear_debug_flags();
    */
}
/*******************************************
 * end scooby_doo_proactive_shutdown_step2()
 *******************************************/

/*!***************************************************************************
 *          scooby_doo_proactive_shutdown_step3()
 *****************************************************************************
 *
 * @brief   This test validates that there are no media error after a raid group
 *          is shutdown with I/O in progress.  It also validates that a raid
 *          group can be quiesced with a differential rebuild active.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group   
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void scooby_doo_proactive_shutdown_step3(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t luns_per_rg,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   position_to_spare;

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    scooby_doo_write_fixed_pattern();

    /* Now start fixed I/O
     */
    scooby_doo_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Set debug flags.
     */
    scooby_doo_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    scooby_doo_wait_for_rdgen_start(SCOOBY_DOO_RDGEN_WAIT_SECS);

    /* Force (1) drive for each raid group to enter proactive copy
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

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* No pull the first drive in each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing sufficient drives to shutdown raid group ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 FBE_TRUE, /* Shutdown raid groups */
                                 FBE_TRUE, /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        scooby_doo_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors. we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives. ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 FBE_TRUE,  /* Raid groups are shutdown */
                                 FBE_TRUE,  /* Re-insert removed drives*/
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {

        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Make sure the fixed pattern can be read still.
     */
    scooby_doo_read_fixed_pattern();

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        scooby_doo_wait_for_rebuild_completion(current_rg_config_p,
                                               FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);

    /* Restart fixed I/O
     */
    scooby_doo_start_fixed_io(ran_abort_msecs);

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_spare,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_TRUE, /* Wait for swap out */
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    scooby_doo_clear_debug_flags();
    */
}
/******************************************
 * end scooby_doo_proactive_shutdown_step3()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_run_shutdown_test()
 *****************************************************************************
 *
 * @brief   This test executes the following:
 *          1.  Validate that no metadata errors are induced (including 512-bps
 *              raid groups) when all drives are sequentially removed from a
 *              mirror raid group.
 *              o Write a fixed pattern to all blocks
 *              o Start fixed I/O at queue depth
 *              o Pull all drives for each raid group (delay between each pull)
 *              o Re-insert all drives (delay between each insert)
 *              o Wait for raid group to become ready (but don't wait for rebuild)
 *              o Validate that all data can be read without error (even if stale)
 *              o Validate that rebuilds start
 *          2.  Validate that drive pulls during rebuild don't cause issues
 *              o Previously the raid group monitor would deadlock when a rebuild
 *                request hit a new dead position (since the monitor is waiting
 *                for the rebuild request it cannot process the edge state change
 *                yet the raid algorithms are waiting for the dead acknowledgement
 *                from the object).
 *              o Wait for rebuilds to complete
 *          3.  Validate no media errors with normal I/O with shutdown
 *              o Start normal I/O at queue depth
 *              o Pull all drives for each raid group (delay between each pull)
 *              o Re-insert all drives (delay between each insert)
 *              o Wait for raid group to become ready
 *              o Wait for rebuilds to complete (so that we can destroy the environment
 *                without any outstanding requests (considered a memory leak)
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed
 * @param   rdgen_operation - The rdgen operation (w/r/c etc)
 * @param   rdgen_pattern - LBA or zeros
 * @param   ran_abort_msecs - Invalid or random time to abort
 *
 * @return  None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from scooby_doo_test.c
 *
 *****************************************************************************/

static void scooby_doo_test_run_shutdown_tests(fbe_test_rg_configuration_t * const rg_config_p,
                                               fbe_u32_t raid_group_count,
                                               fbe_rdgen_operation_t rdgen_operation,
                                               fbe_rdgen_pattern_t rdgen_pattern,
                                               fbe_test_random_abort_msec_t ran_abort_msecs)
{
    /* Currently not referenced */
    FBE_UNREFERENCED_PARAMETER(rdgen_operation);
    FBE_UNREFERENCED_PARAMETER(rdgen_pattern);

    /* Step 1: Validate that no I/O errors are induced when a procative copy
     *         operation is interrupted with a shutdown.
     */
    scooby_doo_proactive_shutdown_step1(rg_config_p, raid_group_count, ran_abort_msecs);

    /* Step 2:  Validate that drive pulls during rebuild don't cause issues
     */
    /*! @todo this test needs to be re-worked */
    /*scooby_doo_degraded_shutdown_step2(rg_config_p, raid_group_count, luns_per_rg,
                                                            ran_abort_msecs);*/

    /* Step 3:  Validate no media errors with normal I/O with shutdown
     */
    /*! @todo this test needs to be re-worked */
    /*scooby_doo_degraded_shutdown_step3(rg_config_p, raid_group_count, luns_per_rg,
                                                            ran_abort_msecs);*/
   
    return;
}
/******************************************
 * end scooby_doo_test_run_shutdown_tests()
 ******************************************/

/*!**************************************************************
 * scooby_doo_lifecycle_state_notification_callback_function()
 ****************************************************************
 * @brief
 *  The the callback function, where the sem is release when a match found.
 *  The test that waits for this notification will be blocked until 
 *  either a match found or timeout reaches.
 *
 * @param update_object_msg  - message fron the notification.   
 * @param context  - notification context.
 *
 * @return None.
 *
 ****************************************************************/
static void scooby_doo_lifecycle_state_notification_callback_function(update_object_msg_t * update_object_msg, void * context)
{
    scooby_doo_lifecycle_state_ns_context_t *ns_context_p = (scooby_doo_lifecycle_state_ns_context_t * )context;
    fbe_status_t                            status;
    fbe_lifecycle_state_t                   state;
    fbe_time_t                              current_time;

    if ((update_object_msg->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE)
        && (update_object_msg->notification_info.object_type == ns_context_p->object_type))
    {
        status = fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type,
                                                                     &state);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        current_time = fbe_get_time();

        /* Lock the notifocation context, so only one notification can update it at a time*/
        fbe_spinlock_lock(&(ns_context_p->lock));

        /* If there is a valid `unexpected' state, then if we encounter that
         * it is an error.
         */
        if ((ns_context_p->unexpected_lifecycle_state != FBE_LIFECYCLE_STATE_INVALID) &&
            (state == ns_context_p->unexpected_lifecycle_state)                          ) {
            /* Unexpected state !
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "scooby_doo notification callback - Unexpected state: %d for object type: 0x%llx object id: 0x%x\n",
                       state, (unsigned long long)update_object_msg->notification_info.object_type,
                       update_object_msg->object_id);
            
            ns_context_p->b_unexpected_state_match = FBE_TRUE;
            MUT_ASSERT_TRUE_MSG(state != ns_context_p->unexpected_lifecycle_state, "Unexpected lifecycle state detected");
        } else if ((ns_context_p->expected_lifecycle_state != FBE_LIFECYCLE_STATE_INVALID) &&
                   (ns_context_p->expected_lifecycle_state == state)                          ) {
                ns_context_p->b_expected_state_match = FBE_TRUE;
                /* Release the semaphore */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "scooby_doo notification callback - Expected state: %d for object type: 0x%llx object id: 0x%x occurred\n",
                           state, (unsigned long long)update_object_msg->notification_info.object_type,
                           update_object_msg->object_id);
                fbe_semaphore_release(&(ns_context_p->sem), 0, 1, FALSE);
        } else {
            /* Else just trace the state change.
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "scooby_doo notification callback - object id: 0x%x state: %d for object type: 0x%llx occurred\n",
                       update_object_msg->object_id, state, (unsigned long long)update_object_msg->notification_info.object_type);
        }
        fbe_spinlock_unlock(&(ns_context_p->lock));
    }
    return;
}
/*************************************************************
 * end scooby_doo_lifecycle_state_notification_callback_function()
 *************************************************************/

/*!****************************************************************************
 *          scooby_doo_register_for_pdo_state_notifications()
 ******************************************************************************
 *
 * @brief   Register for ANY state notifications.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   user_lba_to_object_to - User space lba to inject to
 * @param   number_of_blocks_to_inject_for - Number of bloocks to inject for.
 * @param   num_times_to_inject - Number of times to inject
 *
 * @return  status
 *
 * @author
 *  03/03/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static void scooby_doo_register_for_pdo_state_notifications(fbe_lifecycle_state_t expected_lifecycle_state,
                                                            fbe_lifecycle_state_t unexpected_lifecycle_state,
                                                            fbe_u32_t timeout_ms)
{
    fbe_status_t                            status;
    scooby_doo_lifecycle_state_ns_context_t *ns_context_p = &scooby_doo_ns_context;

    /* Initialize locka and sem*/
    fbe_semaphore_init(&(ns_context_p->sem), 0, 1);
    fbe_spinlock_init(&(ns_context_p->lock));

    /* Initialize other fields
     */
    ns_context_p->object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context_p->expected_lifecycle_state = expected_lifecycle_state;
    ns_context_p->unexpected_lifecycle_state = unexpected_lifecycle_state;
    ns_context_p->b_expected_state_match = FBE_FALSE;
    ns_context_p->b_unexpected_state_match = FBE_FALSE;
    ns_context_p->wait_start_time = fbe_get_time();
    ns_context_p->timeout_in_ms = timeout_ms;

    /* Only register for state changes.
     */
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  ns_context_p->object_type,
                                                                  scooby_doo_lifecycle_state_notification_callback_function,
                                                                  ns_context_p,
                                                                  &ns_context_p->registration_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s notification successfully! ===\n", __FUNCTION__);
    return;
}
/*************************************************************
 * end scooby_doo_register_for_pdo_state_notifications()
 *************************************************************/

/*!****************************************************************************
 * scooby_doo_unregister_lifecycle_state_notification
 ******************************************************************************
 *
 * @brief
 *    This function unregister the notification and destroy the semaphore in the ns_context.
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    04/1/2010 - Created. guov
 *
 ******************************************************************************/
static void scooby_doo_unregister_lifecycle_state_notification(void)
{
    fbe_status_t                        status;
    scooby_doo_lifecycle_state_ns_context_t *ns_context_p = &scooby_doo_ns_context;

    status = fbe_api_notification_interface_unregister_notification(scooby_doo_lifecycle_state_notification_callback_function, 
                                                                    ns_context_p->registration_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK); 
    mut_printf(MUT_LOG_TEST_STATUS, "=== %s unregister Lifecycle State notification successfully! ===\n", __FUNCTION__);
    fbe_spinlock_destroy(&(ns_context_p->lock));
    fbe_semaphore_destroy(&(ns_context_p->sem));
    return;
}
/*************************************************************
 * end scooby_doo_unregister_lifecycle_state_notification()
 *************************************************************/

/*!****************************************************************************
 * scooby_doo_wait_lifecycle_state_notification()
 ******************************************************************************
 *
 * @brief
 *    This function waits and checkes for timeout.
 *
 * @param   ns_context  -  notification context
 *
 * @return   None
 *
 * @version
 *    03/15/2010 - Created.
 *
 ******************************************************************************/
static void scooby_doo_wait_lifecycle_state_notification(void)
{
    fbe_status_t       status;
    scooby_doo_lifecycle_state_ns_context_t *ns_context_p = &scooby_doo_ns_context;

    /* Only wait if the expected state is valid.
     */
    if (ns_context_p->expected_lifecycle_state != FBE_LIFECYCLE_STATE_INVALID) {
        status = fbe_semaphore_wait_ms(&(ns_context_p->sem), ns_context_p->timeout_in_ms);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Wait for lifecycle state notification timed out!");
        MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, ns_context_p->b_expected_state_match, "Lifecycle state did not match!");
    } else if (ns_context_p->unexpected_lifecycle_state != FBE_LIFECYCLE_STATE_INVALID) {
        /* Just sleep for the timeout to make sure the unexpected state is not 
         * encountered.
         */
        fbe_api_sleep(ns_context_p->timeout_in_ms);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_FALSE, ns_context_p->b_unexpected_state_match, "Lifecycle unexpected state found!");
    }
    return;
}
/*************************************************************
 * end scooby_doo_wait_lifecycle_state_notification()
 *************************************************************/

/*!****************************************************************************
 *          scooby_doo_start_error_injection()
 ******************************************************************************
 * @brief
 *  This function starts inserting errors to a PDO.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   user_lba_to_object_to - User space lba to inject to
 * @param   number_of_blocks_to_inject_for - Number of bloocks to inject for.
 * @param   num_times_to_inject - Number of times to inject
 *
 * @return  status
 *
 * @author
 *  03/03/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t scooby_doo_start_error_injection(fbe_test_rg_configuration_t *const rg_config_p,
                                                     fbe_u32_t raid_group_count,
                                                     fbe_u32_t position_to_inject,
                                                     fbe_lba_t user_lba_to_object_to,
                                                     fbe_block_count_t number_of_blocks_to_inject_for,
                                                     fbe_u32_t num_times_to_inject)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t                    *current_rg_config_p = rg_config_p;
    fbe_u32_t                                       rg_index;
    fbe_protocol_error_injection_error_record_t    *record_p = NULL;
    fbe_protocol_error_injection_record_handle_t   *record_handle_p = NULL;
    fbe_test_raid_group_disk_set_t                  disk_location;
    fbe_object_id_t                                 rg_object_id;
    fbe_api_raid_group_get_info_t                   rg_info;
    fbe_block_count_t                               blocks_to_inject_for = number_of_blocks_to_inject_for;

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        fbe_u32_t   rg_position_to_inject = position_to_inject;

        /* If this raid group is not enabled goto the next.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Determine logical range to inject for.
         */
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
            fbe_u32_t out_position_to_inject;

            if (position_to_inject == FBE_RAID_MAX_DISK_ARRAY_WIDTH) {
                rg_position_to_inject = 1;
            }
            sep_rebuild_utils_find_mirror_obj_and_adj_pos_for_r10(current_rg_config_p,
                                                                  rg_position_to_inject, 
                                                                  &rg_object_id, 
                                                                  &out_position_to_inject);

            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } else {
            if (position_to_inject == FBE_RAID_MAX_DISK_ARRAY_WIDTH) {
                if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) {
                    rg_position_to_inject = 1;
                } else {
                    rg_position_to_inject = current_rg_config_p->width - 1;
                }
            }
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* If the blocks to inject is max*/
        if (number_of_blocks_to_inject_for == FBE_LBA_INVALID) {
            blocks_to_inject_for = rg_info.paged_metadata_start_lba / rg_info.num_data_disk;
        }

        /* Invoke the method to initialize, populate and add the record.
         */
        disk_location = current_rg_config_p->rg_disk_set[rg_position_to_inject];
        record_p = &scooby_doo_protocol_error_injection_records[rg_index];
        record_handle_p = &scooby_doo_protocol_error_injection_record_handles_p[rg_index];
        status = fbe_test_neit_utils_populate_protocol_error_injection_record(disk_location.bus,
                                                                              disk_location.enclosure,
                                                                              disk_location.slot,
                                                                              record_p,
                                                                              record_handle_p,
                                                                              user_lba_to_object_to,
                                                                              blocks_to_inject_for,
                                                                              FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
                                                                              FBE_SCSI_READ_16,
                                                                              FBE_SCSI_SENSE_KEY_MEDIUM_ERROR,
                                                                              FBE_SCSI_ASC_READ_DATA_ERROR,
                                                                              FBE_SCSI_ASCQ_GENERAL_QUALIFIER,
                                                                              FBE_PORT_REQUEST_STATUS_SUCCESS, 
                                                                              num_times_to_inject);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove the record, modify it then add it again.
         */
        status = fbe_api_protocol_error_injection_remove_record(*record_handle_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        record_p->num_of_times_to_insert = 1;
        record_p->num_of_times_to_reactivate = num_times_to_inject;
        record_p->secs_to_reactivate = 0;   /*! @note Reactivate immediately */
        status = fbe_api_protocol_error_injection_add_record(record_p, record_handle_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto the next raid group.
         */
        current_rg_config_p++;

    } /* end for all raid groups*/

    /* Start the error injection */
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 

    return status;
}
/****************************************
 * end scooby_doo_start_error_injection()
 ****************************************/

/*!****************************************************************************
 *          scooby_doo_send_one_io()
 ******************************************************************************
 * @brief
 *  This function sends (1) I/O to the first LUN of raid group.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   user_lba_to_inject_to - User space lba to inject to
 * @param   number_of_blocks_to_inject_for - Number of bloocks to inject for.
 *
 * @return  status
 *
 * @author
 *  03/06/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t scooby_doo_send_one_io(fbe_test_rg_configuration_t *const rg_config_p,
                                           fbe_u32_t raid_group_count,
                                           fbe_rdgen_operation_t rdgen_operation,
                                           fbe_lba_t user_lba_to_inject_to,
                                           fbe_block_count_t number_of_blocks_to_inject_for)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t            *context_p = &scooby_doo_test_contexts[0];
    fbe_test_rg_configuration_t        *current_rg_config_p = rg_config_p;
    fbe_u32_t                           rg_index;
    fbe_object_id_t                     lun_object_id;
    fbe_api_rdgen_send_one_io_params_t  params;

    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list->lun_number, &lun_object_id);
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
        params.pattern = FBE_RDGEN_PATTERN_LBA_PASS;
        params.lba = user_lba_to_inject_to;
        params.blocks = number_of_blocks_to_inject_for;
        params.b_async = FBE_TRUE;
        status = fbe_api_rdgen_send_one_io_params(context_p, &params);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        context_p++;
        current_rg_config_p++;
    }

    return status;
}
/****************************************
 * end scooby_doo_send_one_io()
 ****************************************/

/*!****************************************************************************
 *          scooby_doo_wait_for_one_io_to_complete()
 ******************************************************************************
 * @brief
 *  This function wait for(1) I/O to the first LUN of raid group.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 *
 * @return  status
 *
 * @author
 *  03/06/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t scooby_doo_wait_for_one_io_to_complete(fbe_test_rg_configuration_t *const rg_config_p,
                                                           fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t        *context_p = &scooby_doo_test_contexts[0];
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;

    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Wait for I/O to finish.
         */
        fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_SEP_0, 1);

        status = fbe_api_rdgen_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        context_p++;
        current_rg_config_p++;
    }
    return status;
}
/****************************************
 * end scooby_doo_wait_for_one_io_to_complete()
 ****************************************/

/*!****************************************************************************
 *          scooby_doo_wait_for_error_injection()
 ******************************************************************************
 * @brief
 *  This function wait for the specified number of errors to be injected (for
 *  each drive specified).
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   num_times_to_inject - Number of times to inject
 * @param   timeout_msec - Total time to wait for ALL errors to be injected
 *
 * @return  status
 *
 * @author
 *  03/03/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t scooby_doo_wait_for_error_injection(fbe_test_rg_configuration_t *const rg_config_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_u32_t position_to_inject,
                                                        fbe_u32_t num_times_to_inject,
                                                        fbe_u32_t timeout_msec)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t                    *current_rg_config_p = rg_config_p;
    fbe_u32_t                                       rg_index;
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle;
    fbe_u32_t                                       total_time_ms = 0;
    fbe_u32_t                                       expected_num_of_errors = 0;

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    /* For all the raid groups under test add a media error injection record.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {
        fbe_u32_t rg_position_to_inject = position_to_inject;

        if (position_to_inject == FBE_RAID_MAX_DISK_ARRAY_WIDTH) {
            if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) {
                rg_position_to_inject = 1;
            } else if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) {
                rg_position_to_inject = 1;
            } else {
                rg_position_to_inject = current_rg_config_p->width - 1;
            }
        }
        /* If this raid group is not enabled goto the next.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }
    
        /* Wait for the specified number of errors to be injected
         */
        record_handle = scooby_doo_protocol_error_injection_record_handles_p[rg_index];
        status = fbe_api_protocol_error_injection_get_record(record_handle, &record);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /*! @note We use the re-activate with a time of 0 seconds.  Therefore
         *        The `times_inserted' goes to 0 immediately and the `times_reset'
         *        gets incremented.
         */
        if ((current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)      ||
            ((current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) &&
             (current_rg_config_p->width > 2)                                 )   ) {
            /* For 3-way mirrors we will simply switch positions.  Therefore only
             * (1) error is expected.
             */
            expected_num_of_errors = num_times_to_inject;
        } else if ((current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
                   (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)   ) {
            /* Mirror drop into region mode.  Use that multiplier.
             */
             expected_num_of_errors = (num_times_to_inject * SCOOBY_DOO_MIRROR_ERROR_INJECTION_MULTIPLIER);
        } else {
            /* Parity raid types do not drop into mining.
             */
            expected_num_of_errors = (num_times_to_inject * SCOOBY_DOO_PARITY_ERROR_INJECTION_MULTIPLIER);
        }
        while (record.times_reset < expected_num_of_errors) {
            if (total_time_ms > timeout_msec){
                MUT_FAIL_MSG("error not seen in expected time");
            }
            
            fbe_api_sleep(100);
            total_time_ms += 100;
            if ((total_time_ms % 1000) == 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "wait for error to be injected to obj: 0x%x rg id: %d pos: %d",
                           record.object_id, current_rg_config_p->raid_group_id, rg_position_to_inject);
            }
            status = fbe_api_protocol_error_injection_get_record(record_handle, &record);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto the next raid group.
         */
        current_rg_config_p++;

    } /* end for all raid groups*/


    return status;
}
/********************************************
 * end scooby_doo_wait_for_error_injection()
 ********************************************/

/*!****************************************************************************
 *          scooby_doo_disable_error_injection()
 ******************************************************************************
 * @brief
 *  Stop error injection and remove any records.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 *
 * @return  status
 *
 * @author
 *  03/06/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t scooby_doo_disable_error_injection(fbe_test_rg_configuration_t *const rg_config_p,  
                                                       fbe_u32_t raid_group_count)                      
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t                    *current_rg_config_p = rg_config_p;
    fbe_u32_t                                       rg_index;
    fbe_protocol_error_injection_record_handle_t    record_handle;

    /* First stop the error injection.
     */
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    for (rg_index = 0; rg_index < raid_group_count; rg_index++) {

        /* If this raid group is not enabled goto the next.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p)) {
            current_rg_config_p++;
            continue;
        }

        /* Invoke the method to remove record
         */
        record_handle = scooby_doo_protocol_error_injection_record_handles_p[rg_index];
        status = fbe_api_protocol_error_injection_remove_record(record_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Goto next*/
        current_rg_config_p++;
    }

    return status;
}
/****************************************
 * end scooby_doo_disable_error_injection()
 ****************************************/

/*!****************************************************************************
 *          scooby_doo_inject_errors()
 ******************************************************************************
 * @brief
 *  This function starts inserting errors to a PDO.
 *
 * @param   rg_config_p - Pointer to array of array groups under test
 * @param   raid_group_count - Number of raid groups under test
 * @param   position_to_inject - Position in all raid groups to inject to.
 * @param   rdgen_operation - rdgen opcode to inject on
 * @param   user_lba_to_inject_to - User space lba to inject to
 * @param   number_of_blocks_to_inject_for - Number of bloocks to inject for.
 * @param   num_times_to_inject - Number of times to inject
 *
 * @return  status
 *
 * @author
 *  03/03/2015  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t scooby_doo_inject_errors(fbe_test_rg_configuration_t *const rg_config_p,
                                             fbe_u32_t raid_group_count,
                                             fbe_u32_t position_to_inject,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_lba_t user_lba_to_inject_to,
                                             fbe_block_count_t number_of_blocks_to_inject_for,
                                             fbe_u32_t num_times_to_inject)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       num_of_error_injected = 0;

    /* Validate that we haven't exceeded the maximum number of raid groups under test.
     */
    MUT_ASSERT_TRUE(raid_group_count <= FBE_TEST_MAX_RAID_GROUP_COUNT);

    /*! @note We actually loop over the error not the raid groups.  This is due
     *        to the way the PDO thresholds work:
     *
     *          1. We cannot inject `quickly' (within 100ms) otherwise the
     *             `burst' prevention will increase the threshold.
     *
     *          2. We cannot allow successful I/Os otherwise the ratio will
     *             fade.
     *
     *        Therefore here is the sequence for each raid group:
     *          1. Create and add record for each raid group that will inject
     *             the specified number of errors on every I/O.
     *
     *          2. Send exactly (1) read I/O to the associated PDOs.
     *                  
     */

    /* Step 1: Add and start error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start error injection ==", __FUNCTION__);
    status = scooby_doo_start_error_injection(rg_config_p, raid_group_count, position_to_inject,
                                              user_lba_to_inject_to,
                                              number_of_blocks_to_inject_for,
                                              num_times_to_inject);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* For each injection required send (1) I/O, wait for injection, wait 100ms.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s For each error: Send 1 I/O, Wait for error, Delay ==", __FUNCTION__);
    for (num_of_error_injected = 0; num_of_error_injected < num_times_to_inject; num_of_error_injected++)
    {
        /* Step 2: Send One I/O to the block specified.
         */
        status = scooby_doo_send_one_io(rg_config_p, raid_group_count,
                                        rdgen_operation,
                                        user_lba_to_inject_to,
                                        number_of_blocks_to_inject_for);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step 3: Wait for the I/O to complete.
         */
        status = scooby_doo_wait_for_one_io_to_complete(rg_config_p, raid_group_count);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step 4: Validate that expected number of errors has been injected.
         */
        status = scooby_doo_wait_for_error_injection(rg_config_p, raid_group_count,
                                                     position_to_inject,
                                                     1,                     /*! @note Only inject (1) error per pass */
                                                     SCOOBY_DOO_WAIT_MSEC);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

        /* Step 5: Sleep for a short period.
         */
        fbe_api_sleep(100);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection complete ==", __FUNCTION__);

    /* Stop the error injection */
    status = scooby_doo_disable_error_injection(rg_config_p, raid_group_count);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return status;
}
/****************************************
 * end scooby_doo_inject_errors()
 ****************************************/

/*!***************************************************************************
 *          scooby_doo_test_paco_r_validate_emeh_thresholds()
 *****************************************************************************
 *
 * @brief   Disable EOL generation then degraded raid group and confirm that
 *          Extended Media Error Handing (EMEH) is implemented by injecting
 *          errors on read to non-degraded position.
 *              1) Start I/O
 *              2) Disable EOL posting
 *              3) Degraded raid groups
 *              4) Inject errors to non-degraded position
 *              5) Uncorrectable media errors are expected
 *              6) Validate that no drives are failed
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   rdgen_operation - rdgen operation
 * @param   rdgen_pattern - LBA, zero etc
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @return None.
 *
 * @note    All copy operations to degraded raid groups will fail.  Since the
 *          background operation utility used doesn't allow a failure, we only
 *          degraded the raid group after the copy operation is started.
 *
 * @author
 * 03/02/2015   Ron Proulx - Created.
 *
 *****************************************************************************/
static void scooby_doo_test_paco_r_validate_emeh_thresholds(fbe_test_rg_configuration_t * const rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_rdgen_operation_t rdgen_operation,
                                                            fbe_rdgen_pattern_t rdgen_pattern,
                                                            fbe_test_random_abort_msec_t ran_abort_msecs,
                                                            fbe_bool_t b_use_original_drives)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   times_to_inject = 120;
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   position_to_inject;

    /* Change the permanent spare timer back to a large value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(10000); /* large hotspare timeout */

    /* Wait for zeroing and initial verify to finish. 
     * Otherwise we will see our tests hit unexpected results when the initial verify gets kicked off. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* scooby_doo_test_paco_r injects error purposefully so we do not want to run any kind of Monitor IOs to run during the test 
      * disable sniff and background zeroing to run at PVD level
      */
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();

    /* Step 1: Write background pattern.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        scooby_doo_write_zero_background_pattern();
    }
    else
    {
        scooby_doo_write_background_pattern();
    }

    /* Step 2: Enable the Extended Media Error Handling.
     */
    status = fbe_test_sep_raid_group_class_set_emeh_values(
                                  FBE_RAID_EMEH_MODE_ENABLED_NORMAL,    /*! @note Enable EMEH at the raid group class*/
                                                           FBE_FALSE,   /*! @note Do not change thresholds */
                                                           0,           /*! @note No change to defaault threshold increase */
                                                           FBE_FALSE    /*! Don't persist the changes */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Remove drives.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_inject = FBE_RAID_MAX_DISK_ARRAY_WIDTH; /*! Inject into LBA 0*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_inject,
                                 FBE_FALSE,             /* Degrade raid groups */
                                 b_use_original_drives, /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);


    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Validate that the EMEH thresholds have been disabled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that EMEH thresholds have been disabled ==", __FUNCTION__);
    status = fbe_test_sep_raid_group_validate_emeh_values(rg_config_p, raid_group_count, position_to_remove,
                                                          FBE_FALSE,    /* We do not expect the default values*/
                                                          FBE_TRUE,     /* We expect that the thresholds have been disabled */
                                                          FBE_FALSE      /* We do not expect the values to be increased */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Disable posting of EOL.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Disable EOL posting ==", __FUNCTION__);
    status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_PFA_HANDLING, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Register to catch any transtion to the Failed state (unexpected).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Register to catch any new Failed PDOs ==", __FUNCTION__);
    scooby_doo_register_for_pdo_state_notifications(FBE_LIFECYCLE_STATE_INVALID,    /* There is no expected state*/
                                                    FBE_LIFECYCLE_STATE_FAIL,       /* The Failed state is unexpected */
                                                    20000);                         /* 20 seconds */

    /* Step 8: Setup and start protocol error injection.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Start error injection ==", __FUNCTION__);
    status = scooby_doo_inject_errors(rg_config_p, raid_group_count,
                                      position_to_inject,
                                      rdgen_operation,
                                      0,                /*! @note Inject to starting LBA*/
                                      1,                /*! @note Only inject on (1) block */
                                      times_to_inject   /*! @todo This should be more programatic ! */);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Step 9: Verify that the expected state does not occur.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validiate no PDOs are Failed ==", __FUNCTION__);
    scooby_doo_wait_lifecycle_state_notification();
    scooby_doo_unregister_lifecycle_state_notification();

    /* Step 10: Re-insert the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-insert removed drives ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_inject,
                                 FBE_FALSE,             /* Raid group is not shutdown */
                                 b_use_original_drives, /* Replace failed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-insert drives: Successful. ==", __FUNCTION__);

    /* Step 11: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Step 12: Validate that the thresholds have been restored for each drive position
     *         for each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that EMEH thresholds have been restored ==", __FUNCTION__);
    status = fbe_test_sep_raid_group_validate_emeh_values(rg_config_p, raid_group_count, position_to_remove,
                                                          FBE_TRUE,    /* We do expect the default values*/
                                                          FBE_FALSE,   /* We do not expect that the thresholds have been disabled */
                                                          FBE_FALSE    /* We don't expect the values to be increased */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 13: Restore the EMEH values back to the default (either enabled or disabled)
     */
    status = fbe_test_sep_raid_group_class_set_emeh_values(FBE_RAID_EMEH_MODE_DEFAULT,  /*! @note Restore EMEH to defaults*/
                                                           FBE_FALSE,                   /*! @note Ignored */
                                                           0,                           /*! @note Ignored */
                                                           FBE_FALSE                    /*! Don't persist the changes */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 14: Re-enable EOL positing.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-enable EOL posting ==", __FUNCTION__);
    status = fbe_api_drive_configuration_set_control_flag(FBE_DCS_PFA_HANDLING, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 15: Restore permanent spare trigger to low value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Step 16: Restore background services.
     */
    fbe_test_sep_drive_enable_background_zeroing_for_all_pvds();
    fbe_test_sep_drive_enable_sniff_verify_for_all_pvds();

    return;
}
/********************************************************
 * end scooby_doo_test_paco_r_validate_emeh_thresholds()
 ********************************************************/

/*!***************************************************************************
 *          scooby_doo_test_degraded_user_proactive_copy()
 *****************************************************************************
 *
 * @brief   Raid groups are already degraded.  Start a user proactive copy then
 *          insert the drives.
 *              1) Start I/O
 *              2) Start a user proactive copy operation
 *              3) Replace the previously removed drives (i.e. full rebuild)
 *              4) Wait for copy operation to complete
 *              5) Wait for rebuilds to complete
 *              6) Stop I/O and validate no errors
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   rdgen_operation - rdgen operation
 * @param   rdgen_pattern - LBA, zero etc
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @return None.
 *
 * @note    All copy operations to degraded raid groups will fail.  Since the
 *          background operation utility used doesn't allow a failure, we only
 *          degraded the raid group after the copy operation is started.
 *
 * @author
 * 04/22/2012   Ron Proulx - Created.
 *
 *****************************************************************************/
static void scooby_doo_test_degraded_user_proactive_copy(fbe_test_rg_configuration_t * const rg_config_p,
                                                         fbe_u32_t raid_group_count,
                                                         fbe_rdgen_operation_t rdgen_operation,
                                                         fbe_rdgen_pattern_t rdgen_pattern,
                                                         fbe_test_random_abort_msec_t ran_abort_msecs,
                                                         fbe_bool_t b_use_original_drives)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    fbe_u32_t                   position_to_copy;
    fbe_u32_t                   position_to_remove;

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting degraded I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS);
    scooby_doo_start_io(rdgen_operation,rdgen_pattern,scooby_doo_threads,SCOOBY_DOO_MAX_IO_SIZE_BLOCKS,ran_abort_msecs);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting degraded I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);
    
    /* Step 2: Start a user proactive copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_copy,
                                                              copy_type,
                                                              NULL /* Don't specify destination */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive started - successfully. ==", __FUNCTION__);

    /* Step 3: Remove drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE,             /* Degrade raid groups */
                                 b_use_original_drives, /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);


    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Replace the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE,             /* Raid group is not shutdown */
                                 b_use_original_drives, /* Replace failed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives: Successful. ==", __FUNCTION__);

    /* Step 6: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Set debug flags.
     */
    scooby_doo_set_debug_flags(rg_config_p, raid_group_count);

    /* Step 7: Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_copy,
                                                                          copy_type,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for some time.
     */
    fbe_api_sleep(scooby_doo_io_msec_long);   

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O: Successful ==", __FUNCTION__);

    /* Step 8: Validate no errors. We can expect abort failures if we have injected them
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    return;
}
/****************************************************
 * end scooby_doo_test_degraded_user_proactive_copy()
 ****************************************************/

/*!***************************************************************************
 *          scooby_doo_test_proactive_copy_passive_io()
 *****************************************************************************
 *
 * @brief   Write a background pattern to the raid group, start proactive copy,
 *          pull drives, make sure we can read the pattern back degraded.
 *              1) Start proactive copy
 *              2) Degrade raid group
 *              3) Wait for proactive copy to complete
 *              4) Wait for proactive copy cleanup (clear EOL)
 *              5) Move all the spare back to the spare pool
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   rdgen_operation - The rdgen operation
 * @param   rdgen_pattern - LBA or zeros
 * @param   ran_abort_msecs - invalid or random abort time
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @return None.
 *
 * @note    All copy operations to degraded raid groups will fail.  Since the
 *          background operation utility used doesn't allow a failure, we only
 *          degraded the raid group after the copy operation is started.
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void scooby_doo_test_proactive_copy_passive_io(fbe_test_rg_configuration_t * const rg_config_p,
                                                      fbe_u32_t raid_group_count,
                                                      fbe_rdgen_operation_t rdgen_operation,
                                                      fbe_rdgen_pattern_t rdgen_pattern,
                                                      fbe_test_random_abort_msec_t ran_abort_msecs,
                                                      fbe_bool_t b_use_original_drives)
{
    fbe_status_t                status;
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_u32_t                   position_to_remove = SCOOBY_DOO_INVALID_DISK_POSITION;
    fbe_u32_t                   position_to_copy = SCOOBY_DOO_INVALID_DISK_POSITION;

    /* Currently unreferenced */
    FBE_UNREFERENCED_PARAMETER(rdgen_pattern);
    FBE_UNREFERENCED_PARAMETER(ran_abort_msecs);

    /* We write the entire raid group and read it all back after the 
     * rebuild has finished. 
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        scooby_doo_write_zero_background_pattern();
    }
    else
    {
        scooby_doo_write_background_pattern();
    }
 
    /* Step 1: Start the proactive copy (the background ops will automatically
     *         set the `allow automatic sparing' for the associated virtual drive).
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_copy,
                                                              copy_type,
                                                              NULL /* The system selects the destination drives*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__); 
     
    /* Make sure the background pattern can be read still.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how SCOOBY_DOO_FIXED_PATTERN currently defined)here. 
         * If SCOOBY_DOO_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        scooby_doo_read_fixed_pattern();
    }
    else
    {
        scooby_doo_read_background_pattern();
    }
 
    /* Step 2: Remove drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE,             /* Degrade raid groups */
                                 b_use_original_drives, /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);

    /* Step 3: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Set debug flags.
     */
    scooby_doo_set_debug_flags(rg_config_p, raid_group_count);

    /* Step 4: Replace the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Raid groups are not shutdown */
                                 FBE_FALSE, /* Replace failed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives: Successful. ==", __FUNCTION__);

    /* Step 5: Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_copy,
                                                                          copy_type,
                                                                          FBE_FALSE, /* Don't wait for swap out*/
                                                                          FBE_TRUE   /* Skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Make sure the background pattern can be read still.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how SCOOBY_DOO_FIXED_PATTERN currently defined)here. 
         * If SCOOBY_DOO_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        scooby_doo_read_fixed_pattern();
    }
    else
    {
        scooby_doo_read_background_pattern();
    }

    /* Step 6: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Make sure the background pattern can be read still.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how SCOOBY_DOO_FIXED_PATTERN currently defined)here. 
         * If SCOOBY_DOO_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        scooby_doo_read_fixed_pattern();
    }
    else
    {
        scooby_doo_read_background_pattern();
    }

    /* Step 7: Wait for the proactive copy to cleanup
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy cleanup. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_copy_operation_cleanup(rg_config_p, 
                                                                raid_group_count,
                                                                position_to_copy,
                                                                copy_type,
                                                                FBE_FALSE /* Cleanup has not been performed */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy cleanup - successful ==", __FUNCTION__);
    return;
}
/******************************************************
 * end scooby_doo_test_proactive_copy_passive_io()
 ******************************************************/

/*!***************************************************************************
 *          scooby_doo_test_user_copy_with_full_rebuild()
 *****************************************************************************
 *
 * @brief   Start with raid groups non-degraded.  Degraded the raid groups then
 *          start a user copy.  Re-insert the removed drives.  First wait for
 *          rebuild to complete then wait for copy to complete.
 *              1) Start I/O
 *              2) Start a user copy operation
 *              3) Remove drives
 *              4) Replace removed drives (i.e. full rebuild)
 *              5) Wait for rebuild to complete
 *              6) Wait for the user copy operation to complete
 *              7) Validate no errors
 *              8) Move the spares back to the automatic mode
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   rdgen_operation - The rdgen operation (w/r/ c etc)
 * @param   rdgen_pattern - LBA or zeros 
 * @param   ran_abort_msecs - Invalid or random abort time
 *
 * @return  None.
 *
 * @author
 * 04/22/2012   Ron Proulx - Created.
 *
 *****************************************************************************/
static void scooby_doo_test_user_copy_with_full_rebuild(fbe_test_rg_configuration_t * const rg_config_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_rdgen_pattern_t rdgen_pattern,
                                                        fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   position_to_copy;
    fbe_test_raid_group_disk_set_t *dest_array_p = NULL;

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS);
    scooby_doo_start_io(rdgen_operation,rdgen_pattern,scooby_doo_threads,SCOOBY_DOO_MAX_IO_SIZE_BLOCKS,ran_abort_msecs);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 2: Start a user copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    status = scooby_doo_populate_destination_drives(rg_config_p, raid_group_count, &dest_array_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting user copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_copy,
                                                              copy_type,
                                                              dest_array_p /* Specify the destination drives */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__);

    /* Step 3: Remove drives.
     */

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Degrade raid groups */
                                 FBE_FALSE, /* This is a full rebuild test: replace drives*/
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Replace the previously removed drives (i.e. force a full rebuild)
     */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives ==", __FUNCTION__);
    //scooby_doo_insert_all_removed_drives(rg_config_p, raid_group_count, 1,
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Raid groups are not shutdown */
                                 FBE_FALSE, /* Replace failed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives: Successful. ==", __FUNCTION__);

    /* Step 6: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);
    
    /* Step 7: Wait for the user copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_copy,
                                                                          copy_type,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);
  
    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 7: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(7) ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(7): Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Clear debug flags.
     */
    scooby_doo_clear_debug_flags(rg_config_p, raid_group_count);

    /* Step 8: Cleanup the destination drives
     */
    status = scooby_doo_cleanup_destination_drives(rg_config_p, raid_group_count,
                                                   dest_array_p, /* Array of destination drive locations*/
                                                   FBE_FALSE /* The drives are not removed*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Move all the drive to the automatic spare for the next test.
     */
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/***************************************************
 * end scooby_doo_test_user_copy_with_full_rebuild()
 ***************************************************/

/*!***************************************************************************
 *          scooby_doo_test_remove_source_drive()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a copy operation.
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
 * @param   rdgen_operation - The rdgen I/O operation.  For instance w/r/c.
 * @param   rdgen_pattern - LBA or zero
 * @param   ran_abort_msecs - Determines if we will randomly inject aborts or not
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
 *  09/19/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_test_remove_source_drive(fbe_test_rg_configuration_t *const rg_config_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_rdgen_pattern_t rdgen_pattern,
                                                        fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_u32_t       position_to_remove;
    fbe_u32_t       position_to_copy;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t       percentage_rebuilt_before_pause = 5;
    fbe_sep_package_load_params_t scooby_doo_raid_group_debug_hooks = { 0 };

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS);
    scooby_doo_start_io(rdgen_operation,rdgen_pattern,scooby_doo_threads,SCOOBY_DOO_MAX_IO_SIZE_BLOCKS,ran_abort_msecs);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 2: Start a proactive copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;

    /* Step 3: Remove drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Degrade raid groups */
                                 FBE_TRUE,  /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Set `pause' to stop copy operation when EOL is detected.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 4: Validate that EOL is set.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_MARKED_EOL;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test*/
                                                               NULL      /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate EOL - successful. ==", __FUNCTION__); 

    /* Step 5: Replace the previously removed drives.  We need to let the raid
     *          group become non-degraded since the copy cannot proceed until
     *          the raid groups are optimal.
     */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Raid groups are not shutdown */
                                 FBE_TRUE,  /* Re-insert removed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives: Successful. ==", __FUNCTION__);

    /* Step 6: Set `pause' to stop copy operation when copy is 5% complete
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 5% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 7: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Step 8: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &scooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 9: Set the `need replacement drive' timer to (5) seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Step 10: Set a debug hook when virtual drive enters eval rl and is broken
     *          At the same time, set a debug hook on the RG objects so that
     *          we can hit the debug hook of the VD before RG cleans the VD's 
     *          rebuild info for the destination drive (AR554994)
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_ALLOW_CONTINUE_BROKEN,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_LOG,
                                                               FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = scooby_doo_test_pause_set_debug_hook_for_rgs(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_raid_group_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                               FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Pull the source drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position: %d ==", __FUNCTION__, position_to_copy);
    scooby_doo_remove_copy_position(rg_config_p, raid_group_count, position_to_copy,
                                    FBE_FALSE /* Don't transition pvd to fail*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position - complete==", __FUNCTION__);

    /* Step 12: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 13: Validate that the virtual drive runs eval rl and is broken
     *          Then we remove the previousely added debug hooks for the RGs
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    status = scooby_doo_test_pause_wait_and_remove_debug_hook_for_rgs(rg_config_p, raid_group_count,
                                                               &scooby_doo_raid_group_debug_hooks,
                                                               FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    

    /* Step 14: Validate that the virtual drive completes the copy operation
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
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__);

    /* Step 15: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Step 16: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O: Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Restore the spare swap-in timer back to the test value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Return the execution status.
     */
    return status;
}
/************************************************************
 * end scooby_doo_test_remove_source_drive()
 ************************************************************/

/*!***************************************************************************
 *          scooby_doo_test_remove_source_drive_double_degraded()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a copy operation.  It 
 *          waits for the copy operation to be aborted then removes a second
 *          position in the parent raid group.
 *
 * @param   rg_config_p - Array of raid groups test configurations to run against
 * @param   raid_group_count - Number of raid groups under test
 * @param   rdgen_operation - The rdgen I/O operation.  For instance w/r/c.
 * @param   rdgen_pattern - LBA or zeros
 * @param   ran_abort_msecs - Determines if we will randomly inject aborts or not
 *
 * @return  status
 *
 * @note    This test only works for single SP and RAID-6.
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
 *  10/07/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_test_remove_source_drive_double_degraded(fbe_test_rg_configuration_t *const rg_config_p,
                                                                        fbe_u32_t raid_group_count,
                                                                        fbe_rdgen_operation_t rdgen_operation,
                                                                        fbe_rdgen_pattern_t rdgen_pattern,
                                                                        fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             removed_vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             copy_vd_object_id = FBE_OBJECT_ID_INVALID; 
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_u32_t                   position_to_remove;
    fbe_u32_t                   position_to_copy;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t                   percentage_rebuilt_before_pause = 5;

    /*! @note This test only works on (1) raid group of raid type RAID-6.
     */
    MUT_ASSERT_INT_EQUAL(1, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_RAID_GROUP_TYPE_RAID6, rg_config_p->raid_type);
    
    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS);
    scooby_doo_start_io(rdgen_operation,rdgen_pattern,scooby_doo_threads,SCOOBY_DOO_MAX_IO_SIZE_BLOCKS,ran_abort_msecs);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 2: Start a proactive copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;

    /* Step 3: Get the object ids for the raid group, position to remove and
     *         copy position.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &removed_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &copy_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step 4: Set `pause' to stop copy operation when EOL is detected.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 5: Validate that EOL is set.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_MARKED_EOL;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test*/
                                                               NULL      /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate EOL - successful. ==", __FUNCTION__); 

    /* Step 6: Set `pause' to stop copy operation when copy is 5% complete
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 5% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 7: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &scooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 9: Set the `need replacement drive' timer to (5) seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Step 10: Set a debug hook when virtual drive has:
     *              1. Requested `abort copy'
     *              2. Permission granted (parent raid group is not degraded)
     *              3. Abort copy is complete
     *              4. Attempts to exit the failed state
     */
    status = fbe_test_add_debug_hook_active(copy_vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
                                            0, 
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Set a debug hook when the removed position is re-inserted.
     */
    status = fbe_test_add_debug_hook_active(removed_vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
                                            0, 
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 12: Pull the source drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position: %d ==", __FUNCTION__, position_to_copy);
    scooby_doo_remove_copy_position(rg_config_p, raid_group_count, position_to_copy,
                                    FBE_FALSE /* Don't transition pvd to fail*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position - complete==", __FUNCTION__);

    /* Step 13: Wait for the copy virtual drive to hit the `exit failed state'
     *          hook.
     */
    status = fbe_test_wait_for_debug_hook_active(copy_vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0,
                                                 0); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 14: Pull a second position in the raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing second drive. ==", __FUNCTION__);
    big_bird_remove_drives(rg_config_p, 1,
                           FBE_FALSE,   /* Don't transition pvd */
                           FBE_TRUE,  /* We plan on re-inserting this drive later. */ 
                           FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing second drive: Successful. ==", __FUNCTION__);

    /* Step 15: Set a debug hook when the parent raid group runs clear rl
     */
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL, 
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED,
                                            0, 
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 16: Re-insert the removed position
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-insert second drive. ==", __FUNCTION__);
    big_bird_insert_drives(rg_config_p, 1,
                           FBE_FALSE    /* Don't transition pvd */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-insert second drive: Successful. ==", __FUNCTION__);

    /* Step 17: Wait for the removed virtual drive to hit the `exit failed state'
     *          hook.
     */
    status = fbe_test_wait_for_debug_hook_active(removed_vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0,
                                                 0); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 18: Now remove both the copy exit failed and removed exit failed
     *          hooks.
     */
    status = fbe_test_del_debug_hook_active(copy_vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_del_debug_hook_active(removed_vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EXIT_FAILED_STATE,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 19: Immediately clear the clear rl hook.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL, 
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 20: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Step 21: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O: Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Step 22: Validate that the virtual drive completes the copy operation
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
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__);

    /* Restore the spare swap-in timer back to the test value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Return the execution status.
     */
    return status;
}
/************************************************************
 * end scooby_doo_test_remove_source_drive_double_degraded()
 ************************************************************/

/*!***************************************************************************
 *          scooby_doo_test_degraded_remove_reinsert_source_drive()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a copy operation when the
 *          parent raids group is degraded. 
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
 * @param   rdgen_operation - The rdgen I/O operation.  For instance w/r/c.
 * @param   rdgen_pattern - LBA or zeros
 * @param   ran_abort_msecs - Determines if we will randomly inject aborts or not
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
 *  09/14/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_test_degraded_remove_reinsert_source_drive(fbe_test_rg_configuration_t *const rg_config_p,
                                                                          fbe_u32_t raid_group_count,
                                                                          fbe_rdgen_operation_t rdgen_operation,
                                                                          fbe_rdgen_pattern_t rdgen_pattern,
                                                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_u32_t       position_to_remove;
    fbe_u32_t       position_to_copy;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t       percentage_rebuilt_before_pause = 5;
    fbe_bool_t      b_event_found = FBE_FALSE;

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS);
    scooby_doo_start_io(rdgen_operation,rdgen_pattern,scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS,ran_abort_msecs);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 2: Start a user copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;

    /* Step 3: Set `pause' to stop copy operation when virtual drive enters 
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
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__);

    /* Step 4: Start copy operation.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_MODE_SET_TO_MIRROR;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate mirror mode - successful. ==", __FUNCTION__); 

    /* Step 5: Set `pause' to stop copy operation when copy is 5% complete
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 5% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 6: Set a debug hook to log when the virtual drive gets `no permission'
     *         The `no permission' status will prevent the copy from transitioning
     *         to the next state.
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                               FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_LOG,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &scooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8: Remove drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    scooby_doo_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Degrade raid groups */
                                 FBE_TRUE,  /* We plan on re-inserting this drive later. */ 
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);

    /* Step 9: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 10: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 12: Validate that the virtual drive get a `no permission' from raid group.
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 13: Set a debug hook when virtual drive enters `Failed' state
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 14: Set the `need replacement drive' timer to (5) seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Step 15: Set a rebuild mark NR done hook in parent
     */
    fbe_test_sep_rebuild_set_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 16: Pull the source drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position: %d ==", __FUNCTION__, position_to_copy);
    scooby_doo_remove_copy_position(rg_config_p, raid_group_count, position_to_copy,
                                    FBE_FALSE /* Don't transition pvd to fail*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position - complete==", __FUNCTION__);

    /* Step 17: Validate that the virtual drive goes to failed.  Then 
     *         remove the debug hook.
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 18: Set a debug hook when virtual drive gets `abort copy denied'
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_LOG,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 19: Validate that all raid groups goto fail
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that all raid groups goto fail ==", __FUNCTION__);
    status = fbe_test_sep_wait_for_rg_objects_fail(rg_config_p, raid_group_count,
                                                   FBE_FALSE /* This is not a shutdown test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that all raid groups goto fail - successful==", __FUNCTION__);

    /* Step 20: Validate that the virtual drive gets `denied' when requesting `abort copy'
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate `abort copy denied' state ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate `abort copy denied' state - success ==", __FUNCTION__);

    /* Step 21: Validate `abort copy' denied event
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate `abort copy denied' event ==", __FUNCTION__);
    status = fbe_test_sep_event_validate_event_generated(SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         5000 /* Wait up to 5000ms for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate `abort copy denied' event - success ==", __FUNCTION__);

    /* Step 22: Set a debug hook when virtual drive exists the failed state
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 23: Re-insert the source drives.  The source drive must be re-inserted
     *          since even if the non-copy drives are re-inserted they have been
     *          marked `rebuild logging' and will need to be rebuilt.  But the
     *          source drive is NOT marked `rebuild logging' since it was pulled
     *          AFTER the non-copy drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting source drive for copy position: %d ==", __FUNCTION__, position_to_copy);
    scooby_doo_insert_copy_position(rg_config_p, raid_group_count, position_to_copy,
                                    FBE_FALSE /* Don't transition pvd*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting source drive for copy position - complete==", __FUNCTION__);

    /* Step 24: Validate that the virtual drive exists the failed state.
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 25: Wait for parent raid group mark NR
     */
    fbe_test_sep_rebuild_wait_for_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 26: Validate that no chunks were mark NR in the parent raid group
     *          for the copy position.
     */
    if ((rdgen_operation == FBE_RDGEN_OPERATION_READ_ONLY) ||
        (rdgen_operation == FBE_RDGEN_OPERATION_READ_CHECK)   ) {
        fbe_test_sep_rebuild_validate_raid_group_position_not_marked_nr(rg_config_p, raid_group_count, position_to_copy);
    }

    /* Step 27: Remove the eval mark NR done hook.
     */
    fbe_test_sep_rebuild_del_raid_group_mark_nr_done_hook(rg_config_p, raid_group_count);

    /* Step 28: Replace the previously removed drives.  We need to let the raid
     *          group become non-degraded since the copy cannot proceed until
     *          the raid groups are optimal.
     */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Raid groups are not shutdown */
                                 FBE_TRUE,  /* Re-insert removed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives: Successful. ==", __FUNCTION__);

    /* Step 29: Validate that the raid groups are degraded.
     */
    //fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
    //                                            position_to_remove);

    /* Step 30: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Step 31: Validate that the virtual drive completes the copy operation
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
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test. */
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__);

    /* Step 32: Remove any `local' debug hooks.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing `local' debug hooks ==", __FUNCTION__);
    status = scooby_doo_clear_all_hooks();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing `local' debug hooks - Successful==", __FUNCTION__);

    /* Step 33: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O - Successful ==", __FUNCTION__);

    /*! @note We shutdown the raid group with I/O outstanding so we expect errors.
     *        But all the errors should be `Failed I/O' (i.e. no media errors etc).
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, context_p->start_io.statistics.io_failure_error_count);
        if (context_p->start_io.status.status == FBE_STATUS_EDGE_NOT_ENABLED)
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
        }
        else
        {
            MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
        }
    }
    else
    {
        MUT_ASSERT_TRUE(context_p->start_io.statistics.error_count == (context_p->start_io.statistics.io_failure_error_count + context_p->start_io.statistics.aborted_error_count));
        if (context_p->start_io.status.status == FBE_STATUS_EDGE_NOT_ENABLED)
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
        }
        else
        {
            MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
        }
    }

    /*! @todo Step 34: Due to the fact that we unquiesce after setting the
     *                 rebuild checkpoint to the end marker (which results
     *                 in the source drive being set rebuild logging and the
     *                 checkpoint for the source drive being 0) we will get
     *                 an error for a metadata I/O which results in the vd
     *                 go thru active and the parent raid group needs to 
     *                 rebuild.  When the code is fixed to not unquiesce after
     *                 setting he rebuild checkpoint to the end marker remove
     *                 this code.
     */ 
    mut_printf(MUT_LOG_TEST_STATUS, "== %s TODO: Waiting for rebuild due to unquiesce - FIX ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 2);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s TODO: Waiting for rebuild due to unquiesce - FIX - Successful ==", __FUNCTION__);

    /* Restore the spare swap-in timer back to the test value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Return the execution status.
     */
    return status;
}
/************************************************************
 * end scooby_doo_test_degraded_remove_reinsert_source_drive()
 ************************************************************/

/*!***************************************************************************
 *          scooby_doo_test_remove_destination_and_source_drives()
 *****************************************************************************
 *
 * @brief   This test removes both the destination and source drives for a copy
 *          operation (in that order).  It validates the following:
 *          o Destination Drive Fails.
 *              + Quiesce I/O
 *              + Virtual drive remains in Ready state
 *              + Destination position is marked rebuild logging
 *              + Chunks for all write request are marked NR (`Needs Rebuild')
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
 * @param   rdgen_operation - The rdgen I/O operation.  For instance w/r/c.
 * @param   rdgen_pattern - LBA or zeros
 * @param   ran_abort_msecs - Determines if we will randomly inject aborts or not
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
 *  09/21/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_test_remove_destination_and_source_drives(fbe_test_rg_configuration_t *const rg_config_p,
                                                                         fbe_u32_t raid_group_count,
                                                                         fbe_rdgen_operation_t rdgen_operation,
                                                                         fbe_rdgen_pattern_t rdgen_pattern,
                                                                         fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_rdgen_context_t    *context_p = &scooby_doo_test_contexts[0];
    fbe_spare_swap_command_t copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    fbe_u32_t       position_to_remove;
    fbe_u32_t       position_to_copy;
    fbe_test_sep_background_copy_state_t current_copy_state;
    fbe_test_sep_background_copy_state_t copy_state_to_pause_at;
    fbe_test_sep_background_copy_event_t validation_event;
    fbe_u32_t       percentage_rebuilt_before_pause = 5;
    fbe_test_raid_group_disk_set_t *dest_array_p = NULL;

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, scooby_doo_threads, SCOOBY_DOO_MAX_IO_SIZE_BLOCKS);
    scooby_doo_start_io(rdgen_operation,rdgen_pattern,scooby_doo_threads,SCOOBY_DOO_MAX_IO_SIZE_BLOCKS,ran_abort_msecs);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(scooby_doo_io_msec_short);

    /* Step 2: Start a proactive copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove;  /*! @note The source drive will be removed.*/
    status = scooby_doo_populate_destination_drives(rg_config_p, raid_group_count, &dest_array_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Set `pause' to stop copy operation when copy is 5% complete
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
                                                               &scooby_doo_pause_params,
                                                               percentage_rebuilt_before_pause,  /* Stop when 5% of consumed rebuilt*/
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               dest_array_p /* This is a copy to*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set pause when %d percent rebuilt - successfully. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 5: Set the `need replacement drive' timer to a `long' time.
     *         If we don't set it to a long time the virtual drive will
     *         automatically give up after the `spare trigger time' when
     *         the destination drive is pulled.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Step 7: Wait for all the raid groups to hit the copy (rebuild) hook
     */
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &scooby_doo_pause_params,
                                                         FBE_FALSE /* This is not a reboot test*/);

    /* Step 5: Pull the destination drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing destination drive for copy position: %d ==", __FUNCTION__, position_to_copy);
    scooby_doo_remove_copy_destination_drives(rg_config_p, raid_group_count, position_to_copy,
                                              dest_array_p /* Array of destination drive locations*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing destination drive for copy position - complete==", __FUNCTION__);

    /* Step 6: Validate than none of the raid groups are degraded.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that none of the raid groups are degraded==", __FUNCTION__);
    status = fbe_test_sep_validate_raid_groups_are_not_degraded(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate that none of the raid groups are degraded - complete==", __FUNCTION__);

    /* Step 7: Set a debug hook when virtual drive enters `Failed' state
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8: Pull the source drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position: %d ==", __FUNCTION__, position_to_copy);
    scooby_doo_remove_copy_position(rg_config_p, raid_group_count, position_to_copy,
                                    FBE_FALSE /* Don't transition pvd to fail*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing source drive for copy position - complete==", __FUNCTION__);

    /* Step 9: Validate that the raid groups are degraded.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate raid groups are degraded ==", __FUNCTION__);
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate raid groups are degraded - successful ==", __FUNCTION__);

    /* Step 10: Validate virtual drive hits 5% copied hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_DESIRED_PERCENTAGE_REBUILT;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test*/
                                                               dest_array_p /* This is a copy to*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate %d percent rebuilt complete - successful. ==", __FUNCTION__, percentage_rebuilt_before_pause);

    /* Step 11: Validate that the virtual drive goes to failed.  Then 
     *         remove the debug hook.
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12: Set a debug hook when virtual drive gets `abort copy granted'
     */
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &scooby_doo_debug_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_LOG,
                                                               FBE_FALSE    /* This is not a reboot test*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 13: Set the `need replacement drive' timer to (5) seconds.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Step 14: Validate that the virtual drive is allowed to abort the copy operation
     */
    status = fbe_test_sep_background_pause_check_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                        &scooby_doo_debug_hooks,
                                                                        FBE_FALSE    /* This is not a reboot test*/);

    /* Step 15: Validate that the virtual drive completes the copy operation
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
                                                               &scooby_doo_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* This is not a reboot test */
                                                               dest_array_p /* This is a copy to*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy cleanup - successful. ==", __FUNCTION__);

    /* Step 16: Cleanup the destination drives
     */
    status = scooby_doo_cleanup_destination_drives(rg_config_p, raid_group_count,
                                                   dest_array_p, /* Array of destination drive locations*/
                                                   FBE_TRUE /* The drives were removed*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 17: Add drives to the `test spare' pool and so that raid groups
     *          rebuild.
     */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replace failed position with spare ==", __FUNCTION__);
    scooby_doo_insert_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 FBE_FALSE, /* Raid groups are not shutdown */
                                 FBE_FALSE, /* Replace the failed drives */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replace failed position with spare - success ==", __FUNCTION__);

    /* Step 18: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Step 19: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O: Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /*! @note Since this is a degraded test suite we assume that we will be
     *        degrading the raid groups.  Since the test assume a specific
     *        drive will be spared, disable automatic sparing and then move
     *        all the spare drives to the automatic spare pool.
     */
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the spare swap-in timer back to the test value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Return the execution status.
     */
    return status;
}
/************************************************************
 * end scooby_doo_test_remove_destination_and_source_drives()
 ************************************************************/

/*!**************************************************************
 *          scooby_doo_run_tests_for_config()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param raid_group_count - Total number of raid groups.
 * @param luns_per_rg - Luns to create per rg. 
 * @param ran_abort_msecs - Anort ms if enabled         
 *
 * @return none
 *
 * @author
 *  03/09/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
void scooby_doo_run_tests_for_config(fbe_test_rg_configuration_t *rg_config_p, 
                                     fbe_u32_t raid_group_count, 
                                     fbe_u32_t luns_per_rg,
                                     fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_u32_t                   rg_index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_bool_t                  b_is_dualsp_test_mode = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_rdgen_pattern_t         rdgen_pattern = FBE_RDGEN_PATTERN_INVALID;

    /*! @note Since this is a degraded test suite we assume that we will be
     *        degrading the raid groups.  Since the test assume a specific
     *        drive will be spared, disable automatic sparing and then move
     *        all the spare drives to the automatic spare pool.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize out `short' and `long' I/O times
     */
    scooby_doo_set_io_seconds();

    /* Write a zero pattern so that Test 1 can validate data.
     */
    rdgen_pattern = FBE_RDGEN_PATTERN_ZEROS;
    scooby_doo_write_zero_background_pattern();

    /*! @note Copy operations (of any type) are not supported for RAID-0 raid groups.
     *        Therefore skip these tests if the raid group type is RAID-0.  
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        /* Print a message and return.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Skipping test with copy_type: %d.  Copy operations not supported on RAID-0 ==",
                   __FUNCTION__, copy_type);
        return;
    }

    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    mut_printf(MUT_LOG_TEST_STATUS, "%s Running for test level %d", __FUNCTION__, test_level);

    /* If we are doing shutdown only, then skip the degraded tests.
     */
    if (!fbe_test_sep_util_get_cmd_option("-shutdown_only"))
    {

        /* Test 1: After copy is in progress remove different drive then remove
         *         and re-insert source drive for copy operation
         *
         * Invoke method that will run the rebuild tests for all active raid groups
         */
        scooby_doo_test_degraded_remove_reinsert_source_drive(rg_config_p, raid_group_count,
                                                              FBE_RDGEN_OPERATION_READ_CHECK,
                                                              rdgen_pattern, 
                                                              ran_abort_msecs); 


         /* Test 2: Proactive Copy with Resilience (PACO-R).  Confirm that
          *         Extended Media Error Handling (EMEH) thresholds are increased
          *         when a raid group is degraded.
          *
          * When raid groups are degraded the media thresholds (before drive is shot)
          * will be increased.  Issue read I/O.
          */
         scooby_doo_test_paco_r_validate_emeh_thresholds(rg_config_p, raid_group_count,
                                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                                         ran_abort_msecs,
                                                         FBE_TRUE /* Re-insert original drives */); 

        /* Test 3: Proactive Copy without I/O active
         *
         * Run a test where we write a pattern, degrade the raid group by pulling 
         * random drive(s), and then read back while degraded.  
         */
        scooby_doo_test_proactive_copy_passive_io(rg_config_p, raid_group_count,
                                                  FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                  FBE_RDGEN_PATTERN_LBA_PASS,
                                                  ran_abort_msecs, 
                                                  FBE_FALSE /* Do not re-insert drives */);

        /* Test 4: Proactive Copy with I/O active
         *
         * With the raid groups already degraded start I/O, then start and user
         * proactive copy.
         */
         scooby_doo_test_degraded_user_proactive_copy(rg_config_p, raid_group_count,
                                                      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                      FBE_RDGEN_PATTERN_LBA_PASS,
                                                      ran_abort_msecs,
                                                      FBE_FALSE /* Do not re-insert drives */); 

         /* Test 5: Full Rebuild with User Copy
          *
          * Invoke method that will run the rebuild tests for all active raid groups
          */
         scooby_doo_test_user_copy_with_full_rebuild(rg_config_p, raid_group_count,
                                                     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                     FBE_RDGEN_PATTERN_LBA_PASS,
                                                     ran_abort_msecs); 

         /* Test 6: After copy is in progress remove the source drive.
          *
          * Invoke method that will run the rebuild tests for all active raid groups
          */
         scooby_doo_test_remove_source_drive(rg_config_p, raid_group_count,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             FBE_RDGEN_PATTERN_LBA_PASS, 
                                             ran_abort_msecs);

         /* Test 6a: Remove source and make raid group double degraded.
          */
         current_rg_config_p = rg_config_p;
         for (rg_index = 0; rg_index < raid_group_count; rg_index++)
         {
             /* We only run this test on RAID-6.
              */
             if ((b_is_dualsp_test_mode == FBE_FALSE)                          &&
                 (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)    )
             {
                 scooby_doo_test_remove_source_drive_double_degraded(current_rg_config_p, 1,
                                                                     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                                     FBE_RDGEN_PATTERN_LBA_PASS,
                                                                     ran_abort_msecs);

                 /* We are done.
                  */
                 break;
             }

             /* Goto next raid group config.
              */
             current_rg_config_p++;
         }

         /* Test 7: First remove the destination drive and validate raid groups are not
          *         degraded.  Then remove source drive and validate spare swaps-in and
          *         raid group is rebuilt.
          *
          * Invoke method that will run the rebuild tests for all active raid groups
          */
         scooby_doo_test_remove_destination_and_source_drives(rg_config_p, raid_group_count, 
                                                              FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                              FBE_RDGEN_PATTERN_LBA_PASS,
                                                              ran_abort_msecs); 

    }
 
    /*! @todo Other tests are disabled.
     */
    if (scooby_doo_b_are_non_simple_tests_disabled_due_to_DE572 == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Shutdown test disabled due to DE572", __FUNCTION__);
    }
    else
    {
        /* Test 5: Shutdown raid groups with I/O
         *
         * Invoke method that will run I/O and then force a shutdown
         */
        scooby_doo_test_run_shutdown_tests(rg_config_p, raid_group_count,
                                           FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                           FBE_RDGEN_PATTERN_LBA_PASS, 
                                           ran_abort_msecs);
    }

    /* Restore the spare swap-in timer back to the default value
     */
	fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    return;
}
/******************************************
 * end scooby_doo_run_tests_for_config()
 ******************************************/

/*!***************************************************************************
 *          scooby_doo_test_pause_set_debug_hook_for_rgs()
 *****************************************************************************
 *
 * @brief   Set the requested debug hooks for the RG objects
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   hook_array_p - Pointer to array of hooks to use
 * @param   monitor_state - which master hook wanna add
 * @param   monitor_substate- which sub hook wanna add
 * @param   val1, val2 - the value provided to the hook
 * @param   check_type - do what check in the hook
 * @param   action - do what when the hook is triggered
 * @param   b_is_reboot_test - whether to reserve the hook after reboot
 *
 * @return  fbe_status_t 
 *
 * @author
 *  04/27/2013  Zhipeng Hu  - Created.
 *
 *****************************************************************************/
static fbe_status_t scooby_doo_test_pause_set_debug_hook_for_rgs(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position,
                                                               fbe_sep_package_load_params_t *hook_array_p,
                                                               fbe_u32_t monitor_state,
                                                               fbe_u32_t monitor_substate,
                                                               fbe_u64_t val1,
                                                               fbe_u64_t val2,
                                                               fbe_u32_t check_type,
                                                               fbe_u32_t action,
                                                               fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_base_config_downstream_object_list_t downstream_object_list;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Adding debug hook not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > SCOOBY_DOO_MAX_RAID_GROUPS_PER_TEST)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, SCOOBY_DOO_MAX_RAID_GROUPS_PER_TEST);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(rg_object_id, FBE_OBJECT_ID_INVALID);

        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_u32_t mirror_index;
        
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);
            /* For a raid 10 the downstream object list has the IDs of the mirrors. 
             * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
             */
            mirror_index = position / 2;
            rg_object_id = downstream_object_list.downstream_object_list[mirror_index];
        }

        status = fbe_test_sep_background_pause_set_debug_hook(&hook_array_p->scheduler_hooks[rg_index], rg_index,
                                                              rg_object_id,
                                                              monitor_state,
                                                              monitor_substate,
                                                              val1, val2, 
                                                              check_type,
                                                              action,
                                                              b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Set pause hook on raid: 0x%x successfully.",
                   __FUNCTION__, rg_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************
 * end scooby_doo_test_pause_set_debug_hook_for_rgs()
 *********************************************************/


/*!***************************************************************************
*          scooby_doo_test_pause_wait_and_remove_debug_hook_for_rgs()
*****************************************************************************
*
* @brief   Wait and remove the requested debug hooks for the RG objects.
*
* @param   rg_config_p - Array of raid group information  
* @param   raid_group_count - Number of valid raid groups in array 
* @param   hook_array_p - Pointer to array of hooks to use
* @param   b_is_reboot_test - whether to reserve the hook after reboot
*
* @return  fbe_status_t 
*
* @author
*  04/27/2013  Zhipeng Hu  - Created.
*
*****************************************************************************/
static fbe_status_t scooby_doo_test_pause_wait_and_remove_debug_hook_for_rgs(fbe_test_rg_configuration_t *rg_config_p,
                                                                             fbe_u32_t raid_group_count,
                                                                             fbe_sep_package_load_params_t *sep_params_p,
                                                                             fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = 1200;
    fbe_bool_t                              raid_groups_complete[SCOOBY_DOO_MAX_RAID_GROUPS_PER_TEST];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;

    /* Get `this' and other SP.
    */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Validate the debug hooks.
    */
    if ((sep_params_p == NULL)                                                ||
        (sep_params_p->scheduler_hooks[0].object_id == 0)                     ||
        (sep_params_p->scheduler_hooks[0].object_id == FBE_OBJECT_ID_INVALID)    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
            "%s: Debug hook: %p unexpected ", 
            __FUNCTION__, sep_params_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the transport server to the correct SP.
    */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
    */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
    */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {    
            /* Skip raid groups that have completed the event.
            */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }
            
            /* Skip raid groups that are not enabled.
            */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Get the rg object id
            */
            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Check if we have reached the debug hook.
            */
            status = fbe_test_sep_background_pause_check_debug_hook(&sep_params_p->scheduler_hooks[rg_index],
                                                                    &b_is_debug_hook_reached);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If we have reached the debug hook log a message.
            */
            if (b_is_debug_hook_reached == FBE_TRUE)
            {
                /* Flag the debug hook reached.
                */
                mut_printf(MUT_LOG_TEST_STATUS, 
                    "%s: Hit debug hook raid obj: 0x%x",
                    __FUNCTION__, rg_object_id);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;

                /* Immediately remove the debug hook.
                */
                status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                    b_is_reboot_test);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            }

            /* Goto next raid group
            */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
        */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(100);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
    */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
        */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
            */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
            */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Display some information
            */
            mut_printf(MUT_LOG_TEST_STATUS, 
                "fbe_test_sep_background_pause: Timed out debug hook state: %d substate: %d raid: 0x%x",
                sep_params_p->scheduler_hooks[rg_index].monitor_state,
                sep_params_p->scheduler_hooks[rg_index].monitor_substate,
                rg_object_id);

            /* Goto the next raid group
            */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Set the SP back to the original.
        */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_TIMEOUT;
    }

    /* Set the transport server to `this SP'.
    */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************************************
* end fbe_test_sep_background_pause_wait_for_debug_hook()
******************************************************************/


/*!**************************************************************
 *          scooby_doo_run_tests()
 ****************************************************************
 * @brief
 *  We create a config and run the rebuild tests.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  08/20/2010  Ron Proulx  - Created from big_bird_run_tests
 *
 ****************************************************************/
static void scooby_doo_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       raid_group_count = 0;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t    random_abort_msec = FBE_TEST_RANDOM_ABORT_TIME_INVALID;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Configure the dualsp mode.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_TRUE, FBE_TEST_SEP_IO_DUAL_SP_MODE_PEER_ONLY);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {       
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* If the test level is greater than 0, abort I/O every (2) seconds.
     */
    if (test_level > 0)
    {
        random_abort_msec = FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC;
        MUT_ASSERT_TRUE(random_abort_msec >= 1000);
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "==%s test level: %d injects aborts every: %d seconds ==", 
                   __FUNCTION__, test_level, (random_abort_msec / 1000));
    }

    /* Now run the test for the current set of raid groups.
     */
    scooby_doo_run_tests_for_config(rg_config_p, raid_group_count, 
                                    SCOOBY_DOO_LUNS_PER_RAID_GROUP, 
                                    random_abort_msec /* Depending on the test level we will randomly abort I/O*/);

    /* Complete.
     */
    return;
}
/**************************************
 * end scooby_doo_run_tests()
 **************************************/

/*!**************************************************************
 * scooby_doo_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from scooby_doo_test.c
 *
 ****************************************************************/

void scooby_doo_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        raid_group_types = SCOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        raid_group_types = SCOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES;
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
            rg_config_p = &scooby_doo_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            rg_config_p = &scooby_doo_raid_groups_qual[raid_group_type_index][0];
        }

        if (raid_group_type_index + 1 >= raid_group_types) {
           /* Scooby doo will use hot-spares */
           scooby_doo_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, scooby_doo_run_tests,
                                       SCOOBY_DOO_LUNS_PER_RAID_GROUP,
                                       SCOOBY_DOO_CHUNKS_PER_LUN);
        }
        else {

           /* Scooby doo will use hot-spares */
           scooby_doo_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL, scooby_doo_run_tests,
                                          SCOOBY_DOO_LUNS_PER_RAID_GROUP,
                                          SCOOBY_DOO_CHUNKS_PER_LUN,
                                          FBE_FALSE);
        }

    } /* for all raid group types */

    return;
}
/******************************************
 * end scooby_doo_test()
 ******************************************/

/*!**************************************************************
 * scooby_doo_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void scooby_doo_setup(void)
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
            array_p = &scooby_doo_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &scooby_doo_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            fbe_test_rg_populate_extra_drives(&array_p[rg_index][0], SCOOBY_DOO_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);
        }

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
        sep_config_load_sep_and_neit();
    }
    
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end scooby_doo_setup()
 **************************************/

/*!**************************************************************
 * scooby_doo_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the scooby_doo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from scooby_doo_test.c
 *
 ****************************************************************/

void scooby_doo_cleanup(void)
{
    //fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

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
 * end scooby_doo_cleanup()
 ******************************************/

/*!**************************************************************
 * scooby_doo_dualsp_test()
 ****************************************************************
 * @brief
 *  Run test on both SPs.
 *
 * @param  None             
 *
 * @return None.   
 *
 *
 ****************************************************************/

void scooby_doo_dualsp_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        raid_group_types = SCOOBY_DOO_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        raid_group_types = SCOOBY_DOO_QUAL_TEST_CONFIGURATION_TYPES;
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
            rg_config_p = &scooby_doo_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            rg_config_p = &scooby_doo_raid_groups_qual[raid_group_type_index][0];
        }

        /* Scooby doo will use hot-spares */
        if (raid_group_type_index + 1 >= raid_group_types) {
           scooby_doo_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, scooby_doo_run_tests,
                                          SCOOBY_DOO_LUNS_PER_RAID_GROUP,
                                          SCOOBY_DOO_CHUNKS_PER_LUN);
        }
        else {
           scooby_doo_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL, scooby_doo_run_tests,
                                          SCOOBY_DOO_LUNS_PER_RAID_GROUP,
                                          SCOOBY_DOO_CHUNKS_PER_LUN,
                                          FBE_FALSE);
        }

    } /* for all raid group types */

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end scooby_doo_dualsp_test()
 ******************************************/

/*!**************************************************************
 * scooby_doo_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test.
 *
 * @param None.               
 * 
 * @return None.   
 * 
 * @note    Must run in dual-sp mode
 * 
 ****************************************************************/
void scooby_doo_dualsp_setup(void)
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
            array_p = &scooby_doo_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &scooby_doo_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            fbe_test_rg_populate_extra_drives(&array_p[rg_index][0], SCOOBY_DOO_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* Determine and load the physical config and populate all the 
         * raid groups.
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
 * end scooby_doo_dualsp_setup()
 **************************************/

/*!**************************************************************
 * scooby_doo_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup scooby_doo dual sp test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void scooby_doo_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

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
 * end scooby_doo_dualsp_cleanup()
 ******************************************/
