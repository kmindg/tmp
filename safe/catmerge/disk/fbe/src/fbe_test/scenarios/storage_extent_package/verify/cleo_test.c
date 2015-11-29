/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file cleo_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains test for verify for system power failure during
 *  cached writes
 *
 *
 ***************************************************************************/


#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"

char * cleo_short_desc = "test automatic partial verify of outstanding cached writes";
char * cleo_long_desc =
    "\n"
    "\n"
    "The Cleo Scenario tests that when a RAID group comes back from a shutdown condition, it\n"
    "verifies only the areas  that had write requests outstanding when it shutdown.\n"
    "\n"
    "Dependencies:\n"
    "    - Persistent meta-data storage.\n"
    "    - The raid object must support verify-before write operations and any required\n"
    "      verify meta-data.\n"
    "    - The raid object must support shutdown handling with writes in progress.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 5 SAS drives (PDO)\n"
    "    [PP] 5 logical drives (LD)\n"
    "    [SEP] 5 provision drives (PD)\n"
    "    [SEP] 5 virtual drives (VD)\n"
    "    [SEP] 1 raid object (RAID)\n"
    "    [SEP] 3 lun objects (LU)\n"
    "\n"
    "STEP 1: Bring up the initial topology.\n"
    "    - Create and verify the initial physical package config.\n"
    "    - Create all provision drives (PD) with an I/O edge attched to a logical drive (LD).\n"
    "    - Create all virtual drives (VD) with an I/O edge attached to a PD.\n"
    "    - Create a raid object with I/O edges attached to all VDs.\n"
    "    - Create each lun object with an I/O edge attached to the RAID.\n"
    "    - Verify that all configured objects are in the READY state.\n"
    "\n"
    "STEP 2: Shutdown RAID group during cached write I/O\n"
    "    - Start cached write I/O to all LUN objects\n"
    "    - Fail all the PVDs \n"
    "    - Verify that the RAID object goes to the ACTIVATE state\n"
    "    - Verify that the chunks that had cached writes outstanding are marked as\n"
    "    'needing to be verified before write'.\n"
    "\n"
    "STEP 3: Reenable RAID group after failed cached write I/O\n"
    "    - Re-enable all the PVDs\n"
    "    - Verify that the RAID object goes to the READY state\n"
    "    - Verify that the RAID object verifies only the chunks that are marked as\n"
    "    'needing to be verified before write'\n"
    " \n"
    "STEP 4: Shutdown RAID group during non-cached write I/O\n"
    "    - Start non-cached write I/O to all LUN objects\n"
    "    - Fail all the PVDs \n"
    "    - Verify that the RAID object goes to the ACTIVATE state\n"
    "    - Verify that the chunks having non-cached writes outstanding are marked as\n"
    "    'verify for error'\n"
    "\n"
    "STEP 5: Reenable RAID group after failed non-cached write I/O\n"
    "    - Re-enable all the PVDs\n"
    "    - Verify that the RAID object goes to the READY state\n"
    "    - Verify that the RAID object verifies the chunks that are marked as\n"
    "    'verify for error'\n"
   
"Outstanding issues: 1) More raid types need to be added.\n\
                     2) There is a todo in cleo_run_tests for automatic verify.\n\
                     3) Instead of writing background pattern, use zeroing\n\
                     4) I do not think step 4 and 5 belongs to this test. this is a cached write test\n\
\n\
Description last updated: 10/28/2011.\n";

/*!*******************************************************************
 * @def cleo_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define CLEO_MAX_WIDTH 16

/*!*******************************************************************
 * @def cleo_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Number of raid groups we will test with.
 *
 *********************************************************************/
#define CLEO_RAID_GROUP_COUNT   5

/*!*******************************************************************
 * @def cleo_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define CLEO_LUNS_PER_RAID_GROUP    1

/*!*******************************************************************
 * @def cleo_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of the LUNs.
 *********************************************************************/
#define CLEO_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def CLEO_MAX_ERROR_INJECT_WAIT_TIME_SECS
 *********************************************************************
 * @brief Maximum time in seconds to wait for all error injections
 *********************************************************************/
#define CLEO_MAX_ERROR_INJECT_WAIT_TIME_SECS  30  

/*!*******************************************************************
 * @def BIG_BIRD_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define CLEO_MAX_IO_SIZE_BLOCKS (CLEO_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @def CLEO_FIRST_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The first array position to remove a drive for.  This should
 *        not be the position to inject the error on!!
 *
 *********************************************************************/
#define CLEO_FIRST_POSITION_TO_REMOVE   0

/*!*******************************************************************
 * @def CLEO_SECOND_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define CLEO_SECOND_POSITION_TO_REMOVE  1

/*!*******************************************************************
 * @def CLEO_THIRD_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 2nd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define CLEO_THIRD_POSITION_TO_REMOVE  2

/*!*******************************************************************
 * @def CLEO_PARITY_POSITION_TO_INJECT
 *********************************************************************
 * @brief The array position to inject the error on for parity units
 *
 *********************************************************************/
#define CLEO_PARITY_POSITION_TO_INJECT  2

/*!*******************************************************************
 * @def CLEO_MIRROR_POSITION_TO_INJECT
 *********************************************************************
 * @brief The array position to inject the error on for mirror units
 *
 *********************************************************************/
#define CLEO_MIRROR_POSITION_TO_INJECT  0

/*!*******************************************************************
 * @def CLEO_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define CLEO_WAIT_MSEC 30000

#define CLEO_CHUNKS_PER_LUN  1


/*!*******************************************************************
 * @var cleo_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t cleo_raid_group_config[] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},
    {2,         0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    {5,         0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {4,         0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
    {6,         0xE000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            4,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @struct cleo_error_case_t
 *********************************************************************
 * @brief This is a single test case for this test.
 *        Bascially a single error is inject to the first data position
 *        on a `write only' request.
 *
 *********************************************************************/
typedef struct cleo_error_case_s
{
    fbe_lba_t lba_to_read;
    fbe_block_count_t blocks_to_read;
    fbe_bool_t b_correctable;

    /*! Error to inject.
     */
    fbe_api_logical_error_injection_record_t record;
}
cleo_error_case_t;

cleo_error_case_t cleo_error_cases[CLEO_RAID_GROUP_COUNT] = 
{
    /* R5 2+1 */
    {
        0, /* host lba */
        1, /* blocks */
        FBE_TRUE, /* correctable */
        /* First data position for 2 + 1 parity is 2 (i.e. a bitmask of 0x0100: 4) */
        {   (1 << CLEO_PARITY_POSITION_TO_INJECT), 0x11, 0, 0xE000, 
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
            0x0, CLEO_LUNS_PER_RAID_GROUP, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /* Only inject on write opcodes*/
        }
    },
    {
        0, /* host lba */
        1, /* blocks */
        FBE_TRUE, /* correctable */
        /* Since we read from the first data position for mirrors inject it there (0x1) */
        {   (1 << CLEO_MIRROR_POSITION_TO_INJECT), 0x10, 0, 0xE000,
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_ALWAYS,
            0x0, CLEO_LUNS_PER_RAID_GROUP, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /* Only inject on write opcodes*/
        }
    },
    /* R5 4+1 */
    {
        0, /* host lba */
        1, /* blocks */
        FBE_TRUE, /* correctable */
        /* First data position for 4+1 */
        {   (1 << 4), 0x11, 0, 0xE000, 
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP,
            0x0, CLEO_LUNS_PER_RAID_GROUP, 0x1, CLEO_LUNS_PER_RAID_GROUP, 0x0, 0x0, 0x0, 0x0, 0x0,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /* Only inject on write opcodes*/
        }
    },
    /* R6 2+2 */
    {
        0, /* host lba */
        1, /* blocks */
        FBE_TRUE, /* correctable */
        /* First data position for 2+2 */
        {   (1 << 3), 0x11, 0, 0xE000, 
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP,
            0x0, CLEO_LUNS_PER_RAID_GROUP, 0x1, CLEO_LUNS_PER_RAID_GROUP, 0x0, 0x0, 0x0, 0x0, 0x0,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /* Only inject on write opcodes*/
        }
    },
    /* R6 4+2 */
    {
        0, /* host lba */
        1, /* blocks */
        FBE_TRUE, /* correctable */
        /* First data position for 4+2 */
        {   (1 << 5), 0x11, 0, 0xE000, 
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_TIMEOUT_ERROR, FBE_API_LOGICAL_ERROR_INJECTION_MODE_SKIP,
            0x0, CLEO_LUNS_PER_RAID_GROUP, 0x1, CLEO_LUNS_PER_RAID_GROUP, 0x0, 0x0, 0x0, 0x0, 0x0,
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /* Only inject on write opcodes*/
        }
    },
};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/


static void cleo_manual_verify_write_test(fbe_u32_t index, fbe_test_rg_configuration_t * in_current_rg_config_p);

void cleo_write_background_pattern(fbe_u32_t index, fbe_test_rg_configuration_t * in_current_rg_config_p);

static void cleo_start_io(fbe_u32_t rg_index, fbe_test_rg_configuration_t * in_current_rg_config_p);

static void cleo_wait_for_error_injection(fbe_u32_t rg_index,
                                          fbe_test_rg_configuration_t * in_current_rg_config_p);

static void cleo_start_verify_write_io(fbe_u32_t rg_index,
                                       fbe_test_rg_configuration_t* in_current_rg_config_p);

void cleo_inject_error_record(fbe_u32_t rg_index,
                               fbe_test_rg_configuration_t* in_current_rg_config_p);

static void cleo_remove_and_reinsert_drives(fbe_u32_t rg_index, fbe_test_rg_configuration_t* in_current_rg_config_p);

static void cleo_remove_drives(fbe_test_rg_configuration_t* in_rg_config_p);

static void cleo_wait_for_io_to_complete(fbe_u32_t rg_index);
static void cleo_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);


/*!*******************************************************************
 * @var fbe_cleo_test_context_g
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t fbe_cleo_test_context_g[CLEO_RAID_GROUP_COUNT][CLEO_LUNS_PER_RAID_GROUP];

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/


/*!**************************************************************
 * cleo_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the cleo test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void cleo_setup(void)
{        
    mut_printf(MUT_LOG_LOW, "=== cleo setup ===\n");	

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;

        rg_config_p = &cleo_raid_group_config[0];
        
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        
        /* Load the physical package and create the physical configuration. 
         */
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* Load the logical packages. 
         */
        sep_config_load_sep_and_neit();

        /* set the trace level */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;

}   // End cleo_setup()

/*!****************************************************************************
 * cleo_cleanup()
 ******************************************************************************
 * @brief
 *  Cleanup function.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/

void cleo_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * cleo_test()
 ******************************************************************************
 * @brief
 *  Run lun zeroing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/06/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void cleo_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &cleo_raid_group_config[0];

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,cleo_run_tests,
                                   CLEO_LUNS_PER_RAID_GROUP,
                                   CLEO_CHUNKS_PER_LUN);
    return;    

}

/*!**************************************************************
 * cleo_test()
 ****************************************************************
 * @brief
 *  This is the entry point into the cleo test scenario.
 *
 * @param void
 *
 * @return void
 *
 ****************************************************************/
void cleo_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
   fbe_u32_t                   index;
   fbe_test_rg_configuration_t *rg_config_p = NULL;
   fbe_u32_t                   num_raid_groups = 0;


   num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

   mut_printf(MUT_LOG_LOW, "=== Cleo test start ===\n");

    // Perform the write verify test on all raid groups.
    for (index = 0; index < num_raid_groups; index++)
    {
        rg_config_p = rg_config_ptr + index;

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "cleo verify test raid type: %d of %d.\n", 
                        index+1, CLEO_RAID_GROUP_COUNT);

            /* There are now (2) types of tests:
             *  o Manual Verify Write - Simply inject an error on a write request
             *    and then use verify write to detect and correct error
             *  o Automatic Verify Write - Inject errors on writes, disable 
             *    rebuilds, pull enclosure, re-insert enclosure.  Then confirm
             *    LUN issues a verify write.
             */
            cleo_manual_verify_write_test(index, rg_config_p);

            /*! @todo Automatic Verify Write isn't implemented yet
            cleo_automatic_verify_write_test(index, rg_config_p);
            */

            
         }

    }

    return;

}   // End cleo_test()


/*!**************************************************************************
 * cleo_manual_verify_write_test
 ***************************************************************************
 * @brief
 *  This function performs the write verify test. It does the following
 *  on a raid group:
 *  - Writes an initial data pattern.
 *  - Writes correctable and uncorrectable CRC error patterns on two VDs.
 *  - Performs a write verify operation.
 *  - Validates the error counts in the verify report.
 *  - Performs another write verify operation.
 *  - Validates that there are no new error counts in the verify report.
 * 
 * @param rg_index 
 * @param in_current_rg_config_p - Pointer to the raid group config info
 *
 * @return void
 *
 ***************************************************************************/
static void cleo_manual_verify_write_test(fbe_u32_t rg_index, fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t rg_object_id;
    fbe_u32_t       lun_index;
    fbe_test_logical_unit_configuration_t * logical_unit_configuration_p = NULL;

    // Wait for all LUN's to reach background operation state.
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < CLEO_LUNS_PER_RAID_GROUP; lun_index++)
    {
        fbe_object_id_t lun_object_id;
        fbe_lun_verify_report_t verify_report_p;

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, &verify_report_p, 1);
        logical_unit_configuration_p++;
    }

    /* Let system verify complete before we proceed. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);    
    fbe_test_verify_wait_for_verify_completion(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_SYSTEM);

    if (in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_id, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    /*! @todo Until ZOD is working we need to pre-write a background pattern
     */
    cleo_write_background_pattern(rg_index, in_current_rg_config_p);

    cleo_inject_error_record(rg_index, in_current_rg_config_p);

    // Disable the recovery queue so that hot spare does not get swap-in.    
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    // Send a single write request per LUN, per raid group
    cleo_start_io(rg_index, in_current_rg_config_p);

    cleo_wait_for_error_injection(rg_index, in_current_rg_config_p);
        
    cleo_remove_and_reinsert_drives(rg_index, in_current_rg_config_p);

    /* Restore the sector trace level to it's default.
     */
    fbe_test_sep_util_restore_sector_trace_level();

    // Issue (1) verify-write to detect and validate error is corrected
    cleo_start_verify_write_io(rg_index, in_current_rg_config_p);

    // Enable the recovery queue which allows pending hot spare swap-in job to run.    
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);

    if (in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        status = fbe_api_raid_group_set_mirror_prefered_position(rg_object_id, 0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;

}   // End cleo_raid5_verify_test()


/*!**************************************************************************
 * cleo_write_background_pattern
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the LUNs in the RAID group.
 * 
 * @param rg_index - raid grouip index to write background pattern to
 * @param in_current_rg_config_p - pointer to raid group config information
 *
 * @return void
 *
 ***************************************************************************/
void cleo_write_background_pattern(fbe_u32_t rg_index, fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_api_rdgen_context_t *                   context_p = fbe_cleo_test_context_g[rg_index];        
        

        /* Fill each LUN in the raid group will a fixed pattern
         */
        fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                      FBE_OBJECT_ID_INVALID,
                                                      FBE_CLASS_ID_LUN,
                                                      FBE_RDGEN_OPERATION_ZERO_ONLY,
                                                      FBE_LBA_INVALID, /* use max capacity */
                                                      CLEO_ELEMENT_SIZE);

     

    /* Run our I/O test.
     */
    context_p = fbe_cleo_test_context_g[rg_index];
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    mut_printf(MUT_LOG_TEST_STATUS, "cleo background pattern write for object id: 0x%x complete. passes: 0x%x io_count: 0x%x error_count: 0x%x\n",
               context_p->object_id,
               context_p->start_io.statistics.pass_count,
               context_p->start_io.statistics.io_count,
               context_p->start_io.statistics.error_count);
    return;

}   // End cleo_write_background_pattern()


/*!**************************************************************
 * cleo_inject_error_record()
 ****************************************************************
 * @brief
 *  Injects the error record
 *
 * @param rg_index - Index of raid group to inject error for
 * @param in_current_rg_config_p - pointer to raid group config information               
 *
 * @return None.
 *
 * @author
 *  03/05/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

void cleo_inject_error_record(fbe_u32_t rg_index,
                              fbe_test_rg_configuration_t* in_current_rg_config_p)
                              
{
    fbe_status_t                                status;
    fbe_object_id_t                             rg_object_id;
    fbe_api_logical_error_injection_get_stats_t stats;
    cleo_error_case_t *                         error_case_p = &cleo_error_cases[rg_index];
    
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Since we purposefully injecting errors, only trace those sectors that 
     * result `critical' (i.e. for instance error injection mis-matches) errors.
     */
    fbe_test_sep_util_reduce_sector_trace_level();

    /* Disable all the records.
     */
    status = fbe_api_logical_error_injection_disable_records(0, 128);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Add our own records.
     */
    status = fbe_api_logical_error_injection_create_record(&error_case_p->record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable injection on the raid group specified
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Enable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have no records, but enabled objects.
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    return;
}

/*!**************************************************************
 * cleo_start_io()
 ****************************************************************
 * @brief
 *  Start the io asynchronously. I am using asynch io since I 
 *  am just injecting errors. The io will inject error and 
 *  bubble up to RAID library waiting for the RAID object.
 *  Since I am not pulling any drives we will not get event from RAID
 *  Once we stop the io the io will get completed
 *
 * @param rg_index - raid group index to run I/O for
 * @param in_current_rg_config_p -  Pointer to the raid group config info              
 *
 * @return None.
 *
 * @author
 *  10/03/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

static void cleo_start_io(fbe_u32_t rg_index, fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_api_rdgen_context_t *               context_p = &fbe_cleo_test_context_g[rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    fbe_status_t                            status;
    cleo_error_case_t      *                error_case_p = &cleo_error_cases[rg_index];
    fbe_test_logical_unit_configuration_t * logical_unit_configuration_p = NULL;

    /* Send I/O to each LUN in this raid group
     */
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < CLEO_LUNS_PER_RAID_GROUP; lun_index++)
    {

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        /* Send a single I/O to the specified LU object using the error
         * injection information for this raid group.
         */
        status = fbe_api_rdgen_send_one_io_asynch(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_PACKAGE_ID_SEP_0,
                                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                  FBE_RDGEN_PATTERN_LBA_PASS,
                                                  error_case_p->lba_to_read,
                                                  error_case_p->blocks_to_read,
                                                  FBE_RDGEN_OPTIONS_INVALID);

        /* Make sure there were no errors.
        */
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Goto next LUN.
         */
        context_p++;
        logical_unit_configuration_p++;
    }
    return;

}
/******************************************
 * end cleo_start_io()
 ******************************************/


/*!**************************************************************
 * cleo_start_verify_write_io()
 ****************************************************************
 * @brief
 *  Run I/O.
 *  Stop I/O.
 *
 * @param rg_index - raid group index to run I/O for
 * @param in_current_rg_config_p -  Pointer to the raid group config info 
 *
 * @return None.
 *
 *
 ****************************************************************/

static void cleo_start_verify_write_io(fbe_u32_t rg_index,
                                       fbe_test_rg_configuration_t* in_current_rg_config_p)
{
    fbe_status_t                            status;
    fbe_api_rdgen_context_t *               context_p = &fbe_cleo_test_context_g[rg_index][0];
    fbe_u32_t                               lun_index;
    fbe_object_id_t                         lun_object_id;
    cleo_error_case_t      *                error_case_p = &cleo_error_cases[rg_index];
    fbe_test_logical_unit_configuration_t * logical_unit_configuration_p = NULL;

    /* For each LUN in the raid group send exactly (1) VERIFY_WRITE request.
     */
    logical_unit_configuration_p = in_current_rg_config_p->logical_unit_configuration_list;
    for (lun_index = 0; lun_index < CLEO_LUNS_PER_RAID_GROUP; lun_index++)
    {

        fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

        /* Send a single I/O to the specified LU object using the error
         * injection information for this raid group.  Notice that the I/O
         * is synchronous (i.e. we wait for the completion)
         */
        status = fbe_api_rdgen_send_one_io(context_p,
                                           lun_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,
                                           FBE_RDGEN_PATTERN_LBA_PASS,
                                           error_case_p->lba_to_read,
                                           error_case_p->blocks_to_read,
                                           FBE_RDGEN_OPTIONS_INVALID,
                                           0, 0, /* no expiration or abort time */
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Make sure that there was a correctable coherency error
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate correctable coherency errors ==\n", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(context_p->status, FBE_STATUS_OK);

        // Check the packet status, block status and block qualifier
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        mut_printf(MUT_LOG_TEST_STATUS, "counts of errors: c_coh: %d u_coh: %d ",
                   context_p->start_io.statistics.verify_errors.c_coh_count,
                   context_p->start_io.statistics.verify_errors.u_coh_count);
        /* Narrow widths only ever do MR3s so they will not see the errors.
         * Only parity wide widths should get errors since they are doing a 468 write, 
         * mirrors will not get errors since we are writing over the error.
         */
        if ((in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) &&
            (in_current_rg_config_p->width > 3))
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_coh_count, 1);
        }
        else if ((in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) &&
                 (in_current_rg_config_p->width > 4))
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_coh_count +
                                 context_p->start_io.statistics.verify_errors.u_coh_count, 1);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_coh_count, 0);
        }
        /* Send a single I/O to the specified LU object using the error
         * injection information for this raid group.  Notice that the I/O
         * is synchronous (i.e. we wait for the completion)
         */
        status = fbe_api_rdgen_send_one_io(context_p,
                                           lun_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,
                                           FBE_RDGEN_PATTERN_LBA_PASS,
                                           error_case_p->lba_to_read,
                                           error_case_p->blocks_to_read,
                                           FBE_RDGEN_OPTIONS_INVALID,
                                           0, 0, /* no expiration or abort time */
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // Make sure that there were no errors
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate no errors ==\n", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(context_p->status, FBE_STATUS_OK);

        // Check the packet status, block status and block qualifier
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_coh_count, 0);

        /* Goto next LUN.
         */
        context_p++;
        logical_unit_configuration_p++;
    }

    return;
}
/******************************************
 * end cleo_start_verify_write_io()
 ******************************************/

/*!**************************************************************
 * cleo_wait_for_error_injection()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete. Look at the 
 *  logical error injection service stats to see 
 *  whether errors has been injected into the drive
 *  for all the luns
 *
 * @param   rg_index - raid group index to inject error for               
 *
 * @return None.
 *
 * @note    At this point there are I/Os waiting for the raid group
 *          object to acknowledge the newly discovered dead position.
 *          This is due to the fact that the write request was timed
 *          out which is treated like a failed I/O, which in turn is
 *          treated like a dead error.  But the actual state of the
 *          edge hasn't changed so the raid group monitor will never
 *          continue the I/O.
 *
 * @author
 *  10/03/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static void cleo_wait_for_error_injection(fbe_u32_t rg_index,
                                          fbe_test_rg_configuration_t * in_current_rg_config_p)
{
    fbe_status_t    status;
    fbe_u32_t       sleep_count;
    fbe_u32_t       sleep_period_ms = 100;
    fbe_u32_t       max_sleep_count = ((CLEO_MAX_ERROR_INJECT_WAIT_TIME_SECS * 1000) / sleep_period_ms);
    fbe_api_logical_error_injection_get_stats_t stats; 
    fbe_object_id_t rg_object_id;
    UINT_32         ios_outstanding = 0;

    FBE_UNREFERENCED_PARAMETER(rg_index);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);

    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);    

    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        // This is for ARS 378851. 
        // Before the write completes on the drive we pull the drives
        // causing the cleo to fail intermittently. 
        // It does not guarantee that the write completed to the drive.    
        // This code is added to make sure that the writes to the drives gets completed
        // before we pull the drive
        fbe_test_sep_number_of_ios_outstanding(rg_object_id, &ios_outstanding);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s: Number of iosoutstanding is %d ==\n", __FUNCTION__, ios_outstanding);

        if((stats.num_errors_injected == CLEO_LUNS_PER_RAID_GROUP) && (ios_outstanding == CLEO_LUNS_PER_RAID_GROUP))
        {
            // Break out of the loop
            break;
        }
                        
        // Sleep 
        fbe_api_sleep(sleep_period_ms);
    }

    MUT_ASSERT_INT_EQUAL(ios_outstanding,CLEO_LUNS_PER_RAID_GROUP);

    //  The error injection took too long and timed out.
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    return;

} // End cleo_wait_for_error_injection()



/*!**************************************************************
 * cleo_remove_and_reinsert_drives()
 ****************************************************************
 * @brief
 *  Remove drives and reinsert drives
 *
 * @param 
 *  fbe_u32_t rg_index     - raid group index
 *  in_current_rg_config_p -  Pointer to the raid group config info 
 *
 * @return None.
 *
 *
 ****************************************************************/

static void cleo_remove_and_reinsert_drives(fbe_u32_t rg_index,
                                            fbe_test_rg_configuration_t* in_current_rg_config_p)
{

    fbe_status_t           status;
    fbe_object_id_t         rg_object_id = FBE_OBJECT_ID_INVALID;

    fbe_test_rg_configuration_t *rg_config_p = in_current_rg_config_p;

    // Remove the drives according to the raid type
    cleo_remove_drives(rg_config_p);

    /* Get first virtual drive object-id of the RAID group. */
    fbe_api_database_lookup_raid_group_by_number(in_current_rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_FAIL, 20000, FBE_PACKAGE_ID_SEP_0);

    // Wait for the IO's to complete
    cleo_wait_for_io_to_complete(rg_index);

    // Stop logical error injection.  We must do this before trying to get the raid group back.
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_class(in_current_rg_config_p->class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s disable error injection successful==", __FUNCTION__);

    /* Put the drives back in.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive ==\n", __FUNCTION__);
    fbe_test_sep_util_insert_drives(CLEO_FIRST_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting first drive successful. ==\n", __FUNCTION__);


    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive ==\n", __FUNCTION__);
    fbe_test_sep_util_insert_drives(CLEO_SECOND_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting second drive successful. ==\n", __FUNCTION__);

    if (in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting third drive ==\n", __FUNCTION__);
        fbe_test_sep_util_insert_drives(CLEO_THIRD_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Inserting third drive successful. ==\n", __FUNCTION__);
    }

    // waiting for pdo to be ready
    mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for first drive to be ready ==\n", __FUNCTION__);
    fbe_test_sep_util_wait_for_pdo_ready(CLEO_FIRST_POSITION_TO_REMOVE,rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s  waiting for first drive to be ready successful. ==\n", __FUNCTION__);

    if (in_current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s waiting for third drive to be ready ==\n", __FUNCTION__);
        fbe_test_sep_util_wait_for_pdo_ready(CLEO_THIRD_POSITION_TO_REMOVE,rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s  waiting for third drive to be ready successful. ==\n", __FUNCTION__);
    }

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==\n", __FUNCTION__);

    fbe_test_sep_util_wait_for_all_objects_ready(in_current_rg_config_p);
    
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==\n", __FUNCTION__);

    return;

}/******************************************
 * end cleo_remove_and_reinsert_drives()
 ******************************************/

/*!**************************************************************
 * cleo_remove_drives()
 ****************************************************************
 * @brief
 *  Remove drives - Depending upon the Raid type drives will be
 *  removed in different order. This is done to avoid rebuild logging.
 *  We want the RG to go broken without transitioning to degraded.
 *
 * @param
 *  in_current_rg_config_p -  Pointer to the raid group config info 
 *
 * @return None.
 *
 *
 ****************************************************************/
static void cleo_remove_drives(fbe_test_rg_configuration_t* in_rg_config_p)
{
    fbe_raid_group_type_t raid_type;

    raid_type = in_rg_config_p->raid_type;

    switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID5:        
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drives. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_FIRST_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drives successful. ==\n", __FUNCTION__);
    
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_SECOND_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive successful. ==\n", __FUNCTION__); 
            break;

        case FBE_RAID_GROUP_TYPE_RAID6:        
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drives. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_FIRST_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drives successful. ==\n", __FUNCTION__);
    
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_SECOND_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive successful. ==\n", __FUNCTION__); 

            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing third drive. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_THIRD_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing third drive successful. ==\n", __FUNCTION__); 
            break;
    
        case FBE_RAID_GROUP_TYPE_RAID1:
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_SECOND_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing second drive successful. ==\n", __FUNCTION__);
    
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drives. ==\n", __FUNCTION__);
            fbe_test_sep_util_remove_drives(CLEO_FIRST_POSITION_TO_REMOVE,in_rg_config_p);
            mut_printf(MUT_LOG_TEST_STATUS, "== %s removing first drives successful. ==\n", __FUNCTION__);
            break; 

    }
    
    return;
}/******************************************
 * end cleo_remove_drives()
 ******************************************/


/*!**************************************************************
 * cleo_wait_for_io_to_complete()
 ****************************************************************
 * @brief
 *  Wait for the IO's to complete and check the status 
 *
 * @param
 *  fbe_u32_t rg_index
 *
 * @return None.
 *
 *
 ****************************************************************/
static void cleo_wait_for_io_to_complete(fbe_u32_t rg_index)
{

    fbe_status_t status;
    fbe_u32_t   lun_index;
    fbe_api_rdgen_context_t *context_p = &fbe_cleo_test_context_g[rg_index][0];


    for(lun_index = 0; lun_index < CLEO_LUNS_PER_RAID_GROUP; lun_index++)
    {
        /* Wait for the IO's to complete before checking the status    
        */
        status = fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_NEIT, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /* Get the status from the context structure.
         * Make sure errors were encountered since 
         * RG was shutdown when IO's were outstanding 
         */
        status = fbe_api_rdgen_get_status(context_p, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 1);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        
    
        /* Destroy all the contexts (even if we got an error).
         */
        status = fbe_api_rdgen_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        context_p ++;
    }
    
            
    return;
}/******************************************
 * end cleo_wait_for_io_to_complete()
 ******************************************/
