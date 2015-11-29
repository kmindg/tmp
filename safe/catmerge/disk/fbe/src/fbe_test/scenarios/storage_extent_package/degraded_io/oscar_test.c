/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file oscar_test.c
 ***************************************************************************
 *
 * @brief   This file contains RAID-1 degraded I/O tests.  This currently
 *          includes simple rebuild.
 *
 * @version
 *  02/19/2010  Ron Proulx  - Created from oscar_test.c
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
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "sep_rebuild_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * oscar_short_desc = "RAID-1 - Degraded read and write I/O test.";
char * oscar_long_desc ="\n\
The Oscar scenario tests degraded mirror raid group I/O.It also tests shutdowns with I/O for mirror raid groups.\n\
\n\
-raid_test_level 0 and 1.\n\
   - We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
   - In addition to the normal test , we run the same test with random aborts at -rtl 1 .\n\
\n\
Test Specific Switches:\n\
   -shutdown_only - Only run the shutdown and quiesce tests.\n\
\n\
STEP 1: configure all raid groups and LUNs.\n\
        - We always test at least one rg with 512 block size drives.\n\
        - All raid groups have 3 LUNs each\n\
\n\
STEP 2: Quiesce/unquiesce test:\n\
        - Run write/read/compare I/O and send a quiesce to the raid group.  \n\
        - Then send an unquiesce to the raid group. \n\
        - Stop I/O and make sure there were no errors.\n\
\n\
STEP 3: Write a background pattern to all luns and then read it back and make sure it was written successfully\n\
\n\
STEP 4: If shutdown_only option wasn't used, perform steps 5-9:\n\
STEP 5: Full rebuild test without I/O: \n\
        - Write a background pattern to all luns and then read it back and make sure it was written successfully\n\
        - Pull drives to degrade all the raid groups\n\
        - Read and check backgorund pattern\n\
        - Replace removed drives \n\
        - Wait for all objects to become ready \n\
        - Wait for rebuilds to finish \n\
        - Read and check backgorund pattern\n\
\n\
STEP 6: Full rebuild test with I/O: \n\
        - Start write/read/check I/O , allow it to run for random time \n\
        - Pull drives to degrade all the raid groups\n\
        - Allow I/O to continue for random time \n\
        - Stop I/O \n\
        - If random aborts was not used, validate there were no errors. \n\
        - Write/read/check background pattern to all luns, make sure there were no errors\n\
        - Replace drives \n\
        - Wait for all objects to become ready \n\
        - Wait for rebuilds to finish \n\
        - Read and check background pattern \n\
        - Run write/read/check I/O to make sure there are no errors \n\
        - Allow I/O to run random time, and then stop I/O\n\
        - Start write/read/check I/O, allow it to run for random time \n\
        - Pull drives to degrade all the raid groups\n\
        - Allow I/O to run random time\n\
        - Stop I/O \n\
        - If random aborts was not used , validate there were no errors. \n\
        - Start write/read/check I/O \n\
        - Allow I/O to run random time\n\
        - Replace removed drive \n\
        - Wait for all objects to become ready \n\
        - Wait for rebuilds to finish \n\
        - Stop I/O \n\
        - If random aborts was not used , validate there were no errors.\n\
\n\
STEP 7: Rebuild logging test: \n\
        - Start write/read/check I/O \n\
        - Allow I/O to run random time\n\
        - Pull drives to degrade all the raid groups\n\
        - Allow I/O to run random time\n\
        - Stop I/O \n\
        - If random aborts was not used , validate there were no errors. \n\
        - Start write/read/check I/O \n\
        - Allow I/O to run random time\n\
        - Re-insert drives \n\
        - Wait for all objects to become ready \n\
        - Wait for rebuilds to finish \n\
        - Stop i/o \n\
        - If random aborts was not used , validate there were no errors. \n\
\n\
STEP 8: Differential degraded rebuild test (currently commented out):\n\
        - Start write/read/check I/O \n\
        - Allow I/O to run random time\n\
        - Pull drives to degrade all the raid groups\n\
        - Allow I/O to run random time\n\
        - Stop I/O \n\
        - If random aborts was not used , validate there were no errors. \n\
        - Start write/read/check I/O \n\
        - Allow I/O to run random time\n\
        - Re-insert drives \n\
        - Wait for rebuild to start on at least (1) raid group\n\
        - Pull drives to degrade all the raid groups\n\
        - Wait for all objects to become ready \n\
        - Wait for degraded rebuilds to finish \n\
        - Re-inserting drives \n\
        - Waiting for rebuilds to finish \n\
        - Stop I/O \n\
        - If random aborts was not used , validate there were no errors. \n\
\n\
STEP 9: Full rebuild test with zero operation: \n\
        Same as 6 , zero pattern used.\n\
\n\
STEP 10: Shutdown raid groups with fixed I/O\n\
        - Write a fixed pattern to all luns and then read it back and make sure it was written successfully\n\
        - Run fixed I/O to all Luns \n\
        - Allow I/O to run random time\n\
        - Wait for all rdgen threads to start \n\
        - Pull all drives for each raid group.\n\
        - Wait for all the raid groups to go to lifecycle state FAIL.\n\
        - Stop I/O.\n\
        - If random aborts was not used , validate there were no errors. \n\
        - Re-insert the drives.\n\
        - Wait for all objects to become ready.\n\
        - Read and check pattern \n\
        - Wait for all rebuilds to finish \n\
\n\
STEP 11: Shutdown during rebuild - currently commented out:\n\
        - Run write/read/check I/O to all luns.\n\
        - Allow I/O to run random time\n\
        - Wait for 1 position to be rebuilt\n\
        - Remove sufficient drives to degrade raid group\n\
        - Allow I/O to run random time\n\
        - Stop I/O\n\
        - If random aborts was not used , validate there were no errors. \n\
        - Put the drives back in.\n\
        - Wait for rebuild to start on at least (1) raid group \n\
        - Wait for all objects to become ready \n\
        - Pull all the drives to shutdown all the raid groups\n\
        - Make sure all raid groups goto fail \n\
        - Re-inserting all  drives\n\
        - Wait for all objects to become ready\n\
        - Wait for rebuilds to finish\n\
\n\
STEP 12: Shutdown raid groups with  I/O\n\
        - Run write/read/check I/O to all luns.\n\
        - Allow I/O to run random time\n\
        - Wait for all rdgen threads to start \n\
        - Pull all drives for each  raid group.\n\
        - Wait for all the raid groups to go to lifecycle state FAIL.\n\
        - Stop I/O.\n\
        - If random aborts was not used , validate there were no errors. \n\
        - Re-insert the drives.\n\
        - Wait for all objects to become ready.\n\
        - Quiesce/unquiesce raid groups for several iterations - currently commented out\n\
        - Wait for all rebuilds to finish \n\
\n\
STEP 13: For rtl 1 repeat steps 3-12 using random aborts I/O. \n\
\n\
Description last updated: 9/27/2011.\n";

/* Element size of our LUNs.
 */
#define OSCAR_TEST_ELEMENT_SIZE 128 


/*!*******************************************************************
 * @def OSCAR_QUIESCE_TEST_ITERATIONS
 *********************************************************************
 * @brief The number of iterations we will run the quiesce/unquiesce test.
 *
 *********************************************************************/
#define OSCAR_QUIESCE_TEST_ITERATIONS 1

/*!*******************************************************************
 * @def OSCAR_QUIESCE_TEST_MAX_ITERATIONS
 *********************************************************************
 * @brief The number of iterations we will run the quiesce/unquiesce test.
 *
 *********************************************************************/
#define OSCAR_QUIESCE_TEST_MAX_ITERATIONS 50

/*!*******************************************************************
 * @def OSCAR_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define OSCAR_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_BE_XFER_SIZE) // 4096 


/*!*******************************************************************
 * @def OSCAR_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Number of blocks in small io
 *
 *********************************************************************/
#define OSCAR_SMALL_IO_SIZE_BLOCKS 1024  

/*!*******************************************************************
 * @def OSCAR_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives per raid group we will test with.
 *
 *********************************************************************/
#define OSCAR_TEST_MAX_WIDTH 3

/*!*******************************************************************
 * @var oscar_threads
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t oscar_threads = 2;

/*!*******************************************************************
 * @var oscar_light_threads
 *********************************************************************
 * @brief Number of threads we will run for light I/O.
 *
 *********************************************************************/
fbe_u32_t oscar_light_threads = 2;

/*!*******************************************************************
 * @var oscar_test_debug_enable
 *********************************************************************
 * @brief Determines if debug should be enabled or not
 *
 *********************************************************************/
fbe_bool_t  oscar_test_debug_enable = FBE_FALSE;

/*!*******************************************************************
 * @var oscar_library_debug_flags
 *********************************************************************
 * @brief Default value of the raid library debug flags to set
 *
 *********************************************************************/
fbe_u32_t oscar_library_debug_flags = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING  |
                                       FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );

/*!*******************************************************************
 * @var oscar_object_debug_flags
 *********************************************************************
 * @brief Default value of the raid group object debug flags to set
 *
 *********************************************************************/
fbe_u32_t oscar_object_debug_flags = (FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING);

/*!*******************************************************************
 * @var oscar_enable_state_trace
 *********************************************************************
 * @brief Default value of enabling state trace
 *
 *********************************************************************/
fbe_bool_t oscar_enable_state_trace = FBE_TRUE;

/*!***************************************************************************
 * @var oscar_disable_3way_shutdown
 *****************************************************************************
 * @brief Determines if we allow shutdown testing of 3-way mirrors or not
 *        Default value is currently FBE_TRUE (i.e. 3-way shutdown tests are disabled)
 *
 *****************************************************************************/
fbe_bool_t oscar_disable_3way_shutdown = FBE_FALSE;

/*!*******************************************************************
 * @var OSCAR_REMOVE_DELAY
 *********************************************************************
 * @brief Delay in  milliseconds between drive removals
 *
 *********************************************************************/
fbe_u32_t OSCAR_REMOVE_DELAY = 250;

/*!*******************************************************************
 * @var OSCAR_INSERT_DELAY
 *********************************************************************
 * @brief Delay in  milliseconds between drive insertions
 *
 *********************************************************************/
fbe_u32_t OSCAR_INSERT_DELAY = 500;

/*!*******************************************************************
 * @def     OSCAR_FIXED_PATTERN
 *********************************************************************
 * @brief   rdgen fixed pattern to use
 *
 * @todo    Currently the only fixed pattern that rdgen supports is
 *          zeros.
 *
 *********************************************************************/
#define OSCAR_FIXED_PATTERN     FBE_RDGEN_PATTERN_ZEROS

/*!*******************************************************************
 * @def OSCAR_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.
 *
 *********************************************************************/
#define OSCAR_FIRST_POSITION_TO_REMOVE 0

/*!*******************************************************************
 * @def OSCAR_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define OSCAR_SECOND_POSITION_TO_REMOVE 1

/*!*******************************************************************
 * @def OSCAR_THIRD_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 3rd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define OSCAR_THIRD_POSITION_TO_REMOVE 2

/*!*******************************************************************
 * @def OSCAR_QUIESCE_WAIT_SECS
 *********************************************************************
 * @brief The number of seconds we will wait for quiesce/unquiesce
 *
 *********************************************************************/
#define OSCAR_QUIESCE_WAIT_SECS         60

/*!*******************************************************************
 * @def OSCAR_QUIESCE_TEST_MAX_SLEEP_TIME
 *********************************************************************
 * @brief Max time we will sleep for in the quiesce test.
 *        Each iteration of the queisce test does a sleep for a random
 *        amount of time from 0..N seconds.
 *********************************************************************/
#define OSCAR_QUIESCE_TEST_MAX_SLEEP_TIME 2000

/*!*******************************************************************
 * @def OSCAR_LONG_IO_SEC
 *********************************************************************
 * @brief Max time we will sleep for.
 *********************************************************************/
#define OSCAR_LONG_IO_SEC 5

/*!*******************************************************************
 * @def OSCAR_SHORT_IO_SEC
 *********************************************************************
 * @brief Min time we will sleep for.
 *********************************************************************/
#define OSCAR_SHORT_IO_SEC 2

/*!*******************************************************************
 * @def OSCAR_QUIESCE_TEST_THREADS_PER_LUN
 *********************************************************************
 * @brief The number of threads per lun for the quiesce/unquiesce test.
 *
 *********************************************************************/
#define OSCAR_QUIESCE_TEST_THREADS_PER_LUN 2

/*!*******************************************************************
 * @def OSCAR_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define OSCAR_WAIT_MSEC 60000

/*!*******************************************************************
 * @def OSCAR_TEST_RG_CAPACITY
 *********************************************************************
 * @brief capacity of mirror raid groups for oscar.  This determines
 *        how long each tests will take.
 *
 *********************************************************************/
#define OSCAR_TEST_RG_CAPACITY 0xE000

/*!*******************************************************************
 * @def OSCAR_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define OSCAR_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def OSCAR_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define OSCAR_CHUNKS_PER_LUN 9

/*!*******************************************************************
 * @def OSCAR_RDGEN_WAIT_SECS
 *********************************************************************
 * @brief The number of seconds we will wait for rdgen to start
 *
 *********************************************************************/
#define OSCAR_RDGEN_WAIT_SECS         60

/*!*******************************************************************
 * @var oscar_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t oscar_raid_group_config_extended[] = 
{
    /* width,   capacity,               raid type,                  class,               block size     RAID-id.    bandwidth.*/
    {2,         OSCAR_TEST_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {2,         OSCAR_TEST_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          1},
    {3,         OSCAR_TEST_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var oscar_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *        We need to test 3-way mirrors since the configuration luns
 *        use them.
 *
 *********************************************************************/
fbe_test_rg_configuration_t oscar_raid_group_config_qual[] = 
{
    /* width,   capacity,               raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {2,         OSCAR_TEST_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {2,         OSCAR_TEST_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          1},
    {3,         OSCAR_TEST_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            2,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @def OSCAR_TEST_MAX_TOTAL_LUNS
 *********************************************************************
 * @brief maximum total luns for all raid groups
 *
 *********************************************************************/
#define OSCAR_TEST_MAX_TOTAL_LUNS   3

/*!*******************************************************************
 * @var oscar_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t oscar_test_contexts[OSCAR_TEST_MAX_TOTAL_LUNS];

/*!*******************************************************************
 * @enum oscar_set_raid_group_state_t
 *********************************************************************
 * @brief Enumeration of different states to set the RAID-1 raid group
 *        to.
 *
 *********************************************************************/
typedef enum oscar_set_raid_group_state_e
{
    OSCAR_SET_RAID_GROUP_STATE_OPTIMAL = 0,
    OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
    OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN,
    OSCAR_SET_RAID_GROUP_STATE_RESTORE,
} oscar_set_raid_group_state_t;

/*!*******************************************************************
 * @var oscar_io_msec_long
 *********************************************************************
 * @brief The default I/O time we will run for long durations.
 *
 *********************************************************************/
static fbe_u32_t oscar_io_msec_long = 0;

/*!*******************************************************************
 * @var oscar_io_msec_short
 *********************************************************************
 * @brief The default I/O time we will run for short durations.
 * 
 *********************************************************************/
static fbe_u32_t oscar_io_msec_short = 0;

/*!**************************************************************
 * oscar_set_io_seconds()
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

void oscar_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();

    if (long_io_time_seconds >= OSCAR_LONG_IO_SEC)
    {
        oscar_io_msec_long = (long_io_time_seconds * 1000);
    }
    else
    {
        oscar_io_msec_long  = ((fbe_random() % OSCAR_LONG_IO_SEC) + 1) * 1000;
    }
    oscar_io_msec_short = ((fbe_random() % OSCAR_SHORT_IO_SEC) + 1) * 1000;
    return;
}
/******************************************
 * end oscar_set_io_seconds()
 ******************************************/

/*!**************************************************************
 * oscar_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from oscar_write_background_pattern
 *
 ****************************************************************/

void oscar_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
                                            OSCAR_TEST_ELEMENT_SIZE);
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
                                            OSCAR_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end oscar_write_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          oscar_write_fixed_pattern()
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
void oscar_write_fixed_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
                                             OSCAR_FIXED_PATTERN,
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
                                             OSCAR_TEST_ELEMENT_SIZE    /* Max blocks */ );
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
                                             OSCAR_FIXED_PATTERN,
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
                                             OSCAR_TEST_ELEMENT_SIZE    /* Max blocks */ );
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
 * end oscar_write_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          oscar_read_fixed_pattern()
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
void oscar_read_fixed_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
                                             OSCAR_FIXED_PATTERN,
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
                                             OSCAR_TEST_ELEMENT_SIZE    /* Max blocks */ );
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
 * end oscar_read_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          oscar_write_zero_background_pattern()
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
void oscar_write_zero_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
                                             OSCAR_TEST_ELEMENT_SIZE    /* Max blocks */ );
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
                                             OSCAR_TEST_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end oscar_write_fixed_pattern()
 ******************************************/

/*!**************************************************************
 *          oscar_quiesce_mirror_raid_groups()
 ****************************************************************
 *
 * @brief   Quiesce all mirror raid groups   
 *
 * @param total_iterations - Number of iterations to quiesce/un-quiesce               
 *
 * @return None.
 *
 * @author
 *  03/24/2010  Ron Proulx  - Created from oscar_test_quiesce
 *
 ****************************************************************/
static void oscar_quiesce_mirror_raid_groups(fbe_u32_t total_iterations)
{
    fbe_status_t    status;
    fbe_u32_t       iteration;

    /*! For each iteration of our test, quiesce and unquiesce with I/O running. 
     * We also validate that we go quiesced and come out of quiesced. 
     *
     *  @note The private/configuration mirror raid groups must be excluded 
     */
    for ( iteration = 1; iteration <= total_iterations; iteration++)
    {
        fbe_u32_t sleep_time = EMCPAL_MINIMUM_TIMEOUT_MSECS + (fbe_random() % OSCAR_QUIESCE_TEST_MAX_SLEEP_TIME);

        mut_printf(MUT_LOG_TEST_STATUS, "=== starting %s iteration %d sleep_time: %d msec===", 
                   __FUNCTION__, iteration, sleep_time);
        fbe_api_sleep(sleep_time);

        status = fbe_test_sep_util_quiesce_all_raid_groups(FBE_CLASS_ID_MIRROR);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* If there is a heavy load on the system it can take a while for the monitor to 
         * run to process the quiesce condition.  Thus we allow 60 seconds here for 
         * getting all the quiesce flags set. 
         */
        status = fbe_test_sep_utils_wait_for_all_base_config_flags(FBE_CLASS_ID_MIRROR, OSCAR_QUIESCE_WAIT_SECS, 
                                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, FBE_FALSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_test_sep_util_unquiesce_all_raid_groups(FBE_CLASS_ID_MIRROR);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Wait for quiesce flag to be cleared (FBE_TRUE) 
         */
        status = fbe_test_sep_utils_wait_for_all_base_config_flags(FBE_CLASS_ID_MIRROR, OSCAR_QUIESCE_WAIT_SECS, 
                                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/******************************************
 * end oscar_quiesce_mirror_raid_groups()
 ******************************************/

/*!**************************************************************
 * oscar_test_quiesce()
 ****************************************************************
 *
 * @brief   This test runs a quiesce test with I/O.
 *
 * @param   iterations - Number of times to quiesce/unquiesce with
 *                       io               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from oscar_test_quiesce
 *
 ****************************************************************/
static void oscar_test_quiesce(fbe_u32_t iterations)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
    fbe_status_t status;

    /* Write the background pattern to seed this element size.
     */
    oscar_write_background_pattern();


    /* Setup the abort mode to false
     */
    status = fbe_test_sep_io_configure_abort_mode(FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);  

    /* Make sure there were no errors.
     */

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
         fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       OSCAR_QUIESCE_TEST_THREADS_PER_LUN, /* threads per lun */
                                       OSCAR_MAX_IO_SIZE_BLOCKS,
                                       FBE_FALSE, /* Don't inject aborts */
                                       FBE_TRUE  /* Peer I/O */);       
    }
    else 
    {
        fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                       FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_LBA_INVALID,    /* use capacity */
                                       0,    /* run forever*/ 
                                       OSCAR_QUIESCE_TEST_THREADS_PER_LUN, /* threads per lun */
                                       OSCAR_MAX_IO_SIZE_BLOCKS,
                                       FBE_FALSE, /* Don't inject aborts */
                                       FBE_FALSE  /* Peer I/O not supported */);
    }

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Now quiesce and then un-quiesce all mirror raid groups.
     */
    oscar_quiesce_mirror_raid_groups(iterations);

    /* Halt I/O and make sure there were no errors.
     */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end oscar_test_quiesce()
 ******************************************/

/*!**************************************************************
 * oscar_read_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from oscar_read_background_pattern
 *
 ****************************************************************/

void oscar_read_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
                                            OSCAR_TEST_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end oscar_read_background_pattern()
 ******************************************/

/*!**************************************************************
 *          oscar_replace_drive()
 ****************************************************************
 *
 * @brief   Configure hot-spares and ensure they are swapped-in for removed drives
 *
 * @param   rg_index - Index into rg array to insert
 * @param   raid_group_count - number of test raid groups               
 *
 * @return None.
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created
 *
 ****************************************************************/

static void oscar_replace_drive(fbe_test_rg_configuration_t *const rg_config_p,
                                fbe_u32_t raid_group_count)
{
    mut_printf(MUT_LOG_TEST_STATUS, "%s: replace removed drives with hot-spares...",  __FUNCTION__);

    /* Configure extra drives in all raid groups as hot-spares */
    fbe_test_sep_drive_configure_extra_drives_as_hot_spares(rg_config_p, raid_group_count);

    /* Ensure that a hot-spare(s) has swapped in for each RG */
    fbe_test_sep_drive_wait_extra_drives_swap_in(rg_config_p, raid_group_count);

    return;
}
/******************************************
 * end oscar_replace_drive()
 ******************************************/

/*!**************************************************************
 *          oscar_pull_drive()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group, with data preserved
 *
 * @param   rg_index - Index into rg array to remove from
 * @param position_to_remove - The position in each rg to remove.               
 *
 * @return None.
 *
 * @author
 *  03/19/2010  Ron Proulx  - Created
 *
 ****************************************************************/
void oscar_pull_drive(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_u32_t position_to_remove)
{
    fbe_u32_t number_physical_objects;

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: raid group id: %d position: %d", 
               __FUNCTION__, rg_config_p->raid_group_id, position_to_remove);

    fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, position_to_remove, 
                                              (number_physical_objects - 1),
                                              &rg_config_p->drive_handle[position_to_remove]);

    return;
}
/******************************************
 * end oscar_pull_drive()
 ******************************************/

/*!***************************************************************************
 *          oscar_reinsert_drive()
 *****************************************************************************
 *
 * @brief   reinsert (i.e. with data preserved)drives, one per raid group.
 *
 * @param   rg_index - Index into rg array to insert
 * @param   position_to_insert - The raid grouparray index to insert.               
 *
 * @return None.
 *
 * @author
 *  03/19/2010  Ron Proulx  - Created
 *
 *****************************************************************************/

void oscar_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p,
                          fbe_u32_t position_to_insert)
{
    fbe_u32_t number_physical_objects;

    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: raid group id: %d position: %d", 
               __FUNCTION__, rg_config_p->raid_group_id, position_to_insert);

    fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, position_to_insert, 
                                                (number_physical_objects + 1),
                                                &rg_config_p->drive_handle[position_to_insert]);
    return;
}
/******************************************
 * end oscar_reinsert_drive()
 ******************************************/

/*!***************************************************************************
 *          oscar_is_shutdown_allowed_for_raid_group()
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
static fbe_bool_t oscar_is_shutdown_allowed_for_raid_group(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_bool_t  b_allow_shutdown = FBE_TRUE;

    /* Currently of if 3-way mirror and 3-way mirror shutdowns are disabled
     */
    if ((rg_config_p->width > 2)                  &&
        (oscar_disable_3way_shutdown == FBE_TRUE)    )
    {
        b_allow_shutdown = FBE_FALSE;
    }

    return(b_allow_shutdown);
}
/************************************************
 * end oscar_is_shutdown_allowed_for_raid_group()
 ************************************************/

/*!***************************************************************************
 *          oscar_get_position_to_remove()
 *****************************************************************************
 *
 * @brief   Using the configured algoirithm (either sequential or random)
 *          determine the position that should be removed.
 *
 * @param   rg_config_p - Pointer to a single raid group test configuration
 *
 * @return  position_to_remove - Valid position that should be removed
 *
 * @note    oscar_get_position_to_remove and oscar_get_position_to_insert MUST
 *          use the same algorithm.
 *
 * @author
 *  01/13/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_u32_t oscar_get_position_to_remove(fbe_test_rg_configuration_t *rg_config_p)
{
    /* Remove a random position.
     */
    return fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_RANDOM);
}
/************************************************
 * end of oscar_get_position_to_remove()
 ************************************************/

/*!***************************************************************************
 *          oscar_get_position_to_insert()
 *****************************************************************************
 *
 * @brief   Using the configured algoirithm (either sequential or random)
 *          determine the position that should be inserted.
 *
 * @param   rg_config_p - Pointer to a single raid group test configuration
 *
 * @return  position_to_insert - Valid position that should be inserted
 *
 * @note    oscar_get_position_to_remove and oscar_get_position_to_insert MUST
 *          use the same algorithm.
 * @author
 *  01/13/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_u32_t oscar_get_position_to_insert(fbe_test_rg_configuration_t *rg_config_p)
{
    /*! @todo Use fbe_test_sep_util_get_random_position_to_insert()
     */
    return fbe_test_sep_util_get_next_position_to_insert(rg_config_p);
}
/************************************************
 * end of oscar_get_position_to_insert()
 ************************************************/

/*!***************************************************************************
 *          oscar_get_position_to_spare()
 *****************************************************************************
 *
 * @brief   Using the configured algoirithm (either sequential or random)
 *          determine the position that needs to be spared.
 *
 * @param   rg_config_p - Pointer to a single raid group test configuration
 * @param   index - position index to retrieve spare position from
 *
 * @return  position_to_spare - Valid position that should be inserted
 *
 * @author
 *  01/13/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_u32_t oscar_get_position_to_spare(fbe_test_rg_configuration_t *rg_config_p,
                                      fbe_u32_t index)
{
    return fbe_test_sep_util_get_needing_spare_position_from_index(rg_config_p, index);
}
/************************************************
 * end of oscar_get_position_to_spare()
 ************************************************/

/*!**************************************************************
 *          oscar_get_raid_group_state()
 ****************************************************************
 *
 * @brief   Get the current `state' of the raid group:
 *              o Optimal no removed or degraded positions
 *              o Degraded one or more removed or degraded positions
 *              o Shutdown (i.e. Failed)
 *
 * @param   rg_config_p - Pointer to a single raid group to get state for
 * @param   current_state_p - Address of state for this raid group to update
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  05/06/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t oscar_get_raid_group_state(fbe_test_rg_configuration_t *const rg_config_p,
                                               oscar_set_raid_group_state_t *const current_state_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_object_id_t         rg_object_id;
    fbe_lifecycle_state_t   lifecycle_state;

    /* Initialize return value.
     */
    *current_state_p = OSCAR_SET_RAID_GROUP_STATE_OPTIMAL;

    /* Get the raid group object id
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First get the lifecycle state.
     */
    status = fbe_api_get_object_lifecycle_state(rg_object_id, &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY) {
        /* Raid group state is shutdown.
         */
        *current_state_p = OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN;
        return FBE_STATUS_OK;
    }

    /* Check if the raid group is degraded.
     */
    if (fbe_test_rg_is_degraded(rg_config_p)) {
        /* Raid group is degraded.
         */
        *current_state_p = OSCAR_SET_RAID_GROUP_STATE_DEGRADED;
        return FBE_STATUS_OK;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/********************************** 
 * end oscar_get_raid_group_state()
 **********************************/

/*!**************************************************************
 *          oscar_set_raid_group_state()
 ****************************************************************
 *
 * @brief   For all the configured raid groups into the state
 *          requested.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   b_use_original_drives - FBE_TRUE - Pull or re-insert the original 
 *                                             drives
 *                                  FBE_FALSE - Remove or replace drives       
 * @param   requested_state - State to put all configured raid
 *                            groups into
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t  oscar_set_raid_group_state(fbe_test_rg_configuration_t *const rg_config_p,
                                                fbe_u32_t raid_group_count,
                                                fbe_bool_t b_use_original_drives,
                                                oscar_set_raid_group_state_t requested_state)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       index;
    fbe_u32_t                       drives_to_remove = 0;
    fbe_u32_t                       drives_to_insert = 0;
    fbe_u32_t                       width = 0;
    fbe_test_rg_configuration_t    *current_rg_config_p = NULL;
    oscar_set_raid_group_state_t    current_state = OSCAR_SET_RAID_GROUP_STATE_OPTIMAL;

    /* Now that we might have configured spares, make sure to refresh the 
     * disk position info for our raid groups. 
     * We do this prior to removing drives so that we can make sure we 
     * pull out the correct drives. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

    /* If inserting new drives for the removed drives, control will continue a bit differently.  Replacing
     * a failed drive with a new drive is accomplished in both simulation and in hardware by configuring
     * extra drives in each RG as a hot-spare so they can swap-in permanently for the removed drives.
     * The removed drives then become extra drives in the RG for subsequent tests.  Note that "extra drives"
     * are drives maintained in a list in the RG test configuration and are considered valid drives for
     * future use in that RG as a permanent hot-spare.
     */
    if (!b_use_original_drives &&
        ((requested_state == OSCAR_SET_RAID_GROUP_STATE_OPTIMAL) ||
         (requested_state == OSCAR_SET_RAID_GROUP_STATE_RESTORE)))
    {
        /* Replace drives for each configured RG */
        oscar_replace_drive(rg_config_p, raid_group_count);

        /* Now that the drive(s) is replaced with another drive, refresh the disk list for each configured RG */
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

        return status;
    }

    /* For every raid group remove one drive.
     */
    for (index = 0, current_rg_config_p = rg_config_p; 
         index < raid_group_count; 
         index++, current_rg_config_p++)
    {
        /* Skip this RG if it is not configured for this test */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            continue;
        }

        /* If the raid group is already in th request state just continue.
         */
        status = oscar_get_raid_group_state(current_rg_config_p, &current_state);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (current_state == requested_state) {
            continue;
        }

        /* Determine how many drives to remove or insert.
         */
        width = current_rg_config_p->width;
        switch(requested_state)
        {
            case OSCAR_SET_RAID_GROUP_STATE_OPTIMAL:
                /* We assume that we are degraded so re-insert the
                 * required number of drives.
                 */
                switch(width)
                {
                    case 1:
                        drives_to_insert = 0;
                        drives_to_remove = 0;
                        break;
                    
                    case 2:
                        drives_to_insert = 1;
                        drives_to_remove = 0;
                        break;

                    case 3:
                        drives_to_insert = 2;
                        drives_to_remove = 0;
                        break;

                    default:
                       MUT_ASSERT_INT_EQUAL(2, width);
                       break;
                }
                break;

            case OSCAR_SET_RAID_GROUP_STATE_DEGRADED:
                /* Remove the width dependant number of drives until
                 * the raid group is degraded.
                 */
                switch(width)
                {
                    case 1:
                        drives_to_insert = 0;
                        drives_to_remove = 0;
                        break;
                    
                    case 2:
                        drives_to_insert = 0;
                        drives_to_remove = 1;
                        break;

                    case 3:
                        drives_to_insert = 0;
                        drives_to_remove = 2;
                        break;

                    default:
                       MUT_ASSERT_INT_EQUAL(2, width);
                       break;
                }
                break;

            case OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN:
                /* Remove sufficient drives to force the raid group to be 
                 * shutdown.  We only allow shutdown if enabled for this
                 * raid group type.
                 */
                if (oscar_is_shutdown_allowed_for_raid_group(current_rg_config_p))
                {
                    drives_to_insert = 0;
                    drives_to_remove = width;
                }
                else
                {
                    drives_to_insert = 0;
                    drives_to_remove = 0;
                }
                break;
            
            case OSCAR_SET_RAID_GROUP_STATE_RESTORE:
                /* We assume that all drives have been removed, thus re-insert
                 * the `width' number of drives.  We only need to do this if
                 * shutdown was allowed.
                 */
                if (oscar_is_shutdown_allowed_for_raid_group(current_rg_config_p))
                {
                    drives_to_insert = width;
                    drives_to_remove = 0;
                }
                else
                {
                    drives_to_insert = 0;
                    drives_to_remove = 0;
                }
                break;
            
            default:
                MUT_ASSERT_INT_EQUAL(OSCAR_SET_RAID_GROUP_STATE_RESTORE, requested_state);
                break;
        
        } /* end switch on requested state */

        /* Now remove the necessary drives.
         */
        while (drives_to_remove > 0)
        {
            fbe_u32_t position_to_remove = width;

            position_to_remove = oscar_get_position_to_remove(current_rg_config_p);
            MUT_ASSERT_TRUE(position_to_remove < width);
            oscar_pull_drive(current_rg_config_p, position_to_remove); 
            
            fbe_test_sep_util_add_removed_position(current_rg_config_p, position_to_remove);
            drives_to_remove--;

            /* We always mark this position as `needing spare' so that we know 
             * which drives were removed after a replacement is inserted.
             */
            fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_remove);

            /* Delay a little between each removal.
             */
            if (drives_to_remove > 0)
            {
                fbe_api_sleep(fbe_test_sep_util_get_drive_remove_delay(FBE_U32_MAX));
            }

        }
        
        /* Now insert the necessary drives.
         */

        while (drives_to_insert > 0)
        {
            fbe_u32_t position_to_insert = width;

            /* Get the disk position to insert in the RG */
            position_to_insert = oscar_get_position_to_insert(current_rg_config_p);
            MUT_ASSERT_TRUE(position_to_insert < width);

            /* Reinsert the pulled drive at the given position */
            oscar_reinsert_drive(current_rg_config_p, position_to_insert);
            fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert);

            drives_to_insert--;
            
            /* Delay a little between each insertion.
             */
            if (drives_to_insert > 0)
            {
                fbe_api_sleep(fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX));
            }
        }        
    } /* end for all configured raid groups */

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end oscar_set_raid_group_state()
 ******************************************/

/*!***************************************************************************
 *          oscar_wait_for_raid_group_to_fail()
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
static void oscar_wait_for_raid_group_to_fail(fbe_test_rg_configuration_t *const rg_config_p)
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
    if (oscar_is_shutdown_allowed_for_raid_group(rg_config_p))
    {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/******************************************
 * end oscar_wait_for_raid_group_to_fail()
 ******************************************/

/*!**************************************************************
 * oscar_start_io()
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
 *  02/22/2010  Ron Proulx  - Created from oscar_start_io
 *
 ****************************************************************/

static void oscar_start_io(fbe_test_random_abort_msec_t ran_abort_msecs,
                           fbe_rdgen_operation_t rdgen_operation,
                           fbe_u32_t threads,
                           fbe_u32_t io_size_blocks)
{

    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
 * end oscar_start_io()
 ******************************************/


/*!***************************************************************************
 *          oscar_check_if_all_threads_started()
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
static fbe_bool_t oscar_check_if_all_threads_started(fbe_api_rdgen_get_object_info_t  *object_info_p,                          
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
 * end oscar_check_if_all_threads_started()
 ******************************************/

/*!***************************************************************************
 *          oscar_wait_for_rdgen_start()
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
static void oscar_wait_for_rdgen_start(fbe_u32_t wait_seconds)
{
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter;
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

            if (oscar_check_if_all_threads_started(object_info_p,thread_info_p))
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
 * end oscar_wait_for_rdgen_start()
 ******************************************/

/*!***************************************************************************
 *          oscar_start_fixed_io()
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
static void oscar_start_fixed_io(fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t *context_p = &oscar_test_contexts[0];
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
                                             OSCAR_FIXED_PATTERN,
                                             0,    /* Run forever */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             fbe_test_sep_io_get_threads(oscar_threads),    /* threads */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* Number of stripes to write. */
                                             OSCAR_MAX_IO_SIZE_BLOCKS    /* Max blocks */ );
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
 * end oscar_start_fixed_io()
 ******************************************/

/*!***************************************************************************
 *          oscar_wait_for_rebuild_start()
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
static void oscar_wait_for_rebuild_start(fbe_test_rg_configuration_t * const rg_config_p,
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

    /* Walk all the `needing spare' positions and remove them from the 
     * `needing spare' array.
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

            /* Remove the position from the `needing spare' array
             */
            fbe_test_sep_util_delete_needing_spare_position(current_rg_config_p, position_to_rebuild);

        } /* end for all positions that need to be rebuilt */

        current_rg_config_p++;

    } /* end for all raid groups */

    return;
}
/******************************************
 * end oscar_wait_for_rebuild_start()
 ******************************************/

/*!***************************************************************************
 *          oscar_wait_for_rebuild_completion()
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
 * @param   previous_raid_group_state - Determines which position to wait for  
 *
 * @return  None.
 *
 * @author
 * 03/12/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void oscar_wait_for_rebuild_completion(fbe_test_rg_configuration_t * const rg_config_p,
                                              fbe_bool_t b_wait_for_all_degraded_positions_to_rebuild,
                                              oscar_set_raid_group_state_t previous_raid_group_state)
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

    /* Validate that the previous state is correct
     */
    switch(previous_raid_group_state)
    {
        case OSCAR_SET_RAID_GROUP_STATE_DEGRADED:
        case OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN:
            break;

        default:
            /* Other states aren't supported
             */
            MUT_ASSERT_INT_NOT_EQUAL(OSCAR_SET_RAID_GROUP_STATE_DEGRADED, previous_raid_group_state);
            return;
    }

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
        if ((b_wait_for_all_degraded_positions_to_rebuild == FBE_TRUE) ||
            (index == 0)                                                  )  
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
/******************************************
 * end oscar_wait_for_rebuild_completion()
 ******************************************/

/*!**************************************************************
 *          oscar_set_debug_flags()
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
void oscar_set_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                           fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;
    
    /* If debug is not enabled, simply return
     */
    if (oscar_test_debug_enable == FBE_FALSE)
    {
        return;
    }
    
    /* Populate the raid group debug flags to the value desired.
     * (there can only be 1 set of debug flag override)
     */
    raid_group_debug_flags = oscar_object_debug_flags;
        
    /* Populate the raid library debug flags to the value desired.
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(oscar_library_debug_flags);
    
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
     if (oscar_enable_state_trace == FBE_TRUE)
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
 * end oscar_set_debug_flags()
 ******************************************/

/*!**************************************************************
 *          oscar_clear_debug_flags()
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
void oscar_clear_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                             fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;

    /* If debug is not enabled, simply return
     */
    if (oscar_test_debug_enable == FBE_FALSE)
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
    if (oscar_enable_state_trace == FBE_TRUE)
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
 * end oscar_clear_debug_flags()
 ******************************************/

/*!***************************************************************************
 *          oscar_degraded_shutdown_step1()
 *****************************************************************************
 *
 * @brief   This test populates all mirror raid groups (LUNs) with a fixed
 *          pattern.  Then with fixed I/O running it `pulls' all the drive to
 *          for the raid group to be shutdown.  It then re-inserts all the drive
 *          which will force a differential rebuild.  It then validates that
 *          all the data (even thought it is stale) can be read without error.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void oscar_degraded_shutdown_step1(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    oscar_write_fixed_pattern();

    /* Now start fixed I/O
     */
    oscar_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(oscar_io_msec_short);

    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    oscar_wait_for_rdgen_start(OSCAR_RDGEN_WAIT_SECS);

    /* Remove sufficient drives to shutdown the raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s pulling all drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s pulling all drives successful. ==", __FUNCTION__);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        oscar_wait_for_raid_group_to_fail(current_rg_config_p);
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
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_RESTORE); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
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
    oscar_read_fixed_pattern();

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);
    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    oscar_clear_debug_flags();
    */

    return;
}
/******************************************
 * end oscar_degraded_shutdown_step1()
 ******************************************/

/*!***************************************************************************
 *          oscar_degraded_shutdown_step2()
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
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void oscar_degraded_shutdown_step2(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;


    /*
     * !@TODO:  this test needs to be reworked; it is not being used by Oscar now
     */

    /* Step 2:  Validate that drive pulls during rebuild don't cause issues
     */
    
    /* Now start I/O
     */
    oscar_start_io(ran_abort_msecs, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,oscar_threads,OSCAR_MAX_IO_SIZE_BLOCKS);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(oscar_io_msec_short);

    /* We need to wait for at least (1) position to be rebuilt.  The position
     * we wait must not be the position that will get pulled.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for partial rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN,
                                          FBE_FALSE /* Only wait for (1) position */);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for partial rebuilds to finish. (successful)==", __FUNCTION__);

    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Remove sufficient drives to degrade raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_FALSE,
                                        OSCAR_SET_RAID_GROUP_STATE_DEGRADED); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives  successful. ==", __FUNCTION__);
    
    /* Let I/O run for a few seconds */
    fbe_api_sleep(oscar_io_msec_long);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O successful ==", __FUNCTION__);

    /* Validate no errors.we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }
    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_FALSE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives successful. ==", __FUNCTION__);

    /* Wait for rebuild to start on at least (1) raid group (if we wait for all we
     * might miss some).
     */
    current_rg_config_p = &rg_config_p[(raid_group_count - 1)];
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for raid group object id: %d to start rebuild ==", 
               __FUNCTION__, rg_object_id);
    oscar_wait_for_rebuild_start(rg_config_p, raid_group_count, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for raid group object id: %d to start rebuild (successful)==", 
               __FUNCTION__,  rg_object_id);
    
    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==\n", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==\n", __FUNCTION__);

    
    /* Remove sufficient drives to shutdown the raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Pulling all drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Pulling all drives successful. ==", __FUNCTION__);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++ )
    {
        oscar_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }

    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting all  drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_RESTORE); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
    
    /* Clear debug flags.
     */
    oscar_clear_debug_flags(rg_config_p, raid_group_count);

    return;
}
/******************************************
 * end oscar_degraded_shutdown_step2()
 ******************************************/

/*!***************************************************************************
 *          oscar_degraded_shutdown_step3()
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
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void oscar_degraded_shutdown_step3(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    
    /* Step 3:  Validate no media errors with normal I/O with shutdown
     */

    /* Now start normal I/O
     */
    oscar_start_io(ran_abort_msecs, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,oscar_threads,OSCAR_MAX_IO_SIZE_BLOCKS);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(oscar_io_msec_short);
    
    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    oscar_wait_for_rdgen_start(OSCAR_RDGEN_WAIT_SECS);
    
    /* Remove sufficient drives to shutdown the raid group
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Pulling all drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_SHUTDOWN); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Pulling all drives successful. ==", __FUNCTION__);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++ )
    {
        oscar_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }
    
    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors.we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }
    
    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting all  drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_RESTORE); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Now quiesce and then un-quiesce all mirror raid groups.
     */
    /*! @todo This isn't working yet
     oscar_quiesce_mirror_raid_groups();
     */

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);
    
    /* Clear debug flags.
     */
    oscar_clear_debug_flags(rg_config_p, raid_group_count);

    return;
}
/******************************************
 * end oscar_degraded_shutdown_step3()
 ******************************************/

/*!***************************************************************************
 *          oscar_test_shutdown_with_io()
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
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 * @param   rdgen_operation - The rdgen operation (w/r/c, z/r/c, etc)
 *
 * @return  None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 *****************************************************************************/

static void oscar_test_shutdown_with_io(fbe_test_rg_configuration_t * const rg_config_p,
                                        fbe_u32_t raid_group_count,
                                        fbe_test_random_abort_msec_t ran_abort_msecs,
                                        fbe_rdgen_operation_t rdgen_operation)
{

    /*! @note Currently the shutdown test always used a fixed pattern.
     */
    FBE_UNREFERENCED_PARAMETER(rdgen_operation);

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */
    oscar_degraded_shutdown_step1(rg_config_p, raid_group_count,
                                  ran_abort_msecs);

    /* Step 2:  Validate that drive pulls during rebuild don't cause issues
     */
    /* !@TODO: this test needs to be re-worked */
    /*oscar_degraded_shutdown_step2(rg_config_p, raid_group_count,
                                    ran_abort_msecs);*/

    /* Step 3:  Validate no media errors with normal I/O with shutdown
     */
    oscar_degraded_shutdown_step3(rg_config_p, raid_group_count,
                                  ran_abort_msecs);
   
    return;
}
/******************************************
 * end oscar_test_shutdown_with_io()
 ******************************************/

/*!***************************************************************************
 *          oscar_test_degraded_io_write_background_pattern()
 *****************************************************************************
 *
 * @brief   Run I/O then force all the raid groups to degraded.  Validate that
 *          no mis-compares or errors occur.  Then it writes the specified
 *          background pattern.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 * @param   rdgen_operation - The rdgen operation (w/r/c, z/r/c, etc)
 * @param   background_operation - The background operation (w/r/c, z/r/c, etc)
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 *****************************************************************************/
static void oscar_test_degraded_io_write_background_pattern(fbe_test_rg_configuration_t * const rg_config_p,
                                                            fbe_u32_t raid_group_count,
                                                            fbe_bool_t b_use_original_drives,
                                                            fbe_test_random_abort_msec_t ran_abort_msecs,
                                                            fbe_rdgen_operation_t rdgen_operation,
                                                            fbe_rdgen_operation_t background_operation)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;

    /* Start I/O
     */
    oscar_start_io(ran_abort_msecs, rdgen_operation, oscar_threads, OSCAR_MAX_IO_SIZE_BLOCKS);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(oscar_io_msec_short);
    
    /* Remove drives with I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, b_use_original_drives,
                                        OSCAR_SET_RAID_GROUP_STATE_DEGRADED); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(oscar_io_msec_short);   

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O: Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect abort failures if we have injected them
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now write all the luns with the requested pattern while the raid group
     * is still degraded.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        oscar_write_zero_background_pattern();
    } else {
        oscar_write_background_pattern();
    }
    return;
}
/**********************************************************
 * end of oscar_test_degraded_io_write_background_pattern()
 **********************************************************/

/*!***************************************************************************
 *          oscar_test_rebuild_and_check_background_pattern()
 *****************************************************************************
 *
 * @brief   Write a background pattern to the raid group, pull drives,
 *          make sure we can read the pattern back degraded.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 * @param   background_operation - The rdgen operation (w/r/c, z/r/c, etc)
 *
 * @return None.
 *
 * @author
 *  8/5/2011 - Created. Rob Foley
 *
 *****************************************************************************/
static void oscar_test_rebuild_and_check_background_pattern(fbe_test_rg_configuration_t * const rg_config_p,
                                             fbe_u32_t raid_group_count,
                                             fbe_bool_t b_use_original_drives,
                                             fbe_test_random_abort_msec_t ran_abort_msecs,
                                             fbe_rdgen_operation_t background_operation)
{
    fbe_status_t                status;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t index;

    /* Remove drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, b_use_original_drives,
                                        OSCAR_SET_RAID_GROUP_STATE_DEGRADED); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);
    
    /* Make sure the background pattern can be read still.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how OSCAR_FIXED_PATTERN currently defined)here. 
         * If OSCAR_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        oscar_read_fixed_pattern();
    }
    else
    {
        oscar_read_background_pattern();
    }

    /* Replace the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives(1) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_FALSE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives(1): Successful. ==", __FUNCTION__);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(2) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(2): Successful ==", __FUNCTION__);
    
    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(3) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(3): Successful ==", __FUNCTION__);

    /* Make sure the background pattern can be read still.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how OSCAR_FIXED_PATTERN currently defined)here. 
         * If OSCAR_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        oscar_read_fixed_pattern();
    }
    else
    {
        oscar_read_background_pattern();
    }
    return;
}
/******************************************
 * end of oscar_test_rebuild_and_check_background_pattern()
 ******************************************/

/*!***************************************************************************
 *          oscar_test_full_rebuild_with_io()
 *****************************************************************************
 *
 * @brief   Run rebuild test for all the raid groups passed in the
 *          configuration (specified by raid_group_count)
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 * @param   rdgen_operation - The rdgen operation (w/r/c, z/r/c, etc)
 * @param   background_operation - The background operation (w/r/c, z/r/c, etc)
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 *****************************************************************************/
static void oscar_test_full_rebuild_with_io(fbe_test_rg_configuration_t * const rg_config_p,
                                            fbe_u32_t raid_group_count,
                                            fbe_test_random_abort_msec_t ran_abort_msecs,
                                            fbe_rdgen_operation_t rdgen_operation,
                                            fbe_rdgen_operation_t background_operation)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;

    /* Replace the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives(1) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_FALSE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives(1): Successful. ==", __FUNCTION__);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(2) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(2): Successful ==", __FUNCTION__);
    
    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(3) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(3): Successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    oscar_clear_debug_flags(rg_config_p, raid_group_count);

    /* Make sure the background pattern can be read still.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        /* Careful!! We intend to read FBE_RDGEN_PATTERN_ZEROS (how OSCAR_FIXED_PATTERN currently defined)here. 
         * If OSCAR_FIXED_PATTERN ever changes to other pattern, we need to change code here. */
        oscar_read_fixed_pattern();
    }
    else
    {
        oscar_read_background_pattern();
    }
                                    
    /* Run I/O normal mode again to make sure there are no errors in normal mode I/O (like
     * shed stamp errors. 
     */
    oscar_start_io(ran_abort_msecs, rdgen_operation, oscar_threads, OSCAR_MAX_IO_SIZE_BLOCKS);
 
    /* Run I/O briefly.
     */
    fbe_api_sleep(oscar_io_msec_short);

    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Now degraded the raid groups and run a light I/O during the
     * rebuild.
     */
    oscar_test_degraded_io_write_background_pattern(rg_config_p, raid_group_count,
                                                    FBE_FALSE, ran_abort_msecs, 
                                                    rdgen_operation,
                                                    background_operation);

    /* Start I/O
     */
    oscar_start_io(ran_abort_msecs, rdgen_operation, oscar_light_threads, OSCAR_SMALL_IO_SIZE_BLOCKS);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(oscar_io_msec_short);

    /* Replace the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives(4) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_FALSE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives(4): Successful. ==", __FUNCTION__);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(5) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(5): Successful ==", __FUNCTION__);
    
    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(6) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(6): Successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    oscar_clear_debug_flags(rg_config_p, raid_group_count);

    /* Stop I/O.
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
    
    /* Now write all the luns with the requested pattern for the next test.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        oscar_write_zero_background_pattern();
    } else {
        oscar_write_background_pattern();
    }
    return;
}
/******************************************
 * end of oscar_test_full_rebuild_with_io()
 ******************************************/

/*!***************************************************************************
 *          oscar_test_differential_rebuild_with_io()
 *****************************************************************************
 *
 * @brief   Run differential rebuild test for all the raid groups passed in the
 *          configuration (specified by raid_group_count)
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 * @param   rdgen_operation - The rdgen operation (w/r/c, z/r/c, etc)
 * @param   background_operation - The background operation (w/r/c, z/r/c, etc)
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 *****************************************************************************/
static void oscar_test_differential_rebuild_with_io(fbe_test_rg_configuration_t * const rg_config_p,
                                                    fbe_u32_t raid_group_count,
                                                    fbe_test_random_abort_msec_t ran_abort_msecs,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_rdgen_operation_t background_operation)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;

    /* Now degraded the raid groups and run a normal I/O pattern during the
     * rebuild.
     */
    oscar_test_degraded_io_write_background_pattern(rg_config_p, raid_group_count,
                                                    FBE_TRUE, ran_abort_msecs,
                                                    rdgen_operation,
                                                    background_operation);

    /* Start I/O
     */
    oscar_start_io(ran_abort_msecs, FBE_RDGEN_OPERATION_WRITE_READ_CHECK,oscar_light_threads,OSCAR_SMALL_IO_SIZE_BLOCKS);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(oscar_io_msec_short);

    /* Re-insert the previously removed drives (i.e. force a differential rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives(1) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives(1): Successful. ==", __FUNCTION__);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(2) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(2): Successful ==", __FUNCTION__);
    
    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(2) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(2): Successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    oscar_clear_debug_flags(rg_config_p, raid_group_count);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(3) ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(3): Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now write all the luns with the requested pattern for the next test.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        oscar_write_zero_background_pattern();
    } else {
        oscar_write_background_pattern();
    }
    return;
}
/**********************************************
 * end of oscar_test_differential_rebuild_with_io()
 **********************************************/

/*!***************************************************************************
 *          oscar_test_differential_degraded_rebuild_with_io()
 *****************************************************************************
 *
 * @brief   Run differential rebuild test for all the raid groups passed in the
 *          configuration (specified by raid_group_count)
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   ran_abort_msecs - If not 0xffff, abort an I/O every time this msec period
 * @param   rdgen_operation - The rdgen operation (w/r/c, z/r/c, etc)
 * @param   background_operation - The background operation (w/r/c, z/r/c, etc)
 *
 * @return None.
 *
 * @author
 * 01/27/2011   Ron Proulx - Created from oscar_test.c
 *
 *****************************************************************************/
static void oscar_test_differential_degraded_rebuild_with_io(fbe_test_rg_configuration_t * const rg_config_p,
                                                             fbe_u32_t raid_group_count,
                                                             fbe_test_random_abort_msec_t ran_abort_msecs,
                                                             fbe_rdgen_operation_t rdgen_operation,
                                                             fbe_rdgen_operation_t background_operation)
{
    fbe_api_rdgen_context_t    *context_p = &oscar_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;

    /* Now degraded the raid groups and run a normal I/O pattern during the
     * rebuild.
     */
    oscar_test_degraded_io_write_background_pattern(rg_config_p, raid_group_count,
                                                    FBE_TRUE, ran_abort_msecs,
                                                    rdgen_operation,
                                                    background_operation);

    /* Start I/O
     */
    oscar_start_io(ran_abort_msecs, rdgen_operation, oscar_threads,OSCAR_MAX_IO_SIZE_BLOCKS);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(oscar_io_msec_short);

    /* Re-insert the previously removed drives (i.e. force a differential rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives(1) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives(1): Successful. ==", __FUNCTION__);

    /* Wait for rebuild to start on at least (1) raid group (if we wait for all we
     * might miss some).
     */
    current_rg_config_p = &rg_config_p[(raid_group_count - 1)];
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for raid group object id: %d to start rebuild(2) ==", 
               __FUNCTION__, rg_object_id);
    oscar_wait_for_rebuild_start(rg_config_p, raid_group_count, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for raid group object id: %d to start rebuild(2) (successful)==", 
               __FUNCTION__,  rg_object_id);

    /* Remove drives with I/O and rebuilding
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives(3) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_DEGRADED); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives(3) Successful. ==", __FUNCTION__);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(4) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready(4): Successful ==", __FUNCTION__);
    
    /* Set debug flags.
     */
    oscar_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for degraded rebuilds to finish(5) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for degraded rebuilds to finish(5): Successful ==", __FUNCTION__);

    /* Re-insert the previously removed drives (i.e. force a differential rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives(6) ==", __FUNCTION__);
    status = oscar_set_raid_group_state(rg_config_p, raid_group_count, FBE_TRUE,
                                        OSCAR_SET_RAID_GROUP_STATE_OPTIMAL); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Re-inserting drives(6): Successful. ==", __FUNCTION__);
    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish(7) ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for (index = 0; index < raid_group_count; index++)
    {
        oscar_wait_for_rebuild_completion(current_rg_config_p, OSCAR_SET_RAID_GROUP_STATE_DEGRADED,
                                          FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for degraded rebuilds to finish(7) Successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    oscar_clear_debug_flags(rg_config_p, raid_group_count);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(8) ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(8): Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now write all the luns with the requested pattern for the next test.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        oscar_write_zero_background_pattern();
    } else {
        oscar_write_background_pattern();
    }
    return;
}
/******************************************************
 * end of oscar_test_differential_degraded_rebuild_with_io()
 ******************************************************/

/*!**************************************************************
 *          oscar_run_degraded_io_test()
 ****************************************************************
 *
 * @brief   Run quiesce and degraded I/O tests with an lba pattern.
 *
 * @param   rg_config_p - Config to create.
 * @param   context_p - Currently not used.
 *
 * @return none
 *
 * @author
 *  08/20/2010  Ron Proulx  - Created from big_bird_run_tests_for_config
 *
 ****************************************************************/
static void oscar_run_degraded_io_test(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t                       iterations = OSCAR_QUIESCE_TEST_ITERATIONS;
    fbe_test_random_abort_msec_t    random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_rdgen_operation_t           rdgen_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK; 
    fbe_rdgen_operation_t           background_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK; 

    /* Currently not used.
     */
    FBE_UNREFERENCED_PARAMETER(context_p);

    /* If dualsp load balance I/O
     */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_TRUE, FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    } else {       
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Set the random abort time
     */
    if (test_level > 0) {
        random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ONE_TENTH_MSEC;
        mut_printf(MUT_LOG_TEST_STATUS, " Test started with random aborts msecs : %d", 
                   random_abort_msecs);
    }

    /* Test level is our default number of iterations. 
     */
    if (test_level > iterations) {
        iterations = (test_level <= OSCAR_QUIESCE_TEST_MAX_ITERATIONS) ? test_level : OSCAR_QUIESCE_TEST_MAX_ITERATIONS;

        /* Run a quiesce test.
         */
        oscar_test_quiesce(iterations);
    }

    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Initialize our `short' and `long' I/O times
     */
    oscar_set_io_seconds();

    mut_printf(MUT_LOG_TEST_STATUS, "%s Running for test level %d", __FUNCTION__, test_level);

    /*! @note Write a background pattern that is different than the I/O pattern.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        background_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
        oscar_write_background_pattern();
    } else {
        background_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK;
        oscar_write_zero_background_pattern();
    }

    /* Test 1: Run a test where we write a pattern, degrade the raid group by pulling 
     *         random drive(s), and then read back while degraded.  
     */
    oscar_test_rebuild_and_check_background_pattern(rg_config_p, raid_group_count,
                                                    FBE_FALSE, FBE_TEST_RANDOM_ABORT_TIME_INVALID, 
                                                    background_operation);

    /* Test 2: Degraded I/O test
     *
     * Invoke method that will run I/O and then set the raid groups degraded
     */
    oscar_test_degraded_io_write_background_pattern(rg_config_p, raid_group_count,
                                                    FBE_FALSE, random_abort_msecs, 
                                                    rdgen_operation,
                                                    background_operation);


    /* Test 3: Full rebuild with I/O test
     *
     * Invoke method that will run the rebuild tests for all active raid groups
     */
    oscar_test_full_rebuild_with_io(rg_config_p, raid_group_count, 
                                    FBE_TEST_RANDOM_ABORT_TIME_INVALID, 
                                    rdgen_operation,
                                    background_operation);

   
    /* Test 4: Differential rebuild with I/O test
     *
     * Invoke method that will run the rebuild tests for all active raid groups
     */
    oscar_test_differential_rebuild_with_io(rg_config_p, raid_group_count,
                                            FBE_TEST_RANDOM_ABORT_TIME_INVALID, 
                                            rdgen_operation,
                                            background_operation);

    /*! Test 5: Differential rebuild with I/O test
     *
     *  Invoke method that will run the degraded rebuild tests for all active raid groups
     * 
     *  @todo This test is currently disabled.
     */
    /* oscar_test_differential_degraded_rebuild_with_io(rg_config_p, raid_group_count, 
                                                 luns_per_rg, ran_abort_msecs);
     */
    
    return;
}
/******************************************
 * end oscar_run_degraded_io_test()
 ******************************************/

/*!**************************************************************
 *          oscar_run_zero_abort_degraded_io_test()
 ****************************************************************
 *
 * @brief   Test degraded zero I/Os with aborts.
 *
 * @param   rg_config_p - Config to create.
 * @param   context_p - Currently not used.
 *
 * @return none
 *
 * @author
 *  08/20/2010  Ron Proulx  - Created from big_bird_run_tests_for_config
 *
 ****************************************************************/
static void oscar_run_zero_abort_degraded_io_test(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t    random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_rdgen_operation_t           rdgen_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK; 
    fbe_rdgen_operation_t           background_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK; 

    /* Currently not used.
     */
    FBE_UNREFERENCED_PARAMETER(context_p);

    /* If dualsp load random load balancing.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_TRUE, FBE_TEST_SEP_IO_DUAL_SP_MODE_RANDOM);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    } else {       
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Set the random abort time
     */
    if (test_level > 0) {
        random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ONE_TENTH_MSEC;
        mut_printf(MUT_LOG_TEST_STATUS, " Test started with random aborts msecs : %d", 
                   random_abort_msecs);
    }

    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Initialize our `short' and `long' I/O times
     */
    oscar_set_io_seconds();

    mut_printf(MUT_LOG_TEST_STATUS, "%s Running for test level %d", __FUNCTION__, test_level);

    /*! @note Write a background pattern that is different than the I/O pattern.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        background_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
        oscar_write_background_pattern();
    } else {
        background_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK;
        oscar_write_zero_background_pattern();
    }

    /* Test 1: Degraded I/O test with zero operation
     *
     * Invoke method that will run zeroing and then set the raid groups degraded
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Zero tests starting. ===");
    oscar_test_degraded_io_write_background_pattern(rg_config_p, raid_group_count,
                                                    FBE_FALSE, random_abort_msecs, 
                                                    rdgen_operation,
                                                    background_operation);
    
    /* Test 2: Full rebuild with zeroing test
     *
     * Invoke method that will run the rebuild tests for all active raid groups
     */
    oscar_test_full_rebuild_with_io(rg_config_p, raid_group_count, 
                                    random_abort_msecs, 
                                    rdgen_operation,
                                    background_operation);
    mut_printf(MUT_LOG_TEST_STATUS, "=== Zero tests completed. ===");

    return;
}
/*********************************************
 * end oscar_run_zero_abort_degraded_io_test()
 *********************************************/

/*!**************************************************************
 *          oscar_run_shutdown_with_io_test()
 ****************************************************************
 *
 * @brief   This test shutdown with I/O.
 *
 * @param   rg_config_p - Config to create.
 * @param   context_p - Currently not used
 *
 * @return none
 *
 * @author
 *  08/20/2010  Ron Proulx  - Created from big_bird_run_tests_for_config
 *
 ****************************************************************/
static void oscar_run_shutdown_with_io_test(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t    random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    fbe_rdgen_operation_t           rdgen_operation =  FBE_RDGEN_OPERATION_ZERO_READ_CHECK; 
    fbe_rdgen_operation_t           background_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK; 

    /* Currently not used.
     */
    FBE_UNREFERENCED_PARAMETER(context_p);

    /* If dualsp run peer only.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode()) {
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_TRUE, FBE_TEST_SEP_IO_DUAL_SP_MODE_PEER_ONLY);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    } else {       
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Set the random abort time
     */
    if (test_level > 0) {
        random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ONE_TENTH_MSEC;
        mut_printf(MUT_LOG_TEST_STATUS, " Test started with random aborts msecs : %d", 
                   random_abort_msecs);
    }

    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Initialize out `short' and `long' I/O times
     */
    oscar_set_io_seconds();

    mut_printf(MUT_LOG_TEST_STATUS, "%s Running for test level %d", __FUNCTION__, test_level);

    /*! @note Write a background pattern that is different than the I/O pattern.
     */
    if (rdgen_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        background_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
        oscar_write_background_pattern();
    } else {
        background_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK;
        oscar_write_zero_background_pattern();
    }

    /* Test 1: Shutdown raid groups with I/O
     *
     * Invoke method that will run I/O and then force a shutdown
     */
    oscar_test_shutdown_with_io(rg_config_p, raid_group_count,
                                random_abort_msecs,
                                rdgen_operation);

    return;
}
/******************************************
 * end oscar_run_shutdown_with_io_test()
 ******************************************/

/*!**************************************************************
 *          oscar_degraded_test()
 ****************************************************************
 * @brief   Run degraded I/O test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 ****************************************************************/
void oscar_degraded_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &oscar_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &oscar_raid_group_config_qual[0];

    }

     fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, oscar_run_degraded_io_test,
                                                    OSCAR_TEST_LUNS_PER_RAID_GROUP,
                                                    OSCAR_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end oscar_degraded_test()
 ******************************************/

/*!**************************************************************
 *          oscar_degraded_zero_abort_test()  
 ****************************************************************
 * @brief   Run zero I/Os with abort tests.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 ****************************************************************/

void oscar_degraded_zero_abort_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &oscar_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &oscar_raid_group_config_qual[0];

    }

     fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, oscar_run_zero_abort_degraded_io_test,
                                   OSCAR_TEST_LUNS_PER_RAID_GROUP,
                                   OSCAR_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end oscar_degraded_zero_abort_test()
 ******************************************/

/*!**************************************************************
 *          oscar_shutdown_test()
 ****************************************************************
 * @brief   Run I/O then shutdown the RAID-1.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 ****************************************************************/

void oscar_shutdown_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                   test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &oscar_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &oscar_raid_group_config_qual[0];

    }

     fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, oscar_run_shutdown_with_io_test,
                                   OSCAR_TEST_LUNS_PER_RAID_GROUP,
                                   OSCAR_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end oscar_shutdown_test()
 ******************************************/

/*!**************************************************************
 * oscar_setup()
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
void oscar_setup(void)
{    
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &oscar_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &oscar_raid_group_config_qual[0];
        }

        /* Initialize the number of extra drives required by RG. 
         * Used when create physical configuration for RG in simulation. 
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Create the physical configuration.
         */
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    /* The following utility function does some setup for hardware. */
    fbe_test_common_util_test_setup_init();

    fbe_test_disable_background_ops_system_drives();
}
/**************************************
 * end oscar_setup()
 **************************************/

/*!**************************************************************
 * oscar_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the oscar test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from oscar_test.c
 *
 ****************************************************************/

void oscar_cleanup(void)
{
    //fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    //status = fbe_api_rdgen_stop_tests(oscar_test_contexts, OSCAR_TEST_MAX_TOTAL_LUNS);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end oscar_cleanup()
 ******************************************/

/*!**************************************************************
 * oscar_dualsp_degraded_test()
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

void oscar_dualsp_degraded_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &oscar_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &oscar_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, oscar_run_degraded_io_test,
                                                   OSCAR_TEST_LUNS_PER_RAID_GROUP,
                                                   OSCAR_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end oscar_dualsp_degraded_test()
 ******************************************/

/*!**************************************************************
 * oscar_dualsp_degraded_zero_abort_test()
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

void oscar_dualsp_degraded_zero_abort_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &oscar_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &oscar_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, oscar_run_zero_abort_degraded_io_test,
                                                   OSCAR_TEST_LUNS_PER_RAID_GROUP,
                                                   OSCAR_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end oscar_dualsp_degraded_zero_abort_test()
 ******************************************/

/*!**************************************************************
 * oscar_dualsp_shutdown_test()
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

void oscar_dualsp_shutdown_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &oscar_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &oscar_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, NULL, oscar_run_shutdown_with_io_test,
                                                   OSCAR_TEST_LUNS_PER_RAID_GROUP,
                                                   OSCAR_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end oscar_dualsp_shutdown_test()
 ******************************************/

/*!**************************************************************
 * oscar_dualsp_setup()
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
void oscar_dualsp_setup(void)
{    

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &oscar_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &oscar_raid_group_config_qual[0];
        }

        /* Initialize the number of extra drives required by RG. 
         * Used when create physical configuration for RG in simulation. 
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        
        sep_config_load_sep_and_neit_load_balance_both_sps();

    }

    /* The following utility function does some setup for hardware. */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();
}
/**************************************
 * end oscar_dualsp_setup()
 **************************************/


/*!**************************************************************
 * oscar_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup oscar dual sp test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void oscar_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

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
 * end oscar_dualsp_cleanup()
 ******************************************/

/*************************
 * end file oscar_test.c
 *************************/


