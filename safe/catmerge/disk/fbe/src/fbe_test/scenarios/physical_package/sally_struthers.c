/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file sally_struthers.c
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
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_block_transport.h"
#include "sep_tests.h"


char * sally_struthers_short_desc = "Starvation prevention in the block transport service.";
char * sally_struthers_long_desc =
    "\n"
    "\n"
    "The Sally Struthers scenario tests starvation prevention in the block transport service.\n"
    ;


#define SALLY_STRUTHERS_IO_COMPLETION_DELAY EMCPAL_MINIMUM_TIMEOUT_MSECS
#define SALLY_STRUTHERS_IO_SECONDS 30
#define SALLY_STRUTHERS_THREAD_COUNT 30

static fbe_api_rdgen_context_t sally_struthers_test_rdgen_contexts[FBE_PACKET_PRIORITY_LAST - 1];


/*!**************************************************************
 * @fn sally_struthers_test()
 ****************************************************************
 * @brief
 *  This is the main test function of the sally_struthers test.
 *
 * @param  - None.
 *
 * @return - None.
 *
 * HISTORY:
 *  8/10/2011 - Created. MRF
 *
 ****************************************************************/

void sally_struthers_test(void)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t rdgen_stats;
    fbe_rdgen_filter_t filter;
    fbe_u32_t io_seconds = SALLY_STRUTHERS_IO_SECONDS;
    fbe_u32_t priority;
    fbe_u32_t index;
    fbe_object_id_t object_handle = FBE_OBJECT_ID_INVALID;

    fbe_u32_t total_ios = 0;
    fbe_u32_t total_credits = 0;
    fbe_u32_t credits;
    fbe_u32_t expected_ios;
    fbe_u32_t min_expected_ios;
    fbe_u32_t max_expected_ios;
    fbe_u32_t actual_ios[FBE_PACKET_PRIORITY_LAST - 1];


    status = fbe_api_get_physical_drive_object_id_by_location(0, 0, 0, &object_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);


    status = fbe_api_wait_for_object_lifecycle_state(object_handle, 
                                                     FBE_LIFECYCLE_STATE_READY, 
                                                     READY_STATE_WAIT_TIME, 
                                                     FBE_PACKAGE_ID_PHYSICAL);

    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;

        status = fbe_api_rdgen_test_context_init(
            &sally_struthers_test_rdgen_contexts[index], 
            object_handle,
            FBE_CLASS_ID_INVALID,
            FBE_PACKAGE_ID_PHYSICAL,
            FBE_RDGEN_OPERATION_READ_ONLY,
            FBE_RDGEN_PATTERN_LBA_PASS,
            0,  /* run forever*/ 
            0,  /* io count not used */
            0,  /* time not used*/
            SALLY_STRUTHERS_THREAD_COUNT, /* threads per object */
            FBE_RDGEN_LBA_SPEC_RANDOM,
            0,  /* Start lba*/
            0,  /* Min lba */
            FBE_LBA_INVALID,    /* use capacity */
            FBE_RDGEN_BLOCK_SPEC_RANDOM,
            1,  /* Min blocks */
            1   /* Max blocks */
        );
    
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        sally_struthers_test_rdgen_contexts[index].start_io.specification.options |= FBE_RDGEN_OPTIONS_USE_PRIORITY;
        sally_struthers_test_rdgen_contexts[index].start_io.specification.priority = priority;
    }
    /* Run our I/O test
     */
    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        status = fbe_api_rdgen_start_tests(&sally_struthers_test_rdgen_contexts[index], 
                                           FBE_PACKAGE_ID_NEIT, 
                                           1);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_thread_delay(100);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting I/O for %d seconds ==", __FUNCTION__, io_seconds);
    fbe_api_sleep(io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND);

    /* Make sure I/O progressing okay */
    status = fbe_api_rdgen_get_stats(&rdgen_stats, &filter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Stop I/O */
    for(priority = FBE_PACKET_PRIORITY_INVALID + 1; priority < FBE_PACKET_PRIORITY_LAST; priority++)
    {
        total_credits += block_transport_server_get_credits_from_priority(priority, 0);
        index = priority - 1;
        status = fbe_api_rdgen_stop_tests(&sally_struthers_test_rdgen_contexts[index], 1);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        actual_ios[index] = sally_struthers_test_rdgen_contexts[index].start_io.statistics.io_count;
        total_ios += actual_ios[index];

        MUT_ASSERT_INT_EQUAL(sally_struthers_test_rdgen_contexts[index].start_io.statistics.error_count, 0);
    }

    for(priority = FBE_PACKET_PRIORITY_INVALID + 1; priority < FBE_PACKET_PRIORITY_LAST; priority++)
    {
        credits = block_transport_server_get_credits_from_priority(priority, 0);
        index   = priority - 1;

        expected_ios = (total_ios * credits) / total_credits;
        min_expected_ios = (expected_ios * 9) / 10;
        max_expected_ios = (expected_ios * 11) / 10;

        mut_printf(MUT_LOG_LOW, "Priority: %d, Expected I/Os: %d, Actual I/Os: %d", priority, expected_ios, actual_ios[index]);
        mut_printf(MUT_LOG_LOW, "(Values between %d and %d accepted)", min_expected_ios, max_expected_ios);
        MUT_ASSERT_TRUE(actual_ios[index] >= min_expected_ios);
        MUT_ASSERT_TRUE(actual_ios[index] <= max_expected_ios);
    }
 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/**************************************
 * end sally_struthers_test()
 **************************************/

/*!**************************************************************
 * @fn sally_struthers_load_and_verify()
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
 *  8/10/2011 - Created. MRF
 *
 ****************************************************************/

fbe_status_t sally_struthers_load_and_verify(void)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    
    fbe_neit_package_load_params_t neit_params;

    //status = los_vilos_load_and_verify();

    //status = fbe_api_terminator_set_io_global_completion_delay(SALLY_STRUTHERS_IO_COMPLETION_DELAY);
    //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    //return status;

    mut_printf(MUT_LOG_LOW, "=== Initializing Terminator ===\n");
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_set_io_mode(FBE_TERMINATOR_IO_MODE_ENABLED);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_terminator_set_io_global_completion_delay(SALLY_STRUTHERS_IO_COMPLETION_DELAY);
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
 * end sally_struthers_load_and_verify()
 ******************************************/

/*!**************************************************************
/*************************
 * end file sally_struthers.c
 *************************/
