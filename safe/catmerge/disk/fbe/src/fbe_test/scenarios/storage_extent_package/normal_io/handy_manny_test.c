/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file handy_manny_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a zeroing I/O test.
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
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "fbe_test_common_utils.h"

#include "sep_test_io.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * handy_manny_short_desc = "Raid Zeroing I/O test.";
char * handy_manny_long_desc ="\
The HandyManny scenario tests zero fill I/O against all unit types.\n\
\n\
    -raid_test_level 0 and 1\n\
        - Additional combinations of raid group widths and element sizes are tested with -rtl 1.\n\
\n\
STEP 1: Configure all raid groups and LUNs.\n\
        - Tests cover both 512 and 520 block sizes.\n\
        - Tests cover striper (r0), mirror (r1), parity (r5, r3, r6) zeroing.\n\
        - All raid groups have 3 LUNs each.\n\
STEP 2: Start rdgen zero/read/compare test to all LUNs.\n\
        - I/O will run in parallel to all LUNs.\n\
        - Rdgen will choose lbas and block counts randomly for each pass.\n\
        - Each pass will perform a zero, read, and compare.\n\
STEP 3: Allow I/O to continue for several passes.\n\
STEP 4: Stop rdgen.\n\
        - Verify that rdgen I/O did not encounter errors.\n\
STEP 5: Destroy raid groups and LUNs.\n\
\n\
Additional tests to add:\n\
After 100 passes, degrade the raid groups by powering off a single drive in each rg.\n\
        - Wait to determine the drives are powered off.\n\
        - Validate the raid groups are marked needs rebuild.\n\
Allow I/O to continue for 100 more passes.\n\
Power back up the drives.\n\
        - Validate the virtual drives and provisioned drives are ready.\n\
        - Wait for the rebuild to complete.\n\
Run all tests with R10 raid group as well.\n\
\n\
Desription last updated: 9/28/2011.\n";

/* The number of drives in our raid group.
 */
#define HANDY_MANNY_TEST_RAID_GROUP_WIDTH 5 

/* The number of data drives in our raid group.
 */
#define HANDY_MANNY_TEST_RAID_GROUP_DATA_DRIVES (HANDY_MANNY_TEST_RAID_GROUP_WIDTH - 1)

/*!*******************************************************************
 * @def HANDY_MANNY_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define HANDY_MANNY_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def HANDY_MANNY_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define HANDY_MANNY_CHUNKS_PER_LUN 3

/* Element size of our LUNs.
 */
#define HANDY_MANNY_TEST_ELEMENT_SIZE 128 

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define HANDY_MANNY_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (0xE000)

/* The number of blocks in a raid group bitmap chunk.
 */
#define HANDY_MANNY_TEST_RAID_GROUP_CHUNK_SIZE  FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH

/*!*******************************************************************
 * @def HANDY_MANNY_DEFAULT_IO_COUNT
 *********************************************************************
 * @brief Max number of ios we will test with.
 *
 *********************************************************************/
#define HANDY_MANNY_DEFAULT_IO_COUNT 5

/*!*******************************************************************
 * @def HANDY_MANNY_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define HANDY_MANNY_LARGE_IO_SIZE_MAX_BLOCKS HANDY_MANNY_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def HANDY_MANNY_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define HANDY_MANNY_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @var handy_manny_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t handy_manny_test_contexts[HANDY_MANNY_TEST_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @def HANDY_MANNY_TEST_CONFIGURATIONS_COUNT
 *********************************************************************
 * @brief this is the number of test configurations
 *
 *********************************************************************/
#define HANDY_MANNY_TEST_CONFIGURATIONS_COUNT 5

/*!*******************************************************************
 * @def HANDY_MANNY_TEST_CONFIGURATIONS_COUNT_QUAL
 *********************************************************************
 * @brief this is the number of test configurations
 *
 *********************************************************************/
#define HANDY_MANNY_TEST_CONFIGURATIONS_COUNT_QUAL 1

/*!*******************************************************************
 * @var handy_manny_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t handy_manny_raid_group_config_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] =
#else
fbe_test_rg_configuration_array_t handy_manny_raid_group_config_extended[] =
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/    
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            2,          0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          0},

        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            8,          1},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            9,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {
        /* width, capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/    
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,           0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,           0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            3,           0},
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,           1},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            5,           1},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            6,           1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {
        /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            1,          0},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            2,          0},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
        {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            4,          1},
        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            5,          1},
        {16,      0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            6,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {
        /* width, capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/    
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            1,          0},
        {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            2,          0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            3,          1},
        {9,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            4,          1},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {
        /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0},
        {3,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            3,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end handy_manny_raid_group_config_extended
 **************************************/

/*!*******************************************************************
 * @var handy_manny_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t handy_manny_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t handy_manny_raid_group_config_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {1,       0x8000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            8,          0},
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            10,         0},

        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            16,          0},

        {6,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            22,          0},

        {2,       0x8000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/**************************************
 * end handy_manny_raid_group_config_qual
 **************************************/

/*!**************************************************************
 * handy_manny_test_setup_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context structure to run a random I/O test.
 *
 * @param context_p - Context of the operation.
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param capacity - capacity in blocks.
 * @param max_passes - Number of passes to execute.
 * @param io_size_blocks - io size to fill with in blocks.
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t handy_manny_test_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                       fbe_object_id_t object_id,
                                                       fbe_class_id_t class_id,
                                                       fbe_rdgen_operation_t rdgen_operation,
                                                       fbe_lba_t capacity,
                                                       fbe_u32_t max_passes,
                                                       fbe_u32_t threads,
                                                       fbe_u32_t io_size_blocks,
                                                       fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;
    status = fbe_api_rdgen_test_context_init(context_p, 
                                         object_id, 
                                         class_id,
                                         FBE_PACKAGE_ID_SEP_0,
                                         rdgen_operation,
                                         pattern,
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
    return FBE_STATUS_OK;
}
/******************************************
 * end handy_manny_test_setup_rdgen_test_context()
 ******************************************/
/*!**************************************************************
 * handy_manny_test_setup_fill_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context for a fill operation.  This operation
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
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t handy_manny_test_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                            fbe_object_id_t object_id,
                                                            fbe_class_id_t class_id,
                                                            fbe_rdgen_operation_t rdgen_operation,
                                                            fbe_lba_t max_lba,
                                                            fbe_u32_t io_size_blocks,
                                                            fbe_rdgen_pattern_t pattern,
                                                            fbe_u32_t num_passes,
                                                            fbe_u32_t num_ios)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id,
                                             class_id, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             pattern,
                                             num_passes,    /* We do one full sequential pass. */
                                             num_ios,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             max_lba,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             io_size_blocks    /* Max blocks */ );
    return status;
}
/**************************************
 * end handy_manny_test_setup_fill_rdgen_test_context()
 **************************************/
/*!**************************************************************
 * handy_manny_run_zero_io()
 ****************************************************************
 * @brief
 *  Run zero I/O.
 *
 * @param rg_config_p - Current configuration.          
 *
 * @return None.
 *
 * @author
 *  3/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
void handy_manny_run_zero_io(fbe_test_rg_configuration_t *rg_config_p, void * context_ptr)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &handy_manny_test_contexts[0];

    status = handy_manny_test_setup_fill_rdgen_test_context(context_p,
                                                            FBE_OBJECT_ID_INVALID,
                                                            FBE_CLASS_ID_LUN,
                                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                            FBE_LBA_INVALID,    /* use max capacity */
                                                            HANDY_MANNY_LARGE_IO_SIZE_MAX_BLOCKS,
                                                            FBE_RDGEN_PATTERN_LBA_PASS,
                                                            1, /* passes */
                                                            0 /* I/O count not used */);
    /* Write a background pattern to the LUNs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s writing background pattern to all LUNs. ", __FUNCTION__);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s writing background pattern to all LUNs...Finished. ", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    //MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.pass_count, raid_group_count * num_luns);
    
    /* Run some random zero read check I/O.
     */
    status = handy_manny_test_setup_rdgen_test_context(context_p,
                                                       FBE_OBJECT_ID_INVALID,
                                                       FBE_CLASS_ID_LUN,
                                                       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,
                                                       FBE_LBA_INVALID,    /* use capacity */
                                                       fbe_test_sep_io_get_large_io_count(HANDY_MANNY_DEFAULT_IO_COUNT),
                                                       fbe_test_sep_io_get_threads(FBE_U32_MAX),    /* threads */
                                                       HANDY_MANNY_LARGE_IO_SIZE_MAX_BLOCKS,
                                                       FBE_RDGEN_PATTERN_ZEROS);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running zero/read/compare I/Os to all LUNs. max blocks: %d threads: %d passes: %d", 
               __FUNCTION__, HANDY_MANNY_LARGE_IO_SIZE_MAX_BLOCKS, 
               fbe_test_sep_io_get_threads(FBE_U32_MAX), fbe_test_sep_io_get_large_io_count(HANDY_MANNY_DEFAULT_IO_COUNT) );
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running zero/read/compare I/Os to all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/**************************************
 * end handy_manny_run_zero_io()
 **************************************/
/*!**************************************************************
 * handy_manny_test()
 ****************************************************************
 * @brief
 *  Run zero I/O.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

void handy_manny_test(void)
{
    fbe_u32_t                   test_index;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t config_count;

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {
        config_count = HANDY_MANNY_TEST_CONFIGURATIONS_COUNT;
    }
    else
    {
        config_count = HANDY_MANNY_TEST_CONFIGURATIONS_COUNT_QUAL;
    }

    /* Loop over the number of configurations we have.
     */
    for (test_index = 0; test_index < config_count; test_index++)
    {
        if (test_level > 0)
        {
            rg_config_p = &handy_manny_raid_group_config_extended[test_index][0];
        }
        else
        {
            rg_config_p = &handy_manny_raid_group_config_qual[test_index][0];
        }
        if (test_index + 1 >= config_count) {
            fbe_test_run_test_on_rg_config(rg_config_p, NULL, handy_manny_run_zero_io,
                                           HANDY_MANNY_TEST_LUNS_PER_RAID_GROUP,
                                           HANDY_MANNY_CHUNKS_PER_LUN);
        }
        else {
            fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, NULL, handy_manny_run_zero_io,
                                           HANDY_MANNY_TEST_LUNS_PER_RAID_GROUP,
                                           HANDY_MANNY_CHUNKS_PER_LUN,
                                           FBE_FALSE);
        }
    }
    return;
}
/******************************************
 * end handy_manny_test()
 ******************************************/
/*!**************************************************************
 * handy_manny_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
void handy_manny_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_array_t *rg_config_p = NULL;
        fbe_u32_t config_count;
        fbe_u32_t test_index;

        if (test_level > 0)
        {
            rg_config_p = &handy_manny_raid_group_config_extended[0];
            config_count = HANDY_MANNY_TEST_CONFIGURATIONS_COUNT;
        }
        else
        {
            rg_config_p = &handy_manny_raid_group_config_qual[0];
            config_count = HANDY_MANNY_TEST_CONFIGURATIONS_COUNT_QUAL;
        }
        for (test_index = 0; test_index < config_count; test_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(&rg_config_p[test_index][0]);
        }
        fbe_test_sep_util_rg_config_array_load_physical_config(rg_config_p);
    elmo_load_sep_and_neit();
    }
    return;
}
/**************************************
 * end handy_manny_setup()
 **************************************/

/*!**************************************************************
 * handy_manny_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the handy_manny test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

void handy_manny_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
    fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end handy_manny_cleanup()
 ******************************************/
/*************************
 * end file handy_manny_test.c
 *************************/


