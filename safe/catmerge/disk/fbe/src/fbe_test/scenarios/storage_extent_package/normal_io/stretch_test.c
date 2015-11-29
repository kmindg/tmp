/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file stretch_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid 3 objects.
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
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * stretch_short_desc = "Corrupt Data is used to validate RAID's reconstruct algorithms.";
char * stretch_long_desc ="\
The Stretch scenario injects `Corrupt Data' on Write requests and then validates\n\
that the `lost' data is reconstructed from redundancy.  This test exercises all \n\
raid types.  Both non-degraded and degraded raid groups are tested.  Supported  \n\
raid test levels are 0 and 1.  The primary differences between test level 0 and \n\
test level 1 is the number of raid groups and the size of the regions where the \n\
`Corrupt Data' is injected.\n\
\n"\
"\
STEP 1: configure all raid groups and LUNs.     \n\
        -Each raid group contains a single LUN  \n\
        -The configuration for both test levels contains at least (1) raid group \n\
         that use 512-bps drives for each raid group type.  \n\
\n"\
"\
STEP 2: For each set of raid groups (the raid groups in a set are always of the \n\
        same type) run the following sequence:      \n\
        -Write a background pattern                 \n\
        -Run the `Corrupt Data' test non-degraded   \n\
            +For (10) iterations run the following  \n\
                oUsing a random lba and size issue `Corrupt Data' write     \n\
                oIssue Read to same range and (except for RAID-0) validate  \n\
                    >Read completes successfully    \n\
                    >No invalidations               \n\
\n"\
"\
STEP 3: Run the degraded version of the test.   \n\
        -Zero the entire LUN.                   \n\
        -Issue `Corrupt Data' write using the pre-configured range. \n\
        -Issue Read request for same range.                         \n\
            +If the raid group is degraded then we expect the read to fail. \n\
            +If the raid still has redundancy (i.e. 3-way mirror, RAID-6)   \n\
             we expect the Read to succeed without any invalidations.       \n\
\n"\
"\
STEP 4: While still degraded run (1) thread random I/O  \n\
        -Write background pattern                       \n\
        -Run Write `Corrupt Data'/Read/Compare, without `check checksum'    \n\
        -Validate that if the sector was `Corrupt Data', Read returns       \n\
         `Corrupt Data'.  If the sector was written with the rdgen pattern  \n\
         validate the rdgen pattern.                    \n\
\n"\
"\
STEP 5: Re-insert the drives    \n\
\n"\
"\
STEP 6: If the raid groups still has redundancy repeat STEPS 3 and 4.   \n\
\n"\
"\
STEP 7: Destroy raid groups and LUNs    \n\
\n"\
"\
Test Specific Switches: \n\
        None            \n\
\n"\
"\
Outstanding Issues: \n\
        Looks like we are not forcing dual redundacy raid groups to become  \n\
        degraded.   \n\
\n"\
"\
Description Last Updated:   \n\
        09/23/2011          \n\
\n";

/* Imported functions */

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define STRETCH_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/* The number of LUNs in our raid group.
 */
#define STRETCH_TEST_LUNS_PER_RAID_GROUP 1

/* The number of blocks in a raid group bitmap chunk.
 */
#define STRETCH_TEST_RAID_GROUP_CHUNK_SIZE  FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH

/*!*******************************************************************
 * @def STRETCH_IO_SECONDS
 *********************************************************************
 * @brief Time to run I/O for.
 *
 *********************************************************************/
#define STRETCH_IO_SECONDS 5

/*!*******************************************************************
 * @def STRETCH_TEST_MAX_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define STRETCH_TEST_MAX_RAID_GROUP_COUNT 10

/*!*******************************************************************
 * @def STRETCH_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define STRETCH_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def TRETCH_MAX_DRIVES_TO_REMOVE
 *********************************************************************
 * @brief Max number of drives the test will remove from a RAID Group.
 * The worst case is R10, where we remove half the drives (one in each mirror)
 *********************************************************************/
#define STRETCH_MAX_DRIVES_TO_REMOVE (STRETCH_TEST_MAX_WIDTH/2)

/*!*******************************************************************
 * @def STRETCH_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with. Don't do more then 128
 * to avoid creating uncorrectable errors (more then 1 error in a stripe)
 *
 *********************************************************************/
#define STRETCH_SMALL_IO_SIZE_MAX_BLOCKS 128

#define STRETCH_CORRUPT_DATA_LOOPS 10

/*!*******************************************************************
 * @def ELMO_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define STRETCH_LARGE_IO_SIZE_MAX_BLOCKS STRETCH_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def STRETCH_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define STRETCH_CHUNKS_PER_LUN      9 /* Only (1) LU per raid group. */

/*!*******************************************************************
 * @def STRETCH_VERIFY_WAIT_SECONDS
 *********************************************************************
 * @brief Number of seconds we should wait for the verify to finish.
 *
 *********************************************************************/
#define STRETCH_VERIFY_WAIT_SECONDS 30

/*!*******************************************************************
 * @var stretch_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t stretch_test_contexts[STRETCH_TEST_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var STRETCH_PARITY_CHUNK
 *********************************************************************
 * @brief This is the set of parity chunk sizes we use for each rg.
 * They are used in setting up the various error cases.  The bandwidth 
 * case is different because the data alignment is different.
 *********************************************************************/

#define STRETCH_PARITY_CHUNK (FBE_RAID_SECTORS_PER_ELEMENT * FBE_RAID_ELEMENTS_PER_PARITY)

#define STRETCH_BANDWIDTH_PARITY_CHUNK FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 


/*!*******************************************************************
 * @def STRETCH_MAX_CONFIGS
 *********************************************************************
 * @brief The number of configurations we will test in extended mode.
 *
 *********************************************************************/
#define STRETCH_MAX_CONFIGS 14

/*!*******************************************************************
 * @var stretch_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 *********************************************************************/
fbe_test_logical_error_table_test_configuration_t stretch_raid_groups_extended[] = 
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
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            0, FBE_U32_MAX  /* Drives to power off */
        },
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
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
    },

    { /* Raid 1 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,          0},
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
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
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
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
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
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
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            2, FBE_U32_MAX  /* Drives to power off */
        },
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

/*!*******************************************************************
 * @var stretch_raid_groups_qual
 *********************************************************************
 * @brief Test config for raid test level 0.
 *
 *********************************************************************/
fbe_test_logical_error_table_test_configuration_t stretch_raid_groups_qual[] = 
{
    
    { /* Raid 0 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            0, FBE_U32_MAX  /* Drives to power off */
        },
    },
   
    
    { /* Raid 10 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {4,        0x1E000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
            {16,       0x1E000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
    },

    { /* Raid 1 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            0,          0},
            {2,       0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
    },
    
    { /* Raid 5 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
    },
    { /* Raid 3 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {9,       0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            1, FBE_U32_MAX  /* Drives to power off */
        },
    },
    { /* Raid 6 */
        { 
            /* width, capacity    raid type,                  class,                  block size      RAID-id     bandwidth*/
            {4,       0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            0,          0},
            {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            1,          1},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {
            0, 1, FBE_U32_MAX, /* Error tables. */
        },
        { 
            2, FBE_U32_MAX  /* Drives to power off */
        },
    },
    {{{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */ }}},
};

/*!*******************************************************************
 * @struct stretch_expected_status_t
 *********************************************************************
 * @brief Specify the array of status expected by a specific raid group.
 *        We currently only have two types of status
 *
 *********************************************************************/
typedef struct stretch_expected_status_s
{
    fbe_status_t non_parity_status[STRETCH_CORRUPT_DATA_LOOPS];
    fbe_status_t parity_status[STRETCH_CORRUPT_DATA_LOOPS];
}
stretch_expected_status_t;

/* This set of status is for degraded corrupt data write. */
stretch_expected_status_t degraded_write_status = 
{
    {FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, 
        FBE_STATUS_OK, FBE_STATUS_OK},
    {FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, FBE_STATUS_OK, 
        FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE}
};

/* For the non-degraded test, the LBAs to corrupt are chosen randomly.  For the degraded case,
 * the LBAs will be both random and specified explicitly to avoid hitting a disk that has been removed. Since the 
 * above structures are used in calls to functions outside this test, they have been preserved and 
 * The LBAs are specified in new structures described below.
 */

/*!*******************************************************************
 * @struct stretch_error_case_t
 *********************************************************************
 * @brief Specify the sectors to corrupt and read back in a single degraded test I/O.
 *
 *********************************************************************/
typedef struct stretch_error_case_s
{
    fbe_lba_t error_lba;        /*!< Start LBA of error to inject */
    fbe_u32_t error_length;     /*!< Length (in blocks) of error to inject */
}
stretch_error_case_t;

/*!*******************************************************************
 * @struct fbe_stretch_test_drive_removed_info_t
 *********************************************************************
 * @brief Structure to track info on drives we have removed for
 *        a given raid group.
 *
 *********************************************************************/
typedef struct fbe_stretch_test_drive_removed_info_s
{
    fbe_u32_t num_removed; /*! Number of drives removed so far. */
    /*! Array of drive positions in rg we removed. 
     */
    fbe_u32_t drives_removed_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t num_needing_spare; /*!< Total drives to spare. */
}
fbe_stretch_test_drive_removed_info_t;

/*!*******************************************************************
 * @struct stretch_rg_error_desc_t
 *********************************************************************
 * @brief Ror a RG, specify the drives to remove and the sectors to corrupt and read back.
 *
 *********************************************************************/
typedef struct stretch_rg_error_desc_s
{
    fbe_stretch_test_drive_removed_info_t   drives_to_remove;   /*!< Array of drives to remove */
    stretch_error_case_t error[STRETCH_CORRUPT_DATA_LOOPS];             /*!< Array of descriptions of errors to inject */
}
stretch_rg_error_desc_t; 

/*!*******************************************************************
 * @struct stretch_rg_type_error_desc_t
 *********************************************************************
 * @brief The error insertion table
 *
 *********************************************************************/
typedef struct stretch_rg_type_error_desc_s
{
    stretch_rg_error_desc_t rg_error[FBE_TEST_MAX_RAID_GROUP_COUNT];    /*!< Each element contains all the errors to inject into a given RG */
}
stretch_rg_type_error_desc_t;

/*!*******************************************************************
 * @struct stretch_context_t
 *********************************************************************
 * @brief The context containing the number of drives to power down
 *        and the table of errors
 *
 *********************************************************************/
typedef struct stretch_context_s
{
    fbe_u32_t num_drives_to_power_down;
	stretch_rg_type_error_desc_t *rg_type_error_desc;
    
}
stretch_context_t;

void stretch_r10_handle_hooks(fbe_test_rg_configuration_t* rg_config_p,
                              fbe_lba_t       lba,
                              fbe_block_count_t   blocks);

/*  Error Criteria:
 *
 * General:  We reinitialize between I/Os, in order to prevent seeing previous errors if verify is invoked.
 *           The number of drives to be removed will be set during the test. The field is inited to zero.
 *
 *  R0 -  Not used in the degraded test.
 *  R10 - Only test with all specfied drives removed. One drive is removed in each mirrored pair.
 *  R1 -  The is the simplest test since mirror data does not rotate amoung drives.
 *  R5/R3 -  The last IO is sent to the degraded drive
 *  R6 -  We test with both one drive removed (partially degraded) and two drives 
 *        removed (fully degraded). The last IO is sent to the degraded drive
 */
/*!*******************************************************************
 * @var stretch_degraded_error_cases_extended
 *********************************************************************
 * @brief IOs and drives to remove for raid test level 1 and greater.
 *
 *********************************************************************/ 
stretch_rg_type_error_desc_t  stretch_degraded_error_cases_extended[STRETCH_MAX_CONFIGS] =
{
    // R0 - these are just place holders  --- they are unused.
    { { {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}} } },

    // R10 -untested
    /* {num drives, {drives to remove array}}, {{error lba, error length}, ...}*/
    { { {{0, {1,3,5,7,9,11,13,15,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x85, 3}, {0x95, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {0,2,4,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {0,2,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {0,2,4,6,8,10,12,14,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {1,3,5,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {1,3,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {0,2,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}} } }, 

    // R1
    /* {num, {drives to remove array}}, {{error lba, error length}, ...}*/
    { { {{0, {1,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 15}, {0x95, 30}, {0x199,40}, {0x233,47}, {0x318,7}, {0x444,4}, {0x515,199}, {0x999,127}}}  ,
    {{0, {0,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 15}, {0x95, 30}, {0x199,40}, {0x233,47}, {0x318,7}, {0x444,4}, {0x515,199}, {0x999,127}}},
    {{0, {0,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 15}, {0x95, 30}, {0x199,40}, {0x233,47}, {0x318,7}, {0x444,4}, {0x515,199}, {0x999,127}}} } },

    // R5 520 - width 5
    /* {num, {drives to remove array}}, */
    { { {{0, {1,FBE_U32_MAX}},
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {8*STRETCH_PARITY_CHUNK+5, 1} }}, 

    // R5 520 - width 3
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {2*FBE_RAID_SECTORS_PER_ELEMENT, 2}, {2*FBE_RAID_SECTORS_PER_ELEMENT+1, 0x41}, {STRETCH_PARITY_CHUNK,0x10}, 
    {2*STRETCH_PARITY_CHUNK,0x21},  {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},
    /* pos: 1 has been removed: we expected this to fail.*/  
    {STRETCH_PARITY_CHUNK+FBE_RAID_SECTORS_PER_ELEMENT+0x21, 1} }},

    // R5 520 - width 16
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    // This hits the dead position, position 1 so that we get an error.
    {0x2cc0, 1} }}, 
    
    // R5 520 Bandwidth -width 5
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH -FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,0x21},  {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +FBE_RAID_SECTORS_PER_ELEMENT ,0x21},  
    {3*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH+3, 1} }},

    // R5 520 Bandwidth -width 3
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2- FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 + 2*FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2,0x79},    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 +3*FBE_RAID_SECTORS_PER_ELEMENT ,0x21},  
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH + FBE_RAID_SECTORS_PER_ELEMENT/2, 1} }},

    // R5 520 Bandwidth -width 16
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH -FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,0x21},  {2*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +FBE_RAID_SECTORS_PER_ELEMENT,0x21},
    {14*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH+FBE_RAID_SECTORS_PER_ELEMENT/2 ,0x21},  
     }},

    // R5 512 - width 16
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-30 , 10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {2*STRETCH_PARITY_CHUNK-2*FBE_RAID_SECTORS_PER_ELEMENT+2,0x21} }} }}, 

    // R3 512 - width 5
    /* {num, {drives to remove array}}, */
    { { {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {4*FBE_RAID_SECTORS_PER_ELEMENT-0x30,1} }}, 

    // R3 520 - width 9
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {7*FBE_RAID_SECTORS_PER_ELEMENT+1,1} }}, 
    
    // R3 520 Bandwidth -width 5
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH -FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,0x21},  {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +FBE_RAID_SECTORS_PER_ELEMENT ,0x21},  
    {3*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, 1} }},

    // R3 520 Bandwidth -width 9
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH -FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,0x21},  {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +FBE_RAID_SECTORS_PER_ELEMENT ,0x21},  
    {7*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +5,1} }},

    // R3 512 - width 5
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {STRETCH_PARITY_CHUNK-(FBE_RAID_SECTORS_PER_ELEMENT/2),1} }} }}, 

    // R6 520 - width 4
    /* {num, {drives to remove array}}, */
    { { {{0, {0,1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK),0x21},    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {3*STRETCH_PARITY_CHUNK+FBE_RAID_SECTORS_PER_ELEMENT,1} }}, 

    // R6 520 - width 6
    /* {num, {drives to remove array}}, */
    {{0, {2,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {STRETCH_PARITY_CHUNK,0x10}, 
    {2*STRETCH_PARITY_CHUNK,0x21},  {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {(8*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT+2,1} }}, 

    // R6 520 - width 16
    /* {num, {drives to remove array}}, */
    {{0, {0,2,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {14*STRETCH_PARITY_CHUNK+FBE_RAID_SECTORS_PER_ELEMENT+(FBE_RAID_SECTORS_PER_ELEMENT/2),1} }}, 
    
    // R6 520 Bandwidth -width 4
    /* {num, {drives to remove array}}, */
    {{0, {2,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2- FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 + 2*FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2,0x79}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH-0x21,0x21},  
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +3*FBE_RAID_SECTORS_PER_ELEMENT ,1} }},

    // R6 520 Bandwidth -width 6
    /* {num, {drives to remove array}}, */
    {{0, {2,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH -FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,0x21},  {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +FBE_RAID_SECTORS_PER_ELEMENT ,0x21},  
    {3*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH+1,1} }},

    // R6 520 Bandwidth -width 16
    /* {num, {drives to remove array}}, */
    {{0, {2,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/8, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/4)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT, 0x39}, {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH/2+ FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x41},
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH -FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,0x21},  {FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +FBE_RAID_SECTORS_PER_ELEMENT ,0x21},  
    {13*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH +2,0x21} }},

    // R6 512 - width 4
    /* {num, {drives to remove array}}, */
    {{0, {1,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-26 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {3*STRETCH_PARITY_CHUNK+(FBE_RAID_SECTORS_PER_ELEMENT/2),1} }}  }}, 


    { { {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  , // Terminator
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}},
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}} } },
};

/*!*******************************************************************
 * @var stretch_degraded_error_cases_qual
 *********************************************************************
 * @brief IOs and drives to remove for raid test level 0.
 *
 *********************************************************************/ 
stretch_rg_type_error_desc_t  stretch_degraded_error_cases_qual[STRETCH_MAX_CONFIGS] =
{
    // R0 - these are just place holders  --- they are unused.
    { { {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  ,
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}} } },

    // R10 -untested
    /* {num, {drives to remove array}}, {{error lba, error length}, ...}*/
    { { {{0, {1,3,5,7,9,11,13,15,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 3}, {0x95, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {0,2,4,6,8,10,12,14,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}}, 
    {{0, {0,2,4,6,8,10,12,14,FBE_U32_MAX}},  {{0,1}, {0x79, 2}, {FBE_RAID_SECTORS_PER_ELEMENT, 3}, {STRETCH_PARITY_CHUNK -2, 4}, {0x199,5}, {0x233,6}, {0x318,7}, {0x444,8}, {0x515,9}, {0x999,10}}} } }, 

    // R1
    /* {num, {drives to remove array}}, {{error lba, error length}, ...}*/
    { { {{0, {1,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 15}, {0x95, 30}, {0x199,40}, {0x233,47}, {0x318,7}, {0x444,4}, {0x515,199}, {0x999,127}}}  ,
    {{0, {0,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 15}, {0x95, 30}, {0x199,40}, {0x233,47}, {0x318,7}, {0x444,4}, {0x515,199}, {0x999,127}}},
    {{0, {0,FBE_U32_MAX}},  {{0,1}, {0x71, 2}, {0x80, 15}, {0x95, 30}, {0x199,40}, {0x233,47}, {0x318,7}, {0x444,4}, {0x515,199}, {0x999,127}}} } },

    // R5 520 - width 5
    /* {num, {drives to remove array}}, */
    { { {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{5,1}, {FBE_RAID_SECTORS_PER_ELEMENT, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,0x1F}, {FBE_RAID_SECTORS_PER_ELEMENT-16 , 0x11}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x1F, 0x20}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {4*FBE_RAID_SECTORS_PER_ELEMENT-0x20, 1} }}, 

    // R5 520 Bandwidth - width 16
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {5*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  {8*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {14*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH,1} }},

    // R5 512 - width 5
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {8*STRETCH_PARITY_CHUNK+0x21,1} }} }}, 

    // R3 520 - width 5
    /* {num, {drives to remove array}}, */
    { { {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {3*FBE_RAID_SECTORS_PER_ELEMENT+0x48, 1} }}, 

    // R3 520 Bandwidth - width 9
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {3*FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {5*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  {6*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {8*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH-(FBE_RAID_SECTORS_PER_ELEMENT/2),1} }},

    // R3 512 - width 5
    /* {num, {drives to remove array}}, */
    {{0, {1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2 - 1, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {7*FBE_RAID_SECTORS_PER_ELEMENT,0x21} }} }}, 

    // R6 520 - width 4
    /* {num, {drives to remove array}}, */
    { { {{0, {0,1,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2 - 1, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {3*STRETCH_PARITY_CHUNK+FBE_RAID_SECTORS_PER_ELEMENT+(FBE_RAID_SECTORS_PER_ELEMENT/2),1} }}, 

    // R6 520 Bandwidth - width 16
    /* {num, {drives to remove array}}, */
    {{0, {2,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2 - 1, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT-10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x20, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*FBE_RAID_SECTORS_PER_ELEMENT,0x10}, 
    {5*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  {8*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {13*FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH+(FBE_RAID_SECTORS_PER_ELEMENT/2),1} }},

    // R6 512 - width 4
    /* {num, {drives to remove array}}, */
    {{0, {1,0,FBE_U32_MAX}},  
    /* {{error lba, error length}, ...}*/  
    {{0,1}, {FBE_RAID_SECTORS_PER_ELEMENT/2 - 1, 1}, {(FBE_RAID_SECTORS_PER_ELEMENT/2)+ 10,20}, {FBE_RAID_SECTORS_PER_ELEMENT+10 , 30}, 
    {FBE_RAID_SECTORS_PER_ELEMENT + 0x40, 0x39}, {2*FBE_RAID_SECTORS_PER_ELEMENT, 0x41}, {4*STRETCH_PARITY_CHUNK,0x10}, 
    {(4*STRETCH_PARITY_CHUNK)+FBE_RAID_SECTORS_PER_ELEMENT,0x21},   {(4*STRETCH_PARITY_CHUNK)+2*FBE_RAID_SECTORS_PER_ELEMENT,0x21},  
    {3*STRETCH_PARITY_CHUNK+2,0x21} }} }}, 

    { { {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}}  , // Terminator
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}},
    {{0, {FBE_U32_MAX}},  {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}}} } },
};

void stretch_random_corrupt_data_test(fbe_api_rdgen_context_t *context_p,
                                                   fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_bool_t max_degraded,
                                                   stretch_rg_type_error_desc_t *per_rg_error_table);


/*!**************************************************************
 * stretch_r10_handle_hooks()
 ****************************************************************
 * @brief
 *  Cleanup the stretch test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/2012 - Created. Ashwin T.
 *
 ****************************************************************/

void stretch_r10_handle_hooks(fbe_test_rg_configuration_t* rg_config_p,
                              fbe_lba_t       lba,
                              fbe_block_count_t   blocks)
{
   fbe_object_id_t             rg_object_id;
   fbe_object_id_t             hook_object_id;
   fbe_scheduler_debug_hook_t  hook;
   fbe_status_t               status = FBE_STATUS_GENERIC_FAILURE;
   fbe_u32_t                  current_time = 0;
   fbe_u32_t                  timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
   //fbe_raid_geometry_t*       raid_geometry_p = NULL;
   fbe_u32_t                  number_of_objects_to_wait = 0;
   fbe_u32_t                  count = 0;
   fbe_object_id_t            object_id1 = FBE_OBJECT_ID_INVALID;
   fbe_u32_t                  index;
   fbe_api_base_config_downstream_object_list_t downstream_object_list;
    
    hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY;
    hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.action = SCHEDULER_DEBUG_ACTION_LOG;
    hook.val1 = 0;
    hook.val2 = 0;
    hook.counter = 0;

    if (blocks <= (128- (lba % 128)))
    {
       number_of_objects_to_wait = 1;
    }
    else
    {
        number_of_objects_to_wait = 2;
    }

    
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                 &rg_object_id);
    
    fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

    while (current_time < timeout_ms)
    {
        for (index = 0; index < downstream_object_list.number_of_downstream_objects; index++)
        {
            hook_object_id = downstream_object_list.downstream_object_list[index];
            if((hook_object_id == object_id1))
            {
                continue;
            }
            hook.object_id = hook_object_id; 
            status = fbe_api_scheduler_get_debug_hook(&hook);
            if(hook.counter != 0)
            {
                object_id1 = hook_object_id;
                count++;
                status = fbe_api_scheduler_del_debug_hook(hook_object_id,
                                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                      FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                      0,0,
                                                      SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                hook.counter =0;
                status = abby_cadabby_wait_for_raid_group_verify(hook_object_id, STRETCH_VERIFY_WAIT_SECONDS);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if(count == number_of_objects_to_wait)
                {
                    status = FBE_STATUS_OK;
                    break;
                }
            }
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }
    
    fbe_api_scheduler_clear_all_debug_hooks(&hook);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    return;
}
/******************************************
 * end stretch_r10_handle_hooks()
 ******************************************/

/*!**************************************************************
 * stretch_run_corrupt_data_test()
 ****************************************************************
 * @brief
 *
 * @param context_p - context to execute.
 * 
 * @return None.   
 *
 * @author
 *  5/26/10 - Kevin Schleicher Created
 *
 ****************************************************************/
void stretch_run_corrupt_data_test(fbe_api_rdgen_context_t *context_p,
                                   fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t  raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t loop_index;
	stretch_context_t * stretch_context_p = NULL;
    fbe_test_rg_configuration_t *orig_rg_config_p = rg_config_p;
    fbe_lba_t lba[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_u32_t blocks[FBE_TEST_MAX_RAID_GROUP_COUNT];
    fbe_element_size_t elsz[FBE_TEST_MAX_RAID_GROUP_COUNT];


    mut_printf(MUT_LOG_TEST_STATUS, "== Starting %s ==", __FUNCTION__);
    
	stretch_context_p = (stretch_context_t *) context_p;

    /* Randomly select block count.
     * Start at lba 0
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_api_raid_group_get_info_t rg_info;

        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {
            continue;
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        elsz[rg_index] = rg_info.element_size;
        lba[rg_index] = 0;
        blocks[rg_index] = 1 + (fbe_random() % STRETCH_SMALL_IO_SIZE_MAX_BLOCKS);
        rg_config_p++;
    }

    for (loop_index = 0; loop_index < STRETCH_CORRUPT_DATA_LOOPS; loop_index++)
    {
        rg_config_p = orig_rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++,
            rg_config_p++)
        {
            if (!fbe_test_rg_config_is_enabled(rg_config_p))
            {
                continue;
            }
            /* Now run the test for this test case.
             */
            status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                                  &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* wait for system verify complete, otherwise it will has unexpected effect on following steps
             */
            status = abby_cadabby_wait_for_raid_group_verify(rg_object_id, STRETCH_VERIFY_WAIT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            

            mut_printf(MUT_LOG_TEST_STATUS, "start lu: 0x%x rg: 0x%x lba: 0x%llx bl: 0x%llx elsz 0x%x",
                       lun_object_id, rg_object_id, (unsigned long long)lba[rg_index], (unsigned long long)blocks[rg_index], elsz[rg_index]);
            if (rg_config_p->class_id == FBE_CLASS_ID_PARITY)
            {
                status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                              FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                              0,0,
                                              SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            
            
            mut_printf(MUT_LOG_TEST_STATUS, "send corrupt lu: 0x%x rg: 0x%x lba: 0x%llx bl: 0x%llx elsz 0x%x",
                       lun_object_id, rg_object_id, (unsigned long long)lba[rg_index], (unsigned long long)blocks[rg_index], elsz[rg_index]);
            /*!  Send Corrupt Data op.
             *
             *   @todo Currently we corrupt the entire request range.
             *         (Set FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION)
             */
            status = fbe_api_rdgen_send_one_io(context_p,
                                               lun_object_id,
                                               FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_SEP_0,
                                               FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY,
                                               FBE_RDGEN_LBA_SPEC_FIXED,
                                               lba[rg_index],
                                               blocks[rg_index],
                                               (FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC | FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION), 
                                               0, 0,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);

            /* Make sure no error.
             */
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

            /* We only expect success.
             */
            if (context_p->start_io.status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "%s block status of %d is unexpected", 
                        __FUNCTION__, context_p->start_io.status.block_status);
                MUT_FAIL_MSG("block status from rdgen was unexpected");
            }


            mut_printf(MUT_LOG_TEST_STATUS, "send read lu: 0x%x rg: 0x%x lba: 0x%llx bl: 0x%llx elsz 0x%x",
                       lun_object_id, rg_object_id, (unsigned long long)lba[rg_index], (unsigned long long)blocks[rg_index], elsz[rg_index]);
            /* Send READ for hitting the insert crc error
             */
            status = fbe_api_rdgen_send_one_io(context_p,
                                               lun_object_id,
                                               FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_SEP_0,
                                               FBE_RDGEN_OPERATION_READ_ONLY,
                                               FBE_RDGEN_LBA_SPEC_FIXED,
                                               lba[rg_index],
                                               blocks[rg_index],
                                               0, 0, 0,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);

            if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
            {
                /* Raid 0's will be uncorrectable.
                 */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
                

                /* Blocks should have been invalidated.
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, blocks[rg_index]);
            }
            else
            {
                /* Make sure no error was found.
                 */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

                /* All CRC errors should have been fixed
                 */
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.bad_crc_blocks_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 0);
            }

            /* Advance the lba.
             */
            lba[rg_index] += 1 + (fbe_random() % STRETCH_SMALL_IO_SIZE_MAX_BLOCKS) + 
                             (STRETCH_SMALL_IO_SIZE_MAX_BLOCKS * rg_config_p->width);
            blocks[rg_index] = 1 + (fbe_random() % STRETCH_SMALL_IO_SIZE_MAX_BLOCKS);
        }

        rg_config_p = orig_rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++,
            rg_config_p++)
        {
            if (!fbe_test_rg_config_is_enabled(rg_config_p))
            {
                continue;
            }

            /* Wait for verify.
             */
            status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id,
                                                                  &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


            mut_printf(MUT_LOG_TEST_STATUS, "wait verify lu: 0x%x rg: 0x%x lba: 0x%llx bl: 0x%llx elsz 0x%x",
                       lun_object_id, rg_object_id, (unsigned long long)lba[rg_index], (unsigned long long)blocks[rg_index], elsz[rg_index]);
            if (rg_config_p->class_id == FBE_CLASS_ID_PARITY)
            {
                /* Parity groups need to wait for verify to clean up this stripe before proceeding to avoid 
                 * uncorrectables.  An uncorrectable could occur if verify did not fix this corruption before we added 
                 * another in the same strip. 
                 */
                status = fbe_test_wait_for_debug_hook(rg_object_id, 
                                                     SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                     FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                     SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                          FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                          0,0,
                                                          SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }

            status = abby_cadabby_wait_for_raid_group_verify(rg_object_id, STRETCH_VERIFY_WAIT_SECONDS);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Complete %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end stretch_run_corrupt_data_test()
 ******************************************/
/*!**************************************************************
 * stretch_run_non_degraded_random_corrupt_data_test()
 ****************************************************************
 * @brief
 *  Run a test where we issue random corrupt datas to non-degraded configs.
 *
 * @param context_p - context to execute.
 * @param rg_config_p - Raid group configuration.
 * 
 * @return None.   
 *
 * @author
 *  5/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void stretch_run_non_degraded_random_corrupt_data_test(fbe_api_rdgen_context_t *context_p,
                                                       fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t  raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
	stretch_context_t * stretch_context_p = (stretch_context_t *) context_p;

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting %s ==", __FUNCTION__);
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {            
            continue;
        }

        /* If the raid group has redundancy run a corrupt data/read/check test, 
         * where we expect the errors to be corrected. 
         */
        if (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)
        {
            fbe_api_rdgen_get_stats_t rdgen_stats;
            fbe_rdgen_filter_t filter;

            stretch_random_corrupt_data_test(context_p, rg_config_p, FBE_FALSE, /* Not max degraded */
                                             stretch_context_p->rg_type_error_desc);

            status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if ((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
                (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10))
            {
                /* We expect no errors.  We should be able to reconstruct the data.
                 */
                MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.error_count, 0);
            }
            else if (rg_config_p->class_id == FBE_CLASS_ID_PARITY)
            {
                MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.error_count, 
                                     rdgen_stats.historical_stats.media_error_count);
                MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.aborted_error_count, 0);
                MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.invalid_request_error_count, 0);
                MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.io_failure_error_count, 0);
            }
        }
        rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Complete %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end stretch_run_non_degraded_random_corrupt_data_test()
 ******************************************/

/*!**************************************************************
 * stretch_run_degraded_corrupt_data_test()
 ****************************************************************
 * @brief
 *
 * @param context_p   - context to execute.
 * @param rg_config_g - Struct with RG and test parameters
 * @param num_drives_to_power_down - Number of drives that will be powered down in this RG
 * @param max_degraded - If true, the RG is as degraded as it can be and still be functional
 * @param per_rg_error_table - Pointer to table of blocks to corrupt
 * 
 * @return None.   
 *
 * @author
 *  8/05/10 - Harvey Weiner Created
 *
 ****************************************************************/
void stretch_run_degraded_corrupt_data_test(fbe_api_rdgen_context_t *context_p,
                                   fbe_test_rg_configuration_t *rg_config_p,
                                   fbe_bool_t max_degraded,
                                   stretch_rg_type_error_desc_t *per_rg_error_table,
                                   fbe_status_t *expected_status)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_object_id_t rg_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t  raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t loop_index;
    fbe_test_rg_configuration_t *local_rg_config_p = rg_config_p;

    mut_printf(MUT_LOG_TEST_STATUS, "== Starting %s ==", __FUNCTION__);

    local_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++,
                                                     local_rg_config_p++)
    {
        fbe_lba_t       lba;
        fbe_u32_t       blocks;

        if (!fbe_test_rg_config_is_enabled(local_rg_config_p))
        {            
            continue;
        }

        /* Assuming 1 LUN per RG here */
        fbe_api_database_lookup_lun_by_number(local_rg_config_p->logical_unit_configuration_list->lun_number,
                    &lun_object_id);
        fbe_api_database_lookup_raid_group_by_number(local_rg_config_p->raid_group_id,
                    &rg_object_id);

        for (loop_index = 0; loop_index < STRETCH_CORRUPT_DATA_LOOPS; loop_index++)
        {
            
            /* Now run the test for this test case.
             */
            lba    = per_rg_error_table->rg_error[rg_index].error[loop_index].error_lba;
            blocks = per_rg_error_table->rg_error[rg_index].error[loop_index].error_length;

            /* Cleanup from any previous tests in the loop */
            status = fbe_api_rdgen_send_one_io(context_p,
                                               lun_object_id,
                                               FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_SEP_0,
                                               FBE_RDGEN_OPERATION_ZERO_ONLY,
                                               FBE_RDGEN_PATTERN_ZEROS,
                                               0,
                                               FBE_LBA_INVALID,    /* max lba */
                                               FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC, 
                                               0, 0,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);

             MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK , "zero data failed");


            /*! Send Corrupt Data op.
             *
             *   @todo Currently we corrupt the entire request range.
             *         (Set FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION)
             */
            status = fbe_api_rdgen_send_one_io(context_p,
                                               lun_object_id,
                                               FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_SEP_0,
                                               FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY,
                                               FBE_RDGEN_LBA_SPEC_FIXED,
                                               lba,
                                               blocks,
                                               (FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC | FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION), 
                                               0, 0,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);

            /* Check against the expected status array. */
            MUT_ASSERT_INT_EQUAL_MSG(status, expected_status[loop_index], "corrupt data failed");
            
            /* Further check error count. */
            if (status == FBE_STATUS_GENERIC_FAILURE)
            {
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status,
                    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier,
                    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.media_error_count, 1);
                
                /* If we get here, we didn't really write any data. Exit now. */
                break;
            }

            /* Make sure no error.
             */
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status,
                FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

            /* Send READ for hitting the insert crc error
             */
            status = fbe_api_rdgen_send_one_io(context_p,
                                               lun_object_id,
                                               FBE_CLASS_ID_INVALID,
                                               FBE_PACKAGE_ID_SEP_0,
                                               FBE_RDGEN_OPERATION_READ_ONLY,
                                               FBE_RDGEN_LBA_SPEC_FIXED,
                                               lba,
                                               blocks,
                                               0, 0, 0,
                                               FBE_API_RDGEN_PEER_OPTIONS_INVALID);

            if (max_degraded == FBE_TRUE) 
            {
                    /* If the RAID Group is completely degraded, an error should result in an uncorrectable.
                     * Blocks should have been invalidated.
                     */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, 
                    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier,
                    FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.media_error_count, 1);
                MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 0);
            }
           else 
                {
                /* Make sure no error was found.  All CRC errors should have been fixed.
                 */
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, 
                    FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.bad_crc_blocks_count, 0);
                MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.inv_blocks_count, 0);
            }

        }

        /* Do not insert drives here. They will be inserted after the degraded test. */
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== Complete %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end stretch_run_degraded_corrupt_data_test()
 ******************************************/

/*!**************************************************************
 * stretch_test_set_specific_drives_to_remove()
 ****************************************************************
 * @brief
 *   Initializes the raid group config with the specific drives to 
 *   be removed
 *
 * @param config_p
 * @param raid_group_count
 * @param stretch_error_tables
 * 
 * @return None.   
 * 
 *
 ****************************************************************/
void stretch_test_set_specific_drives_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                  fbe_u32_t raid_group_count,
                                                stretch_rg_type_error_desc_t *error_table_p)
                                                
{
    fbe_u32_t rg_index;

    for ( rg_index = 0; rg_index < raid_group_count; rg_index++, rg_config_p++)
    {
        if (!fbe_test_rg_config_is_enabled(rg_config_p))
        {            
            continue;
        }
        fbe_test_set_specific_drives_to_remove(rg_config_p,
                                              &(error_table_p->rg_error[rg_index].drives_to_remove.drives_removed_array[0]));
                                                                                                                                            
    }

}

/*!**************************************************************
 * stretch_random_corrupt_data_test()
 ****************************************************************
 * @brief Start rdgen test to all luns executing the degraded corrupt_data/read/check
 * opcode.
 *
 * @param context_p   - context to execute.
 * @param rg_config_g - Struct with RG and test parameters
 * @param num_drives_to_power_down - Number of drives that will be powered down in this RG
 * @param max_degraded - If true, the RG is as degraded as it can be and still be functional
 * @param per_rg_error_table - Pointer to table of blocks to corrupt
 * 
 * @return None.   
 *
 * @author
 *  12/15/10 - Ruomu Gao Created
 *
 ****************************************************************/
void stretch_random_corrupt_data_test(fbe_api_rdgen_context_t *context_p,
                                                   fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_bool_t max_degraded,
                                                   stretch_rg_type_error_desc_t *per_rg_error_table)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;
    fbe_u32_t io_seconds;
    fbe_rdgen_options_t rdgen_options = 0;
    mut_printf(MUT_LOG_TEST_STATUS, "== Starting %s ==", __FUNCTION__);

    /*
     * We allow the user to input a time to run I/O. 
     * By default io seconds is 0. 
     *  
     * By default we will run I/O until we see enough errors is encountered.
     */
    io_seconds = fbe_test_sep_util_get_io_seconds();
    

    /* Set up for issuing corrupt data/read/check forever
     */
    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_CLASS_ID_LUN,
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                0,      /* run forever*/ 
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                1,      /* threads per lun */
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                FBE_LBA_INVALID,    /* use capacity */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                STRETCH_SMALL_IO_SIZE_MAX_BLOCKS /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*! @todo Currently we always invalidate all the blocks.  In the future
     *        this should be randomized.
     */
    rdgen_options |= FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION;

    /* For degraded parity raid groups we cannot validate the `invalidation reason'.
     * Therefore set the flag to ignore this if it is the case.
     */
    if ((max_degraded == FBE_TRUE)                     &&
        (rg_config_p->class_id == FBE_CLASS_ID_PARITY)    )
    {
        rdgen_options |= FBE_RDGEN_OPTIONS_DO_NOT_CHECK_INVALIDATION_REASON;
    }

    /* We expect that if we still have some redundancy, then a corrupt data will be correctable on read.
     * We will check for the seed of 0, which was written as background pattern.
     */
    if (max_degraded == FBE_FALSE)
    {
        status = fbe_api_rdgen_set_sequence_count_seed(&context_p->start_io.specification, 0);
        rdgen_options |= FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED;
    }
    if (rg_config_p->class_id == FBE_CLASS_ID_PARITY)
    {
        /* When doing corrupt data on a parity raid type, it is possible to get more than one 
         * corrupt block on a strip at the same time, resulting in uncorrectables on the reads.
         */
        rdgen_options |= FBE_RDGEN_OPTIONS_INVALID_AND_VALID_PATTERNS;
    }
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                        rdgen_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (io_seconds > 0)
    {
        /* Let it run for a reasonable time before stopping it.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O for %d seconds ==", __FUNCTION__, io_seconds);
    }
    else
    {
        /* Let I/O run for a few seconds */
        if (fbe_sep_test_util_get_raid_testing_extended_level() > 0)
        {
            io_seconds = 5;
        }
        else
        {
            /* Sleep for less in the 'qual' version.
             */
            io_seconds = 5;
        }
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O for %d seconds ==", __FUNCTION__, io_seconds);
    fbe_api_sleep(io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND);

    /* Make sure I/O progressing okay */
    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Stop I/O */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== Completed %s ==", __FUNCTION__);
    return;
}
/******************************************
 * end stretch_random_corrupt_data_test()
 ******************************************/

/*!**************************************************************
 * stretch_run_tests_for_degraded_corrupt_config()
 ****************************************************************
 * @brief
 *   For each config, run corrupt CRC and remove drives test.
 *   This will run both fixed and random IO test.
 *
 * @param config_p - Struct with RG and test parameters
 * @param stretch_error_tables - Pointer to the error table
 * 
 * @return None.   
 *
 * @author
 *  11/22/10 - Ruomu Gao Pulled to a new function
 *
 ****************************************************************/
void stretch_run_tests_for_degraded_corrupt_config(fbe_api_rdgen_context_t *context_p,
                                                   fbe_test_rg_configuration_t *rg_config_p,
                                                   stretch_rg_type_error_desc_t *error_table_p,
                                                   fbe_u32_t num_drives_to_power_down)
{
    fbe_status_t status;
    fbe_u32_t   num_to_power_down;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t rg_index = 0;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;

    /* These RAID Groups are not participating in the degraded test, double check.
     */
    if( num_drives_to_power_down == 0 )
    {
        return;
    }
       
    for( num_to_power_down = 1; num_to_power_down <= num_drives_to_power_down ; num_to_power_down++ )
    {
        fbe_bool_t max_degraded = ( num_to_power_down == num_drives_to_power_down ) ?
            FBE_TRUE : FBE_FALSE;

        fbe_status_t *expected_status_p;

        /* RAID 10 is a special case because it has 2 layers - Striper and Mirror.
         * The current structure of the degraded test limits us to testing the case in which one disk in 
         * each mirrored pair is removed. This is because when we call stretch_run_degraded_corrupt_data_test()
         * we expect that the test I/O's will either all fail (max_degraded is FBE_TRUE) or succeed (max_degraded is FBE_FALSE).
         * Thus, we only run the test when half the drives in the RG will be removed.
         */

        /* Parity and non-parity raid group have different set of return status. */
        if ((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) ||
            (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3) ||
            (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6))
        {
            expected_status_p = degraded_write_status.parity_status;
        }
        else
        {
            expected_status_p = degraded_write_status.non_parity_status;
        }

        /* First remove drive(s) to degrade the RAID Group.
         * We will reinsert when done to allow further
         * testing with these configurations.
         */
        stretch_test_set_specific_drives_to_remove(rg_config_p, raid_group_count, error_table_p);
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) 
        {
            max_degraded = FBE_TRUE;

            /* For R10 we power off half the drives in the raid group, so we will 
             * get an expected result from every I/O when we are degraded. 
             */
            for (rg_index = 0; rg_index < raid_group_count; rg_index++)
            {
                big_bird_remove_all_drives(&rg_config_p[rg_index], 1, rg_config_p[rg_index].width / 2,
                                           FBE_TRUE, /* We are reinserting the same drive*/
                                           0,        /* do not wait between removals */
                                           FBE_DRIVE_REMOVAL_MODE_SPECIFIC);
            }
        }
        else
        {
            big_bird_remove_all_drives(rg_config_p, raid_group_count, num_to_power_down,
                                       FBE_TRUE,    /* We are reinserting the same drive*/
                                       0,    /* do not wait between removals */
                                       FBE_DRIVE_REMOVAL_MODE_SPECIFIC);         
        }

        /* For the degraded test, LUNs will be initialized within the test.
         * We remove drives in this test.
         */
        stretch_run_degraded_corrupt_data_test(context_p,
                                  rg_config_p,
                                  max_degraded,
                                  error_table_p,
                                  expected_status_p);

        /* Clean up the previous test data */
        big_bird_write_background_pattern();

        /* This test needs to run after the previous one, since it assume
         * the drives are removed from the previous test.
         * Drives are inserted in this test.
         */
        stretch_random_corrupt_data_test(context_p, rg_config_p, max_degraded, error_table_p);

        /* Make sure I/O progressing okay */
        status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* We either expect lots of media errors or no media errors. 
         * In either case the total error count and the media error count are equal. 
         */
        MUT_ASSERT_INT_EQUAL(rdgen_stats.historical_stats.error_count, rdgen_stats.historical_stats.media_error_count);

        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) 
        {
            /* For R10 we power off half the drives in the raid group, so we will 
             * get an expected result from every I/O when we are degraded. 
             */
            for (rg_index = 0; rg_index < raid_group_count; rg_index++)
            {
                big_bird_insert_all_drives(&rg_config_p[rg_index], 1, rg_config_p[rg_index].width / 2,
                                           FBE_TRUE /* We are reinserting the same drive*/);
            }
        }
        else
        {
            big_bird_insert_all_drives(rg_config_p, raid_group_count, num_to_power_down, 
                                       FBE_TRUE /* We are reinserting the same drive*/);
        }

    } /* end for all drives to remove */
}



/*!**************************************************************
 * stretch_test_set()
 ****************************************************************
 * @brief
 *   Set of tests to run for stretch
 *
 * @param config_p - Struct with RG and test parameters
 * @param stretch_error_tables - Pointer to the error table
 * 
 * @return None.   
 *
 * @author
 *  5/08/11 - Created. Deanna Heng
 *
 ****************************************************************/
void stretch_test_set(fbe_test_rg_configuration_t *rg_config_p, 
					  void * context_p)
{
	stretch_context_t * stretch_context_p = NULL;
	fbe_api_rdgen_context_t *rdgen_context_p = &stretch_test_contexts[0];
	fbe_u32_t num_to_power_down = 0;

	stretch_context_p = (stretch_context_t *) context_p;

	big_bird_write_background_pattern();
	stretch_run_corrupt_data_test(rdgen_context_p, rg_config_p);

	big_bird_write_background_pattern();
	stretch_run_non_degraded_random_corrupt_data_test(rdgen_context_p, rg_config_p);

	big_bird_write_background_pattern();

	/* Next, run degraded corrupt test for each raid type */
	num_to_power_down = stretch_context_p->num_drives_to_power_down;
	if (num_to_power_down > 0)
	{
		stretch_run_tests_for_degraded_corrupt_config(rdgen_context_p,
			rg_config_p,
			stretch_context_p->rg_type_error_desc,
			num_to_power_down);
	}

	/* Make sure we did not get any trace errors.  We do this in between each
	* raid group test since it stops the test sooner. 
	*/
	fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
}

/******************************************
 * end stretch_test_set()
 ******************************************/
/*!**************************************************************
 * stretch_run_tests()
 ****************************************************************
 * @brief
 *   Create a config and run corrupt CRC and remove drives test.
 *
 * @param config_p - Struct with RG and test parameters
 * @param stretch_error_tables - Pointer to the error table
 * 
 * @return None.   
 *
 * @author
 *  8/05/10 - Harvey Weiner Created
 *
 ****************************************************************/
void stretch_run_tests(fbe_test_logical_error_table_test_configuration_t *config_p, 
                       stretch_rg_type_error_desc_t *stretch_error_tables,
                       fbe_raid_group_type_t raid_type)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t raid_type_index = 0;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t config_count = abby_cadabby_get_config_count(config_p);
	stretch_context_t stretch_context_p = {0, NULL};

    for (raid_type_index = 0; raid_type_index < config_count; raid_type_index++)
    {
        rg_config_p = &config_p[raid_type_index].raid_group_config[0];

        fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
        if (!fbe_sep_test_util_raid_type_enabled(rg_config_p->raid_type) ||
            (raid_type != rg_config_p->raid_type))
        {
            continue;
        }

		stretch_context_p.num_drives_to_power_down = config_p[raid_type_index].num_drives_to_power_down[0];
		stretch_context_p.rg_type_error_desc = &stretch_error_tables[raid_type_index];
		
        if (raid_type_index + 1 >= config_count) {
            /* Now create the raid groups and run the tests
             */
            fbe_test_run_test_on_rg_config(rg_config_p, 
                                           &stretch_context_p,
                                           stretch_test_set, 
                                           STRETCH_TEST_LUNS_PER_RAID_GROUP, 
                                           STRETCH_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, 
                                           &stretch_context_p,
                                           stretch_test_set, 
                                           STRETCH_TEST_LUNS_PER_RAID_GROUP, 
                                           STRETCH_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }
    }
    return;
}
/******************************************
 * end stretch_run_tests()
 ******************************************/

/*!**************************************************************
 * stretch_test()
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

void stretch_test(fbe_raid_group_type_t raid_type)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_logical_error_table_test_configuration_t *config_p = NULL;
    stretch_rg_type_error_desc_t *stretch_error_tables = NULL;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_p = &stretch_raid_groups_extended[0];
        stretch_error_tables = stretch_degraded_error_cases_extended;
    }
    else
    {
        config_p = &stretch_raid_groups_qual[0];
        stretch_error_tables = stretch_degraded_error_cases_qual;
    }

    /* Run the test.
     */
    stretch_run_tests(config_p, stretch_error_tables, raid_type);
    return;
}
/******************************************
 * end stretch_test()
 ******************************************/
/*!**************************************************************
 * stretch_setup()
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
void stretch_setup(fbe_raid_group_type_t raid_type)
{
    /* This test injects error */
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
            config_p = &stretch_raid_groups_extended[0];
        }
        else
        {
            config_p = &stretch_raid_groups_qual[0];
        }
        abby_cadabby_simulation_setup(config_p,
                                      FBE_TRUE);
        abby_cadabby_load_physical_config_raid_type(config_p, raid_type);

        elmo_load_sep_and_neit();
    }

    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    return;
}
/**************************************
 * end stretch_setup()
 **************************************/

/*!**************************************************************
 * stretch_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the stretch test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void stretch_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
void stretch_raid0_test(void)
{
    stretch_test(FBE_RAID_GROUP_TYPE_RAID0);
}
void stretch_raid1_test(void)
{
    stretch_test(FBE_RAID_GROUP_TYPE_RAID1);
}
void stretch_raid10_test(void)
{
    stretch_test(FBE_RAID_GROUP_TYPE_RAID10);
}
void stretch_raid3_test(void)
{
    stretch_test(FBE_RAID_GROUP_TYPE_RAID3);
}
void stretch_raid5_test(void)
{
    stretch_test(FBE_RAID_GROUP_TYPE_RAID5);
}
void stretch_raid6_test(void)
{
    stretch_test(FBE_RAID_GROUP_TYPE_RAID6);
}

void stretch_raid0_setup(void)
{
    stretch_setup(FBE_RAID_GROUP_TYPE_RAID0);
}
void stretch_raid1_setup(void)
{
    stretch_setup(FBE_RAID_GROUP_TYPE_RAID1);
}
void stretch_raid10_setup(void)
{
    stretch_setup(FBE_RAID_GROUP_TYPE_RAID10);
}
void stretch_raid5_setup(void)
{
    stretch_setup(FBE_RAID_GROUP_TYPE_RAID5);
}
void stretch_raid3_setup(void)
{
    stretch_setup(FBE_RAID_GROUP_TYPE_RAID3);
}
void stretch_raid6_setup(void)
{
    stretch_setup(FBE_RAID_GROUP_TYPE_RAID6);
}
/*************************
 * end file stretch_test.c
 *************************/


