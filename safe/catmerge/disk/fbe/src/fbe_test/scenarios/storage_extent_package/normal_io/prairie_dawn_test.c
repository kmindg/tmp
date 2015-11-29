/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file prairie_dawn_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid 10 objects.
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
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe_test_common_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * prairie_dawn_short_desc = "raid 1/0 read and write I/O test.";
char * prairie_dawn_long_desc ="\
The Prairie Dawn raid-10 scenario tests non-degraded raid-1/0 I/O.\n\
\n"\
"-raid_test_level 0 and 1\n\
     We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: Run a get-info test for the striper class for raid-10 raid groups.\n\
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
 * @def PRAIRIE_DAWN_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in each raid group.
 *
 *********************************************************************/
#define PRAIRIE_DAWN_LUNS_PER_RAID_GROUP 3 

/*!*******************************************************************
 * @def PRAIRIE_DAWN_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define PRAIRIE_DAWN_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def PRAIRIE_DAWN_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define PRAIRIE_DAWN_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def PRAIRIE_DAWN_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define PRAIRIE_DAWN_LARGE_IO_SIZE_MAX_BLOCKS PRAIRIE_DAWN_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def PRAIRIE_DAWN_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define PRAIRIE_DAWN_TEST_MAX_WIDTH FBE_RAID_MAX_DISK_ARRAY_WIDTH

/*!*******************************************************************
 * @def GROVER_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define PRAIRIE_DAWN_TEST_BLOCK_OP_MAX_BLOCK 4096

/*!*******************************************************************
 * @var prairie_dawn_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t prairie_dawn_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
    {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            1,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var prairie_dawn_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t prairie_dawn_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            3,          0},
    {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            4,          0},
    {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            5,          0},

    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            6,          1},
    {6,       0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            7,          1},
    {16,      0x2A000,     FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   520,            8,          1},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var prairie_dawn_get_info_test_cases
 *********************************************************************
 * @brief Test cases to test raid 5 get info.
 *
 *********************************************************************/
fbe_test_raid_get_info_test_case_t prairie_dawn_get_info_test_cases[] = 
{  
    /* Standard cases where we input the exported capacity. 
     * We expect success for these cases. 
     */
    { 
        2, /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0xE000, /* exported cap (input) */
        /* imported cap is the exported cap rounded to the stripe/chunk size plus metadata
         */
        0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 1),
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 1)) / 1,   
        FBE_TRUE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        1, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        4,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0x9801,   /* exported cap (input) */
        /* imported cap is the exported cap rounded to the stripe/chunk size plus metadata
         */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2)) / 2,      
        FBE_FALSE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        2,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        8,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0x9880,     /* exported cap (input) */
        /* imported cap is the exported cap rounded to the stripe/chunk size plus metadata
         */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4)) / 4,
        FBE_TRUE,   /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        4,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        16,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0x9880,     /* exported cap (input) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xC000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 8), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xC000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 8)) / 8,
        FBE_TRUE,   /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        8,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },

    /* Cases where single drive capacity is input.
     */
    { 
        2, /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0xE000, /* exported cap (output) */
        0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 1), /* imported cap (exported cap plus metadata) */
        /* single drive capacity (input)
         * This is the imported cap divided by data disks.
         */
        (0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 1)) / 1,   
        FBE_TRUE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        1, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        4,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0xA000,   /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 1 to make sure we know how to round the single drive capacity down to a
         * multiple of the chunk size. 
         */
        ((0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 2)) / 2) + 1,      
        FBE_FALSE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        2,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        8,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0xA000,   /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 0x380 to make sure we know how to round the single drive capacity down 
         * to a multiple of the chunk size. 
         */
        ((0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 4)) / 4) + 0x380,
        FBE_FALSE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        4,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        16,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
        0xC000,     /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xC000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 8), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 0x100 to make sure we know how to round the single drive capacity down
         * to a multiple of the chunk size.
         */
        ((0xC000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 8)) / 8) + 0x100,
        FBE_TRUE,   /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        8,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity not used for striper */
        FBE_STATUS_OK,                          /* expected status */
    },

    /* Invalid width case.
     */
    { 
        17,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
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
        14,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
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
        14,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
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
        1,      /* Width */
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
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
        FBE_RAID_GROUP_TYPE_RAID10,
        FBE_CLASS_ID_STRIPER,
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
 * prairie_dawn_test_get_info()
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

void prairie_dawn_test_get_info(void)
{
    fbe_test_raid_class_get_info_cases(&prairie_dawn_get_info_test_cases[0]);
    return;
}
/******************************************
 * end prairie_dawn_test_get_info()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid6 I/O test.
 *
 * @param rg_config_p - config to run test against.
 *
 * @return None.
 *
 * @author
 *  2/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

void prairie_dawn_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_bool_t b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();

    fbe_test_raid_group_get_info(rg_config_p);

    /* Run I/O to this set of raid groups with the I/O sizes specified
     * (no aborts and no peer I/O)
     */
    if (!b_abort_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          PRAIRIE_DAWN_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          PRAIRIE_DAWN_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,    /* Don't inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Now run random I/Os and periodically abort some outstanding requests.
     */
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                      PRAIRIE_DAWN_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      PRAIRIE_DAWN_LARGE_IO_SIZE_MAX_BLOCKS,
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
                                          PRAIRIE_DAWN_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          PRAIRIE_DAWN_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,    /* Don't Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);

        fbe_test_sep_util_validate_no_raid_errors_both_sps();
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end prairie_dawn_test_rg_config()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_test_rg_config_block_ops()
 ****************************************************************
 * @brief
 *  Run a raid 10 block operations test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void prairie_dawn_test_rg_config_block_ops(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
                                      PRAIRIE_DAWN_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      PRAIRIE_DAWN_TEST_BLOCK_OP_MAX_BLOCK,
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
 * end prairie_dawn_test_rg_config_block_ops()
 ******************************************/
/*!**************************************************************
 * prairie_dawn_test()
 ****************************************************************
 * @brief
 *  Run a raid 10 I/O test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void prairie_dawn_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &prairie_dawn_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &prairie_dawn_raid_group_config_qual[0];
    }
    prairie_dawn_test_get_info();

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, prairie_dawn_test_rg_config,
                                   PRAIRIE_DAWN_LUNS_PER_RAID_GROUP,
                                   PRAIRIE_DAWN_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end prairie_dawn_test()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_block_opcode_test()
 ****************************************************************
 * @brief
 *  Run a raid 10 I/O test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void prairie_dawn_block_opcode_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0) {
        rg_config_p = &prairie_dawn_raid_group_config_extended[0];
    }
    else {
        rg_config_p = &prairie_dawn_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, prairie_dawn_test_rg_config_block_ops,
                                   PRAIRIE_DAWN_LUNS_PER_RAID_GROUP,
                                   PRAIRIE_DAWN_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end prairie_dawn_block_opcode_test()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 10 objects on both SPs.
 *
 * @param  None.
 *
 * @return None.
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *  
 ****************************************************************/

void prairie_dawn_dualsp_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &prairie_dawn_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &prairie_dawn_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, prairie_dawn_test_rg_config,
                                   PRAIRIE_DAWN_LUNS_PER_RAID_GROUP,
                                   PRAIRIE_DAWN_CHUNKS_PER_LUN);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end prairie_dawn_dualsp_test()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 0 I/O test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void prairie_dawn_setup(void)
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
            rg_config_p = &prairie_dawn_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &prairie_dawn_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, 
                         PRAIRIE_DAWN_LUNS_PER_RAID_GROUP, 
                         PRAIRIE_DAWN_CHUNKS_PER_LUN);
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
/******************************************
 * end prairie_dawn_setup()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 10 I/O test to run on both sps.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *
 ****************************************************************/
void prairie_dawn_dualsp_setup(void)
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
            rg_config_p = &prairie_dawn_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &prairie_dawn_raid_group_config_qual[0];
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

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    /*! @todo Currently we reduce the sector trace error level to critical
     *        to prevent dumping errored secoter tracing logs while running
     *        the corrupt crc and corrupt data tests. We will remove this code 
     *        once fix the issue of unexpected errored sector tracing logs.
     */
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_reduce_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}
/**************************************
 * end prairie_dawn_dualsp_setup()
 **************************************/

/*!**************************************************************
 * prairie_dawn_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the prairie_dawn test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void prairie_dawn_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end prairie_dawn_cleanup()
 ******************************************/

/*!**************************************************************
 * prairie_dawn_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the prairie_dawn test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

void prairie_dawn_dualsp_cleanup(void)
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
 * end prairie_dawn_dualsp_cleanup()
 ******************************************/
/*************************
 * end file prairie_dawn_test.c
 *************************/


