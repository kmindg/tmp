/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file dixie_doo_test.c
 ***************************************************************************
 *
 * @brief
 *    This file contains tests to check that sniff verify can remap bad
 *    blocks on consumed extents.
 *
 * @version
 *    12/01/2009 - Created. Vicky Guo
 *
 ***************************************************************************/

/*
 * INCLUDE FILES:
 */

#include "mut.h"                                    // define mut framework interfaces and structures
#include "fbe_test_configurations.h"                // define test configuration interfaces and structures
#include "fbe_test_package_config.h"                // define package load/unload interfaces
#include "fbe/fbe_api_database_interface.h"    // define configuration interfaces and structures
#include "fbe/fbe_api_discovery_interface.h"        // define discovery interfaces and structures
#include "fbe/fbe_api_logical_error_injection_interface.h" 
                                                    // define logical error injection interfaces and structures
#include "fbe/fbe_api_lun_interface.h"              // define lun object interfaces and structures 
#include "fbe/fbe_api_provision_drive_interface.h"  // define provision drive interfaces and structures
#include "fbe/fbe_api_raid_group_interface.h"       // define RAID group object interfaces and structures
#include "fbe/fbe_api_sim_server.h"                 // define sim server interfaces
#include "fbe/fbe_api_utils.h"                      // define fbe utility interfaces
#include "sep_tests.h"                              // define sep test interfaces
#include "sep_utils.h"                              // define sep utility interfaces
#include "sep_test_io.h"                            // define sep io interfaces
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "sep_rebuild_utils.h"
#include "sep_zeroing_utils.h"
#include "pp_utils.h"

/*
 * NAMED CONSTANTS:
 */

// LU configuration parameters
#define DIXIE_DOO_LUN_CAPACITY              0x2000  // LU capacity (in blocks)
#define DIXIE_DOO_CHUNKS_PER_LUN                 4																										
#define DIXIE_DOO_LUN_ELEMENT_SIZE             128  // LU element size
#define DIXIE_DOO_LUN_NUMBER                     0  // LU number
#define DIXIE_DOO_LUNS_PER_RAID_GROUP            5  // number of LUs in RAID group
#define DIXIE_DOO_JOURNAL_VERIFY_RECORDS         5
#define DIXIE_DOO_JOURNAL_RECORDS                2

// RAID group configuration parameters
#define DIXIE_DOO_RAID_GROUP_CAPACITY       0xE000  // RAID group capacity (in blocks)

#define DIXIE_DOO_RAID_GROUP_COUNT               2  // number of RAID groups

#define DIXIE_DOO_RAID_GROUP_ID_0                0  // RAID group id
#define DIXIE_DOO_RAID_GROUP_ID_1                1  // RAID group id
#define DIXIE_DOO_RAID_GROUP_ID_2                2  // RAID group id                                                    // 
#define DIXIE_DOO_RAID_GROUP_WIDTH               3  // number of disks in RAID group

// miscellaneous constants
#define DIXIE_DOO_MEDIA_ERROR_CASE_MAX           1  // number of media error cases
#define DIXIE_DOO_POLLING_INTERVAL             100  // polling interval (in ms)
#define DIXIE_DOO_RG_VERIFY_WAIT_TIME           30  // max. time (in seconds) to wait for RAID group verify to complete
#define DIXIE_DOO_SNIFF_VERIFY_IO_WAIT_TIME      10  // max. time (in seconds) to wait for sniff verify i/o to complete
#define DIXIE_DOO_SNIFF_WAIT_TIME              20000

#define DIXIE_DOO_FIRST_POSITION_TO_REMOVE       0
#define DIXIE_DOO_SECOND_POSITION_TO_REMOVE       1


/*
 * TYPEDEFS:
 */

// media error case structure used by Dixie Doo test
typedef struct dixie_doo_media_error_case_s
{
    char*                                     description;           // error case description
    fbe_lba_t                                 sv_lba;                // sniff verify i/o start lba
    fbe_block_count_t                         sv_blocks;             // sniff verify i/o block count
    fbe_u64_t                                 read_errors_injected;  // number of "verify" errors injected
    fbe_u64_t                                 write_errors_injected; // number of "write verify" errors injected 
    fbe_provision_drive_verify_error_counts_t expected;              // expected verify error counts
    fbe_api_logical_error_injection_record_t  pvd_record;            // logical error injection record for pvd
    fbe_bool_t                                degrade_rg;
    fbe_bool_t                                broken_rg;

} dixie_doo_media_error_case_t;



/*
 * GLOBAL DATA:
 */

// information used to configure RAID groups
fbe_test_rg_configuration_t dixie_doo_raid_group_configuration_g[DIXIE_DOO_RAID_GROUP_COUNT+1] =
{
    // RAID group 0 configuration
    {
        DIXIE_DOO_RAID_GROUP_WIDTH,                 // RAID group width
        DIXIE_DOO_RAID_GROUP_CAPACITY,              // RAID group capacity (in blocks)
        FBE_RAID_GROUP_TYPE_RAID5,                  // RAID type
        FBE_CLASS_ID_PARITY,                        // RAID type's class id
        520,                                        // block size
        DIXIE_DOO_RAID_GROUP_ID_0,                  // RAID group id
        0                                           // bandwidth
    },

    // RAID group 1 configuration
    {
        4,                                          // RAID group width
        DIXIE_DOO_RAID_GROUP_CAPACITY,              // RAID group capacity (in blocks)
        FBE_RAID_GROUP_TYPE_RAID10,                  // RAID type
        FBE_CLASS_ID_STRIPER,                        // RAID type's class id
        520,                                        // block size
        DIXIE_DOO_RAID_GROUP_ID_1,                  // RAID group id
        0                                           // bandwidth
    },

     /* Terminator.
     */
    { 
        FBE_U32_MAX,
        FBE_U32_MAX,
    },
};

// rdgen test context (used to initialize all LUs in a single operation)
fbe_api_rdgen_context_t dixie_doo_rdgen_test_context_g[DIXIE_DOO_LUNS_PER_RAID_GROUP * DIXIE_DOO_RAID_GROUP_COUNT];


// media error cases used by Dixie Doo test.  Each test case broken up per lun.
dixie_doo_media_error_case_t dixie_doo_media_error_cases_g [] =
{
    // first lun - test single block hard media error handling
	{
        "Test single block hard media error handling (first lun)",

        0x10000,                                    // i/o start lba
        1,                                          // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd
            0x4, 0x10, 0x10000, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
        FBE_FALSE,
        FBE_FALSE
    },
   
	 // second Lun - requires different LBA for error injection
    {
        "Test single block hard media error handling (second lun)",

        0x11800,                                     // i/o start lba
        1,                                          // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd.
            0x4, 0x10, 0x11801, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },

        FBE_FALSE,
        FBE_FALSE
    },

     // 3rd Lun - requires different LBA for error injection
    {
        "Test single block hard media error handling (third lun)",

        0x12800,                                     // i/o start lba
        1,                                          // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd.
            0x4, 0x10, 0x12800, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },

        FBE_FALSE,
        FBE_FALSE
    },

    // 4th Lun - Degrade RG test case
    {
        "Test single block hard media error handling (4th lun)",

        0x11800,                                     // i/o start lba
        1,                                          // i/o block count
        3,                                          // number of "verify" errors injected (2 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd.
            0x4, 0x10, 0x11801, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },

        FBE_TRUE,
        FBE_FALSE
    },

    // 5th Lun - BROKEN RG test case
    {
        "Test single block hard media error handling (5th lun)",

        0x10000,                                          // i/o start lba
        1,                                          // i/o block count
        3,                                          // number of "verify" errors injected (2 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd
            0x4, 0x10, 0x10000, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
       
        FBE_FALSE,
        FBE_TRUE
    },
};

dixie_doo_media_error_case_t dixie_doo_4k_boundary_media_error_cases_g [] = 
{
    // test single block hard media error handling.  Front of 4k boundary
    {
        "Test hard media error handling at end of 4k boundary (first lun)",

        0x10000,                                    // i/o start lba
        0x800,                                      // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd
            0x4, 0x10, 0x10000, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
        FBE_FALSE,
        FBE_FALSE
    },

    // test single block hard media error handling.  End of 4k boundary
    {
        "Test hard media error handling at end of 4k boundary (first lun)",

        0x10000,                                    // i/o start lba
        0x800,                                      // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd
            0x4, 0x10, 0x10007, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
        FBE_FALSE,
        FBE_FALSE
    },

    // test single block hard media error handling.  Middle of 4k boundary
    {
        "Test hard media error handling at middle of 4k boundary (first lun)",

        0x10000,                                    // i/o start lba
        0x800,                                      // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd
            0x4, 0x10, 0x10004, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
        FBE_FALSE,
        FBE_FALSE
    },
    // test single block hard media error handling.  Accross 4k boundary
    {
        "Test hard media error handling overlapping 4k boundary(first lun)",

        0x10000,                                    // i/o start lba
        0x800,                                      // i/o block count
        3,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        2,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },

        {                                           // logical error injection record for pvd
            0x4, 0x10, 0x10007, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
        FBE_FALSE,
        FBE_FALSE
    },

    // Validates setting a non 1MB aligned verify checkpoint.  
    {
        "validate invalid verify_checkpoint",

        0x10400,                                    // verify_checkpoint LBA.  It's not aligned so it will be adjusted to previous aligned LBA.
        0x800,                                      // i/o block count
        2,                                          // number of "verify" errors injected (1 Verify error by PVD + 1 read read from RAID
        1,                                          // number of "write verify" errors injected

        {                                           // expected verify error counts
            0,                                      // - recoverable read errors
            1                                       // - unrecoverable read errors
        },
        {                                           // logical error injection record for pvd.   Injecting to an LBA prior to the start LBA above.  
                                                    // The checkpoint should be re-aligned to prior 1MB causing this error record to be injected.
            0x4, 0x10, 0x10000, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
            FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
            0
        },
        FBE_FALSE,
        FBE_FALSE
    },
    {NULL,} /*test case terminator*/
};


/*!*******************************************************************
 * @var dixie_doo_journal_verify_test_record
 *********************************************************************
 * @brief Test case for testing sniff of journal areas.
 * We inject on non-chunk aligned boundaries to catch overruns
 *   as in AR-570304
 *
 *********************************************************************/
fbe_api_logical_error_injection_record_t dixie_doo_journal_verify_test_record[DIXIE_DOO_JOURNAL_VERIFY_RECORDS][DIXIE_DOO_JOURNAL_RECORDS] =
{
    {
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c180, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY // Only inject on verify 
        },
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c180, 0x1, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY // Only inject on verify 
        }
    },

    {
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c180, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY // Only inject on verify 
        },
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c180, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY // Only inject on verify 
        }

    },

    /* Test 4k boundary - Inject front.   Done by test cases above. */

    /* Test 4k boundary - Inject end.*/
    {
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c186, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY // Only inject on verify 
        },
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c186, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY // Only inject on verify 
        }
    },

    /* Test 4k boundary - Inject middle.*/
    {
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c184, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY // Only inject on verify 
        },
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c184, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY // Only inject on verify 
        }
    },

    /* Test 4k boundary - Inject overlap.*/
    {
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c187, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY // Only inject on verify 
        },
        {                                           // logical error injection record for pvd
                0x4, 0x10, 0x2c187, 0x2, FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,
                FBE_API_LOGICAL_ERROR_INJECTION_MODE_INJECT_UNTIL_REMAPPED, 0x1, 0x15, 0x0, 0x15, 0x1, 0x0, 0x0, 0x0, 0x0,
                FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY // Only inject on verify 
        }
    }

};


/*
 * FORWARD REFERENCES:
 */

// execute verify i/o
void
dixie_doo_execute_verify_io
(
    fbe_object_id_t                                 in_pvd_object_id,
    fbe_lba_t                                       in_start_lba,
    fbe_block_count_t                               in_block_count,
    fbe_provision_drive_verify_report_t*            in_verify_report_p
);

// run media error test
void
dixie_doo_run_media_error_test
(
    fbe_object_id_t                                 in_lu_object_id,
    fbe_object_id_t                                 in_rg_object_id,
    fbe_object_id_t                                 in_pvd_object_id,
    dixie_doo_media_error_case_t*                   in_error_case_p,
    fbe_test_rg_configuration_t*                    rg_config_p
);


// stop verify
void
dixie_doo_stop_verify
(
    fbe_object_id_t                                 in_pvd_object_id
);

// compute total error counts for specified verify results
void
dixie_doo_sum_verify_results
(
    fbe_provision_drive_verify_error_counts_t*      in_verify_results_p,
    fbe_u32_t*                                      out_error_count_p
);

// wait for RAID group verify to complete
fbe_status_t
dixie_doo_wait_for_rg_verify_completion
(
    fbe_object_id_t                                 in_rg_object_id,
    fbe_u32_t                                       in_timeout_sec 
);

// wait for verify i/o completion
void
dixie_doo_wait_for_verify_io_completion
(
    fbe_object_id_t                                 in_pvd_object_id,
    fbe_lba_t                                       in_end_lba,
    fbe_u32_t                                       in_timeout_sec 
);

// write background pattern to all congifured LUs
void
dixie_doo_write_background_pattern
(
    fbe_api_rdgen_context_t*                        in_test_context_p,
    fbe_u32_t                                       in_element_size
);

void dixie_doo_get_sniff_hook(fbe_object_id_t object_id, fbe_scheduler_hook_state_t    hook_state,
                              fbe_scheduler_hook_substate_t hook_substate, fbe_bool_t was_hit);
void dixie_doo_set_sniff_hook(fbe_object_id_t object_id, fbe_scheduler_hook_state_t    hook_state,
                              fbe_scheduler_hook_substate_t hook_substate);
void dixie_doo_delete_sniff_hook(fbe_object_id_t object_id, fbe_scheduler_hook_state_t    hook_state,
                                 fbe_scheduler_hook_substate_t hook_substate);
void dixie_doo_wait_for_hook(fbe_object_id_t object_id, fbe_scheduler_hook_state_t    hook_state,
                             fbe_scheduler_hook_substate_t hook_substate);

void dixie_doo_test_rg_degraded(fbe_test_rg_configuration_t*  rg_config_p, fbe_object_id_t    rg_object_id,
                                fbe_object_id_t    pvd_object_id, fbe_lba_t  start_lba,
                                fbe_block_count_t                    in_block_count);

void dixie_doo_test_rg_broken(fbe_test_rg_configuration_t*  rg_config_p, fbe_object_id_t    rg_object_id,
                              fbe_object_id_t    pvd_object_id, fbe_lba_t       start_lba,
                              fbe_block_count_t                    in_block_count);

void dixie_doo_run_journal_error_test(fbe_object_id_t in_pvd_object_id,
                                      fbe_object_id_t in_rg_object_id);


/*
 * TEST DESCRIPTION:
 */

char* dixie_doo_short_desc = "Sniff Verify remaps bad blocks on consumed extents";
char* dixie_doo_long_desc =
    "\n"
    "\n"
    "The Dixie Doo Scenario tests that sniff verify can remap bad blocks on consumed extents.\n"
    "\n"
    "It covers the following cases:\n"
    "    - it verifies that sniff verify can detect back blocks on consumed extents\n"
    "    - it verifies that bad blocks are remapped by RAID\n"
    "\n"
    "Dependencies: \n"
    "    - Skippy Doo must pass before this scenario can run successfully\n"
    "    - PVD objects must be able to detect bad blocks and inform RAID\n"
    "    - RAID object must be able to remap bad blocks\n"
    "\n"
    "Starting Config: \n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 3 512 SAS drives and corresponding logical drives\n"
    "    [PP] 3 520 SAS drives and corresponding logical drives\n"
    "    [SEP] 6 provision drives (3 for 512 SAS and 3 for 520 SAS drives)\n"
    "    [SEP] 6 virtual drives (3 for 512 SAS and 3 for 520 SAS drives)\n"
    "    [SEP] 2 RAID-5 RAID objects (RG-A, RG-B)\n"
    "    [SEP] 4 LU objets \n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package configuration\n"
    "    - Create all provision drives and attach edges to a logical drive\n"
    "    - Create all virtual drives and attach edges to provision drives\n"
    "    - Create a 3-drive RAID-5 RAID group object and attach edges to virtual drives\n"
    "    - Create a 3-drive RAID-5 RAID group object and attach edges to virtual drives\n"
    "    - Create 2 LUs (LU-A) with an I/O edge attached to the RAID-5 RAID group object(RG-A)\n"
    "    - Create 2 LUs (LU-B) with an I/O edge attached to the RAID-5 RAID group object(RG-B)\n"
    "\n"
    "STEP 2: Write initial background data pattern\n"
    "    - Use rdgen to write an initial data pattern to all LU objects\n"
    "\n"
    "STEP 3: Set-up NEIT to inject media errors on targeted 512 SAS drive\n"
    "    - Create error injection record for each media error test case\n"
    "    - Set-up edge hook to inject media errors on selected drive\n"
    "    - Enable error injection on selected drive\n"
    "\n"
    "STEP 4: Run Sniff Verify I/O on targeted 512 SAS drive\n"
    "    - Clear sniff verify report and check that pass/error counters are zero\n"
    "    - Set verify checkpoint for specified LBA for I/O\n"
    "    - Set verify enable to start sniff verify I/O\n"
    "    - Wait for sniff verify I/O to complete\n"
    "    - Validate that bad blocks were remapped as appropriate for 512 SAS drives by RAID\n"
    "      and that error counters in sniff verify reports reflect injected media errors\n"
    "    - Validate that the right lba was remapped \n"
    "\n"
    "STEP 5: Disable error injection on targeted 512 SAS drive\n"
    "    - Remove edge hook and disable error injection on selected drive\n"
    "    - Check that error injection statistics are as expected\n"
    "\n"
    "STEP 6: Set-up NEIT to inject media errors on targeted 520 SAS drive\n"
    "    - Create error injection record for each media error test case\n"
    "    - Set-up edge hook to inject media errors on selected drive\n"
    "    - Enable error injection on selected drive\n"
    "\n"
    "STEP 7: Run Sniff Verify I/O on targeted 520 SAS drive\n"
    "    - Clear sniff verify report and check that pass/error counters are zero\n"
    "    - Set verify checkpoint for specified LBA for I/O\n"
    "    - Set verify enable to start sniff verify I/O\n"
    "    - Wait for sniff verify I/O to complete\n"
    "    - Validate that bad blocks were remapped as appropriate for 520 SAS drives by RAID\n"
    "      and that error counters in sniff verify reports reflect injected media errors\n"
    "    - Validate that the right lba was remapped \n"
    "\n"
    "STEP 8: Disable error injection on targeted 520 SAS drive\n"
    "    - Remove edge hook and disable error injection on selected drive\n"
    "\n";


/*!****************************************************************************
 *  dixie_doo_setup
 ******************************************************************************
 *
 * @brief
 *    This is the setup function for the Dixie Doo test. It configures all
 *    objects and attaches them into the topology as follows:
 *
 *    1. Create and verify the initial physical package configuration
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    05/28/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
dixie_doo_setup(void)
{ 
    fbe_test_rg_configuration_t *rg_config_p = &dixie_doo_raid_group_configuration_g[0];
    fbe_u32_t                   num_raid_groups;  

    if (fbe_test_util_is_simulation())
    {  
        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);   
             
        fbe_test_pp_utils_set_default_520_sas_drive(FBE_SAS_DRIVE_SIM_520_FLASH_UNMAP);
        // load the physical package and create the physical configuration
        elmo_create_physical_config_for_rg( rg_config_p, num_raid_groups);
        
        // load the storage extent and neit packages
        mut_printf( MUT_LOG_LOW, "=== Starting Storage Extent and Neit Packages ===\n" );
        sep_config_load_sep_and_neit();

        // set trace level
        elmo_set_trace_level( FBE_TRACE_LEVEL_INFO );
    }

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_set_chunks_per_rebuild(1);
    return;
    
}   // end dixie_doo_setup()



/*!****************************************************************************
 *  dixie_doo_run_on_rg_config()
 ******************************************************************************
 *
 * @brief
 *    This is the main entry point for Dixie Doo Scenario tests. These tests
 *    check that sniff verify can remap bad blocks on consumed extents based
 *    on 512 block and 520 block drives.
 *
 * @param   rg_config
 *
 * @return  void
 *
 * @author
 *    05/28/2010 - Created. Randy Black
 *
 * @note
 *    05/12/2011 - Updated by Maya D. to run on hardware
 *
 *****************************************************************************/

void
dixie_doo_run_on_rg_config( fbe_test_rg_configuration_t *rg_config_p, void * context_p)
              
{
    fbe_object_id_t                 lu_object_id = FBE_OBJECT_ID_INVALID;       // LU object id
    fbe_u32_t						lu_idx;			    // LU index
    dixie_doo_media_error_case_t*   test_case_p = NULL;        // media error test case
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;      // provision drive object id
    fbe_u32_t                       rg_idx;             // RAID group index
    fbe_object_id_t                 *rg_object_id = NULL;       // RAID group object id
    fbe_status_t                    status;             // fbe status
    fbe_u32_t                       test_idx;           // test case index
    fbe_u32_t                       raid_group_count = 0;
    fbe_test_rg_configuration_t*    cur_rg_config_p = NULL;
    fbe_object_id_t rg_object_ids[FBE_TEST_MAX_RAID_GROUP_COUNT] = {FBE_OBJECT_ID_INVALID};
    fbe_api_raid_group_get_info_t raid_group_info;
    dixie_doo_media_error_case_t*   test_case_4k_p = NULL;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Let system verify complete before we proceed, or this verify will steal errors from the sniff verify test.
     * This also waits for zero to complete, so we need to do it before disabling zeroing.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

	/* Disable the background zeroing operation for the Rg's disk set
	 */
	status = fbe_test_sep_util_rg_config_disable_zeroing(rg_config_p);
	MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

	// write a background pattern for configured LUs
	dixie_doo_write_background_pattern( dixie_doo_rdgen_test_context_g, DIXIE_DOO_LUN_ELEMENT_SIZE );

    // run all hard and soft media error tests
    for ( test_idx = 0; test_idx < DIXIE_DOO_MEDIA_ERROR_CASE_MAX; test_idx++ )
    {
        // test media error handling on each RAID group
        for ( rg_idx = 0; rg_idx < raid_group_count; rg_idx++ )
        {
            fbe_test_get_rg_object_ids(rg_config_p, &rg_object_ids[0]);

			// get configuration info associated with this RAID group
			cur_rg_config_p = &rg_config_p[rg_idx];

			if(fbe_test_rg_config_is_enabled(cur_rg_config_p))
            {
                rg_object_id = &rg_object_ids[rg_idx];

                for (lu_idx=0; lu_idx < DIXIE_DOO_LUNS_PER_RAID_GROUP ; lu_idx++)
                {

                    // get base of media error case structure
                    test_case_p = &dixie_doo_media_error_cases_g[test_idx+lu_idx*DIXIE_DOO_MEDIA_ERROR_CASE_MAX];

                    // annouce this media error test
                    mut_printf( MUT_LOG_TEST_STATUS,
                                "Dixie Doo: *** Case %d-%d-%d - %s on %d disks\n",
                                (test_idx + 1), (rg_idx + 1),(lu_idx + 1), test_case_p->description, rg_config_p->block_size
                              );

                    // get id of LU object associated with this RAID group 
                    status = fbe_api_database_lookup_lun_by_number(
                                                                  cur_rg_config_p->logical_unit_configuration_list[lu_idx].lun_number, &lu_object_id );
                    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

                    // get id of a PVD object associated with this RAID group
                    status = fbe_api_provision_drive_get_obj_id_by_location( cur_rg_config_p->rg_disk_set[0].bus,
                                                                             cur_rg_config_p->rg_disk_set[0].enclosure,
                                                                             cur_rg_config_p->rg_disk_set[0].slot,
                                                                             &pvd_object_id
                                                                           );
                    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

#if 0
                    // run specified hard/soft media error test
                    dixie_doo_run_media_error_test( lu_object_id, *rg_object_id, pvd_object_id, test_case_p,
                                                    cur_rg_config_p );
#endif
                }

                // get id of a PVD object associated with this RAID group
                status = fbe_api_provision_drive_get_obj_id_by_location( cur_rg_config_p->rg_disk_set[0].bus,
                                                                         cur_rg_config_p->rg_disk_set[0].enclosure,
                                                                         cur_rg_config_p->rg_disk_set[0].slot,
                                                                         &pvd_object_id);

                fbe_api_raid_group_get_info(*rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);

                if(raid_group_info.write_log_physical_capacity != 0)
                {
                    dixie_doo_run_journal_error_test(pvd_object_id, *rg_object_id);
                }
                else
                {
                    mut_printf( MUT_LOG_TEST_STATUS,
                                "Dixie Doo: Skipping Journal Test journal capacity is Zero on ID:%d, Type:%d \n",
                                *rg_object_id, raid_group_info.raid_type);
                }

                /* --------------------
                   Test 4k boundaries
                */

                // get id of first LU object associated with this RAID group 
                status = fbe_api_database_lookup_lun_by_number(cur_rg_config_p->logical_unit_configuration_list[0].lun_number, &lu_object_id );
                MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

                test_case_4k_p = dixie_doo_4k_boundary_media_error_cases_g;
                while (test_case_4k_p->description != NULL)
                {

                    // get first PVD object associated with this RAID group
                    status = fbe_api_provision_drive_get_obj_id_by_location( cur_rg_config_p->rg_disk_set[0].bus,
                                                                             cur_rg_config_p->rg_disk_set[0].enclosure,
                                                                             cur_rg_config_p->rg_disk_set[0].slot,
                                                                             &pvd_object_id);
                    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );


                    // run specified hard/soft media error test
                    dixie_doo_run_media_error_test( lu_object_id, *rg_object_id, pvd_object_id, test_case_4k_p,
                                                    cur_rg_config_p );

                    test_case_4k_p++;
                }


            } /* rg_config_is_enabled*/

        }
    }

    return;
}   // end dixie_doo_test()


void dixie_doo_test()
{

    fbe_test_rg_configuration_t *rg_config_p = &dixie_doo_raid_group_configuration_g[0];

    /* Now create the raid groups and run the test
     */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, dixie_doo_run_on_rg_config,
                                   DIXIE_DOO_LUNS_PER_RAID_GROUP,
                                   DIXIE_DOO_CHUNKS_PER_LUN);

}


/*!****************************************************************************
 *  dixie_doo_cleanup
 ******************************************************************************
 *
 * @brief
 *    This is the clean-up function for the Dixie Doo test.
 *
 * @param   void
 *
 * @return  void
 *
 * @author
 *    05/28/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
dixie_doo_cleanup(
                   void
                 )
{
    fbe_test_sep_util_restore_sector_trace_level();
    
    if (fbe_test_util_is_simulation())
    fbe_test_sep_util_destroy_neit_sep_physical();

    return;
}   // end dixie_doo_cleanup()


/*!****************************************************************************
 *  dixie_doo_execute_verify_io
 ******************************************************************************
 *
 * @brief
 *    This function starts a verify i/o on the specified provision drive and
 *    then waits for it to complete. After the verify i/o finishes, a verify
 *    report is returned to the caller.
 *
 *    The procedure to execute a verify i/o is:
 *
 *     1. Set the verify checkpoint to the specified start lba
 *     2. Set the provision drive's verify enable to start verify i/o
 *     3. Wait up to 5 seconds for the verify i/o to finish
 *     4. Get the verify report and return it to the caller
 *
 * @param   in_pvd_object_id    -  provision drive object id
 * @param   in_start_lba        -  verify i/o start lba
 * @param   in_block_count      -  verify i/o block count
 * @param   in_verify_report_p  -  pointer to verify report
 *
 * @return  void
 *
 * @author
 *    05/28/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
dixie_doo_execute_verify_io(
                             fbe_object_id_t                      in_pvd_object_id,
                             fbe_lba_t                            in_start_lba,
                             fbe_block_count_t                    in_block_count,
                             fbe_provision_drive_verify_report_t* in_verify_report_p
                           )
{
    fbe_lba_t                                ending_lba;      // verify i/o ending lba
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status

    // determine ending lba for this verify i/o
    ending_lba = in_start_lba + in_block_count;

    // set provision drive's verify checkpoint to specified starting lba
    fbe_test_sep_util_provision_drive_set_verify_checkpoint( in_pvd_object_id, in_start_lba );

    // Set the hook so that we know when the remap event gets queued
    //dixie_doo_set_sniff_hook(in_pvd_object_id);

    // enable verify on this provision drive
    fbe_test_sep_util_provision_drive_enable_verify( in_pvd_object_id );

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

    // check that verify on provision drive is enabled
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_TRUE );

    // announce that verify i/o started on this provision drive
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: sniff verify i/o started at lba: 0x%llx size: 0x%llx on object: 0x%x",
                (unsigned long long)in_start_lba,
		(unsigned long long)in_block_count, in_pvd_object_id
              );

    // Wait for the hook to be hit and then delete it
    //dixie_doo_wait_for_hook(in_pvd_object_id);
    //dixie_doo_delete_sniff_hook(in_pvd_object_id);

    // now, wait for verify i/o to finish on this provision drive
    dixie_doo_wait_for_verify_io_completion( in_pvd_object_id, ending_lba, DIXIE_DOO_SNIFF_VERIFY_IO_WAIT_TIME );

    // get the verify report for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, in_verify_report_p );

    return;
}   // end dixie_doo_execute_verify_io()


/*!****************************************************************************
 *  dixie_doo_run_media_error_test
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
 *    2. Run a Sniff Verify i/o on the selected drive
 *       - Clear verify report and check that pass/error counters are zero
 *       - Set verify checkpoint for specified lba for i/o
 *       - Set verify enable to start sniff verifies
 *       - Wait for a single sniff verify i/o to complete
 *       - Validate that bad blocks were remapped as appropriate for selected
 *         drive by RAID and that error counters in sniff verify report reflect
 *         injected media errors
 *
 *    3. Disable error injection on the selected drive
 *       - Remove edge hook and disable error injection on selected drive
 *       - Check that error injection statistics are as expected
 *
 * @param   in_lu_object_id   -  LU object id
 * @param   in_rg_object_id   -  RAID group object id
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
dixie_doo_run_media_error_test(
                                fbe_object_id_t               in_lu_object_id,
                                fbe_object_id_t               in_rg_object_id,
                                fbe_object_id_t               in_pvd_object_id,
                                dixie_doo_media_error_case_t* in_error_case_p,
                                fbe_test_rg_configuration_t*  rg_config_p
                              )
{
    fbe_u32_t                                           current_count;       // error counts for current pass
    fbe_u32_t                                           expected_count;      // expected error counts
    fbe_u32_t                                           previous_count;      // error counts for previous passes
    fbe_api_logical_error_injection_get_object_stats_t  pvd_stats;           // provision drive error injection statistics
    fbe_api_logical_error_injection_get_stats_t         stats;               // error injection service statistics
    fbe_status_t                                        status;              // fbe status
    fbe_u32_t                                           total_count;         // accumulated total error counts 
    fbe_provision_drive_verify_report_t                 verify_report;       // provision drive verify report

    // stop verify on specified provision drive
    dixie_doo_stop_verify( in_pvd_object_id );

    // clear sniff verify report for specified provision drive
    fbe_test_sep_util_provision_drive_clear_verify_report( in_pvd_object_id );

    // get sniff verify report for specified provision drive
    fbe_test_sep_util_provision_drive_get_verify_report( in_pvd_object_id, &verify_report );

    // check that verify pass count is 0
    MUT_ASSERT_TRUE( verify_report.pass_count == 0 );

    // check current sniff verify result counters; should be zero
    dixie_doo_sum_verify_results( &verify_report.current, &current_count );
    MUT_ASSERT_TRUE( current_count == 0 );

    // check previous sniff verify result counters; should be zero
    dixie_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( previous_count == 0 );

    // check total sniff verify result counters; should be zero
    dixie_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == 0 );

    // clear background verify report for specified LU
    status = fbe_api_lun_clear_verify_report( in_lu_object_id, FBE_PACKET_FLAG_NO_ATTRIB );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // disable any previous error injection records
    status = fbe_api_logical_error_injection_disable_records( 0, 255 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    status = fbe_api_logical_error_injection_create_record( &in_error_case_p->pvd_record );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // enable error injection on specified provision drive
    status = fbe_api_logical_error_injection_enable_object( in_pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // get error injection statistics for this provision drive
    status = fbe_api_logical_error_injection_get_object_stats( &pvd_stats, in_pvd_object_id );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // display error injection statistics 
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: PVD errors injected - reads: %lld, writes: %lld, total: %lld",
                (long long)pvd_stats.num_read_media_errors_injected, (long long)pvd_stats.num_write_verify_blocks_remapped, (long long)pvd_stats.num_errors_injected );

    // enable error injection service
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // check status of enabling error injection service
    status = fbe_api_logical_error_injection_get_stats( &stats );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // check state of logical error injection service, should be enabled 
    MUT_ASSERT_INT_EQUAL( stats.b_enabled, FBE_TRUE );

    // display number of error injections records and objects
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: error injection service: records: %d, objects: %d",
                stats.num_records, stats.num_objects_enabled );

    // check number of error injection records and objects
    MUT_ASSERT_INT_EQUAL( stats.num_records, 1 );
    MUT_ASSERT_INT_EQUAL( stats.num_objects_enabled, 1 );

    // get error injection statistics for this provision drive
    status = fbe_api_logical_error_injection_get_object_stats( &pvd_stats, in_pvd_object_id );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    // display error injection statistics 
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: PVD errors injected - reads: %lld, writes: %lld, total: %lld",
                (long long)pvd_stats.num_read_media_errors_injected, (long long)pvd_stats.num_write_verify_blocks_remapped, (long long)pvd_stats.num_errors_injected );


    if(in_error_case_p->degrade_rg)
    {
        dixie_doo_test_rg_degraded(rg_config_p, in_rg_object_id, in_pvd_object_id, in_error_case_p->sv_lba, in_error_case_p->sv_blocks);
        dixie_doo_stop_verify( in_pvd_object_id );

        // disable error injection on this provision drive
        status = fbe_api_logical_error_injection_disable_object( in_pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
        
        // disable error injection service
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        return;
    }

    if(in_error_case_p->broken_rg)
    {
        dixie_doo_test_rg_broken(rg_config_p, in_rg_object_id, in_pvd_object_id, in_error_case_p->sv_lba, in_error_case_p->sv_blocks);
        dixie_doo_stop_verify( in_pvd_object_id );

        // disable error injection on this provision drive
        status = fbe_api_logical_error_injection_disable_object( in_pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
        
        // disable error injection service
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        return;
    }
        // run sniff verify i/o on specified provision drive
    dixie_doo_execute_verify_io( in_pvd_object_id, in_error_case_p->sv_lba, in_error_case_p->sv_blocks, &verify_report );

    // stop verify on specified provision drive
    dixie_doo_stop_verify( in_pvd_object_id );

    // Delete the sniff hook
    //dixie_doo_delete_sniff_hook(in_pvd_object_id, in_error_case_p->pvd_record.lba);

    // sum expected verify result counters
    dixie_doo_sum_verify_results( &in_error_case_p->expected, &expected_count );

    // check current and previous verify result counters;
    // should be equal to expected verify result counters
    dixie_doo_sum_verify_results( &verify_report.current, &current_count );
    dixie_doo_sum_verify_results( &verify_report.previous, &previous_count );
    MUT_ASSERT_TRUE( (current_count + previous_count) == expected_count );

    // check total verify result counters; should be equal
    // to expected verify result counters
    dixie_doo_sum_verify_results( &verify_report.totals, &total_count );
    MUT_ASSERT_TRUE( total_count == expected_count );

    // wait for verify to complete on specified RAID group
    status = dixie_doo_wait_for_rg_verify_completion( in_rg_object_id, DIXIE_DOO_RG_VERIFY_WAIT_TIME );
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

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


	//check that  pvd statistics is as expected
	if(pvd_stats.num_read_media_errors_injected!=in_error_case_p->read_errors_injected)
	{
        mut_printf(MUT_LOG_TEST_STATUS,
                   "Not enough read errors injected %lld found %d expected", 
                   (long long)pvd_stats.num_read_media_errors_injected, (int)in_error_case_p->read_errors_injected);
        MUT_FAIL_MSG("unexpected number of remaps detected");
    }
     
	if(pvd_stats.num_write_verify_blocks_remapped!=in_error_case_p->write_errors_injected)
	{
        mut_printf(MUT_LOG_TEST_STATUS,
                   "Not enough write veriy blocks remapped %lld found %d expected", 
                   (long long)pvd_stats.num_write_verify_blocks_remapped, (int)in_error_case_p->read_errors_injected);
        MUT_FAIL_MSG("unexpected number of remaps detected");
    }

    // display error injection statistics 
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: PVD errors injected - reads: %llu, writes: %llu, total: %llu",
                (unsigned long long)pvd_stats.num_read_media_errors_injected,
		(unsigned long long)pvd_stats.num_write_verify_blocks_remapped,
		(unsigned long long)pvd_stats.num_errors_injected );


    // display error detection statistics 
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: verify report : errors detected - recovered reads: %d, unrecovered reads: %d",
                verify_report.totals.recov_read_count, verify_report.totals.unrecov_read_count );

    return;
}   // end dixie_doo_run_media_error_test()




/*!****************************************************************************
 *  dixie_doo_run_journal_error_test
 ******************************************************************************
 *
 * @brief
 *    This function inject error in the PVD corresponding to the journal
 *   area of RAID. 
 *
 *
 * @param   in_pvd_object_id  -  provision drive object id
 * @param   in_error_case_p   -  pointer to media error test case
 *
 * @return  void
 *
 * @author
 *    03/20/2012 - Created. Ashwin Tamilarasan
 *
 *****************************************************************************/

void
dixie_doo_run_journal_error_test(
                                fbe_object_id_t               pvd_object_id,
                                fbe_object_id_t               rg_object_id
                              )
{
    fbe_u32_t                    index;																			 
    fbe_status_t                 status; 
    fbe_lba_t                    start_lba;
    fbe_provision_drive_get_verify_status_t      verify_status;
 

    for(index = 0; index <DIXIE_DOO_JOURNAL_VERIFY_RECORDS; index++)
    {
        // stop verify on specified provision drive
        dixie_doo_stop_verify( pvd_object_id );
    
        // disable any previous error injection records
        status = fbe_api_logical_error_injection_disable_records( 0, 255 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
        status = fbe_api_logical_error_injection_create_record( &dixie_doo_journal_verify_test_record[index][0] );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

        status = fbe_api_logical_error_injection_create_record( &dixie_doo_journal_verify_test_record[index][1] );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
        // enable error injection on specified provision drive
        status = fbe_api_logical_error_injection_enable_object( pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
        // enable error injection service
        status = fbe_api_logical_error_injection_enable();
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
        mut_printf(MUT_LOG_LOW, "=== Testing sniff on RG journal area =========\n");
    
        start_lba = dixie_doo_journal_verify_test_record[index][0].lba;
        start_lba = start_lba/0x800*0x800;  /* align to 1MB boundary prior to error injection */
        fbe_test_sep_util_provision_drive_set_verify_checkpoint(pvd_object_id, start_lba );
    
        if(dixie_doo_journal_verify_test_record[index][0].blocks > 1)
        {
             // Set the hook so that we know when the remap event gets queued
            dixie_doo_set_sniff_hook(rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
                                 FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR );

        }
         // Set the hook so that we know when the remap event gets queued
        dixie_doo_set_sniff_hook(rg_object_id,SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
                                 FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED );
        
        fbe_test_sep_util_provision_drive_enable_verify( pvd_object_id );
        fbe_test_sep_util_provision_drive_get_verify_status( pvd_object_id, &verify_status );
        MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_TRUE );
    
        // Wait for the hook to be hit and then delete it
        dixie_doo_wait_for_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
                                FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED);
    
        dixie_doo_delete_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
                                    FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED);

        if(dixie_doo_journal_verify_test_record[index][0].blocks > 1)
        {
             // Wait for the hook to be hit and then delete it
            dixie_doo_wait_for_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
                                FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR);
    
            dixie_doo_delete_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY,
                                    FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR);

        }
    
        // wait for verify to complete on specified RAID group
        status = dixie_doo_wait_for_rg_verify_completion( rg_object_id, DIXIE_DOO_RG_VERIFY_WAIT_TIME );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    
        // disable error injection on this provision drive
        status = fbe_api_logical_error_injection_disable_object( pvd_object_id, FBE_PACKAGE_ID_SEP_0 );
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
        
        // disable error injection service
        status = fbe_api_logical_error_injection_disable();
        MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );

    }
    

    return;
}   // end dixie_doo_run_media_error_test()

/*!****************************************************************************
 *  dixie_doo_stop_verify
 ******************************************************************************
 *
 * @brief
 *    This function stops verify i/o on the specified provision drive.
 *
 * @param   in_pvd_object_id  -  provision drive object id
 *
 * @return  void
 *
 * @author
 *    04/28/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
dixie_doo_stop_verify(
                       fbe_object_id_t in_pvd_object_id
                     )
{
    fbe_provision_drive_get_verify_status_t  verify_status;   // verify status

    // disable verify i/o on this provision drive
    fbe_test_sep_util_provision_drive_disable_verify( in_pvd_object_id );

    // get verify status for this provision drive
    fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

    // check that verify i/o on provision drive is disabled
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_FALSE );
    
    // verify i/o stopped for this provision drive
    mut_printf( MUT_LOG_TEST_STATUS, "Dixie Doo: sniff verify i/o stopped on object: 0x%x", in_pvd_object_id );

    return;
}   // end dixie_doo_stop_verify()


/*!****************************************************************************
 *  dixie_doo_sum_verify_results
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
 *    05/28/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
dixie_doo_sum_verify_results(
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
}   // end dixie_doo_sum_verify_results()


/*!****************************************************************************
 *  dixie_doo_wait_for_rg_verify_completion
 ******************************************************************************
 *
 * @brief
 *    This function waits the specified timeout interval for verify to complete
 *    on a RAID group.   Note that the RAID group is polled every 100 ms during
 *    the timeout interval for a completed verify.
 *
 * @param   in_rg_object_id  -  RAID group object to check for completed verify
 * @param   in_timeout_sec   -  RAID group verify timeout interval (in seconds)
 *
 * @return  fbe_status_t     -  FBE_STATUS_OK
 *                              FBE_STATUS_TIMEOUT
 *
 * @author
 *    06/14/2010 - Created. Randy Black
 *
 *****************************************************************************/

fbe_status_t
dixie_doo_wait_for_rg_verify_completion(
                                         fbe_object_id_t  in_rg_object_id,
                                         fbe_u32_t        in_timeout_sec 
                                       )
{
    fbe_u32_t                       elapsed_time_msec=0;     // elapsed time
    fbe_api_raid_group_get_info_t   rg_info;            // RAID group info
    fbe_status_t                    status;             // fbe status
	fbe_u32_t in_timeout_msec = in_timeout_sec * FBE_TIME_MILLISECONDS_PER_SECOND;
    
    // wait specified timeout interval for RAID group verify to complete
	while(elapsed_time_msec < in_timeout_msec)
	{
        // get info for specified RAID group
        status = fbe_api_raid_group_get_info( in_rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB );

        // unable to retrieve RAID group info; return error status
        if ( status != FBE_STATUS_OK )
        {
            return status;
        }

		if ( (rg_info.error_verify_checkpoint == FBE_LBA_INVALID) && 
             (rg_info.journal_verify_checkpoint == FBE_LBA_INVALID) &&
             (rg_info.b_is_event_q_empty == FBE_TRUE) )
        {
            return FBE_STATUS_OK;
        }
       
        // wait for next poll
        fbe_api_sleep( DIXIE_DOO_POLLING_INTERVAL );
		elapsed_time_msec += DIXIE_DOO_POLLING_INTERVAL;
    }

    // RAID group verify failed to complete
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: RAID group verify failed to complete in %d seconds for object: 0x%x", 
                in_timeout_sec, in_rg_object_id
              );

    // RAID group verify failed to complete, return error status
    return FBE_STATUS_TIMEOUT;

}   //end dixie_doo_wait_for_rg_verify_completion()


/*!****************************************************************************
 *  dixie_doo_wait_for_verify_io_completion
 ******************************************************************************
 *
 * @brief
 *    This function waits the specified timeout interval for a verify i/o to
 *    complete on a provision drive. Note that the provision drive is polled
 *    every 100 ms during the timeout interval for a completed verify i/o.
 *
 * @param   in_pvd_object_id  -  provision drive to check for a verify i/o
 * @param   in_end_lba        -  verify i/o end lba
 * @param   in_timeout_sec    -  verify i/o timeout interval (in seconds)
 *
 * @return  void
 *
 * @author
 *    05/28/2010 - Created. Randy Black
 *
 *****************************************************************************/

void
dixie_doo_wait_for_verify_io_completion(
                                         fbe_object_id_t  in_pvd_object_id,
                                         fbe_lba_t        in_end_lba,
                                         fbe_u32_t        in_timeout_sec 
                                       )
{
    fbe_u32_t                       elapsed_time_msec=0;     // elapsed time
    fbe_provision_drive_get_verify_status_t verify_status;  // verify status
	fbe_u32_t in_timeout_msec = in_timeout_sec * FBE_TIME_MILLISECONDS_PER_SECOND;
    
    // wait specified timeout interval for a verify i/o to complete
    while(elapsed_time_msec < in_timeout_msec)
    {
        // get verify status for this provision drive
        fbe_test_sep_util_provision_drive_get_verify_status( in_pvd_object_id, &verify_status );

        mut_printf( MUT_LOG_TEST_STATUS,
                    "Dixie Doo: sniff verify checkpoint 0x%llx, i/o ending lba: 0x%llx\n",
                    (unsigned long long)verify_status.verify_checkpoint,
		    (unsigned long long)in_end_lba
                  );

        // finished when verify i/o completes
        if ( verify_status.verify_checkpoint >= in_end_lba )
        {
            // wait extra poll cycle for verify report to get updated
            fbe_api_sleep( DIXIE_DOO_POLLING_INTERVAL );

            // verify i/o completed for provision drive
            mut_printf( MUT_LOG_TEST_STATUS,
                        "Dixie Doo: sniff verify i/o completed for object: 0x%x",
                        in_pvd_object_id
                      );

            return;
        }

        // wait for next poll
        fbe_api_sleep( DIXIE_DOO_POLLING_INTERVAL );
		elapsed_time_msec += DIXIE_DOO_POLLING_INTERVAL;
    }

    // verify i/o failed to complete for provision drive
    mut_printf( MUT_LOG_TEST_STATUS,
                "Dixie Doo: sniff verify i/o failed to complete in %d seconds for object: 0x%x", 
                in_timeout_sec, in_pvd_object_id
              );
    MUT_FAIL_MSG("sniff verify i/o failed to complete");

    return;
}   //end dixie_doo_wait_for_verify_io_completion()


/*!****************************************************************************
 *  dixie_doo_write_background_pattern
 ******************************************************************************
 *
 * @brief
 *    This function writes a background pattern to all LUs in configuration.
 *
 * @param   in_test_context_p  -  rdgen test context containing one entry for
 *                                each LU in the current configuration
 * @param   in_element_size    -  element size
 *
 * @return  void
 *
 * @author
 *    06/08/2010 - Created. Randy Black
 *
 *****************************************************************************/
void
dixie_doo_write_background_pattern(
                                    fbe_api_rdgen_context_t* in_test_context_p,
                                    fbe_u32_t                in_element_size
                                  )
{
    fbe_status_t                    status;             // fbe status

    // set-up rdgen context to do a write (fill) operation
    fbe_test_sep_io_setup_fill_rdgen_test_context( in_test_context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             FBE_LBA_INVALID,
                                             in_element_size
                                           );

    // now, write a background pattern to all LUs in configuration 
    status = fbe_api_rdgen_test_context_run_all_tests( in_test_context_p, FBE_PACKAGE_ID_NEIT, 1 );

    // check that no errors occurred
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    MUT_ASSERT_INT_EQUAL( in_test_context_p->start_io.statistics.error_count, 0 );

    // set-up rdgen context to do a read check operation
    fbe_test_sep_io_setup_fill_rdgen_test_context( in_test_context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_LBA_INVALID,
                                             in_element_size
                                           );

    // now, read back pattern and check it for all LUs in configuration 
    status = fbe_api_rdgen_test_context_run_all_tests( in_test_context_p, FBE_PACKAGE_ID_NEIT, 1 );

    // check that no errors occurred
    MUT_ASSERT_INT_EQUAL( status, FBE_STATUS_OK );
    MUT_ASSERT_INT_EQUAL( in_test_context_p->start_io.statistics.error_count, 0 );

    return;
}   //end dixie_doo_write_background_pattern()


/*!****************************************************************************
 *  dixie_doo_test_rg_degraded
 ******************************************************************************
 *
 * @brief
 *    This function tests the raid group degraded case where the event must be 
 *    completed back to the pvd.
 *
 * @param   in_pvd_object_id  -  provision drive object id
 *
 * @return  void
 *
 * @author
 *    03/12/2012 - Created. Ashwin Tamilarasan
 *
 *****************************************************************************/

void dixie_doo_test_rg_degraded(fbe_test_rg_configuration_t*  rg_config_p,
                              fbe_object_id_t    rg_object_id,
                              fbe_object_id_t    pvd_object_id,
                              fbe_lba_t       start_lba,
                              fbe_block_count_t in_block_count)
{

    fbe_u32_t           				number_physical_objects = 0;
	fbe_api_terminator_device_handle_t  drive_info;
    fbe_status_t                        status;
    fbe_provision_drive_get_verify_status_t  verify_status;
    fbe_lba_t                                ending_lba;      // verify i/o ending lba
    

    mut_printf(MUT_LOG_LOW, "=== Testing sniff on degraded RG=========\n");

    // determine ending lba for this verify i/o
    ending_lba = start_lba + in_block_count;

    fbe_test_sep_util_provision_drive_set_verify_checkpoint(pvd_object_id, start_lba );

     // Set the hook so that we know when the remap event gets queued
    dixie_doo_set_sniff_hook(rg_object_id,SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                             FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED );
    dixie_doo_set_sniff_hook(pvd_object_id,SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                             FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED );
    

    fbe_test_sep_util_provision_drive_enable_verify( pvd_object_id );
    fbe_test_sep_util_provision_drive_get_verify_status( pvd_object_id, &verify_status );
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_TRUE );

    // Wait for the hook to be hit and then delete it
    dixie_doo_wait_for_hook(rg_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED);
    

    mut_printf(MUT_LOG_LOW, "=== Removing the 1st position in the raid group\n");
    /* need to remove a drive here */
	status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
	sep_rebuild_utils_remove_drive_and_verify(rg_config_p, DIXIE_DOO_FIRST_POSITION_TO_REMOVE,
                                              number_physical_objects, &drive_info);


    // Wait for the hook to be hit and then delete it
    dixie_doo_wait_for_hook(pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED);

    // Delete both the hooks
    dixie_doo_delete_sniff_hook(pvd_object_id,SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED );

    dixie_doo_delete_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED);
    
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, DIXIE_DOO_FIRST_POSITION_TO_REMOVE,
                                              number_physical_objects+1, &drive_info);

    // now, wait for verify i/o to finish on this provision drive
    dixie_doo_wait_for_verify_io_completion( pvd_object_id, ending_lba, DIXIE_DOO_SNIFF_VERIFY_IO_WAIT_TIME );
    
    return;
}   // end dixie_doo_test_rg_degraded()


/*!****************************************************************************
 *  dixie_doo_test_rg_broken
 ******************************************************************************
 *
 * @brief
 *    This function tests the raid group broken case where the event must be 
 *    completed back to the pvd.
 *
 * @param   in_pvd_object_id  -  provision drive object id
 *
 * @return  void
 *
 * @author
 *    03/12/2012 - Created. Ashwin Tamilarasan
 *
 *****************************************************************************/

void dixie_doo_test_rg_broken(fbe_test_rg_configuration_t*  rg_config_p,
                              fbe_object_id_t    rg_object_id,
                              fbe_object_id_t    pvd_object_id,
                              fbe_lba_t       start_lba,
                              fbe_block_count_t in_block_count)
{

    fbe_u32_t           				number_physical_objects = 0;
	fbe_api_terminator_device_handle_t  drive_info[2];
    fbe_status_t                        status;
    fbe_provision_drive_get_verify_status_t  verify_status;
    fbe_lba_t                                ending_lba;      // verify i/o ending lba


    mut_printf(MUT_LOG_LOW, "=== Testing sniff on broken RG=========\n");

    // determine ending lba for this verify i/o
    ending_lba = start_lba + in_block_count;

    fbe_test_sep_util_provision_drive_set_verify_checkpoint(pvd_object_id, start_lba );

     // Set the hook so that we know when the remap event gets queued
    dixie_doo_set_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                             FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED );

    // This hook prevents the rg from marking Rebuild logging
    dixie_doo_set_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                              FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST );

    // Hook to make sure we went through pending function and emptied the event queue
    dixie_doo_set_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                              FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN );

    dixie_doo_set_sniff_hook(pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                              FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED );

    fbe_test_sep_util_provision_drive_enable_verify( pvd_object_id );
    fbe_test_sep_util_provision_drive_get_verify_status( pvd_object_id, &verify_status );
    MUT_ASSERT_TRUE( verify_status.verify_enable == FBE_TRUE );

    // Wait for the hook to be hit and then delete it
    dixie_doo_wait_for_hook(rg_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED);
    

    mut_printf(MUT_LOG_LOW, "=== Removing the 1st position in the raid group\n");
    /* need to remove a drive here */
	status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
	sep_rebuild_utils_remove_drive_and_verify(rg_config_p, DIXIE_DOO_FIRST_POSITION_TO_REMOVE,
                                              number_physical_objects, &drive_info[0]);

    mut_printf(MUT_LOG_LOW, "=== Removing the 2nd position in the raid group\n");
    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_remove_drive_and_verify(rg_config_p, DIXIE_DOO_SECOND_POSITION_TO_REMOVE,
                                              number_physical_objects, &drive_info[1]);

    dixie_doo_delete_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                              FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST );

    // Wait for the hook to be hit and then delete it
    dixie_doo_wait_for_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                              FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN );

    // Wait for the hook to be hit and then delete it
    dixie_doo_wait_for_hook(pvd_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED);

    // Delete the hooks
    dixie_doo_delete_sniff_hook(pvd_object_id,SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED );

    dixie_doo_delete_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED);


    dixie_doo_delete_sniff_hook(rg_object_id, SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                              FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN );

    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, DIXIE_DOO_SECOND_POSITION_TO_REMOVE,
                                              number_physical_objects+1, &drive_info[1]);

    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, DIXIE_DOO_FIRST_POSITION_TO_REMOVE,
                                              number_physical_objects+1, &drive_info[0]);

    // now, wait for verify i/o to finish on this provision drive
    dixie_doo_wait_for_verify_io_completion( pvd_object_id, ending_lba, DIXIE_DOO_SNIFF_VERIFY_IO_WAIT_TIME );

    return;
}   // end dixie_doo_test_rg_broken()


/*!**************************************************************
 * dixie_doo_set_sniff_hook()
 ****************************************************************
 * @brief
 *  Set a sniff verify hook for pvd object id
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for      
 *
 * @return  None
 *
 * @author
 *  03/6/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
void dixie_doo_set_sniff_hook(fbe_object_id_t object_id,
                              fbe_scheduler_hook_state_t    hook_state,
                              fbe_scheduler_hook_substate_t hook_substate) 
{
	fbe_status_t						status = FBE_STATUS_OK;

	status = fbe_api_scheduler_add_debug_hook(object_id,
                                              hook_state,
                                              hook_substate,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end dixie_doo_set_sniff_hook()
 ******************************************/

/*!**************************************************************
 * dixie_doo_get_sniff_hook()
 ****************************************************************
 * @brief
 *  Check if the sniff hook counter was incremented
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for   
 *          was_hit - flag to check if the hook was hit or not hit      
 *
 * @return  None
 *
 * @author
 *  03/6/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
void dixie_doo_get_sniff_hook(fbe_object_id_t object_id,
                              fbe_scheduler_hook_state_t    hook_state,
                              fbe_scheduler_hook_substate_t hook_substate,
                              fbe_bool_t        was_hit) 
{
	fbe_scheduler_debug_hook_t          hook;
	fbe_status_t						status = FBE_STATUS_OK;

	mut_printf(MUT_LOG_TEST_STATUS, "Getting sniff verify Debug Hook OBJ %d\n", object_id);

	hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_STATES;
	hook.monitor_state = hook_state;
	hook.monitor_substate = hook_substate;
	hook.val1 = NULL;
	hook.val2 = NULL;
	hook.counter = 0;

	status = fbe_api_scheduler_get_debug_hook(&hook);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %d times\n", (int)hook.counter);

    if (was_hit) {
        MUT_ASSERT_UINT64_NOT_EQUAL(0, hook.counter);
    } else {
        MUT_ASSERT_UINT64_EQUAL(0, hook.counter);
    }
}
/******************************************
 * end dixie_doo_get_sniff_hook()
 ******************************************/

/*!**************************************************************
 * dixie_doo_delete_sniff_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for         
 *
 * @return  None
 *
 * @author
 *  03/6/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
void dixie_doo_delete_sniff_hook(fbe_object_id_t object_id,
                                 fbe_scheduler_hook_state_t    hook_state,
                                 fbe_scheduler_hook_substate_t hook_substate) 
{
	fbe_status_t status = FBE_STATUS_OK;

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook OBJ %d\n", object_id);

	status = fbe_api_scheduler_del_debug_hook(object_id,
                                              hook_state,
                                              hook_substate,
                                              NULL,
                                              NULL,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end dixie_doo_delete_sniff_hook()
 ******************************************/


/*!**************************************************************
 * kit_cloudkicker_wait_for_hook()
 ****************************************************************
 * @brief
 *  Wait for the hook to be hit
 *
 * @param   object_id - pvd object id to check hook for
 *          lba_checkpoint - the checkpoint to wait for         
 *
 * @return  None
 * 
 *
 ****************************************************************/
void dixie_doo_wait_for_hook(fbe_object_id_t object_id,
                             fbe_scheduler_hook_state_t    hook_state,
                             fbe_scheduler_hook_substate_t hook_substate) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;

    hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_STATES;
	hook.monitor_state = hook_state;
	hook.monitor_substate = hook_substate;
	hook.val1 = NULL;
	hook.val2 = NULL;
	hook.counter = 0;

    status = sep_zeroing_utils_wait_for_hook(&hook, DIXIE_DOO_SNIFF_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end kit_cloudkicker_wait_for_hook()
 ******************************************/


