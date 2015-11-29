/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file howdy_doo_test.c
 ***************************************************************************
 *
 * @brief
 *    This file contains tests to check that sniff verify can remap bad
 *    blocks on non-consumed extents.
 *
 * @version
 *    09/04/2009 - Created. Vicky Guo
 *    06/17/2014 - Added 4K sector drives.   Wayne Garrett
 *
 ***************************************************************************/

/*
 * INCLUDE FILES:
 */

#include "mut.h"                                    // define mut framework interfaces and structures
#include "fbe_test_configurations.h"                // define test configuration interfaces and structures
#include "fbe_test_common_utils.h"                  // define common utility functions 
#include "fbe_test_package_config.h"                // define package load/unload interfaces
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_discovery_interface.h"        // define discovery interfaces and structures
#include "fbe/fbe_api_logical_error_injection_interface.h" 
                                                    // define logical error injection interfaces and structures
#include "fbe/fbe_api_provision_drive_interface.h"  // define provision drive interfaces and structures
#include "fbe/fbe_api_sim_server.h"                 // define sim server interfaces
#include "fbe/fbe_api_utils.h"                      // define fbe utility interfaces                                                    
#include "sep_tests.h"                              // define sep test interfaces
#include "sep_utils.h"                              // define sep utility interfaces
#include "pp_utils.h"                               // define physical package util functions

/*
 * NAMED CONSTANTS:
 */

// Required minimum disk to perform test
#define HOWDY_DOO_4160_DISK_COUNT               1 // number of minimum disk having 4160 block size
#define HOWDY_DOO_520_DISK_COUNT                1 // number of minimum disk having 520 block size

// miscellaneous constants
#define HOWDY_DOO_POLLING_INTERVAL             100  // polling interval (in ms)
#define HOWDY_DOO_PVD_READY_WAIT_TIME           20  // max. time (in seconds) to wait for pvd to become ready
#define HOWDY_DOO_VERIFY_PASS_WAIT_TIME         30  // max. time (in seconds) to wait for verify pass to complete
#define HOWDY_DOO_CHUNK_SIZE                  0x800

/*
 * TYPEDEFS:
 */

// media error case structure used by Howdy Doo test
typedef struct howdy_doo_media_error_case_s
{
    char*                                     description;           // error case description
    fbe_u32_t                                 passes;                // number of verify passes to run
    fbe_u64_t                                 read_errors_injected;  // number of "verify" errors injected
    fbe_u64_t                                 w_verify_blocks_remapped; // number of "write verify" blocks remapped
    fbe_provision_drive_verify_error_counts_t expected;              // expected verify error counts
    fbe_api_logical_error_injection_record_t  record;                // logical error injection record

} howdy_doo_media_error_case_t;


/*
 * GLOBAL DATA:
 */

// user space and metadata disk media error cases used by Howdy Doo test
howdy_doo_media_error_case_t howdy_doo_disk_media_error_cases_g[] =
{
    // test basic hard media error handling on 520/4k block disks
    {
        "Case 0 - Test 4k boundary - inject front - hard",

        0,                                         // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10000, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 1 - Test 4k boundary - inject front - soft",

        0,                                         // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            1,                                      // - recoverable read errors
            0,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10000, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 2 - Test 4k boundary - inject middle - hard",

        0,                                         // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10004, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 3 - Test 4k boundary - inject middle - soft",

        0,                                         // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            1,                                      // - recoverable read errors
            0,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10004, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 4 - Test 4k boundary - inject end - hard",

        0,                                         // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10007, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 5 - Test 4k boundary - inject end - soft",

        0,                                         // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            1,                                      // - recoverable read errors
            0,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10007, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 6 - Test 4k boundary - inject overlap - hard",

        0,                                         // number of passes to run
        4,                                         // number of "verify" errors injected
        4,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10007, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 7 - Test 4k boundary - inject overlap - soft",

        0,                                         // number of passes to run
        4,                                         // number of "verify" errors injected
        4,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            1,                                      // - recoverable read errors
            0,                                      // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x10007, 0x4, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {
        "Case 8 - chunk overlap - hard media error handling",

        1,                                          // number of passes to run
        12,                                         // number of "verify" errors injected
        12,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            2,                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x127f8, 0xc, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },


    // test hard media error retry handling on 520 block disks
    {
        "Case 9 - hard media error retry handling",

        1,                                          // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1,                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x12768, 0xc, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },

    // test basic soft media error handling on 520 block disks
    {
        "Case 10 - chunk overlap - soft media error handling",

        1,                                          // number of passes to run
        12,                                         // number of "verify" errors injected
        12,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            2,                                      // - recoverable read errors
            0                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x127f8, 0xc, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },

    // test soft media error retry handling on 520 block disks
    {
        "Case 11 - soft media error retry handling",

        1,                                          // number of passes to run
        2,                                         // number of "verify" errors injected
        2,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            1,                                      // - recoverable read errors
            0                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x12768, 0xc, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SOFT_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_SAME_LBA, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },

    // test basic hard media error handling in Metadata region on 520 block disks
    {
        "Case 12 - hard media error handling in metadata region",

        1,                                          // number of passes to run
        1,                                         // number of "verify" errors injected
        1,                                         // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record
            0x4, 0x10, 0x5f010, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
        }
    },
    {NULL,} /*test case terminator*/
};

/*!*******************************************************************
 * @struct howdy_doo_unconsumed_areas_t
 *********************************************************************
 * @brief Allows us to keep track of the first and last unconsumed
 *        areas, which is what this test will be validating.
 *
 *********************************************************************/
typedef struct howdy_doo_unconsumed_areas_s
{
    fbe_lba_t start_lba;
    fbe_block_count_t blocks;
}
howdy_doo_unconsumed_areas_t;

enum {
    HOWDY_DOO_UNCONSUMED_AREAS = 2,
};

/*!*******************************************************************
 * @var howdy_doo_unconsumed_test_record
 *********************************************************************
 * @brief Test case for testing that we do not sniff consumed areas.
 *        We will fill in the lba and blocks by looking for unconsumed
 *        areas inside the private space.
 *
 *********************************************************************/
fbe_api_logical_error_injection_record_t howdy_doo_unconsumed_test_record[] =
{
    {
        0x4, 0x10, FBE_LBA_INVALID, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
    },
    {
        0x4, 0x10, FBE_LBA_INVALID, 0x0, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
        FBE_API_LOGICAL_ERROR_INJECTION_MODE_COUNT, 0x0, 0x1, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0
    }
};

/*
 * FORWARD REFERENCES:
 */

void howdy_doo_run_test(fbe_test_raid_group_disk_set_t * disk_set_p, howdy_doo_media_error_case_t * test_case_p);

// run media error test
void
howdy_doo_run_media_error_test
(
    fbe_object_id_t                                 in_pvd_object_id,
    howdy_doo_media_error_case_t*                   in_error_case_p
);

// run verify
void
howdy_doo_run_verify
(
    howdy_doo_media_error_case_t*                   in_error_case_p,
    fbe_object_id_t                                 in_pvd_object_id,
    fbe_provision_drive_verify_report_t*            in_verify_report_p
);

// stop verify
void
howdy_doo_stop_verify
(
    fbe_object_id_t                                 in_pvd_object_id
);

// compute total error counts for specified verify results
void
howdy_doo_sum_verify_results
(
    fbe_provision_drive_verify_error_counts_t*      in_verify_results_p,
    fbe_u32_t*                                      out_error_count_p
);

// wait for verify pass completion
void
howdy_doo_wait_for_verify_pass_completion
(
    howdy_doo_media_error_case_t*                   in_error_case_p,
    fbe_object_id_t                                 in_pvd_object_id,
    fbe_u32_t                                       in_timeout_sec ,
    fbe_provision_drive_verify_report_t*            in_verify_report_p
);

void
howdy_doo_wait_for_verify_error_injection_completion(howdy_doo_media_error_case_t* in_error_case_p,
                                                     fbe_object_id_t in_pvd_object_id,
                                                     fbe_u32_t       in_timeout_sec);

/*
 * TEST DESCRIPTION:
 */

char* howdy_doo_short_desc = "Sniff Verify remaps bad blocks on non-consumed extents";
char* howdy_doo_long_desc =
    "\n"
    "\n"
    "The Howdy Doo Scenario tests that sniff verify can remap bad blocks on non-consumed\n"
    "extents for 512 block and 520 block disks.\n"
    "\n"
    "It covers the following cases:\n"
    "    - it verifies that sniff verify can detect back blocks on non-consumed extents\n"
    "    - it verifies that bad blocks are remapped\n"
    "\n"
    "Dependencies: \n"
    "    - Skippy Doo must pass before this scenario can run successfully\n"
    "    - PVD objects must be able to detect and remap bad blocks\n"
    "    - NEIT must be able to inject hard and soft media errors on PVD objects\n"
    "\n"
    "Starting Config: \n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 512 SAS drive (PDO-A)\n"
    "    [PP] 1 520 SAS drive (PDO-B)\n"
    "    [PP] 2 logical drives (LDO-A, LDO-B)\n"
    "    [SEP] 2 provision drives (PVD-A, PVD-B)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package configuration\n"
    "    - Create all provision drives (PVD) with an I/O edge attached to a logical drive (LD)\n"
    "\n"
    "STEP 2: Set-up NEIT to inject media errors on targeted 512 SAS drive (PVD-A)\n"
    "    - Create error injection record for each media error test case\n"
    "    - Set-up edge hook to inject media errors on selected drive\n"
    "    - Enable error injection on selected drive\n"
    "\n"
    "STEP 3: Run Sniff Verify on targeted 512 SAS drive (PVD-A)\n"
    "    - Clear verify report and check that pass/error counters are zero\n"
    "    - Set verify checkpoint to cover entire exported capacity\n"
    "    - Set verify enable to start sniff verifies\n"
    "    - Wait for a single sniff verify pass to finish\n"
    "    - Validate that bad blocks were remapped as appropriate for 512 SAS drives\n"
    "      and that error counters in the verify report reflect injected media errors\n"
    "\n"
    "STEP 4: Disable error injection on targeted 512 SAS drive (PVD-A)\n"
    "    - Remove edge hook and disable error injection on selected drive\n"
    "    - Check that error injection statistics are as expected\n" 
    "\n"
    "STEP 5: Set-up NEIT to inject media errors on targeted 520 SAS drive (PVD-B)\n"
    "    - Create error injection record for each media error test case\n"
    "    - Set-up edge hook to inject media errors on selected drive\n"
    "    - Enable error injection on selected drive\n"
    "\n"
    "STEP 6: Run Sniff Verify on targeted 520 SAS drive (PVD-B)\n"
    "    - Clear verify report and check that pass/error counters are zero\n"
    "    - Set verify checkpoint to cover entire exported capacity\n"
    "    - Set verify enable to start sniff verifies\n"
    "    - Wait for a single sniff verify pass to finish\n"
    "    - Validate that bad blocks were remapped as appropriate for 520 SAS drives\n"
    "      and that error counters in the verify report reflect injected media errors\n"
    "\n"
    "STEP 7: Disable error injection on targeted 520 SAS drive (PVD-B)\n"
    "    - Remove edge hook and disable error injection on selected drive\n"
    "    - Check that collected error injection statistics are as expected\n" 
    "\n";





/*!****************************************************************************
 *  howdy_doo_run_media_error_test
 ******************************************************************************
 *
 * @brief
 *    This function tests that the targeted provision drive can successfully
 *    run the specified media error case.
 *
 *    The procedure to run a media error case is as follows:
 *
 *    1. Set-up NEIT to inject media errors on the selected drive
 *       - Create an error injection record for the media error case
 *       - Set-up an edge hook to inject media errors on selected drive
 *       - Enable error injection on the selected drive
 *
 *    2. Run a Sniff Verify on the selected drive
 *       - Clear verify report and check that pass/error counters are zero
 *       - Set verify checkpoint to cover entire exported capacity
 *       - Set verify enable to start sniff verifies
 *       - Wait for a single sniff verify pass to finish
 *       - Validate that bad blocks were remapped as appropriate for selected
 *         drive and that error counters in the verify report reflect injected
 *         media errors
 *
 *    3. Disable error injection on the selected drive
 *       - Remove edge hook and disable error injection on selected drive
 *       - Check that error injection statistics are as expected
 *
 * @param   in_pvd_object_id  -  provision drive object id
 * @param   in_error_case_p   -  pointer to media error test case
 *
 * @return  void
 *
 * @author
 *    04/30/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
howdy_doo_run_media_error_test(
                                fbe_object_id_t               in_pvd_object_id,
                                howdy_doo_media_error_case_t* in_error_case_p
                              )
{
    fbe_u32_t                                           current_count;     // error counts for current pass
    fbe_u32_t                                           expected_count;    // expected error counts
    fbe_u32_t                                           previous_count;    // error counts for previous passes
    fbe_api_logical_error_injection_get_object_stats_t  pvd_stats;         // provision drive error injection statistics
    fbe_u64_t                                           r_errors_injected; // read errors injected on provision drive
    fbe_api_logical_error_injection_get_stats_t         stats;             // error injection service statistics
    fbe_status_t                                        status;            // fbe status
    fbe_u32_t                                           total_count;       // accumulated total error counts 
    fbe_provision_drive_verify_report_t                 verify_report;     // provision drive verify report
    fbe_u64_t                                           w_verify_blocks_remapped; // Number of write verify

    // announce this media error test
    mut_printf( MUT_LOG_TEST_STATUS, "Howdy Doo: *** %s\n", in_error_case_p->description );

    // stop verify on this provision drive
    howdy_doo_stop_verify( in_pvd_object_id );

    // clear verify report for this provision drive
    fbe_test_sep_util_provision_drive_clear_verify_report( in_pvd_object_id );

    // get verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

    // check that verify pass count is 0
    MUT_ASSERT_TRUE( verify_report.pass_count == 0 );

    // check current verify result counters; should be zero
    howdy_doo_sum_verify_results( &verify_report.current, &current_count );
    MUT_ASSERT_TRUE( current_count == 0 );

    // check previous verify result counters; should be zero
    howdy_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( previous_count == 0 );

    // check total verify result counters; should be zero
    howdy_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == 0 );

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

	/* Before starting the sniff related operation disable the disk zeroing.
	 */
    status = fbe_api_provision_drive_set_zero_checkpoint(in_pvd_object_id, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Could not set PVD zero flag!");
	status = fbe_test_sep_util_provision_drive_disable_zeroing(in_pvd_object_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    // disable any previous error injection records
    status = fbe_api_logical_error_injection_disable_records( 0, 255 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // add new error injection record for selected error case
    status = fbe_api_logical_error_injection_create_record( &in_error_case_p->record );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // enable error injection on this provision drive
    status = fbe_api_logical_error_injection_enable_object( in_pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // enable error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // check status of enabling error injection service
    status = fbe_api_logical_error_injection_get_stats( &stats );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // check state of logical error injection service, should be enabled 
    MUT_ASSERT_INT_EQUAL( stats.b_enabled, FBE_TRUE );

    // check number of error injection records and objects; both should be one
    MUT_ASSERT_INT_EQUAL( stats.num_records, 1 );
    MUT_ASSERT_INT_EQUAL( stats.num_objects_enabled, 1 );

    // run specified number of verify passes on this provision drive
    howdy_doo_run_verify( in_error_case_p, in_pvd_object_id, &verify_report );

    // stop verify on this provision drive
    howdy_doo_stop_verify( in_pvd_object_id );

    // check that verify pass count is as expected
    MUT_ASSERT_TRUE( verify_report.pass_count == in_error_case_p->passes );

    // sum expected verify result counters
    howdy_doo_sum_verify_results( &in_error_case_p->expected, &expected_count );

    // check current and previous verify result counters;
    // should be equal to expected verify result counters
    howdy_doo_sum_verify_results( &verify_report.current, &current_count );
    howdy_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_INT_EQUAL( (current_count + previous_count), expected_count );

    // check total verify result counters; should be equal
    // to expected verify result counters
    howdy_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_INT_EQUAL( total_count, expected_count );

    // get error injection service statistics
    status = fbe_api_logical_error_injection_get_stats( &stats );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );


    // disable error injection on this provision drive
    status = fbe_api_logical_error_injection_disable_object( in_pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
    // disable error injection service
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // get error injection statistics for this provision drive
    status = fbe_api_logical_error_injection_get_object_stats( &pvd_stats, in_pvd_object_id );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // get counts of errors injected on this provision drive
    r_errors_injected = pvd_stats.num_read_media_errors_injected;
    w_verify_blocks_remapped = pvd_stats.num_write_verify_blocks_remapped;

    // check that number of read errors injected is as expected
	if(in_error_case_p->read_errors_injected != r_errors_injected)
	{
        mut_printf(MUT_LOG_TEST_STATUS,
                   "Not enough read errors. found %d expected %d", 
                   (int)r_errors_injected, (int)in_error_case_p->read_errors_injected);
        MUT_FAIL_MSG("unexpected number of read errors");
    }

    // check that number of write errors injected is as expected
    if(in_error_case_p->w_verify_blocks_remapped != w_verify_blocks_remapped)
	{
        mut_printf(MUT_LOG_TEST_STATUS,
                   "Not enough remaps. found %d expected %d", 
                   (int)w_verify_blocks_remapped, (int)in_error_case_p->w_verify_blocks_remapped);
        MUT_FAIL_MSG("unexpected number of write verify remaps detected");
    }

    // display error injection statistics 
    mut_printf( MUT_LOG_TEST_STATUS,
                "Howdy Doo: errors injected - reads: %llu, writes: %llu, total: %llu",
                (unsigned long long)pvd_stats.num_read_media_errors_injected,
		(unsigned long long)pvd_stats.num_write_verify_blocks_remapped,
		(unsigned long long)stats.num_errors_injected
              );

    // display error detection statistics 
    mut_printf( MUT_LOG_TEST_STATUS,
                "Howdy Doo: errors detected - recovered reads: %d, unrecovered reads: %d",
                verify_report.totals.recov_read_count, verify_report.totals.unrecov_read_count
              );

    /* Restore the sector trace level to it's default.
     */
    fbe_test_sep_util_restore_sector_trace_level();

    return;
}   // end howdy_doo_run_media_error_test()


/*!****************************************************************************
 *  howdy_doo_run_verify
 ******************************************************************************
 *
 * @brief
 *    This function enables and waits for the selected number of verify passes
 *    to complete on the entire exported capacity of  the  specified provision
 *    drive. After all the verify passes complete, a verify report is returned
 *    to the caller.
 *
 *    The procedure to run one or more verify passes is:
 *
 *    1. Set verify checkpoint to zero to cover the entire exported capacity
 *    2. Set verify enable to start a verify pass
 *    3. For each verify pass, wait up to 30 seconds for verify pass to finish
 *    4. Get verify report and return it to the caller
 *
 *    It is assumed that the verify report for the specified  provision  drive
 *    has already been cleared before calling this function.
 *
 * @param   in_verify_passes    -  number of verify passes to run
 * @param   in_pvd_object_id    -  provision drive object id
 * @param   in_verify_report_p  -  pointer to verify report
 *
 * @return  void
 *
 * @author
 *    04/20/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
howdy_doo_run_verify(
                      howdy_doo_media_error_case_t*         in_error_case_p,
                      fbe_object_id_t                       in_pvd_object_id,
                      fbe_provision_drive_verify_report_t*  out_verify_report_p
                    )
{
    fbe_lba_t                                checkpoint = 0;  // verify checkpoint
    fbe_u32_t                                pass_count;      // verify pass count
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status

    if ( in_error_case_p->passes == 0)   
    {
        /* Special case for skipping a full pass. Start checkpoint at or just before error injection area.*/

        checkpoint = (in_error_case_p->record.lba / 0x800) * 0x800;   
    }


    // set verify checkpoint to check provision drive's entire exported capacity
    fbe_test_sep_util_provision_drive_set_verify_checkpoint( in_pvd_object_id, checkpoint);

    // enable verify on this provision drive
    fbe_test_sep_util_provision_drive_enable_verify( in_pvd_object_id );

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

    // check that verify on provision drive is enabled
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_TRUE );

    // verify started for this provision drive
    mut_printf( MUT_LOG_TEST_STATUS, "Howdy Doo: verifies started on object: 0x%x", in_pvd_object_id );

    if ( in_error_case_p->passes == 0)   
    {
        /* Special case for skipping a full pass. Stop after checkpoint passes error injection area.*/
        howdy_doo_wait_for_verify_error_injection_completion(in_error_case_p, in_pvd_object_id, HOWDY_DOO_VERIFY_PASS_WAIT_TIME);
    }
    else
    {
        // now, wait for the specified number of verify passes to complete
        for ( pass_count = 0; pass_count < in_error_case_p->passes; pass_count++ )
        {
            // wait for verifies to finish on this provision drive
            howdy_doo_wait_for_verify_pass_completion(in_error_case_p, in_pvd_object_id, HOWDY_DOO_VERIFY_PASS_WAIT_TIME,out_verify_report_p );
        }
    }

    if(fbe_test_util_is_simulation())
    {
        // get the verify report for this provision drive
        fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, out_verify_report_p );
        howdy_doo_stop_verify( in_pvd_object_id );
    }

    return;
}   // end howdy_doo_run_verify()


/*!****************************************************************************
 *  howdy_doo_stop_verify
 ******************************************************************************
 *
 * @brief
 *    This function stops verifies on the specified provision drive.
 *
 * @param   in_pvd_object_id  -  provision drive object id
 *
 * @return  void
 *
 * @author
 *    04/30/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
howdy_doo_stop_verify(
                       fbe_object_id_t in_pvd_object_id
                     )
{
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status

    // disable verify on this provision drive
    fbe_test_sep_util_provision_drive_disable_verify( in_pvd_object_id );

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

    // check that verify on provision drive is disabled
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_FALSE );
    
    // verify stopped for this provision drive
    mut_printf( MUT_LOG_TEST_STATUS, "Howdy Doo: verifies stopped on object: 0x%x", in_pvd_object_id );

    return;
}   // end howdy_doo_stop_verify()


/*!****************************************************************************
 *  howdy_doo_sum_verify_results
 ******************************************************************************
 *
 * @brief
 *    This function computes total error counts for specified verify results.
 *
 * @param   in_verify_results_p  -  pointer to verify results
 * @param   out_error_count_p    -  pointer to count of reported errors
 *
 * @return  void
 *
 * @author
 *    04/30/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
howdy_doo_sum_verify_results(
                              fbe_provision_drive_verify_error_counts_t* in_verify_results_p,
                              fbe_u32_t*                                 out_error_count_p
                            )
{
    fbe_u32_t                       count;              // error count

    // sum verify error counters
    count  = in_verify_results_p->recov_read_count;
    count += in_verify_results_p->unrecov_read_count;

    // output count of reported verify errors
    *out_error_count_p = count;

    return;
}   // end howdy_doo_sum_verify_results()


/*!****************************************************************************
 *  howdy_doo_wait_for_verify_pass_completion
 ******************************************************************************
 *
 * @brief
 *    This function waits the specified timeout interval for a verify pass to
 *    complete on a provision drive.  Note that the provision drive is polled
 *    every 100 ms during the timeout interval for a completed verify pass.
 *
 * @param   in_pvd_object_id  -  provision drive to check for a verify pass
 * @param   in_timeout_sec    -  verify pass timeout interval (in seconds)
 *
 * @return  void
 *
 * @author
 *    04/20/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
howdy_doo_wait_for_verify_pass_completion( howdy_doo_media_error_case_t* in_error_case_p,
                                           fbe_object_id_t in_pvd_object_id,
                                           fbe_u32_t       in_timeout_sec,
                                           fbe_provision_drive_verify_report_t*  in_verify_report_p
                                         )
{
    fbe_u32_t                                poll_time;            // poll time
    fbe_u32_t                                verify_pass;          // verify pass to wait for
    fbe_provision_drive_verify_report_t      verify_report = {0};  // verify report
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status
    
    
    // get verify report for specified provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

    // determine verify pass to wait for
    verify_pass = verify_report.pass_count + 1;

    // wait specified timeout interval for a verify pass to complete
    for ( poll_time = 0; poll_time < (in_timeout_sec * 100); poll_time++ )
    {
        // get verify report for specified provision drive
        fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

        // finished when next verify pass completes
        if ( verify_report.pass_count == verify_pass )
        {
            // verify pass completed for provision drive
            mut_printf( MUT_LOG_TEST_STATUS,
                        "Howdy Doo: verify pass %d completed for object: 0x%x",
                        verify_report.pass_count, in_pvd_object_id
                      );
           
            return;
        }

        // get verify status for this provision drive
        fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

        if(!fbe_test_util_is_simulation())
        {
            fbe_u32_t                                expected_count;    // expected error counts
            fbe_u32_t                                total_count;    // total error counts
            fbe_u32_t                                current_count;    // current error counts
            fbe_u32_t                                previous_count;    // previous error counts

            // sum expected verify result counters
            howdy_doo_sum_verify_results( &in_error_case_p->expected, &expected_count );
            // to expected verify result counters
            howdy_doo_sum_verify_results( &verify_report.totals, &total_count ); 

            howdy_doo_sum_verify_results( &verify_report.current, &current_count );
            howdy_doo_sum_verify_results( &verify_report.previous, &previous_count );
            
            if((total_count == expected_count)&&((current_count + previous_count) == expected_count))
            {
                *in_verify_report_p = verify_report;
                in_verify_report_p->pass_count = 1;
                howdy_doo_stop_verify( in_pvd_object_id );
                return;
            }
        }
            
        // wait for next poll
        fbe_api_sleep( HOWDY_DOO_POLLING_INTERVAL );
    }

    // verify pass failed to complete for provision drive
    mut_printf( MUT_LOG_TEST_STATUS,
                "Howdy Doo: verify pass failed to complete in %d seconds for object: 0x%x", 
                in_timeout_sec, in_pvd_object_id
              );

    MUT_ASSERT_TRUE( FBE_FALSE );

    return;
}   //end howdy_doo_wait_for_verify_pass_completion()



void
howdy_doo_wait_for_verify_error_injection_completion(howdy_doo_media_error_case_t* in_error_case_p,
                                                     fbe_object_id_t in_pvd_object_id,
                                                     fbe_u32_t       in_timeout_sec)
{
    fbe_status_t status;
    fbe_provision_drive_get_verify_status_t verify_status;
    fbe_provision_drive_verify_report_t verify_report;
    fbe_u32_t wait_time_ms = 0;

    mut_printf( MUT_LOG_TEST_STATUS, "%s: pvd:0x%x, lba:0x%llx, blocks:0x%llx",  
            __FUNCTION__, in_pvd_object_id, in_error_case_p->record.lba, in_error_case_p->record.blocks);

    /* loop until verify checkpoint passes error injection range,  or timeout is exceeded.*/
    do
    {
        fbe_api_sleep(250);
        wait_time_ms += 250;

        status = fbe_test_sep_util_provision_drive_get_verify_status(in_pvd_object_id, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(verify_status.verify_enable, FBE_TRUE);

        if (wait_time_ms >= in_timeout_sec*1000)
        {
            mut_printf( MUT_LOG_TEST_STATUS,
                        "Howdy Doo: %s timeout exceeded. pvd:0x%x, cp:0x%llx",  
                        __FUNCTION__, in_pvd_object_id, verify_status.verify_checkpoint);
            MUT_FAIL();
        }
    } while (verify_status.verify_checkpoint < (in_error_case_p->record.lba + in_error_case_p->record.blocks) &&
             wait_time_ms < in_timeout_sec*1000);
    
    howdy_doo_stop_verify( in_pvd_object_id );

    //if(!fbe_test_util_is_simulation())
    {
        fbe_u32_t                                expected_count;    // expected error counts
        fbe_u32_t                                total_count;    // total error counts
        fbe_u32_t                                current_count;    // current error counts
        fbe_u32_t                                previous_count;    // previous error counts

        fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

        // sum expected verify result counters
        howdy_doo_sum_verify_results( &in_error_case_p->expected, &expected_count );
        // to expected verify result counters
        howdy_doo_sum_verify_results( &verify_report.totals, &total_count ); 

        howdy_doo_sum_verify_results( &verify_report.current, &current_count );
        howdy_doo_sum_verify_results( &verify_report.previous, &previous_count );

        MUT_ASSERT_INT_EQUAL_MSG(expected_count, total_count, "expected != total");
        MUT_ASSERT_INT_EQUAL_MSG((current_count + previous_count), expected_count, "curr+prev != expected");
    }

}

/*!**************************************************************
 * howdy_doo_unconsumed_case()
 ****************************************************************
 * @brief
 *  Run a single case where we set the checkpoint and make sure
 *  that this region gets skipped.
 *
 * @param pvd_object_id - pvd to run test on.  Must be system drive with
 *                        unconsumed area.
 * @param start_lba - Start of region where we set verify chkpt.
 * @param end_lba - end of area that we expect to get skipped.
 *
 * @return None.
 *
 * @author
 *  3/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

void howdy_doo_unconsumed_case(fbe_object_id_t pvd_object_id,
                               fbe_lba_t start_lba,
                               fbe_lba_t end_lba)
{
    fbe_status_t status;
    fbe_provision_drive_get_verify_status_t verify_status;
    /* Stop verify. Move the the checkpoint with verify stopped. Then enable verify
     */
    status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_provision_drive_set_verify_checkpoint(pvd_object_id, start_lba);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_provision_drive_enable_verify(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Let's poll to see that the checkpoint moves so sniff skips over this area 0..start_lba
     */
    verify_status.verify_checkpoint = start_lba;
    while (verify_status.verify_checkpoint == start_lba)
    {
        status = fbe_test_sep_util_provision_drive_get_verify_status(pvd_object_id, &verify_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(verify_status.verify_enable, FBE_TRUE);
        fbe_api_sleep(250);
    }
    /* Make sure we skipped over this area.
     */
    if ((verify_status.verify_checkpoint != start_lba) &&
        (verify_status.verify_checkpoint < end_lba))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%llx sniff checkpoint unexpected\n", (unsigned long long)verify_status.verify_checkpoint);
        MUT_FAIL();
    }
    /* Leave sniffing disabled.
     */
    status = fbe_test_sep_util_provision_drive_disable_verify(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end howdy_doo_unconsumed_case()
 ******************************************/
/*!**************************************************************
 * howdy_doo_get_unconsumed_lbas()
 ****************************************************************
 * @brief
 *  Use the private space layout to find two lbas that
 *  are not consumed by sep.
 *
 * @param fru_number - The fru number to check.               
 *
 * @return None.
 *
 * @author
 *  5/24/2012 - Created. Rob Foley
 *
 ****************************************************************/

void howdy_doo_get_unconsumed_lbas(fbe_u32_t fru_number,
                                   howdy_doo_unconsumed_areas_t *unconsumed_p,
                                   fbe_u32_t num_areas)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t fru_index;
    fbe_private_space_layout_region_t region;
    fbe_bool_t b_found_fru;
    fbe_u32_t current_area_index = 0;

    for (index = 0; index < num_areas; index++)
    {
        unconsumed_p[index].start_lba = FBE_LBA_INVALID;
        unconsumed_p[index].blocks = 0;
    }

    for ( index = 0; index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; index++)
    {
        status = fbe_private_space_layout_get_region_by_index(index, &region);

        if (region.region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            /* Hit terminator.
             */
            break;
        }
        /* See if this region has this fru.
         */
        b_found_fru = FBE_FALSE;
        for(fru_index = 0; fru_index < region.number_of_frus; fru_index++)
        {
            if (region.fru_id[fru_index] == fru_number)
            {
                b_found_fru = FBE_TRUE;
                break;
            }
        }
        if (b_found_fru == FBE_FALSE)
        {
            continue;
        }
        /* Unused and external are regions not used by SEP.
         */
        if ((region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE) ||
            (region.region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED))
        {
            unconsumed_p[current_area_index].start_lba = region.starting_block_address;
            unconsumed_p[current_area_index].blocks = region.size_in_blocks;

            /* The last index will contain the last area that we find, so we will
             * not increment the area index past this index.
             */ 
            if (current_area_index < num_areas - 1)
            {
                current_area_index++;
            }
        }
    }
}
/******************************************
 * end howdy_doo_get_unconsumed_lbas()
 ******************************************/
/*!**************************************************************
 * howdy_doo_unconsumed_extent_test()
 ****************************************************************
 * @brief
 *  - This tests that Sniff is skips over unconsumed areas of the disk.
 *  - This test will setup error injections on an unconsumed area
 *  - Then we will set the checkpoint of sniff so that
 *  - it runs across this area of the disk.
 *
 * @param None.               
 *
 * @return None. 
 *
 * @author
 *  3/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void howdy_doo_unconsumed_extent_test(void)
{
    fbe_status_t status;
    fbe_object_id_t pvd_object_id;
    fbe_api_logical_error_injection_get_object_stats_t  pvd_lei_stats;
    fbe_api_logical_error_injection_get_stats_t         stats;
    fbe_provision_drive_verify_report_t verify_report;
    howdy_doo_unconsumed_areas_t unconsumed_areas[HOWDY_DOO_UNCONSUMED_AREAS];

    howdy_doo_get_unconsumed_lbas(0, &unconsumed_areas[0], HOWDY_DOO_UNCONSUMED_AREAS);

    howdy_doo_unconsumed_test_record[0].lba = unconsumed_areas[0].start_lba;
    howdy_doo_unconsumed_test_record[0].blocks = unconsumed_areas[0].blocks;
    howdy_doo_unconsumed_test_record[1].lba = unconsumed_areas[1].start_lba;
    howdy_doo_unconsumed_test_record[1].blocks = unconsumed_areas[1].blocks;

    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 0, &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s starting pvd_id: 0x%x\n", __FUNCTION__, pvd_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id,
                                                     FBE_LIFECYCLE_STATE_READY,
                                                     (HOWDY_DOO_PVD_READY_WAIT_TIME * 1000),
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    howdy_doo_stop_verify(pvd_object_id);

    fbe_test_sep_util_provision_drive_clear_verify_report(pvd_object_id);

    fbe_test_sep_util_provision_drive_get_verify_report(pvd_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(verify_report.pass_count, 0);

    /* First add error injection record on the unconsumed area.
     */


	/* Before starting the sniff related operation disable the disk zeroing.
	 */
	status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_object_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    status = fbe_api_logical_error_injection_disable_records(0, 255);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We have one record at the start of the drive and another in the middle of the drive. 
     *  
     *    ******** <--- lba 0
     *    *      *
     *    ******** <---- First unconsumed area from 0..HOWDY_DOO_UNCONSUMED_START_LBA_0
     *    *      * <---- Some number of consumed areas start here
     *    *      * 
     *    ********
     *    *      * <--- Next unconsumed area from HOWDY_DOO_UNCONSUMED_START_LBA_1
     *    *      * 
     *    *      * 
     *  
     */
    status = fbe_api_logical_error_injection_create_record(&howdy_doo_unconsumed_test_record[0]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_create_record(&howdy_doo_unconsumed_test_record[1]);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);

    MUT_ASSERT_INT_EQUAL(stats.num_records, 2);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    /* First make sure that if we put checkpoint to unconsumed_areas[0].start_lba, we will skip over extent
     * unconsumed_areas[0].start_lba..unconsumed_areas[0].start_lba + unconsumed_areas[0].blocks
     */
    howdy_doo_unconsumed_case(pvd_object_id, 
                              unconsumed_areas[0].start_lba, 
                              unconsumed_areas[0].start_lba + unconsumed_areas[0].blocks);

    /* Next make sure that if we put checkpoint to START_LBA_1, 
     * we will skip over extent start_lba to start_lba + blocks 
     * this just makes sure that we skip over the start of the extent. 
     */
    howdy_doo_unconsumed_case(pvd_object_id, 
                              unconsumed_areas[1].start_lba,
                              unconsumed_areas[1].start_lba + unconsumed_areas[1].blocks);

    /* Make sure there were no errors in either of the two areas we injected on.
     */
    fbe_test_sep_util_provision_drive_get_verify_report(pvd_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(verify_report.current.recov_read_count, 0);
    MUT_ASSERT_INT_EQUAL(verify_report.current.unrecov_read_count, 0);

    status = fbe_api_logical_error_injection_get_object_stats(&pvd_lei_stats, pvd_object_id);
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    if (pvd_lei_stats.num_errors_injected != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%d errors injected during sniff\n", (int)pvd_lei_stats.num_errors_injected); 
        MUT_FAIL();
    }

    // disable error injection on this provision drive
    status = fbe_api_logical_error_injection_disable_object(pvd_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    // disable error injection service
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Complete pvd_id: 0x%x\n", __FUNCTION__, pvd_object_id);
    return;
}
/******************************************
 * end howdy_doo_unconsumed_extent_test()
 ******************************************/


/*!****************************************************************************
 *  howdy_doo_test
 ******************************************************************************
 *
 * @brief
 *    This is the main entry point for Howdy Doo Scenario tests.   These tests
 *    check that provision drives can remap bad blocks on non-consumed extents
 *    for 512 block and 520 block drives.
 *
 *    Refer to individual media error case descriptions for more details.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    04/30/2010 - Created. Randy Black
 *    05/12/2011 - Modified Amit D 
 *
 *****************************************************************************/

void howdy_doo_test(void)
{
    fbe_status_t                    status;            // status to validate
    fbe_test_raid_group_disk_set_t  disk_set;
    fbe_test_discovered_drive_locations_t      drive_locations;   // available drive location details
    howdy_doo_media_error_case_t * test_case_p = howdy_doo_disk_media_error_cases_g;
    fbe_block_size_t block_size;

    // fill out the available disk locations
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);


    if (fbe_test_util_is_simulation())
    {    
        block_size = fbe_sep_test_util_get_default_block_size();
        if (block_size == FBE_TEST_RG_CONFIG_RANDOM_TAG)
        {
            block_size = (fbe_random()%2)? 520 : 4160;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s Using block size %d", __FUNCTION__, block_size);

        switch(block_size)
        {
            case 520:
                status = fbe_test_get_520_drive_location(&drive_locations, &disk_set);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                break;
    
            case 4160:
                status = fbe_test_get_4160_drive_location(&drive_locations, &disk_set);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                break;
    
            default:
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid block size %d", __FUNCTION__, block_size);
                MUT_FAIL();
        }

        howdy_doo_run_test(&disk_set, test_case_p);
    }
    else  /*hw*/
    {
        /* on hw do both 520 and 4k tests.
        */
        if(fbe_test_get_520_drive_location(&drive_locations, &disk_set) == FBE_STATUS_OK)
        {
            howdy_doo_run_test(&disk_set, test_case_p);
        }
        if(fbe_test_get_4160_drive_location(&drive_locations, &disk_set) == FBE_STATUS_OK)
        {
            howdy_doo_run_test(&disk_set, test_case_p);
        }
    }

    howdy_doo_unconsumed_extent_test();

    return;

}   // end howdy_doo_test()



void howdy_doo_run_test(fbe_test_raid_group_disk_set_t * disk_set_p, howdy_doo_media_error_case_t * test_case_p)
{
    fbe_status_t status;
    fbe_object_id_t pvd_id;

    status = fbe_api_provision_drive_get_obj_id_by_location(disk_set_p->bus,
                                                             disk_set_p->enclosure,
                                                             disk_set_p->slot,
                                                             &pvd_id );

    // insure that there were no errors
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // wait 20 seconds for this provision drive to become READY
    status = fbe_api_wait_for_object_lifecycle_state( pvd_id,
                                                      FBE_LIFECYCLE_STATE_READY,
                                                      (HOWDY_DOO_PVD_READY_WAIT_TIME * 1000),
                                                      FBE_PACKAGE_ID_SEP_0
                                                    );

    // insure that provision drive is now READY
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // run media error tests 

    while (test_case_p->description != NULL)
    {
        howdy_doo_run_media_error_test( pvd_id, test_case_p );
        test_case_p++;
    }

}
/*!****************************************************************************
 *  howdy_doo_setup
 ******************************************************************************
 *
 * @brief
 *    This is the setup function for the Howdy Doo test.  It configures all
 *    objects and attaches them into the topology as follows:
 *
 *    1. Create and verify the initial physical package configuration
 *    2. Create all provision drives (PVD) with an I/O edge attached to its
 *       corresponding logical drive (LD)
 *    
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    04/30/2010 - Created. Randy Black
 *
 *****************************************************************************/

void howdy_doo_setup(void)
{
    fbe_status_t                    status = FBE_STATUS_OK;            // status to validate     

    // Only load config in simulation.
    if (fbe_test_util_is_simulation())
    {
        fbe_test_pp_util_create_physical_config_for_disk_counts(HOWDY_DOO_520_DISK_COUNT,
                                                                HOWDY_DOO_4160_DISK_COUNT,
                                                                TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

        elmo_load_sep_and_neit();

    }


    // disable error injection on all objects
    status = fbe_api_logical_error_injection_disable_object( FBE_OBJECT_ID_INVALID, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    
    return;

}   // end howdy_doo_setup()


/*!****************************************************************************
 *  howdy_doo_cleanup
 ******************************************************************************
 *
 * @brief
 *    This is the clean-up function for the Howdy Doo test.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    04/30/2010 - Created. Randy Black
 *
 *****************************************************************************/

void howdy_doo_cleanup(void)
{
    // Only destroy config in simulation  
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    return;

}   // end howdy_doo_cleanup()

/*************************
 * end file howdy_doo_test.c
 *************************/
