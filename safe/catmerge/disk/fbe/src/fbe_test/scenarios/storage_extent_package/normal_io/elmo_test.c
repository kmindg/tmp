/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file elmo_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an I/O test for raid 0 objects.
 *
 * @version
 *   7/14/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe_database.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * elmo_short_desc = "Raid 0 normal I/O and Raid 0 I/O with raid group shutdowns.";
char * elmo_long_desc = "\
The Elmo scenario tests raid 0 I/O and raid 0 I/O with shutdown raid groups.\n\
\n\
\n"\
"-raid_test_level 0 and 1\n\
   - We test additional combinations of raid group widths and element sizes at -rtl 1.\n\
\n\
STEP 1: Run a get-info test for the striper class for Raid-0.\n\
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
"STEP 9: Shutdown test.\n\
         - Run write/read/compare I/O to the raid group.\n\
         - Pull 1 drive to shutdown each raid group.\n\
         - Wait for all the raid groups to goto lifecycle state FAIL.\n\
         - Stop I/O.\n\
         - Insert the drives.\n\
         - Wait for the raid groups/luns to come ready.\n\
\n\
STEP 10: Destroy raid groups and LUNs.\n\
\n\
Test Specific Switches:\n\
          -abort_cases_only - Only run the abort tests.\n\
          -shutdown_only    - Only run the shutdown tests.\n\
\n\
Description last updated: 10/04/2011.\n";

/*!*******************************************************************
 * @def ELMO_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs per raid group.
 *
 *********************************************************************/
#define ELMO_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def ELMO_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define ELMO_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def ELMO_SMALL_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ELMO_SMALL_IO_SIZE_MAX_BLOCKS 1024

/*!*******************************************************************
 * @def ELMO_LARGE_IO_SIZE_MAX_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define ELMO_LARGE_IO_SIZE_MAX_BLOCKS ELMO_TEST_MAX_WIDTH * FBE_RAID_MAX_BE_XFER_SIZE

/*!*******************************************************************
 * @def ELMO_TEST_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define ELMO_TEST_MAX_WIDTH 16


/*!*******************************************************************
 * @def ELMO_TEST_BLOCK_OP_MAX_BLOCK
 *********************************************************************
 * @brief Max number of blocks for block operation.
 *********************************************************************/
#define ELMO_TEST_BLOCK_OP_MAX_BLOCK 4096

/*!*******************************************************************
 * @var elmo_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations for qual (level 0).
 *
 *********************************************************************/
fbe_test_rg_configuration_t elmo_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            0,          0},
    {5,       0xF000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var elmo_raid_group_config_extended
 *********************************************************************
 * @brief Configuration for extended testing (level 1 and greater).
 *
 *********************************************************************/
fbe_test_rg_configuration_t elmo_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/

    {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            4,          0},
    {3,       0x9000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            5,          0},
    {5,       0xF000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            6,          0},
    {16,      0xF000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            7,          0},

    {1,       0x8000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            8,          1},
    {3,       0x9000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,            9,          1},
    {5,       0xF000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,           10,          1},
    {16,      0xF000,     FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,           11,          1},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

/*!**************************************************************
 * elmo_set_trace_level()
 ****************************************************************
 * @brief
 *  Return the number of I/Os to perform during this test.
 *
 * @param  level - the new default trace level to set.             
 *
 * @return - None.
 *
 ****************************************************************/

void elmo_set_trace_level(fbe_trace_level_t level)
{
    fbe_test_sep_util_set_trace_level(level, FBE_PACKAGE_ID_SEP_0);
    return;
}
/******************************************
 * end elmo_set_trace_level()
 ******************************************/
/*!*******************************************************************
 * @var elmo_get_info_test_cases
 *********************************************************************
 * @brief Test cases to test raid 0 get info.
 *
 *********************************************************************/
fbe_test_raid_get_info_test_case_t elmo_get_info_test_cases[] = 
{  
    { 
        1, /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0xE000, /* exported cap (input) */
        /* imported cap is the exported cap rounded to the stripe/chunk size plus metadata
         */
        0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3),
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3)) / 1,   
        FBE_TRUE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        1, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        3, /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0xE000, /* exported cap (input) */
        /* imported cap is the exported cap rounded to the stripe/chunk size plus metadata
         */
        0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3 * 3), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3 * 3)) / 3,   
        FBE_TRUE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        3, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        5,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0x9801,   /* exported cap (input) */
        /* imported cap is the exported cap rounded to the stripe/chunk size plus metadata
         */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 5 * 5), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 5 * 5)) / 5,      
        FBE_FALSE, /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        5,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        16,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0x9880,     /* exported cap (input) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0x10000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 16 * 16), 
        /* single drive capacity (output)
         * This is the imported cap divided by data disks.  
         */
        (0x10000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 16 * 16)) / 16,
        FBE_TRUE,   /* b_bandwidth */
        FBE_TRUE, /* b_input_exported_capacity */
        16,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },

    /* Cases where single drive capacity is input.
     */
    { 
        1, /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0xE000, /* exported cap (output) */
        0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3), /* imported cap (exported cap plus metadata) */
        /* single drive capacity (input)
         * This is the imported cap divided by data disks.
         */
        (0xE000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3)) / 1,   
        FBE_TRUE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        1, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        3, /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0xF000, /* exported cap (output) */
        0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3 * 3), /* imported cap (exported cap plus metadata) */
        /* single drive capacity (input)
         * This is the imported cap divided by data disks.
         */
        (0xF000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 3 * 3)) / 3,   
        FBE_TRUE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        3, /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        5,        /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0xA000,   /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 5 * 5), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 0x100 to make sure we know how to round the single drive capacity down
         * to a multiple of the chunk size.
         */
        ((0xA000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 5 * 5)) / 5) + 1,      
        FBE_FALSE, /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        5,        /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },
    { 
        16,         /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
        FBE_CLASS_ID_STRIPER,
        0x10000,     /* exported cap (output) */
        /* imported cap (exported cap rounded to a chunk plus metadata) */
        0x10000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 16 * 16), 
        /* single drive capacity (input) This is the imported cap divided by data disks.
         * We add 0x100 to make sure we know how to round the single drive capacity down
         * to a multiple of the chunk size.
         */
        ((0x10000 + (FBE_RAID_DEFAULT_CHUNK_SIZE * 16 * 16)) / 16) + 0x100,
        FBE_TRUE,   /* b_bandwidth */
        FBE_FALSE, /* b_input_exported_capacity */
        16,         /* data disks */
        FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH, /* element size */
        0, /* elements per parity*/
        FBE_STATUS_OK,                          /* expected status */
    },

    /* Invalid width case.
     */
    { 
        17,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
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
        15,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
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
        15,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
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
        0,       /* Width */
        FBE_RAID_GROUP_TYPE_RAID0,
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
 * elmo_test_get_info()
 ****************************************************************
 * @brief
 *  Test the get info ioctl with a few test cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_test_get_info(void)
{
    fbe_test_raid_class_get_info_cases(&elmo_get_info_test_cases[0]);
    return;
}
/******************************************
 * end elmo_test_get_info()
 ******************************************/


/*!**************************************************************
 * elmo_create_raid_groups()
 ****************************************************************
 * @brief
 *  Create a configuration given a set of parameters.
 *
 * @param rg_config_p - Config to create.
 * @param luns_per_rg - 
 * @param chunks_per_lun -                
 *
 * @return None.
 *
 * @author
 *  8/24/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_create_raid_groups(fbe_test_rg_configuration_t *rg_config_p,
                             fbe_u32_t luns_per_rg,
                             fbe_u32_t chunks_per_lun)
{
    fbe_u32_t  raid_group_count = 0;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t debug_flags;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, raid_group_count,
                                                      chunks_per_lun, 
                                                      luns_per_rg);

    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);

    if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &debug_flags))
    {
        fbe_test_sep_util_set_rg_library_debug_flags_both_sps(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
    }
    return;
}
/******************************************
 * end elmo_create_raid_groups()
 ******************************************/

/*!**************************************************************
 * elmo_load_sep_and_neit()
 ****************************************************************
 * @brief
 *  Create a configuration given a set of parameters.
 *
 * @param none.             
 *
 * @return None.
 *
 * @author
 *  8/24/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_load_sep_and_neit(void)
{
    fbe_sep_package_load_params_t sep_params; 
    fbe_neit_package_load_params_t neit_params; 

    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    sep_config_load_sep_and_neit_params(&sep_params, &neit_params);

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_io_flags();
    return;
}
/******************************************
 * end elmo_load_sep_and_neit()
 ******************************************/

/*!**************************************************************
 * elmo_load_esp_sep_and_neit()
 ****************************************************************
 * @brief
 *  Create a configuration given a set of parameters.
 *
 * @param none.             
 *
 * @return None.
 *
 * @author
 *  8/24/2010 - Created. Saranyadevi Ganesan.
 *              Ref. elmo_load_sep_and_neit function.
 *
 ****************************************************************/

void elmo_load_esp_sep_and_neit(void)
{
    fbe_sep_package_load_params_t sep_params; 
    fbe_neit_package_load_params_t neit_params; 

    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    sep_config_load_esp_sep_and_neit_params(&sep_params, &neit_params);

    /* Setup some default I/O flags, which includes checking to see if they were 
     * included on the command line. 
     */
    fbe_test_sep_util_set_default_io_flags();
    return;
}
/******************************************
 * end elmo_load_sep_and_neit()
 ******************************************/

/*!**************************************************************
 * elmo_load_config()
 ****************************************************************
 * @brief
 *  Create a configuration given a set of parameters.
 *
 * @param rg_config_p - Config to create.
 * @param luns_per_rg - 
 * @param chunks_per_lun -                
 *
 * @return None.
 *
 * @author
 *  9/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_load_config(fbe_test_rg_configuration_t *rg_config_p,
                      fbe_u32_t luns_per_rg,
                      fbe_u32_t chunks_per_lun)
{
    fbe_u32_t  raid_group_count = 0;
    const fbe_char_t *raid_type_string_p = NULL;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

    elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);
    elmo_load_sep_and_neit();
    return;
}
/******************************************
 * end elmo_load_config()
 ******************************************/

/*!**************************************************************
 * elmo_create_config()
 ****************************************************************
 * @brief
 *  Create a configuration given a set of parameters.
 *
 * @param rg_config_p - Config to create.
 * @param luns_per_rg - 
 * @param chunks_per_lun -                
 *
 * @return None.
 *
 * @author
 *  7/27/2010 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_create_config(fbe_test_rg_configuration_t *rg_config_p,
                        fbe_u32_t luns_per_rg,
                        fbe_u32_t chunks_per_lun)
{
    fbe_u32_t  raid_group_count = 0;
    const fbe_char_t *raid_type_string_p = NULL;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

    elmo_create_physical_config_for_rg(rg_config_p, raid_group_count);
    elmo_load_sep_and_neit();
    elmo_create_raid_groups(rg_config_p, luns_per_rg, chunks_per_lun);
    return;
}
/******************************************
 * end elmo_create_config()
 ******************************************/

/*!**************************************************************
 * elmo_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid 0 I/O test.
 *
 * @param   rg_config_p               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_bool_t b_abort_cases = fbe_test_sep_util_get_cmd_option("-abort_cases_only");
    fbe_bool_t b_shutdown_cases = fbe_test_sep_util_get_cmd_option("-shutdown_only");
    fbe_bool_t b_normal_only = fbe_test_sep_util_get_cmd_option("-normal_io_only");
    fbe_u32_t raid_group_count = 0;
    fbe_bool_t b_run_dualsp_io = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_api_rdgen_peer_options_t peer_options = FBE_RDGEN_PEER_OPTIONS_INVALID;
    void big_bird_degraded_shutdown_test(fbe_test_rg_configuration_t * const rg_config_p,
                                         fbe_u32_t raid_group_count,
                                         fbe_u32_t luns_per_rg,
                                         fbe_api_rdgen_peer_options_t peer_options);

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    fbe_test_raid_group_get_info(rg_config_p);

    /* Run I/O to this set of raid groups w/o aborts and peer I/O if enabled
     */
    if (!b_abort_cases && !b_shutdown_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_PREFILL,
                                          ELMO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ELMO_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,                             /* Don't inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Now run random I/Os and periodically abort some outstanding requests.
     */
    if (!b_shutdown_cases && !b_normal_only)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_STANDARD_WITH_ABORTS,
                                          ELMO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ELMO_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_TRUE,                             /* Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }

    /* Now run caterpillar I/Os and periodically abort some outstanding requests.
     */ 
    if (!b_abort_cases && !b_shutdown_cases)
    {
        fbe_test_sep_io_run_test_sequence(rg_config_p,
                                          FBE_TEST_SEP_IO_SEQUENCE_TYPE_CATERPILLAR,
                                          ELMO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                          ELMO_LARGE_IO_SIZE_MAX_BLOCKS,
                                          FBE_FALSE,    /* Inject aborts */
                                          FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                          b_run_dualsp_io,
                                          FBE_TEST_SEP_IO_DUAL_SP_MODE_LOAD_BALANCE);
    }
    /* We do not expect any errors.
     */
    fbe_test_sep_util_validate_no_raid_errors_both_sps();

    if (!b_abort_cases && !b_normal_only)
    {
        if (b_run_dualsp_io)
        {
            fbe_u32_t random_num = fbe_random() % 2;
            if (random_num == 0)
            {
                peer_options = FBE_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
            }
            else
            {
                peer_options = FBE_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
            }
        }
        /*! @todo for now we will only run the shutdown test in simulation  
         *        the way we power off drives on hardware needs to be looked at. 
         */
        if (fbe_test_util_is_simulation())
        {
            /* Run a shutdown test.
             */
            big_bird_degraded_shutdown_test(rg_config_p, 
                                            raid_group_count, 
                                            ELMO_TEST_LUNS_PER_RAID_GROUP,
                                            peer_options);
        }
    }

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    return;
}
/******************************************
 * end elmo_test_rg_config()
 ******************************************/

/*!**************************************************************
 * elmo_test_rg_config_block_ops()
 ****************************************************************
 * @brief
 *  Run a raid 0 block operations test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_test_rg_config_block_ops(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_sep_io_run_test_sequence(rg_config_p,
                                      FBE_TEST_SEP_IO_SEQUENCE_TYPE_TEST_BLOCK_OPERATIONS,
                                      ELMO_SMALL_IO_SIZE_MAX_BLOCKS, 
                                      ELMO_TEST_BLOCK_OP_MAX_BLOCK,
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
 * end elmo_test_rg_config_block_ops()
 ******************************************/
/*!**************************************************************
 * elmo_test()
 ****************************************************************
 * @brief
 *  Run a raid 0 I/O test on a single SP.
 *
 * @param  None             
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Test get-info for raid 0 class.
     */
    elmo_test_get_info();

    if (test_level > 0)
    {
        rg_config_p = &elmo_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &elmo_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Now create the raid groups and run the test
     */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, elmo_test_rg_config,
                                   ELMO_TEST_LUNS_PER_RAID_GROUP,
                                   ELMO_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end elmo_test()
 ******************************************/

/*!**************************************************************
 * elmo_block_opcode_test()
 ****************************************************************
 * @brief
 *  Run a raid 0 I/O test on a single SP.
 *
 * @param  None             
 *
 * @return None.   
 *
 * @author
 *  3/28/2014 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_block_opcode_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &elmo_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &elmo_raid_group_config_qual[0];
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Now create the raid groups and run the test
     */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, elmo_test_rg_config_block_ops,
                                   ELMO_TEST_LUNS_PER_RAID_GROUP,
                                   ELMO_CHUNKS_PER_LUN);

    return;
}
/******************************************
 * end elmo_block_opcode_test()
 ******************************************/

/*!**************************************************************
 * elmo_dualsp_test()
 ****************************************************************
 * @brief
 *  Run a raid 0 I/O test on both SPs.
 *
 * @param  None             
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_dualsp_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Test get-info for raid 0 class.
     */
    elmo_test_get_info();

    if (test_level > 0)
    {
        rg_config_p = &elmo_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &elmo_raid_group_config_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Now create the raid groups and run the tests
     */
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, elmo_test_rg_config,
                                   ELMO_TEST_LUNS_PER_RAID_GROUP,
                                   ELMO_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end elmo_dualsp_test()
 ******************************************/

/*!**************************************************************
 * elmo_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 0 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @note    Only single-sp mode is supported
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void elmo_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;  

    if (fbe_test_util_is_simulation())
    { 
        if (test_level > 0)
        {
            rg_config_p = &elmo_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &elmo_raid_group_config_qual[0];
        }

        /* We no longer create the raid groups during the setup
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_random_block_sizes(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p,
                         ELMO_TEST_LUNS_PER_RAID_GROUP,
                         ELMO_CHUNKS_PER_LUN);
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/******************************************
 * end elmo_setup()
 ******************************************/

/*!**************************************************************
 * elmo_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 0 I/O test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void elmo_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;

        if (test_level > 0)
        {
            rg_config_p = &elmo_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &elmo_raid_group_config_qual[0];
        }

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

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/******************************************
 * end elmo_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * elmo_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the elmo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_cleanup(void)
{
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
 * end elmo_cleanup()
 ******************************************/

/*!**************************************************************
 * elmo_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the elmo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void elmo_dualsp_cleanup(void)
{
	fbe_test_sep_util_print_dps_statistics();

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
 * end elmo_dualsp_cleanup()
 ******************************************/

/*************************
 * end file elmo_test.c
 *************************/


