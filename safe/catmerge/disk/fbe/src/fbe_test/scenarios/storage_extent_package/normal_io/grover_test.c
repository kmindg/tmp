/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file grover_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid 5 objects.
 *
 * @version
 *   7/14/2009:  Created. rfoley
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
#include "fbe/fbe_api_sim_server.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * grover_short_desc = "This scenario will drive I/O from rdgen to raid 5 luns.";
char * grover_long_desc ="\
The Grover scenario tests raid-5 non-degraded I/O.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: Run a get-info test for the parity class for raid-5 raid groups.\n\
        - The purpose of this test is to validate the functionality of the\n\
          get-info usurper.  This usurper is used when we create a raid group to\n\
          validate the parameters of raid group creation such as width, element size, etc.\n\
        - Issue get info usurper to the striper class and validate various\n\
          test cases including normal and error cases.\n\
\n\
STEP 2: configure all raid groups and LUNs.\n\
        - We always test at least one rg with 512 block size drives.\n\
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
Description last updated: 10/03/2011.\n";

/*!*******************************************************************
 * @def GROVER_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define GROVER_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def GROVER_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define GROVER_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def GROVER_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define GROVER_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def GROVER_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define GROVER_TEST_BLOCK_OP_MAX_BLOCK 4096

/*!*******************************************************************
 * @def GROVER_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define GROVER_LARGE_IO_SIZE_MAX_BLOCKS GROVER_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def GROVER_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define GROVER_TEST_MAX_WIDTH 16

/*!*******************************************************************
 * @var grover_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t grover_raid_group_config_qual[] = 
{
    /* width,   capacity  raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},
    {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0},

    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,   520,  2,  FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/*!*******************************************************************
 * @var grover_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t grover_raid_group_config_extended[] = 
{
    /* width, capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0},
    {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},

    {3,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            3,          1},
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,          1},
    {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            5,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var grover_get_info_test_cases
 *********************************************************************
 * @brief Test cases to test raid 5 get info.
 *
 *********************************************************************/
fbe_test_raid_get_info_test_case_t grover_get_info_test_cases[] = 
{  
    { 
        3, /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0xE000, /* exported cap (input) */
        0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2), /* imported cap (exported cap plus metadata) */
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2)) / 2,   
        FBE_TRUE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        2, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        5,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x9801,   /* exported cap (input) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4)) / 4,      
        FBE_FALSE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        4,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        FBE_RAID_ELEMENTS_PER_PARITY, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        16,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x9880,     /* exported cap (input) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 15), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 15)) / 15,
        FBE_TRUE,   /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        15,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },

    /* Cases where single drive capacity is input.
     */
    { 
        3, /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0xE000, /* exported cap (output) */
        0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2), /* imported cap (exported cap plus metadata) */
        /* single drive capacity (input)
         * This is the imported cap divided by data disks.
         */
        (0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2)) / 2,   
        FBE_TRUE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        2, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        5,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0xA000,   /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 0x100 to make sure we know how to round the single drive capacity down
         * to a multiple of the chunk size.
         */
        ((0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4)) / 4) + 1,      
        FBE_FALSE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        4,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        FBE_RAID_ELEMENTS_PER_PARITY, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        16,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0xF000,     /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 15), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 0x100 to make sure we know how to round the single drive capacity down
         * to a multiple of the chunk size.
         */
        ((0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 15)) / 15) + 0x100,
        FBE_TRUE,   /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        15,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },

    /* Invalid width case.
     */
    { 
        17,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x0,   /* exported cap (input) */
        0x0,      /* imported cap */
        0x0,      /* single drive capacity (output) */
        FBE_FALSE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        0,       /* data disks */
        0x0, /* element size */
        0x0, /* elements per parity*/
        FBE_STATUS_GENERIC_FAILURE,             /* expected status */
    },
    /* invalid imported capacity (too small)
     */
    { 
        15,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x0,   /* exported cap */
        0x0,      /* imported cap */
        0x80,      /* single drive capacity (input) */
        FBE_FALSE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        0,       /* data disks */
        0x0, /* element size */
        0x0, /* elements per parity*/
        FBE_STATUS_GENERIC_FAILURE,             /* expected status */
    },
    /* invalid imported capacity (too small)
     */
    { 
        15,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x0,   /* exported cap */
        0x0,      /* imported cap */
        0x80,      /* single drive capacity (input) */
        FBE_FALSE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        0,       /* data disks */
        0x0, /* element size */
        0x0, /* elements per parity*/
        FBE_STATUS_GENERIC_FAILURE,             /* expected status */
    },
    /* invalid width (too small)
     */
    { 
        2,      /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x0,   /* exported cap */
        0x0,      /* imported cap */
        0x80,      /* single drive capacity (input) */
        FBE_FALSE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        0,       /* data disks */
        0x0, /* element size */
        0x0, /* elements per parity*/
        FBE_STATUS_GENERIC_FAILURE,             /* expected status */
    },
    /* invalid width (too small)
     */
    { 
        0,      /* Width */
        FBE_RAID_GROUP_TYPE_RAID5,
        FBE_CLASS_ID_PARITY,
        0x0,   /* exported cap */
        0x0,      /* imported cap */
        0x80,      /* single drive capacity (input) */
        FBE_FALSE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        0,       /* data disks */
        0x0, /* element size */
        0x0, /* elements per parity*/
        FBE_STATUS_GENERIC_FAILURE,             /* expected status */
    },

    /* Terminator.
     */
    { 
        FBE_U32_MAX,
        FBE_U32_MAX,
    },
};

/*!**************************************************************
 * grover_test_get_info()
 ****************************************************************
 * @brief
 *  Test the get info ioctl with a few test cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void grover_test_get_info(void)
{
    fbe_test_raid_class_get_info_cases(&grover_get_info_test_cases[0]);
    return;
}
/******************************************
 * end grover_test_get_info()
 ******************************************/

/*!**************************************************************
 * fbe_test_raid_class_get_info_case()
 ****************************************************************
 * @brief
 *  Test a single test case and validate all values returned.
 *
 * @param test_case_p
 *
 * @return None.
 *
 * @author
 *  2/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_raid_class_get_info_case(fbe_test_raid_get_info_test_case_t *test_case_p)
{
    fbe_api_raid_group_class_get_info_t get_info;
    fbe_status_t status;

    /* Set all the inputs.
     */
    get_info.raid_type = test_case_p->raid_type;
    get_info.width = test_case_p->width;
    get_info.b_bandwidth = test_case_p->b_bandwidth;

    if (test_case_p->b_input_exported_capacity)
    {
        get_info.exported_capacity = test_case_p->exported_capacity;
        get_info.single_drive_capacity = FBE_LBA_INVALID;
    }
    else
    {
        get_info.single_drive_capacity = test_case_p->single_drive_capacity;
        get_info.exported_capacity = FBE_LBA_INVALID;
    }

    status = fbe_api_raid_group_class_get_info(&get_info, test_case_p->class_id);

    /* Validate the status matches the test case status.
     */
    MUT_ASSERT_INT_EQUAL(status, test_case_p->expected_status);

    /* If the status was OK.
     * Validate remainder of outputs match the expected values.
     */
    if (status == FBE_STATUS_OK)
    {
        MUT_ASSERT_INT_EQUAL(get_info.element_size, test_case_p->element_size);
        MUT_ASSERT_INT_EQUAL(get_info.elements_per_parity, test_case_p->elements_per_parity);
        MUT_ASSERT_INT_EQUAL(get_info.data_disks, test_case_p->data_disks);
#if 0
        if (get_info.imported_capacity != test_case_p->imported_capacity)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "imported cap %llx != expected imported cap %llx",
                       get_info.imported_capacity, test_case_p->imported_capacity);
            MUT_FAIL_MSG("imported cap unexpected");
        }
    
        if (test_case_p->b_input_exported_capacity)
        {
            /* Exported cap input and single drive capacity is output.
             */
            if (get_info.single_drive_capacity != test_case_p->single_drive_capacity)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "single drive cap %llx != expected single drive cap %llx",
                           get_info.single_drive_capacity, test_case_p->single_drive_capacity);
                MUT_FAIL_MSG("single drive cap unexpected");
            }
        }
        else
        {
            /* Single drive cap was input and exported capacity is output.
             */
            if (get_info.single_drive_capacity != (test_case_p->imported_capacity / test_case_p->data_disks))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "single drive cap %llx != expected single drive cap %llx",
                           get_info.single_drive_capacity, 
                           (test_case_p->imported_capacity / test_case_p->data_disks));
                MUT_FAIL_MSG("single drive cap unexpected");
            }
        }
        if (get_info.exported_capacity != test_case_p->exported_capacity)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "exported cap %llx != expected exported cap %llx",
                       get_info.exported_capacity, test_case_p->exported_capacity);
            MUT_FAIL_MSG("exported cap unexpected");
        }
#endif
    }
    return;
}
/******************************************
 * end fbe_test_raid_class_get_info_case()
 ******************************************/
/*!**************************************************************
 * fbe_test_raid_class_get_info_cases()
 ****************************************************************
 * @brief
 *  Test a single test case and validate all values returned.
 *
 * @param test_case_p
 *
 * @return None.
 *
 * @author
 *  2/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_raid_class_get_info_cases(fbe_test_raid_get_info_test_case_t *test_case_p)
{
    /* Simply iterate over all test cases until we hit a terminator.
     */
    while (test_case_p->width != FBE_U32_MAX)
    {
        fbe_test_raid_class_get_info_case(test_case_p);
        test_case_p++;
    }
    return;
}
/******************************************
 * end fbe_test_raid_class_get_info_cases()
 ******************************************/

/*!**************************************************************
 * grover_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 5 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  2/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void grover_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_bool_t b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    if (test_level == 2) {
        /* This runs the standard grover dual sp test only. 
         * use -fbe_io_seconds seconds to set the run time of the test. 
         * use -fbe_threads threads    to set the threads.
         */ 
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
                                          GROVER_SMALL_IO_SIZE_MAX_BLOCKS, /*! Tweak this to modify the max blocks.  */
                                          GROVER_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE, /* Do not Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
        /* Make sure no errors occurred.
         */
        fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
        return;
    }
    fbe_test_raid_group_get_info(rg_config_p);

    /* Run I/O to this set of raid groups with the I/O sizes specified
     * (no aborts and no peer I/O)
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          GROVER_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          GROVER_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                            /* Don't inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Now run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      GROVER_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      GROVER_LARGE_IO_SIZE_MAX_BLOCKS,
                                      FBE_TRUE,                             /* Inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC,
                                      b_run_dualsp_io,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

    /* Now run caterpillar I/Os and periodically abort some outstanding requests.
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
                                          GROVER_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          GROVER_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                         /* Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

        /* We do not expect any errors.
         */
        fbe_test_sep_util_validate_no_raid_errors_both_sps();
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end grover_test_rg_config()
 ******************************************/

/*!**************************************************************
 * grover_test_rg_config_block_ops()
 ****************************************************************
 * @brief
 *  Run a raid 5 block operations test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void grover_test_rg_config_block_ops(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
                                      GROVER_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      GROVER_TEST_BLOCK_OP_MAX_BLOCK,
                                      FBE_FALSE,    /* Don't inject aborts */
                                      FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                      FBE_FALSE,
                                      FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID); 
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end grover_test_rg_config_block_ops()
 ******************************************/

/*!**************************************************************
 * grover_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void grover_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &grover_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &grover_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, grover_test_rg_config,
                                   GROVER_TEST_LUNS_PER_RAID_GROUP,
                                   GROVER_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end grover_test()
 ******************************************/
/*!**************************************************************
 * grover_block_opcode_test()
 ****************************************************************
 * @brief
 *  Test block operations for RAID-5
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void grover_block_opcode_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &grover_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &grover_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, grover_test_rg_config_block_ops,
                                   GROVER_TEST_LUNS_PER_RAID_GROUP,
                                   GROVER_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end grover_block_opcode_test()
 ******************************************/
/*!**************************************************************
 * grover_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects on both SPs.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void grover_dualsp_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &grover_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &grover_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, grover_test_rg_config,
                                   GROVER_TEST_LUNS_PER_RAID_GROUP,
                                   GROVER_CHUNKS_PER_LUN);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end grover_dualsp_test()
 ******************************************/

/*!**************************************************************
 * grover_setup()
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
void grover_setup(void)
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
            rg_config_p = &grover_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &grover_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, 
                           GROVER_TEST_LUNS_PER_RAID_GROUP, 
                           GROVER_CHUNKS_PER_LUN);
    }

    /* Initialize any required fields 
     */
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
 * end grover_setup()
 **************************************/

/*!**************************************************************
 * grover_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test to run on both sps.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void grover_dualsp_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups;

        if (test_level > 0)
        {
            rg_config_p = &grover_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &grover_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
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
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *	      the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/**************************************
 * end grover_dualsp_setup()
 **************************************/

/*!**************************************************************
 * grover_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the grover test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void grover_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

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
 * end grover_cleanup()
 ******************************************/

/*!**************************************************************
 * grover_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the grover test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void grover_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

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
 * end grover_dualsp_cleanup()
 ******************************************/

/*************************
 * end file grover_test.c
 *************************/


