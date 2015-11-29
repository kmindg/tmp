/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file baby_bear_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains RBA tracing and performace statistics tests.
 *
 * @version
 *   02/16/2010 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
//#include "mut_assert.h"
#include "mut.h"   
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_traffic_trace.h"
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe_test_common_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * baby_bear_short_desc = "Performance statistics test.";
char * baby_bear_long_desc ="\
The baby bear scenario tests [TODO]RBA tracing.\n\
\n\
-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and capacities at -rtl 1.\n\
\n\
STEP 1: Bring up the initial topology.\n\
		- Create at least 1 raid group with 3 LUNs each.\n\
STEP 2: [TODO]Run Baby Bear RBA Trace test for each raid group.\n\
		- Enable tracing for different object types.\n\
		- [TODO]Run IO.\n\
		- Disable Tracing for different object types.\n\
		- [TODO]Check that the proper tracing has been done.\n\
STEP 3: Run Baby Bear Mirror Read Optimization Test for each RAID 1 raid group.\n\
		- Run mirror read io with multiple threads on one lun.\n\
		- Verify disk reads perfromance statistics.\n\
STEP 4: Destroy raid groups and LUNs.\n\
\n\
Description last updated: 9/27/2011.\n";

/*!*******************************************************************
 * @def BABY_BEAR_LUNS_PERF_STATS_ENABLE
 * @def BABY_BEAR_LUNS_PERF_STATS_DISABLE 
 * @def BABY_BEAR_LUNS_PERF_STATS_GET 
 *********************************************************************
 * @brief Different operation related to LUN performance statistics.
 *********************************************************************/
#define BABY_BEAR_LUNS_PERF_STATS_ENABLE    0 
#define BABY_BEAR_LUNS_PERF_STATS_DISABLE   1
#define BABY_BEAR_LUNS_PERF_STATS_GET       2

/*!*******************************************************************
 * @def BABY_BEAR_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define BABY_BEAR_LUNS_PER_RAID_GROUP       3 

/*!*******************************************************************
 * @def BABY_BEAR_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of the LUNs.
 *********************************************************************/
#define BABY_BEAR_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def BABY_BEAR_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BABY_BEAR_CHUNKS_PER_LUN    3

/*!*******************************************************************
 * @def BABY_BEAR_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define BABY_BEAR_RAID_GROUP_COUNT      3

/*!*******************************************************************
 * @def BABY_BEAR_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BABY_BEAR_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def BABY_BEAR_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define BABY_BEAR_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @def BABY_BEAR_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define BABY_BEAR_LARGE_IO_SIZE_MAX_BLOCKS BABY_BEAR_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def BABY_BEAR_RDGEN_START_LBA
 *********************************************************************
 * @brief start lba for the rdgen operations.
 *
 *********************************************************************/
#define BABY_BEAR_RDGEN_START_LBA 0x0

/*!*******************************************************************
 * @def BABY_BEAR_RDGEN_BLOCK_COUNT
 *********************************************************************
 * @brief block count for the rdgen operations.
 *
 *********************************************************************/
#define BABY_BEAR_RDGEN_BLOCK_COUNT 0x2

/*!*******************************************************************
 * @def BABY_BEAR_RDGEN_BLOCK_COUNT
 *********************************************************************
 * @brief block count for the rdgen operations.
 *
 *********************************************************************/
#define BABY_BEAR_RDGEN_LARGE_BLOCK_COUNT 0x1200

 /*!*******************************************************************
 * @def BABY_BEAR_RDGEN_BLOCK_COUNT_BIG
 *********************************************************************
 * @brief block count for the rdgen operations.
 *
 *********************************************************************/
#define BABY_BEAR_RDGEN_BLOCK_COUNT_BIG 0x100

 static fbe_api_rdgen_context_t baby_bear_test_contexts[BABY_BEAR_LUNS_PER_RAID_GROUP];

/*!*******************************************************************
 * @var baby_bear_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t baby_bear_raid_group_config_qual[] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,         0x32000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {6,         0x32000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            1,          0},
    {4,         0xE000,       FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            2,          0},
    {5,         0xE000,       FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {5,         0x32000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {4,         0xE000,       FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            5,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var baby_bear_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t baby_bear_raid_group_config_extended[] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {2,         0x32000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            0,          0},
    {3,         0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          1},
    {6,         0x32000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            2,          0},
    {3,         0x9000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            3,          0},
    {6,         0x2A000,     FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            4,          0},
    {4,         0xE000,      FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,  520,            5,          0},
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            6,          0},
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,   520,            7,          0},
    {5,         0x32000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            8,          0},
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            9,          0},
    {8,         0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            10,         0},
    {6,         0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            11,         0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************************
 * baby_bear_rba_test
 ***************************************************************************
 * @brief
 *  This function performs the RBA tests. It does the following
 *  on a raid group:
 * 
 * @param 
 *
 * @return void
 *
 ***************************************************************************/
static void baby_bear_rba_test(fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_test_rg_configuration_t                     *rg_config_p = NULL;
    fbe_u32_t                                       rg_index;
    fbe_u32_t                                       raid_group_count;
    fbe_test_logical_unit_configuration_t *         logical_unit_configuration_p = NULL;
    fbe_object_id_t                                 lun_object_id_1;
    fbe_object_id_t                                 lun_object_id_2;
    rg_config_p = in_current_rg_config_p;
    raid_group_count = fbe_test_get_rg_count(rg_config_p);

    for ( rg_index = 0; rg_index < raid_group_count; rg_index ++,
                                                     rg_config_p ++)
    {
        
        mut_printf(MUT_LOG_TEST_STATUS, "Ring buffer archive iteration %d of %d.\n", 
                    rg_index+1, raid_group_count);
        /* BABY_BEAR_LUNS_PER_RAID_GROUP should be more than 2*/
        logical_unit_configuration_p = &(in_current_rg_config_p[rg_index].logical_unit_configuration_list[0]);
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id_1);

        
        logical_unit_configuration_p = &(in_current_rg_config_p[rg_index].logical_unit_configuration_list[1]);
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id_2);

        /* RBA get initialized while initializing the package in physical_package_init_services and sep_init_services
        hence no need to initialize */
        
        /*On Array, Use RBA tool to enable traffic buffer, use following command
        rba -o c:\Rbalogs\log1.ktr -r traffic -t lun -t fru -t vd -t pvd */
        /*On Simulator, nothing has to do */
        fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_LUN, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_RG, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_RG_FRU, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_VD, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_enable(FBE_TRAFFIC_TRACE_CLASS_PVD, FBE_PACKAGE_ID_SEP_0);
        /* Run I/O to this set of raid groups with the I/O sizes specified
         * (no aborts and no peer I/O)
         *
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          BABY_BEAR_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          BABY_BEAR_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                            /* Don't inject aborts *
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          FBE_FALSE,                            /* Peer I/O not supported *
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);*/

        /* Run IO
        /*Use PRBA tool to display traffic buffer, use following command
        prba -o c:\Rbalogs\log1.ktr */
        /*On simulator, check the logs in console window*/
        /* disable rba traces now */
        fbe_api_traffic_trace_disable(FBE_TRAFFIC_TRACE_CLASS_LUN, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_disable(FBE_TRAFFIC_TRACE_CLASS_RG, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_disable(FBE_TRAFFIC_TRACE_CLASS_VD, FBE_PACKAGE_ID_SEP_0);
        fbe_api_traffic_trace_disable(FBE_TRAFFIC_TRACE_CLASS_PVD, FBE_PACKAGE_ID_SEP_0);
    }
    return;
}
/******************************************
 * end baby_bear_rba_test()
 ******************************************/



/*!**************************************************************************
 * baby_bear_run_read_io_on_mirror
 ***************************************************************************
 * @brief
 *  This function checks if mirror read optimization works as designed.
 *  on a raid group:
 * 
 * @param 
 *
 * @return void
 *
 ***************************************************************************/
static void baby_bear_run_read_io_on_mirror(fbe_object_id_t lun_object_id_1)
{
    fbe_api_rdgen_context_t                      *context_p = &baby_bear_test_contexts[0];
    fbe_lba_t                                    end_lba = BABY_BEAR_RDGEN_START_LBA + BABY_BEAR_RDGEN_LARGE_BLOCK_COUNT - 1;
    fbe_status_t                                 status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "Run mirror read io in multiple thread.\n");


    //  Set up the context to do a read operation with 10 threads 
    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                lun_object_id_1,
                                                FBE_CLASS_ID_INVALID, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_RDGEN_OPERATION_READ_ONLY,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                10,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_FIXED,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                end_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                128    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Write a background pattern to all of the LUNs in the entire configuration
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

}

/*!**************************************************************************
 * baby_bear_run_verify_and_read_io_on_mirror
 ***************************************************************************
 * @brief
 *  This function checks if mirror read optimization works as designed.
 *  on a raid group:
 * 
 * @param 
 *
 * @return void
 *
 ***************************************************************************/
static void baby_bear_run_verify_and_read_io_on_mirror(fbe_object_id_t lun_object_id_1)
{
    fbe_api_rdgen_context_t                      *context_p = &baby_bear_test_contexts[0];
    fbe_lba_t                                    end_lba = BABY_BEAR_RDGEN_START_LBA + BABY_BEAR_RDGEN_LARGE_BLOCK_COUNT - 1;
    fbe_status_t                                 status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "Run mirror first verify and then read io in multiple thread.\n");


    //  Set up the context to do a verify operation with 10 threads 
    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                lun_object_id_1,
                                                FBE_CLASS_ID_INVALID, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_RDGEN_OPERATION_VERIFY,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                10,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_FIXED,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                end_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                128    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Write a background pattern to all of the LUNs in the entire configuration
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    //  Set up the context to do a verify operation with 10 threads 
    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                lun_object_id_1,
                                                FBE_CLASS_ID_INVALID, 
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_RDGEN_OPERATION_READ_ONLY,
                                                FBE_RDGEN_PATTERN_LBA_PASS,
                                                1,    /* We do one full sequential pass. */
                                                0,    /* num ios not used */
                                                0,    /* time not used*/
                                                10,    /* threads */
                                                FBE_RDGEN_LBA_SPEC_FIXED,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                end_lba,    /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                                16,    /* Number of stripes to write. */
                                                128    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    //  Write a background pattern to all of the LUNs in the entire configuration
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

}


/*!**************************************************************************
 * baby_bear_mirror_read_optimization_test
 ***************************************************************************
 * @brief
 *  This function checks if mirror read optimization works as designed.
 *  on a raid group:
 * 
 * @param 
 *
 * @return void
 *
 ***************************************************************************/
static void baby_bear_mirror_read_optimization_test(fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_test_rg_configuration_t                  *rg_config_p = NULL;
    fbe_u32_t                                    rg_index;
    fbe_u32_t                                    raid_group_count;
    fbe_test_logical_unit_configuration_t *      logical_unit_configuration_p = NULL;
    fbe_object_id_t                              lun_object_id_1;
    fbe_status_t                                 status = FBE_STATUS_OK;
    fbe_block_count_t                            elsz;
    fbe_lun_performance_counters_t               lun_stats;
    fbe_perfstats_lun_sum_t                      summed_stats;

    rg_config_p = in_current_rg_config_p;
    raid_group_count = fbe_test_get_rg_count(rg_config_p);

    if (rg_config_p->b_bandwidth)
    {
        elsz = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        elsz = FBE_RAID_SECTORS_PER_ELEMENT;
    }


    mut_printf(MUT_LOG_TEST_STATUS, "Mirror read optimization starts.\n");
    for ( rg_index = 0; rg_index < raid_group_count; rg_index ++,
                                                     rg_config_p ++)
    {
       if(rg_config_p->raid_type!=FBE_RAID_GROUP_TYPE_RAID1 ) 
        {
            continue;
        }
        /* BABY_BEAR_LUNS_PER_RAID_GROUP should be more than 2*/
        logical_unit_configuration_p = &(in_current_rg_config_p[rg_index].logical_unit_configuration_list[0]);
        MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id_1);
        
        fbe_api_perfstats_enable_statistics_for_package(FBE_PACKAGE_ID_SEP_0);
        baby_bear_run_read_io_on_mirror(lun_object_id_1);
        /* Get performance statistics*/
        status = fbe_api_lun_get_performance_stats(lun_object_id_1,
                                                   &lun_stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_perfstats_get_summed_lun_stats(&summed_stats,
                                                        &lun_stats);
            
        MUT_ASSERT_INTEGER_NOT_EQUAL(summed_stats.disk_reads[1], 0);
        if(rg_config_p->width>2)
        {
            MUT_ASSERT_INTEGER_NOT_EQUAL(summed_stats.disk_reads[2], 0);
        }
    }
    return;

}
/******************************************
 * end baby_bear_mirror_read_optimization_test()
 ******************************************/

/*!**************************************************************
 * baby_bear_test_rg_config()
 ****************************************************************
 * @brief
 *  Run baby bear test on a set of configs.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/24/2011 - Created. Rob Foley
 *
 ****************************************************************/

void baby_bear_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{

    /* Test if RBA tracing works
     */
     baby_bear_rba_test(rg_config_p);

    /* Test if mirror read optimization works as designed
     */
     baby_bear_mirror_read_optimization_test(rg_config_p);

     return;
}
/******************************************
 * end baby_bear_test_rg_config()
 ******************************************/

/*!**************************************************************
 * baby_bear_test()
 ****************************************************************
 * @brief
 *  RunThis is the entry point into the baby bear test scenario.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/16/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void baby_bear_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    
    if (extended_testing_level == 0)
    {
        /* Qual.
         */
        rg_config_p = &baby_bear_raid_group_config_qual[0];
    }
    else
    {
        /* Extended. 
         */
        rg_config_p = &baby_bear_raid_group_config_extended[0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, baby_bear_test_rg_config,
                                   BABY_BEAR_LUNS_PER_RAID_GROUP,
                                   BABY_BEAR_CHUNKS_PER_LUN);
     return;
}
/******************************************
 * end baby_bear_test()
 ******************************************/

/*!**************************************************************
 * baby_bear_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the baby bear test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   02/16/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void baby_bear_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level > 0)
        {
            rg_config_p = &baby_bear_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &baby_bear_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, BABY_BEAR_LUNS_PER_RAID_GROUP, BABY_BEAR_CHUNKS_PER_LUN);
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************
 * end baby_bear_setup()
 ******************************************/

/*!**************************************************************
 * baby_bear_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the sector tracing test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/16/2010 - Created. Swati Fursule
 *
 ****************************************************************/

void baby_bear_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end baby_bear_cleanup()
 ******************************************/
