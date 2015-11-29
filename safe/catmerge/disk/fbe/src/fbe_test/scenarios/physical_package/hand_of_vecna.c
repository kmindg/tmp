
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file hand_of_vecna.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Sally Struthers test, which is an fbe_test for
 *  starvation avoidance.
 *
 * @revision
 *   8/10/2011:  Created. MRF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "physical_package_tests.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test_package_config.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_media_error_edge_tap.h"
#include "pp_utils.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "neit_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe/fbe_api_perfstats_sim.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_board_interface.h"

char * hand_of_vecna_short_desc = "Performance statistics logging accuracy and functionality for Physical Package.";
char * hand_of_vecna_long_desc =
    "\n"
    "\n"
    "This test will alternate between reads and writes on every LDO in the system, then check the perfstats service to make sure the counters are accurate.\n"
    ;

/*!*******************************************************************
 * @def HAND_OF_VECNA_IO_COMPLETION_DELAY
 *********************************************************************
 * @brief Terminator IO delay for this test.
 *********************************************************************/
#define HAND_OF_VECNA_IO_COMPLETION_DELAY EMCPAL_MINIMUM_TIMEOUT_MSECS

/*!*******************************************************************
 * @def HAND_OF_VECNA_IO_COUNT
 *********************************************************************
 * @brief Number of IOs to issue to each PDO for this test.
 *********************************************************************/
#define HAND_OF_VECNA_IO_COUNT 50

/*!*******************************************************************
 * @def HAND_OF_VECNA_BLOCK_COUNT
 *********************************************************************
 * @brief Block count for IOs issued during this test.
 *********************************************************************/
#define HAND_OF_VECNA_BLOCK_COUNT 16

/*!*******************************************************************
 * @def HAND_OF_VECNA_THREAD_COUNT
 *********************************************************************
 * @brief Thread count for rdgen contexts in this test
 *********************************************************************/
#define HAND_OF_VECNA_THREAD_COUNT 5

/*!**************************************************************
 * @fn hand_of_vecna_test()
 ****************************************************************
 * @brief
 *  This is the main test function of the hand_of_vecna test.
 *
 * @param  - None.
 *
 * @return - None.
 *
 * HISTORY:
 *  7/11/2012 - Created. Ryan Gadsby
 *
 ****************************************************************/

void hand_of_vecna_test(void)
{
    fbe_status_t status;
    fbe_object_id_t object_handle = FBE_OBJECT_ID_INVALID;
    
    fbe_u32_t                                       pdo_i;
    fbe_u32_t                                       pdo_count;
    fbe_u32_t                                       core;
    fbe_u32_t                                       total_cores;
    fbe_bool_t                                      is_enabled;

    
    fbe_api_rdgen_context_t                         context;
    fbe_board_mgmt_platform_info_t                  platform_info;

    fbe_pdo_performance_counters_t                  pdo_stats;
    fbe_object_id_t                                 *pdo_list;

    //get core count
    mut_printf(MUT_LOG_TEST_STATUS, "Getting board object ID.");
    status = fbe_api_get_board_object_id(&object_handle);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get board object ID!");

    mut_printf(MUT_LOG_TEST_STATUS,"Getting platform information.");
    status = fbe_api_board_get_platform_info(object_handle, &platform_info);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get platform info!");

    if (FBE_IS_TRUE(platform_info.is_simulation)){
        total_cores = 6;
    }
    else{
        total_cores = (fbe_u32_t) platform_info.hw_platform_info.processorCount;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "Getting PDO object list");
    status = fbe_api_enumerate_class(FBE_CLASS_ID_SAS_PHYSICAL_DRIVE,FBE_PACKAGE_ID_PHYSICAL,&pdo_list,&pdo_count);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not get PDO object list!");
    mut_printf(MUT_LOG_TEST_STATUS, "PDO list head: 0x%X, pdo_count = %d", *pdo_list, pdo_count);

    //enable stat logging
    mut_printf(MUT_LOG_TEST_STATUS, "Enabling PDO stats");
    status = fbe_api_perfstats_enable_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics enabling failed for physical!");

    //check to see if stat logging took hold
    mut_printf(MUT_LOG_TEST_STATUS, "Testing if PDO stats enabled correctly");
    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                                            &is_enabled);
    MUT_ASSERT_TRUE_MSG((is_enabled && status == FBE_STATUS_OK), "Package-wide statistics enabling failed for Physcial!");

    for (pdo_i = 0; pdo_i < pdo_count; pdo_i++) 
    {
        status = fbe_api_wait_for_object_lifecycle_state(*(pdo_list+pdo_i), 
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     READY_STATE_WAIT_TIME, 
                                                     FBE_PACKAGE_ID_PHYSICAL);

        //set up rdgen context
        mut_printf(MUT_LOG_TEST_STATUS, "Initiating rdgen context for PDO: %d", pdo_i);
        status = fbe_api_rdgen_test_context_init(&context,                      //context
                                                 *(pdo_list+pdo_i),             //pdo object
                                                 FBE_CLASS_ID_INVALID,          //class ID not needed
                                                 FBE_PACKAGE_ID_PHYSICAL,       //testing Physical and down
                                                 (pdo_i % 2) ? FBE_RDGEN_OPERATION_READ_ONLY : FBE_RDGEN_OPERATION_WRITE_ONLY,                      //rdgen operation
                                                 FBE_RDGEN_PATTERN_LBA_PASS,    //rdgen pattern
                                                 0,                             //number of passes (not needed because of manual stop)
                                                 HAND_OF_VECNA_IO_COUNT,        //number of IOs 
                                                 0,                             //time before stop (not needed because of manual stop)
                                                 HAND_OF_VECNA_THREAD_COUNT,    //thread count
                                                 FBE_RDGEN_LBA_SPEC_RANDOM,     //seek pattern
                                                 0x0,                           //starting LBA offset
                                                 0x0,                           //min LBA is 0
                                                 FBE_LBA_INVALID,               //stroke the entirety of the PDO
                                                 FBE_RDGEN_BLOCK_SPEC_RANDOM,   //constant IO size
                                                 HAND_OF_VECNA_BLOCK_COUNT,     //min IO size
                                                 HAND_OF_VECNA_BLOCK_COUNT);    //max IO size, same as min for constancy
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't initialize rdgen context!");

        //set affinity
        mut_printf(MUT_LOG_TEST_STATUS, "Setting rdgen affinity for PDO: %d", pdo_i);
        core = (fbe_u32_t) pdo_i % total_cores;
        status = fbe_api_rdgen_io_specification_set_affinity(&context.start_io.specification,
                                                             FBE_RDGEN_AFFINITY_FIXED,
                                                             core);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't set rdgen context affinity!");

        //kick off IO
        mut_printf(MUT_LOG_TEST_STATUS, "Starting IO for PDO: %d", pdo_i);
        status = fbe_api_rdgen_start_tests(&context,
                                           FBE_PACKAGE_ID_NEIT,
                                           1);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

        //wait to finish
        mut_printf(MUT_LOG_TEST_STATUS, "Waiting for IO to finish on PDO: %d", pdo_i);
        status = fbe_api_rdgen_wait_for_ios(&context,
                                            FBE_PACKAGE_ID_NEIT,
                                            1);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Couldn't wait for IOs to finish!");

        status = fbe_api_rdgen_test_context_destroy(&context);

        //get perfstats
        mut_printf(MUT_LOG_TEST_STATUS, "Getting perf stats for PDO: %d", pdo_i);
        status = fbe_api_physical_drive_get_perf_stats(*(pdo_list+pdo_i),
                                                       &pdo_stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        //verify reads/writes are accurate, other stat buckets hit correctly.
        mut_printf(MUT_LOG_TEST_STATUS, "Verifying counters for PDO: %d", pdo_i);
        if (pdo_i % 2) //writes
        {
            MUT_ASSERT_TRUE_MSG((pdo_stats.disk_reads[core] == HAND_OF_VECNA_IO_COUNT * HAND_OF_VECNA_THREAD_COUNT), "read count mismatch");
            MUT_ASSERT_TRUE_MSG((pdo_stats.disk_blocks_read[core] == HAND_OF_VECNA_IO_COUNT * HAND_OF_VECNA_BLOCK_COUNT * HAND_OF_VECNA_THREAD_COUNT), "Blocks_read mismatch");
        }
        else //reads
        {
            MUT_ASSERT_TRUE_MSG((pdo_stats.disk_writes[core] == HAND_OF_VECNA_IO_COUNT * HAND_OF_VECNA_THREAD_COUNT), "Write count mismatch");
            MUT_ASSERT_TRUE_MSG((pdo_stats.disk_blocks_written[core] == HAND_OF_VECNA_IO_COUNT * HAND_OF_VECNA_BLOCK_COUNT * HAND_OF_VECNA_THREAD_COUNT), "Blocks_written mismatch");
        }

        //always verify other counters
        MUT_ASSERT_TRUE_MSG((pdo_stats.sum_blocks_seeked[core] > 0), "Blocks seeked should exceed zero");
        MUT_ASSERT_TRUE_MSG((pdo_stats.sum_arrival_queue_length[core] > 0), "Sum Queue Length should exceed zero");
        //MUT_ASSERT_TRUE_MSG((pdo_stats->pdo_stats[disk_offset].busy_ticks[core] > 0), "Busy ticks should exceed zero"); 
        //MUT_ASSERT_TRUE_MSG((pdo_stats->pdo_stats[disk_offset].idle_ticks[core] > 0 ), "Idle ticks should exceed zero");
        MUT_ASSERT_TRUE_MSG((pdo_stats.non_zero_queue_arrivals[core] > 0 ), "Nonzero queue arrivals should exceed zero");
    }

    //enable stat logging
    mut_printf(MUT_LOG_TEST_STATUS, "Disabling PDO stats");
    status = fbe_api_perfstats_disable_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics disabling failed for physical!");

    //check to see if stat logging took hold
    mut_printf(MUT_LOG_TEST_STATUS, "Testing if PDO stats enabled correctly");
    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                                            &is_enabled);
    MUT_ASSERT_TRUE_MSG((!is_enabled && status == FBE_STATUS_OK), "Package-wide statistics enabling failed for Physcial!");

    //enable stat logging
    mut_printf(MUT_LOG_TEST_STATUS, "Disabling PDO stats");
    status = fbe_api_perfstats_disable_statistics_for_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Statistics disabling failed for physical!");

    //check to see if stat logging took hold
    mut_printf(MUT_LOG_TEST_STATUS, "Testing if PDO stats enabled correctly");
    status = fbe_api_perfstats_is_statistics_collection_enabled_for_package(FBE_PACKAGE_ID_PHYSICAL,
                                                                            &is_enabled);
    MUT_ASSERT_TRUE_MSG((!is_enabled && status == FBE_STATUS_OK), "Package-wide statistics enabling failed for Physcial!");

    mut_printf(MUT_LOG_TEST_STATUS, "Statistics for all available PDO objects verified!");

    return;
}
/**************************************
 * end hand_of_vecna_struthers_test()
 **************************************/

/*!**************************************************************
 * @fn hand_of_vecna_load_and_verify()
 ****************************************************************
 * @brief
 *   This function creates the configuration needed for this test.
 *   We create one 520 SAS drive
 *
 * @param  - None.
 *
 * @return - fbe_status_t
 *
 * HISTORY:
 *  7/11/2011 - Created. Ryan Gadsby
 *
 ****************************************************************/

fbe_status_t hand_of_vecna_load_and_verify(void)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_neit_package_load_params_t neit_params;

    mut_printf(MUT_LOG_LOW, "=== Initializing Terminator ===\n");
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_set_io_global_completion_delay(HAND_OF_VECNA_IO_COMPLETION_DELAY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_load_simple_config();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting FBE API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting Physical Package ===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting NEIT package ===\n");
    neit_config_fill_load_params(&neit_params);
    neit_params.load_raw_mirror = FBE_FALSE;
    status = fbe_api_sim_server_load_package_params(FBE_PACKAGE_ID_NEIT, &neit_params, sizeof(neit_params));
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set NEIT package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_thread_delay(3000);

    return(status);
}
/******************************************
 * end hand_of_vecna_load_and_verify()
 ******************************************/

/*!**************************************************************
/*************************
 * end file hand_of_vecna.c
 *************************/
