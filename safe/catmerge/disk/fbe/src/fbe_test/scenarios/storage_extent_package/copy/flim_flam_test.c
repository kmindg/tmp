/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    flim_flam_test.c
 *****************************************************************************
 *
 * @brief   This file contains tests that execute a copy operation to a raid
 *          group that contains (1) or more system drive(s).
 *
 * @author
 *  04/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "sep_tests.h"
#include "sep_test_background_ops.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_private_space_layout.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * flim_flam_short_desc = "This scenario test Copy Operations to a raid group that contains one or more system disks.";
char * flim_flam_long_desc = 
    "\n"
"This test will initiate a copy operation to a raid group that contains one or more system disks.\n"
"-raid_test_level 0 and 1\n\
     We remove and re-insert drives during copy at -rtl 1.\n\
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
Description last updated: 04/05/2012.\n";

/*!*******************************************************************
 * @def FLIM_FLAM_EXTRA_CAPACITY_FOR_METADATA
 *********************************************************************
 * @brief This is the number of user raid groups for each raid type.
 *
 *********************************************************************/
#define FLIM_FLAM_EXTRA_CAPACITY_FOR_METADATA       0x100000 /*Extra blocks for objects metadata */

/*!*******************************************************************
 * @def     FLIM_FLAM_INVALID_DISK_POSITION
 *********************************************************************
 * @brief   Invalid disk position.  Used when searching for a disk 
 *          position and no disk is found that meets the criteria.
 *
 *********************************************************************/
#define FLIM_FLAM_INVALID_DISK_POSITION             ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def FLIM_FLAM_DRIVE_WAIT_MSEC
 *********************************************************************
 * @brief Delay to run I/O before starting next phase.
 *
 *********************************************************************/
#define FLIM_FLAM_DRIVE_WAIT_MSEC          60000

/*!*******************************************************************
 * @def     FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY
 *********************************************************************
 * @brief This is the number of `extra' drives to reserve for each
 *        raid group for the copy operation.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY    1

/*!*******************************************************************
 * @def     FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_REBUILD
 *********************************************************************
 * @brief This is the number of `extra' drives to reserve for each
 *        raid group for the rebuild operation.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_REBUILD 1

/*!*******************************************************************
 * @def     FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE
 *********************************************************************
 * @brief This is the number of `extra' drives to reserve for each
 *        raid group.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE              (FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY + FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_REBUILD)

/*!*********************************************************************
 * @def     FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_SYSTEM_DRIVE_OVERLAP
 ***********************************************************************
 * @brief This is the number of `extra' drives to reserve for each
 *        raid group due to the fact that we don't include the system
 *        drives when calculating how many drives are required.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_SYSTEM_DRIVE_OVERLAP 4

/*!*******************************************************************
 * @def FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE
 *********************************************************************
 * @brief This is the number of user raid groups for each raid type.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE     1

/*!*******************************************************************
 * @def FLIM_FLAM_USER_RAID_GROUP_INDEX
 *********************************************************************
 * @brief This is the index of the user raid group.
 *
 *********************************************************************/
#define FLIM_FLAM_USER_RAID_GROUP_INDEX             0

/*!*******************************************************************
 * @def FLIM_FLAM_NUM_SYSTEM_RAID_GROUPS_PER_TYPE
 *********************************************************************
 * @brief This is the number of system raid groups for each raid type.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_SYSTEM_RAID_GROUPS_PER_TYPE   1

/*!*******************************************************************
 * @def     FLIM_FLAM_SYSTEM_RAID_GROUP_INDEX
 *********************************************************************
 * @brief   This is the index of the system raid group.
 *
 *********************************************************************/
#define FLIM_FLAM_SYSTEM_RAID_GROUP_INDEX           (FLIM_FLAM_USER_RAID_GROUP_INDEX + FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE)

/*!*******************************************************************
 * @def     FLIM_FLAM_INDEX_RESERVED_FOR_COPY
 *********************************************************************
 * @brief   This is the raid group index that is reserved for the copy
 *          operation.
 *
 *********************************************************************/
#define FLIM_FLAM_INDEX_RESERVED_FOR_COPY              1

/*!*******************************************************************
 * @def     FLIM_FLAM_NUM_RAID_GROUPS_TO_TEST
 *********************************************************************
 * @brief This is the number of raid groups to tests for each raid
 *        group type.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_RAID_GROUPS_TO_TEST           (FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE + FLIM_FLAM_NUM_SYSTEM_RAID_GROUPS_PER_TYPE)

/*!*******************************************************************
 * @def FLIM_FLAM_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FLIM_FLAM_LUNS_PER_RAID_GROUP               3

/*!*******************************************************************
 * @def FLIM_FLAM_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define FLIM_FLAM_TEST_MAX_WIDTH                    16

/*!*******************************************************************
 * @def FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS
 *********************************************************************
 * @brief Number of test contexts to create and test against.
 *
 *********************************************************************/
#define FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS             (FLIM_FLAM_LUNS_PER_RAID_GROUP * FLIM_FLAM_NUM_RAID_GROUPS_TO_TEST)

/*!*******************************************************************
 * @var flim_flam_io_msec_short
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `short' period of time.
 *
 *********************************************************************/
#define FLIM_FLAM_SHORT_IO_TIME_SECS  5
static fbe_u32_t flim_flam_io_msec_short = (FLIM_FLAM_SHORT_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var flim_flam_io_msec_long
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `long' period of time.
 *
 *********************************************************************/
#define FLIM_FLAM_LONG_IO_TIME_SECS  30
static fbe_u32_t flim_flam_io_msec_long = (FLIM_FLAM_LONG_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var flim_flam_threads
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t flim_flam_threads = 5;

/*!*******************************************************************
 * @def FLIM_FLAM_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Number of blocks in small io
 *
 *********************************************************************/
#define FLIM_FLAM_SMALL_IO_SIZE_BLOCKS 1024  

/*!*******************************************************************
 * @def FLIM_FLAM_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define FLIM_FLAM_MAX_IO_SIZE_BLOCKS (FLIM_FLAM_TEST_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @var flim_flam_b_disable_shutdown
 *********************************************************************
 * @brief This variable controls of shutdown tests are enabled for 
 *        certain or all raid groups.
 *
 *********************************************************************/
static fbe_bool_t flim_flam_b_disable_shutdown = FBE_TRUE;

/*!*******************************************************************
 * @var flim_flam_b_debug_enable
 *********************************************************************
 * @brief Determines if debug should be enabled or not
 *
 *********************************************************************/
fbe_bool_t flim_flam_b_debug_enable             = FBE_FALSE;

/*!*******************************************************************
 * @var flim_flam_library_debug_flags
 *********************************************************************
 * @brief Default value of the raid library debug flags to set
 *
 *********************************************************************/
fbe_u32_t flim_flam_library_debug_flags = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                           FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                           FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING  |
                                           FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );

/*!*******************************************************************
 * @var flim_flam_object_debug_flags
 *********************************************************************
 * @brief Default value of the raid group object debug flags to set
 *
 *********************************************************************/
fbe_u32_t flim_flam_object_debug_flags = (FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING);

/*!*******************************************************************
 * @var flim_flam_virtual_drive_debug_flags
 *********************************************************************
 * @brief Default value of the virtual drive debug flags to set
 *
 *********************************************************************/
fbe_u32_t flim_flam_virtual_drive_debug_flags   = (FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING   |
                                                   FBE_VIRTUAL_DRIVE_DEBUG_FLAG_COPY_TRACING      |
                                                   FBE_VIRTUAL_DRIVE_DEBUG_FLAG_CONFIG_TRACING    |
                                                   FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING      |
                                                   FBE_VIRTUAL_DRIVE_DEBUG_FLAG_QUIESCE_TRACING   |
                                                   FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING          );

/*!*******************************************************************
 * @var flim_flam_enable_state_trace
 *********************************************************************
 * @brief Default value of enabling state trace
 *
 *********************************************************************/
fbe_bool_t flim_flam_enable_state_trace         = FBE_TRUE;

/*!*******************************************************************
 * @def     FLIM_FLAM_FIXED_IO_SIZE
 *********************************************************************
 * @brief   The I/O size in blocks for fixed I/O.
 *
 *********************************************************************/
#define FLIM_FLAM_FIXED_IO_SIZE                 128

/*!*******************************************************************
 * @def     FLIM_FLAM_FIXED_PATTERN
 *********************************************************************
 * @brief   rdgen fixed pattern to use
 *
 * @todo    Currently the only fixed pattern that rdgen supports is
 *          zeros.
 *
 *********************************************************************/
#define FLIM_FLAM_FIXED_PATTERN     FBE_RDGEN_PATTERN_ZEROS

/*!*******************************************************************
 * @def FLIM_FLAM_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 * @note  This needs to be fairly large so that we test I/O with copy.
 *********************************************************************/
#define FLIM_FLAM_CHUNKS_PER_LUN            5

/*!*******************************************************************
 * @def FLIM_FLAM_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define FLIM_FLAM_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def FLIM_FLAM_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define FLIM_FLAM_TEST_BLOCK_OP_MAX_BLOCK   4096


/*!*******************************************************************
 * @def FLIM_FLAM_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define FLIM_FLAM_LARGE_IO_SIZE_MAX_BLOCKS FLIM_FLAM_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def FLIM_FLAM_EXTENDED_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types.
 *
 *********************************************************************/
#define FLIM_FLAM_EXTENDED_TEST_CONFIGURATION_TYPES 5

/*!*******************************************************************
 * @def FLIM_FLAM_QUAL_TEST_CONFIGURATION_TYPES
 *********************************************************************
 * @brief this is the number of test configuration types
 *
 *********************************************************************/
#define FLIM_FLAM_QUAL_TEST_CONFIGURATION_TYPES     2

/*!***************************************************************************
 * @var     flim_flam_b_is_abort_testing_disabled_due_to_DE543
 *****************************************************************************
 * @brief   Determines if abort testing is disabled due to DE543 (insufficient
 *          payload to allocate stripe lock for restart I/O).
 *
 * @todo    Fix this issue so that abort testing can be performed.
 *
 *****************************************************************************/
static fbe_bool_t flim_flam_b_is_abort_testing_disabled_due_to_DE543 = FBE_TRUE; /*! Currently abort testing is disabled */

/*!*******************************************************************
 * @var     flim_flam_test_contexts
 *********************************************************************
 * @brief   This contains our context objects for running rdgen I/O.
 *
 *
 * @note    We will test (3) system LUNs on a second system raid group.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t flim_flam_test_contexts[FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS];

/*!*******************************************************************
 * @var     flim_flam_drive_to_remove
 *********************************************************************
 * @brief   This is the system drive that we will remove.
 *
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t flim_flam_drive_to_remove = {0, 0, 0};

/*!*******************************************************************
 * @var     flim_flam_raid_groups_extended
 *********************************************************************
 * @brief   Test config for raid test level 1 and greater.
 *
 * @note    Currently only (1) raid group for each type is allowed.
 *          this is due to the fact that we will select particular drives.
 *          
 * @note    Note that the specific drives to use are defined.
 *
 * @note    We always use position 0 for the copy operation, therefore
 *          {0, 0, 0} should be included in all raid groups under test.
 *
 * @note    The second raid group in each set is simply a `dummy'
 *          raid group that will be populated with the system raid
 *          group information.
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t flim_flam_raid_groups_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t flim_flam_raid_groups_extended[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* RAID-0 - Full overlap with system drives.
     */
    {
       /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth. */
        {9,       0x20000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,               1,
         {{0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 0, 3}, {0, 1, 4}, {0, 1, 5}, {0, 1, 6}, {0, 1, 7}, {0, 1, 8}}},
        /* (1) Dummy raid group for system raid group. */
        {9,       0x20000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,               1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,},
    },
    /* RAID-1 - (1) drive overlap with system drives.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, raid group id,  bandwidth */
        {3,         0x20000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        0,                  1,
         {{0, 0, 0}, {0, 1, 1}, {0, 1, 2}}},
        /* (1) Dummy raid group for system raid group. */
        {3,         0x20000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        1,                  1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* RAID-3 - (1) drive overlap with system drives.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,        0,                  1,
         {{0, 0, 0}, {0, 1, 1}, {0, 1, 2}, {0, 1, 3}, {0, 1, 4}}},
        /* (1) Dummy raid group for system raid group. */
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,        1,                  1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* RAID-5 - (2) drive overlap with system drives.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,        0,                  0,
         {{0, 0, 0}, {0, 0, 3}, {0, 1, 2}, {0, 1, 3}, {0, 1, 4}}},
        /* (1) Dummy raid group for system raid group. */
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,        1,                  0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* RAID-6 - (2) drive overlap with system drives.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {6,         0x20000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,        0,                  0,
         {{0, 0, 0}, {0, 0, 3}, {0, 1, 2}, {0, 1, 3}, {0, 1, 4}, {0, 1, 5}}},
        /* (1) Dummy raid group for system raid group. */
        {6,         0x20000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,        1,                  0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var     flim_flam_raid_groups_qual
 *********************************************************************
 * @brief   Test config for raid test level 0 (default test level).
 *
 * @note    Currently all the raid groups for each type must be the
 *          same width and type.
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t flim_flam_raid_groups_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t flim_flam_raid_groups_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* RAID-1 - Full overlap with system drives.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, raid group id,  bandwidth */
        {2,         0x20000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        0,                  1,
         {{0, 0, 0}, {0, 0, 3}}},
        /* (1) Dummy raid group for system raid group. */
        {2,         0x20000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,        1,                  1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* RAID-5 - (2) drive overlap with system drives.
     */
    {
        /* width,   capacity    raid type,                  class,                  block size, RAID-id.    bandwidth. */
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,        0,              0,
         {{0, 0, 0}, {0, 0, 3}, {0, 1, 2}, {0, 1, 3}, {0, 1, 4}}},
        /* (1) Dummy raid group for system raid group. */
        {5,         0x20000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,        1,              0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * flim_flam_get_user_raid_group_count()
 ****************************************************************
 * @brief
 *  Get the number of raid_groups in the array.
 *  Simply returns the number of user raid groups.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
static fbe_u32_t flim_flam_get_user_raid_group_count(fbe_test_rg_configuration_t *rg_config_p)
{
    return FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE;
}
/******************************************
 * end flim_flam_get_user_raid_group_count()
 ******************************************/

/*!*******************************************************************
 * flim_flam_rg_config_array_with_extra_drives_load_physical_config()
 *********************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *  
 *  Note that this also will fill out the disk set for each of the
 *  raid groups passed in.
 *
 * @param array_p - The array of raid group configs we will create.
 *
 * @notes It is assumed that the raid group configs are not created
 *        all at the same time, but each array index is created and
 *        then destroyed before creating the next array index.
 *        Thus, the disk sets between different array indexes can overlap.
 *
 * @return None.
 *
 * @author
 *  04/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static void flim_flam_rg_config_array_with_extra_drives_load_physical_config(fbe_test_rg_configuration_array_t *array_p)
{
    fbe_u32_t                   raid_type_index;
    fbe_u32_t                   CSX_MAYBE_UNUSED extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;
    fbe_u32_t current_num_520_drives = 0;
    fbe_u32_t current_num_4160_drives = 0;
    fbe_u32_t current_520_extra_drives = 0;
    fbe_u32_t current_4160_extra_drives = 0;
    fbe_u32_t raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    /*! @note We MUST use the system drive capacity since we are binding with at least (1) system drive !!*/
    fbe_block_count_t drive_capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;

    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = flim_flam_get_user_raid_group_count(rg_config_p);

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
    }

    /* Add the raid and extra maximum drives
     */
    num_520_drives += num_520_extra_drives;
    num_4160_drives += num_4160_extra_drives;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 512 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives );

    /* Fill in our raid group disk sets with this physical config.
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = flim_flam_get_user_raid_group_count(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }

        /*! @note We use the disk set specified.
         */
        //fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, num_520_drives, num_4160_drives);
    }

    /*caculate the drive capacity now
     */
    for (raid_type_index = 0; raid_type_index < raid_type_count; raid_type_index++)
    {
        fbe_block_count_t current_drive_capacity;
        rg_config_p = &array_p[raid_type_index][0];
        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "raid type %s (%d)not enabled\n", 
                       raid_type_string_p, rg_config_p->raid_type);
            continue;
        }
        raid_group_count = flim_flam_get_user_raid_group_count(rg_config_p);

        if (raid_group_count == 0)
        {
            continue;
        }
        current_drive_capacity = fbe_test_sep_util_get_maximum_drive_capacity(rg_config_p, 
                                                                              raid_group_count);
        if(current_drive_capacity > drive_capacity)
        {
            drive_capacity = current_drive_capacity;
        }
    }

    /*! @note We MUST use the system drive capacity since we are binding with at
     *        least (1) system drive !!
     */
    if (drive_capacity < TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY)
    {
        drive_capacity = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY;
    }

    /* need to add some more blocks for metadata of the objects which are not calculated as
     * we only considered the user capacity during calculations
     */
    drive_capacity += FLIM_FLAM_EXTRA_CAPACITY_FOR_METADATA;

    mut_printf(MUT_LOG_TEST_STATUS, "Drive capacity 0x%llx ", 
                          (unsigned long long)drive_capacity);

    /*! @note Although we don't need all the drives created, this is the
     *        simplest thing to do.
     */
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            drive_capacity);
    return;
}
/********************************************************************************
 * end flim_flam_rg_config_array_with_extra_drives_load_physical_config()
 ********************************************************************************/

/*!**************************************************************
 * flim_flam_set_io_seconds()
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
void flim_flam_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();

    if (long_io_time_seconds >= FLIM_FLAM_LONG_IO_TIME_SECS)
    {
        flim_flam_io_msec_long = (long_io_time_seconds * 1000);
    }
    else
    {
        flim_flam_io_msec_long  = ((fbe_random() % FLIM_FLAM_LONG_IO_TIME_SECS) + 1) * 1000;
    }
    flim_flam_io_msec_short = ((fbe_random() % FLIM_FLAM_SHORT_IO_TIME_SECS) + 1) * 1000;
    return;
}
/******************************************
 * end flim_flam_set_io_seconds()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_get_max_lba_to_test()
 *****************************************************************************
 *
 * @brief   Due to the fact that we are testing system luns, we do not want to
 *          test the entire capacity of all the luns.  Instead we use the
 *          capacity of the user raid group luns.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 *
 * @return  lba - capacity of user luns
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_lba_t flim_flam_get_max_lba_to_test(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_lba_t   max_lba = 0;

    max_lba = rg_config_p->logical_unit_configuration_list[0].capacity;

    return max_lba;
}
/******************************************
 * end flim_flam_get_max_lba_to_test()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_setup_standard_rdgen_test_context()
 *****************************************************************************
 *
 * @brief   Setup the standard rdgen context for all luns under test.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group 
 * @param   rdgen_operation 
 * @param   pattern 
 * @param   max_passes
 * @param   threads
 * @param   io_size_blocks
 * @param   b_inject_random_aborts
 * @param   b_dualsp_io
 *
 * @return  status
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t flim_flam_setup_standard_rdgen_test_context(fbe_test_rg_configuration_t *const rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t luns_per_rg,
                                                                fbe_bool_t b_sequential_fixed,
                                                                fbe_rdgen_operation_t rdgen_operation,
                                                                fbe_rdgen_pattern_t pattern,
                                                                fbe_u32_t max_passes,
                                                                fbe_u32_t threads,
                                                                fbe_u64_t io_size_blocks,
                                                                fbe_bool_t b_inject_random_aborts,
                                                                fbe_bool_t b_dualsp_io)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t                *context_p = &flim_flam_test_contexts[0];
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_test_logical_unit_configuration_t  *lu_config_p = NULL;
    fbe_object_id_t                         lu_object_id;
    fbe_u32_t                               rg_index;
    fbe_u32_t                               lu_index;
    fbe_u32_t                               io_seconds;
    fbe_time_t                              io_time;
    fbe_lba_t                               max_lba = flim_flam_get_max_lba_to_test(rg_config_p);

    /* First determine if a run time is enabled or not.  If it is, it overrides
     * the maximum number of passes.
     */
    io_seconds = fbe_test_sep_io_get_io_seconds();
    io_time = io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    if (io_seconds > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s using I/O seconds of %d", __FUNCTION__, io_seconds);
        max_passes = 0;
    }

    /*! Now configure the standard rdgen test context values
     *
     *  @todo Add abort and peer options to the context setup
     */
    MUT_ASSERT_TRUE((raid_group_count * luns_per_rg) <= FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* For each lun in the raid group.
         */
        for (lu_index = 0; lu_index < luns_per_rg; lu_index++)
        {
            /* Initialize the rdgen context for each lun in the raid group.
             */
            lu_config_p = &current_rg_config_p->logical_unit_configuration_list[lu_index];
            status = fbe_api_database_lookup_lun_by_number(lu_config_p->lun_number, &lu_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Determine if it is a sequential fixed pattern or not.
             */
            if (b_sequential_fixed)
            {
                status = fbe_api_rdgen_test_context_init(context_p,
                                                         lu_object_id,
                                                         FBE_CLASS_ID_INVALID,
                                                         FBE_PACKAGE_ID_SEP_0,
                                                         rdgen_operation,
                                                         pattern,
                                                         1,    /* We do one full sequential pass. */
                                                         0,    /* num ios not used */
                                                         0,    /* time not used*/
                                                         1,    /* threads */
                                                         FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                                         0,    /* Start lba*/
                                                         0,    /* Min lba */
                                                         max_lba,    /* max lba */
                                                         FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                         16,    /* Number of stripes to write. */
                                                         io_size_blocks    /* Max blocks */ );
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }
            else
            {   
                /* Else we run a radom pattern.
                 */
                status = fbe_api_rdgen_test_context_init(context_p, 
                                                         lu_object_id,
                                                         FBE_CLASS_ID_INVALID,
                                                         FBE_PACKAGE_ID_SEP_0,
                                                         rdgen_operation,
                                                         pattern,                       /* data pattern specified */
                                                         max_passes,
                                                         0,                             /* io count not used */
                                                         io_time,
                                                         threads,
                                                         FBE_RDGEN_LBA_SPEC_RANDOM,     /* Standard I/O mode is random */
                                                         0,                             /* Start lba*/
                                                         0,                             /* Min lba */
                                                         max_lba,                       /* max lba */
                                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,   /* Standard I/O transfer size is random */
                                                         1,                             /* Min blocks per request */
                                                         io_size_blocks                 /* Max blocks per request */ );
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            /* Goto the next rdgen context.
             */
            context_p++;

        } /* for all luns to test*/

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* for all raid groups*/

    /* If the injection of random aborts is enabled update the context.
     */
    if (b_inject_random_aborts == FBE_TRUE)
    {
        fbe_test_random_abort_msec_t    rdgen_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC;

        /*! @note Assumes that rdgen abort msecs has been configured
         */
        if (rdgen_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification,
                                                     rdgen_abort_msecs);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, 
                                                                FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            /* Else if random aborts are enabled they should have been configured
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Need to configure abort mode to enable aborts", __FUNCTION__);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    /* If peer I/O is enabled update the context
     */
    if (b_dualsp_io == FBE_TRUE)
    {
        fbe_api_rdgen_peer_options_t    rdgen_peer_options = FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE;

        /*! @note Assumes that rdgen peer options has been configured
         */
        if (rdgen_peer_options != FBE_API_RDGEN_PEER_OPTIONS_INVALID)
        {
            status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                     rdgen_peer_options);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            /* If peer I/O is enabled, it should have been previously configured
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Need to configure peer options to enable peer I/O", __FUNCTION__);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    return FBE_STATUS_OK;
}
/*********************************************************
 * end flim_flam_setup_standard_rdgen_test_context()
 *********************************************************/

/*!***************************************************************************
 *          flim_flam_write_background_pattern()
 *****************************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param   rg_config_p - Pointer to array of raid groups to test
 * @param   raid_group_count - Number or raid groups to test
 * @param   luns_per_rg - Number of luns per raid group      
 * @param   b_inject_random_aborts - FBE_TRUE - Periodically inject aborts 
 * @param   b_dualsp_enabled - FBE_TRUE dualsp mode testing is enabled   
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from flim_flam_write_background_pattern
 *
 *****************************************************************************/
static void flim_flam_write_background_pattern(fbe_test_rg_configuration_t *const rg_config_p, 
                                               fbe_u32_t raid_group_count,                     
                                               fbe_u32_t luns_per_rg,
                                               fbe_bool_t b_inject_random_aborts,
                                               fbe_bool_t b_dualsp_enabled)

{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
    
    /* We currently ignore abort and dualsp
     */
    FBE_UNREFERENCED_PARAMETER(b_inject_random_aborts);
    FBE_UNREFERENCED_PARAMETER(b_dualsp_enabled);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write and verify background pattern ==", __FUNCTION__);

    /* Write a background pattern to the LUNs.
     */
    status = flim_flam_setup_standard_rdgen_test_context(rg_config_p,
                                                         raid_group_count,
                                                         luns_per_rg,
                                                         FBE_TRUE, /* Sequential fixed */
                                                         FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                                         1,
                                                         1,
                                                         FLIM_FLAM_FIXED_IO_SIZE,
                                                         FBE_FALSE, /* Don't inject aborts*/
                                                         FBE_FALSE  /* Don't run on both sps */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = flim_flam_setup_standard_rdgen_test_context(rg_config_p,
                                                         raid_group_count,
                                                         luns_per_rg,
                                                         FBE_TRUE, /* Sequential fixed */
                                                         FBE_RDGEN_OPERATION_READ_CHECK,
                                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                                         1,
                                                         1,
                                                         FLIM_FLAM_FIXED_IO_SIZE,
                                                         FBE_FALSE, /* Don't inject aborts*/
                                                         FBE_FALSE  /* Don't run on both sps */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write and verify background pattern - Success ==", __FUNCTION__);
    return;
}
/******************************************
 * end flim_flam_write_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_write_fixed_pattern()
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
void flim_flam_write_fixed_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
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
                                             FLIM_FLAM_FIXED_PATTERN,
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
                                             FLIM_FLAM_FIXED_IO_SIZE    /* Max blocks */ );
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

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);
    
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
                                             FLIM_FLAM_FIXED_PATTERN,
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
                                             FLIM_FLAM_FIXED_IO_SIZE    /* Max blocks */ );
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
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end flim_flam_write_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_read_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Read all lUNs and validate fixed pattern
 *
 * @param   rg_config_p - Pointer to array of raid groups to test
 * @param   raid_group_count - Number or raid groups to test
 * @param   luns_per_rg - Number of luns per raid group      
 * @param   b_inject_random_aborts - FBE_TRUE - Periodically inject aborts 
 * @param   b_dualsp_enabled - FBE_TRUE dualsp mode testing is enabled   
 *
 * @return  None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void flim_flam_read_fixed_pattern(fbe_test_rg_configuration_t *const rg_config_p, 
                                         fbe_u32_t raid_group_count,                     
                                         fbe_u32_t luns_per_rg,
                                         fbe_bool_t b_inject_random_aborts,
                                         fbe_bool_t b_dualsp_enabled)
{
    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating fixed pattern ==", 
               __FUNCTION__);

    /* Read back the pattern and check it was written OK.
     */
    status = flim_flam_setup_standard_rdgen_test_context(rg_config_p,
                                                         raid_group_count,
                                                         luns_per_rg,
                                                         FBE_TRUE, /* Sequential fixed */
                                                         FBE_RDGEN_OPERATION_READ_CHECK,
                                                         FLIM_FLAM_FIXED_PATTERN,
                                                         1,
                                                         1,
                                                         FLIM_FLAM_FIXED_IO_SIZE,
                                                         b_inject_random_aborts,
                                                         b_dualsp_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;
}
/******************************************
 * end flim_flam_read_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_write_zero_background_pattern()
 *****************************************************************************
 *
 * @brief   Seed all the LUNs with a zero pattern.
 *
 * @param   rg_config_p - Pointer to array of raid groups to test
 * @param   raid_group_count - Number or raid groups to test
 * @param   luns_per_rg - Number of luns per raid group      
 * @param   b_inject_random_aborts - FBE_TRUE - Periodically inject aborts 
 * @param   b_dualsp_enabled - FBE_TRUE dualsp mode testing is enabled   
 *
 * @return  None.
 *
 * @author
 *  03/08/2011  Ruomu Gao  - Copied from big_bird_test.c
 *
 *****************************************************************************/
static void flim_flam_write_zero_background_pattern(fbe_test_rg_configuration_t *const rg_config_p, 
                                                    fbe_u32_t raid_group_count,                     
                                                    fbe_u32_t luns_per_rg,
                                                    fbe_bool_t b_inject_random_aborts,
                                                    fbe_bool_t b_dualsp_enabled)

{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
    
    /* We currently ignore abort and dualsp
     */
    FBE_UNREFERENCED_PARAMETER(b_inject_random_aborts);
    FBE_UNREFERENCED_PARAMETER(b_dualsp_enabled);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Writing zeros pattern ==", 
               __FUNCTION__);

    /* Write a zeros pattern to the LUNs.
     */
    status = flim_flam_setup_standard_rdgen_test_context(rg_config_p,
                                                         raid_group_count,
                                                         luns_per_rg,
                                                         FBE_TRUE, /* Sequential fixed */
                                                         FBE_RDGEN_OPERATION_ZERO_ONLY,
                                                         FBE_RDGEN_PATTERN_ZEROS,
                                                         1,
                                                         1,
                                                         FLIM_FLAM_FIXED_IO_SIZE,
                                                         FBE_FALSE, /* Don't inject aborts*/
                                                         FBE_FALSE  /* Don't run on both sps */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = flim_flam_setup_standard_rdgen_test_context(rg_config_p,
                                                         raid_group_count,
                                                         luns_per_rg,
                                                         FBE_TRUE, /* Sequential fixed */
                                                         FBE_RDGEN_OPERATION_READ_CHECK,
                                                         FBE_RDGEN_PATTERN_ZEROS,
                                                         1,
                                                         1,
                                                         FLIM_FLAM_FIXED_IO_SIZE,
                                                         FBE_FALSE, /* Don't inject aborts*/
                                                         FBE_FALSE  /* Don't run on both sps */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;
}
/***********************************************
 * end flim_flam_write_zero_background_pattern()
 ***********************************************/

/*!***************************************************************************
 *          flim_flam_read_background_pattern()
 *****************************************************************************
 *
 * @brief   Read and validate the pattern.
 *
 * @param   rg_config_p - Pointer to array of raid groups to test
 * @param   raid_group_count - Number or raid groups to test
 * @param   luns_per_rg - Number of luns per raid group      
 * @param   b_inject_random_aborts - FBE_TRUE - Periodically inject aborts 
 * @param   b_dualsp_enabled - FBE_TRUE dualsp mode testing is enabled   
 *
 * @return  None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from flim_flam_read_background_pattern
 *
 *****************************************************************************/
static void flim_flam_read_background_pattern(fbe_test_rg_configuration_t *const rg_config_p, 
                                              fbe_u32_t raid_group_count,                     
                                              fbe_u32_t luns_per_rg,
                                              fbe_bool_t b_inject_random_aborts,
                                              fbe_bool_t b_dualsp_enabled)
{
    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
    fbe_status_t status;
    
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern ==", 
               __FUNCTION__);

    /* Read back the pattern and check it was written OK.
     */
    status = flim_flam_setup_standard_rdgen_test_context(rg_config_p,
                                                         raid_group_count,
                                                         luns_per_rg,
                                                         FBE_TRUE, /* Sequential fixed */
                                                         FBE_RDGEN_OPERATION_READ_CHECK,
                                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                                         1,
                                                         1,
                                                         FLIM_FLAM_FIXED_IO_SIZE,
                                                         b_inject_random_aborts,
                                                         b_dualsp_enabled);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Execute the I/O
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, FLIM_FLAM_NUM_OF_RDGEN_CONTEXTS); 

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end flim_flam_read_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_is_shutdown_allowed_for_raid_group()
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
static fbe_bool_t flim_flam_is_shutdown_allowed_for_raid_group(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_bool_t  b_allow_shutdown = FBE_TRUE;

    /* Currently al
     */
    if (flim_flam_b_disable_shutdown == FBE_TRUE)
    {
        b_allow_shutdown = FBE_FALSE;
    }

    return(b_allow_shutdown);
}
/*****************************************************
 * end flim_flam_is_shutdown_allowed_for_raid_group()
 *****************************************************/

/*!**************************************************************
 * flim_flam_populate_rg_num_extra_drives()
 ****************************************************************
 * @brief
 *  Initialize the rg config with number of extra drive required for each rg. This 
 *  information will use by a test while removing a drive and insert a new drive.
 *
 * @note   
 *  this function only needs to call from a fbe_test which needs to support
 *  use case of remove a drive and insert a new drive in simulation or on hardware. 
 *  By default extra drive count remains set to zero for a test which does not need 
 *  functionality to insert a new drive.
 *
 * @param rg_config_p - Array of raidgroup to determine how many extra drive needed 
 *                                for each rg and to populate extra drive count.
 *
 * 
 *
 * @return None.
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created
 *
 ****************************************************************/
static void flim_flam_populate_rg_num_extra_drives(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t   rg_index = 0;
    fbe_u32_t   num_req_extra_drives;
    fbe_u32_t   num_drives_for_copy_operation = FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY;

    /* Loop until we find the table number or we hit the terminator.
     */
    while (rg_config_p->width != FBE_U32_MAX)
    {
        if (rg_index < FLIM_FLAM_SYSTEM_RAID_GROUP_INDEX)
        {
            if (fbe_test_rg_config_is_enabled(rg_config_p))
            {
                fbe_test_sep_util_get_rg_type_num_extra_drives(rg_config_p->raid_type,
                                                               rg_config_p->width,
                                                               &num_req_extra_drives);

                /* set the required number of extra drives for each rg in rg config 
                 */      
                num_req_extra_drives = num_req_extra_drives + num_drives_for_copy_operation;
                rg_config_p->num_of_extra_drives = num_req_extra_drives;
            }
        }

        /* Goto next raid group
         */
        rg_config_p++;
        rg_index++;
    }

    return;
}
/***********************************************
 * end flim_flam_populate_rg_num_extra_drives()
 ***********************************************/

/*!**************************************************************
 * flim_flam_discover_raid_group_disk_set()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
 *  This allows us to use a physical configuration, which is not
 *  configurable.
 *
 *  Note that if the number of disks is not sufficient for the 
 *  raid group config, then we simply enable the configs that we can.
 * 
 *  A later call to this function will enable the remaining configs
 *  to be tested.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param drive_locations_p - The set of drives we discovered.
 *
 * @return -   
 *
 * @author
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t flim_flam_discover_raid_group_disk_set(fbe_test_rg_configuration_t * rg_config_p,
                                                           fbe_u32_t raid_group_count,
                                                           fbe_test_discovered_drive_locations_t *drive_locations_p)
{
   /* For a given configuration fill in the actual drives we will
    * be using.  This function makes it easy to create flexible
    * configurations without having to know exactly which disk goes
    * in which raid group.  We first create a set of disk sets for
    * the 512 and 520 disks.  Then we call this function and
    * pick drives from those disk sets to use in our raid groups.
     */
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t rg_disk_index;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_drive_type_t        drive_type;
    fbe_test_block_size_t   block_size;
    fbe_bool_t              b_disks_found;
    fbe_test_raid_group_disk_set_t * disk_set_p = NULL;

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));


    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 512 and 520. 
     */
    for(rg_index = 0; rg_index < FLIM_FLAM_SYSTEM_RAID_GROUP_INDEX; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(rg_config_p[rg_index].block_size, &block_size);
        if (rg_config_p[rg_index].magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(&rg_config_p[rg_index]);
        }
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        /* Discover the Drive type we will use for this RG
         */
        b_disks_found = false;
        for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
        {
            /* We have already selected the drives for the raid groups.
             * Now locate the `extra' drives.
             */
            if ((rg_config_p[rg_index].width + rg_config_p[rg_index].num_of_extra_drives) 
                                                                    <= local_drive_counts[block_size][drive_type])
            {
                /* Set the drive type
                 */
                rg_config_p[rg_index].drive_type = drive_type;

                /* populate required extra disk set if extra drive count is set by consumer test
                 */
                for (rg_disk_index = 0; rg_disk_index < rg_config_p[rg_index].num_of_extra_drives; rg_disk_index++)
                {
                    disk_set_p = NULL;
                    local_drive_counts[block_size][drive_type]--;

                    /* Fill in the next drive.
                     */
                    status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                                    drive_locations_p->disk_set[block_size][drive_type],
                                                                    &disk_set_p);
                    if (status != FBE_STATUS_OK)
                    {
                        return status;
                    }

                    /*! @note We due populate the extra disk set.
                     */ 
                    rg_config_p[rg_index].extra_disk_set[rg_disk_index] = *disk_set_p;
                }
                
                b_disks_found = true;
                break;
            }
        }
        if(b_disks_found == false)
        {
            /* We did make sure we had enough drives before entering this function
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)
            return status;
        }
    }
    return FBE_STATUS_OK;
}
/***********************************************
 * end flim_flam_discover_raid_group_disk_set()
 ***********************************************/

/*!**************************************************************
 * flim_flam_setup_rg_config()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
 *  This allows us to use a physical configuration, which is not
 *  configurable.
 *
 *  Note that if the number of disks is not sufficient for the 
 *  raid group config, then we simply enable the configs that we can.
 * 
 *  A later call to this function will enable the remaining configs
 *  to be tested.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param drive_locations_p - The set of drives we discovered.
 *
 * @return -   
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t flim_flam_setup_rg_config(fbe_test_rg_configuration_t * rg_config_p,
                                              fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   rg_index = 0;
    fbe_test_rg_configuration_t * const         orig_rg_config_p = rg_config_p;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_u32_t raid_group_count = 0;
    fbe_test_block_size_t   block_size;
    fbe_drive_type_t        drive_type;

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));
    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* For the give drive counts, disable any configs not valid for this 
     * physical config. 
     * In other words if we do not have enough drives total to create the 
     * given raid group, then disable it. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        if (!rg_config_p->b_already_tested)
        {
            /* Check if we can find enough drives of same drive type
            */
            rg_config_p->b_create_this_pass = FBE_FALSE;
            rg_config_p->b_valid_for_physical_config = FBE_FALSE;

            if (rg_index < FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE)
            {
                for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
                {
                    /* We need to add (4) drives to the total drives.
                     */
                    if (local_drive_counts[block_size][drive_type] > 0)
                    {
                        local_drive_counts[block_size][drive_type] += FLIM_FLAM_NUM_OF_DRIVES_TO_RESERVE_FOR_SYSTEM_DRIVE_OVERLAP;
                    }
                    if ((rg_config_p->width + rg_config_p->num_of_extra_drives) <= local_drive_counts[block_size][drive_type])
                    {
                        /* We will use this in the current pass. 
                         * Mark it as used so that the next time through we will not create it. 
                         */
                        rg_config_p->b_create_this_pass = FBE_TRUE;
                        rg_config_p->b_already_tested = FBE_TRUE;
                        rg_config_p->b_valid_for_physical_config = FBE_TRUE;

                        local_drive_counts[block_size][drive_type] -= rg_config_p->width;
                        local_drive_counts[block_size][drive_type] -= rg_config_p->num_of_extra_drives;
                        break;
                    }
                }
            }
            else
            {
                /* Else this is a system raid group and we have the drives.
                 */
                rg_config_p->b_create_this_pass = FBE_TRUE;
                rg_config_p->b_already_tested = FBE_TRUE;
                rg_config_p->b_valid_for_physical_config = FBE_TRUE;
            }
        }
        else
        {
            rg_config_p->b_create_this_pass = FBE_FALSE;
        }

        rg_config_p++;
    }

    /* For both simulation and hardware use the drives already configured.
     */
    raid_group_count = flim_flam_get_user_raid_group_count(rg_config_p);
    if(fbe_test_util_is_simulation())
    {
        status = flim_flam_discover_raid_group_disk_set(orig_rg_config_p,
                                                        raid_group_count,
                                                        drive_locations_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* Consider drive capacity into account while running the test on hardware.
         */

        /*! @todo When we will change the spare selection algorithm to select spare drive based
         *   on minimum raid group drive capacity then we do not need this special handling.
         */
        status = fbe_test_sep_util_discover_raid_group_disk_set_with_capacity(orig_rg_config_p,
                                                    raid_group_count,
                                                    drive_locations_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /*! @todo really we should change the below function to take a fbe_test_drive_locations_t structure.
     */

    /* This function will update unused drive info for those raid groups
       which are going to be used in this pass
    */
    fbe_test_sep_util_set_unused_drive_info(orig_rg_config_p,
                                            raid_group_count,
                                            local_drive_counts,
                                            drive_locations_p);

    return status;
}
/******************************************
 * end flim_flam_setup_rg_config()
 ******************************************/

/*!**************************************************************
 *          flim_flam_fill_system_raid_group_lun_config()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
 *  This allows us to use a physical configuration, which is not
 *  configurable.
 *
 *  Note that if the number of disks is not sufficient for the 
 *  raid group config, then we simply enable the configs that we can.
 * 
 *  A later call to this function will enable the remaining configs
 *  to be tested.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param drive_locations_p - The set of drives we discovered.
 *
 * @return -   
 *
 * @author
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t flim_flam_fill_system_raid_group_lun_config(fbe_test_rg_configuration_t * raid_group_configuration_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_lun_number_t lun_number,
                                                                fbe_u32_t logical_unit_count)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               rg_index = 0;
    fbe_u32_t                               lu_index = 0;
    fbe_raid_group_number_t                 raid_group_id = FBE_RAID_ID_INVALID;
    fbe_object_id_t                         lun_object_id;
    fbe_api_lun_get_lun_info_t              lun_info;
    fbe_test_logical_unit_configuration_t  *lun_config_p = NULL;

    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 512 and 520. 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if(raid_group_configuration_p == NULL) {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User has given invalid RAID group configuration.", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }                    

        /* Get the RAID group id. */
        raid_group_id = raid_group_configuration_p->raid_group_id;
        if(raid_group_id == FBE_RAID_ID_INVALID)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s User(* thas given invalid RAID group id.", __FUNCTION__);
            continue;
        }

        /* Fill out the lun information for this RAID group. 
         */
        for (lu_index = 0; lu_index < logical_unit_count; lu_index++)
        {
            /* Based on index get the lun object id.
             */
            switch(lu_index)
            {
                case 0:
                    lun_number = FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_0;
                    lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0;
                    break;
                case 1:
                    lun_number = FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_1;
                    lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_1;
                    break;
                case 2:
                    lun_number = FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_2;
                    lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_2;
                    break;
                default:
                    MUT_ASSERT_TRUE_MSG((logical_unit_count <= 3), "More than (3) private luns requested");
                    break;
            }
            status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            lun_config_p = &raid_group_configuration_p->logical_unit_configuration_list[lu_index];
            lun_config_p->imported_capacity = lun_info.capacity; /*! @note We won't modify lun metadata */
            lun_config_p->capacity = lun_info.capacity;
            lun_config_p->lun_number = lun_number;
            lun_config_p->raid_group_id = raid_group_id;
            lun_config_p->raid_type = raid_group_configuration_p->raid_type;
        }

        /* Update the number of luns per RAID group details. */
        raid_group_configuration_p->number_of_luns_per_rg = logical_unit_count;
        raid_group_configuration_p++;
    }
    return FBE_STATUS_OK;
}
/***************************************************
 * end flim_flam_fill_system_raid_group_lun_config()
 ***************************************************/

/*!**************************************************************
 * flim_flam_fill_lun_configurations_rounded()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
 *  This allows us to use a physical configuration, which is not
 *  configurable.
 *
 *  Note that if the number of disks is not sufficient for the 
 *  raid group config, then we simply enable the configs that we can.
 * 
 *  A later call to this function will enable the remaining configs
 *  to be tested.
 *
 * @param   rg_config_p - Array of raid groups configs for (1) type
 * @param   raid_group_count - Number of raid groups to test
 * @param   chunks_per_lun - Chunks per lun
 * @param   luns_per_rg - Luns per raid group
 *
 * @return  None.
 *
 * @author
 *  04/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static void flim_flam_fill_lun_configurations_rounded(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t raid_group_count, 
                                                      fbe_u32_t chunks_per_lun,
                                                      fbe_u32_t luns_per_rg)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           rg_index;
    fbe_lun_number_t                    lun_number = 0;
    fbe_u32_t                           current_chunks_per_lun;
    fbe_api_raid_group_class_get_info_t rg_class_info;
    fbe_lba_t                           lun_capacity;
    fbe_api_raid_group_get_info_t       rg_info;

    /* Fill up the logical unit configuration for each RAID group of this test index.
     * We round each capacity up to a default number of chunks. 
     */
    MUT_ASSERT_INT_EQUAL(FLIM_FLAM_NUM_RAID_GROUPS_TO_TEST, raid_group_count);
    MUT_ASSERT_INT_EQUAL(2, raid_group_count);
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            continue;
        }

        current_chunks_per_lun = chunks_per_lun;

        /* The first raid group is the user raid group.  The second is the system.
         */
        if (rg_index == FLIM_FLAM_USER_RAID_GROUP_INDEX)
        {
            /* For the given config create a LUN with the appropriate number of chunks.
             */
            MUT_ASSERT_INT_EQUAL(1, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);
            rg_class_info.width = rg_config_p[rg_index].width;
            rg_class_info.raid_type = rg_config_p[rg_index].raid_type;
            rg_class_info.single_drive_capacity = FBE_U32_MAX;
            rg_class_info.exported_capacity = FBE_LBA_INVALID; /* set to invalid to indicate not used. */
            status = fbe_api_raid_group_class_get_info(&rg_class_info, rg_config_p[rg_index].class_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
            /* Update the chunks per lun based of raid group configuration
             */
            current_chunks_per_lun = fbe_test_sep_util_increase_lun_chunks_for_config(rg_config_p,
                                                                                      current_chunks_per_lun,
                                                                                      rg_class_info.data_disks);

            /* Now round to the nearest chunks size
             */
            lun_capacity = (FBE_RAID_DEFAULT_CHUNK_SIZE *((fbe_block_count_t) current_chunks_per_lun)) * rg_class_info.data_disks;

            mut_printf(MUT_LOG_TEST_STATUS, "LUN: 0x%x, Capacity 0x%llx ", lun_number, (unsigned long long)lun_capacity);

            status = fbe_test_sep_util_fill_raid_group_lun_configuration(&rg_config_p[rg_index],
                                                                         1, /* Number to fill. */
                                                                         lun_number,
                                                                         luns_per_rg,
                                                                         lun_capacity);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            lun_number += luns_per_rg;
        }
        else
        {
            /* Else populate the system raid group information.
             */
            MUT_ASSERT_INT_EQUAL(1, FLIM_FLAM_NUM_SYSTEM_RAID_GROUPS_PER_TYPE);
            status = fbe_api_raid_group_get_info(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                                 &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            rg_config_p[rg_index].width         = rg_info.width;
            rg_config_p[rg_index].capacity      = rg_info.capacity;
            rg_config_p[rg_index].raid_type     = rg_info.raid_type;
            rg_config_p[rg_index].class_id      = FBE_CLASS_ID_MIRROR;
            rg_config_p[rg_index].block_size    = rg_info.imported_block_size;
            rg_config_p[rg_index].raid_group_id = FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR;
            rg_config_p[rg_index].b_bandwidth   = (rg_info.element_size == 128) ? 0 : 1;

            /* Now populate the raid group config lun information from the
             * private lun information.
             */
            status = flim_flam_fill_system_raid_group_lun_config(&rg_config_p[rg_index],
                                                                 1, /* Number to fill. */
                                                                 FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_VCX_LUN_0,
                                                                 luns_per_rg);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            lun_number += luns_per_rg;
        }

    } /* for all raid groups */

    return;
}
/******************************************
 * end flim_flam_setup_rg_config()
 ******************************************/

/*!**************************************************************
 * flim_flam_run_test_on_rg_config()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
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
 *  04/04/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static void flim_flam_run_test_on_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                            void * context_p,
                                            fbe_test_rg_config_test test_fn,
                                            fbe_u32_t luns_per_rg,
                                            fbe_u32_t chunks_per_lun)
{
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_u32_t raid_group_count_this_pass = 0;
    fbe_u32_t raid_group_count = 0;

    /* There should be exactly (1) raid group 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FLIM_FLAM_NUM_RAID_GROUPS_TO_TEST, raid_group_count);

    if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the array of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Get the raid group config for the first pass.
     */
    flim_flam_setup_rg_config(rg_config_p, &drive_locations);
    raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    MUT_ASSERT_INT_EQUAL(raid_group_count, raid_group_count_this_pass);

    /* Continue to loop until we have no more raid groups.
     */
    while (raid_group_count_this_pass > 0)
    {
        fbe_u32_t debug_flags;
        fbe_u32_t trace_level = FBE_TRACE_LEVEL_INFO;
        mut_printf(MUT_LOG_LOW, "testing %d raid groups \n", raid_group_count_this_pass);

        /* Create the raid groups.
         */
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Setup the lun capacity with the given number of chunks for each lun.
         */
        flim_flam_fill_lun_configurations_rounded(rg_config_p, raid_group_count,
                                                  chunks_per_lun, 
                                                  luns_per_rg);
        raid_group_count = flim_flam_get_user_raid_group_count(rg_config_p);

        /* Kick of the creation of all the raid group with Logical unit configuration.
         */
        fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);

        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_trace_level", &trace_level))
        {
            fbe_test_sep_util_set_rg_trace_level_both_sps(rg_config_p, (fbe_trace_level_t) trace_level);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, (fbe_raid_group_debug_flags_t)debug_flags);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_library_debug_flags_both_sps(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
        }
        /* With this option we will skip the test, we just create/destroy the configs.
         */
        if (fbe_test_sep_util_get_cmd_option("-just_create_config"))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s -just_create_config selected, skipping test\n", __FUNCTION__);
            mut_pause();
        }
        else
        {
            fbe_u32_t repeat_count = 1;
            fbe_u32_t loop_count = 0;
            /* The default number of times to run is 1, -repeat_count can alter this.
             */
            fbe_test_sep_util_get_cmd_option_int("-repeat_count", &repeat_count);
            /* Run the tests some number of times.
             * Note it is expected that a 0 repeat count will cause us to not run the test.
             */
            while (loop_count < repeat_count)
            {
                test_fn(rg_config_p, context_p);
                loop_count++;
                /* Only trace if the user supplied a repeat count.
                 */
                if (repeat_count > 0)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "test iteration [%d of %d] completed", loop_count, repeat_count);
                }
            }
        }

        /* Check for errors and destroy the rg config.
         */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);

        /* Setup the rgs for the next pass.
         */
        flim_flam_setup_rg_config(rg_config_p, &drive_locations);
        raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    }
    fbe_test_sep_util_report_created_rgs(rg_config_p);

    /* Done testing rgconfigs. Mark them invalid.
     */
    fbe_test_rg_config_mark_invalid(rg_config_p);
    return;
}
/******************************************
 * end flim_flam_run_test_on_rg_config()
 ******************************************/

/*!**************************************************************
 * flim_flam_run_test_on_rg_config_with_extra_disk()
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
static void flim_flam_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
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

    /* populate the required extra disk count in rg config */
    flim_flam_populate_rg_num_extra_drives(rg_config_p);

    /* Generally, the extra disks on rg will be configured as hot spares. In order to make configuring 
    particular PVDs(extra disks) as hot spares work, we need to set set automatic hot spare not working (set to false). */    
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* run the test */
    flim_flam_run_test_on_rg_config(rg_config_p, context_p, test_fn,
                                    luns_per_rg,
                                    chunks_per_lun);

    return;
}
/********************************************************
 * end flim_flam_run_test_on_rg_config_with_extra_disk()
 ********************************************************/

/*!***************************************************************************
 *          flim_flam_wait_for_raid_group_to_fail()
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
static void flim_flam_wait_for_raid_group_to_fail(fbe_test_rg_configuration_t *const rg_config_p)
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
    if (flim_flam_is_shutdown_allowed_for_raid_group(rg_config_p))
    {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/******************************************
 * end flim_flam_wait_for_raid_group_to_fail()
 ******************************************/

/*!**************************************************************
 * flim_flam_configure_extra_drives_as_hot_spares()
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
static void flim_flam_configure_extra_drives_as_hot_spares(fbe_test_rg_configuration_t *rg_config_p,
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
    fbe_u32_t                       spared_index = FLIM_FLAM_INVALID_DISK_POSITION;
    fbe_u32_t                       index_to_spare = FLIM_FLAM_INVALID_DISK_POSITION;

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
        spared_position = FLIM_FLAM_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {    
            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                              &&
                (spared_position == FLIM_FLAM_INVALID_DISK_POSITION)    )
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
                                                          FLIM_FLAM_DRIVE_WAIT_MSEC);
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
                                                              FLIM_FLAM_DRIVE_WAIT_MSEC);
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
 * end flim_flam_configure_extra_drives_as_hot_spares()
 *************************************************************/

/*!**************************************************************
 * flim_flam_wait_extra_drives_swap_in()
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
static void flim_flam_wait_extra_drives_swap_in(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count)
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
    fbe_u32_t                       spared_index = FLIM_FLAM_INVALID_DISK_POSITION;
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
        spared_position = FLIM_FLAM_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {
            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == FLIM_FLAM_INVALID_DISK_POSITION)    )
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
        spared_position = FLIM_FLAM_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < saved_num_removed_drives[index]; drive_number++)
        {
            /* Get the next position to insert a new drive. */
            position_to_insert = saved_position_to_reinsert[index][drive_number];

            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == FLIM_FLAM_INVALID_DISK_POSITION)    )
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
                                                          FLIM_FLAM_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    return;
}
/*************************************************************
 * flim_flam_wait_extra_drives_swap_in()
 *************************************************************/

/*!***************************************************************************
 *          flim_flam_get_next_position_to_insert()
 *****************************************************************************
 * 
 * @brief   Find the next position to insert using the removed array.
 *
 * @param   rg_config_p - Pointer to raid group config          
 *
 * @return  fbe_u32_t - Position to insert
 *
 *****************************************************************************/
static fbe_u32_t flim_flam_get_next_position_to_insert(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t   position_to_insert = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t   rg_index;

    /*! @note This assumes that drives have been removed
     */
    MUT_ASSERT_TRUE((fbe_s32_t)rg_config_p->num_removed > 0);
    MUT_ASSERT_TRUE(rg_config_p->num_removed <= rg_config_p->width); 
     
    /* Walk thru the removed array (skipping over the copy index) and
     * locate first valid position.
     */
    for (rg_index = 0; rg_index < rg_config_p->width; rg_index++)
    {
        /* Check if not invalid.
         */
        if (rg_config_p->drives_removed_array[rg_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
        {
            position_to_insert = rg_config_p->drives_removed_array[rg_index];
            break;
        }
    }
    MUT_ASSERT_TRUE(position_to_insert < rg_config_p->width); 
    return position_to_insert;
}
/*************************************************************
 * ehnd flim_flam_get_next_position_to_insert()
 *************************************************************/

/*!**************************************************************
 * flim_flam_insert_drives()
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
static void flim_flam_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_bool_t b_transition_pvd)
{
    fbe_u32_t index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t status;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_insert;

    /* For every raid group insert one drive.
     */
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        position_to_insert = flim_flam_get_next_position_to_insert(current_rg_config_p);
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
 * end flim_flam_insert_drives()
 ******************************************/

/*!**************************************************************
 * flim_flam_insert_all_removed_drives()
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
static void flim_flam_insert_all_removed_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                                 fbe_u32_t raid_group_count, 
                                                 fbe_u32_t drives_to_insert,
                                                 fbe_bool_t b_reinsert_same_drive)
{
    fbe_u32_t drive_number;
    fbe_u32_t num_to_insert = 0;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;

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
        flim_flam_configure_extra_drives_as_hot_spares(rg_config_p, raid_group_count);

        /* make sure that hs swap in complete 
         */        
        flim_flam_wait_extra_drives_swap_in(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);

        /* set target server to SPA (test expects SPA will be left as target server and previous call may change that) 
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    else
    {
        /* insert the same drive back 
         */
        for (drive_number = 0; drive_number < num_to_insert; drive_number++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive index %d of %d. ==", __FUNCTION__, drive_number, num_to_insert);
            flim_flam_insert_drives(rg_config_p, 
                                    FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE, 
                                    FBE_FALSE);    /* don't fail pvd */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive index %d of %d. Complete. ==", __FUNCTION__, drive_number, num_to_insert);
        }
    }

    return;
}
/********************************************
 * end flim_flam_insert_all_removed_drives()
 ********************************************/

/*!**************************************************************
 * flim_flam_start_io()
 ****************************************************************
 * @brief
 *  Run I/O.
 *  Degrade the raid group.
 *  Allow I/O to continue degraded.
 *  Bring drive back and allow it to rebuild.
 *  Stop I/O.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from flim_flam_start_io
 *
 ****************************************************************/

static void flim_flam_start_io(fbe_test_random_abort_msec_t ran_abort_msecs,
                           fbe_rdgen_operation_t rdgen_operation,
                           fbe_u32_t threads,
                           fbe_u32_t io_size_blocks)
{

    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
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

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end flim_flam_start_io()
 ******************************************/


/*!***************************************************************************
 *          flim_flam_check_if_all_threads_started()
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
static fbe_bool_t flim_flam_check_if_all_threads_started(fbe_api_rdgen_get_object_info_t  *object_info_p,                          
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
 * end flim_flam_check_if_all_threads_started()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_wait_for_rdgen_start()
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
static void flim_flam_wait_for_rdgen_start(fbe_u32_t wait_seconds)
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

            if (flim_flam_check_if_all_threads_started(object_info_p,thread_info_p))
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
 * end flim_flam_wait_for_rdgen_start()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_start_fixed_io()
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
static void flim_flam_start_fixed_io(fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t *context_p = &flim_flam_test_contexts[0];
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
                                             FLIM_FLAM_FIXED_PATTERN,
                                             0,    /* Run forever */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             fbe_test_sep_io_get_threads(flim_flam_threads),    /* threads */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* Number of stripes to write. */
                                             FLIM_FLAM_MAX_IO_SIZE_BLOCKS    /* Max blocks */ );
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
 * end flim_flam_start_fixed_io()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_wait_for_rebuild_start()
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
static void flim_flam_wait_for_rebuild_start(fbe_test_rg_configuration_t * const rg_config_p,
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
 * end flim_flam_wait_for_rebuild_start()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_wait_for_rebuild_completion()
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
static void flim_flam_wait_for_rebuild_completion(fbe_test_rg_configuration_t * const rg_config_p,
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
 * end flim_flam_wait_for_rebuild_completion()
 **********************************************/

/*!**************************************************************
 * flim_flam_raid_group_refresh_disk_info()
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
static void flim_flam_raid_group_refresh_disk_info(fbe_test_rg_configuration_t * rg_config_p, 
                                                    fbe_u32_t raid_group_count)
{

    fbe_u32_t                               rg_index = 0;
    fbe_u32_t                               index = 0;
    fbe_u32_t                               removed_drive_index = 0;
    fbe_u32_t                               drive_type = 0;
    fbe_u32_t                               extra_drive_index = 0;
    fbe_u32_t                               unused_drive_index = 0;
    fbe_lba_t                               rg_drive_capacity = FBE_LBA_INVALID;
    fbe_lba_t                               max_rg_drive_capacity = FBE_LBA_INVALID;
    fbe_lba_t                               unused_drive_capacity = FBE_LBA_INVALID;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_block_size_t                   block_size;
    fbe_bool_t                              skip_removed_drive;
        

    /* find the all available drive info */
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* find the all available unused pvd info */
    fbe_test_sep_util_get_all_unused_pvd_location(&drive_locations, &unused_pvd_locations);

    /*! @note The current architecteure states that only user raid groups
     *        will be spared.  Thus skip all non-user raid groups.
     */    
    for ( rg_index = 0; rg_index < FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE; rg_index++)
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

            if(skip_removed_drive == FBE_TRUE)
            {
                continue;
            }
        
            /* find out the drive capacity */
            fbe_test_sep_util_get_drive_capacity(&rg_config_p[rg_index].rg_disk_set[index], &rg_drive_capacity);

            /* mut_printf(MUT_LOG_TEST_STATUS, "== refresh extra disk  -rg disk (%d_%d_%d), cap 0x%x . ==", 
                    rg_config_p[rg_index].rg_disk_set[index].bus, rg_config_p[rg_index].rg_disk_set[index].enclosure, rg_config_p[rg_index].rg_disk_set[index].slot, rg_drive_capacity );
            */

            if(max_rg_drive_capacity < rg_drive_capacity)
            {
                max_rg_drive_capacity = rg_drive_capacity;
            }
        }

        /* Set the index to 0 */
        extra_drive_index = 0;

        /* scan all the unused drive and pick up the drive which is not yet selected and match the capacity 
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

                    /*! @note Do not check-in enabled (too verbose)*/
                    
                    mut_printf(MUT_LOG_TEST_STATUS,  "filling extra disk pvd(%d_%d_%d) block size %d, drive type %d, rg drive capacity 0x%x, unused drive cnt %d, un dri cap 0x%x, rg index %d\n",
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].bus,
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].enclosure,
                        rg_config_p[rg_index].extra_disk_set[extra_drive_index].slot,
                        block_size, drive_type, (unsigned int)rg_drive_capacity, unused_pvd_locations.drive_counts[block_size][drive_type] ,
                        (unsigned int)unused_drive_capacity, rg_index); 
                    

                    extra_drive_index++;

                    /* invalid the drive slot information for selected drive so that next time it will skip from selection
                    */
                    unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot = FBE_INVALID_SLOT_NUM;

                    /* check if all extra drive selected for this rg 
                    */
                    if(extra_drive_index == rg_config_p[rg_index].num_of_extra_drives)
                    {
                        break;
                    }
                } /* end if((max_rg_drive_capacity == unused_drive_capacity) */

            } /* end if (unused_pvd_locations.disk_set[block_size][drive_type][unused_drive_index].slot!= FBE_INVALID_SLOT_NUM) */

        } /* end for(unused_drive_index =0; unused_drive_index <unused_pvd_locations.drive_counts[block_size][drive_type] ; unused_drive_index++) */

        /* We have taken into account the index reserved for copy
         */
        if (extra_drive_index != rg_config_p[rg_index].num_of_extra_drives)
        {
            /* We had make sure that have enough drives during setup. Let's fail the test and see
             * why could not get the suitable extra drive
             */
            mut_printf(MUT_LOG_TEST_STATUS,  "Do not find unused pvd, block size %d, drive type %d, max rg drive capacity 0x%x, unused drive cnt %d, rg index %d\n",
                block_size, drive_type, (unsigned int)max_rg_drive_capacity, unused_pvd_locations.drive_counts[block_size][drive_type], rg_index );

            /* fail the test to figure out that why test did not able to find out the unused pvd */
            MUT_ASSERT_TRUE_MSG(FBE_FALSE, "Do not find unused pvd while refresh extra disk info table\n");
        }
    } /* end for ( rg_index = 0; rg_index < raid_group_count; rg_index++) */

    return;
}
/**********************************************
 * end flim_flam_raid_group_refresh_disk_info()
 **********************************************/

/*!**************************************************************
 * flim_flam_remove_all_drives()
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

static void flim_flam_remove_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
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
        flim_flam_raid_group_refresh_disk_info(rg_config_p, raid_group_count);
    }

    /* For each raid group remove the specified number of drives.
     */
    for (drive_number = 0; drive_number < drives_to_remove; drive_number++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive index %d of %d. ==", __FUNCTION__, drive_number, drives_to_remove);
        big_bird_remove_drives(rg_config_p, 
                               FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE, 
                               FBE_FALSE,    /* don't fail pvd */
                               b_reinsert_same_drive,
                               removal_mode);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive index %d of %d. Completed. ==", __FUNCTION__, drive_number, drives_to_remove);
    }
    return;
}
/******************************************
 * end flim_flam_remove_all_drives()
 ******************************************/

/*!**************************************************************
 *          flim_flam_set_debug_flags()
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
void flim_flam_set_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                               fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;
    
    /* If debug is not enabled, simply return
     */
    if (flim_flam_b_debug_enable == FBE_FALSE)
    {
        return;
    }
    
    /* Populate the raid group debug flags to the value desired.
     * (there can only be 1 set of debug flag override)
     */
    raid_group_debug_flags = flim_flam_object_debug_flags;
        
    /* Populate the raid library debug flags to the value desired.
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(flim_flam_library_debug_flags);
    
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
     if (flim_flam_enable_state_trace == FBE_TRUE)
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
 * end flim_flam_set_debug_flags()
 ******************************************/

/*!**************************************************************
 *          flim_flam_clear_debug_flags()
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
void flim_flam_clear_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                             fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;

    /* If debug is not enabled, simply return
     */
    if (flim_flam_b_debug_enable == FBE_FALSE)
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
    if (flim_flam_enable_state_trace == FBE_TRUE)
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
 * end flim_flam_clear_debug_flags()
 ******************************************/

/*!***************************************************************************
 *          flim_flam_run_degraded_user_copy_without_io()
 *****************************************************************************
 *
 * @brief   Write a background pattern to the raid group, pull the designated
 *          system drive, wait for the spare to swap-in, wait for the rebuild
 *          to complete then initiate a copy back operation.  The sequence is:
 *              1.  Remove system drive.
 *              2.  Start user initiated copy operation.
 *              3.  When copy completes original spare drive is detached and
 *                  added back to spare pool.
 *              4.  New new drive is inserted into removed system drive slot.
 *              5.  Spare drive is rebuilt from other raid group positions.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group
 * @param   b_inject_random_aborts - FBE_TRUE - Periodically inject aborts 
 * @param   b_dualsp_enabled - FBE_TRUE dualsp mode testing is enabled   
 * @param   rdgen_operation - The type of rdgen operation (write, zero etc) 
 *
 * @return None.
 *
 * @author
 *  04/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void flim_flam_run_degraded_user_copy_without_io(fbe_test_rg_configuration_t * const rg_config_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_u32_t luns_per_rg,
                                                        fbe_bool_t b_inject_random_aborts,
                                                        fbe_bool_t b_dualsp_enabled,
                                                        fbe_rdgen_operation_t rdgen_operation)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   index;
    fbe_u32_t                   remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    /*! @note The `copy-back' operation uses the user API of selecting a source and destination. */
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    fbe_u32_t                   source_position = FLIM_FLAM_INDEX_RESERVED_FOR_COPY;

    /* We write the entire raid group and read it all back after the 
     * rebuild has finished. 
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        flim_flam_write_zero_background_pattern(rg_config_p, raid_group_count, luns_per_rg,
                                                b_inject_random_aborts, b_dualsp_enabled);
    }
    else
    {
        flim_flam_write_background_pattern(rg_config_p, raid_group_count, luns_per_rg,
                                           b_inject_random_aborts, b_dualsp_enabled);
    }

    /* Disable sparing by moving all spares to the `automatic' spare pool.
     */
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: Remove the system drive.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing system drive: (%d_%d_%d) ==", 
           __FUNCTION__,  flim_flam_drive_to_remove.bus, flim_flam_drive_to_remove.enclosure, flim_flam_drive_to_remove.slot);
    flim_flam_remove_all_drives(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE, 1, 
                                FBE_FALSE,      /* We don't plan on re-inserting the same drive.*/ 
                                remove_delay,   /* msecs between removals */
                                FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing system drive - Successful. ==", __FUNCTION__);

   /* Set debug flags.
    */
    flim_flam_set_debug_flags(rg_config_p, raid_group_count);

    /* Step 2: Start user initiated (copy-back) from spare to new system drive
     *         The `destination' array is simply the extra drive array.  The
     *         destination comes from the `extra_drives' array and the swap-in
     *         occurs because it should select a drive from the automatic pool.
     */
    MUT_ASSERT_TRUE(rg_config_p->num_of_extra_drives >= FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting user copy for: (%d_%d_%d) ==", 
               __FUNCTION__, flim_flam_drive_to_remove.bus, flim_flam_drive_to_remove.enclosure, flim_flam_drive_to_remove.slot);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE,
                                                              source_position,
                                                              copy_type,
                                                              &rg_config_p->extra_disk_set[0]);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__); 

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE,
                                                                          source_position,
                                                                          copy_type,
                                                                          FBE_TRUE,  /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Make sure the background pattern can be read still.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how FLIM_FLAM_FIXED_PATTERN currently defined)here. 
         * If FLIM_FLAM_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        flim_flam_read_fixed_pattern(rg_config_p, raid_group_count, luns_per_rg, 
                                     b_inject_random_aborts, b_dualsp_enabled); 
    }
    else
    {
        flim_flam_read_background_pattern(rg_config_p, raid_group_count, luns_per_rg, 
                                          b_inject_random_aborts, b_dualsp_enabled); 
    }

    /* Enable sparing by moving all the spares back to the hot-spare pool.
     */
    status = fbe_test_sep_util_add_hotspares_to_hotspare_pool(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Replace the system drive with a new one.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Insert new drive into system drive slot. ==", __FUNCTION__);
    flim_flam_insert_all_removed_drives(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE, 1,
                                    FBE_FALSE /* Insert a new drive into the system drive slot. */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Insert new drive into system drive slot - Successful. ==", __FUNCTION__);

    /* Make sure all objects are ready.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready- Successful ==", __FUNCTION__);
    
    /* Step 4: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        flim_flam_wait_for_rebuild_completion(current_rg_config_p,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Make sure the background pattern can be read still.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how FLIM_FLAM_FIXED_PATTERN currently defined)here. 
         * If FLIM_FLAM_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        flim_flam_read_fixed_pattern(rg_config_p, raid_group_count, luns_per_rg, 
                                     b_inject_random_aborts, b_dualsp_enabled);  
    }
    else
    {
        flim_flam_read_background_pattern(rg_config_p, raid_group_count, luns_per_rg, 
                                          b_inject_random_aborts, b_dualsp_enabled);  
    }

    return;
}
/***********************************************************
 * end of flim_flam_run_degraded_user_copy_without_io()
 ***********************************************************/

/*!***************************************************************************
 *          flim_flam_run_tests_for_config()
 *****************************************************************************
 *
 * @brief   Run the set of tests that will validate that copy operations work
 *          with more than (1) raid group bound on the associated drives.
 *
 * @param   rg_config_p - Config to create.
 * @param   raid_group_count - Total number of raid groups.
 * @param   luns_per_rg - Luns to create per rg.
 * @param   b_inject_random_aborts - FBE_TRUE - Periodically inject aborts 
 * @param   b_dualsp_enabled - FBE_TRUE dualsp mode testing is enabled            
 *
 * @return none
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
void flim_flam_run_tests_for_config(fbe_test_rg_configuration_t * rg_config_p, 
                                    fbe_u32_t raid_group_count, 
                                    fbe_u32_t luns_per_rg,
                                    fbe_bool_t b_inject_random_aborts,
                                    fbe_bool_t b_dualsp_enabled)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_spare_swap_command_t    copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;

    /*! @note Since this is a degraded test suite we assume that we will be
     *        degrading the raid groups.  Since the test assume a specific
     *        drive will be spared, disable automatic sparing and then move
     *        all the spare drives to the automatic spare pool.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize our `short' and `long' I/O times
     */
    flim_flam_set_io_seconds();

    /*! @note Copy operations (of any type) are not supported for RAID-0 raid groups.
     *        Therefore skip these tests if the raid group type is RAID-0.  
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        /* Just run a sanity test for RAID-0.
         */
        flim_flam_write_background_pattern(rg_config_p, raid_group_count, luns_per_rg,
                                           b_inject_random_aborts, b_dualsp_enabled);

        /* Print a message and return.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Skipping test with copy_type: %d.  Copy operations not supported on RAID-0 ==",
                   __FUNCTION__, copy_type);
        return;
    }

    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Display some info.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running for test level %d", __FUNCTION__, test_level);

    /* If we are doing shutdown only, then skip the degraded tests.
     */
    if (!fbe_test_sep_util_get_cmd_option("-shutdown_only"))
    {
        /* Test 1: Simple Degraded I/O test
         *
         * Run a test where we write a pattern, pull a system drive, (go degraded)
         * then initiate a copy on a different position.  
         */
        flim_flam_run_degraded_user_copy_without_io(rg_config_p, raid_group_count, luns_per_rg,
                                                    b_inject_random_aborts,
                                                    b_dualsp_enabled,
                                                    FBE_RDGEN_OPERATION_WRITE_READ_CHECK);
    }

    /*! @note Move all spares back to the `default' mode of automatic sparing.
     */
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, FLIM_FLAM_NUM_USER_RAID_GROUPS_PER_TYPE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the spare swap-in timer back to default of 5 minutes.
     */
	fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    return;
}
/******************************************
 * end flim_flam_run_tests_for_config()
 ******************************************/

/*!**************************************************************
 *          flim_flam_run_tests()
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
static void flim_flam_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       raid_group_count = 0;
    fbe_u32_t       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_bool_t      b_inject_random_aborts = FBE_FALSE;
    fbe_bool_t      b_dualsp_enabled = FBE_FALSE;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Configure the injection of random aborts based on test level
     */
    if (test_level > 0)
    {
        b_inject_random_aborts = FBE_TRUE;
    }

    /* Configure the dualsp mode.
     */
    b_dualsp_enabled = fbe_test_sep_util_get_dualsp_test_mode();
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

    /* Always run the standard test without aborts
     */
    flim_flam_run_tests_for_config(rg_config_p, raid_group_count, FLIM_FLAM_LUNS_PER_RAID_GROUP, 
                                   FBE_FALSE,           /* We alwasy run a pass w/o injecting aborts*/
                                   b_dualsp_enabled);

    /* If the test level is greater than 0, run a second pass with random
     * abort testing enabled.
     */
    if ((test_level > 0)                                      &&
        (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)    )
    {
        /*! @todo Due to DE543 enabling abort testing will encounter an issue
         *        with aborting metadata I/O during the copy.
         */
        if (flim_flam_b_is_abort_testing_disabled_due_to_DE543 == FBE_TRUE)
        {
            /*! @todo Fix the issue so that we can run a second pass with random
             *        abort testing enabled.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s Skipping abort testing due to DE543.", 
                       __FUNCTION__);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s Test started with random aborts msecs : %d", 
                        __FUNCTION__, FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC);
            flim_flam_run_tests_for_config(rg_config_p, raid_group_count, FLIM_FLAM_LUNS_PER_RAID_GROUP,
                                           b_inject_random_aborts,
                                           b_dualsp_enabled); 
        }
    }

    return;
}
/**************************************
 * end flim_flam_run_tests()
 **************************************/

/*!**************************************************************
 * flim_flam_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from flim_flam_test.c
 *
 ****************************************************************/
void flim_flam_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        raid_group_types = FLIM_FLAM_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        raid_group_types = FLIM_FLAM_QUAL_TEST_CONFIGURATION_TYPES;
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
            rg_config_p = &flim_flam_raid_groups_extended[raid_group_type_index][0];
        }
        else
        {
            rg_config_p = &flim_flam_raid_groups_qual[raid_group_type_index][0];
        }

        /* Flim Flam will use hot-spares */
        flim_flam_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, flim_flam_run_tests,
                                       FLIM_FLAM_LUNS_PER_RAID_GROUP,
                                       FLIM_FLAM_CHUNKS_PER_LUN);

    } /* for all raid group types */

    return;
}
/******************************************
 * end flim_flam_test()
 ******************************************/

/*!**************************************************************
 * flim_flam_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  04/03/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
void flim_flam_setup(void)
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
            array_p = &flim_flam_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &flim_flam_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            flim_flam_populate_rg_num_extra_drives(&array_p[rg_index][0]);
        }

        /*! @todo Determine and load the physical config and populate all the 
         *        raid groups.
         */
        flim_flam_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);

        sep_config_load_sep_and_neit();
    }
    
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end flim_flam_setup()
 **************************************/

/*!**************************************************************
 * flim_flam_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the flim_flam test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from flim_flam_test.c
 *
 ****************************************************************/

void flim_flam_cleanup(void)
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
 * end flim_flam_cleanup()
 ******************************************/

/*!**************************************************************
 * flim_flam_dualsp_test()
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

void flim_flam_dualsp_test(void)
{
    fbe_u32_t                       raid_group_type_index;
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    fbe_u32_t                       raid_group_types;
    fbe_test_rg_configuration_array_t *array_p = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        raid_group_types = FLIM_FLAM_EXTENDED_TEST_CONFIGURATION_TYPES;
    }
    else
    {
        raid_group_types = FLIM_FLAM_QUAL_TEST_CONFIGURATION_TYPES;
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
            rg_config_p = &flim_flam_raid_groups_extended[raid_group_type_index][0];
            array_p = &flim_flam_raid_groups_extended[0];
        }
        else
        {
            rg_config_p = &flim_flam_raid_groups_qual[raid_group_type_index][0];
            array_p = &flim_flam_raid_groups_qual[0];
        }

        /* This populates and then executes a test for (1) raid group type. */
        flim_flam_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, flim_flam_run_tests,
                                       FLIM_FLAM_LUNS_PER_RAID_GROUP,
                                       FLIM_FLAM_CHUNKS_PER_LUN);

    } /* for all raid group types */

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end flim_flam_dualsp_test()
 ******************************************/

/*!**************************************************************
 * flim_flam_dualsp_setup()
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
void flim_flam_dualsp_setup(void)
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
            array_p = &flim_flam_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            array_p = &flim_flam_raid_groups_qual[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&array_p[rg_index][0]);

            /* Initialize the number of extra drives required by RG. 
             * Used when create physical configuration for RG in simulation. 
             */
            flim_flam_populate_rg_num_extra_drives(&array_p[rg_index][0]);
        }

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /*! @todo Determine and load the physical config and populate all the 
         *        raid groups.
         */
        flim_flam_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /*! @todo Determine and load the physical config and populate all the 
         *        raid groups.
         */
        flim_flam_rg_config_array_with_extra_drives_load_physical_config(&array_p[0]);

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
 * end flim_flam_dualsp_setup()
 **************************************/

/*!**************************************************************
 * flim_flam_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup flim_flam dual sp test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void flim_flam_dualsp_cleanup(void)
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
 * end flim_flam_dualsp_cleanup()
 ******************************************/


/****************************
 * end file flim_flam_test.c
 ****************************/


